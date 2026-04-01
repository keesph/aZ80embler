#include "token.h"
#include "identifier.h"
#include "logging/logging.h"
#include "utility/alloc_w.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STRING_MAX_LENGTH 256

token_t tokenize_identifier(char *identifier)
{

  // Allocate a new token object
  token_t token = {0};

  token.type = token_invalid;

  // Iterate through identifier array which maps the string form to propper
  // types
  for (int i = 0; i < (int)(IDENTIFIER_COUNT); i++)
  {
    if (strcasecmp(identifier, identifiers[i].name) == 0)
    {
      token.type = identifiers[i].type;
      if (token.type == token_opcode)
      {
        token.data.opcodeType = identifiers[i].identifier.opcode;
      }
      else if (token.type == token_register)
      {
        token.data.registerType = identifiers[i].identifier.reg;
      }
      else
      {
        token.data.directiveType = identifiers[i].identifier.directive;
      }
      break;
    }
  }

  // If identifier coult not be mapped to predefined ones, it is seen as symbol
  if (token.type == token_invalid)
  {
    token.type = token_symbol;
    token.data.symbol = malloc(strlen(identifier) + 1);

    if (token.data.symbol == NULL)
    {
      LOG_ERROR("Could not allocate memory for symbol token!");
      token.type = token_invalid;
      return token;
    }

    strcpy(token.data.symbol, identifier);
  }
  return token;
}

token_t tokenize_literal(char *literal)
{
  token_t token = {0};

  token.type = token_invalid;

  errno = 0;
  char *end_ptr = NULL;

  long number = strtol(literal, &end_ptr, 0);

  if (((number == 0) && (errno != 0)) || (errno == EINVAL) || *end_ptr != '\0')
  {
    // Invalid string. Could not parse
    token.type = token_invalid;
    return token;
  }

  if (number >= INT8_MIN && number <= UINT16_MAX)
  {
    if ((number >= INT8_MIN) && (number <= UINT8_MAX))
    {
      token.type = token_literal_byte;
      token.data.literal_byte = (uint8_t)number;
    }
    else
    {
      token.type = token_literal_word;
      token.data.literal_word = number;
    }
  }
  else
  {
    token.type = token_invalid;
  }

  return token;
}

token_t tokenize_string(char *string)
{
  token_t token = {0};
  char buffer[STRING_MAX_LENGTH] = {0};

  // Check if string start with "
  if (*string != '"')
  {
    token.type = token_invalid;
    return token;
  }

  string++;

  // Check if string is empty
  if (*string == '"')
  {
    token.type = token_invalid;
    return token;
  }

  uint16_t characterCount = 0;
  while (*string != '"')
  {
    // Check for invalid symbols
    if (*string == '\n' || *string == '\r')
    {
      token.type = token_invalid;
      return token;
    }

    buffer[characterCount] = *string;
    string++;
    characterCount++;

    // Error if string is too long. Account for terminator
    if (characterCount >= (STRING_MAX_LENGTH - 1))
    {
      token.type = token_invalid;
      return token;
    }
  } // While

  token.data.string = malloc(strlen(buffer) + 1);
  if (token.data.string == NULL)
  {
    LOG_ERROR("Failed to allocate memory for string literal!");
    token.type = token_invalid;
    return token;
  }
  token.type = token_string;
  return token;
}

void token_toString(token_types_t type, char **buffer)
{
  switch (type)
  {
  case token_invalid:
    *buffer = strdup_w("INVALID");
    break;

  case token_eof:
    *buffer = strdup_w("EOF");
    break;

  case token_eol:
    *buffer = strdup_w("EOL");
    break;

  case token_comma:
    *buffer = strdup_w("COMMA");
    break;

  case token_lparenthesis:
    *buffer = strdup_w("L-PARENTHESIS");
    break;

  case token_rparenthesis:
    *buffer = strdup_w("R-PARENTHESIS");
    break;

  case token_opcode:
    *buffer = strdup_w("OPCODE");
    break;

  case token_register:
    *buffer = strdup_w("REGISTER");
    break;

  case token_directive:
    *buffer = strdup_w("DIRECTIVE");
    break;

  case token_label:
    *buffer = strdup_w("LABEL");
    break;
  case token_symbol:
    *buffer = strdup_w("SYMBOL");
    break;
  case token_literal_byte:
    *buffer = strdup_w("LITERAL_BYTE");
    break;

  case token_literal_sbyte:
    *buffer = strdup_w("LITERAL_SIGNED_BYTE");
    break;

  case token_literal_word:
    *buffer = strdup_w("LITERAL_WORD");
    break;

  case token_string:
    *buffer = strdup_w("STRING");
    break;

  case token_comment:
    *buffer = strdup_w("COMMENT");
    break;

  case token_plus:
    *buffer = strdup_w("PLUS");
    break;

  case token_minus:
    *buffer = strdup_w("MINUS");
    break;

  case token_div:
    *buffer = strdup_w("DIVISION");
    break;

  case token_mul:
    *buffer = strdup_w("MULTIPLICATION");
    break;

  default:
    LOG_ERROR("Invalid token type to string! Aboring!");
    abort();
    break;
  }
}
