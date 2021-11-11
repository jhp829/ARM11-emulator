#include "utils.h"
#include "instructions.h"
#include "symbol_table.h"
#include "encode.h"

#define ASSERT(a) do { \
  asserts_ran++; \
  if (!(a)) {\
     printf("%s(line %d) => assert failed: %s\n", __func__, __LINE__, #a);\
     asserts_failed++;\
  }\
} while (0)

#define ASSERT_EQ(got, expected, format) do { \
  asserts_ran++; \
  if ((got) != (expected)) {\
     printf("%s(line %d) => " #format "\n", __func__, __LINE__, got, expected);\
     asserts_failed++;\
  }\
} while (0)

#define ASSERT_INT_EQ(a, b) ASSERT_EQ(a, b, got: %d | expected: %d)

#define ASSERT_HEX_EQ(a, b) ASSERT_EQ(a, b, got: %08x | expected: %08x)

#define ASSERT_MEM_EQ(mem1, mem2) do {\
  for(int idx = 0; idx < MEMORY_SIZE; idx++) {\
    ASSERT_EQ((mem1)[idx], (mem2)[idx], got: 0x%02x | expected: 0x%02x);\
  }\
} while (0)

#define ASSERT_REG_EQ(reg1, reg2) do {\
  ASSERT_EQ((reg1)[0], (reg2)[0], got reg0: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[1], (reg2)[1], got reg1: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[2], (reg2)[2], got reg2: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[3], (reg2)[3], got reg3: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[4], (reg2)[4], got reg4: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[5], (reg2)[5], got reg5: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[6], (reg2)[6], got reg6: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[7], (reg2)[7], got reg7: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[8], (reg2)[8], got reg8: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[9], (reg2)[9], got reg9: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[10], (reg2)[10], got reg10: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[11], (reg2)[11], got reg11: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[12], (reg2)[12], got reg12: 0x%d | expected: 0x%d);\
  ASSERT_EQ((reg1)[15], (reg2)[15], got PC: 0x%08x | expected: 0x%08x);\
  ASSERT_EQ((reg1)[16], (reg2)[16], got NZCV: 0x%08x | expected: 0x%08x);\
} while (0)

#define RUN_TEST(test) do { \
  asserts_ran = 0;\
  asserts_failed = 0;\
  test(); \
  tests_ran++;\
  if (asserts_failed > 0) { \
    tests_failed++; \
    printf("%s: %d asserts failed out of %d asserts\n\n", #test, asserts_failed, asserts_ran); \
  }\
} while (0)

int asserts_ran = 0;
int asserts_failed = 0;
int tests_ran = 0;
int tests_failed = 0;

void test_execute_branch(void) {
  BYTE *memory = calloc(1, MEMORY_SIZE);
  WORD *reg = allocate_register();
  reg[15] = 0x14;
  State state = {memory, reg};

  branch instr1 = {0xa, 0x14};

  BYTE *expected_memory1 = calloc(1, MEMORY_SIZE);
  WORD *expected_reg1 = allocate_register();
  expected_reg1[15] = 0x64; //0x14 + (0x14 << 2)

  int branch_success = execute_branch(&state, &instr1);
  ASSERT(branch_success);
  ASSERT_MEM_EQ(memory, expected_memory1);
  ASSERT_REG_EQ(reg, expected_reg1);
  free(expected_memory1);
  free(expected_reg1);

  branch instr2 = {0xa, 0x00FFFFEC};

  BYTE *expected_memory2 = calloc(1, MEMORY_SIZE);
  WORD *expected_reg2 = allocate_register();
  expected_reg2[15] = 0x14; //0x64 - (0x14 << 2)

  branch_success = execute_branch(&state, &instr2);
  ASSERT(branch_success);
  ASSERT_MEM_EQ(memory, expected_memory2);
  ASSERT_REG_EQ(reg, expected_reg2);
  free(expected_memory2);
  free(expected_reg2);

  free(memory);
  free(reg);
}

