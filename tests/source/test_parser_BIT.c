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

// BIT set, reset and test operations with r as operand
void test_operand_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    uint8_t expectedBit;
    operand_type_t expectedOperand2;
    register_type_t expectedregister;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"BIT 0, A\n", encoding_BIT_b_r, opcode_BIT, operand_n, 0, operand_r, register_A},
      {"BIT 1, B\n", encoding_BIT_b_r, opcode_BIT, operand_n, 1, operand_r, register_B},
      {"BIT 2, C\n", encoding_BIT_b_r, opcode_BIT, operand_n, 2, operand_r, register_C},
      {"BIT 3, D\n", encoding_BIT_b_r, opcode_BIT, operand_n, 3, operand_r, register_D},
      {"BIT 4, E\n", encoding_BIT_b_r, opcode_BIT, operand_n, 4, operand_r, register_E},
      {"BIT 5, H\n", encoding_BIT_b_r, opcode_BIT, operand_n, 5, operand_r, register_H},
      {"BIT 6, L\n", encoding_BIT_b_r, opcode_BIT, operand_n, 6, operand_r, register_L},
      {"BIT 7, (HL)\n", encoding_BIT_b_derefHL, opcode_BIT, operand_n, 7, operand_deref_HL, register_HL},

      {"SET 0, A\n", encoding_SET_b_r, opcode_SET, operand_n, 0, operand_r, register_A},
      {"SET 1, B\n", encoding_SET_b_r, opcode_SET, operand_n, 1, operand_r, register_B},
      {"SET 2, C\n", encoding_SET_b_r, opcode_SET, operand_n, 2, operand_r, register_C},
      {"SET 3, D\n", encoding_SET_b_r, opcode_SET, operand_n, 3, operand_r, register_D},
      {"SET 4, E\n", encoding_SET_b_r, opcode_SET, operand_n, 4, operand_r, register_E},
      {"SET 5, H\n", encoding_SET_b_r, opcode_SET, operand_n, 5, operand_r, register_H},
      {"SET 6, L\n", encoding_SET_b_r, opcode_SET, operand_n, 6, operand_r, register_L},
      {"SET 7, (HL)\n", encoding_SET_b_derefHL, opcode_SET, operand_n, 7, operand_deref_HL, register_HL},

      {"RES 0, A\n", encoding_RES_b_m, opcode_RES, operand_n, 0, operand_r, register_A},
      {"RES 1, B\n", encoding_RES_b_m, opcode_RES, operand_n, 1, operand_r, register_B},
      {"RES 2, C\n", encoding_RES_b_m, opcode_RES, operand_n, 2, operand_r, register_C},
      {"RES 3, D\n", encoding_RES_b_m, opcode_RES, operand_n, 3, operand_r, register_D},
      {"RES 4, E\n", encoding_RES_b_m, opcode_RES, operand_n, 4, operand_r, register_E},
      {"RES 5, H\n", encoding_RES_b_m, opcode_RES, operand_n, 5, operand_r, register_H},
      {"RES 6, L\n", encoding_RES_b_m, opcode_RES, operand_n, 6, operand_r, register_L},
      {"RES 7, (HL)\n", encoding_RES_b_m, opcode_RES, operand_n, 7, operand_deref_HL, register_HL},
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
    TEST_ASSERT_EQUAL(driver[i].expectedBit, statement->instruction.operand1.data.immediate_n);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedregister, statement->instruction.operand2.data.r);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// Rotate/Shift operations with (IX/Y+d) as operand
void test_operand_idx(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    uint8_t expectedbit;
    operand_type_t expectedOperand2;
    register_type_t expectedregister;
    int8_t expectedOffset;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"BIT 0, (IX+10)\n", encoding_BIT_b_derefIXd, opcode_BIT, operand_n, 0, operand_deref_idx, register_IX, 10},
      {"BIT 0, (IY-10)\n", encoding_BIT_b_derefIYd, opcode_BIT, operand_n, 0, operand_deref_idx, register_IY, -10},

      {"SET 0, (IX+10)\n", encoding_SET_b_derefIXd, opcode_SET, operand_n, 0, operand_deref_idx, register_IX, 10},
      {"SET 0, (IY-10)\n", encoding_SET_b_derefIYd, opcode_SET, operand_n, 0, operand_deref_idx, register_IY, -10},

      {"RES 0, (IX+10)\n", encoding_RES_b_m, opcode_RES, operand_n, 0, operand_deref_idx, register_IX, 10},
      {"RES 0, (IY-10)\n", encoding_RES_b_m, opcode_RES, operand_n, 0, operand_deref_idx, register_IY, -10},
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
    TEST_ASSERT_EQUAL(driver[i].expectedbit, statement->instruction.operand1.data.immediate_n);
    TEST_ASSERT_EQUAL(driver[i].expectedregister, statement->instruction.operand2.data.dereference_idx.index_register);
    TEST_ASSERT_EQUAL(driver[i].expectedOffset, statement->instruction.operand2.data.dereference_idx.index);
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
  RUN_TEST(test_operand_r);
  RUN_TEST(test_operand_idx);
  return UNITY_END();
}