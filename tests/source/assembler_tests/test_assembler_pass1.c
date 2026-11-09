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

// Test debug function - iterator to print all
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

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directives(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "NEWORG EQU 0xAABB \n",                   // PC = 0
      "ORGLABEL: ORG NEWORG \n",                // PC = 0 - Set PC to NEWORG
      "START: DB 0xAA, 0xBB, 0xCC, 0xDD\n",     // PC = 0xAABB - Define 4 bytes
      "START2: DW 0x1122, 0x3344, 0x5566\n",    // PC = 0xAABF - define 3 words
      "START3: DW 0x1122, 0x3344, 0x5566\n",    // PC = 0xAAC5 - define 3 words
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

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Get symbol lists and check symbol count in each list
  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(10, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(2, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(2, linkedList_count(exportedSymbols));

  // Check existence and assigned address of expected symbols
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

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directives_org(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "LINE1: ORG 0xAA \n",   // PC = 0
      "LINE2: ORG 0xBBCC \n", // PC = 0xAA
      "LINE3: DB 10 \n"       // PC = 0xBBCC
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Get symbol lists and check symbol count in each list
  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(3, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(exportedSymbols));

  // Check the PC value for each line
  // Line 1
  ListNode *currentNode = linkedList_getFirstNode(internalSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("LINE1", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0x0, currentSymbol->value);

  // Line 2
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("LINE2", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xAA, currentSymbol->value);

  // Line 3
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL_STRING("LINE3", currentSymbol->symbol);
  TEST_ASSERT_EQUAL(0xBBCC, currentSymbol->value);
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directives_org_invalid(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "ORG LINE3 \n", // Invalid forwad reference
      "LINE2: ORG 0xBBCC \n",
      "LINE3: DB 10 \n",
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));

  // Pass 1 should fail due to forward referencing a symbol in an ORG directive
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_double_symbol(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "LINE1: ORG 0xAA \n",
      "LINE1: ORG 0xBBCC \n",
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));

  // Pass 1 fails due to multiple definitions of same symbol
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_double_import(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "IMPORT SYMBOL1 \n",
      "IMPORT SYMBOL1 \n",
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));

  // Pass 1 fails due to multiple imports of same symbol
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_double_export(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "EXPORT SYMBOL1 \n",
      "EXPORT SYMBOL1 \n",
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));

  // Pass 1 fails due to multiple imports of same symbol
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directive_section(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "SECTION TEST1 \n",
  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));

  // Pass 1 fails due to unsupported directive
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_loadgroup8(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "LD A, B \n",                // PC = 0, size = 1
      "LINE1: LD A, 5 \n",         // PC = 1, size = 2
      "LINE2: LD A, (HL) \n",      // PC = 3, size = 1
      "LINE3: LD A, (IX+1) \n",    // PC = 4, size = 3
      "LINE4: LD A, (IY+1) \n",    // PC = 7, size = 3
      "LINE5: LD (HL), A \n",      // PC = 10, size = 1
      "LINE6: LD (IX+1), A \n",    // PC = 11, size = 3
      "LINE7: LD (IY+1), A \n",    // PC = 14, size = 3
      "LINE8: LD (HL), 5 \n",      // PC = 17, size = 2
      "LINE9: LD (IX+1), 5 \n",    // PC = 19, size = 4
      "LINE10: LD (IY+1), 5 \n",   // PC = 23, size = 4
      "LINE11: LD A, (BC) \n",     // PC = 27, size = 1
      "LINE12: LD A, (DE) \n",     // PC = 28, size = 1
      "LINE13: LD A, (0xAABB) \n", // PC = 29, size = 3
      "LINE14: LD (BC), A \n",     // PC = 32, size = 1
      "LINE15: LD (DE), A \n",     // PC = 33, size = 1
      "LINE16: LD (0xAABB), A \n", // PC = 34, size = 3
      "LINE17: LD A, I \n",        // PC = 37, size = 2
      "LINE18: LD A, R \n",        // PC = 39, size = 2
      "LINE19: LD I, A \n",        // PC = 41, size = 2
      "LINE20: LD R, A \n",        // PC = 43, size = 2
      "LINE21: DB 1 \n",           // PC 45

  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Get symbol lists and check symbol count in each list
  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(21, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(exportedSymbols));

  // Check the PC value for each line
  // Line 1
  ListNode *currentNode = linkedList_getFirstNode(internalSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(1, currentSymbol->value);

  // Line 2
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(3, currentSymbol->value);

  // Line 3
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(4, currentSymbol->value);

  // Line 4
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(7, currentSymbol->value);

  // Line 5
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(10, currentSymbol->value);

  // Line 6
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(11, currentSymbol->value);

  // Line 7
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(14, currentSymbol->value);

  // Line 8
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(17, currentSymbol->value);

  // Line 9
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(19, currentSymbol->value);

  // Line 10
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(23, currentSymbol->value);

  // Line 11
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(27, currentSymbol->value);

  // Line 12
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(28, currentSymbol->value);

  // Line 13
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(29, currentSymbol->value);

  // Line 14
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(32, currentSymbol->value);

  // Line 15
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(33, currentSymbol->value);

  // Line 16
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(34, currentSymbol->value);

  // Line 17
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(37, currentSymbol->value);

  // Line 18
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(39, currentSymbol->value);

  // Line 19
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(41, currentSymbol->value);

  // Line 20
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(43, currentSymbol->value);

  // Line 21
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(45, currentSymbol->value);
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_loadgroup16(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "LD BC, 0xAABB \n",            // PC = 0, size = 3
      "LINE1: LD IX, 0xAABB \n",     // PC = 3, size = 4
      "LINE2: LD IY, 0xAABB \n",     // PC = 7, size = 4
      "LINE3: LD HL, (0xAABB) \n",   // PC = 11, size = 3
      "LINE4: LD BC, (0xAABB) \n",   // PC = 14, size = 4
      "LINE5: LD IX, (0xAABB) \n",   // PC = 18, size = 4
      "LINE6: LD IY, (0xAABB) \n",   // PC = 22, size = 4
      "LINE7: LD (0xAABB), HL \n",   // PC = 26, size = 3
      "LINE8: LD (0xAABB), BC \n",   // PC = 29, size = 4
      "LINE9: LD (0xAABB), IX \n",   // PC = 33, size = 4
      "LINE10: LD  (0xAABB), IY \n", // PC = 37, size = 4
      "LINE11: LD  SP, HL \n",       // PC = 41, size = 1
      "LINE12: LD  SP, IX \n",       // PC = 42, size = 2
      "LINE13: LD  SP, IY \n",       // PC = 44, size = 2
      "LINE14: PUSH  BC \n",         // PC = 46, size = 1
      "LINE15: PUSH  IX \n",         // PC = 47, size = 2
      "LINE16: PUSH  IY \n",         // PC = 49, size = 2
      "LINE17: POP  BC \n",          // PC = 51, size = 1
      "LINE18: POP  IX \n",          // PC = 52, size = 2
      "LINE19: POP  IY \n",          // PC = 54, size = 2
      "LINE20: DB 1 \n",             // PC = 56

  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Get symbol lists and check symbol count in each list
  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(20, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(exportedSymbols));

  // Check the PC value for each line
  // Line 1
  ListNode *currentNode = linkedList_getFirstNode(internalSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(3, currentSymbol->value);

  // Line 2
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(7, currentSymbol->value);

  // Line 3
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(11, currentSymbol->value);

  // Line 4
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(14, currentSymbol->value);

  // Line 5
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(18, currentSymbol->value);

  // Line 6
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(22, currentSymbol->value);

  // Line 7
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(26, currentSymbol->value);

  // Line 8
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(29, currentSymbol->value);

  // Line 9
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(33, currentSymbol->value);

  // Line 10
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(37, currentSymbol->value);

  // Line 11
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(41, currentSymbol->value);

  // Line 12
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(42, currentSymbol->value);

  // Line 13
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(44, currentSymbol->value);

  // Line 14
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(46, currentSymbol->value);

  // Line 15
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(47, currentSymbol->value);

  // Line 16
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(49, currentSymbol->value);

  // Line 17
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(51, currentSymbol->value);

  // Line 18
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(52, currentSymbol->value);

  // Line 19
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(54, currentSymbol->value);

  // Line 20
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(56, currentSymbol->value);
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_exchangegroup(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "EX DE, HL \n",          // PC = 0, size = 1
      "LINE1: EX AF, AF \n",   // PC = 1, size = 1
      "LINE2: EXX \n",         // PC = 2, size = 1
      "LINE3: EX (SP), HL \n", // PC = 3, size = 1
      "LINE4: EX (SP), IX \n", // PC = 4, size = 2
      "LINE5: EX (SP), IY \n", // PC = 6, size = 2
      "LINE6: LDI \n",         // PC = 8, size = 2
      "LINE7: LDIR \n",        // PC = 10, size = 2
      "LINE8: LDD \n",         // PC = 12, size = 2
      "LINE9: LDDR \n",        // PC = 14, size = 2
      "LINE10: CPI \n",        // PC = 16, size = 2
      "LINE11: CPIR \n",       // PC = 18, size = 2
      "LINE12: CPD \n",        // PC = 20, size = 2
      "LINE13: CPDR \n",       // PC = 22, size = 2
      "LINE14: DB 1 \n",       // PC = 24

  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Get symbol lists and check symbol count in each list
  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(14, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(exportedSymbols));

  // Check the PC value for each line
  // Line 1
  ListNode *currentNode = linkedList_getFirstNode(internalSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(1, currentSymbol->value);

  // Line 2
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(2, currentSymbol->value);

  // Line 3
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(3, currentSymbol->value);

  // Line 4
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(4, currentSymbol->value);

  // Line 5
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(6, currentSymbol->value);

  // Line 6
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(8, currentSymbol->value);

  // Line 7
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(10, currentSymbol->value);

  // Line 8
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(12, currentSymbol->value);

  // Line 9
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(14, currentSymbol->value);

  // Line 10
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(16, currentSymbol->value);

  // Line 11
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(18, currentSymbol->value);

  // Line 12
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(20, currentSymbol->value);

  // Line 13
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(22, currentSymbol->value);

  // Line 14
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(24, currentSymbol->value);
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_arithmeticgroup8(void)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Define the test instructions
  const char *driver[] = {
      "ADD A, B \n",              // PC = 0, size = 1
      "LINE1: ADD A, 1 \n",       // PC = 1, size = 2
      "LINE2: ADD A, (HL) \n",    // PC = 3, size = 1
      "LINE3: ADD A, (IX+1) \n",  // PC = 4, size = 3
      "LINE4: ADD A, (IY+1) \n",  // PC = 7, size = 3
      "LINE5: ADC A, A \n",       // PC = 10, size = 1
      "LINE6: ADC A, 1 \n",       // PC = 11, size = 2
      "LINE7: ADC A, (HL) \n",    // PC = 13, size = 1
      "LINE8: ADC A, (IX+1) \n",  // PC = 14, size = 3
      "LINE9: ADC A, (IY+1) \n",  // PC = 17, size = 3
      "LINE10: SUB A \n",         // PC = 20, size = 1
      "LINE11: SUB 1 \n",         // PC = 21, size = 2
      "LINE12: SUB (HL) \n",      // PC = 23, size = 1
      "LINE13: SUB (IX+1) \n",    // PC = 24, size = 3
      "LINE14: SUB (IY+1) \n",    // PC = 27, size = 3
      "LINE15: SBC A, A \n",      // PC = 30, size = 1
      "LINE16: SBC A, 1 \n",      // PC = 31, size = 2
      "LINE17: SBC A, (HL) \n",   // PC = 33, size = 1
      "LINE18: SBC A, (IX+1) \n", // PC = 34, size = 3
      "LINE19: SBC A, (IY+1) \n", // PC = 37, size = 3
      "LINE20: AND A \n",         // PC = 40, size = 1
      "LINE21: AND 1 \n",         // PC = 41, size = 2
      "LINE22: AND (HL) \n",      // PC = 43, size = 1
      "LINE23: AND (IX+1) \n",    // PC = 44, size = 3
      "LINE24: AND (IY+1) \n",    // PC = 47, size = 3
      "LINE25: OR A \n",          // PC = 50, size = 1
      "LINE26: OR 1 \n",          // PC = 51, size = 2
      "LINE27: OR (HL) \n",       // PC = 53, size = 1
      "LINE28: OR (IX+1) \n",     // PC = 54, size = 3
      "LINE29: OR (IY+1) \n",     // PC = 57, size = 3
      "LINE30: XOR A \n",         // PC = 60, size = 1
      "LINE31: XOR 1 \n",         // PC = 61, size = 2
      "LINE32: XOR (HL) \n",      // PC = 63, size = 1
      "LINE33: XOR (IX+1) \n",    // PC = 64, size = 3
      "LINE34: XOR (IY+1) \n",    // PC = 67, size = 3
      "LINE35: CP A \n",          // PC = 70, size = 1
      "LINE36: CP 1 \n",          // PC = 71, size = 2
      "LINE37: CP (HL) \n",       // PC = 73, size = 1
      "LINE38: CP (IX+1) \n",     // PC = 74, size = 3
      "LINE39: CP (IY+1) \n",     // PC = 77, size = 3
      "LINE40: INC A \n",         // PC = 80, size = 1
      "LINE41: INC (HL) \n",      // PC = 81, size = 1
      "LINE42: INC (IX+1) \n",    // PC = 82, size = 3
      "LINE43: INC (IY+1) \n",    // PC = 85, size = 3
      "LINE44: DEC A \n",         // PC = 88, size = 1
      "LINE45: DEC (HL) \n",      // PC = 89, size = 1
      "LINE46: DEC (IX+1) \n",    // PC = 90, size = 3
      "LINE47: DEC (IY+1) \n",    // PC = 93, size = 3
      "LINE48: DB 1 \n",          // PC = 96

  };
  size_t lineCount = sizeof(driver) / sizeof(char *);

  // Write instructions into a file
  for (int i = 0; i < lineCount; i++)
  {
    fprintf(testFile, "%s", driver[i]);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Get symbol lists and check symbol count in each list
  SymbolList *internalSymbols = assembler->symbolList;
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(48, linkedList_count(internalSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(exportedSymbols));

  // Check the PC value for each line
  // Line 1
  ListNode *currentNode = linkedList_getFirstNode(internalSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(1, currentSymbol->value);

  // Line 2
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(3, currentSymbol->value);

  // Line 3
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(4, currentSymbol->value);

  // Line 4
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(7, currentSymbol->value);

  // Line 5
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(10, currentSymbol->value);

  // Line 6
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(11, currentSymbol->value);

  // Line 7
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(13, currentSymbol->value);

  // Line 8
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(14, currentSymbol->value);

  // Line 9
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(17, currentSymbol->value);

  // Line 10
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(20, currentSymbol->value);

  // Line 11
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(21, currentSymbol->value);

  // Line 12
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(23, currentSymbol->value);

  // Line 13
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(24, currentSymbol->value);

  // Line 14
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(27, currentSymbol->value);

  // Line 15
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(30, currentSymbol->value);

  // Line 16
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(31, currentSymbol->value);

  // Line 17
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(33, currentSymbol->value);

  // Line 18
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(34, currentSymbol->value);

  // Line 19
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(37, currentSymbol->value);

  // Line 20
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(40, currentSymbol->value);

  // Line 21
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(41, currentSymbol->value);

  // Line 22
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(43, currentSymbol->value);

  // Line 23
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(44, currentSymbol->value);

  // Line 24
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(47, currentSymbol->value);

  // Line 25
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(50, currentSymbol->value);

  // Line 26
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(51, currentSymbol->value);

  // Line 27
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(53, currentSymbol->value);

  // Line 28
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(54, currentSymbol->value);

  // Line 29
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(57, currentSymbol->value);

  // Line 30
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(60, currentSymbol->value);

  // Line 31
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(61, currentSymbol->value);

  // Line 32
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(63, currentSymbol->value);

  // Line 33
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(64, currentSymbol->value);

  // Line 34
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(67, currentSymbol->value);

  // Line 35
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(70, currentSymbol->value);

  // Line 36
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(71, currentSymbol->value);

  // Line 37
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(73, currentSymbol->value);

  // Line 38
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(74, currentSymbol->value);

  // Line 39
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(77, currentSymbol->value);

  // Line 40
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(80, currentSymbol->value);

  // Line 41
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(81, currentSymbol->value);

  // Line 42
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(82, currentSymbol->value);

  // Line 43
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(85, currentSymbol->value);

  // Line 44
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(88, currentSymbol->value);

  // Line 45
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(89, currentSymbol->value);

  // Line 46
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(90, currentSymbol->value);

  // Line 47
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(93, currentSymbol->value);

  // Line 48
  currentNode = listNode_getNext(currentNode);
  currentSymbol = listNode_getData(currentNode);
  TEST_ASSERT_EQUAL(96, currentSymbol->value);
}

/**************************************************************************************************/
/**************************************************************************************************/
void test_assembler_pass1()
{
  RUN_TEST(test_directives);
  RUN_TEST(test_directives_org);
  RUN_TEST(test_directives_org_invalid);
  RUN_TEST(test_double_symbol);
  RUN_TEST(test_double_import);
  RUN_TEST(test_double_export);
  RUN_TEST(test_directive_section);
  RUN_TEST(test_instruction_size_loadgroup8);
  RUN_TEST(test_instruction_size_loadgroup16);
  RUN_TEST(test_instruction_size_exchangegroup);
  RUN_TEST(test_instruction_size_arithmeticgroup8);
}