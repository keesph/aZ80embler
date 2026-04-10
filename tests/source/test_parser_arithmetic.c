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

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_Arith_r_r);
  RUN_TEST(test_Arith_A_n);
  RUN_TEST(test_Arith_A_Indexed);
  return UNITY_END();
}