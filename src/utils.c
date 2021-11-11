#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

BYTE *allocate_memory() {
  BYTE *memory = calloc(1, MEMORY_SIZE);
  fail_if(!memory, "Failed to allocate emulated memory");
  return memory;
}

void fail_if(int cond, char *fail_message) {
  if (cond) {
    printf("ERROR: %s\n", fail_message);
    exit(EXIT_FAILURE);
  }
}

void verbose_print(char *message) {
  if (VERBOSE) {
    printf("%s", message);
  }
}

FILE *open_file(char *file_name, char *mode) {
  verbose_print("Opening given file");
  FILE *input_file = fopen(file_name, mode);
  fail_if(!input_file,
    "Failed to open file. Exiting...");
  return input_file;
}

int get_file_size(FILE *file) {
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);
  return size;
}

void load_file_to_array(BYTE *array, int size, FILE *file) {
  size_t ret_code = fread(array, sizeof(BYTE), size, file);
  fail_if(ret_code != size,
    "Something failed while loading");
}

void load_memory(FILE *input_file, int size,  BYTE *memory) {
  verbose_print("Loading memory from given file");

  fail_if(size > MEMORY_SIZE,
    "This file is too large to fit in emulated memory");

  load_file_to_array(memory, size, input_file);
  verbose_print("File loaded succesfully\n");
}

void print_binary(WORD c, int n) {
  for(int i = n-1; i >= 0; i--) {
    printf("%d", 1 & (c >> i));
  }
}

void print_memory(const BYTE *memory, int start, int end) {
  printf("Content of emulated memory:\n");
  for(int i = start; i < end; i+=4) {
    printf("M[%04x] = ", i);
    for(int j = 0; j < 4 && i + j < end; j++) {
      print_binary(memory[i + j], 8);
      printf(" ");
    }
    printf("\n");
  }
}

void print_nonzero_memory(const BYTE *memory) {
  printf("Non-zero memory:\n");
  for(int i = 0; i < MEMORY_SIZE; i+=4) {
    bool nonzero = false;
    for(int j = 0; j < 4 && i + j < MEMORY_SIZE; j++){
      nonzero |= memory[i+j];
    }
    if(!nonzero){
      continue;
    }

    printf("0x%08x: 0x", i);
    for(int j = 0; j < 4 && i + j < MEMORY_SIZE; j++) {
      printf("%02x",memory[i+j]);
    }
    printf("\n");
  }
}

WORD *allocate_register() {
  WORD *reg = (WORD *) calloc(REGISTER_N, sizeof(int));
  fail_if(!reg, "Cannot allocate memory for register");
  return reg;
}

void print_register(WORD *reg, int index) {
  char register_name[5];
  int value = (int)reg[index];
  if(index == 15) {
    sprintf(register_name, "PC  ");
  }else if(index == 16) {
    sprintf(register_name, "CPSR");
  }else if(index < 10){
    sprintf(register_name, "$%d  ", index);
  }else {
    sprintf(register_name, "$%d ", index);
  }
  printf("%s: %10d (0x%08x)\n", register_name, value, value);
}

void print_registers(WORD *reg) {
  printf("Registers:\n");
  for(int i = 0; i < REGISTER_N; i++) {
    if(i == 13 || i == 14){
      continue;
    }
    print_register(reg, i);
  }
}

void print_state(State *state) {
  print_registers(state->reg);
  print_nonzero_memory(state->memory);
}

WORD get_bits(WORD src, int start, int end) {
  int bit_mask = 0;
  for (int i = 0; i < end - start + 1; i++) {
    bit_mask += (1 << i);
  }

  src >>= start;
  return bit_mask & src;
}

WORD get_bit(WORD src, int index) {
  return get_bits(src, index, index);
}

void set_bits(WORD *src, int start, int end) {
  for (int i = start; i <= end; i++) {
    *src = *src | (1 << i);
  }
}

void set_bit(WORD *src, int index) {
  set_bits(src, index, index);
}

void clear_bit(WORD *src, int index) {
  *src = *src & ~(1 << index);
}

void set_bits_to(WORD *src, int start, int end, WORD value) {
  for(int i = 0; i <= end - start; i++) {
    if (get_bit(value, i)) {
      set_bit(src, start + i);
    } else {
      clear_bit(src, start + i);
    }
  }
}

int shift_arithmetic_right(WORD value, int shift) {
  int sign_bit = get_bit(value, 31);
  value >>= shift;
  if (sign_bit) {
    set_bits(&value, 31 - shift + 1 ,31);
  }
  return value;
}

int rotate_right(WORD value, int rot) {
  int rotated_bits = get_bits(value, 0, rot - 1);
  value >>= rot;
  return value | (rotated_bits) << (31 - (rot - 1));
}

