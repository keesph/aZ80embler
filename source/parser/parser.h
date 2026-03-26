#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"

typedef struct parser parser_t;
typedef LinkedList statement_list_t;

/**
 * @brief Converts a given list of tokens into
 *
 * @param tokenlist
 * @return true
 * @return false
 */
statement_list_t *parser_do_it(token_list_t *tokenlist);

#endif
