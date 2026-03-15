#include "lexer.h"
#include "lexer/token.h"
#include "logging/logging.h"
#include "utility/linked_list.h"
// #include "token_list.h"

#include <assert.h>
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

static char peek_current_symbol(lexer_state *state);
static char pop_current_symbol(lexer_state *state);
static bool match_current_symbol(lexer_state *state, char symbol);

static bool is_valid_identifier_symbol(char symbol);

/**************************************************************************************************/
/**************************************************************************************************/
token_list_t *lexer_tokenize(FILE *fp)
{
  lexer_state lexerState = {0};

  char *lexemeIndex;

  char lineBuffer[LINE_BUFFER_SIZE];
  char lexeme[MAX_LEXEME_LENGTH];
  token_t newToken;

  token_list_t *tokenList = linkedList_initialize(sizeof(token_t), NULL, NULL);

  if (tokenList == NULL)
  {
    return NULL;
  }

  // Iterate through file, line by line
  while (fgets(&lineBuffer[0], LINE_BUFFER_SIZE, fp) != NULL)
  {
    lexerState.lineNumber++;
    lexerState.current = lineBuffer;

    // Iterate through line, character by character. Add tokens to the list
    // along the way
    while (!match_current_symbol(&lexerState, '\0') && !match_current_symbol(&lexerState, ';'))
    {
      // Ignore leading white spaces and check again for end of line
      while (isspace(peek_current_symbol(&lexerState)))
      {
        pop_current_symbol(&lexerState);
      }

      // Can stop already if EOL or comment is reached
      if (match_current_symbol(&lexerState, '\0') || match_current_symbol(&lexerState, ';'))
      {
        break;
      }

      // Handle case where lexeme starts with a character (lables, ....)
      if (isalpha(peek_current_symbol(&lexerState)))
      {
        // Create identifier lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        // Emplace first character
        *lexemeIndex++ = pop_current_symbol(&lexerState);

        while (!match_current_symbol(&lexerState, '\0') && is_valid_identifier_symbol(peek_current_symbol(&lexerState)))
        {
          *lexemeIndex++ = pop_current_symbol(&lexerState);
        }
        *lexemeIndex = '\0';

        // Directly create label token if found, else create identifier token
        if (match_current_symbol(&lexerState, ':'))
        {
          pop_current_symbol(&lexerState);
          newToken.type = token_label;
          strncpy(newToken.data.label, lexeme, sizeof(newToken.data.label));
        }
        else
        {
          newToken = tokenize_identifier(lexeme);
        }
        if (newToken.type == token_invalid)
        {
          LOG_ERROR("Invalid Token (Identifier), Line: %d, %s", lexerState.lineNumber, &lexeme[0]);
          linkedList_destroy(tokenList);
          return NULL;
        }
        linkedList_append(tokenList, &newToken);
      }
      // Handle case where next lexeme starts with a number
      else if (isdigit(peek_current_symbol(&lexerState)))
      {
        // Create literal lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        *lexemeIndex++ = pop_current_symbol(&lexerState);

        char tempChar = peek_current_symbol(&lexerState);
        while (!match_current_symbol(&lexerState, '\0') &&
               (isdigit(tempChar) || (tempChar >= 'a' && tempChar <= 'f') || (tempChar >= 'A' && tempChar <= 'F') ||
                tempChar == 'x' || tempChar == 'X'))
        {
          *lexemeIndex++ = pop_current_symbol(&lexerState);
        }
        newToken = tokenize_literal(lexeme);
        lexerState.tokenNumber++;
        if (newToken.type == token_invalid)
        {
          LOG_ERROR("Invalid Token, Number: %d, Type: Literal, Line: %d", lexerState.tokenNumber,
                    lexerState.lineNumber);
          linkedList_destroy(tokenList);
          return NULL;
        }
        linkedList_append(tokenList, &newToken);
      }
      // Handle case where next lexeme starts with a ", resulting in a string
      // literal
      else if (match_current_symbol(&lexerState, '"'))
      {
        // Create string lexeme
        memset(&lexeme[0], 0, sizeof(lexeme));
        lexemeIndex = &lexeme[0];

        *lexemeIndex++ = pop_current_symbol(&lexerState);
        while (!match_current_symbol(&lexerState, '\n') && !match_current_symbol(&lexerState, '\0'))
        {
          *lexemeIndex++ = pop_current_symbol(&lexerState);
          if (match_current_symbol(&lexerState, '"'))
          {
            pop_current_symbol(&lexerState);
            break;
          }
        }
        newToken = tokenize_string(lexeme);
        lexerState.tokenNumber++;
        if (newToken.type == token_invalid)
        {
          LOG_ERROR("Invalid Token, Number: %d, Type: String, Line: %d", lexerState.tokenNumber, lexerState.lineNumber);
          linkedList_destroy(tokenList);
          return NULL;
        }
        linkedList_append(tokenList, &newToken);
      }
      else
      {
        // Create simple tokens consisting of single characters directly
        if (match_current_symbol(&lexerState, ','))
        {
          newToken.type = token_comma;
        }
        else if (match_current_symbol(&lexerState, '('))
        {
          newToken.type = token_lparenthesis;
        }
        else if (match_current_symbol(&lexerState, ')'))
        {
          newToken.type = token_rparenthesis;
        }
        else if (match_current_symbol(&lexerState, '+'))
        {
          newToken.type = token_plus;
        }
        else if (match_current_symbol(&lexerState, '-'))
        {
          newToken.type = token_minus;
        }
        else if (match_current_symbol(&lexerState, '/'))
        {
          newToken.type = token_div;
        }
        else if (match_current_symbol(&lexerState, '*'))
        {
          newToken.type = token_mul;
        }
        else
        {
          LOG_ERROR("Invalid token in Line %d. Unknown type", lexerState.lineNumber);
          linkedList_destroy(tokenList);
          return NULL;
        }
        pop_current_symbol(&lexerState);
        lexerState.tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
    }
    // Reached end of a line
    newToken.type = token_eol;
    lexerState.tokenNumber++;
    linkedList_append(tokenList, &newToken);
  }
  // Reached end of the file
  newToken.type = token_eof;
  lexerState.tokenNumber++;
  linkedList_append(tokenList, &newToken);

  LOG_INFO("Finished parsing file. Retrieved %d tokens", lexerState.tokenNumber);
  return tokenList;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool is_valid_identifier_symbol(char symbol)
{
  return ((symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || (symbol >= '0' && symbol <= '9') ||
          symbol == '_' || symbol == '-');
}

/**************************************************************************************************/
/**************************************************************************************************/
static char peek_current_symbol(lexer_state *state)
{
  assert(state);

  return *state->current;
}

/**************************************************************************************************/
/**************************************************************************************************/
static char pop_current_symbol(lexer_state *state)
{
  assert(state);

  char current = *state->current;
  if (current != '\0')
  {
    state->current++;
  }
  return current;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool match_current_symbol(lexer_state *state, char symbol)
{
  assert(state);
  return *state->current == symbol;
}
