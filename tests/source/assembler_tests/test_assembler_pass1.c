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

// A single line of assembly together with the expected result of pass 1.
// If the line defines a label, expectedSymbol is the label name and expectedValue the address it must resolve to.
// If the line defines no label, expectedSymbol is NULL and expectedValue is ignored.
typedef struct
{
  const char *asmLine;
  const char *expectedSymbol;
  uint16_t expectedValue;
} test_driver_t;

#define DRIVER_COUNT(driver) (sizeof(driver) / sizeof(test_driver_t))

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

// Write the driver's assembly lines to a file, then lex and parse it. The returned assembler is ready for pass 1.
static assembler_t *prepare_assembler(const test_driver_t *driver, size_t driverCount)
{
  FILE *testFile = fopen("directive_test.asm", "w+");
  TEST_ASSERT_NOT_NULL(testFile);

  // Write instructions into a file
  for (size_t i = 0; i < driverCount; i++)
  {
    fprintf(testFile, "%s", driver[i].asmLine);
  }

  rewind(testFile);
  assembler_t *assembler = assembler_initialize();

  // Process test file up to assembler pass 1
  TEST_ASSERT_TRUE(lexer_tokenize(assembler->lexer, testFile));
  token_list_t *tokenList = lexer_getTokenList(assembler->lexer);
  TEST_ASSERT_TRUE(parser_do_it(assembler->parser, tokenList));

  return assembler;
}

