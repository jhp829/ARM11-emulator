#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "symbol_table.h"
#include "encode.h"

#define MAX_LINE_LENGTH 512

int main(int argc, char **argv) {
  fail_if(argc < 3,
          "You must pass an input file name as the first argument "
          "and output binary file name as the second argument");

  FILE *input_file  = open_file(argv[1], "r");
  FILE *output_file = open_file(argv[2], "wb");

  table *sym_table = table_create();

  // first pass
  WORD address = 0;
  char line[MAX_LINE_LENGTH];
  while(fgets(line, sizeof(line), input_file)){
    if(is_label(line)){
      table_insert(sym_table, strtok(line,":"), address);
    }
    else if (!is_empty(line)){
        address += 4;
    }
  }

  // now addr represents number of words we need for all instructions
  // except for the halt instruction at the end
  WORD out_size = address;
  address = 0;

  // second pass
  BYTE *output = calloc(out_size, sizeof(BYTE));
  rewind(input_file);
  while(fgets(line, sizeof(line), input_file)) {
    if(!is_label(line) && !is_empty(line)) {
      assemble(line, sym_table, &output, &out_size, address);
        address += 4;
    }
  }

  // write encoded bytes to binary file
  write_binary_file(output_file, output, out_size);

  fclose(output_file);
  fclose(input_file);
}
