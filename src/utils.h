#ifndef UTILS
#define UTILS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MEMORY_SIZE (1u << 16u)
#define REGISTER_N 17
#define VERBOSE 0
#define MAX_OPERAND_N 5

typedef unsigned char BYTE;
typedef unsigned int WORD;

//Struct for the arm machine's state
//Consist of 2^16 memory and 17 registers
typedef struct {
  BYTE *memory;
  WORD *reg;
} State;

// allocate memory in heap for machine memory
BYTE *allocate_memory();

//If cond is true, exit the program
//Print the fail_message if VERBOSE is set in utils.h
void fail_if(int cond, char *fail_message);

void verbose_print(char *message);

//Read the file with the given filename
//Returns a file stream
FILE *open_file(char *file_name, char mode[2]);

//Returns file size in bytes
int get_file_size(FILE *file);

//Loads size bytes from the given file stream into the given array
void load_file_to_array(BYTE *array, int size, FILE *file);

//Combine all of the functions above
//Try to load file, then load the memory
void load_memory(FILE *input_file, int size,  BYTE *memory);

//Print first n bits of input c
void print_binary(WORD c, int n);

// Print the memory in a similar format to xxd -b c4 "filename"
// For testing only
void print_memory(const BYTE *memory, int start, int end);

//Print the non-zero words in memory
//The format is "memory_address : value" both in hexadecimal
void print_nonzero_memory(const BYTE *memory);

// allocate memory in heap for register
WORD *allocate_register();

// print the content of all registers in the machine
void print_registers(WORD *reg);

// print the content of register with specific index in the machine
void print_register(WORD *reg, int index);

// print the current state (content of memory and registers) of the machine
void print_state(State *arm_state);

//return the bits of src from start to end
WORD get_bits(WORD src, int start, int end);

//return the bit of src at index
WORD get_bit(WORD src, int index);

void clear_bit(WORD *src, int index);

//sets the bits of src from start to end according to
//bit 0 to (end - start) of value
void set_bits_to(WORD *src, int start, int end, WORD value);

//sets the bits of src from start to end to 1
void set_bits(WORD *src, int start, int end);

//set bit at index in src to 1
void set_bit(WORD *src, int index);

//set bit at index in src to 0
void clear_bit(WORD *src, int index);

//rotates a value to the right by a certain amount
int rotate_right(WORD value, int rot);

//rotates a value to the left by a certain amount
int rotate_left(WORD value, int rot);

//shifted register algorithm for DP
int operand2_decode(WORD operand2, WORD *reg, bool set_c);

//shifted register algorithm for SDT
int offset_decode(WORD offset, WORD *reg);

//test if a line of characters is a label
bool is_label(char *line);

//test if a line is whitespace only
bool is_empty(char *line);

//write an array of encoded instructions in bytes into a binary file
void write_binary_file(FILE *output_file, BYTE *output, int size);

//a tokenizer that parse a line into token (parse result in **token)
//first token is the opcode, following tokens are the operands
//return the number of tokens
int tokenize(char *line, char **token);

//checks if a char is in a string
bool contained_in(char elem, const char *list);

// a custom tokenizer that includes the delimiter as part of the token
// good for separating sentences delimited by punctuation marks
// to keep the punctuation as part of the sentence
char **tokenize_with(const char *str, const char *delimiters);

// yet another tokenizer that includes certain delimiters as
// a separate token, and treats other delimiters as normal
char **tokenize_separate(char *line, char *token_delimiters, char *delimiters);

int roman_char_value(char roman);

int roman_to_decimal(char *roman);

#endif