// Check the internal symbol list against the driver entries (in order of definition)
static void check_internal_symbols(assembler_t *assembler, const test_driver_t *driver, size_t driverCount)
{
  size_t expectedSymbolCount = 0;
  for (size_t i = 0; i < driverCount; i++)
  {
    if (driver[i].expectedSymbol != NULL)
    {
      expectedSymbolCount++;
    }
  }

  SymbolList *internalSymbols = assembler->symbolList;
  TEST_ASSERT_EQUAL(expectedSymbolCount, linkedList_count(internalSymbols));

  ListNode *symbolNode = linkedList_getFirstNode(internalSymbols);
  for (size_t i = 0; i < driverCount; i++)
  {
    if (driver[i].expectedSymbol == NULL)
    {
      continue;
    }

    TEST_ASSERT_NOT_NULL(symbolNode);
    symbol_t *symbol = listNode_getData(symbolNode);
    TEST_ASSERT_EQUAL_STRING(driver[i].expectedSymbol, symbol->symbol);
    TEST_ASSERT_EQUAL(driver[i].expectedValue, symbol->value);

    symbolNode = listNode_getNext(symbolNode);
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directives(void)
{
  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"NEWORG EQU 0xAABB \n", "NEWORG", 0xAABB},                   // PC = 0
      {"ORGLABEL: ORG NEWORG \n", "ORGLABEL", 0},                   // PC = 0 - Set PC to NEWORG
      {"START: DB 0xAA, 0xBB, 0xCC, 0xDD\n", "START", 0xAABB},      // PC = 0xAABB - Define 4 bytes
      {"START2: DW 0x1122, 0x3344, 0x5566\n", "START2", 0xAABF},    // PC = 0xAABF - define 3 words
      {"START3: DW 0x1122, 0x3344, 0x5566\n", "START3", 0xAAC5},    // PC = 0xAAC5 - define 3 words
      {"START4: DS \"This is a test string\"\n", "START4", 0xAACB}, // PC = 0xAACB
      {"START5: EXPORT START\n", "START5", 0xAAE1},                 // PC = 0xAAE1
      {"START6: EXPORT START2\n", "START6", 0xAAE1},                // PC = 0xAAE1
      {"START7: IMPORT IMPORTED1\n", "START7", 0xAAE1},             // PC = 0xAAE1
      {"START8: IMPORT IMPORTED2\n", "START8", 0xAAE1},             // PC = 0xAAE1
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  // Check internal symbols and their assigned addresses
  check_internal_symbols(assembler, driver, driverCount);

  // Get symbol lists and check symbol count in each list
  SymbolList *importedSymbols = assembler->importedSymbols;
  SymbolList *exportedSymbols = assembler->exportedSymbols;

  TEST_ASSERT_EQUAL(2, linkedList_count(importedSymbols));
  TEST_ASSERT_EQUAL(2, linkedList_count(exportedSymbols));

  // Exported Symbols
  ListNode *currentNode = linkedList_getFirstNode(exportedSymbols);
  symbol_t *currentSymbol = listNode_getData(currentNode);
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
  // Define the test instructions together with the expected result
  test_driver_t driver[] = {
      {"LINE1: ORG 0xAA \n", "LINE1", 0x0},    // PC = 0
      {"LINE2: ORG 0xBBCC \n", "LINE2", 0xAA}, // PC = 0xAA
      {"LINE3: DB 10 \n", "LINE3", 0xBBCC},    // PC = 0xBBCC
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directives_org_invalid(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"ORG LINE3 \n", NULL, 0}, // Invalid forward reference
      {"LINE2: ORG 0xBBCC \n", NULL, 0},
      {"LINE3: DB 10 \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 should fail due to forward referencing a symbol in an ORG directive
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_double_symbol(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"LINE1: ORG 0xAA \n", NULL, 0},
      {"LINE1: ORG 0xBBCC \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails due to multiple definitions of same symbol
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_double_import(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"IMPORT SYMBOL1 \n", NULL, 0},
      {"IMPORT SYMBOL1 \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails due to multiple imports of same symbol
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_double_export(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"EXPORT SYMBOL1 \n", NULL, 0},
      {"EXPORT SYMBOL1 \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails due to multiple exports of same symbol
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directive_section(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"SECTION TEST1 \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails due to unsupported directive
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_label_only(void)
{
  // Lines consisting of only a label define a symbol at the current PC but do not advance it
  test_driver_t driver[] = {
      {"LABEL1: \n", "LABEL1", 0},          // PC = 0, size = 0
      {"LD A, B \n", NULL, 0},              // PC = 0, size = 1
      {"LABEL2: \n", "LABEL2", 1},          // PC = 1, size = 0
      {"LABEL3: LD A, 5 \n", "LABEL3", 1},  // PC = 1, size = 2
      {"LABEL4: \n", "LABEL4", 3},          // PC = 3, size = 0
      {"LABEL5: DB 1 \n", "LABEL5", 3},     // PC = 3
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directive_equ(void)
{
  // EQU assigns the given constant to the symbol and does not advance the PC
  test_driver_t driver[] = {
      {"BYTECONST EQU 0x12 \n", "BYTECONST", 0x12},     // PC = 0, byte literal
      {"WORDCONST EQU 0x1234 \n", "WORDCONST", 0x1234}, // PC = 0, word literal
      {"LINE1: DB 1 \n", "LINE1", 0},                   // PC = 0
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directive_org_symbol_backwards(void)
{
  // ORG accepts a previously defined symbol and may move the PC backwards
  test_driver_t driver[] = {
      {"ORGADDR EQU 0x20 \n", "ORGADDR", 0x20}, // PC = 0
      {"ORG ORGADDR \n", NULL, 0},              // PC = 0x20 (from symbol)
      {"L1: DB 1 \n", "L1", 0x20},              // PC = 0x20
      {"L2: ORG 0x10 \n", "L2", 0x21},          // PC = 0x21 - move PC backwards
      {"L3: DB 1 \n", "L3", 0x10},              // PC = 0x10
      {"L4: ORG 0x05 \n", "L4", 0x11},          // PC = 0x11 - move PC backwards again
      {"L5: DB 1 \n", "L5", 0x05},              // PC = 0x05
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_symbols_case_sensitive(void)
{
  // Symbols that only differ in case are different symbols
  test_driver_t driver[] = {
      {"lbl: DB 1 \n", "lbl", 0}, // PC = 0
      {"LBL: DB 1 \n", "LBL", 1}, // PC = 1
      {"Lbl: DB 1 \n", "Lbl", 2}, // PC = 2
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_directive_lists(void)
{
  // DB and DW take lists of values, DS reserves the string including terminator. Each DW value is one word, also if written as byte literal
  test_driver_t driver[] = {
      {"L1: DB 0x01, 0x02, 0x03 \n", "L1", 0},  // PC = 0, size = 3
      {"L2: DW 0x1122, 0x3344 \n", "L2", 3},    // PC = 3, size = 4
      {"L3: DW 1, 2 \n", "L3", 7},              // PC = 7, size = 4 - byte literals are widened
      {"L4: DW 0xFFFF, L1 \n", "L4", 11},       // PC = 11, size = 4 - word literal and symbol
      {"L5: DS \"abc\" \n", "L5", 15},          // PC = 15, size = 4 - 3 characters plus terminator
      {"L6: DB 1 \n", "L6", 19},                // PC = 19
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_export_forward_declared(void)
{
  // A symbol may be exported before it is defined
  test_driver_t driver[] = {
      {"EXPORT LATER \n", NULL, 0},   // PC = 0
      {"LATER: DB 1 \n", "LATER", 0}, // PC = 0
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(1, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_export_undefined(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"LINE1: DB 1 \n", NULL, 0},
      {"EXPORT NOTDEFINED \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails because an exported symbol is never defined
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_pc_end_of_memory(void)
{
  // Statements ending exactly at the end of the 64K address space are valid
  test_driver_t driver[] = {
      {"L1: ORG 0xFFFE \n", "L1", 0},   // PC = 0
      {"L2: LD A, 5 \n", "L2", 0xFFFE}, // PC = 0xFFFE, size = 2 - last byte is 0xFFFF
      {"CONST EQU 5 \n", "CONST", 5},   // EQU does not define an address
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_pc_overflow_directive(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"L1: ORG 0xFFFF \n", NULL, 0},
      {"L2: DW 0x1122 \n", NULL, 0}, // PC = 0xFFFF, size = 2
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails because the word does not fit into the address space anymore
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_pc_overflow_instruction(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"L1: ORG 0xFFFF \n", NULL, 0},
      {"L2: LD A, 5 \n", NULL, 0}, // PC = 0xFFFF, size = 2
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails because the instruction does not fit into the address space anymore
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_pc_overflow_label(void)
{
  // Define the test instructions. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"L1: ORG 0xFFFF \n", NULL, 0},
      {"L2: DB 1 \n", NULL, 0}, // PC = 0xFFFF, size = 1
      {"L3: DB 1 \n", NULL, 0}, // PC = 0x10000
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails because L3 would be defined behind the end of the address space
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_empty_source(void)
{
  // A source file without any content. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails because there is nothing to assemble
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_comment_only_source(void)
{
  // A source file with only comments. Pass 1 is expected to fail, so no symbols are checked
  test_driver_t driver[] = {
      {"; This file contains only a comment \n", NULL, 0},
  };

  assembler_t *assembler = prepare_assembler(driver, DRIVER_COUNT(driver));

  // Pass 1 fails because there is nothing to assemble
  TEST_ASSERT_FALSE(assembler_pass_one(assembler));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_loadgroup8(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"LD A, B \n", NULL, 0},                     // PC = 0, size = 1
      {"LINE1: LD A, 5 \n", "LINE1", 1},           // PC = 1, size = 2
      {"LINE2: LD A, (HL) \n", "LINE2", 3},        // PC = 3, size = 1
      {"LINE3: LD A, (IX+1) \n", "LINE3", 4},      // PC = 4, size = 3
      {"LINE4: LD A, (IY+1) \n", "LINE4", 7},      // PC = 7, size = 3
      {"LINE5: LD (HL), A \n", "LINE5", 10},       // PC = 10, size = 1
      {"LINE6: LD (IX+1), A \n", "LINE6", 11},     // PC = 11, size = 3
      {"LINE7: LD (IY+1), A \n", "LINE7", 14},     // PC = 14, size = 3
      {"LINE8: LD (HL), 5 \n", "LINE8", 17},       // PC = 17, size = 2
      {"LINE9: LD (IX+1), 5 \n", "LINE9", 19},     // PC = 19, size = 4
      {"LINE10: LD (IY+1), 5 \n", "LINE10", 23},   // PC = 23, size = 4
      {"LINE11: LD A, (BC) \n", "LINE11", 27},     // PC = 27, size = 1
      {"LINE12: LD A, (DE) \n", "LINE12", 28},     // PC = 28, size = 1
      {"LINE13: LD A, (0xAABB) \n", "LINE13", 29}, // PC = 29, size = 3
      {"LINE14: LD (BC), A \n", "LINE14", 32},     // PC = 32, size = 1
      {"LINE15: LD (DE), A \n", "LINE15", 33},     // PC = 33, size = 1
      {"LINE16: LD (0xAABB), A \n", "LINE16", 34}, // PC = 34, size = 3
      {"LINE17: LD A, I \n", "LINE17", 37},        // PC = 37, size = 2
      {"LINE18: LD A, R \n", "LINE18", 39},        // PC = 39, size = 2
      {"LINE19: LD I, A \n", "LINE19", 41},        // PC = 41, size = 2
      {"LINE20: LD R, A \n", "LINE20", 43},        // PC = 43, size = 2
      {"LINE21: DB 1 \n", "LINE21", 45},           // PC = 45
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_loadgroup16(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"LD BC, 0xAABB \n", NULL, 0},                 // PC = 0, size = 3
      {"LINE1: LD IX, 0xAABB \n", "LINE1", 3},       // PC = 3, size = 4
      {"LINE2: LD IY, 0xAABB \n", "LINE2", 7},       // PC = 7, size = 4
      {"LINE3: LD HL, (0xAABB) \n", "LINE3", 11},    // PC = 11, size = 3
      {"LINE4: LD BC, (0xAABB) \n", "LINE4", 14},    // PC = 14, size = 4
      {"LINE5: LD IX, (0xAABB) \n", "LINE5", 18},    // PC = 18, size = 4
      {"LINE6: LD IY, (0xAABB) \n", "LINE6", 22},    // PC = 22, size = 4
      {"LINE7: LD (0xAABB), HL \n", "LINE7", 26},    // PC = 26, size = 3
      {"LINE8: LD (0xAABB), BC \n", "LINE8", 29},    // PC = 29, size = 4
      {"LINE9: LD (0xAABB), IX \n", "LINE9", 33},    // PC = 33, size = 4
      {"LINE10: LD  (0xAABB), IY \n", "LINE10", 37}, // PC = 37, size = 4
      {"LINE11: LD  SP, HL \n", "LINE11", 41},       // PC = 41, size = 1
      {"LINE12: LD  SP, IX \n", "LINE12", 42},       // PC = 42, size = 2
      {"LINE13: LD  SP, IY \n", "LINE13", 44},       // PC = 44, size = 2
      {"LINE14: PUSH  BC \n", "LINE14", 46},         // PC = 46, size = 1
      {"LINE15: PUSH  IX \n", "LINE15", 47},         // PC = 47, size = 2
      {"LINE16: PUSH  IY \n", "LINE16", 49},         // PC = 49, size = 2
      {"LINE17: POP  BC \n", "LINE17", 51},          // PC = 51, size = 1
      {"LINE18: POP  IX \n", "LINE18", 52},          // PC = 52, size = 2
      {"LINE19: POP  IY \n", "LINE19", 54},          // PC = 54, size = 2
      {"LINE20: DB 1 \n", "LINE20", 56},             // PC = 56
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_exchangegroup(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"EX DE, HL \n", NULL, 0},             // PC = 0, size = 1
      {"LINE1: EX AF, AF \n", "LINE1", 1},   // PC = 1, size = 1
      {"LINE2: EXX \n", "LINE2", 2},         // PC = 2, size = 1
      {"LINE3: EX (SP), HL \n", "LINE3", 3}, // PC = 3, size = 1
      {"LINE4: EX (SP), IX \n", "LINE4", 4}, // PC = 4, size = 2
      {"LINE5: EX (SP), IY \n", "LINE5", 6}, // PC = 6, size = 2
      {"LINE6: LDI \n", "LINE6", 8},         // PC = 8, size = 2
      {"LINE7: LDIR \n", "LINE7", 10},       // PC = 10, size = 2
      {"LINE8: LDD \n", "LINE8", 12},        // PC = 12, size = 2
      {"LINE9: LDDR \n", "LINE9", 14},       // PC = 14, size = 2
      {"LINE10: CPI \n", "LINE10", 16},      // PC = 16, size = 2
      {"LINE11: CPIR \n", "LINE11", 18},     // PC = 18, size = 2
      {"LINE12: CPD \n", "LINE12", 20},      // PC = 20, size = 2
      {"LINE13: CPDR \n", "LINE13", 22},     // PC = 22, size = 2
      {"LINE14: DB 1 \n", "LINE14", 24},     // PC = 24
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_arithmeticgroup8(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"ADD A, B \n", NULL, 0},                   // PC = 0, size = 1
      {"LINE1: ADD A, 1 \n", "LINE1", 1},         // PC = 1, size = 2
      {"LINE2: ADD A, (HL) \n", "LINE2", 3},      // PC = 3, size = 1
      {"LINE3: ADD A, (IX+1) \n", "LINE3", 4},    // PC = 4, size = 3
      {"LINE4: ADD A, (IY+1) \n", "LINE4", 7},    // PC = 7, size = 3
      {"LINE5: ADC A, A \n", "LINE5", 10},        // PC = 10, size = 1
      {"LINE6: ADC A, 1 \n", "LINE6", 11},        // PC = 11, size = 2
      {"LINE7: ADC A, (HL) \n", "LINE7", 13},     // PC = 13, size = 1
      {"LINE8: ADC A, (IX+1) \n", "LINE8", 14},   // PC = 14, size = 3
      {"LINE9: ADC A, (IY+1) \n", "LINE9", 17},   // PC = 17, size = 3
      {"LINE10: SUB A \n", "LINE10", 20},         // PC = 20, size = 1
      {"LINE11: SUB 1 \n", "LINE11", 21},         // PC = 21, size = 2
      {"LINE12: SUB (HL) \n", "LINE12", 23},      // PC = 23, size = 1
      {"LINE13: SUB (IX+1) \n", "LINE13", 24},    // PC = 24, size = 3
      {"LINE14: SUB (IY+1) \n", "LINE14", 27},    // PC = 27, size = 3
      {"LINE15: SBC A, A \n", "LINE15", 30},      // PC = 30, size = 1
      {"LINE16: SBC A, 1 \n", "LINE16", 31},      // PC = 31, size = 2
      {"LINE17: SBC A, (HL) \n", "LINE17", 33},   // PC = 33, size = 1
      {"LINE18: SBC A, (IX+1) \n", "LINE18", 34}, // PC = 34, size = 3
      {"LINE19: SBC A, (IY+1) \n", "LINE19", 37}, // PC = 37, size = 3
      {"LINE20: AND A \n", "LINE20", 40},         // PC = 40, size = 1
      {"LINE21: AND 1 \n", "LINE21", 41},         // PC = 41, size = 2
      {"LINE22: AND (HL) \n", "LINE22", 43},      // PC = 43, size = 1
      {"LINE23: AND (IX+1) \n", "LINE23", 44},    // PC = 44, size = 3
      {"LINE24: AND (IY+1) \n", "LINE24", 47},    // PC = 47, size = 3
      {"LINE25: OR A \n", "LINE25", 50},          // PC = 50, size = 1
      {"LINE26: OR 1 \n", "LINE26", 51},          // PC = 51, size = 2
      {"LINE27: OR (HL) \n", "LINE27", 53},       // PC = 53, size = 1
      {"LINE28: OR (IX+1) \n", "LINE28", 54},     // PC = 54, size = 3
      {"LINE29: OR (IY+1) \n", "LINE29", 57},     // PC = 57, size = 3
      {"LINE30: XOR A \n", "LINE30", 60},         // PC = 60, size = 1
      {"LINE31: XOR 1 \n", "LINE31", 61},         // PC = 61, size = 2
      {"LINE32: XOR (HL) \n", "LINE32", 63},      // PC = 63, size = 1
      {"LINE33: XOR (IX+1) \n", "LINE33", 64},    // PC = 64, size = 3
      {"LINE34: XOR (IY+1) \n", "LINE34", 67},    // PC = 67, size = 3
      {"LINE35: CP A \n", "LINE35", 70},          // PC = 70, size = 1
      {"LINE36: CP 1 \n", "LINE36", 71},          // PC = 71, size = 2
      {"LINE37: CP (HL) \n", "LINE37", 73},       // PC = 73, size = 1
      {"LINE38: CP (IX+1) \n", "LINE38", 74},     // PC = 74, size = 3
      {"LINE39: CP (IY+1) \n", "LINE39", 77},     // PC = 77, size = 3
      {"LINE40: INC A \n", "LINE40", 80},         // PC = 80, size = 1
      {"LINE41: INC (HL) \n", "LINE41", 81},      // PC = 81, size = 1
      {"LINE42: INC (IX+1) \n", "LINE42", 82},    // PC = 82, size = 3
      {"LINE43: INC (IY+1) \n", "LINE43", 85},    // PC = 85, size = 3
      {"LINE44: DEC A \n", "LINE44", 88},         // PC = 88, size = 1
      {"LINE45: DEC (HL) \n", "LINE45", 89},      // PC = 89, size = 1
      {"LINE46: DEC (IX+1) \n", "LINE46", 90},    // PC = 90, size = 3
      {"LINE47: DEC (IY+1) \n", "LINE47", 93},    // PC = 93, size = 3
      {"LINE48: DB 1 \n", "LINE48", 96},          // PC = 96
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_gparithmetic_cpucontrol(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"DAA \n", NULL, 0},               // PC = 0, size = 1
      {"LINE1: CPL \n", "LINE1", 1},     // PC = 1, size = 1
      {"LINE2: NEG \n", "LINE2", 2},     // PC = 2, size = 2
      {"LINE3: CCF \n", "LINE3", 4},     // PC = 4, size = 1
      {"LINE4: SCF \n", "LINE4", 5},     // PC = 5, size = 1
      {"LINE5: NOP \n", "LINE5", 6},     // PC = 6, size = 1
      {"LINE6: HALT \n", "LINE6", 7},    // PC = 7, size = 1
      {"LINE7: DI \n", "LINE7", 8},      // PC = 8, size = 1
      {"LINE8: EI \n", "LINE8", 9},      // PC = 9, size = 1
      {"LINE9: IM 0 \n", "LINE9", 10},   // PC = 10, size = 2
      {"LINE10: IM 1 \n", "LINE10", 12}, // PC = 12, size = 2
      {"LINE11: IM 2 \n", "LINE11", 14}, // PC = 14, size = 2
      {"LINE12: DB 1 \n", "LINE12", 16}, // PC = 16
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_arithmeticgroup16(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"ADD HL, BC \n", NULL, 0},              // PC = 0, size = 1
      {"LINE1: ADD HL, DE \n", "LINE1", 1},    // PC = 1, size = 1
      {"LINE2: ADD HL, HL \n", "LINE2", 2},    // PC = 2, size = 1
      {"LINE3: ADD HL, SP \n", "LINE3", 3},    // PC = 3, size = 1
      {"LINE4: ADD IX, BC \n", "LINE4", 4},    // PC = 4, size = 2
      {"LINE5: ADD IX, DE \n", "LINE5", 6},    // PC = 6, size = 2
      {"LINE6: ADD IX, IX \n", "LINE6", 8},    // PC = 8, size = 2
      {"LINE7: ADD IX, SP \n", "LINE7", 10},   // PC = 10, size = 2
      {"LINE8: ADD IY, BC \n", "LINE8", 12},   // PC = 12, size = 2
      {"LINE9: ADD IY, DE \n", "LINE9", 14},   // PC = 14, size = 2
      {"LINE10: ADD IY, IY \n", "LINE10", 16}, // PC = 16, size = 2
      {"LINE11: ADD IY, SP \n", "LINE11", 18}, // PC = 18, size = 2
      {"LINE12: ADC HL, BC \n", "LINE12", 20}, // PC = 20, size = 2
      {"LINE13: ADC HL, DE \n", "LINE13", 22}, // PC = 22, size = 2
      {"LINE14: ADC HL, HL \n", "LINE14", 24}, // PC = 24, size = 2
      {"LINE15: ADC HL, SP \n", "LINE15", 26}, // PC = 26, size = 2
      {"LINE16: SBC HL, BC \n", "LINE16", 28}, // PC = 28, size = 2
      {"LINE17: SBC HL, DE \n", "LINE17", 30}, // PC = 30, size = 2
      {"LINE18: SBC HL, HL \n", "LINE18", 32}, // PC = 32, size = 2
      {"LINE19: SBC HL, SP \n", "LINE19", 34}, // PC = 34, size = 2
      {"LINE20: INC BC \n", "LINE20", 36},     // PC = 36, size = 1
      {"LINE21: INC DE \n", "LINE21", 37},     // PC = 37, size = 1
      {"LINE22: INC HL \n", "LINE22", 38},     // PC = 38, size = 1
      {"LINE23: INC SP \n", "LINE23", 39},     // PC = 39, size = 1
      {"LINE24: INC IX \n", "LINE24", 40},     // PC = 40, size = 2
      {"LINE25: INC IY \n", "LINE25", 42},     // PC = 42, size = 2
      {"LINE26: DEC BC \n", "LINE26", 44},     // PC = 44, size = 1
      {"LINE27: DEC DE \n", "LINE27", 45},     // PC = 45, size = 1
      {"LINE28: DEC HL \n", "LINE28", 46},     // PC = 46, size = 1
      {"LINE29: DEC SP \n", "LINE29", 47},     // PC = 47, size = 1
      {"LINE30: DEC IX \n", "LINE30", 48},     // PC = 48, size = 2
      {"LINE31: DEC IY \n", "LINE31", 50},     // PC = 50, size = 2
      {"LINE32: DB 1 \n", "LINE32", 52},       // PC = 52
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_rotateshiftgroup(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"RLCA \n", NULL, 0},                    // PC = 0, size = 1
      {"LINE1: RRCA \n", "LINE1", 1},          // PC = 1, size = 1
      {"LINE2: RLA \n", "LINE2", 2},           // PC = 2, size = 1
      {"LINE3: RRA \n", "LINE3", 3},           // PC = 3, size = 1
      {"LINE4: RLC B \n", "LINE4", 4},         // PC = 4, size = 2
      {"LINE5: RLC (HL) \n", "LINE5", 6},      // PC = 6, size = 2
      {"LINE6: RLC (IX+1) \n", "LINE6", 8},    // PC = 8, size = 4
      {"LINE7: RLC (IY+1) \n", "LINE7", 12},   // PC = 12, size = 4
      {"LINE8: RRC B \n", "LINE8", 16},        // PC = 16, size = 2
      {"LINE9: RRC (HL) \n", "LINE9", 18},     // PC = 18, size = 2
      {"LINE10: RRC (IX+1) \n", "LINE10", 20}, // PC = 20, size = 4
      {"LINE11: RRC (IY+1) \n", "LINE11", 24}, // PC = 24, size = 4
      {"LINE12: RL B \n", "LINE12", 28},       // PC = 28, size = 2
      {"LINE13: RL (HL) \n", "LINE13", 30},    // PC = 30, size = 2
      {"LINE14: RL (IX+1) \n", "LINE14", 32},  // PC = 32, size = 4
      {"LINE15: RL (IY+1) \n", "LINE15", 36},  // PC = 36, size = 4
      {"LINE16: RR B \n", "LINE16", 40},       // PC = 40, size = 2
      {"LINE17: RR (HL) \n", "LINE17", 42},    // PC = 42, size = 2
      {"LINE18: RR (IX+1) \n", "LINE18", 44},  // PC = 44, size = 4
      {"LINE19: RR (IY+1) \n", "LINE19", 48},  // PC = 48, size = 4
      {"LINE20: SLA B \n", "LINE20", 52},      // PC = 52, size = 2
      {"LINE21: SLA (HL) \n", "LINE21", 54},   // PC = 54, size = 2
      {"LINE22: SLA (IX+1) \n", "LINE22", 56}, // PC = 56, size = 4
      {"LINE23: SLA (IY+1) \n", "LINE23", 60}, // PC = 60, size = 4
      {"LINE24: SRA B \n", "LINE24", 64},      // PC = 64, size = 2
      {"LINE25: SRA (HL) \n", "LINE25", 66},   // PC = 66, size = 2
      {"LINE26: SRA (IX+1) \n", "LINE26", 68}, // PC = 68, size = 4
      {"LINE27: SRA (IY+1) \n", "LINE27", 72}, // PC = 72, size = 4
      {"LINE28: SRL B \n", "LINE28", 76},      // PC = 76, size = 2
      {"LINE29: SRL (HL) \n", "LINE29", 78},   // PC = 78, size = 2
      {"LINE30: SRL (IX+1) \n", "LINE30", 80}, // PC = 80, size = 4
      {"LINE31: SRL (IY+1) \n", "LINE31", 84}, // PC = 84, size = 4
      {"LINE32: RLD \n", "LINE32", 88},        // PC = 88, size = 2
      {"LINE33: RRD \n", "LINE33", 90},        // PC = 90, size = 2
      {"LINE34: DB 1 \n", "LINE34", 92},       // PC = 92
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_bitgroup(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"BIT 0, A \n", NULL, 0},                   // PC = 0, size = 2
      {"LINE1: BIT 7, B \n", "LINE1", 2},         // PC = 2, size = 2
      {"LINE2: BIT 3, (HL) \n", "LINE2", 4},      // PC = 4, size = 2
      {"LINE3: BIT 4, (IX+1) \n", "LINE3", 6},    // PC = 6, size = 4
      {"LINE4: BIT 5, (IY+1) \n", "LINE4", 10},   // PC = 10, size = 4
      {"LINE5: SET 0, A \n", "LINE5", 14},        // PC = 14, size = 2
      {"LINE6: SET 7, B \n", "LINE6", 16},        // PC = 16, size = 2
      {"LINE7: SET 3, (HL) \n", "LINE7", 18},     // PC = 18, size = 2
      {"LINE8: SET 4, (IX+1) \n", "LINE8", 20},   // PC = 20, size = 4
      {"LINE9: SET 5, (IY+1) \n", "LINE9", 24},   // PC = 24, size = 4
      {"LINE10: RES 0, A \n", "LINE10", 28},      // PC = 28, size = 2
      {"LINE11: RES 7, B \n", "LINE11", 30},      // PC = 30, size = 2
      {"LINE12: RES 3, (HL) \n", "LINE12", 32},   // PC = 32, size = 2
      {"LINE13: RES 4, (IX+1) \n", "LINE13", 34}, // PC = 34, size = 4
      {"LINE14: RES 5, (IY+1) \n", "LINE14", 38}, // PC = 38, size = 4
      {"LINE15: DB 1 \n", "LINE15", 42},          // PC = 42
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_jumpgroup(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"JP 0xAABB \n", NULL, 0},                // PC = 0, size = 3
      {"LINE1: JP NZ, 0xAABB \n", "LINE1", 3},  // PC = 3, size = 3
      {"LINE2: JP Z, 0xAABB \n", "LINE2", 6},   // PC = 6, size = 3
      {"LINE3: JP NC, 0xAABB \n", "LINE3", 9},  // PC = 9, size = 3
      {"LINE4: JP C, 0xAABB \n", "LINE4", 12},  // PC = 12, size = 3
      {"LINE5: JP PO, 0xAABB \n", "LINE5", 15}, // PC = 15, size = 3
      {"LINE6: JP PE, 0xAABB \n", "LINE6", 18}, // PC = 18, size = 3
      {"LINE7: JP P, 0xAABB \n", "LINE7", 21},  // PC = 21, size = 3
      {"LINE8: JP M, 0xAABB \n", "LINE8", 24},  // PC = 24, size = 3
      {"LINE9: JP (HL) \n", "LINE9", 27},       // PC = 27, size = 1
      {"LINE10: JP (IX) \n", "LINE10", 28},     // PC = 28, size = 2
      {"LINE11: JP (IY) \n", "LINE11", 30},     // PC = 30, size = 2
      {"LINE12: JR 10 \n", "LINE12", 32},       // PC = 32, size = 2
      {"LINE13: JR NZ, 10 \n", "LINE13", 34},   // PC = 34, size = 2
      {"LINE14: JR Z, 10 \n", "LINE14", 36},    // PC = 36, size = 2
      {"LINE15: JR NC, 10 \n", "LINE15", 38},   // PC = 38, size = 2
      {"LINE16: JR C, 10 \n", "LINE16", 40},    // PC = 40, size = 2
      {"LINE17: DJNZ 10 \n", "LINE17", 42},     // PC = 42, size = 2
      {"LINE18: DB 1 \n", "LINE18", 44},        // PC = 44
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_callreturngroup(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"CALL 0xAABB \n", NULL, 0},                // PC = 0, size = 3
      {"LINE1: CALL NZ, 0xAABB \n", "LINE1", 3},  // PC = 3, size = 3
      {"LINE2: CALL Z, 0xAABB \n", "LINE2", 6},   // PC = 6, size = 3
      {"LINE3: CALL NC, 0xAABB \n", "LINE3", 9},  // PC = 9, size = 3
      {"LINE4: CALL C, 0xAABB \n", "LINE4", 12},  // PC = 12, size = 3
      {"LINE5: CALL PO, 0xAABB \n", "LINE5", 15}, // PC = 15, size = 3
      {"LINE6: CALL PE, 0xAABB \n", "LINE6", 18}, // PC = 18, size = 3
      {"LINE7: CALL P, 0xAABB \n", "LINE7", 21},  // PC = 21, size = 3
      {"LINE8: CALL M, 0xAABB \n", "LINE8", 24},  // PC = 24, size = 3
      {"LINE9: RET \n", "LINE9", 27},             // PC = 27, size = 1
      {"LINE10: RET NZ \n", "LINE10", 28},        // PC = 28, size = 1
      {"LINE11: RET Z \n", "LINE11", 29},         // PC = 29, size = 1
      {"LINE12: RET NC \n", "LINE12", 30},        // PC = 30, size = 1
      {"LINE13: RET C \n", "LINE13", 31},         // PC = 31, size = 1
      {"LINE14: RET PO \n", "LINE14", 32},        // PC = 32, size = 1
      {"LINE15: RET PE \n", "LINE15", 33},        // PC = 33, size = 1
      {"LINE16: RET P \n", "LINE16", 34},         // PC = 34, size = 1
      {"LINE17: RET M \n", "LINE17", 35},         // PC = 35, size = 1
      {"LINE18: RETI \n", "LINE18", 36},          // PC = 36, size = 2
      {"LINE19: RETN \n", "LINE19", 38},          // PC = 38, size = 2
      {"LINE20: RST 0x00 \n", "LINE20", 40},      // PC = 40, size = 1
      {"LINE21: RST 0x08 \n", "LINE21", 41},      // PC = 41, size = 1
      {"LINE22: RST 0x10 \n", "LINE22", 42},      // PC = 42, size = 1
      {"LINE23: RST 0x18 \n", "LINE23", 43},      // PC = 43, size = 1
      {"LINE24: RST 0x20 \n", "LINE24", 44},      // PC = 44, size = 1
      {"LINE25: RST 0x28 \n", "LINE25", 45},      // PC = 45, size = 1
      {"LINE26: RST 0x30 \n", "LINE26", 46},      // PC = 46, size = 1
      {"LINE27: RST 0x38 \n", "LINE27", 47},      // PC = 47, size = 1
      {"LINE28: DB 1 \n", "LINE28", 48},          // PC = 48
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_inputoutputgroup(void)
{
  // Define the test instructions together with the expected result.
  // The expected value of a label is the PC of the line it is defined on.
  test_driver_t driver[] = {
      {"IN A, (0x10) \n", NULL, 0},               // PC = 0, size = 2
      {"LINE1: IN A, (C) \n", "LINE1", 2},        // PC = 2, size = 2
      {"LINE2: IN B, (C) \n", "LINE2", 4},        // PC = 4, size = 2
      {"LINE3: IN C, (C) \n", "LINE3", 6},        // PC = 6, size = 2
      {"LINE4: IN D, (C) \n", "LINE4", 8},        // PC = 8, size = 2
      {"LINE5: IN E, (C) \n", "LINE5", 10},       // PC = 10, size = 2
      {"LINE6: IN H, (C) \n", "LINE6", 12},       // PC = 12, size = 2
      {"LINE7: IN L, (C) \n", "LINE7", 14},       // PC = 14, size = 2
      {"LINE8: INI \n", "LINE8", 16},             // PC = 16, size = 2
      {"LINE9: INIR \n", "LINE9", 18},            // PC = 18, size = 2
      {"LINE10: IND \n", "LINE10", 20},           // PC = 20, size = 2
      {"LINE11: INDR \n", "LINE11", 22},          // PC = 22, size = 2
      {"LINE12: OUT (0x10), A \n", "LINE12", 24}, // PC = 24, size = 2
      {"LINE13: OUT (C), A \n", "LINE13", 26},    // PC = 26, size = 2
      {"LINE14: OUT (C), B \n", "LINE14", 28},    // PC = 28, size = 2
      {"LINE15: OUT (C), C \n", "LINE15", 30},    // PC = 30, size = 2
      {"LINE16: OUT (C), D \n", "LINE16", 32},    // PC = 32, size = 2
      {"LINE17: OUT (C), E \n", "LINE17", 34},    // PC = 34, size = 2
      {"LINE18: OUT (C), H \n", "LINE18", 36},    // PC = 36, size = 2
      {"LINE19: OUT (C), L \n", "LINE19", 38},    // PC = 38, size = 2
      {"LINE20: OUTI \n", "LINE20", 40},          // PC = 40, size = 2
      {"LINE21: OTIR \n", "LINE21", 42},          // PC = 42, size = 2
      {"LINE22: OUTD \n", "LINE22", 44},          // PC = 44, size = 2
      {"LINE23: OTDR \n", "LINE23", 46},          // PC = 46, size = 2
      {"LINE24: DB 1 \n", "LINE24", 48},          // PC = 48
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
}

/**************************************************************************************************/
/**************************************************************************************************/
static void test_instruction_size_symbol_operands(void)
{
  // Instructions with symbols as operands. The sizes are the same as if the symbol was replaced by the value it
  // resolves to, regardless of whether the symbol is defined before (BACK) or after (FWD) the instruction.
  test_driver_t driver[] = {
      {"BACK: DB 1 \n", "BACK", 0},                   // PC = 0
      {"LINE1: ADD A, FWD \n", "LINE1", 1},           // PC = 1, size = 2
      {"LINE2: ADC A, FWD \n", "LINE2", 3},           // PC = 3, size = 2
      {"LINE3: SUB FWD \n", "LINE3", 5},              // PC = 5, size = 2
      {"LINE4: SBC A, FWD \n", "LINE4", 7},           // PC = 7, size = 2
      {"LINE5: AND FWD \n", "LINE5", 9},              // PC = 9, size = 2
      {"LINE6: OR FWD \n", "LINE6", 11},              // PC = 11, size = 2
      {"LINE7: XOR FWD \n", "LINE7", 13},             // PC = 13, size = 2
      {"LINE8: CP FWD \n", "LINE8", 15},              // PC = 15, size = 2
      {"LINE9: LD A, FWD \n", "LINE9", 17},           // PC = 17, size = 2
      {"LINE10: LD B, BACK \n", "LINE10", 19},        // PC = 19, size = 2
      {"LINE11: LD (HL), FWD \n", "LINE11", 21},      // PC = 21, size = 2
      {"LINE12: LD (IX+FWD), BACK \n", "LINE12", 23}, // PC = 23, size = 4
      {"LINE13: LD (IY+FWD), 5 \n", "LINE13", 27},    // PC = 27, size = 4
      {"LINE14: LD A, (IX+FWD) \n", "LINE14", 31},    // PC = 31, size = 3
      {"LINE15: ADD A, (IY+BACK) \n", "LINE15", 34},  // PC = 34, size = 3
      {"LINE16: INC (IX+FWD) \n", "LINE16", 37},      // PC = 37, size = 3
      {"LINE17: BIT 0, (IX+FWD) \n", "LINE17", 40},   // PC = 40, size = 4
      {"LINE18: RLC (IY+FWD) \n", "LINE18", 44},      // PC = 44, size = 4
      {"LINE19: LD HL, FWD \n", "LINE19", 48},        // PC = 48, size = 3
      {"LINE20: LD BC, BACK \n", "LINE20", 51},       // PC = 51, size = 3
      {"LINE21: LD SP, FWD \n", "LINE21", 54},        // PC = 54, size = 3
      {"LINE22: LD IX, FWD \n", "LINE22", 57},        // PC = 57, size = 4
      {"LINE23: LD IY, BACK \n", "LINE23", 61},       // PC = 61, size = 4
      {"LINE24: LD A, (FWD) \n", "LINE24", 65},       // PC = 65, size = 3
      {"LINE25: LD (FWD), A \n", "LINE25", 68},       // PC = 68, size = 3
      {"LINE26: LD HL, (FWD) \n", "LINE26", 71},      // PC = 71, size = 3
      {"LINE27: LD (FWD), HL \n", "LINE27", 74},      // PC = 74, size = 3
      {"LINE28: LD BC, (FWD) \n", "LINE28", 77},      // PC = 77, size = 4
      {"LINE29: LD (BACK), DE \n", "LINE29", 81},     // PC = 81, size = 4
      {"LINE30: LD IX, (FWD) \n", "LINE30", 85},      // PC = 85, size = 4
      {"LINE31: LD (FWD), IY \n", "LINE31", 89},      // PC = 89, size = 4
      {"LINE32: JP FWD \n", "LINE32", 93},            // PC = 93, size = 3
      {"LINE33: JP NZ, FWD \n", "LINE33", 96},        // PC = 96, size = 3
      {"LINE34: CALL BACK \n", "LINE34", 99},         // PC = 99, size = 3
      {"LINE35: CALL C, FWD \n", "LINE35", 102},      // PC = 102, size = 3
      {"LINE36: IN A, (FWD) \n", "LINE36", 105},      // PC = 105, size = 2
      {"LINE37: OUT (BACK), A \n", "LINE37", 107},    // PC = 107, size = 2
      {"FWD: DB 1 \n", "FWD", 109},                   // PC = 109
  };
  size_t driverCount = DRIVER_COUNT(driver);

  assembler_t *assembler = prepare_assembler(driver, driverCount);
  TEST_ASSERT_TRUE(assembler_pass_one(assembler));

  check_internal_symbols(assembler, driver, driverCount);
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->importedSymbols));
  TEST_ASSERT_EQUAL(0, linkedList_count(assembler->exportedSymbols));
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
  RUN_TEST(test_label_only);
  RUN_TEST(test_directive_equ);
  RUN_TEST(test_directive_org_symbol_backwards);
  RUN_TEST(test_symbols_case_sensitive);
  RUN_TEST(test_directive_lists);
  RUN_TEST(test_export_forward_declared);
  RUN_TEST(test_export_undefined);
  RUN_TEST(test_pc_end_of_memory);
  RUN_TEST(test_pc_overflow_directive);
  RUN_TEST(test_pc_overflow_instruction);
  RUN_TEST(test_pc_overflow_label);
  RUN_TEST(test_empty_source);
  RUN_TEST(test_comment_only_source);
  RUN_TEST(test_instruction_size_loadgroup8);
  RUN_TEST(test_instruction_size_loadgroup16);
  RUN_TEST(test_instruction_size_exchangegroup);
  RUN_TEST(test_instruction_size_arithmeticgroup8);
  RUN_TEST(test_instruction_size_gparithmetic_cpucontrol);
  RUN_TEST(test_instruction_size_arithmeticgroup16);
  RUN_TEST(test_instruction_size_rotateshiftgroup);
  RUN_TEST(test_instruction_size_bitgroup);
  RUN_TEST(test_instruction_size_jumpgroup);
  RUN_TEST(test_instruction_size_callreturngroup);
  RUN_TEST(test_instruction_size_inputoutputgroup);
  RUN_TEST(test_instruction_size_symbol_operands);
}
