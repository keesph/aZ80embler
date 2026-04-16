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

// Input and Output group instructions without operands
void test_no_operand(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"INI\n", encoding_INI, opcode_INI, operand_NA, operand_NA},
      {"INIR\n", encoding_INIR, opcode_INIR, operand_NA, operand_NA},
      {"IND\n", encoding_IND, opcode_IND, operand_NA, operand_NA},
      {"INDR\n", encoding_INDR, opcode_INDR, operand_NA, operand_NA},
      {"OUTI\n", encoding_OUTI, opcode_OUTI, operand_NA, operand_NA},
      {"OTIR\n", encoding_OTIR, opcode_OTIR, operand_NA, operand_NA},
      {"OUTD\n", encoding_OUTD, opcode_OUTD, operand_NA, operand_NA},
      {"OTDR\n", encoding_OTDR, opcode_OTDR, operand_NA, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// Input and Output group instructions without operands
void test_IN_a_deref_n(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
    uint8_t expectedImmediate;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"IN A, (0x00)\n", encoding_IN_A_derefn, opcode_IN, operand_r, register_A, operand_deref_n, 0},
      {"IN A, (0xFF)\n", encoding_IN_A_derefn, opcode_IN, operand_r, register_A, operand_deref_n, 0xFF},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedImmediate, statement->instruction.operand2.data.immediate_n);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// Input and Output group instructions without operands
void test_IN_r_deref_c(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
    uint8_t expectedregister2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"IN A, (C)\n", encoding_IN_r_derefC, opcode_IN, operand_r, register_A, operand_deref_C, register_C},
      {"IN B, (C)\n", encoding_IN_r_derefC, opcode_IN, operand_r, register_B, operand_deref_C, register_C},
      {"IN C, (C)\n", encoding_IN_r_derefC, opcode_IN, operand_r, register_C, operand_deref_C, register_C},
      {"IN D, (C)\n", encoding_IN_r_derefC, opcode_IN, operand_r, register_D, operand_deref_C, register_C},
      {"IN H, (C)\n", encoding_IN_r_derefC, opcode_IN, operand_r, register_H, operand_deref_C, register_C},
      {"IN L, (C)\n", encoding_IN_r_derefC, opcode_IN, operand_r, register_L, operand_deref_C, register_C},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedregister2, statement->instruction.operand2.data.r);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_IN_deref_n_A(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    uint8_t expectedImmediate;
    operand_type_t expectedOperand2;
    register_type_t expectedRegister;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"OUT (0x00), A\n", encoding_OUT_derefn_A, opcode_OUT, operand_deref_n, 0x00, operand_r, register_A},
      {"OUT (0xFF), A\n", encoding_OUT_derefn_A, opcode_OUT, operand_deref_n, 0xFF, operand_r, register_A},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister, statement->instruction.operand2.data.r);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_OUT_deref_c_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
    uint8_t expectedregister2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"OUT (C), A\n", encoding_OUT_derefC_r, opcode_OUT, operand_deref_C, register_C, operand_r, register_A},
      {"OUT (C), B\n", encoding_OUT_derefC_r, opcode_OUT, operand_deref_C, register_C, operand_r, register_B},
      {"OUT (C), C\n", encoding_OUT_derefC_r, opcode_OUT, operand_deref_C, register_C, operand_r, register_C},
      {"OUT (C), D\n", encoding_OUT_derefC_r, opcode_OUT, operand_deref_C, register_C, operand_r, register_D},
      {"OUT (C), H\n", encoding_OUT_derefC_r, opcode_OUT, operand_deref_C, register_C, operand_r, register_H},
      {"OUT (C), L\n", encoding_OUT_derefC_r, opcode_OUT, operand_deref_C, register_C, operand_r, register_L},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedregister2, statement->instruction.operand2.data.r);

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
  RUN_TEST(test_no_operand);
  RUN_TEST(test_IN_a_deref_n);
  RUN_TEST(test_IN_r_deref_c);
  RUN_TEST(test_IN_deref_n_A);
  RUN_TEST(test_OUT_deref_c_r);
  return UNITY_END();
}