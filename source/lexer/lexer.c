#include "lexer.h"
#include "lexer/token.h"
#include "logging/logging.h"
#include "utility/linked_list.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define LINE_BUFFER_SIZE 1024
#define MAX_LEXEME_LENGTH 1024

// Convinience Macro to bounds-check the lexeme array before adding a new one
#define APPEND_SYMBOL_OR_CLEANUP(symbol)                                                                               \
  if (lexemeIndex >= &lexeme[MAX_LEXEME_LENGTH])                                                                       \
  {                                                                                                                    \
    return lexer_fail(tokenList, "[LINE: %d]: Malformed input. Lexeme size for a single token exceeded maximum!",      \
                      lexerState.lineNumber);                                                                          \
  }                                                                                                                    \
  *lexemeIndex = symbol;                                                                                               \
  lexemeIndex++;

typedef struct
{
  char *current;

  uint32_t lineNumber;
  uint32_t tokenNumber;
} lexer_state_t;

static char peek_current_symbol(lexer_state_t *state);
static char pop_current_symbol(lexer_state_t *state);
static bool match_current_symbol(lexer_state_t *state, char symbol);
static void *lexer_fail(token_list_t *list, const char *fmt, ...);
static token_t lex_identifier(lexer_state_t *state);
static token_t lex_literal(lexer_state_t *state);
static token_t lex_string(lexer_state_t *state);

static bool is_valid_identifier_symbol(char symbol);
static bool is_valid_literal_symbol(char symbol, bool isHex);

