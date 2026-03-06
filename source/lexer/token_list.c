#include "token_list.h"
#include "lexer/token.h"

#include <stdlib.h>

#define DEFAULT_CAPACITY 128

typedef struct tokenList
{
  Token *tokens;
  uint32_t capacity;
  uint32_t count;
  uint32_t currentIndex;
} TokenList;

bool tokenList_init(TokenList *list)
{
  list->capacity = DEFAULT_CAPACITY;
  list->count = 0;
  list->currentIndex = 0;
  list->tokens = malloc(sizeof(Token) * list->capacity);
  return (list->tokens != NULL);
}

bool tokenList_free(TokenList *list)
{
  if (list->tokens != NULL)
  {
    free(list->tokens);
    return true;
  }

  return false;
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
  list->count++;
  list->tokens[list->count] = token;
  return true;
}

Token tokenList_getNextToken(TokenList *list)
{
  Token token = {.type = token_invalid};

  // Check if list it initialized or if no more tokens available
  if ((list->tokens == NULL) || (list->currentIndex >= (list->count - 1)))
  {
    return token;
  }
  token = list->tokens[list->currentIndex];
  list->currentIndex++;
  return token;
}

Token tokenList_getPreviousToken(TokenList *list)
{
  Token token = {.type = token_invalid};

  // Check if list it initialized or if no previous token is available
  if ((list->tokens == NULL) || (list->currentIndex == 0))
  {
    return token;
  }

  token = list->tokens[(list->currentIndex - 1)];
  return token;
}
