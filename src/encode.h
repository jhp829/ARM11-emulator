#ifndef ENCODE
#define ENCODE
#include "utils.h"
#include "symbol_table.h"
#include "instructions.h"

ftable *map_opcode_to_function(void);

// Main assemble function for all instructions
// tokenize line and map the mnemonic opcode to a function pointer of its assemble function
// the encoded instructions result is put into the passed output array
void assemble(char *line, table *sym_table, BYTE **output, WORD *out_size, WORD address);

#define ASSEMBLE_FUNC(mnemonic)\
WORD assemble_##mnemonic(char **tokens, int token_n, table *symbol_table, BYTE **output, WORD *out_size, WORD current_address)

// Assembles the data processing instruction variants
ASSEMBLE_FUNC(add);
ASSEMBLE_FUNC(sub);
ASSEMBLE_FUNC(rsb);
ASSEMBLE_FUNC(and);
ASSEMBLE_FUNC(eor);
ASSEMBLE_FUNC(orr);
ASSEMBLE_FUNC(mov);
ASSEMBLE_FUNC(tst);
ASSEMBLE_FUNC(teq);
ASSEMBLE_FUNC(cmp);

// Assembles the multiply instruction variants
ASSEMBLE_FUNC(mul);
ASSEMBLE_FUNC(mla);

// Assembles the single data transfer instruction variants
ASSEMBLE_FUNC(ldr);
ASSEMBLE_FUNC(str);

// Assembles the branch instruction variants
ASSEMBLE_FUNC(beq);
ASSEMBLE_FUNC(bne);
ASSEMBLE_FUNC(bge);
ASSEMBLE_FUNC(blt);
ASSEMBLE_FUNC(bgt);
ASSEMBLE_FUNC(ble);
ASSEMBLE_FUNC(bal);

// Assembles the special instruction variants
ASSEMBLE_FUNC(lsl);
ASSEMBLE_FUNC(andeq);

#endif //ARM11_21_ENCODE_H
