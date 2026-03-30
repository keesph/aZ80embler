#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"

typedef struct parser parser_t;
typedef LinkedList statement_list_t;

/**
 * @brief Initializes the parser state object
 *
 * @return parser_t*
 */
parser_t *parser_initialize();

/**
 * @brief Converts a given list of tokens into a list of intermediate instruction representations
 *
 * @param parser
 * @param tokenlist
 * @return true if success, false if not
 */
bool parser_do_it(parser_t *parser, token_list_t *list);

#endif
