#ifndef OPERAND_TYPES_H
#define OPERAND_TYPES_H

#include "lexer/token.h"
#include "parser/symbol.h"
#include "register_types.h"

#include <stdint.h>

/**
 * @brief Enumeration of the different types of opcode operands
 *
 */
typedef enum
{
  operand_NA,        // no operand present
  operand_r,         // Register A, B, C, D, E, E, H, L
  operand_rr,        // Register Pairs BC, DE, IX, IY, HL or SP
  operand_deref_rr,  // (HL),(BC),(DE),(SP)
  operand_deref_idx, // (IX+d),(IY+d)
  operand_n,         // 1 byte unsigned integer 0 - 255
  operand_nn,        // 2 byte unsigned integer 0 - 65535
  operand_deref_nn,  // (nn)
  operand_b,         // one bit expression in the range 0 - 7 (7 = MSB, 0 = LSB)
  operand_e,         // one byte signed integer -126 - + 129 for relative jump offsets
  operand_cc,        // Status in the Flag register (NZ, Z, NC, C, PO, PE, P, M)
  operand_symbol,    // Placeholder for a value represented by a symbol
  operand_string,    // String literal "string_literal"
} operand_type_t;

/**
 * @brief Struct representing a dereferencing indexed register operand (IX+d) or
 * (IY+d)
 *
 */
typedef struct
{
  register_type index_register;
  int8_t index;
} deref_idx;

/**
 * @brief Union of the data representing the different operands
 *
 */
typedef union
{
  register_type r;
  register_type rr;
  register_type dereference_rr;
  deref_idx dereference_idx;
  uint8_t immediate_n;
  uint16_t immediate_nn;
  uint16_t dereference_nn;
  uint8_t bit_expression_b;
  int16_t relative_offset_e;
  register_type status_flag;
  symbol_t symbol;
  char string_literal[STRING_MAX_LENGTH];
} operand_data_t;

typedef struct
{
  operand_type_t type;
  operand_data_t data;
} operand_t;

#endif
