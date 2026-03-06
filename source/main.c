#include "lexer/lexer.h"

int main(void)
{
  // Just a dummy for compilation. Not an issue that it is NULL for now!
  FILE *sourceFile = NULL;
  lexer_tokenize(sourceFile);
  return 0;
}
