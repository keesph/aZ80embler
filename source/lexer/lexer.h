#ifndef LEXER_H
#define LEXER_H

#include "utility/linked_list.h"
#include <stdio.h>

typedef LinkedList token_list_t;

/**
 * @brief Reads the input file and returns a linked list consisting of valid Z80
 *        assembly tokens
 *
 * @param fp Pointer to an already opened file to lex
 * @return TokenList* linked list containing all found tokens
 */
token_list_t *lexer_tokenize(FILE *fp);

#endif
