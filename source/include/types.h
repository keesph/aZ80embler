#ifndef TYPES_H
#define TYPES_H

#include "parser/instruction_encoding.h"

#include <stdint.h>
#include <stdlib.h>

#define OPCODE_MAX_LENGTH 4

/**************************************************************************************/
// Opcode Types
/**************************************************************************************/
typedef enum
{
  opcode_LD,
  opcode_PUSH,
  opcode_POP,
  opcode_EX,
  opcode_EXX,
  opcode_LDI,
  opcode_LDIR,
  opcode_LDD,
  opcode_LDDR,
  opcode_CPI,
  opcode_CPIR,
  opcode_CPD,
  opcode_CPDR,
  opcode_ADD,
  opcode_ADC,
  opcode_SUB,
  opcode_SBC,
  opcode_AND,
  opcode_OR,
  opcode_XOR,
  opcode_CP,
  opcode_INC,
  opcode_DEC,
  opcode_DAA,
  opcode_CPL,
  opcode_NEG,
  opcode_CCF,
  opcode_SCF,
  opcode_NOP,
  opcode_HALT,
  opcode_DI,
  opcode_EI,
  opcode_IM,
  opcode_RLCA,
  opcode_RLA,
  opcode_RRCA,
  opcode_RRA,
  opcode_RLC,
  opcode_RL,
  opcode_RRC,
  opcode_RR,
  opcode_SLA,
  opcode_SRA,
  opcode_SRL,
  opcode_RLD,
  opcode_RRD,
  opcode_BIT,
  opcode_SET,
  opcode_RES,
  opcode_JP,
  opcode_JR,
  opcode_DJNZ,
  opcode_CALL,
  opcode_RET,
  opcode_RETI,
  opcode_RETN,
  opcode_RST,
  opcode_IN,
  opcode_INI,
  opcode_INIR,
  opcode_IND,
  opcode_INDR,
  opcode_OUT,
  opcode_OUTI,
  opcode_OTIR,
  opcode_OUTD,
  opcode_OTDR,
} opcode_t;

/**************************************************************************************/
// Register Types
/**************************************************************************************/
typedef enum
{
  // General Purpose Single
  register_A,
  register_B,
  register_C,
  register_D,
  register_E,
  register_H,
  register_L,

  // General Purpose Double
  register_BC,
  register_DE,
  register_HL,
  register_AF,

  // Special Purpose
  register_IX,
  register_IY,
  register_SP,
  register_I,
  register_R,

  // Flag Registers (C is already defined as normal register)
  register_NZ,
  register_Z,
  register_NC,
  register_PO,
  register_PE,
  register_P,
  register_M
} register_type_t;

/**************************************************************************************/
// Symbol Types
/**************************************************************************************/
typedef struct
{
  bool isResolved;
  char *symbol;
  uint16_t value;
} symbol_t;

/**************************************************************************************/
// Operand Types
/**************************************************************************************/
typedef enum
{
  operand_NA = 0,       // no operand present
  operand_invalid,      // Invalid operand (e.g. (A))
  operand_r,            // Register A, B, C, D, E, E, H, L
  operand_I,            // Register I
  operand_R,            // Register R
  operand_rr,           // Register Pairs BC, DE, IX, IY, HL or SP
  operand_deref_HL,     // Often used as separate operand
  operand_deref_rr,     // (BC),(DE),(SP)
  operand_deref_IX_IY,  // (IX), (IY)
  operand_deref_idx,    // (IX+d),(IY+d)
  operand_deref_C,      // (C)
  operand_n,            // 1 byte unsigned integer 0 - 255
  operand_nn,           // 2 byte unsigned integer 0 - 65535
  operand_deref_n,      // (n)
  operand_deref_nn,     // (nn)
  operand_b,            // one bit expression in the range 0 - 7 (7 = MSB, 0 = LSB)
  operand_e,            // one byte signed integer -126 - + 129 for relative jump offsets
  operand_cc,           // Status in the Flag register (NZ, Z, NC, C, PO, PE, P, M)
  operand_symbol,       // Placeholder for a value represented by a symbol
  operand_deref_symbol, // Placeholder for dereferencing a symbol
  operand_string,       // String literal "string_literal"
  operand_end = 0xFFFF  // Used as sentinel for vargs of operand types
} operand_type_t;

/**
 * @brief Struct representing a dereferencing indexed register operand (IX+d) or
 * (IY+d)
 *
 */
typedef struct
{
  register_type_t index_register;
  int8_t index;
} deref_idx;

/**
 * @brief Union of the data representing the different operands
 *
 */
typedef union
{
  register_type_t r;
  register_type_t rr;
  deref_idx dereference_idx;
  uint8_t immediate_n;
  uint16_t immediate_nn;
  uint8_t dereference_n;
  uint16_t dereference_nn;
  uint8_t bit_expression_b;
  int16_t relative_offset_e;
  register_type_t status_flag;
  symbol_t symbol;
  char *string_literal;
} operand_data_t;

typedef struct
{
  operand_type_t type;
  operand_data_t data;
} operand_t;

/**************************************************************************************/
// Directive Types
/**************************************************************************************/
typedef enum
{
  directive_ORG,
  directive_EXPORT,
  directive_IMPORT,
  directive_SECTION,
  directive_DB,
  directive_DW,
  directive_DS,
  directive_EQU
} directive_types_t;

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

/**************************************************************************************/
// Statement Types
/**************************************************************************************/
/**
 * @brief Enum listing the different types of statements. Used to simplify
 * processing
 *
 */
typedef enum
{
  statement_undefined,
  statement_label,
  statement_directive,
  statement_instruction
} statement_types_t;

/**
 * @brief Struct representing an instruction, i.e. opcode, operands, machine
 * code and relocation information
 *
 */
typedef struct
{
  encoding_t encoding;
  uint8_t machinecode[OPCODE_MAX_LENGTH];
  opcode_t opcode;
  operand_t operand1;
  operand_t operand2;

  bool relocatable;
  uint8_t relocate_offset1;
  uint8_t relocate_offset2;

  size_t lineNumber;
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
