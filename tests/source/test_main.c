#include "unity.h"

#include "test_parser.h"

static char *testFileName = "test-file.asm";
static int statementCount;

parser_t *parser;
lexer_state_t *lexer;
FILE *parser_test_file;

void setUp(void)
{
  parser_test_file = fopen(testFileName, "w+");
  TEST_ASSERT_NOT_NULL(parser_test_file);

  lexer = lexer_initialize();
  TEST_ASSERT_NOT_NULL(lexer);

  parser = parser_initialize();
  TEST_ASSERT_NOT_NULL(parser);
}

void tearDown(void)
{
  fclose(parser_test_file);
  lexer_destroy(lexer);
  parser_destroy(parser);
}

static void test_lexer_parser()
{
  test_arithmetic();
  test_BIT();
  test_call_return();
  test_EX();
  test_gparith_cpucontrol();
  test_input_output();
  test_jump();
  test_LD();
  test_push_pop();
  test_rotate_shift();
}

int main(void)
{
  UNITY_BEGIN();

  test_lexer_parser();

  return UNITY_END();
}