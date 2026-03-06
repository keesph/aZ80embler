#include "lexer.h"
#include "lexer/token.h"
#include "token_list.h"

#include <ctype.h>
#include <string.h>

#define LINE_BUFFER_SIZE 1024
#define MAX_LEXEME_LENGTH 1024

typedef struct lexerstate
{
  char *current;
  uint32_t position;
  uint32_t lineNumber;
  uint32_t tokenNumber;
} lexer_state;

static char peek_currentSymbol(lexer_state *state);
static char pop_currentSymbol(lexer_state *state);
static bool match_currentSymbol(lexer_state *state, char symbol);

static bool isValid_identifierSymbol(char symbol);

TokenList *lexer_tokenize(FILE *fp)
{
  lexer_state lexerState = {0};

  char *lexemeIndex;

  char lineBuffer[LINE_BUFFER_SIZE];
  char lexeme[MAX_LEXEME_LENGTH];
  Token newToken;

  TokenList *tokenList = tokenList_init();

  if (tokenList == NULL)
  {
    return NULL;
  }

  // Iterate through file
  while (fgets(&lineBuffer[0], LINE_BUFFER_SIZE, fp) != NULL)
  {
    lexerState.lineNumber++;
    lexerState.current = lineBuffer;

    // Iterate through line
    while (!match_currentSymbol(&lexerState, '\0') &&
           !match_currentSymbol(&lexerState, ';'))
    {
      // Ignore leading white spaces and check again for end of line
      while (isspace(peek_currentSymbol(&lexerState)))
      {
        pop_currentSymbol(&lexerState);
      }
      if (match_currentSymbol(&lexerState, '\0') ||
          match_currentSymbol(&lexerState, ';'))
      {
        break;
      }
      if (isalpha(peek_currentSymbol(&lexerState)))
      {
        // Create identifier lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        // Emplace first character
        *lexemeIndex++ = pop_currentSymbol(&lexerState);

        while (!match_currentSymbol(&lexerState, '\0') &&
               isValid_identifierSymbol(peek_currentSymbol(&lexerState)))
        {
          *lexemeIndex++ = pop_currentSymbol(&lexerState);
        }
        *lexemeIndex = '\0';

        // Directly create label token if found, else create identifier token
        if (match_currentSymbol(&lexerState, ':'))
        {
          pop_currentSymbol(&lexerState);
          newToken.type = token_label;
          strncpy(newToken.data.label, lexeme, sizeof(newToken.data.label));
        }
        else
        {
          newToken = tokenize_identifier(lexeme);
        }
        if (newToken.type == token_invalid)
        {
          printf("Invalid Token, Number: %d, Type: Identifier, Line: %d",
                 lexerState.tokenNumber, lexerState.lineNumber);
          tokenList_destroy(tokenList);
          return NULL;
        }
        tokenList_addToken(tokenList, newToken);
      }
      else if (isdigit(peek_currentSymbol(&lexerState)))
      {
        // Create literal lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        *lexemeIndex++ = pop_currentSymbol(&lexerState);

        char tempChar = peek_currentSymbol(&lexerState);
        while (!match_currentSymbol(&lexerState, '\0') &&
               (isdigit(tempChar) || (tempChar >= 'a' && tempChar <= 'f') ||
                (tempChar >= 'A' && tempChar <= 'F') || tempChar == 'x' ||
                tempChar == 'X'))
        {
          *lexemeIndex++ = pop_currentSymbol(&lexerState);
        }
        newToken = tokenize_literal(lexeme);
        lexerState.tokenNumber++;
        if (newToken.type == token_invalid)
        {
          printf("Invalid Token, Number: %d, Type: Literal, Line: %d",
                 lexerState.tokenNumber, lexerState.lineNumber);
          tokenList_destroy(tokenList);
          return NULL;
        }
        tokenList_addToken(tokenList, newToken);
      }
      else if (match_currentSymbol(&lexerState, '"'))
      {
        // Create string lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        *lexemeIndex++ = pop_currentSymbol(&lexerState);
        while (!match_currentSymbol(&lexerState, '\n') &&
               !match_currentSymbol(&lexerState, '\0'))
        {
          *lexemeIndex++ = pop_currentSymbol(&lexerState);
          if (match_currentSymbol(&lexerState, '"'))
          {
            pop_currentSymbol(&lexerState);
            break;
          }
        }
        newToken = tokenize_string(lexeme);
        lexerState.tokenNumber++;
        if (newToken.type == token_invalid)
        {
          printf("Invalid Token, Number: %d, Type: String, Line: %d",
                 lexerState.tokenNumber, lexerState.lineNumber);
          tokenList_destroy(tokenList);
          return NULL;
        }
        tokenList_addToken(tokenList, newToken);
      }
      else
      {
        // Parse simple tokens directly
        if (match_currentSymbol(&lexerState, ','))
        {
          newToken.type = token_comma;
        }
        else if (match_currentSymbol(&lexerState, '('))
        {
          newToken.type = token_lparenthesis;
        }
        else if (match_currentSymbol(&lexerState, ')'))
        {
          newToken.type = token_rparenthesis;
        }
        else if (match_currentSymbol(&lexerState, '+'))
        {
          newToken.type = token_plus;
        }
        else if (match_currentSymbol(&lexerState, '-'))
        {
          newToken.type = token_minus;
        }
        else if (match_currentSymbol(&lexerState, '/'))
        {
          newToken.type = token_div;
        }
        else if (match_currentSymbol(&lexerState, '*'))
        {
          newToken.type = token_mul;
        }
        else
        {
          printf("Invalid token in Line %d. Unknown type",
                 lexerState.lineNumber);
          tokenList_destroy(tokenList);
          return NULL;
        }
        pop_currentSymbol(&lexerState);
        lexerState.tokenNumber++;
        tokenList_addToken(tokenList, newToken);
      }
    }
    newToken.type = token_eol;
    lexerState.tokenNumber++;
    tokenList_addToken(tokenList, newToken);
  }
  newToken.type = token_eof;
  lexerState.tokenNumber++;
  tokenList_addToken(tokenList, newToken);

  printf("Finished parsing file. Retrieved %d tokens", lexerState.tokenNumber);
  return tokenList;
}

static bool isValid_identifierSymbol(char symbol)
{
  return ((symbol >= 'a' && symbol <= 'z') ||
          (symbol >= 'A' && symbol <= 'Z') ||
          (symbol >= '0' && symbol <= '9') || symbol == '_' || symbol == '-');
}

static char peek_currentSymbol(lexer_state *state) { return *state->current; }

static char pop_currentSymbol(lexer_state *state)
{
  char current = *state->current;
  if (current != '\0')
  {
    state->current++;
  }
  return current;
}

static bool match_currentSymbol(lexer_state *state, char symbol)
{
  return *state->current == symbol;
}
