#include "token_list.h"
#include "lexer/token.h"
#include "logging/logging.h"

#include <stdlib.h>

#define DEFAULT_CAPACITY 128

typedef struct tokenList
{
  Token *tokens;
  uint32_t capacity;
  uint32_t count;
  uint32_t currentIndex;
} TokenList;

typedef struct tokenListIterator
{
  Token *start;
  Token *end;
  Token *current_pos;
} TokenList_Iterator;

TokenList *tokenList_init()
{
  TokenList *list = (TokenList *)malloc(sizeof(TokenList));
  if (list == NULL)
  {
    return list;
  }
  list->capacity = DEFAULT_CAPACITY;
  list->count = 0;
  list->currentIndex = 0;
  list->tokens = malloc(sizeof(Token) * list->capacity);
  if (list->tokens == NULL)
  {
    return NULL;
  }
  return list;
}

bool tokenList_addToken(TokenList *list, Token token)
{
  // Increase array size of required
  if (list->count >= list->capacity)
  {
    list->capacity *= 2;
    list->tokens = realloc(list->tokens, (sizeof(Token) * list->capacity));
    if (list->tokens == NULL)
    {
      return false;
    }
  }

  // Add new token to end of array
  list->tokens[list->count++] = token;
  return true;
}

uint32_t tokenList_count(TokenList *list) { return list->count; }

void tokenList_destroy(TokenList *list)
{
  if (list)
  {
    free(list->tokens);
    free(list);
  }
}

TokenList_Iterator *tokenList_getIterator(TokenList *list)
{
  TokenList_Iterator *iterator = malloc(sizeof(TokenList_Iterator));
  if (iterator == NULL)
  {
    LOG_ERROR("Failed to allocate memory for token list iterator!");
    return NULL;
  }
  iterator->start = &list->tokens[0];
  iterator->end = &list->tokens[list->count];
  iterator->current_pos = iterator->start;

  return iterator;
}

Token *tokenList_getToken(TokenList_Iterator *iterator)
{
  if (iterator->current_pos < iterator->end)
  {
    Token *currentToken = iterator->current_pos;
    iterator->current_pos++;
    return currentToken;
  }
  else
  {
    LOG_INFO("Can not retrieve next token. Arrived at end of list");
    return NULL;
  }
}

Token *tokenList_peekNext(TokenList_Iterator *iterator)
{
  if ((iterator->current_pos == (iterator->end - 1)) ||
      (iterator->current_pos == iterator->end))
  {
    LOG_INFO("Can not Peek at next token, iterate is at end of list");
    return NULL;
  }

  return (iterator->current_pos + 1);
}

bool tokenList_atEnd(TokenList_Iterator *iterator)
{
  return (iterator->current_pos >= iterator->end);
}

void tokenList_resetIterator(TokenList_Iterator *iterator)
{
  iterator->current_pos = iterator->start;
}
