#include "unity.h"

#include "lexer/lexer.h"
#include "logging/logging.h"
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

// JMP with 16 bit immediate operand
void test_jp_nn(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    uint16_t expectedAddress;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"JP 0\n", encoding_JP_nn, opcode_JP, operand_n, 0, operand_NA},
      {"JP 255\n", encoding_JP_nn, opcode_JP, operand_n, 0xFF, operand_NA},
      {"JP 0x0100\n", encoding_JP_nn, opcode_JP, operand_nn, 0x0100, operand_NA},
      {"JP 0xFFFF\n", encoding_JP_nn, opcode_JP, operand_nn, 0xFFFF, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedAddress, statement->instruction.operand1.data.immediate_nn);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// Conditional JMP with 16 bit immediate operand
void test_jp_cc_nn(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
    uint16_t expectedAddress;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"JP NZ, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_NZ, operand_nn, 0xAABB},
      {"JP Z, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_Z, operand_nn, 0xAABB},
      {"JP NC, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_NC, operand_nn, 0xAABB},
      {"JP C, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_C, operand_nn, 0xAABB},
      {"JP PO, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_PO, operand_nn, 0xAABB},
      {"JP PE, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_PE, operand_nn, 0xAABB},
      {"JP P, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_P, operand_nn, 0xAABB},
      {"JP M, 0xAABB\n", encoding_JP_cc_nn, opcode_JP, operand_r, register_M, operand_nn, 0xAABB},
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
    TEST_ASSERT_EQUAL(driver[i].expectedAddress, statement->instruction.operand2.data.immediate_nn);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// JMP with deref register operand
void test_jp_deref_r(void)
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
      {"JP (HL)\n", encoding_JP_derefHL, opcode_JP, operand_deref_HL, register_HL, operand_NA},
      {"JP (IX)\n", encoding_JP_derefIX, opcode_JP, operand_deref_IX_IY, register_IX, operand_NA},
      {"JP (IY)\n", encoding_JP_derefIY, opcode_JP, operand_deref_IX_IY, register_IY, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister, statement->instruction.operand1.data.rr);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// jump with immediate e operand
void test_jr_djnz_e(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    int16_t expectedOffset;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"JR -126\n", encoding_JR_e, opcode_JR, operand_e, -126},
      {"JR 0\n", encoding_JR_e, opcode_JR, operand_n, 0},
      {"JR 129\n", encoding_JR_e, opcode_JR, operand_n, 129},

      {"DJNZ -126\n", encoding_DJNZ_e, opcode_DJNZ, operand_e, -126},
      {"DJNZ 0\n", encoding_DJNZ_e, opcode_DJNZ, operand_n, 0},
      {"DJNZ 129\n", encoding_DJNZ_e, opcode_DJNZ, operand_n, 129},
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
    TEST_ASSERT_EQUAL(driver[i].expectedOffset, statement->instruction.operand1.data.relative_offset_e);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// jump with immediate e operand
void test_jr_cc_e(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister;
    operand_type_t expectedOperand2;
    int16_t expectedOffset;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"JR C, -126\n", encoding_JR_C_e, opcode_JR, operand_r, register_C, operand_e, -126},
      {"JR NC, 0\n", encoding_JR_NC_e, opcode_JR, operand_r, register_NC, operand_n, 0},
      {"JR Z, 129\n", encoding_JR_Z_e, opcode_JR, operand_r, register_Z, operand_n, 129},
      {"JR NZ, 129\n", encoding_JR_NZ_e, opcode_JR, operand_r, register_NZ, operand_n, 129},
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
    TEST_ASSERT_EQUAL(driver[i].expectedOffset, statement->instruction.operand2.data.relative_offset_e);

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
  RUN_TEST(test_jp_nn);
  RUN_TEST(test_jp_cc_nn);
  RUN_TEST(test_jp_deref_r);
  RUN_TEST(test_jr_djnz_e);
  RUN_TEST(test_jr_cc_e);
  return UNITY_END();
}