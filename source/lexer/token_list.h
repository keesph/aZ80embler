#ifndef TOKEN_LIST_H
#define TOKEN_LIST_H

#include "token.h"

#include <stdbool.h>

typedef struct tokenList TokenList;
typedef struct tokenListIterator TokenList_Iterator;

TokenList *tokenList_init();
bool tokenList_addToken(TokenList *list, Token token);
uint32_t tokenList_count(TokenList *list);
void tokenList_destroy(TokenList *list);

TokenList_Iterator *tokenList_getIterator(TokenList *list);
Token *tokenList_getToken(TokenList_Iterator *iterator);
Token *tokenList_peekNext(TokenList_Iterator *iterator);
bool tokenList_atEnd(TokenList_Iterator *iterator);
void tokenList_resetIterator(TokenList_Iterator *iterator);
#endif
