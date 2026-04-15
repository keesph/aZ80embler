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

// Rotate/Shift operations without operand
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
      {"RLCA\n", encoding_RLCA, opcode_RLCA, operand_NA, operand_NA},
      {"RLA\n", encoding_RLA, opcode_RLA, operand_NA, operand_NA},
      {"RRCA\n", encoding_RRCA, opcode_RRCA, operand_NA, operand_NA},
      {"RRA\n", encoding_RRA, opcode_RRA, operand_NA, operand_NA},
      {"RLD\n", encoding_RLD, opcode_RLD, operand_NA, operand_NA},
      {"RRD\n", encoding_RRD, opcode_RRD, operand_NA, operand_NA},
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

// Rotate/Shift operations with r as operand
void test_operand_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedregister;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"RLC A\n", encoding_RLC_r, opcode_RLC, operand_r, register_A, operand_NA},
      {"RLC B\n", encoding_RLC_r, opcode_RLC, operand_r, register_B, operand_NA},
      {"RLC C\n", encoding_RLC_r, opcode_RLC, operand_r, register_C, operand_NA},
      {"RLC D\n", encoding_RLC_r, opcode_RLC, operand_r, register_D, operand_NA},
      {"RLC E\n", encoding_RLC_r, opcode_RLC, operand_r, register_E, operand_NA},
      {"RLC H\n", encoding_RLC_r, opcode_RLC, operand_r, register_H, operand_NA},
      {"RLC L\n", encoding_RLC_r, opcode_RLC, operand_r, register_L, operand_NA},
      {"RLC (HL)\n", encoding_RLC_derefHL, opcode_RLC, operand_deref_HL, register_HL, operand_NA},

      {"RL A\n", encoding_RL_m, opcode_RL, operand_r, register_A, operand_NA},
      {"RL B\n", encoding_RL_m, opcode_RL, operand_r, register_B, operand_NA},
      {"RL C\n", encoding_RL_m, opcode_RL, operand_r, register_C, operand_NA},
      {"RL D\n", encoding_RL_m, opcode_RL, operand_r, register_D, operand_NA},
      {"RL E\n", encoding_RL_m, opcode_RL, operand_r, register_E, operand_NA},
      {"RL H\n", encoding_RL_m, opcode_RL, operand_r, register_H, operand_NA},
      {"RL L\n", encoding_RL_m, opcode_RL, operand_r, register_L, operand_NA},
      {"RL (HL)\n", encoding_RL_m, opcode_RL, operand_deref_HL, register_HL, operand_NA},

      {"RRC A\n", encoding_RRC_m, opcode_RRC, operand_r, register_A, operand_NA},
      {"RRC B\n", encoding_RRC_m, opcode_RRC, operand_r, register_B, operand_NA},
      {"RRC C\n", encoding_RRC_m, opcode_RRC, operand_r, register_C, operand_NA},
      {"RRC D\n", encoding_RRC_m, opcode_RRC, operand_r, register_D, operand_NA},
      {"RRC E\n", encoding_RRC_m, opcode_RRC, operand_r, register_E, operand_NA},
      {"RRC H\n", encoding_RRC_m, opcode_RRC, operand_r, register_H, operand_NA},
      {"RRC L\n", encoding_RRC_m, opcode_RRC, operand_r, register_L, operand_NA},
      {"RRC (HL)\n", encoding_RRC_m, opcode_RRC, operand_deref_HL, register_HL, operand_NA},

      {"RR A\n", encoding_RR_m, opcode_RR, operand_r, register_A, operand_NA},
      {"RR B\n", encoding_RR_m, opcode_RR, operand_r, register_B, operand_NA},
      {"RR C\n", encoding_RR_m, opcode_RR, operand_r, register_C, operand_NA},
      {"RR D\n", encoding_RR_m, opcode_RR, operand_r, register_D, operand_NA},
      {"RR E\n", encoding_RR_m, opcode_RR, operand_r, register_E, operand_NA},
      {"RR H\n", encoding_RR_m, opcode_RR, operand_r, register_H, operand_NA},
      {"RR L\n", encoding_RR_m, opcode_RR, operand_r, register_L, operand_NA},
      {"RR (HL)\n", encoding_RR_m, opcode_RR, operand_deref_HL, register_HL, operand_NA},

      {"SLA A\n", encoding_SLA_m, opcode_SLA, operand_r, register_A, operand_NA},
      {"SLA B\n", encoding_SLA_m, opcode_SLA, operand_r, register_B, operand_NA},
      {"SLA C\n", encoding_SLA_m, opcode_SLA, operand_r, register_C, operand_NA},
      {"SLA D\n", encoding_SLA_m, opcode_SLA, operand_r, register_D, operand_NA},
      {"SLA E\n", encoding_SLA_m, opcode_SLA, operand_r, register_E, operand_NA},
      {"SLA H\n", encoding_SLA_m, opcode_SLA, operand_r, register_H, operand_NA},
      {"SLA L\n", encoding_SLA_m, opcode_SLA, operand_r, register_L, operand_NA},
      {"SLA (HL)\n", encoding_SLA_m, opcode_SLA, operand_deref_HL, register_HL, operand_NA},

      {"SRA A\n", encoding_SRA_m, opcode_SRA, operand_r, register_A, operand_NA},
      {"SRA B\n", encoding_SRA_m, opcode_SRA, operand_r, register_B, operand_NA},
      {"SRA C\n", encoding_SRA_m, opcode_SRA, operand_r, register_C, operand_NA},
      {"SRA D\n", encoding_SRA_m, opcode_SRA, operand_r, register_D, operand_NA},
      {"SRA E\n", encoding_SRA_m, opcode_SRA, operand_r, register_E, operand_NA},
      {"SRA H\n", encoding_SRA_m, opcode_SRA, operand_r, register_H, operand_NA},
      {"SRA L\n", encoding_SRA_m, opcode_SRA, operand_r, register_L, operand_NA},
      {"SRA (HL)\n", encoding_SRA_m, opcode_SRA, operand_deref_HL, register_HL, operand_NA},

      {"SRL A\n", encoding_SRL_m, opcode_SRL, operand_r, register_A, operand_NA},
      {"SRL B\n", encoding_SRL_m, opcode_SRL, operand_r, register_B, operand_NA},
      {"SRL C\n", encoding_SRL_m, opcode_SRL, operand_r, register_C, operand_NA},
      {"SRL D\n", encoding_SRL_m, opcode_SRL, operand_r, register_D, operand_NA},
      {"SRL E\n", encoding_SRL_m, opcode_SRL, operand_r, register_E, operand_NA},
      {"SRL H\n", encoding_SRL_m, opcode_SRL, operand_r, register_H, operand_NA},
      {"SRL L\n", encoding_SRL_m, opcode_SRL, operand_r, register_L, operand_NA},
      {"SRL (HL)\n", encoding_SRL_m, opcode_SRL, operand_deref_HL, register_HL, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedregister, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expectedOperand2, statement->instruction.operand2.type);

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
    register_type_t expectedregister;
    int8_t expectedOffset;
    operand_type_t expectedOperand2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"RLC (IX+10)\n", encoding_RLC_derefIXd, opcode_RLC, operand_deref_idx, register_IX, 10, operand_NA},
      {"RLC (IY-10)\n", encoding_RLC_derefIYd, opcode_RLC, operand_deref_idx, register_IY, -10, operand_NA},

      {"RL (IX+10)\n", encoding_RL_m, opcode_RL, operand_deref_idx, register_IX, 10, operand_NA},
      {"RL (IY-10)\n", encoding_RL_m, opcode_RL, operand_deref_idx, register_IY, -10, operand_NA},

      {"RRC (IX+10)\n", encoding_RRC_m, opcode_RRC, operand_deref_idx, register_IX, 10, operand_NA},
      {"RRC (IY-10)\n", encoding_RRC_m, opcode_RRC, operand_deref_idx, register_IY, -10, operand_NA},

      {"RR (IX+10)\n", encoding_RR_m, opcode_RR, operand_deref_idx, register_IX, 10, operand_NA},
      {"RR (IY-10)\n", encoding_RR_m, opcode_RR, operand_deref_idx, register_IY, -10, operand_NA},

      {"SLA (IX+10)\n", encoding_SLA_m, opcode_SLA, operand_deref_idx, register_IX, 10, operand_NA},
      {"SLA (IY-10)\n", encoding_SLA_m, opcode_SLA, operand_deref_idx, register_IY, -10, operand_NA},

      {"SRA (IX+10)\n", encoding_SRA_m, opcode_SRA, operand_deref_idx, register_IX, 10, operand_NA},
      {"SRA (IY-10)\n", encoding_SRA_m, opcode_SRA, operand_deref_idx, register_IY, -10, operand_NA},

      {"SRL (IX+10)\n", encoding_SRL_m, opcode_SRL, operand_deref_idx, register_IX, 10, operand_NA},
      {"SRL (IY-10)\n", encoding_SRL_m, opcode_SRL, operand_deref_idx, register_IY, -10, operand_NA},
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
    TEST_ASSERT_EQUAL(driver[i].expectedregister, statement->instruction.operand1.data.dereference_idx.index_register);
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
  RUN_TEST(test_no_operand);
  RUN_TEST(test_operand_r);
  RUN_TEST(test_operand_idx);
  return UNITY_END();
}