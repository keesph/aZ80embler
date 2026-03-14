#ifndef LX_TOKEN_H
#define LX_TOKEN_H

#include <stdbool.h>
#include <stdint.h>

#include "directive_types.h"
#include "opcode_types.h"
#include "register_types.h"

#define LABEL_MAX_LENGTH 16
#define DIRECTIVE_MAX_LENGTH 16
#define OPERAND_MAX_LENGTH 8
#define REGISTER_MAX_LENGTH 2
#define STRING_MAX_LENGTH 256
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
} token_type;

typedef union
{
  opcode_type opcodeType;
  register_type registerType;
  directive_type directiveType;
  char label[LABEL_MAX_LENGTH];
  char string[STRING_MAX_LENGTH];
  uint16_t literal_word;
  uint8_t literal_byte;
  int8_t literal_sbyte;
  char symbol[LABEL_MAX_LENGTH];
} token_data;

typedef struct token
{
  token_type type;
  token_data data;
} Token;

Token tokenize_identifier(char *identifer);

Token tokenize_literal(char *literal);

Token tokenize_string(char *string);

char *token_toString(token_type type);

#endif
