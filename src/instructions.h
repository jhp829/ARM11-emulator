#ifndef INSTRUCTIONS
#define INSTRUCTIONS
#include "utils.h"
#include <stdbool.h>

typedef enum instruction_type {
  DATA_PROCESSING,
  MULTIPLY,
  SINGLE_DATA_TRANSFER,
  BRANCH,
  HALT
} instr_type;

typedef struct data_processing {
  WORD cond;
  WORD i;
  WORD opcode;
  WORD s;
  WORD rn;
  WORD rd;
  WORD operand2;
} data_processing;

data_processing decode_data_processing(WORD src);
WORD encode_data_processing(data_processing instr);

bool execute_data_processing(State *arm_state, data_processing *params);

typedef struct multiply {
  WORD cond;
  WORD a;
  WORD s;
  WORD rd;
  WORD rn;
  WORD rs;
  WORD rm;
} multiply;

multiply decode_multiply(WORD src);
WORD encode_multiply(multiply instr);

bool execute_multiply(State *arm_state, multiply *params);

typedef struct single_data_transfer {
  WORD cond;
  WORD i;
  WORD p;
  WORD u;
  WORD l;
  WORD rn;
  WORD rd;
  WORD offset;
} single_data_transfer;

single_data_transfer decode_single_data_transfer(WORD src);
WORD encode_single_data_transfer(single_data_transfer instr);

bool execute_single_data_transfer(State *arm_state, single_data_transfer *params);

typedef struct branch {
  WORD cond;
  WORD offset;
} branch;

branch decode_branch(WORD src);
WORD encode_branch(branch instr);

bool execute_branch(State *arm_state, branch *params);

#endif