/**************************************************************************************************/
/**************************************************************************************************/
token_list_t *lexer_tokenize(FILE *fp)
{
  lexer_state_t lexerState = {0};

  char lineBuffer[LINE_BUFFER_SIZE];
  char lexeme[MAX_LEXEME_LENGTH];
  char *lexemeIndex;
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

    size_t len = strlen(lineBuffer);
    if ((len > 0) && (lineBuffer[strlen(lineBuffer) - 1] != '\n') && !feof(fp))
    {
      return lexer_fail(tokenList, "[LINE: %d]: Line length exceeded buffer!", lexerState.lineNumber);
    }

    lexerState.current = lineBuffer;

    // Iterate through line, character by character. Add tokens to the list
    // along the way
    while (!match_current_symbol(&lexerState, '\0') && !match_current_symbol(&lexerState, ';'))
    {
      memset(&newToken, 0, sizeof(newToken));
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

      // Handle case where lexeme starts with a character (lables, identifer, ...)
      if (isalpha(peek_current_symbol(&lexerState)))
      {
        newToken = lex_identifier(&lexerState);

        if (newToken.type == token_invalid)
        {
          return lexer_fail(tokenList, "[LINE: %d]: Invalid Token (Identifier)", lexerState.lineNumber);
        }
        lexerState.tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
      // Handle case where next lexeme starts with a number
      else if (isdigit(peek_current_symbol(&lexerState)))
      {
        newToken = lex_literal(&lexerState);

        if (newToken.type == token_invalid)
        {
          return lexer_fail(tokenList, "[LINE: %d]: Invalid Token Type: Literal", lexerState.lineNumber);
        }
        lexerState.tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
      // Handle case where next lexeme starts with a ", resulting in a string
      // literal
      else if (match_current_symbol(&lexerState, '"'))
      {

        newToken = lex_string(&lexerState);

        if (newToken.type == token_invalid)
        {
          return lexer_fail(tokenList, "[LINE: %d]: Invalid Token, Type: String", lexerState.lineNumber);
        }
        lexerState.tokenNumber++;
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
          return lexer_fail(tokenList, "[LINE: %d]: Invalid token, Unknown type", lexerState.lineNumber);
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
          symbol == '_');
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool is_valid_literal_symbol(char symbol, bool isHex)
{
  if (isHex)
  {
    return (isdigit(symbol) || (symbol >= 'a' && symbol <= 'f') || (symbol >= 'A' && symbol <= 'F'));
  }
  else
  {
    return isdigit(symbol);
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
static char peek_current_symbol(lexer_state_t *state)
{
  assert(state);

  return *state->current;
}

/**************************************************************************************************/
/**************************************************************************************************/
static char pop_current_symbol(lexer_state_t *state)
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
static bool match_current_symbol(lexer_state_t *state, char symbol)
{
  assert(state);
  return *state->current == symbol;
}

/**************************************************************************************************/
/**************************************************************************************************/
static void *lexer_fail(token_list_t *list, const char *fmt, ...)
{
  va_list params;
  va_start(params, fmt);
  LOG_ERROR(fmt, params);
  va_end(params);
  linkedList_destroy(list);
  return NULL;
}

/**************************************************************************************************/
/**************************************************************************************************/
static token_t lex_identifier(lexer_state_t *state)
{
  char lexeme[MAX_LEXEME_LENGTH] = {0};
  char *lexemeIndex;
  token_t newToken = {0};

  // Create identifier lexeme
  lexemeIndex = &lexeme[0];

  // Emplace first character
  char symbol = pop_current_symbol(state);
  *lexemeIndex = symbol;
  lexemeIndex++;

  while (!match_current_symbol(state, '\0') && is_valid_identifier_symbol(peek_current_symbol(state)))
  {
    if (lexemeIndex >= &lexeme[MAX_LEXEME_LENGTH - 1])
    {
      LOG_ERROR("[LINE: %d]: Malformed input. Lexeme size for a single token exceeded maximum!", state->lineNumber);
      return newToken;
    }

    *lexemeIndex = pop_current_symbol(state);
    lexemeIndex++;
  }
  *lexemeIndex = '\0';

  // Directly create label token if found, else create identifier token
  if (match_current_symbol(state, ':'))
  {
    pop_current_symbol(state);
    newToken.type = token_label;

    strncpy(newToken.data.label, lexeme, sizeof(newToken.data.label) - 1);
    newToken.data.label[sizeof(newToken.data.label) - 1] = '\0';
  }
  else
  {
    newToken = tokenize_identifier(lexeme);
  }

  return newToken;
}

/**************************************************************************************************/
/**************************************************************************************************/
static token_t lex_literal(lexer_state_t *state)
{
  char lexeme[MAX_LEXEME_LENGTH] = {0};
  char *lexemeIndex;
  token_t newToken = {0};

  // Create identifier lexeme
  lexemeIndex = &lexeme[0];

  // Handle hex and decimal numbers separate, remove leading zeroes for decimal
  bool isHexLiteral = false;
  if (peek_current_symbol(state) == '0')
  {
    char tmpLeadingZero = pop_current_symbol(state);
    if (match_current_symbol(state, 'x') || match_current_symbol(state, 'X'))
    {
      *lexemeIndex = tmpLeadingZero;
      lexemeIndex++;
      *lexemeIndex = pop_current_symbol(state);
      lexemeIndex++;
      isHexLiteral = true;
    }
    else
    {
      // Get rid of leading zeroes
      while (peek_current_symbol(state) == '0')
      {
        pop_current_symbol(state);
      }

      // If nothing valid follows, its a plane 0
      if (!isdigit(peek_current_symbol(state)))
      {
        *lexemeIndex = '0';
        lexemeIndex++;
      }
    }
  }

  while (!match_current_symbol(state, '\0') && (is_valid_literal_symbol(peek_current_symbol(state), isHexLiteral)))
  {
    if (lexemeIndex >= &lexeme[MAX_LEXEME_LENGTH - 1])
    {
      newToken.type = token_invalid;
      LOG_ERROR("[LINE: %d]: Malformed input. Lexeme size for a single token exceeded maximum!", state->lineNumber);
      return newToken;
    }
    *lexemeIndex = pop_current_symbol(state);
    lexemeIndex++;
  }

  return tokenize_literal(lexeme);
}

/**************************************************************************************************/
/**************************************************************************************************/
static token_t lex_string(lexer_state_t *state)
{
  char lexeme[MAX_LEXEME_LENGTH] = {0};
  char *lexemeIndex;
  token_t newToken = {0};

  // Create identifier lexeme
  lexemeIndex = &lexeme[0];

  // First remove the leading "
  pop_current_symbol(state);

  while (!match_current_symbol(state, '"'))
  {
    if (match_current_symbol(state, '\n') || match_current_symbol(state, '\0'))
    {
      LOG_ERROR("[LINE: %d]: String literal did not end with trailing quotation marks!", state->lineNumber);
      newToken.type = token_invalid;
      return newToken;
    }

    if (lexemeIndex >= &lexeme[MAX_LEXEME_LENGTH - 1])
    {
      LOG_ERROR("[LINE: %d]: Malformed input. Lexeme size for a single token exceeded maximum!", state->lineNumber);
      newToken.type = token_invalid;
      return newToken;
    }
    *lexemeIndex = pop_current_symbol(state);
    lexemeIndex++;
  }
  // Remove trailing "
  pop_current_symbol(state);
  return tokenize_string(lexeme);
}
