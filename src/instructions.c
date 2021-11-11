#include "instructions.h"
#include "utils.h"

data_processing decode_data_processing(WORD src) {
  WORD cond     = get_bits(src, 28, 31);
  WORD i        = get_bit(src, 25);
  WORD opcode   = get_bits(src, 21, 24);
  WORD s        = get_bit(src, 20);
  WORD rn       = get_bits(src, 16, 19);
  WORD rd       = get_bits(src, 12, 15);
  WORD operand2 = get_bits(src, 0, 11);
  return (data_processing) {cond, i, opcode, s, rn, rd, operand2};
}

WORD encode_data_processing(data_processing instr) {
  WORD result = 0;
  set_bits_to(&result, 28, 31, instr.cond);
  set_bits_to(&result, 25, 25, instr.i);
  set_bits_to(&result, 21, 24, instr.opcode);
  set_bits_to(&result, 20, 20, instr.s);
  set_bits_to(&result, 16, 19, instr.rn);
  set_bits_to(&result, 12, 15, instr.rd);
  set_bits_to(&result, 0, 11, instr.operand2);
  return result;
}

multiply decode_multiply(WORD src) {
  WORD cond = get_bits(src, 28, 31);
  WORD a    = get_bit(src, 21);
  WORD s    = get_bit(src, 20);
  WORD rd   = get_bits(src, 16, 19);
  WORD rn   = get_bits(src, 12, 15);
  WORD rs   = get_bits(src, 8, 11);
  WORD rm   = get_bits(src, 0, 3);
  return (multiply) {cond, a, s, rd, rn, rs, rm};
}

WORD encode_multiply(multiply instr) {
  WORD result = 0;
  set_bits_to(&result, 28, 31, instr.cond);
  set_bits_to(&result, 21, 21, instr.a);
  set_bits_to(&result, 20, 20, instr.s);
  set_bits_to(&result, 16, 19, instr.rd);
  set_bits_to(&result, 12, 15, instr.rn);
  set_bits_to(&result, 8, 11, instr.rs);
  set_bit(&result, 7);
  set_bit(&result, 4);
  set_bits_to(&result, 0, 3, instr.rm);
  return result;
}

single_data_transfer decode_single_data_transfer(WORD src) {
  WORD cond   = get_bits(src, 28, 31);
  WORD i      = get_bit(src, 25);
  WORD p      = get_bit(src, 24);
  WORD u      = get_bit(src, 23);
  WORD l      = get_bit(src, 20);
  WORD rn     = get_bits(src, 16, 19);
  WORD rd     = get_bits(src, 12, 15);
  WORD offset = get_bits(src, 0, 11);
  return (single_data_transfer) {cond, i, p, u, l, rn, rd, offset};
}

WORD encode_single_data_transfer(single_data_transfer instr) {
  WORD result = 0;
  set_bits_to(&result, 28, 31, instr.cond);
  set_bit(&result, 26);
  set_bits_to(&result, 25, 25, instr.i);
  set_bits_to(&result, 24, 24, instr.p);
  set_bits_to(&result, 23, 23, instr.u);
  set_bits_to(&result, 20, 20, instr.l);
  set_bits_to(&result, 16, 19, instr.rn);
  set_bits_to(&result, 12, 15, instr.rd);
  set_bits_to(&result, 0, 11, instr.offset);
  return result;
}

branch decode_branch(WORD src) {
  WORD cond   = get_bits(src, 28, 31);
  WORD offset = get_bits(src, 0, 23);
  return (branch) {cond, offset};
}

WORD encode_branch(branch instr) {
  WORD result = 0;
  set_bits_to(&result, 28, 31, instr.cond);
  set_bit(&result, 27);
  set_bit(&result, 25);
  set_bits_to(&result, 0, 23, instr.offset);
  return result;
}

bool cond_check(WORD nzcv, WORD cond) {
  switch(cond) {
    case 0:
      return get_bit(nzcv, 2);
    case 1:
      return !get_bit(nzcv, 2);
    case 10:
      return get_bit(nzcv, 3) == get_bit(nzcv, 1);
    case 11:
      return get_bit(nzcv, 3) != get_bit(nzcv, 1);
    case 12:
      return !get_bit(nzcv, 2) && get_bit(nzcv, 3) == get_bit(nzcv, 1);
    case 13:
      return get_bit(nzcv, 2) || get_bit(nzcv, 3) != get_bit(nzcv, 1);
    case 14:
      return true;
    default:
      return false;
  }
}

