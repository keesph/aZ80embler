#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "lexer/lexer.h"
#include "types.h"
#include "utility/linked_list.h"

#include <stddef.h>

typedef ListNode token_list_node_t;
typedef LinkedList statement_list_t;
typedef LinkedList symbol_list_t;

typedef enum
{
  parse_line_error,
  parse_line_next,
  parse_line_eof
} parse_line_result_t;

typedef struct parser
{
  statement_list_t *statementList;
  uint32_t programCounter; // Current offset in the program where new
                           // instructions will be placed

  token_list_t *inputTokenList;        // List of tokens to parse
  token_list_node_t *currentTokenNode; // Current token node being worked on

  size_t lineNumber;            // Count of processed lines
  statement_t currentStatement; // Is filled while parsing a line

  FILE *objectFile;
  symbol_list_t *internal_symbols;
  symbol_list_t *exported_symbols;
  symbol_list_t *imported_symbols;

} parser_t;

#endif
