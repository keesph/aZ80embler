#include "lexer.h"

#include "lexer/token.h"
#include "logging/logging.h"
#include "utility/alloc_w.h"
#include "utility/linked_list.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define LINE_BUFFER_SIZE 1024
#define MAX_LEXEME_LENGTH 1024

typedef struct lexer_state
{
  char *current;
  token_list_t *tokenList;
  size_t lineNumber;
  uint32_t tokenNumber;
} lexer_state_t;

static char peek_current_symbol(lexer_state_t *state);
static char pop_current_symbol(lexer_state_t *state);
static bool match_current_symbol(lexer_state_t *state, char symbol);
static void *lexer_fail(lexer_state_t *lexer, const char *msg);
static token_t lex_identifier(lexer_state_t *state);
static token_t lex_literal(lexer_state_t *state);
static token_t lex_string(lexer_state_t *state);

static bool is_valid_identifier_symbol(char symbol);
static bool is_valid_literal_symbol(char symbol, bool isHex);

static void token_free_callback(void *nextToken);

lexer_state_t *lexer_initialize()
{
  lexer_state_t *state = calloc_w(1, sizeof(lexer_state_t));

  state->tokenList = linkedList_initialize(sizeof(token_t), token_free_callback, NULL);
  return state;
}

/**************************************************************************************************/
/**************************************************************************************************/
void lexer_destroy(lexer_state_t *lexer)
{
  assert(lexer);

  linkedList_destroy(lexer->tokenList);
  free(lexer);
}

/**************************************************************************************************/
/**************************************************************************************************/
void lexer_reset(lexer_state_t *lexer)
{
  assert(lexer);

  linkedList_clear(lexer->tokenList);
  lexer->tokenNumber = 0;
  lexer->lineNumber = 0;
  lexer->current = NULL;
}

/**************************************************************************************************/
/**************************************************************************************************/
bool lexer_tokenize(lexer_state_t *lexer, FILE *fp)
{
  assert(lexer);
  assert(fp);

  char lineBuffer[LINE_BUFFER_SIZE];
  token_t newToken;

  token_list_t *tokenList = lexer->tokenList;

  if (tokenList == NULL)
  {
    LOG_ERROR("Token List was NULL!");
    return NULL;
  }

  // Iterate through file, line by line
  while (fgets(&lineBuffer[0], LINE_BUFFER_SIZE, fp) != NULL)
  {
    lexer->lineNumber++;
    size_t len = strlen(lineBuffer);
    if ((len > 0) && (lineBuffer[strlen(lineBuffer) - 1] != '\n') && !feof(fp))
    {
      return lexer_fail(lexer, "Line length exceeded buffer!");
    }

    lexer->current = lineBuffer;

    // Iterate through line, character by character. Add tokens to the list along the way
    while (!match_current_symbol(lexer, '\0') && !match_current_symbol(lexer, ';'))
    {
      // Ignore leading white spaces and check again for end of line
      while (isspace(peek_current_symbol(lexer)))
      {
        pop_current_symbol(lexer);
      }

      // Can stop already if EOL or comment is reached
      if (match_current_symbol(lexer, '\0') || match_current_symbol(lexer, ';'))
      {
        break;
      }

      // Handle case where lexeme starts with a character (lables, identifer, ...)
      if (isalpha(peek_current_symbol(lexer)))
      {
        newToken = lex_identifier(lexer);

        if (newToken.type == token_invalid)
        {
          return lexer_fail(lexer, "Invalid Token (Identifier)");
        }
        lexer->tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
      // Handle case where next lexeme starts with a number
      else if (isdigit(peek_current_symbol(lexer)))
      {
        newToken = lex_literal(lexer);

        if (newToken.type == token_invalid)
        {
          return lexer_fail(lexer, "Invalid Token Type: Literal");
        }
        lexer->tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
      // Handle case where next lexeme starts with a ", resulting in a string
      // literal
      else if (match_current_symbol(lexer, '"'))
      {

        newToken = lex_string(lexer);

        if (newToken.type == token_invalid)
        {
          return lexer_fail(lexer, "Invalid Token, Type: String");
        }
        lexer->tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
      else
      {
        memset(&newToken, 0, sizeof(newToken));
        // Create simple tokens consisting of single characters directly
        if (match_current_symbol(lexer, ','))
        {
          newToken.type = token_comma;
        }
        else if (match_current_symbol(lexer, '('))
        {
          newToken.type = token_lparenthesis;
        }
        else if (match_current_symbol(lexer, ')'))
        {
          newToken.type = token_rparenthesis;
        }
        else if (match_current_symbol(lexer, '+'))
        {
          newToken.type = token_plus;
        }
        else if (match_current_symbol(lexer, '-'))
        {
          newToken.type = token_minus;
        }
        else if (match_current_symbol(lexer, '/'))
        {
          newToken.type = token_div;
        }
        else if (match_current_symbol(lexer, '*'))
        {
          newToken.type = token_mul;
        }
        else
        {
          return lexer_fail(lexer, "Invalid token, Unknown type");
        }
        pop_current_symbol(lexer);
        lexer->tokenNumber++;
        linkedList_append(tokenList, &newToken);
      }
    }
    // Reached end of a line
    newToken.type = token_eol;
    lexer->tokenNumber++;
    linkedList_append(tokenList, &newToken);
  }
  // Reached end of the file
  newToken.type = token_eof;
  lexer->tokenNumber++;
  linkedList_append(tokenList, &newToken);

  return tokenList;
}

/**************************************************************************************************/
/**************************************************************************************************/
token_list_t *lexer_getTokenList(lexer_state_t *lexer) { return lexer->tokenList; }

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
static void *lexer_fail(lexer_state_t *lexer, const char *msg)
{
  LOG_LEXER_ERROR(lexer, msg);
  linkedList_destroy(lexer->tokenList);
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

    // Allocate memory and store only pointer
    newToken.data.label = malloc(strlen(lexeme) + 1);
    if (newToken.data.label == NULL)
    {
      newToken.type = token_invalid;
      LOG_ERROR("Failed to allocate label memory!");
      return newToken;
    }

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
      // Get rid of leading zeroes. Who needs octal literals anyways?
      while (peek_current_symbol(state) == '0')
      {
        pop_current_symbol(state);
      }

      // If nothing valid follows, its a simple 0
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

/**************************************************************************************************/
/**************************************************************************************************/
static void token_free_callback(void *nextToken)
{
  token_t *token = (token_t *)nextToken;
  if (token->type == token_symbol)
  {
    free(token->data.symbol);
  }
  else if (token->type == token_string)
  {
    free(token->data.string);
  }
}
