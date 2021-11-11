#include "utils.h"
#include "cycle.h"
#include "instructions.h"

void fetch(State *arm_state, BYTE *buffer) {
  for(int i = 0; i < sizeof(WORD); i++) {
    buffer[i] = arm_state->memory[arm_state->reg[PC_INDEX] + i];
  }
}

void increment_pc(State *arm_state){
  arm_state->reg[PC_INDEX] += sizeof(WORD);
}

WORD decode(const BYTE *bytes) {
  WORD decoded = *((WORD *) bytes);
  if (VERBOSE) {
    printf("decode result: ");
    print_binary(decoded, 32);
    printf("\n");
  }
  return decoded;
}

instr_type clarify_instruction(WORD decoded){
  // decoded[31..0] = 0
  if(!decoded){
    return HALT;
  }

  // clarify instruction based on bit 27, 26
  WORD bits = get_bits(decoded, 26, 27);
  switch(bits) {
    // decoded[27..26] = 00
    case 0: {
      WORD bit25 = get_bits(decoded, 25, 25);
      WORD bit7 = get_bits(decoded, 7, 7);
      WORD bit4 = get_bits(decoded, 4, 4);
      if(!bit25 && bit7 && bit4) {
        return MULTIPLY;
      } else {
        return DATA_PROCESSING;
      }
    }
    // decoded[27..26] = 01
    case 1: {
      return SINGLE_DATA_TRANSFER;
    }
    // decoded[27..26] = 10
    case 2: {
      return BRANCH;
    }
    // decoded[27..26] = 11
    default: {
      return HALT;
    }
  }
}

exec_cond execute(State *arm_state, WORD decoded) {
  instr_type instruction = clarify_instruction(decoded);

  // execute instruction based on its type
  switch(instruction) {
    case DATA_PROCESSING: {
      data_processing params = decode_data_processing(decoded);
      execute_data_processing(arm_state, &params);
      return CONTINUE;
    }
    case MULTIPLY: {
      multiply params = decode_multiply(decoded);
      execute_multiply(arm_state, &params);
      return CONTINUE;
    }
    case SINGLE_DATA_TRANSFER: {
      single_data_transfer params = decode_single_data_transfer(decoded);
      execute_single_data_transfer(arm_state, &params);
      return CONTINUE;
    }
    case BRANCH: {
      branch params = decode_branch(decoded);
      return execute_branch(arm_state, &params) ? SKIP : CONTINUE;
    }
    default: {
      return STOP;
    }
  }
}

void cycle(State *arm_state) {
  BYTE *fetched = malloc(sizeof(WORD));
  WORD decoded;

  // first cycle
  fetch(arm_state, fetched);
  increment_pc(arm_state);

  // for the following cycles
  exec_cond cond = SKIP;
  while (arm_state->reg[PC_INDEX] < MEMORY_SIZE && cond != STOP) {
    if (cond == CONTINUE) {
      //execute as usual
      cond = execute(arm_state, decoded);
    } else if (cond == SKIP) {
      // skip current execution (refresh pipeline)
      cond = CONTINUE;
    }
    if (cond != STOP) {
      decoded = decode(fetched);
      fetch(arm_state, fetched);
      increment_pc(arm_state);
    }
  }
  free(fetched);
}
