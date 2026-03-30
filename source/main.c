#include "assembler/assembler.h"
#include "lexer/lexer.h"
#include "lexer/token.h"

#include "logging/logging.h"
#include "parser/parser.h"
#include "utility/argument_parser.h"
#include "utility/linked_list.h"

static void token_iterateCb(void *token, uint32_t iteration)
{
  LOG_INFO("%d:\t%s", ++iteration, token_toString(((token_t *)token)->type));
}

int main(int argc, char *argv[])
{

  bool verbose = false;

  assembler_t assembler = {0};
  argument_list_t *arguments = get_argument_list(argc, argv);
  if (!arguments)
  {
    // Error already logged
    return -1;
  }
  assembler.lexer = lexer_initialize();
  if (!assembler.lexer)
  {
    LOG_ERROR("Failed lexer initialization. Aborting!");
    return -1;
  }

  assembler.parser = parser_initialize();
  if (!assembler.parser)
  {
    LOG_ERROR("Failed parser initialization. Aborting!");
    return -1;
  }

  FILE *sourceFile = sourceFile = fopen("testfile.txt", "r");

  if (!sourceFile)
  {
    LOG_ERROR("Failed to open input file. Aborting!");
    return -1;
  }

  if (!lexer_tokenize(assembler.lexer, sourceFile))
  {
    LOG_ERROR("Lexer failed! Aborting!");
    return -1;
  }

  token_list_t *list = lexer_getTokenList(assembler.lexer);
  if (verbose)
  {
    // Output all found tokens
    int count = linkedList_count(list);
    LOG_INFO("Tokenized File. Got %d tokens", count);
    linkedList_iterate(list, token_iterateCb);
  }

  if (!parser_do_it(assembler.parser, list))
  {
    LOG_ERROR("Parser failed! Aborting!");
    return -1;
  }

  return 0;
}
