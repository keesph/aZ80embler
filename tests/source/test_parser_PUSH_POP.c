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
static char *testFileName = "ld-test-file.asm";
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

void test_PUSH_POP(void)
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
      {"PUSH BC\n", encoding_PUSH_qq, opcode_PUSH, operand_rr, register_BC, operand_NA},
      {"PUSH DE\n", encoding_PUSH_qq, opcode_PUSH, operand_rr, register_DE, operand_NA},
      {"PUSH HL\n", encoding_PUSH_qq, opcode_PUSH, operand_rr, register_HL, operand_NA},
      {"PUSH AF\n", encoding_PUSH_qq, opcode_PUSH, operand_rr, register_AF, operand_NA},
      {"PUSH IX\n", encoding_PUSH_IX, opcode_PUSH, operand_rr, register_IX, operand_NA},
      {"PUSH IY\n", encoding_PUSH_IY, opcode_PUSH, operand_rr, register_IY, operand_NA},
      {"POP BC\n", encoding_POP_qq, opcode_POP, operand_rr, register_BC, operand_NA},
      {"POP DE\n", encoding_POP_qq, opcode_POP, operand_rr, register_DE, operand_NA},
      {"POP HL\n", encoding_POP_qq, opcode_POP, operand_rr, register_HL, operand_NA},
      {"POP AF\n", encoding_POP_qq, opcode_POP, operand_rr, register_AF, operand_NA},
      {"POP IX\n", encoding_POP_IX, opcode_POP, operand_rr, register_IX, operand_NA},
      {"POP IY\n", encoding_POP_IY, opcode_POP, operand_rr, register_IY, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedEncoding, statement->instruction.encoding);
    TEST_ASSERT_EQUAL(driver[i].expectedOpCode, statement->instruction.opcode);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand1, statement->instruction.operand1.type);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.r);

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
  RUN_TEST(test_PUSH_POP);
  return UNITY_END();
}