#ifndef LEXER_H
#define LEXER_H

#include "utility/linked_list.h"
#include <stdio.h>

typedef LinkedList token_list_t;
typedef struct lexer_state lexer_state_t;

/**
 * @brief Initializes a lexer state object
 *
 * @return lexer_state_t*
 */
lexer_state_t *lexer_initialize();

/**
 * @brief Reads the input file and returns a linked list consisting of valid Z80
 *        assembly tokens
 *
 * @param lexer Pointer to a lexer state object
 * @param fp Pointer to an already opened file to lex
 * @return true of lexing was successfull, false if not
 */
bool lexer_tokenize(lexer_state_t *lexer, FILE *fp);

/**
 * @brief Return list of already parsed tokens from the lexer
 *
 * @param lexer
 * @return token_list_t*
 */
token_list_t *lexer_getTokenList(lexer_state_t *lexer);

#endif
