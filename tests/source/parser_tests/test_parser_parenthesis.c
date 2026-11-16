#include "unity.h"

#include "test_parser.h"

#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "utility/linked_list.h"

#include <stdio.h>

static void reset_context()
{
  rewind(parser_test_file);
  lexer_reset(lexer);
  parser_reset(parser);
}

/**************************************************************************************************/
// Returns the first token node of the next line, i.e. the node following the next EOL token
/**************************************************************************************************/
static ListNode *next_line_start(ListNode *node)
{
  while (node != NULL && ((token_t *)listNode_getData(node))->type != token_eol)
  {
    node = listNode_getNext(node);
  }

  return (node == NULL) ? NULL : listNode_getNext(node);
}

typedef struct
{
  char *operandLine;
  bool expected;
} test_driver_t;

/**************************************************************************************************/
// Writes every line of the driver into the test file and tokenizes it. Then runs the check on each line,
// starting at its first token, and compares it with the expected result
/**************************************************************************************************/
static void run_line_checks(const test_driver_t *driver, int lineCount, bool (*check)(parser_t *))
{
  reset_context();

  for (int i = 0; i < lineCount; i++)
  {
    fprintf(parser_test_file, "%s", driver[i].operandLine);
  }

  rewind(parser_test_file);

  lexer_tokenize(lexer, parser_test_file);
  ListNode *lineStart = linkedList_getFirstNode(lexer_getTokenList(lexer));

  for (int i = 0; i < lineCount; i++)
  {
    TEST_ASSERT_NOT_NULL_MESSAGE(lineStart, driver[i].operandLine);

    parser->currentTokenNode = lineStart;

    TEST_ASSERT_EQUAL_MESSAGE(driver[i].expected, check(parser), driver[i].operandLine);

    // The look-ahead must not move the parser
    TEST_ASSERT_TRUE_MESSAGE(parser->currentTokenNode == lineStart, driver[i].operandLine);

    lineStart = next_line_start(lineStart);
  }
}

void test_parenthesis_deref(void)
{
  test_driver_t driver[] = {
      // Operand is enclosed in a single parenthesis pair
      {"(hl)\n", true},
      {"(ix+1), a\n", true},
      {"((1+2)*3)\n", true},
      {"(1)\n", true},

      // Parenthesis pair does not span the whole operand
      {"(1+2)*(3)\n", false},
      {"(1)+(2)\n", false},
      {"(a)+(b)\n", false},
      {"(1+2)*3\n", false},

      // Unbalanced parenthesises
      {"(a\n", false},
      {"(a))(\n", false},
      {"(a))\n", false},
      {"((a)\n", false},

      // Does not start with an opening parenthesis
      {"hl\n", false},
      {"1+(2)\n", false},

      // Documents current behaviour: an empty pair is accepted here, rejecting it is up to the operand parsing
      {"()\n", true},
  };

  run_line_checks(driver, sizeof(driver) / sizeof(test_driver_t), parser_checkParenthesisDeref);
}

void test_parenthesis_balance(void)
{
  test_driver_t driver[] = {
      // Balanced
      {"hl\n", true}, // no parenthesises at all
      {"(hl)\n", true},
      {"()\n", true},
      {"((1+2)*3)\n", true},
      {"(1)+(2)\n", true},
      {"(ix+1), (hl)\n", true}, // the whole line is checked, not only the first operand

      // Closing parenthesis without a matching opening one
      {")(\n", false}, // equal counts, but wrong order
      {"a)\n", false},
      {"(a))\n", false},
      {"(a))(\n", false},

      // Opening parenthesis is never closed
      {"(a\n", false},
      {"((a)\n", false},
      {"(a), (b\n", false},
  };

  run_line_checks(driver, sizeof(driver) / sizeof(test_driver_t), parser_checkParenthesisBalance);
}

void test_parenthesis_balance_starts_at_current_token(void)
{
  reset_context();

  fprintf(parser_test_file, "(a), b\n");
  rewind(parser_test_file);

  lexer_tokenize(lexer, parser_test_file);
  ListNode *first = linkedList_getFirstNode(lexer_getTokenList(lexer));

  // Starting at '(' the line is balanced
  parser->currentTokenNode = first;
  TEST_ASSERT_TRUE(parser_checkParenthesisBalance(parser));

  // Starting one token later (at 'a') the ')' has no matching '(' in the examined range
  parser->currentTokenNode = listNode_getNext(first);
  TEST_ASSERT_FALSE(parser_checkParenthesisBalance(parser));
}

void test_expression_operand(void)
{
  test_driver_t driver[] = {
      // Single values, registers and symbols are no expressions
      {"42\n", false},
      {"label\n", false},
      {"a\n", false},
      {"(hl)\n", false},

      // Unary + and - at the start of the operand are no expressions
      {"+1\n", false},
      {"-1\n", false},
      {"-5\n", false},
      {"(-1)\n", false},
      {"-(1)\n", false},

      // Index register with offset is no expression
      {"(ix+1)\n", false},
      {"(iy-5)\n", false},

      // Binary operators
      {"1+2\n", true},
      {"label+1\n", true},
      {"(label+1)\n", true},
      {"2*3\n", true},
      {"label*2\n", true},
      {"(3)*5\n", true},
      {"5*(3+2)\n", true},
      {"4/2\n", true},

      // A leading unary operator does not hide a following binary operator
      {"+1-2\n", true},
      {"-1-2\n", true},
      {"+1+2\n", true},
      {"-(1)+2\n", true},

      // Index register offset followed by further operations
      {"(ix+1)+2\n", true},
      {"(ix+1+2)\n", true},

      // Unary operators after a binary operator are not supported, the second sign is a binary operator
      {"1 - -2\n", true},
      {"3*(-2)\n", true},

      // Only the operand up to the next comma is examined
      {"1+2, b\n", true},
      {"a, 1+2\n", false},
      {"(ix+1), 1+2\n", false},

      // Documents current behaviour: any register exempts the following operator, other registers than IX/IY
      // are rejected by the operand parsing
      {"b+1\n", false},
      {"(ix*2)\n", false},

      // A trailing operator without right operand is still reported as expression, the operand parsing fails later
      {"2*\n", true},
  };

  run_line_checks(driver, sizeof(driver) / sizeof(test_driver_t), parser_checkExpressionOperand);
}

void test_parenthesis()
{
  RUN_TEST(test_parenthesis_deref);
  RUN_TEST(test_parenthesis_balance);
  RUN_TEST(test_parenthesis_balance_starts_at_current_token);
  RUN_TEST(test_expression_operand);
}
