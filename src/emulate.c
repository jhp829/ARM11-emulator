#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "cycle.h"

int main(int argc, char** argv) {
  fail_if(argc < 2,
    "You must pass a file name as the first argument");

  FILE *input_file = open_file(argv[1], "rb");
  int size = get_file_size(input_file);

  BYTE *memory = allocate_memory();
  load_memory(input_file, size, memory);

  fclose(input_file);

  WORD *reg = allocate_register();

  State arm_state = {memory, reg};
  cycle(&arm_state);
  print_state(&arm_state);

  free(memory);
  free(reg);
}
