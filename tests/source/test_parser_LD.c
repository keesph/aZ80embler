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
      {"LD A, I\n", encoding_LD_A_I, opcode_LD, operand_r, register_A, operand_I, register_I},
      {"LD A, R\n", encoding_LD_A_R, opcode_LD, operand_r, register_A, operand_R, register_R},
      {"LD I, A\n", encoding_LD_I_A, opcode_LD, operand_I, register_I, operand_r, register_A},
      {"LD R, A\n", encoding_LD_R_A, opcode_LD, operand_R, register_R, operand_r, register_A},
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
    TEST_ASSERT_EQUAL(driver[i].expectedRegister2, statement->instruction.operand2.data.r);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_LD_r_n(void)
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
      {"LD A, 0\n", encoding_LD_r_n, opcode_LD, operand_r, register_A, operand_n, 0},
      {"LD B, 255\n", encoding_LD_r_n, opcode_LD, operand_r, register_B, operand_n, 255},
      {"LD C, 128\n", encoding_LD_r_n, opcode_LD, operand_r, register_C, operand_n, 128},
      {"LD D, 10\n", encoding_LD_r_n, opcode_LD, operand_r, register_D, operand_n, 10},
      {"LD E, 5\n", encoding_LD_r_n, opcode_LD, operand_r, register_E, operand_n, 5},
      {"LD H, 19\n", encoding_LD_r_n, opcode_LD, operand_r, register_H, operand_n, 19},
      {"LD L, 11\n", encoding_LD_r_n, opcode_LD, operand_r, register_L, operand_n, 11},
      {"LD (HL), 11\n", encoding_LD_derefHL_n, opcode_LD, operand_deref_HL, register_HL, operand_n, 11},
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
    TEST_ASSERT_EQUAL(driver[i].expectedImmediate, statement->instruction.operand2.data.immediate_n);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_LD_derefrr(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedRegister1;
    operand_type_t expectedOperand2;
    register_type_t expectedrr;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LD H, (HL)\n", encoding_LD_r_derefHL, opcode_LD, operand_r, register_H, operand_deref_HL, register_HL},
      {"LD (HL), B\n", encoding_LD_derefHL_r, opcode_LD, operand_deref_HL, register_HL, operand_r, register_B},
      {"LD A, (BC)\n", encoding_LD_A_derefBC, opcode_LD, operand_r, register_A, operand_deref_rr, register_BC},
      {"LD A, (DE)\n", encoding_LD_A_derefDE, opcode_LD, operand_r, register_A, operand_deref_rr, register_DE},
      {"LD (BC), A\n", encoding_LD_derefBC_A, opcode_LD, operand_deref_rr, register_BC, operand_r, register_A},
      {"LD (DE), A\n", encoding_LD_derefDE_A, opcode_LD, operand_deref_rr, register_DE, operand_r, register_A},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  TEST_ASSERT_TRUE(lexer_tokenize(lexer, ld_test_file));
  TEST_ASSERT_TRUE(parser_do_it(parser, lexer_getTokenList(lexer)));

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
    TEST_ASSERT_EQUAL(driver[i].expectedrr, statement->instruction.operand2.data.rr);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_LD_reg_idx(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedregister;
    operand_type_t expectedOperand2;
    register_type_t expectedidx;
    int8_t expectedIndex;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LD A, (IX+0)\n", encoding_LD_r_derefIXd, opcode_LD, operand_r, register_A, operand_deref_idx, register_IX, 0},
      {"LD B, (IX+127)\n", encoding_LD_r_derefIXd, opcode_LD, operand_r, register_B, operand_deref_idx, register_IX,
       127},
      {"LD B, (IY-0)\n", encoding_LD_r_derefIYd, opcode_LD, operand_r, register_B, operand_deref_idx, register_IY, 0},
      {"LD B, (IY-128)\n", encoding_LD_r_derefIYd, opcode_LD, operand_r, register_B, operand_deref_idx, register_IY,
       -128},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  TEST_ASSERT_TRUE(lexer_tokenize(lexer, ld_test_file));
  TEST_ASSERT_TRUE(parser_do_it(parser, lexer_getTokenList(lexer)));

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
    TEST_ASSERT_EQUAL(driver[i].expectedregister, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expectedidx, statement->instruction.operand2.data.dereference_idx.index_register);
    TEST_ASSERT_EQUAL(driver[i].expectedIndex, statement->instruction.operand2.data.dereference_idx.index);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_LD_idx_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedregister1;
    int8_t expectedIndex;
    operand_type_t expectedOperand2;
    register_type_t expectedRegister2;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LD (IX+0), A\n", encoding_LD_derefIXd_r, opcode_LD, operand_deref_idx, register_IX, 0, operand_r, register_A},
      {"LD (IX+127), B\n", encoding_LD_derefIXd_r, opcode_LD, operand_deref_idx, register_IX, 127, operand_r,
       register_B},
      {"LD (IX-128), C\n", encoding_LD_derefIXd_r, opcode_LD, operand_deref_idx, register_IX, -128, operand_r,
       register_C},
      {"LD (IY+0), D\n", encoding_LD_derefIYd_r, opcode_LD, operand_deref_idx, register_IY, 0, operand_r, register_D},
      {"LD (IY+127), E\n", encoding_LD_derefIYd_r, opcode_LD, operand_deref_idx, register_IY, 127, operand_r,
       register_E},
      {"LD (IY-128), H\n", encoding_LD_derefIYd_r, opcode_LD, operand_deref_idx, register_IY, -128, operand_r,
       register_H},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  TEST_ASSERT_TRUE(lexer_tokenize(lexer, ld_test_file));
  TEST_ASSERT_TRUE(parser_do_it(parser, lexer_getTokenList(lexer)));

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
    TEST_ASSERT_EQUAL(driver[i].expectedregister1, statement->instruction.operand1.data.dereference_idx.index_register);
    TEST_ASSERT_EQUAL(driver[i].expectedIndex, statement->instruction.operand1.data.dereference_idx.index);
    TEST_ASSERT_EQUAL(driver[i].expectedRegister2, statement->instruction.operand2.data.r);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_LD_idx_n(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedregister1;
    int8_t expectedIndex;
    operand_type_t expectedOperand2;
    uint8_t expextedImmediate;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LD (IX+0), 0\n", encoding_LD_derefIXd_n, opcode_LD, operand_deref_idx, register_IX, 0, operand_n, 0},
      {"LD (IX+127), 255\n", encoding_LD_derefIXd_n, opcode_LD, operand_deref_idx, register_IX, 127, operand_n, 255},
      {"LD (IY+0), 0\n", encoding_LD_derefIYd_n, opcode_LD, operand_deref_idx, register_IY, 0, operand_n, 0},
      {"LD (IY+127), 255\n", encoding_LD_derefIYd_n, opcode_LD, operand_deref_idx, register_IY, 127, operand_n, 255},
  };
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  TEST_ASSERT_TRUE(lexer_tokenize(lexer, ld_test_file));
  TEST_ASSERT_TRUE(parser_do_it(parser, lexer_getTokenList(lexer)));

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
    TEST_ASSERT_EQUAL(driver[i].expectedregister1, statement->instruction.operand1.data.dereference_idx.index_register);
    TEST_ASSERT_EQUAL(driver[i].expectedIndex, statement->instruction.operand1.data.dereference_idx.index);
    TEST_ASSERT_EQUAL(driver[i].expextedImmediate, statement->instruction.operand2.data.immediate_n);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

static void token_iterateCb(void *token, size_t iteration)
{
  char *string;
  token_toString(((token_t *)token)->type, &string);
  LOG_INFO("%d: %s", ++iteration, string);
  free(string);
}

void test_LD_r_nn(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    register_type_t expectedregister1;
    operand_type_t expectedOperand2;
    uint16_t expextedImmediate;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LD A, (0)\n", encoding_LD_A_derefnn, opcode_LD, operand_r, register_A, operand_deref_n, 0},
      {"LD A, (0xFFFF)\n", encoding_LD_A_derefnn, opcode_LD, operand_r, register_A, operand_deref_nn, 0xFFFF},
      {"LD BC, 300\n", encoding_LD_dd_nn, opcode_LD, operand_rr, register_BC, operand_nn, 300},
      {"LD DE, 400\n", encoding_LD_dd_nn, opcode_LD, operand_rr, register_DE, operand_nn, 400},
      {"LD HL, 500\n", encoding_LD_dd_nn, opcode_LD, operand_rr, register_HL, operand_nn, 500},
      {"LD SP, 0xFFFF\n", encoding_LD_dd_nn, opcode_LD, operand_rr, register_SP, operand_nn, 0xFFFF},
      {"LD IX, 0xAABB\n", encoding_LD_IX_nn, opcode_LD, operand_rr, register_IX, operand_nn, 0xAABB},
      {"LD IY, 0xCCDD\n", encoding_LD_IY_nn, opcode_LD, operand_rr, register_IY, operand_nn, 0xCCDD},
      {"LD HL, (0xCCDD)\n", encoding_LD_dd_derefnn, opcode_LD, operand_rr, register_HL, operand_deref_nn, 0xCCDD},
      {"LD BC, (300)\n", encoding_LD_dd_derefnn, opcode_LD, operand_rr, register_BC, operand_deref_nn, 300},
      {"LD DE, (400)\n", encoding_LD_dd_derefnn, opcode_LD, operand_rr, register_DE, operand_deref_nn, 400},
      {"LD HL, (500)\n", encoding_LD_dd_derefnn, opcode_LD, operand_rr, register_HL, operand_deref_nn, 500},
      {"LD SP, (0xFFFF)\n", encoding_LD_dd_derefnn, opcode_LD, operand_rr, register_SP, operand_deref_nn, 0xFFFF},
      {"LD IX, (0xAABB)\n", encoding_LD_IX_derefnn, opcode_LD, operand_rr, register_IX, operand_deref_nn, 0xAABB},
      {"LD IY, (0xCCDD)\n", encoding_LD_IY_derefnn, opcode_LD, operand_rr, register_IY, operand_deref_nn, 0xCCDD},
  };

  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  TEST_ASSERT_TRUE(lexer_tokenize(lexer, ld_test_file));
  TEST_ASSERT_TRUE(parser_do_it(parser, lexer_getTokenList(lexer)));

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
    TEST_ASSERT_EQUAL(driver[i].expectedregister1, statement->instruction.operand1.data.r);
    TEST_ASSERT_EQUAL(driver[i].expextedImmediate, statement->instruction.operand2.data.immediate_nn);

    statementNode = listNode_getNext(statementNode);
    if (statementNode != NULL)
    {
      statement = listNode_getData(statementNode);
    }
  }
}

