#include "token.h"
#include "identifier.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

  // If identifier coult not be mapped to predefined ones, it is seen as symbol
  if (token.type == token_invalid)
  {
    token.type = token_symbol;
    strncpy(token.data.symbol, identifier, LABEL_MAX_LENGTH);
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

char *token_toString(token_type type)
{
  switch (type)
  {
  case token_invalid:
    return "INVALID";
    break;

  case token_eof:
    return "EOF";
    break;

  case token_eol:
    return "EOL";
    break;

  case token_comma:
    return "COMMA";
    break;

  case token_lparenthesis:
    return "L-PARENTHESIS";
    break;

  case token_rparenthesis:
    return "R-PARENTHESIS";
    break;

  case token_opcode:
    return "OPCODE";
    break;

  case token_register:
    return "REGISTER";
    break;

  case token_directive:
    return "DIRECTIVE";
    break;

  case token_label:
    return "LABEL";
    break;
  case token_symbol:
    return "SYMBOL";
    break;
  case token_literal_byte:
    return "LITERAL_BYTE";
    break;

  case token_literal_sbyte:
    return "LITERAL_SIGNED_BYTE";
    break;

  case token_literal_word:
    return "LITERAL_WORD";
    break;

  case token_string:
    return "STRING";
    break;

  case token_comment:
    return "COMMENT";
    break;

  case token_plus:
    return "PLUS";
    break;

  case token_minus:
    return "MINUS";
    break;

  case token_div:
    return "DIVISION";
    break;

  case token_mul:
    return "MULTIPLICATION";
    break;

  default:
    return NULL;
    break;
  }
  return NULL;
}