int rotate_left(WORD value, int rot) {
  int rotated_bits = get_bits(value, 32 - rot, 31);
  value <<= rot;
  return value | rotated_bits;
}

int operand2_decode(WORD operand2, WORD *reg, bool set_c) {
  WORD shift_type = get_bits(operand2, 5, 6);
  WORD shift_value = get_bits(operand2, 7, 11);
  WORD value_to_shift = reg[get_bits(operand2, 0, 3)];
  if (shift_value == 0) {
    return value_to_shift;
  }
  switch (shift_type) {
    case 0: {
      if (set_c) {
        WORD carry = get_bit(value_to_shift, 32 - value_to_shift);
        set_bits_to(&(reg[16]), 31, 31, carry);
      }
      return value_to_shift << shift_value;
    }
    case 1: {
      return value_to_shift >> shift_value;
    }
    case 2: {
      return shift_arithmetic_right(value_to_shift, shift_value);
    }
    case 3: {
      return rotate_right(value_to_shift, shift_value);
    }
    default:{
      return 0;
    }
  }
}

int offset_decode(WORD offset, WORD *reg) {
  return operand2_decode(offset, reg, false);
}

// a line is a label if it starts with an alphabet
// and it ends with a ':' (note that the last char of line is a newline)
bool is_label(char *line){
  return isalpha(line[0]) && line[strlen(line) - 2] == ':';
}

bool is_empty(char *line) {
  for (int i = 0; line[i]; i++) {
    if (!isspace(line[i])) {
      return false;
    }
  }
  return true;
}

void write_binary_file(FILE *output_file, BYTE *output, int size){
  fwrite(output, sizeof(BYTE), size, output_file);
}

// line format: 'opcode operand1,operand2,operand3'
int tokenize(char *line, char **token){
  // tokenize opcode (first token)
  int index = 0;
  char *tok = strtok(line, " ,\n");

  // then tokenize operands (following tokens)
  while(tok != NULL){
    token[index] = tok;
    index++;
    tok = strtok(NULL, " ,\n");
  }
  return index;
}

bool contained_in(char elem, const char *list) {
  int i = 0;
  while (list[i]) {
    if (elem == list[i]) {
      return true;
    }
    i++;
  }
  return false;
}

#define MAX_LINE_LENGTH 512
char **tokenize_with(const char *str, const char *delimiters) {
  char **tokens = calloc(100, sizeof(char *));
  int cursor = 0;
  int t_num = 0;
  while (str[cursor]) {
    //remove trailing whitespace
    while(str[cursor] && isspace(str[cursor])) {
      cursor++;
    }
    char *next = calloc(MAX_LINE_LENGTH, sizeof(char));
    int i = 0;
    while (!contained_in(str[cursor], delimiters)) {
      if (!str[cursor]) {
        free(next);
        return tokens;
      }
      next[i] = str[cursor];
      i++;
      cursor++;
    }
    next[i] = str[cursor];//the delimiter
    cursor++;
    next[i + 1] = '\0';
    tokens[t_num] = next;
    t_num++;
  }
  return tokens;
}

char **tokenize_separate(char *str, char *token_delimiters, char *delimiters) {
  char **tokens = calloc(100, sizeof(char *));
  int cursor = 0;
  int t_num = 0;
  while (str[cursor]) {
    //remove trailing delimiters
    while(contained_in(str[cursor], delimiters)) {
      cursor++;
    }
    char *next = calloc(MAX_LINE_LENGTH, sizeof(char));
    int i = 0;
    while (!contained_in(str[cursor], delimiters)
           && !contained_in(str[cursor], token_delimiters)) {
      if (!str[cursor]) {
        free(next);
        return tokens;
      }
      next[i] = str[cursor];
      i++;
      cursor++;
    }
    if (i != 0) {
      next[i] = '\0';
      tokens[t_num] = next;
      t_num++;
    } else {
      free(next);
    }
    if (contained_in(str[cursor], token_delimiters)) {
      char *d_token = calloc(2, sizeof(char));
      d_token[0] = str[cursor];
      tokens[t_num] = d_token;
      t_num++;
    }
    cursor++;
  }
  return tokens;
}

int roman_char_value(char roman) {
  switch (roman) {
    case 'I': return 1;
    case 'V': return 5;
    case 'X': return 10;
    case 'L': return 50;
    default : return 0;
  }
}

int roman_to_decimal(char *roman) {
  int total = 0;
  int last = 0;
  for(int i = 0; roman[i]; i++) {
    if (last < roman_char_value(roman[i])) {
      total -= last;
    } else {
      total += last;
    }
    last = roman_char_value(roman[i]);
  }
  total += last;
  return total;
}