void test_execute_multiply(void) {
  for(int i = -10; i < 10; i++) {
    for(int j = -10; j < 10; j++) {
      BYTE *memory = calloc(1, MEMORY_SIZE);
      WORD *reg = allocate_register();
      reg[0] = i;
      reg[5] = j;
      State state = {memory, reg};

      multiply instr = {
        .cond = 0xa,
        .a = 0,
        .s = 1,
        .rd = 1,
        .rn = 2,
        .rs = 0,
        .rm = 5};

      BYTE *expected_memory = calloc(1, MEMORY_SIZE);
      WORD *expected_reg = allocate_register();
      expected_reg[0] = i;
      expected_reg[5] = j;
      expected_reg[1] = i * j;
      if (i*j < 0) {
        set_bit((WORD *)&(expected_reg[16]), 31);
      }
      if (i*j == 0) {
        set_bit((WORD *)&(expected_reg[16]), 30);
      }

      int branch_success = execute_multiply(&state, &instr);
      ASSERT(branch_success);
      ASSERT_MEM_EQ(memory, expected_memory);
      ASSERT_REG_EQ(reg, expected_reg);
      free(expected_memory);
      free(expected_reg);
      free(memory);
      free(reg);
    }
  }
}

void test_symbol_table(void) {
  table *t = table_create();
  WORD value;

  #define ASSERT_TABLE(key, val) do {\
    ASSERT(table_get(t, key, &value));\
    ASSERT_INT_EQ(value, val);\
  } while(0);

  for (int i = 0; i < 100; i++) {
    char label[3];
    sprintf(label, "%d", i);
    table_insert(t, label, i);
    ASSERT_TABLE(label, i)
  }

  for (int i = 100; i < 1000; i++) {
    char label[4];
    sprintf(label, "%d", i);
    ASSERT(!table_get(t, label, &value));
  }

  table_free(t);
}

