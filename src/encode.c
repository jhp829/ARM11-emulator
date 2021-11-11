#include "encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "symbol_table.h"
#include "instructions.h"

ftable *map_opcode_to_function(void){
  ftable *opfunc = ftable_create();
  ftable_insert(opfunc, "add", &assemble_add);
  ftable_insert(opfunc, "sub", &assemble_sub);
  ftable_insert(opfunc, "rsb", &assemble_rsb);
  ftable_insert(opfunc, "and", &assemble_and);
  ftable_insert(opfunc, "eor", &assemble_eor);
  ftable_insert(opfunc, "orr", &assemble_orr);
  ftable_insert(opfunc, "mov", &assemble_mov);
  ftable_insert(opfunc, "tst", &assemble_tst);
  ftable_insert(opfunc, "teq", &assemble_teq);
  ftable_insert(opfunc, "cmp", &assemble_cmp);
  ftable_insert(opfunc, "mul", &assemble_mul);
  ftable_insert(opfunc, "mla", &assemble_mla);
  ftable_insert(opfunc, "ldr", &assemble_ldr);
  ftable_insert(opfunc, "str", &assemble_str);
  ftable_insert(opfunc, "beq", &assemble_beq);
  ftable_insert(opfunc, "bne", &assemble_bne);
  ftable_insert(opfunc, "bge", &assemble_bge);
  ftable_insert(opfunc, "blt", &assemble_blt);
  ftable_insert(opfunc, "bgt", &assemble_bgt);
  ftable_insert(opfunc, "ble", &assemble_ble);
  ftable_insert(opfunc, "b",   &assemble_bal);
  ftable_insert(opfunc, "lsl", &assemble_lsl);
  ftable_insert(opfunc, "andeq", &assemble_andeq);
  return opfunc;
}

// convert the encoded instruction (WORD) to BYTE
// put the result into the output array
void byte_output(WORD instr, BYTE *output, WORD address){
  for (int i = 0; i < 4; i++) {
    output[address + i] = ((BYTE *)(&instr))[i];
  }
}

void assemble(char *line, table *sym_table, BYTE **output, WORD *out_size, WORD address){
  char **tokens = calloc(1 + MAX_OPERAND_N, sizeof(char *));
  int token_n = tokenize(line, tokens);
  if (token_n < 2) {
    return;
  }
  char *opcode = tokens[0];
  ftable *opfunc = map_opcode_to_function();
  word_func func;

  fail_if(!ftable_get(opfunc, opcode, &func), "Wrong instruction mnemonic");

  WORD (*assemble_func) (char **, int, table *, BYTE **, WORD *, WORD)
    = (WORD (*)(char **, int, table *, BYTE **, WORD *, WORD))func;

  WORD encoded_instr = assemble_func(tokens, token_n, sym_table, output, out_size, address);
  byte_output(encoded_instr, *output, address);
}

// return a WORD by parsing a string with certain format below
WORD parse_value(char *token){
  if (token[2] == 'x') {
    // hexadecimal: _0x(value)
    return strtoul(token + 3 * sizeof(char), NULL, 16);
  } else {
    // decimal: _(value)
    return strtoul(token + sizeof(char), NULL, 10);
  }
}

// return a WORD by parsing a string with format r(value)
WORD parse_register(char *token){
  return strtoul(token + sizeof(char), NULL, 10);
}

// encode (assemble) assemble_data_processing that compute result
// and eor sub rsb add orr
WORD assemble_data_processing_with_result(char **tokens, WORD instr_opcode){
  data_processing instr;
  instr.cond     = 14;
  instr.i        = (tokens[3][0] == '#');
  instr.opcode   = instr_opcode;
  instr.s        = 0;
  instr.rn       = parse_register(tokens[2]);
  instr.rd       = parse_register(tokens[1]);
  instr.operand2 = instr.i ? parse_value(tokens[3]) : parse_register(tokens[3]);
  return encode_data_processing(instr);
}

#define DPRES_FUNC(type, instr_opcode) ASSEMBLE_FUNC(type) {\
  return assemble_data_processing_with_result(tokens, instr_opcode);\
}

DPRES_FUNC(and, 0)
DPRES_FUNC(eor, 1)
DPRES_FUNC(sub, 2)
DPRES_FUNC(rsb, 3)
DPRES_FUNC(add, 4)
DPRES_FUNC(orr, 12)

// encode (assemble) assemble_data_processing other than the ones that compute result
// tst teq cmp mov
WORD assemble_data_processing(char **tokens, WORD instr_opcode, WORD set){
  data_processing instr;
  instr.cond     = 14;
  instr.i        = (tokens[2][0] == '#' || tokens[2][0] == '=');
  instr.opcode   = instr_opcode;
  instr.s        = set;
  instr.rn       =  set ? parse_register(tokens[1]) : 0;
  instr.rd       = !set ? parse_register(tokens[1]) : 0;
  if (instr.i) {
    WORD value = parse_value(tokens[2]);
    WORD rot;
    for(rot = 0; rot < 16 && value > 0xFF; rot++) {
      value = rotate_left(value, 2);
    }
    set_bits_to(&value, 8, 11, rot);
    instr.operand2 = value;
  } else {
    instr.operand2 = parse_register(tokens[2]);
  }
  return encode_data_processing(instr);
}

