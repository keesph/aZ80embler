#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer/lexer.h"
#include "parser/parser.h"

typedef struct
{
  lexer_state_t *lexer;
  parser_t *parser;
} assembler_t;

bool pass_one(statement_list_t *statementList);
bool pass_two(statement_list_t *statementList, bool outputBinary);

#endif
