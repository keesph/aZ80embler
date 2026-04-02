#ifndef LEXER_H
#define LEXER_H

#include "utility/linked_list.h"
#include <stdio.h>

typedef LinkedList token_list_t;
typedef struct lexer_state lexer_state_t;

// Allocate a new lexer
lexer_state_t *lexer_initialize();

// DeAllocate an existing lexer
void lexer_destroy(lexer_state_t *lexer);

// Resets a lexer back to the initialized state
void lexer_reset(lexer_state_t *lexer);

// Reads the content of the file and turns it into a list of tokens
// The file handle must be already open and valid
bool lexer_tokenize(lexer_state_t *lexer, FILE *fp);

// Retrieve the list of tokens generated during tokenization
token_list_t *lexer_getTokenList(lexer_state_t *lexer);

#endif
