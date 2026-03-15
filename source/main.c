#include "lexer/lexer.h"
#include "lexer/token.h"

#include "logging/logging.h"
#include "utility/linked_list.h"

static void iterateCb(void *token, uint32_t iteration)
{
  LOG_INFO("%d:\t%s", ++iteration, token_toString(((token_t *)token)->type));
}

int main(void)
{
  // Just a dummy for compilation. Not an issue that it is NULL for now!
  FILE *sourceFile = NULL;

  sourceFile = fopen("testfile.txt", "r");

  if (sourceFile != NULL)
  {
    token_list_t *list = lexer_tokenize(sourceFile);
    if (list == NULL)
    {
      LOG_ERROR("Failed to tokenize file!");
      return -1;
    }

    int count = linkedList_count(list);
    LOG_INFO("Tokenized File. Got %d tokens", count);
    linkedList_iterate(list, iterateCb);

    return 0;
  }
}
