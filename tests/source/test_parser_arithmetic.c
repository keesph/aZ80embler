#include "unity.h"

#include "lexer/lexer.h"
#include "parser/instruction_encoding.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "types.h"
#include "utility/linked_list.h"

#include <stdio.h>

static FILE *ld_test_file;
static lexer_state_t *lexer;
static parser_t *parser;
static char *testFileName = "test-file.asm";
static int statementCount;

static void reset_context()
{
  rewind(ld_test_file);
  lexer_reset(lexer);
  parser_reset(parser);
}

void setUp(void)
{
  ld_test_file = fopen(testFileName, "w+");
  TEST_ASSERT_NOT_NULL(ld_test_file);

  lexer = lexer_initialize();
  TEST_ASSERT_NOT_NULL(lexer);

  parser = parser_initialize();
  TEST_ASSERT_NOT_NULL(parser);
}

void tearDown(void)
{
  fclose(ld_test_file);
  lexer_destroy(lexer);
  parser_destroy(parser);
}

// 8- and 16-bit arithmetic instructions with two register type operands
void test_Arith_r_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister1;
    operand_type_t expectedOperand2;
    register_type_t expectedRegister2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"ADD A, A\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_A},
      {"ADD A, B\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_B},
      {"ADD A, C\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_C},
      {"ADD A, D\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_D},
      {"ADD A, E\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_E},
      {"ADD A, H\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_H},
      {"ADD A, L\n", encoding_ADD_A_r, opcode_ADD, operand_r, register_A, operand_r, register_L},
      {"ADD A, (HL)\n", encoding_ADD_A_derefHL, opcode_ADD, operand_r, register_A, operand_deref_HL, register_HL},
      {"ADD HL, BC\n", encoding_ADD_HL_ss, opcode_ADD, operand_rr, register_HL, operand_rr, register_BC},
      {"ADD HL, DE\n", encoding_ADD_HL_ss, opcode_ADD, operand_rr, register_HL, operand_rr, register_DE},
      {"ADD HL, HL\n", encoding_ADD_HL_ss, opcode_ADD, operand_rr, register_HL, operand_rr, register_HL},
      {"ADD HL, SP\n", encoding_ADD_HL_ss, opcode_ADD, operand_rr, register_HL, operand_rr, register_SP},
      {"ADD IX, BC\n", encoding_ADD_IX_pp, opcode_ADD, operand_rr, register_IX, operand_rr, register_BC},
      {"ADD IX, DE\n", encoding_ADD_IX_pp, opcode_ADD, operand_rr, register_IX, operand_rr, register_DE},
      {"ADD IX, IX\n", encoding_ADD_IX_pp, opcode_ADD, operand_rr, register_IX, operand_rr, register_IX},
      {"ADD IX, SP\n", encoding_ADD_IX_pp, opcode_ADD, operand_rr, register_IX, operand_rr, register_SP},
      {"ADD IY, BC\n", encoding_ADD_IY_rr, opcode_ADD, operand_rr, register_IY, operand_rr, register_BC},
      {"ADD IY, DE\n", encoding_ADD_IY_rr, opcode_ADD, operand_rr, register_IY, operand_rr, register_DE},
      {"ADD IY, IY\n", encoding_ADD_IY_rr, opcode_ADD, operand_rr, register_IY, operand_rr, register_IY},
      {"ADD IY, SP\n", encoding_ADD_IY_rr, opcode_ADD, operand_rr, register_IY, operand_rr, register_SP},
      {"ADC A, A\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_A},
      {"ADC A, B\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_B},
      {"ADC A, C\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_C},
      {"ADC A, D\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_D},
      {"ADC A, E\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_E},
      {"ADC A, H\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_H},
      {"ADC A, L\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_r, register_L},
      {"ADC A, (HL)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_HL, register_HL},
      {"ADC HL, BC\n", encoding_ADC_HL_ss, opcode_ADC, operand_rr, register_HL, operand_rr, register_BC},
      {"ADC HL, DE\n", encoding_ADC_HL_ss, opcode_ADC, operand_rr, register_HL, operand_rr, register_DE},
      {"ADC HL, HL\n", encoding_ADC_HL_ss, opcode_ADC, operand_rr, register_HL, operand_rr, register_HL},
      {"ADC HL, SP\n", encoding_ADC_HL_ss, opcode_ADC, operand_rr, register_HL, operand_rr, register_SP},
      {"SBC A, A\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_A},
      {"SBC A, B\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_B},
      {"SBC A, C\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_C},
      {"SBC A, D\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_D},
      {"SBC A, E\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_E},
      {"SBC A, H\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_H},
      {"SBC A, L\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_r, register_L},
      {"SBC A, (HL)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_HL, register_HL},
      {"SBC HL, BC\n", encoding_SBC_HL_ss, opcode_SBC, operand_rr, register_HL, operand_rr, register_BC},
      {"SBC HL, DE\n", encoding_SBC_HL_ss, opcode_SBC, operand_rr, register_HL, operand_rr, register_DE},
      {"SBC HL, HL\n", encoding_SBC_HL_ss, opcode_SBC, operand_rr, register_HL, operand_rr, register_HL},
      {"SBC HL, SP\n", encoding_SBC_HL_ss, opcode_SBC, operand_rr, register_HL, operand_rr, register_SP},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(statementCount, linkedList_count(statements));
  for (int i = 0; i < statementCount; i++)
  {
    printf("iteration: %d\n", i);
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.rr);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister2, statement->instruction.operand2.data.rr);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// arithmetics with A as first and immediate 8-bit as second operand
void test_Arith_A_n(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister1;
    operand_type_t expectedOperand2;
    uint8_t expectedImmediate;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"ADD A, 0\n", encoding_ADD_A_n, opcode_ADD, operand_r, register_A, operand_n, 0x00},
      {"ADD A, 255\n", encoding_ADD_A_n, opcode_ADD, operand_r, register_A, operand_n, 0xFF},
      {"ADC A, 0\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_n, 0x00},
      {"ADC A, 255\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_n, 0xFF},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(statementCount, linkedList_count(statements));
  for (int i = 0; i < statementCount; i++)
  {
    printf("iteration: %d\n", i);
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.rr);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedImmediate, statement->instruction.operand2.data.immediate_n);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// arithmetics with A as first and (IX/Y+d) as second operand
void test_Arith_A_Indexed(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister1;
    operand_type_t expectedOperand2;
    register_type_t expectedRegister2;
    int8_t expectedOffset;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"ADD A, (IX+0)\n", encoding_ADD_A_derefIXd, opcode_ADD, operand_r, register_A, operand_deref_idx, register_IX,
       0x00},
      {"ADD A, (IX+127)\n", encoding_ADD_A_derefIXd, opcode_ADD, operand_r, register_A, operand_deref_idx, register_IX,
       0x7F},
      {"ADD A, (IX-128)\n", encoding_ADD_A_derefIXd, opcode_ADD, operand_r, register_A, operand_deref_idx, register_IX,
       0x80},
      {"ADD A, (IY+0)\n", encoding_ADD_A_derefIYd, opcode_ADD, operand_r, register_A, operand_deref_idx, register_IY,
       0x00},
      {"ADD A, (IY+127)\n", encoding_ADD_A_derefIYd, opcode_ADD, operand_r, register_A, operand_deref_idx, register_IY,
       0x7F},
      {"ADD A, (IY-128)\n", encoding_ADD_A_derefIYd, opcode_ADD, operand_r, register_A, operand_deref_idx, register_IY,
       0x80},
      {"ADC A, (IX+0)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_idx, register_IX, 0x00},
      {"ADC A, (IX+127)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_idx, register_IX, 0x7F},
      {"ADC A, (IX-128)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_idx, register_IX, 0x80},
      {"ADC A, (IY+0)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_idx, register_IY, 0x00},
      {"ADC A, (IY+127)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_idx, register_IY, 0x7F},
      {"ADC A, (IY-128)\n", encoding_ADC_A_s, opcode_ADC, operand_r, register_A, operand_deref_idx, register_IY, 0x80},
      {"SBC A, (IX+0)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_idx, register_IX, 0x00},
      {"SBC A, (IX+127)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_idx, register_IX, 0x7F},
      {"SBC A, (IX-128)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_idx, register_IX, 0x80},
      {"SBC A, (IY+0)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_idx, register_IY, 0x00},
      {"SBC A, (IY+127)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_idx, register_IY, 0x7F},
      {"SBC A, (IY-128)\n", encoding_SBC_A_s, opcode_SBC, operand_r, register_A, operand_deref_idx, register_IY, 0x80},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(statementCount, linkedList_count(statements));
  for (int i = 0; i < statementCount; i++)
  {
    printf("iteration: %d\n", i);
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.rr);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister2, statement->instruction.operand2.data.dereference_idx.index_register);
    TEST_ASSERT_EQUAL(driver[i].expectedOffset, statement->instruction.operand2.data.dereference_idx.index);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// arithmetics with a register as operand
void test_Arith_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister1;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"SUB A\n", encoding_SUB_s, opcode_SUB, operand_r, register_A, operand_NA},
      {"SUB B\n", encoding_SUB_s, opcode_SUB, operand_r, register_B, operand_NA},
      {"SUB C\n", encoding_SUB_s, opcode_SUB, operand_r, register_C, operand_NA},
      {"SUB D\n", encoding_SUB_s, opcode_SUB, operand_r, register_D, operand_NA},
      {"SUB E\n", encoding_SUB_s, opcode_SUB, operand_r, register_E, operand_NA},
      {"SUB H\n", encoding_SUB_s, opcode_SUB, operand_r, register_H, operand_NA},
      {"SUB L\n", encoding_SUB_s, opcode_SUB, operand_r, register_L, operand_NA},
      {"SUB (HL)\n", encoding_SUB_s, opcode_SUB, operand_deref_HL, register_HL, operand_NA},

      {"AND A\n", encoding_AND_s, opcode_AND, operand_r, register_A, operand_NA},
      {"AND B\n", encoding_AND_s, opcode_AND, operand_r, register_B, operand_NA},
      {"AND C\n", encoding_AND_s, opcode_AND, operand_r, register_C, operand_NA},
      {"AND D\n", encoding_AND_s, opcode_AND, operand_r, register_D, operand_NA},
      {"AND E\n", encoding_AND_s, opcode_AND, operand_r, register_E, operand_NA},
      {"AND H\n", encoding_AND_s, opcode_AND, operand_r, register_H, operand_NA},
      {"AND L\n", encoding_AND_s, opcode_AND, operand_r, register_L, operand_NA},
      {"AND (HL)\n", encoding_AND_s, opcode_AND, operand_deref_HL, register_HL, operand_NA},

      {"OR A\n", encoding_OR_s, opcode_OR, operand_r, register_A, operand_NA},
      {"OR B\n", encoding_OR_s, opcode_OR, operand_r, register_B, operand_NA},
      {"OR C\n", encoding_OR_s, opcode_OR, operand_r, register_C, operand_NA},
      {"OR D\n", encoding_OR_s, opcode_OR, operand_r, register_D, operand_NA},
      {"OR E\n", encoding_OR_s, opcode_OR, operand_r, register_E, operand_NA},
      {"OR H\n", encoding_OR_s, opcode_OR, operand_r, register_H, operand_NA},
      {"OR L\n", encoding_OR_s, opcode_OR, operand_r, register_L, operand_NA},
      {"OR (HL)\n", encoding_OR_s, opcode_OR, operand_deref_HL, register_HL, operand_NA},

      {"XOR A\n", encoding_XOR_s, opcode_XOR, operand_r, register_A, operand_NA},
      {"XOR B\n", encoding_XOR_s, opcode_XOR, operand_r, register_B, operand_NA},
      {"XOR C\n", encoding_XOR_s, opcode_XOR, operand_r, register_C, operand_NA},
      {"XOR D\n", encoding_XOR_s, opcode_XOR, operand_r, register_D, operand_NA},
      {"XOR E\n", encoding_XOR_s, opcode_XOR, operand_r, register_E, operand_NA},
      {"XOR H\n", encoding_XOR_s, opcode_XOR, operand_r, register_H, operand_NA},
      {"XOR L\n", encoding_XOR_s, opcode_XOR, operand_r, register_L, operand_NA},
      {"XOR (HL)\n", encoding_XOR_s, opcode_XOR, operand_deref_HL, register_HL, operand_NA},

      {"CP A\n", encoding_CP_s, opcode_CP, operand_r, register_A, operand_NA},
      {"CP B\n", encoding_CP_s, opcode_CP, operand_r, register_B, operand_NA},
      {"CP C\n", encoding_CP_s, opcode_CP, operand_r, register_C, operand_NA},
      {"CP D\n", encoding_CP_s, opcode_CP, operand_r, register_D, operand_NA},
      {"CP E\n", encoding_CP_s, opcode_CP, operand_r, register_E, operand_NA},
      {"CP H\n", encoding_CP_s, opcode_CP, operand_r, register_H, operand_NA},
      {"CP L\n", encoding_CP_s, opcode_CP, operand_r, register_L, operand_NA},
      {"CP (HL)\n", encoding_CP_s, opcode_CP, operand_deref_HL, register_HL, operand_NA},

      {"INC A\n", encoding_INC_m, opcode_INC, operand_r, register_A, operand_NA},
      {"INC B\n", encoding_INC_m, opcode_INC, operand_r, register_B, operand_NA},
      {"INC C\n", encoding_INC_m, opcode_INC, operand_r, register_C, operand_NA},
      {"INC D\n", encoding_INC_m, opcode_INC, operand_r, register_D, operand_NA},
      {"INC E\n", encoding_INC_m, opcode_INC, operand_r, register_E, operand_NA},
      {"INC H\n", encoding_INC_m, opcode_INC, operand_r, register_H, operand_NA},
      {"INC L\n", encoding_INC_m, opcode_INC, operand_r, register_L, operand_NA},
      {"INC IX\n", encoding_INC_IX, opcode_INC, operand_rr, register_IX, operand_NA},
      {"INC IY\n", encoding_INC_IY, opcode_INC, operand_rr, register_IY, operand_NA},
      {"INC BC\n", encoding_INC_ss, opcode_INC, operand_rr, register_BC, operand_NA},
      {"INC DE\n", encoding_INC_ss, opcode_INC, operand_rr, register_DE, operand_NA},
      {"INC HL\n", encoding_INC_ss, opcode_INC, operand_rr, register_HL, operand_NA},
      {"INC SP\n", encoding_INC_ss, opcode_INC, operand_rr, register_SP, operand_NA},
      {"INC (HL)\n", encoding_INC_m, opcode_INC, operand_deref_HL, register_HL, operand_NA},

      {"DEC A\n", encoding_DEC_m, opcode_DEC, operand_r, register_A, operand_NA},
      {"DEC B\n", encoding_DEC_m, opcode_DEC, operand_r, register_B, operand_NA},
      {"DEC C\n", encoding_DEC_m, opcode_DEC, operand_r, register_C, operand_NA},
      {"DEC D\n", encoding_DEC_m, opcode_DEC, operand_r, register_D, operand_NA},
      {"DEC E\n", encoding_DEC_m, opcode_DEC, operand_r, register_E, operand_NA},
      {"DEC H\n", encoding_DEC_m, opcode_DEC, operand_r, register_H, operand_NA},
      {"DEC L\n", encoding_DEC_m, opcode_DEC, operand_r, register_L, operand_NA},
      {"DEC IX\n", encoding_DEC_IX, opcode_DEC, operand_rr, register_IX, operand_NA},
      {"DEC IY\n", encoding_DEC_IY, opcode_DEC, operand_rr, register_IY, operand_NA},
      {"DEC BC\n", encoding_DEC_ss, opcode_DEC, operand_rr, register_BC, operand_NA},
      {"DEC DE\n", encoding_DEC_ss, opcode_DEC, operand_rr, register_DE, operand_NA},
      {"DEC HL\n", encoding_DEC_ss, opcode_DEC, operand_rr, register_HL, operand_NA},
      {"DEC SP\n", encoding_DEC_ss, opcode_DEC, operand_rr, register_SP, operand_NA},
      {"DEC (HL)\n", encoding_DEC_m, opcode_DEC, operand_deref_HL, register_HL, operand_NA},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(statementCount, linkedList_count(statements));
  for (int i = 0; i < statementCount; i++)
  {
    printf("iteration: %d\n", i);
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.rr);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// arithmetics with immediate 8-bit as operand
void test_Arith_n(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    uint8_t expectedImmediate;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"SUB 0\n", encoding_SUB_s, opcode_SUB, operand_n, 0x00, operand_NA},
      {"SUB 0xFF\n", encoding_SUB_s, opcode_SUB, operand_n, 0xFF, operand_NA},

      {"AND 0\n", encoding_AND_s, opcode_AND, operand_n, 0x00, operand_NA},
      {"AND 0xFF\n", encoding_AND_s, opcode_AND, operand_n, 0xFF, operand_NA},

      {"OR 0\n", encoding_OR_s, opcode_OR, operand_n, 0x00, operand_NA},
      {"OR 0xFF\n", encoding_OR_s, opcode_OR, operand_n, 0xFF, operand_NA},

      {"XOR 0\n", encoding_XOR_s, opcode_XOR, operand_n, 0x00, operand_NA},
      {"XOR 0xFF\n", encoding_XOR_s, opcode_XOR, operand_n, 0xFF, operand_NA},

      {"CP 0\n", encoding_CP_s, opcode_CP, operand_n, 0x00, operand_NA},
      {"CP 0xFF\n", encoding_CP_s, opcode_CP, operand_n, 0xFF, operand_NA},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(statementCount, linkedList_count(statements));
  for (int i = 0; i < statementCount; i++)
  {
    printf("iteration: %d\n", i);
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedImmediate, statement->instruction.operand1.data.immediate_n);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// arithmetics with index dereference as operand
void test_Arith_indexed_deref(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister1;
    int8_t expectedOffset;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"SUB (IX-128)\n", encoding_SUB_s, opcode_SUB, operand_deref_idx, register_IX, -128, operand_NA},
      {"SUB (IX+0)\n", encoding_SUB_s, opcode_SUB, operand_deref_idx, register_IX, 0, operand_NA},
      {"SUB (IX+127)\n", encoding_SUB_s, opcode_SUB, operand_deref_idx, register_IX, 127, operand_NA},
      {"SUB (IY-128)\n", encoding_SUB_s, opcode_SUB, operand_deref_idx, register_IY, -128, operand_NA},
      {"SUB (IY+0)\n", encoding_SUB_s, opcode_SUB, operand_deref_idx, register_IY, 0, operand_NA},
      {"SUB (IY+127)\n", encoding_SUB_s, opcode_SUB, operand_deref_idx, register_IY, 127, operand_NA},

      {"AND (IX-128)\n", encoding_AND_s, opcode_AND, operand_deref_idx, register_IX, -128, operand_NA},
      {"AND (IX+0)\n", encoding_AND_s, opcode_AND, operand_deref_idx, register_IX, 0, operand_NA},
      {"AND (IX+127)\n", encoding_AND_s, opcode_AND, operand_deref_idx, register_IX, 127, operand_NA},
      {"AND (IY-128)\n", encoding_AND_s, opcode_AND, operand_deref_idx, register_IY, -128, operand_NA},
      {"AND (IY+0)\n", encoding_AND_s, opcode_AND, operand_deref_idx, register_IY, 0, operand_NA},
      {"AND (IY+127)\n", encoding_AND_s, opcode_AND, operand_deref_idx, register_IY, 127, operand_NA},

      {"OR (IX-128)\n", encoding_OR_s, opcode_OR, operand_deref_idx, register_IX, -128, operand_NA},
      {"OR (IX+0)\n", encoding_OR_s, opcode_OR, operand_deref_idx, register_IX, 0, operand_NA},
      {"OR (IX+127)\n", encoding_OR_s, opcode_OR, operand_deref_idx, register_IX, 127, operand_NA},
      {"OR (IY-128)\n", encoding_OR_s, opcode_OR, operand_deref_idx, register_IY, -128, operand_NA},
      {"OR (IY+0)\n", encoding_OR_s, opcode_OR, operand_deref_idx, register_IY, 0, operand_NA},
      {"OR (IY+127)\n", encoding_OR_s, opcode_OR, operand_deref_idx, register_IY, 127, operand_NA},

      {"XOR (IX-128)\n", encoding_XOR_s, opcode_XOR, operand_deref_idx, register_IX, -128, operand_NA},
      {"XOR (IX+0)\n", encoding_XOR_s, opcode_XOR, operand_deref_idx, register_IX, 0, operand_NA},
      {"XOR (IX+127)\n", encoding_XOR_s, opcode_XOR, operand_deref_idx, register_IX, 127, operand_NA},
      {"XOR (IY-128)\n", encoding_XOR_s, opcode_XOR, operand_deref_idx, register_IY, -128, operand_NA},
      {"XOR (IY+0)\n", encoding_XOR_s, opcode_XOR, operand_deref_idx, register_IY, 0, operand_NA},
      {"XOR (IY+127)\n", encoding_XOR_s, opcode_XOR, operand_deref_idx, register_IY, 127, operand_NA},

      {"CP (IX-128)\n", encoding_CP_s, opcode_CP, operand_deref_idx, register_IX, -128, operand_NA},
      {"CP (IX+0)\n", encoding_CP_s, opcode_CP, operand_deref_idx, register_IX, 0, operand_NA},
      {"CP (IX+127)\n", encoding_CP_s, opcode_CP, operand_deref_idx, register_IX, 127, operand_NA},
      {"CP (IY-128)\n", encoding_CP_s, opcode_CP, operand_deref_idx, register_IY, -128, operand_NA},
      {"CP (IY+0)\n", encoding_CP_s, opcode_CP, operand_deref_idx, register_IY, 0, operand_NA},
      {"CP (IY+127)\n", encoding_CP_s, opcode_CP, operand_deref_idx, register_IY, 127, operand_NA},

      {"INC (IX-128)\n", encoding_INC_m, opcode_INC, operand_deref_idx, register_IX, -128, operand_NA},
      {"INC (IX+0)\n", encoding_INC_m, opcode_INC, operand_deref_idx, register_IX, 0, operand_NA},
      {"INC (IX+127)\n", encoding_INC_m, opcode_INC, operand_deref_idx, register_IX, 127, operand_NA},
      {"INC (IY-128)\n", encoding_INC_m, opcode_INC, operand_deref_idx, register_IY, -128, operand_NA},
      {"INC (IY+0)\n", encoding_INC_m, opcode_INC, operand_deref_idx, register_IY, 0, operand_NA},
      {"INC (IY+127)\n", encoding_INC_m, opcode_INC, operand_deref_idx, register_IY, 127, operand_NA},

      {"DEC (IX-128)\n", encoding_DEC_m, opcode_DEC, operand_deref_idx, register_IX, -128, operand_NA},
      {"DEC (IX+0)\n", encoding_DEC_m, opcode_DEC, operand_deref_idx, register_IX, 0, operand_NA},
      {"DEC (IX+127)\n", encoding_DEC_m, opcode_DEC, operand_deref_idx, register_IX, 127, operand_NA},
      {"DEC (IY-128)\n", encoding_DEC_m, opcode_DEC, operand_deref_idx, register_IY, -128, operand_NA},
      {"DEC (IY+0)\n", encoding_DEC_m, opcode_DEC, operand_deref_idx, register_IY, 0, operand_NA},
      {"DEC (IY+127)\n", encoding_DEC_m, opcode_DEC, operand_deref_idx, register_IY, 127, operand_NA},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(statementCount, linkedList_count(statements));
  for (int i = 0; i < statementCount; i++)
  {
    printf("iteration: %d\n", i);
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.dereference_idx.index_register);
    TEST_ASSERT_EQUAL(driver[i].expectedOffset, statement->instruction.operand1.data.dereference_idx.index);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_Arith_r_r);
  RUN_TEST(test_Arith_A_n);
  RUN_TEST(test_Arith_A_Indexed);
  RUN_TEST(test_Arith_r);
  RUN_TEST(test_Arith_n);
  RUN_TEST(test_Arith_indexed_deref);
  return UNITY_END();
}