#define DP_FUNC(type, instr_opcode, set) ASSEMBLE_FUNC(type) {\
  return assemble_data_processing(tokens, instr_opcode, set);\
}

DP_FUNC(tst, 8, 1)
DP_FUNC(teq, 9, 1)
DP_FUNC(cmp, 10, 1)
DP_FUNC(mov, 13, 0)

// encode (assemble) multiply instructions (mul, mla)
WORD assemble_multiply(char **tokens, int token_n, WORD accum){
  multiply instr;
  instr.cond = 0xE;
  instr.a    = accum;
  instr.s    = 0;
  instr.rd   = parse_register(tokens[1]);
  instr.rn   = token_n == 5 ? parse_register(tokens[4]) : 0;
  instr.rs   = parse_register(tokens[3]);
  instr.rm   = parse_register(tokens[2]);
  return encode_multiply(instr);
}

#define MULTIPLY_FUNC(type, accum) ASSEMBLE_FUNC(type) {\
  return assemble_multiply(tokens, token_n, accum);\
}

MULTIPLY_FUNC(mul, 0)
MULTIPLY_FUNC(mla, 1)

// encode (assemble) single data transfer instructions
// 4 cases of input (excluding the optional ones)
WORD assemble_single_data_transfer(char **tokens, int token_n, BYTE **output, WORD *out_size,
        WORD current_address, int load_store){
  single_data_transfer instr;
  instr.cond   = 0xE;
  instr.i      = 0;
  instr.u      = 1;
  instr.l      = load_store;
  instr.rd     = parse_register(tokens[1]);

  // numeric constant <=expression>
  if(tokens[2][0] == '='){
    WORD expression = (WORD)parse_value(tokens[2]);
    if(expression <= 0xFF){
      // assemble mov instructions with the operands
      return assemble_data_processing(tokens, 13, 0);
    }
    instr.p      = 1;
    instr.offset = *out_size - current_address - 8;
    instr.rn     = 15;

    *output = realloc(*output, *out_size + 4);
    byte_output(expression, *output, *out_size);
    *out_size   += 4;
  }

  // pre-indexed with no offset
  else if(token_n == 3){
    instr.p      = 1;
    instr.rn     = parse_register(strtok(&tokens[2][1],"]"));
    instr.offset = 0;
  }

  // pre-indexed with offset
  else if(tokens[2][strlen(tokens[2]) - 1] != ']'){
    instr.p        = 1;
    instr.rn       = parse_register(&tokens[2][1]);
    if (tokens[3][1] == '-') {
      instr.offset = parse_value(tokens[3] + 1);
      instr.u      = 0;
    } else {
      instr.offset = parse_value(tokens[3]);
    }
  }

  //post-indexed
  else{
    instr.p      = 0;
    instr.rn     = parse_register(strtok(&tokens[2][1], ",]"));
    instr.offset = parse_value(tokens[3]);
  }

  return encode_single_data_transfer(instr);
}

#define SINGLE_DATA_TRANSFER_FUNC(type, load_store) ASSEMBLE_FUNC(type) {\
  return assemble_single_data_transfer(tokens, token_n, output, out_size, current_address, load_store);\
}

SINGLE_DATA_TRANSFER_FUNC(str, 0)
SINGLE_DATA_TRANSFER_FUNC(ldr, 1)

// encode (assemble) branch instructions
WORD parse_branch_address(char *token, table *symbol_table) {
  WORD address;
  if (token[0] == '#') {
    address = parse_value(token);
  } else {
    fail_if(!table_get(symbol_table, token, &address),
            "Cannot branch to a non-existent label");
  }
  return address;
}

WORD assemble_branch(char **tokens, table *symbol_table, WORD current_address, WORD cond){
  branch instr;
  instr.cond = cond;
  int offset = (int)(parse_branch_address(tokens[1], symbol_table) - current_address - 8);
  instr.offset = (WORD)offset >> 2u;
  return encode_branch(instr);
}

#define BRANCH_FUNC(suffix, cond) ASSEMBLE_FUNC(b##suffix) {\
  return assemble_branch(tokens, symbol_table, current_address, cond);\
}

BRANCH_FUNC(eq, 0)
BRANCH_FUNC(ne, 1)
BRANCH_FUNC(ge, 10)
BRANCH_FUNC(lt, 11)
BRANCH_FUNC(gt, 12)
BRANCH_FUNC(le, 13)
BRANCH_FUNC(al, 14)

ASSEMBLE_FUNC(lsl) {
  char **new_tokens = calloc(5, sizeof(char *));
  new_tokens[0] = "mov";
  new_tokens[1] = tokens[1];
  new_tokens[2] = tokens[1];
  new_tokens[3] = "lsl";
  new_tokens[4] = tokens[2];
  WORD instr = assemble_data_processing(new_tokens, 13, 0);
  //shifted register is not implemented in mov
  set_bits_to(&instr, 7, 11, parse_value(tokens[2]));
  return instr;
}

ASSEMBLE_FUNC(andeq) {
  return 0;
}
