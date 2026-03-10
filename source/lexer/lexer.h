#ifndef LEXER_H
#define LEXER_H

// #include "lexer/token_list.h"
#include "utility/linked_list.h"
#include <stdio.h>

typedef LinkedList TokenList;

TokenList *lexer_tokenize(FILE *fp);

#endif
