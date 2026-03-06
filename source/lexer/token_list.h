#ifndef TOKEN_LIST_H
#define TOKEN_LIST_H

#include "token.h"

#include <stdbool.h>

typedef struct tokenList TokenList;

bool tokenList_init(TokenList *list);
bool tokenList_free(TokenList *list);
bool tokenList_addToken(TokenList *list, Token token);
Token tokenList_getNextToken(TokenList *list);
Token tokenList_getPreviousToken(TokenList *list);
bool tokenList_seek(TokenList *list, int32_t offset);

#endif
