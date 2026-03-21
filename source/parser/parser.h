#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"
#include "lexer/token.h"

typedef struct parser parser_t;

bool parser_do_it(parser_t *parser, token_list_t *tokenlist);

// Return the currently processed token
token_t *get_token(parser_t *parser);

// Return true if the current token matches the expected type
bool expect_token(parser_t *parser, token_types_t expectedType);

// Adds the current statement to the statement list
void emit_statement(parser_t *parser);

// Move to the next token in the list. return NULL if no further entry in list
token_t *consume_token(parser_t *parser);

#endif
