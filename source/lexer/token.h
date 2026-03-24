#ifndef LX_TOKEN_H
#define LX_TOKEN_H

#include "types.h"

#include <stdbool.h>
#include <stdint.h>

// Definition of all supported token types
typedef enum
{
  token_invalid,

  // File Structure
  token_eof,
  token_eol,

  // Symbols
  token_comma,
  token_lparenthesis,
  token_rparenthesis,

  // Identifier
  token_opcode,
  token_register,
  token_flag,
  token_directive,
  token_label,
  token_symbol,

  // Data
  token_literal_byte,
  token_literal_sbyte,
  token_literal_word,
  token_string,
  token_comment,

  // Math
  token_plus,
  token_minus,
  token_div,
  token_mul
} token_types_t;

typedef union
{
  opcode_type opcodeType;
  register_type_t registerType;
  directive_types_t directiveType;
  char *label;
  char *string;
  uint16_t literal_word;
  uint8_t literal_byte;
  int8_t literal_sbyte;
  char *symbol;
} token_data;

typedef struct token
{
  token_types_t type;
  token_data data;
} token_t;

token_t tokenize_identifier(char *identifer);

token_t tokenize_literal(char *literal);

token_t tokenize_string(char *string);

char *token_toString(token_types_t type);

#endif
