#include "lexer/lexer.h"
#include "parser/parser.h"
#include "unity.h"

#include "test_assembler.h"

#include "assembler/assembler.h"
#include "lexer/token.h"
#include "logging/logging.h"
#include "types.h"
#include "utility/linked_list.h"

#include <stdio.h>

static void token_iterateCb(void *token, size_t iteration)
{
  char *tokenstring = NULL;
  token_toString(((token_t *)token)->type, &tokenstring);

  LOG_INFO("%d: %s", ++iteration, tokenstring);
  free(tokenstring);
}

static void symbol_iterateCb(void *thisSymbol, size_t iteration)
{
  symbol_t *symbol = (symbol_t *)thisSymbol;
  LOG_INFO("%d: %s", ++iteration, symbol->symbol);
}

static void test_directives(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions together with the expected result
  const char *driver[] = {
      "NEWORG EQU 0xAABB \n",                   // PC = 0
      "ORGLABEL: ORG NEWORG \n",                // PC = 0
      "START: DB 0xAA, 0xBB, 0xCC, 0xDD\n",     // PC = 0xAABB
      "START2: DW 0x1122, 0x3344, 0x5566\n",    // PC = 0xAABF
      "START3: DW 0x1122, 0x3344, 0x5566\n",    // PC = 0xAAC5
      "START4: DS \"This is a test string\"\n", // PC = 0xAACB
      "START5: EXPORT START\n",                 // PC = 0xAAE1
      "START6: EXPORT START2\n",                // PC = 0xAAE1
      "START7: IMPORT IMPORTED1\n",             // PC = 0xAAE1
      "START8: IMPORT IMPORTED2\n",             // PC = 0xAAE1
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(10, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(2, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(2, linkedList_count(exportedSymbols));

  // Line 1
  ListNode *currentNode = linkedList_getFirstNode(internalSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("NEWORG", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAABB, currentSymbol->value);

  // Line 2
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("ORGLABEL", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0, currentSymbol->value);

  // Line 3
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAABB, currentSymbol->value);

  // Line 4
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START2", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAABF, currentSymbol->value);

  // Line 5
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START3", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAAC5, currentSymbol->value);

  // Line 6
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START4", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAACB, currentSymbol->value);

  // Line 7
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START5", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAAE1, currentSymbol->value);

  // Line 8
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START6", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAAE1, currentSymbol->value);

  // Line 9
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START7", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAAE1, currentSymbol->value);

  // Line 10
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START8", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAAE1, currentSymbol->value);

  // Exported Symbols
  currentNode = linkedList_getFirstNode(exportedSymbols);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0, currentSymbol->value); // Will be resolved in pass 2

  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("START2", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0, currentSymbol->value); // Will be resolved in pass 2

  // Imported Symbols
  currentNode = linkedList_getFirstNode(importedSymbols);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("IMPORTED1", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0, currentSymbol->value);

  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("IMPORTED2", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0, currentSymbol->value);
}

void test_assembler_pass1() { RUN_TEST(test_directives); }