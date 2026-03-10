#include "parser.h"
#include "lexer/token_list.h"
#include "logging/logging.h"
#include <stdio.h>
#include <string.h>

typedef struct
{
  bool done_pass1;
  bool done_pass2;

} parser_state;
static parser_state parserState;

static bool pass_one(TokenList *list);
static bool pass_two(TokenList *list);

FILE *parse_module(TokenList *list)
{
  // Reset parser state before parsing
  memset(&parserState, 0, sizeof(parserState));

  if (!pass_one(list))
  {
    LOG_ERROR("Parser Pass One failed!");
    return NULL;
  }
  if (!pass_two(list))
  {
    LOG_ERROR("Parser Pass Two failed!");
    return NULL;
  }

  // Open File and truncate it if already existing
  char *dummyName = "dummy.o";
  FILE *objectFile = fopen(dummyName, "w");
  if (!objectFile)
  {
    LOG_ERROR("Could not open Object File %s for writing", dummyName);
    return NULL;
  }
  return NULL;
}

static bool pass_one(TokenList *list)
{
  (void)list;
  return false;
}
static bool pass_two(TokenList *list)
{
  (void)list;
  return false;
}