void test_LD_nn_r(void)
{
  typedef struct
  {
    char *asmLine;
    encoding_t expectedEncoding;
    opcode_t expectedOpCode;
    operand_type_t expectedOperand1;
    operand_type_t expectedOperand2;
    uint16_t expextedImmediate;
    register_type_t expectedregister1;
  } test_driver_t;

  reset_context();

  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LD (0), A\n", encoding_LD_derefnn_A, opcode_LD, operand_deref_n, operand_r, 0, register_A},
      {"LD (0xFFFF), A\n", encoding_LD_derefnn_A, opcode_LD, operand_deref_nn, operand_r, 0xFFFF, register_A}};
  statementCount = (sizeof(driver) / sizeof(test_driver_t));

  // Write instructions into a file
  for (int i = 0; i < statementCount; i++)
  {
    fprintf(ld_test_file, "%s", driver[i].asmLine);
  }

  rewind(ld_test_file);

  // Process test file
  TEST_ASSERT_TRUE(lexer_tokenize(lexer, ld_test_file));
  TEST_ASSERT_TRUE(parser_do_it(parser, lexer_getTokenList(lexer)));

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
    TEST_ASSERT_EQUAL(driver[i].expextedImmediate, statement->instruction.operand1.data.immediate_nn);
    TEST_ASSERT_EQUAL(driver[i].expectedregister1, statement->instruction.operand2.data.r);

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
  RUN_TEST(test_LD_r_n);
  RUN_TEST(test_LD_derefrr);
  RUN_TEST(test_LD_reg_idx);
  RUN_TEST(test_LD_idx_r);
  RUN_TEST(test_LD_r_nn);
  RUN_TEST(test_LD_nn_r);
  return UNITY_END();
}