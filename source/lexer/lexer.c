#include "lexer.h"
#include "lexer/token.h"
#include "token_list.h"

#include <string.h>

#define LINE_BUFFER_SIZE 1024
#define MAX_LEXEME_LENGTH 1024

static bool treatAs_identifier(char *symbol);
static bool isValid_identifierSymbol(char *symbol);

static bool treatAs_literal(char *symbol);

static bool treatAs_string(char *symbol);

TokenList *lexer_tokenize(FILE *fp)
{

  uint32_t lineNumber = 0;
  uint32_t columnNumber = 0;
  uint32_t tokenNumber = 0;

  char lineBuffer[LINE_BUFFER_SIZE];
  char lexeme[MAX_LEXEME_LENGTH];
  char *currentChar;
  Token newToken;
  TokenList tokenList;

  if (!tokenList_init(&tokenList))
  {
    return NULL;
  }

  // Iterate through file
  while (!feof(fp))
  {
    fgets(&lineBuffer[0], LINE_BUFFER_SIZE, fp);
    lineNumber++;

    currentChar = &lineBuffer[0];

    // Iterate through line
    while ((*currentChar != '\0') ||
           (*currentChar != ';')) // Done at line end or start of comment
    {
      // Ignore leading white spaces
      while (*currentChar == ' ')
      {
        currentChar++;
        columnNumber++;
      }

      if (treatAs_identifier(currentChar))
      {
        // Create identifier lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));

        // Emplace first character
        lexeme[columnNumber] = *currentChar;
        currentChar++;

        while (*currentChar != '\0' && isValid_identifierSymbol(currentChar))
        {
          lexeme[columnNumber] = *currentChar;
          currentChar++;
          columnNumber++;
        }
        newToken = tokenize_identifier(lexeme);
        tokenNumber++;
        if (newToken.type == token_invalid)
        {
          printf("Invalid Token, Number: %d, Type: Identifier, Line: %d",
                 tokenNumber, lineNumber);
          return NULL;
        }
        tokenList_addToken(&tokenList, newToken);
      }
      else if (treatAs_literal(currentChar))
      // Create literal lexeme
      {
      }
      // Create string lexeme
      else if (treatAs_string(currentChar))
      {
      }
    }
  }
}

static bool treatAs_identifier(char *symbol)
{
  return ((*symbol >= 'a' && *symbol <= 'z') ||
          (*symbol >= 'A' && *symbol <= 'Z'));
}

static bool isValid_identifierSymbol(char *symbol)
{
  return ((*symbol >= 'a' && *symbol <= 'z') ||
          (*symbol >= 'A' && *symbol <= 'Z') ||
          (*symbol >= '0' && *symbol <= '9') || *symbol == '_' ||
          *symbol == '-');
}

static bool treatAs_literal(char *symbol)
{
  return (*symbol >= '0' && *symbol <= '9');
}

static bool treatAs_string(char *symbol) { return *symbol == '"'; }
