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

  uint32_t programCounter; // 32 bit to detect a overflow of the 16 bit address space
} assembler_t;

typedef struct
{
  bool success;        // Indicates errors during evaluation
  bool resolved;       // If any symbols within expression where resolved
  int32_t value;       // Evaluation result. If resolved == false it contains the value
                       // that must be added to the symbols after it is resolved by the linker
  LinkedList *symbols; // Contains any unresolved symbols. Must be evaluated by the linker
} expression_result_t;

assembler_t *assembler_initialize(void);
bool assembler_pass_one(assembler_t *assembler);
bool assembler_pass_two(assembler_t *assembler, bool outputBinary);

expression_result_t evaluate_expression(operand_t *operand);

#endif
