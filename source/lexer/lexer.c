#include "lexer.h"
#include "lexer/token.h"
#include "token_list.h"

#include <ctype.h>
#include <string.h>

#define LINE_BUFFER_SIZE 1024
#define MAX_LEXEME_LENGTH 1024

static bool isValid_identifierSymbol(char *symbol);

TokenList *lexer_tokenize(FILE *fp)
{

  uint32_t lineNumber = 0;
  uint32_t tokenNumber = 0;
  char *lexemeIndex;

  char lineBuffer[LINE_BUFFER_SIZE];
  char lexeme[MAX_LEXEME_LENGTH];
  char *currentChar;
  Token newToken;

  TokenList *tokenList = tokenList_init();

  if (tokenList == NULL)
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
    while ((*currentChar != '\0') &&
           (*currentChar != ';')) // Done at line end or start of comment
    {
      // Ignore leading white spaces
      while (isspace(*currentChar))
      {
        currentChar++;
      }

      if (isalpha(*currentChar))
      {
        // Create identifier lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        // Emplace first character
        *lexemeIndex++ = *currentChar;
        currentChar++;

        while (*currentChar != '\0' && isValid_identifierSymbol(currentChar))
        {
          *lexemeIndex++ = *currentChar;
          currentChar++;
        }
        *lexemeIndex = '\0';

        // Directly create label token if found, else create identifier token
        if (*currentChar == ':')
        {
          newToken.type = token_label;
          strncpy(newToken.data.label, lexeme, sizeof(newToken.data.label));
        }
        else
        {
          newToken = tokenize_identifier(lexeme);
        }
        tokenNumber++;
        if (newToken.type == token_invalid)
        {
          printf("Invalid Token, Number: %d, Type: Identifier, Line: %d",
                 tokenNumber, lineNumber);
          return NULL;
        }
        tokenList_addToken(tokenList, newToken);
      }
      else if (isdigit(*currentChar))
      {
        // Create literal lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        *lexemeIndex++ = *currentChar++;
        if (isdigit(*currentChar) || *currentChar == 'x' || *currentChar == 'X')
        {
          *lexemeIndex++ = *currentChar++;
          if (!isdigit(*currentChar))
          {
            printf(
                "Invalid literal lexeme in line %d! Ends in H or h is invalid!",
                lineNumber);
            return NULL;
          }
          while (*currentChar != '\0' && isdigit(*currentChar))
          {
            *lexemeIndex++ = *currentChar++;
          }
          newToken = tokenize_literal(lexeme);
          tokenNumber++;
          if (newToken.type == token_invalid)
          {
            printf("Invalid Token, Number: %d, Type: Literal, Line: %d",
                   tokenNumber, lineNumber);
            return NULL;
          }
          tokenList_addToken(tokenList, newToken);
        }
      }
      else if (*currentChar == '"')
      {
        // Create string lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        *lexemeIndex++ = *currentChar++;
        while (*currentChar != '\n')
        {
          *lexemeIndex++ = *currentChar;
          if (*currentChar == '"')
          {
            break;
          }
          currentChar++;
        }
        newToken = tokenize_string(lexeme);
        tokenNumber++;
        if (newToken.type == token_invalid)
        {
          printf("Invalid Token, Number: %d, Type: String, Line: %d",
                 tokenNumber, lineNumber);
          return NULL;
        }
        tokenList_addToken(tokenList, newToken);
      }
    }
  }
  return tokenList;
}

static bool isValid_identifierSymbol(char *symbol)
{
  return ((*symbol >= 'a' && *symbol <= 'z') ||
          (*symbol >= 'A' && *symbol <= 'Z') ||
          (*symbol >= '0' && *symbol <= '9') || *symbol == '_' ||
          *symbol == '-');
}
