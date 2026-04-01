#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"
#include "types.h"

typedef struct parser parser_t;
typedef LinkedList statement_list_t;

// Allocarte and initialize parser object
parser_t *parser_initialize();

// Parse a given token list using a parser object
bool parser_do_it(parser_t *parser, token_list_t *list);

// After parsing, retrieve the generated statement list
statement_list_t *parser_getStatementList(parser_t *parser);

// Utility functions to get string representations of different types
void parser_statementType_toString(statement_types_t type, char **buffer);
void parser_opcode_toString(opcode_t type, char **buffer);
void parser_register_toString(register_type_t type, char **buffer);
void parser_directive_toString(directive_t *dir, char **buffer);
#endif
