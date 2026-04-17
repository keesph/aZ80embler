#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "utility/linked_list.h"

typedef LinkedList SymbolList;

typedef struct
{
  lexer_state_t *lexer;
  parser_t *parser;
  SymbolList *symbolList;
  SymbolList *exportedSymbols;
  SymbolList *importedSymbols;

  uint16_t programCounter;
} assembler_t;

void assembler_initialize(assembler_t *assembler);
bool assembler_pass_one(assembler_t *assembler);
bool assembler_pass_two(assembler_t *assembler, bool outputBinary);

#endif
