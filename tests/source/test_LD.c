#include "lexer/lexer.h"
#include "parser/instruction_encoding.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "types.h"
#include "unity.h"
#include "utility/linked_list.h"
#include <stdio.h>

static FILE *ld_test_file;
static lexer_state_t *lexer;
static parser_t *parser;

static void reset_context()
{
  lexer_reset(lexer);
  parser_reset(parser);
}

void setUp(void)
{
  ld_test_file = fopen("ld-test-file.asm", "w+");
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

void test_LD_r_r(void)
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
      {"LD A, A\n", encoding_LD_r_r, opcode_LD, operand_r, register_A, operand_r, register_A},
      {"LD A, B\n", encoding_LD_r_r, opcode_LD, operand_r, register_A, operand_r, register_B},
      {"LD B, C\n", encoding_LD_r_r, opcode_LD, operand_r, register_B, operand_r, register_C},
      {"LD C, D\n", encoding_LD_r_r, opcode_LD, operand_r, register_C, operand_r, register_D},
      {"LD D, E\n", encoding_LD_r_r, opcode_LD, operand_r, register_D, operand_r, register_E},
      {"LD E, H\n", encoding_LD_r_r, opcode_LD, operand_r, register_E, operand_r, register_H},
      {"LD H, L\n", encoding_LD_r_r, opcode_LD, operand_r, register_H, operand_r, register_L},
  };

  // Write instructions into a file
  for (int i = 0; i < (sizeof(driver) / sizeof(test_driver_t)); i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }
  fseek(ld_test_file, 0, SEEK_SET);

  // Process test file
  lexer_tokenize(lexer, ld_test_file);
  parser_do_it(parser, lexer_getTokenList(lexer));

  statement_list_t *statements = parser_getStatementList(parser);
  ListNode *statementNode = linkedList_getFirstNode(statements);
  statement_t *statement = listNode_getData(statementNode);

  // Test driver entries against parsed statements
  TEST_ASSERT_EQUAL(7, linkedList_count(statements));
  for (int i = 0; i < (sizeof(driver) / sizeof(test_driver_t)); i++)
  {
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister2, statement->instruction.operand2.data.r);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// not needed when using generate_test_runner.rb
int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_LD_r_r);
  return UNITY_END();
}