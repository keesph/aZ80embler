#ifndef LEXER_H
#define LEXER_H

#include "lexer/token_list.h"

#include <stdio.h>

TokenList *lexer_tokenize(FILE *fp);

#endif