void test_assemble_branch(void) {
  table *sym_table = table_create();

  WORD value1 = 0x0;
  WORD value2 = 0xFFFF;
  WORD value3 = 0xFF;

  table_insert(sym_table, "start", value1);
  table_insert(sym_table, "end", value2);
  table_insert(sym_table, "middle", value3);
  WORD current_address = 0x0;

  // these functions assume the first token is correct
  // we can "abuse" this to check each branch type without modifiying
  // the first token.
  char *instr_tokens[2] = {"b", "start"};
  WORD expected = 0x0AFFFFFE; //0b0000'1010'0...
  ASSERT_HEX_EQ(assemble_beq(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0x1AFFFFFE; //0b0001'1010'0...
  ASSERT_HEX_EQ(assemble_bne(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xAAFFFFFE; //0b1010'1010'0...
  ASSERT_HEX_EQ(assemble_bge(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xBAFFFFFE; //0b1011'1010'0...
  ASSERT_HEX_EQ(assemble_blt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xCAFFFFFE; //0b1100'1010'0...
  ASSERT_HEX_EQ(assemble_bgt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xDAFFFFFE; //0b1101'1010'0...
  ASSERT_HEX_EQ(assemble_ble(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xEAFFFFFE; //0b1110'1010'0...
  ASSERT_HEX_EQ(assemble_bal(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);

  instr_tokens[1] = "end";
  expected = 0x0A003FFD; //0b0000'1010'0...
  ASSERT_HEX_EQ(assemble_beq(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0x1A003FFD; //0b0001'1010'0...
  ASSERT_HEX_EQ(assemble_bne(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xAA003FFD; //0b1010'1010'0...
  ASSERT_HEX_EQ(assemble_bge(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xBA003FFD; //0b1011'1010'0...
  ASSERT_HEX_EQ(assemble_blt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xCA003FFD; //0b1100'1010'0...
  ASSERT_HEX_EQ(assemble_bgt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xDA003FFD; //0b1101'1010'0...
  ASSERT_HEX_EQ(assemble_ble(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xEA003FFD; //0b1110'1010'0...
  ASSERT_HEX_EQ(assemble_bal(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);

  instr_tokens[1] = "middle";
  expected = 0x0A00003D; //0b0000'1010'0...
  ASSERT_HEX_EQ(assemble_beq(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0x1A00003D; //0b0001'1010'0...
  ASSERT_HEX_EQ(assemble_bne(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xAA00003D; //0b1010'1010'0...
  ASSERT_HEX_EQ(assemble_bge(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xBA00003D; //0b1011'1010'0...
  ASSERT_HEX_EQ(assemble_blt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xCA00003D; //0b1100'1010'0...
  ASSERT_HEX_EQ(assemble_bgt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xDA00003D; //0b1101'1010'0...
  ASSERT_HEX_EQ(assemble_ble(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xEA00003D; //0b1110'1010'0...
  ASSERT_HEX_EQ(assemble_bal(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);

  instr_tokens[1] = "#0xFFFF";
  expected = 0x0A003FFD; //0b0000'1010'0...
  ASSERT_HEX_EQ(assemble_beq(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0x1A003FFD; //0b0001'1010'0...
  ASSERT_HEX_EQ(assemble_bne(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xAA003FFD; //0b1010'1010'0...
  ASSERT_HEX_EQ(assemble_bge(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xBA003FFD; //0b1011'1010'0...
  ASSERT_HEX_EQ(assemble_blt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xCA003FFD; //0b1100'1010'0...
  ASSERT_HEX_EQ(assemble_bgt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xDA003FFD; //0b1101'1010'0...
  ASSERT_HEX_EQ(assemble_ble(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xEA003FFD; //0b1110'1010'0...
  ASSERT_HEX_EQ(assemble_bal(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);

  instr_tokens[1] = "#65535";
  expected = 0x0A003FFD; //0b0000'1010'0...
  ASSERT_HEX_EQ(assemble_beq(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0x1A003FFD; //0b0001'1010'0...
  ASSERT_HEX_EQ(assemble_bne(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xAA003FFD; //0b1010'1010'0...
  ASSERT_HEX_EQ(assemble_bge(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xBA003FFD; //0b1011'1010'0...
  ASSERT_HEX_EQ(assemble_blt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xCA003FFD; //0b1100'1010'0...
  ASSERT_HEX_EQ(assemble_bgt(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xDA003FFD; //0b1101'1010'0...
  ASSERT_HEX_EQ(assemble_ble(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);
  expected = 0xEA003FFD; //0b1110'1010'0...
  ASSERT_HEX_EQ(assemble_bal(instr_tokens, 2, sym_table, NULL, NULL, current_address), expected);

  table_free(sym_table);
}

void test_assemble_single_data_transfer(void) {

  WORD current_address = 0x0;
  WORD out_size1 = 0x1000;
  BYTE *output1 = calloc(out_size1,sizeof(BYTE));
  WORD out_size2 = 0xABC;
  BYTE *output2 = calloc(out_size2,sizeof(BYTE));

  // Similar to the tests on branch, these functions assume the first token to be correct.
  // Therefore I will not be specifying the ldr/str functions in the tokens.

  // Case 1: Instruction of the form ldr Rd,=HEXNUM, and HEXNUM > 0xFF
  char* instr_tokens1[3] = {"ldr", "r6","=0xFFF"};  //ldr r6,=0x
  WORD expected = 0xE59F6FF8;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens1, 3, NULL, &output1, &out_size1, current_address), expected);
  expected = 0xE59F6AB4;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens1, 3, NULL, &output2, &out_size2, current_address), expected);

  current_address = 0xC;
  instr_tokens1[1] = "r8";
  instr_tokens1[2] = "=0x123"; //ldr r8,=0x123
  expected = 0xE59F8FF4;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens1, 3, NULL, &output1, &out_size1, current_address), expected);
  expected = 0xE59F8AB0;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens1, 3, NULL, &output2, &out_size2, current_address), expected);

  // Case 2: Instruction of the form ldr Rd,=HEXNUM, and HEXNUM <= 0xFF
  char *instr_tokens2[3] = {"ldr", "r2", "=0xFE"};
  expected = 0xE3A020FE;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens2, 3, NULL, &output1, NULL, current_address), expected);
  instr_tokens2[1] = "r12";
  expected = 0xE3A0C0FE;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens2, 3, NULL, &output1, NULL, current_address), expected);
  instr_tokens2[2] = "=0xAA";
  expected = 0xE3A0C0AA;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens2, 3, NULL, &output1, NULL, current_address), expected);

  // Case 3: Instructions of the form ldr/str Rd,[Rn]
  char *instr_tokens3[10] = {"ldr", "r2", calloc(10, sizeof(char))};
  sprintf(instr_tokens3[2], "[r6]");
  expected = 0xE5962000;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens3, 3, NULL, &output1, NULL, current_address), expected);
  expected = 0xE5862000;
  ASSERT_HEX_EQ(assemble_str(instr_tokens3, 3, NULL, NULL, NULL, current_address), expected);

  sprintf(instr_tokens3[2], "[r10]");
  expected = 0xE59A2000;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens3, 3, NULL, &output1, NULL, current_address), expected);
  expected = 0xE58A2000;
  ASSERT_HEX_EQ(assemble_str(instr_tokens3, 3, NULL, NULL, NULL, current_address), expected);

  instr_tokens3[1] = "r5";
  expected = 0xE59A5000;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens3, 3, NULL, &output1, NULL, current_address), expected);
  expected = 0xE59A5000;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens3, 3, NULL, NULL, NULL, current_address), expected);

  free(instr_tokens3[2]);

  // Case 4: Instructions of the form ldr/str Rd,[Rn,<#expression>] (Optional)
  char *instr_tokens4[4] = {"ldr","r0","[r2","#230]"};
  expected = 0xE59200E6;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens4, 4, NULL, &output1, NULL, current_address), expected);
  expected = 0xE58200E6;
  ASSERT_HEX_EQ(assemble_str(instr_tokens4, 4, NULL, NULL, NULL, current_address), expected);
  instr_tokens4[3] = "#0xAF7]";
  expected = 0xE5920AF7;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens4, 4, NULL, &output1, NULL, current_address), expected);
  expected = 0xE5820AF7;
  ASSERT_HEX_EQ(assemble_str(instr_tokens4, 4, NULL, NULL, NULL, current_address), expected);


  // Case 5: Instructions of the form ldr/str Rd,[Rn],<#expression>
  char *instr_tokens5[4] = {"ldr","r0","[r2","#230]"};
  expected = 0xE59200E6;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens5, 4, NULL, &output1, NULL, current_address), expected);
  expected = 0xE58200E6;
  ASSERT_HEX_EQ(assemble_str(instr_tokens5, 4, NULL, NULL, NULL, current_address), expected);
  instr_tokens5[3] = "#0xAF7";
  expected = 0xE5920AF7;
  ASSERT_HEX_EQ(assemble_ldr(instr_tokens5, 4, NULL, &output1, NULL, current_address), expected);
  expected = 0xE5820AF7;
  ASSERT_HEX_EQ(assemble_str(instr_tokens5, 4, NULL, NULL, NULL, current_address), expected);

  free(output1);
  free(output2);
}

void test_assemble_multiply(void) {
  // Test for mul instruction
  char *instr_tokens_mul[4] = {"mul", "r1", "r3", "r5"};
  WORD expected = 0xE0010593;
  ASSERT_HEX_EQ(assemble_mul(instr_tokens_mul, 4, NULL, NULL, NULL, 0), expected);
  instr_tokens_mul[3] = "r10";
  expected = 0xE0010A93;
  ASSERT_HEX_EQ(assemble_mul(instr_tokens_mul, 4, NULL, NULL, NULL, 0), expected);
  instr_tokens_mul[1] = "r11";
  expected = 0xE00B0A93;
  ASSERT_HEX_EQ(assemble_mul(instr_tokens_mul, 4, NULL, NULL, NULL, 0), expected);
  instr_tokens_mul[2] = "r2";
  expected = 0xE00B0A92;
  ASSERT_HEX_EQ(assemble_mul(instr_tokens_mul, 4, NULL, NULL, NULL, 0), expected);

  // Test for mla instruction
  char *instr_tokens_mla[5] = {"mla", "r1", "r3", "r5", "r7"};
  expected = 0xE0217593;
  ASSERT_HEX_EQ(assemble_mla(instr_tokens_mla, 5, NULL, NULL, NULL, 0), expected);
  instr_tokens_mla[1] = "r2";
  expected = 0xE0227593;
  ASSERT_HEX_EQ(assemble_mla(instr_tokens_mla, 5, NULL, NULL, NULL, 0), expected);
  instr_tokens_mla[2] = "r4";
  expected = 0xE0227594;
  ASSERT_HEX_EQ(assemble_mla(instr_tokens_mla, 5, NULL, NULL, NULL, 0), expected);
  instr_tokens_mla[3] = "r6";
  expected = 0xE0227694;
  ASSERT_HEX_EQ(assemble_mla(instr_tokens_mla, 5, NULL, NULL, NULL, 0), expected);
  instr_tokens_mla[4] = "r8";
  expected = 0xE0228694;
  ASSERT_HEX_EQ(assemble_mla(instr_tokens_mla, 5, NULL, NULL, NULL, 0), expected);
}

void test_assemble_dp(void) {
  // Like mentioned previously, these functions assume that the first token is correct.
  // Case 1: the and, eor, sub, rsb, add, orr functions

  char *instr_tokens1[4] = {"and", "r1", "r3", "#0xAB"};
  WORD expected = 0xE20310AB;
  ASSERT_HEX_EQ(assemble_and(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE22310AB;
  ASSERT_HEX_EQ(assemble_eor(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE24310AB;
  ASSERT_HEX_EQ(assemble_sub(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE26310AB;
  ASSERT_HEX_EQ(assemble_rsb(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE28310AB;
  ASSERT_HEX_EQ(assemble_add(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE38310AB;
  ASSERT_HEX_EQ(assemble_orr(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);

  instr_tokens1[1] = "r2";
  expected = 0xE20320AB;
  ASSERT_HEX_EQ(assemble_and(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE22320AB;
  ASSERT_HEX_EQ(assemble_eor(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE24320AB;
  ASSERT_HEX_EQ(assemble_sub(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE26320AB;
  ASSERT_HEX_EQ(assemble_rsb(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE28320AB;
  ASSERT_HEX_EQ(assemble_add(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE38320AB;
  ASSERT_HEX_EQ(assemble_orr(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);

  instr_tokens1[2] = "r4";
  expected = 0xE20420AB;
  ASSERT_HEX_EQ(assemble_and(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE22420AB;
  ASSERT_HEX_EQ(assemble_eor(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE24420AB;
  ASSERT_HEX_EQ(assemble_sub(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE26420AB;
  ASSERT_HEX_EQ(assemble_rsb(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE28420AB;
  ASSERT_HEX_EQ(assemble_add(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE38420AB;
  ASSERT_HEX_EQ(assemble_orr(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);

  instr_tokens1[3] = "#2815";
  expected = 0xE2042AFF;
  ASSERT_HEX_EQ(assemble_and(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE2242AFF;
  ASSERT_HEX_EQ(assemble_eor(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE2442AFF;
  ASSERT_HEX_EQ(assemble_sub(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE2642AFF;
  ASSERT_HEX_EQ(assemble_rsb(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE2842AFF;
  ASSERT_HEX_EQ(assemble_add(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);
  expected = 0xE3842AFF;
  ASSERT_HEX_EQ(assemble_orr(instr_tokens1, 4, NULL, NULL, NULL, 0), expected);

  // Case 2: tst, teq, cmp, mov
  char *instr_tokens2[3] = {"tst", "r1", "#0x4A8"};
  expected = 0xE31100A8;
  ASSERT_HEX_EQ(assemble_tst(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE33100A8;
  ASSERT_HEX_EQ(assemble_teq(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE35100A8;
  ASSERT_HEX_EQ(assemble_cmp(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE3A010A8;
  ASSERT_HEX_EQ(assemble_mov(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);

  instr_tokens2[1] = "r10";
  expected = 0xE31A00A8;
  ASSERT_HEX_EQ(assemble_tst(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE33A00A8;
  ASSERT_HEX_EQ(assemble_teq(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE35A00A8;
  ASSERT_HEX_EQ(assemble_cmp(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE3A0A0A8;
  ASSERT_HEX_EQ(assemble_mov(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);

  instr_tokens2[2] = "#10";
  expected = 0xE31A000A;
  ASSERT_HEX_EQ(assemble_tst(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE33A000A;
  ASSERT_HEX_EQ(assemble_teq(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE35A000A;
  ASSERT_HEX_EQ(assemble_cmp(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
  expected = 0xE3A0A00A;
  ASSERT_HEX_EQ(assemble_mov(instr_tokens2, 3, NULL, NULL, NULL, 0), expected);
}

int main(int argc, char **argv) {
  tests_ran = 0;
  tests_failed = 0;
  RUN_TEST(test_execute_branch);
  RUN_TEST(test_execute_multiply);
  RUN_TEST(test_symbol_table);
  RUN_TEST(test_assemble_branch);
  RUN_TEST(test_assemble_single_data_transfer);
  RUN_TEST(test_assemble_multiply);
  RUN_TEST(test_assemble_dp);

  printf("%d/%d tests successful.\n", tests_ran - tests_failed, tests_ran);
}