bool execute_data_processing(State *arm_state, data_processing *params){
  WORD nczv = get_bits((WORD)arm_state->reg[16], 28, 31);
  bool exec  = cond_check(nczv, params->cond);
  if (exec){
    //finding how much to offset by
    WORD operand2;
    if (params->i) {
      WORD value = get_bits(params->operand2, 0, 7);
      WORD rotation = get_bits(params->operand2, 8, 11);
      operand2 = rotate_right(value, 2 * rotation);
    } else {
      //same exact method is used to decode operand2
      operand2 = operand2_decode(params->operand2, arm_state->reg, params->s);
    }

    WORD result;
    switch (params->opcode){
      //case AND
      case 0x0:
        result = arm_state->reg[params->rn] & operand2;
        arm_state->reg[params->rd] = result;
        break;
      //case EOR
      case 0x1:
        result = arm_state->reg[params->rn] ^ operand2;
        arm_state->reg[params->rd] = result;
        break;
      //case SUB
      case 0x2:
        result = arm_state->reg[params->rn] - operand2;
        arm_state->reg[params->rd] = result;
        if (params->s) {
          if (arm_state->reg[params->rn] < operand2) {
            clear_bit(&(arm_state->reg[16]), 29);
          } else {
            set_bit(&(arm_state->reg[16]), 29);
          }
        }
        break;
      //case RSB
      case 0x3:
        result = operand2 - arm_state->reg[params->rn];
        arm_state->reg[params->rd] = result;
        if (params->s) {
          if (operand2 < arm_state->reg[params->rn]) {
            clear_bit(&(arm_state->reg[16]), 29);
          } else {
            set_bit(&(arm_state->reg[16]), 29);
          }
        }
        break;
      //case ADD
      case 0x4:
        result = arm_state->reg[params->rn] + operand2;
        arm_state->reg[params->rd] = result;
        if (params->s) {
          if (result < arm_state->reg[params->rd]) {
            set_bit(&(arm_state->reg[16]), 29);
          } else {
            clear_bit(&(arm_state->reg[16]), 29);
          }
        }
        break;
      //case TST
      case 0x8:
        result = arm_state->reg[params->rn] & operand2;
        break;
      //case TEQ
      case 0x9:
        result = arm_state->reg[params->rn] ^ operand2;
        break;
      //case CMP
      case 0xA:
        result = arm_state->reg[params->rn] - operand2;
        if (params->s) {
          if (arm_state->reg[params->rn] < operand2) {
            clear_bit(&(arm_state->reg[16]), 29);
          } else {
            set_bit(&(arm_state->reg[16]), 29);
          }
        }
        break;
      //case ORR
      case 0xC:
        result = arm_state->reg[params->rn] | operand2;
        arm_state->reg[params->rd] = result;
        break;
      //case MOV
      case 0xD:
        result = operand2;
        arm_state->reg[params->rd] = result;
        break;
    }
    if (params->s) {
      //set Z
      if (result == 0) {
        set_bit(&(arm_state->reg[16]), 30);
      } else {
        clear_bit(&(arm_state->reg[16]), 30);
      }
      //set N
      set_bits_to(&(arm_state->reg[16]), 31, 31, get_bit(result, 31));
    }
  }
  return exec;
}

bool execute_multiply(State *arm_state, multiply *params){
  WORD nczv  = get_bits((WORD)arm_state->reg[16], 28, 31);
  bool exec   = cond_check(nczv, params->cond);
  int result = arm_state->reg[params -> rm] * arm_state->reg[params -> rs];

  if (exec) {
    if (params->a) {
      result += arm_state->reg[params -> rn];
    }
    //Update CPSR flags
    if (params->s){
      if (result < 0) {
        set_bit((WORD *)&arm_state->reg[16], 31);
      } else if (result == 0) {
        set_bit((WORD *)&arm_state->reg[16], 30);
      }
      // The spec does not tell us to add this but it might become useful, so I left this here.
      /*else if (result >= (1 << 31) || result < -(1 << 31)) {
        set_bit(&arm_state->reg[16], 29);
      }*/
    }

    arm_state->reg[params->rd] = result;

    }
  return exec;
}

bool execute_single_data_transfer(State *arm_state, single_data_transfer *params){
  WORD nczv = get_bits((WORD)arm_state->reg[16], 28, 31);
  bool exec = cond_check(nczv, params->cond);
  int mem_location;

  if (exec) {
    //finding how much to offset by
    int offset_value;
    if (!(params->i)) {
      offset_value = params->offset;
    } else {
      offset_value = offset_decode(params->offset, arm_state->reg);
    }

    //deciding whether to add or subtract
    if (!(params->u)) {
      offset_value = -offset_value;
    }

    //pre-index vs post-index
    if (params->p) {
      mem_location = arm_state->reg[params->rn] + offset_value;
    } else {
      mem_location = arm_state->reg[params->rn];
      arm_state->reg[params->rn] += offset_value;
    }

    if (0 > mem_location || mem_location >= MEMORY_SIZE) {
      printf("Error: Out of bounds memory access at address 0x%08x\n", mem_location);
      return false;
    }

    //store vs load
    WORD *mem_ptr = (WORD *)&(arm_state->memory[mem_location]);
    if (params->l) {
      WORD fetched = *(mem_ptr);
      arm_state->reg[params->rd] = fetched;
    } else {
      *(mem_ptr) = arm_state->reg[params->rd];
    }

  }
  return exec;
}

bool execute_branch(State *arm_state, branch *params){
  WORD nczv = get_bits((WORD)arm_state->reg[16], 28, 31);
  bool exec  = cond_check(nczv, params->cond);
  if (exec) {
    WORD delta = params->offset << 2;
    //sign extend
    WORD sign = get_bit(delta, 25);
    if(sign) {
      WORD set_bits = 0x3F << 26;
      delta = delta | set_bits;
    }
    signed int offset = (signed int) delta;
    //check if the offset will cause memory OOB
    int new_pc = arm_state->reg[15] + offset;
    if (new_pc < 0 || new_pc >= MEMORY_SIZE) {
      return false;
    }
    arm_state->reg[15] = new_pc;
  }
  return exec;
}
