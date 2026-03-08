#ifndef TOKEN_LIST_H
#define TOKEN_LIST_H

#include "token.h"

#include <stdbool.h>

typedef struct tokenList TokenList;

TokenList *tokenList_init();
bool tokenList_free(TokenList *list);
bool tokenList_addToken(TokenList *list, Token token);
Token tokenList_getNextToken(TokenList *list);
Token tokenList_getPreviousToken(TokenList *list);
Token *tokenList_getIterator(TokenList *list);
uint32_t tokenList_count(TokenList *list);
bool tokenList_seek(TokenList *list, int32_t offset);
void tokenList_destroy(TokenList *list);

#endif
