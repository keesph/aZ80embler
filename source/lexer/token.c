#include "token.h"
#include "identifier.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

Token tokenize_identifier(char *identifier)
{

  // Allocate a new token object
  Token token = {0};

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

  return token;
}

Token tokenize_literal(char *literal)
{
  Token token = {0};

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

Token tokenize_string(char *string)
{
  Token token = {0};

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

    token.data.string[characterCount] = *string;
    string++;
    characterCount++;

    // Error if string is too long. Account for terminator
    if (characterCount >= (STRING_MAX_LENGTH - 1))
    {
      token.type = token_invalid;
      return token;
    }
  } // While

  token.type = token_string;
  return token;
}
