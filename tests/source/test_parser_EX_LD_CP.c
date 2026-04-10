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

// EX instruction with two 16 bit operands
void test_EX_rr_rr(void)
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
      {"EX DE, HL\n", encoding_EX_DE_HL, opcode_EX, operand_rr, register_DE, operand_rr, register_HL},
      {"EX AF, AF\n", encoding_EX_AF_AF, opcode_EX, operand_rr, register_AF, operand_rr, register_AF},
      {"EX (SP), HL\n", encoding_EX_derefSP_HL, opcode_EX, operand_deref_rr, register_SP, operand_rr, register_HL},
      {"EX (SP), IX\n", encoding_EX_derefSP_IX, opcode_EX, operand_deref_rr, register_SP, operand_rr, register_IX},
      {"EX (SP), IY\n", encoding_EX_derefSP_IY, opcode_EX, operand_deref_rr, register_SP, operand_rr, register_IY},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister1, statement->instruction.operand1.data.rr);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister2, statement->instruction.operand2.data.rr);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

// EXX, LDIx, LDDx, CPx instruction without operands
void test_EX_LD_CP_na_na(void)
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
      {"EXX\n", encoding_EXX, opcode_EXX, operand_NA, operand_NA},
      {"LDI\n", encoding_LDI, opcode_LDI, operand_NA, operand_NA},
      {"LDIR\n", encoding_LDIR, opcode_LDIR, operand_NA, operand_NA},
      {"LDD\n", encoding_LDD, opcode_LDD, operand_NA, operand_NA},
      {"LDDR\n", encoding_LDDR, opcode_LDDR, operand_NA, operand_NA},
      {"CPI\n", encoding_CPI, opcode_CPI, operand_NA, operand_NA},
      {"CPIR\n", encoding_CPIR, opcode_CPIR, operand_NA, operand_NA},
      {"CPD\n", encoding_CPD, opcode_CPD, operand_NA, operand_NA},
      {"CPDR\n", encoding_CPDR, opcode_CPDR, operand_NA, operand_NA},
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
  RUN_TEST(test_EX_rr_rr);
  RUN_TEST(test_EX_LD_CP_na_na);
  return UNITY_END();
}