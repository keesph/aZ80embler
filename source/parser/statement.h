#ifndef STATEMENT_H
#define STATEMENT_H

#include "directive_types.h"
#include "opcode_types.h"
#include "operand.h"
#include "parser/symbol.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_TOKENS_PER_OPERAND 5
#define OPCODE_MAX_LENGTH 4

/**
 * @brief Enum listing the different types of statements. Used to simplify
 * processing
 *
 */
typedef enum
{
  statement_label,
  statement_directive,
  statement_labeled_directive,
  statement_instruction,
  statement_labeled_instruction
} statement_types_t;

/**
 * @brief Struct representing a compiler directive consisting of type and
 * potential operand
 *
 */
typedef struct
{
  directive_types_t type;
  operand_t operand;
} directive_t;

/**
 * @brief Struct representing an instruction, i.e. opcode, operands, machine
 * code and relocation information
 *
 */
typedef struct
{
  uint8_t machinecode[OPCODE_MAX_LENGTH];

  opcode_type opcode;
  operand_t operand1;
  operand_t operand2;

  bool relocatable;
  uint8_t relocate_offset1;
  uint8_t relocate_offset2;
} instruction_t;

/**
 * @brief Struct representing a complete line statement
 *
 */
typedef struct
{
  size_t size;
  statement_types_t type;
  symbol_t label;
  directive_t directive;
  instruction_t instruction;
} statement_t;

#endif
