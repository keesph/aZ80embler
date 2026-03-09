#include "lexer/lexer.h"
#include "lexer/token.h"
#include "lexer/token_list.h"

#include "logging/logging.h"

int main(void)
{
  // Just a dummy for compilation. Not an issue that it is NULL for now!
  FILE *sourceFile = NULL;

  sourceFile = fopen("testfile.txt", "r");

  if (sourceFile != NULL)
  {
    TokenList *list = lexer_tokenize(sourceFile);
    if (list == NULL)
    {
      LOG_ERROR("Failed to tokenize file!");
      return -1;
    }
    TokenList_Iterator *listIterator = tokenList_getIterator(list);

    if (listIterator == NULL)
    {
      LOG_ERROR("Could not retrieve List iterator!\n");
      return -2;
    }

    int count = tokenList_count(list);
    int i = 1;
    LOG_INFO("Tokenized File. Got %d tokens", count);
    while (!tokenList_atEnd(listIterator))
    {

      LOG_INFO("%d:\t%s", i++,
               token_toString(tokenList_getToken(listIterator)->type));
    }

    return 0;
  }
}
