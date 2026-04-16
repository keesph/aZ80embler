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

// Call and Return group instructions without operands
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
      {"RET\n", encoding_RET, opcode_RET, operand_NA, operand_NA},
      {"RETI\n", encoding_RETI, opcode_RETI, operand_NA, operand_NA},
      {"RETN\n", encoding_RETN, opcode_RETN, operand_NA, operand_NA},
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

// Call with nn
void test_call_nn(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    uint16_t expectedImmediate;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"CALL 0x00\n", encoding_CALL_nn, opcode_CALL, operand_n, 0x00, operand_NA},
      {"CALL 0xFF\n", encoding_CALL_nn, opcode_CALL, operand_n, 0xFF, operand_NA},
      {"CALL 0x01FF\n", encoding_CALL_nn, opcode_CALL, operand_nn, 0x01FF, operand_NA},
      {"CALL 0xFFFF\n", encoding_CALL_nn, opcode_CALL, operand_nn, 0xFFFF, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedImmediate, statement->instruction.operand1.data.immediate_nn);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// Call with cc, nn
void test_call_cc_nn(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
    uint16_t expectedImmediate;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"CALL NZ, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_NZ, operand_nn, 0xABCD},
      {"CALL Z, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_Z, operand_nn, 0xABCD},
      {"CALL NC, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_NC, operand_nn, 0xABCD},
      {"CALL C, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_C, operand_nn, 0xABCD},
      {"CALL PO, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_PO, operand_nn, 0xABCD},
      {"CALL PE, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_PE, operand_nn, 0xABCD},
      {"CALL P, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_P, operand_nn, 0xABCD},
      {"CALL M, 0xABCD\n", encoding_CALL_cc_nn, opcode_CALL, operand_r, register_M, operand_nn, 0xABCD},
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
    TEST_ASSERT_EQUAL(driver[i].expectedImmediate, statement->instruction.operand2.data.immediate_nn);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// RET cc
void test_ret_cc(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"RET NZ\n", encoding_RET_cc, opcode_RET, operand_r, register_NZ, operand_NA},
      {"RET Z\n", encoding_RET_cc, opcode_RET, operand_r, register_Z, operand_NA},
      {"RET NC\n", encoding_RET_cc, opcode_RET, operand_r, register_NC, operand_NA},
      {"RET C\n", encoding_RET_cc, opcode_RET, operand_r, register_C, operand_NA},
      {"RET PO\n", encoding_RET_cc, opcode_RET, operand_r, register_PO, operand_NA},
      {"RET PE\n", encoding_RET_cc, opcode_RET, operand_r, register_PE, operand_NA},
      {"RET P\n", encoding_RET_cc, opcode_RET, operand_r, register_P, operand_NA},
      {"RET M\n", encoding_RET_cc, opcode_RET, operand_r, register_M, operand_NA},
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
    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// RET cc
void test_rst_p(void)
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
      {"RST 0x00\n", encoding_RST_p, opcode_RST, operand_n, 0x00, operand_NA},
      {"RST 0x08\n", encoding_RST_p, opcode_RST, operand_n, 0x08, operand_NA},
      {"RST 0x10\n", encoding_RST_p, opcode_RST, operand_n, 0x10, operand_NA},
      {"RST 0x18\n", encoding_RST_p, opcode_RST, operand_n, 0x18, operand_NA},
      {"RST 0x20\n", encoding_RST_p, opcode_RST, operand_n, 0x20, operand_NA},
      {"RST 0x28\n", encoding_RST_p, opcode_RST, operand_n, 0x28, operand_NA},
      {"RST 0x30\n", encoding_RST_p, opcode_RST, operand_n, 0x30, operand_NA},
      {"RST 0x38\n", encoding_RST_p, opcode_RST, operand_n, 0x38, operand_NA},
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

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_no_operand);
  RUN_TEST(test_call_nn);
  RUN_TEST(test_call_cc_nn);
  RUN_TEST(test_ret_cc);
  RUN_TEST(test_rst_p);
  return UNITY_END();
}