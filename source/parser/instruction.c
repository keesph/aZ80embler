#include "parser/instruction.h"

#include "lexer/token.h"
#include "parser/instruction_encoding.h"
#include "parser/operand.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"

#include "logging/logging.h"
#include "types.h"

#include <assert.h>
#include <stdarg.h>

/**************************************************************************************************/
// Defines and Types
/**************************************************************************************************/
// Convenience syntax error loging macros
#define LOG_INVALID_COMBINATION(instruction)                                                                           \
  LOG_SYNTAX_ERROR(instruction, "Invalid combination of operands [%s, %s]",                                            \
                   operand_toString(instruction->operand1.type), operand_toString(instruction->operand2.type))

#define LOG_INVALID_OPERAND1(instruction)                                                                              \
  LOG_SYNTAX_ERROR(instruction, "Operand 1 [%s] is invalid", operand_toString(instruction->operand1.type))

#define LOG_INVALID_OPERAND2(instruction)                                                                              \
  LOG_SYNTAX_ERROR(instruction, "Operand 2 [%s] is invalid", operand_toString(instruction->operand2.type))

/**************************************************************************************************/
// Static Function Declarations
/**************************************************************************************************/
static bool determine_encoding_LD(instruction_t *instruction);
static bool determine_encoding_PUSH_POP(instruction_t *instruction);
static bool determine_encoding_EX_EXX(instruction_t *instruction);
static bool determine_encoding_LDI_LDIR(instruction_t *instruction);
static bool determine_encoding_LDD_LDDR(instruction_t *instruction);
static bool determine_encoding_CPI_CPIR(instruction_t *instruction);
static bool determine_encoding_CPD_CPDR(instruction_t *instruction);
static bool determine_encoding_ADD_ADDC(instruction_t *instruction); // 8 bit and 16 bit
static bool determine_encoding_SUB_SBC(instruction_t *instruction);
static bool determine_encoding_LOGIC(instruction_t *instruction); // AND, OR, XOR, CP
static bool determine_encoding_INC_DEC(instruction_t *instruction);
static bool determine_encoding_cpu_control(instruction_t *instruction);
static bool determine_encoding_rotate_shift(instruction_t *instruction);
static bool determine_encoding_BIT_SET_RES(instruction_t *instruction);
static bool determine_encoding_jump_group(instruction_t *instruction);
static bool determine_encoding_call_return_group(instruction_t *instruction);
static bool determine_encoding_io_group(instruction_t *instruction);

static bool operand_is_r(operand_t *operand);
static bool operand_is_dd(operand_t *operand);
static bool operand_is_qq(operand_t *operand);
static bool operand_is_cc(operand_t *operand);
static bool operand_is_ss(operand_t *operand);
static bool operand_is_s(operand_t *operant);
static bool operand_is_m(operand_t *operand);
static bool operand_is_pp(operand_t *operand);
static bool operand_is_rr(operand_t *operand); // rr in the standard means something else than operator_rr!
static bool operand_is_valid_e(operand_t *operand);
static bool operand_is_valid_nn(operand_t *operand);
static bool operand_is_valid_deref_nn(operand_t *operand);

static bool expect_operand1(instruction_t *instruction, operand_type_t type);
static bool expect_operand2(instruction_t *instruction, operand_type_t type);
static bool expect_operands(instruction_t *instruction, operand_type_t type1, operand_type_t type2);

/**************************************************************************************************/
// Public function definitions
/**************************************************************************************************/
void instruction_free_callback(void *instructionToFree)
{
  instruction_t *instruction = (instruction_t *)instructionToFree;

  switch (instruction->operand1.type)
  {
  case operand_symbol:
  case operand_deref_symbol:
    free(instruction->operand1.data.symbol.symbol);
    break;

  case operand_string:
    free(instruction->operand1.data.string_literal);
    break;

  default:
    // Nothing to do
    break;
  }

  switch (instruction->operand2.type)
  {
  case operand_symbol:
  case operand_deref_symbol:
    free(instruction->operand2.data.symbol.symbol);
    break;

  case operand_string:
    free(instruction->operand2.data.string_literal);
    break;

  default:
    // Nothing to do
    break;
  }
}

bool instruction_parse(parser_t *parser)
{
  bool result = false;

  if (!expect_token(parser, token_opcode))
  {
    return LOG_SYNTAX_ERROR(parser, "Instruction must start with opcode!");
  }

  instruction_t *instruction = &parser->currentStatement.instruction;
  instruction->opcode = get_token(parser)->data.opcodeType;
  consume_token(parser);

  // Parse possible Operand 1
  operand_t operand;
  if (!expect_token(parser, token_eol) && !expect_token(parser, token_eof))
  {
    operand = operand_parse(parser);
    if (operand.type == operand_invalid)
    {
      return LOG_SYNTAX_ERROR(parser, "Error when parsing operand 1");
    }
    instruction->operand1 = operand;
  }

  // Parse second operand, if one is present
  if (expect_token(parser, token_comma))
  {
    consume_token(parser);
    if (!expect_token(parser, token_eol) && !expect_token(parser, token_eof))
    {
      operand = operand_parse(parser);
      if (operand.type == operand_invalid)
      {
        return LOG_SYNTAX_ERROR(parser, "Error when parsing operand 2");
      }
      instruction->operand2 = operand;
    }
  }

  if (!expect_token(parser, token_eol) && !expect_token(parser, token_eof))
  {
    return LOG_SYNTAX_ERROR(parser, "Expected EOF or EOL after operand!");
  }

  instruction->lineNumber = parser->lineNumber;

  switch (instruction->opcode)
  {
  case opcode_LD:
    result = determine_encoding_LD(instruction);
    break;

  case opcode_PUSH:
  case opcode_POP:
    result = determine_encoding_PUSH_POP(instruction);
    break;

  case opcode_EX:
  case opcode_EXX:
    result = determine_encoding_EX_EXX(instruction);
    break;

  case opcode_LDI:
  case opcode_LDIR:
    result = determine_encoding_LDI_LDIR(instruction);
    break;

  case opcode_LDD:
  case opcode_LDDR:
    result = determine_encoding_LDD_LDDR(instruction);
    break;

  case opcode_CPI:
  case opcode_CPIR:
    result = determine_encoding_CPI_CPIR(instruction);
    break;

  case opcode_CPD:
  case opcode_CPDR:
    result = determine_encoding_CPD_CPDR(instruction);
    break;

  case opcode_ADD:
  case opcode_ADC:
    result = determine_encoding_ADD_ADDC(instruction); // 8 bit and 16 bit
    break;

  case opcode_SUB:
  case opcode_SBC:
    result = determine_encoding_SUB_SBC(instruction); // 8 bit and 16 bit
    break;

  case opcode_AND:
  case opcode_OR:
  case opcode_XOR:
  case opcode_CP:
    result = determine_encoding_LOGIC(instruction);
    break;

  case opcode_INC:
  case opcode_DEC:
    result = determine_encoding_INC_DEC(instruction); // 8 bit and 16 bit
    break;

  case opcode_DAA:
  case opcode_CPL:
  case opcode_NEG:
  case opcode_CCF:
  case opcode_SCF:
  case opcode_NOP:
  case opcode_HALT:
  case opcode_DI:
  case opcode_EI:
  case opcode_IM:
    result = determine_encoding_cpu_control(instruction);
    break;

  case opcode_RLCA:
  case opcode_RLA:
  case opcode_RRCA:
  case opcode_RRA:
  case opcode_RLC:
  case opcode_RL:
  case opcode_RRC:
  case opcode_RR:
  case opcode_SLA:
  case opcode_SRA:
  case opcode_SRL:
  case opcode_RLD:
  case opcode_RRD:
    result = determine_encoding_rotate_shift(instruction);
    break;

  case opcode_BIT:
  case opcode_SET:
  case opcode_RES:
    result = determine_encoding_BIT_SET_RES(instruction);
    break;

  case opcode_JP:
  case opcode_JR:
  case opcode_DJNZ:
    result = determine_encoding_jump_group(instruction);
    break;

  case opcode_CALL:
  case opcode_RET:
  case opcode_RETI:
  case opcode_RETN:
  case opcode_RST:
    result = determine_encoding_call_return_group(instruction);
    break;

  case opcode_IN:
  case opcode_INI:
  case opcode_INIR:
  case opcode_IND:
  case opcode_INDR:
  case opcode_OUT:
  case opcode_OUTI:
  case opcode_OTIR:
  case opcode_OUTD:
  case opcode_OTDR:
    result = determine_encoding_io_group(instruction);
    break;
  default:
    LOG_ERROR("Got UNDEFINED opcode in parser!");
    return false;
  }

  parser->currentStatement.type = statement_instruction;
  emit_statement(parser);
  return result;
}

/**************************************************************************************************/
// Static function definitions
/**************************************************************************************************/
static bool determine_encoding_LD(instruction_t *instruction)
{
  if (expect_operands(instruction, operand_NA, operand_NA))
  {
    return LOG_SYNTAX_ERROR(instruction, "LD requires 2 operands!");
  }
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (operand1.type)
  {
  case operand_r:
    // [r, r]
    if (expect_operand2(instruction, operand_r))
    {
      instruction->encoding = encoding_LD_r_r;
    }
    // [r, n]
    else if (expect_operand2(instruction, operand_n))
    {
      instruction->encoding = encoding_LD_r_n;
    }
    // [r, (HL))]
    else if (expect_operand2(instruction, operand_deref_HL))
    {
      instruction->encoding = encoding_LD_r_derefHL;
    }
    // [r, (IX+d)]
    else if (expect_operand2(instruction, operand_deref_idx) &&
             operand2.data.dereference_idx.index_register == register_IX)
    {
      instruction->encoding = encoding_LD_r_derefIXd;
    }
    // [r, (IY+d)]
    else if (expect_operand2(instruction, operand_deref_idx) &&
             operand2.data.dereference_idx.index_register == register_IY)
    {
      instruction->encoding = encoding_LD_r_derefIYd;
    }
    else if (operand1.data.r == register_A)
    {
      // [A, (BC)]
      if (expect_operand2(instruction, operand_deref_rr) && operand2.data.rr == register_BC)
      {
        instruction->encoding = encoding_LD_A_derefBC;
      }
      // [A, (DE)]
      else if (expect_operand2(instruction, operand_deref_rr) && operand2.data.rr == register_BC)
      {
        instruction->encoding = encoding_LD_A_derefDE;
      }
      // [A, (nn)]
      else if (operand_is_valid_deref_nn(&operand2))
      {
        instruction->encoding = encoding_LD_A_derefnn;
      }
      // [A, I]
      else if (expect_operand2(instruction, operand_I))
      {
        instruction->encoding = encoding_LD_A_I;
      }
      // [A, R]
      else if (expect_operand2(instruction, operand_R))
      {
        instruction->encoding = encoding_LD_A_R;
      }
      else
      {
        return LOG_INVALID_COMBINATION(instruction);
      }
    }
    else
    {
      // DOH!
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case operand_deref_HL:
    // [(HL), r]
    if (expect_operand2(instruction, operand_r))
    {
      instruction->encoding = encoding_LD_derefHL_r;
    }
    // [(HL), n]
    else if (expect_operand2(instruction, operand_n))
    {
      instruction->encoding = encoding_LD_derefHL_n;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case operand_deref_rr:
    if (expect_operand2(instruction, operand_r) && operand2.data.r == register_A)
    {
      // [(BC), A]
      if (operand1.data.rr == register_BC)
      {
        instruction->encoding = encoding_LD_derefBC_A;
      }
      // [(DE), A]
      else if (operand1.data.rr == register_DE)
      {
        instruction->encoding = encoding_LD_derefDE_A;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else if (operand1.data.rr == register_HL)
    {
      // [(HL), r]
      if (expect_operand2(instruction, operand_r))
      {
        instruction->encoding = encoding_LD_derefHL_r;
      }
      // [(HL), n]
      else if (expect_operand2(instruction, operand_n))
      {
        instruction->encoding = encoding_LD_derefHL_n;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case operand_deref_idx:
    if (expect_operand2(instruction, operand_n))
    {
      // [(IX+d), n]
      if (operand1.data.dereference_idx.index_register == register_IX)
      {
        instruction->encoding = encoding_LD_derefIXd_n;
      }
      // [(IY+d), n]
      else
      {
        instruction->encoding = encoding_LD_derefIYd_n;
      }
    }
    else if (expect_operand2(instruction, operand_r))
    {
      // [(IX+d), r]
      if (operand1.data.dereference_idx.index_register == register_IX)
      {
        instruction->encoding = encoding_LD_derefIXd_r;
      }
      // [(IY+d), r]
      else if (operand1.data.dereference_idx.index_register == register_IY)
      {
        instruction->encoding = encoding_LD_derefIYd_r;
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case operand_rr:
    if (operand_is_dd(&operand1))
    {
      // [dd, (nn)]
      if (operand_is_valid_deref_nn(&operand2))
      {
        instruction->encoding = encoding_LD_dd_derefnn;
      }
      // [dd, nn]
      else if (operand_is_valid_nn(&operand2))
      {
        instruction->encoding = encoding_LD_dd_nn;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else if (operand1.data.rr == register_IX)
    {
      // [IX, (nn)]
      if (operand_is_valid_deref_nn(&operand2))
      {
        instruction->encoding = encoding_LD_IX_derefnn;
      }
      // [IX, nn]
      else if (operand_is_valid_nn(&operand2))
      {
        instruction->encoding = encoding_LD_IX_nn;
      }
      else
      {
        LOG_INVALID_OPERAND2(instruction);
      }
    }
    else if (operand1.data.rr == register_IY)
    {
      // [IY, (nn)]
      if (operand_is_valid_deref_nn(&operand2))
      {
        instruction->encoding = encoding_LD_IY_derefnn;
      }
      // [IY, nn]
      else if (operand_is_valid_nn(&operand2))
      {
        instruction->encoding = encoding_LD_IY_nn;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else if (operand1.data.rr == register_SP && expect_operand2(instruction, operand_rr))
    {
      // [SP, IX]
      if (operand2.data.rr == register_IX)
      {
        instruction->encoding = encoding_LD_SP_IX;
      }
      // [SP, IY]
      else if (operand2.data.rr == register_IY)
      {
        instruction->encoding = encoding_LD_SP_IY;
      }
      // [SP, HL]
      else if (operand2.data.rr == register_HL)
      {
        instruction->encoding = encoding_LD_SP_HL;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case operand_R:
    // [R, A]
    if (expect_operand2(instruction, operand_r) && operand2.data.r == register_A)
    {
      instruction->encoding = encoding_LD_R_A;
    }
    else
    {
      return LOG_INVALID_OPERAND2(instruction);
    }
    break;
  case operand_I:
    // [I, A]
    if (expect_operand2(instruction, operand_r) && operand2.data.r == register_A)
    {
      instruction->encoding = encoding_LD_I_A;
    }
    else
    {
      return LOG_INVALID_OPERAND2(instruction);
    }
    break;

  case operand_deref_n:
  case operand_deref_nn:
  case operand_deref_symbol:
    // [(nn), A]
    if (expect_operand2(instruction, operand_r) && operand2.data.r == register_A)
    {
      instruction->encoding = encoding_LD_derefnn_A;
    }
    // [(nn), IX/IY/dd]
    else if (expect_operand2(instruction, operand_rr))
    {
      if (operand_is_dd(&operand2))
      {
        instruction->encoding = encoding_LD_derefnn_dd;
      }
      else if (operand2.data.rr == register_IX)
      {
        instruction->encoding = encoding_LD_derefnn_IX;
      }
      else if (operand2.data.rr == register_IY)
      {
        instruction->encoding = encoding_LD_derefnn_IY;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;
  default:
    return LOG_INVALID_OPERAND1(instruction);
    break;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_PUSH_POP(instruction_t *instruction)
{
  if (!expect_operand2(instruction, operand_NA))
  {
    return LOG_INVALID_OPERAND2(instruction);
  }

  operand_t operand1 = instruction->operand1;
  if (expect_operand1(instruction, operand_rr) &&
      (operand_is_qq(&operand1) || operand1.data.rr == register_IX || operand1.data.rr == register_IY))
  {
    switch (instruction->opcode)
    {
    case opcode_PUSH:
      if (operand_is_qq(&operand1))
      {
        instruction->encoding = encoding_PUSH_qq;
      }
      else if (operand1.data.rr == register_IX)
      {
        instruction->encoding = encoding_PUSH_IX;
      }
      else if (operand1.data.rr == register_IY)
      {
        instruction->encoding = encoding_PUSH_IY;
      }
      else
      {
        return LOG_INVALID_OPERAND1(instruction);
      }
      break;

    case opcode_POP:
      if (operand_is_qq(&operand1))
      {
        instruction->encoding = encoding_POP_qq;
      }
      else if (operand1.data.rr == register_IX)
      {
        instruction->encoding = encoding_POP_IX;
      }
      else if (operand1.data.rr == register_IY)
      {
        instruction->encoding = encoding_POP_IY;
      }
      else
      {
        return LOG_INVALID_OPERAND1(instruction);
      }
      break;
    default:
      assert(false); // Should not happen
      break;
    }
  }
  else
  {
    return LOG_INVALID_OPERAND1(instruction);
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_EX_EXX(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (instruction->opcode == opcode_EX)
  {
    if (expect_operand2(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND2(instruction);
    }

    // [DE, HL]
    if ((expect_operand1(instruction, operand_rr) && operand1.data.rr == register_DE) &&
        (expect_operand2(instruction, operand_rr) && operand2.data.rr == register_HL))
    {
      instruction->encoding = encoding_EX_DE_HL;
    }
    // [AF, AF]
    else if ((expect_operand1(instruction, operand_rr) && operand1.data.rr == register_AF) &&
             (expect_operand2(instruction, operand_rr) && operand2.data.rr == register_AF))
    {
      instruction->encoding = encoding_EX_AF_AF;
    }
    else if ((expect_operand1(instruction, operand_deref_rr) && operand1.data.rr == register_SP) &&
             (expect_operand2(instruction, operand_rr)))
    {
      // [SP, HL]
      if (operand2.data.rr == register_HL)
      {
        instruction->encoding = encoding_EX_derefSP_HL;
      }
      // [SP, IX]
      else if (operand2.data.rr == register_IX)
      {
        instruction->encoding = encoding_EX_derefSP_IX;
      }
      // [SP, IY]
      else if (operand2.data.rr == register_IY)
      {
        instruction->encoding = encoding_EX_derefSP_IY;
      }
      else
      {
        return LOG_INVALID_OPERAND2(instruction);
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  }
  else
  {
    // []
    if (expect_operands(instruction, operand_invalid, operand_invalid))
    {
      instruction->encoding = encoding_EXX;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_LDI_LDIR(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  result = operand1.type == operand_NA && operand2.type == operand_NA;
  if (!result)
  {
    LOG_SYNTAX_ERROR(instruction, "LDI/LDIR does not have operands!");
  }

  // Set instruction encoding
  if (instruction->opcode == opcode_LDI)
    instruction->encoding = encoding_LDI;
  else
    instruction->encoding = encoding_LDIR;

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_LDD_LDDR(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  result = operand1.type == operand_NA && operand2.type == operand_NA;
  if (!result)
  {
    LOG_SYNTAX_ERROR(instruction, "LDI/LDIR does not have operands!");
  }

  // Set instruction encoding
  if (instruction->opcode == opcode_LDD)
    instruction->encoding = encoding_LDD;
  else
    instruction->encoding = encoding_LDDR;

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_CPI_CPIR(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  result = operand1.type == operand_NA && operand2.type == operand_NA;
  if (!result)
  {
    LOG_SYNTAX_ERROR(instruction, "LDI/LDIR does not have operands!");
  }

  // Set instruction encoding
  if (instruction->opcode == opcode_CPI)
    instruction->encoding = encoding_CPI;
  else
    instruction->encoding = encoding_CPIR;

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_CPD_CPDR(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  result = operand1.type == operand_NA && operand2.type == operand_NA;
  if (!result)
  {
    LOG_SYNTAX_ERROR(instruction, "LDI/LDIR does not have operands!");
  }

  // Set instruction encoding
  if (instruction->opcode == opcode_CPD)
    instruction->encoding = encoding_CPD;
  else
    instruction->encoding = encoding_CPDR;

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_ADD_ADDC(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (operand1.type == operand_r && operand1.data.r == register_A)
  {
    switch (instruction->opcode)
    {
    case opcode_ADD:
      if (operand_is_r(&operand2))
      {
        instruction->encoding = encoding_ADD_A_r;
        result = true;
      }
      else if (operand2.type == operand_n)
      {
        instruction->encoding = encoding_ADD_A_n;
        result = true;
      }
      else if (operand2.type == operand_deref_HL)
      {
        instruction->encoding = encoding_ADD_A_derefHL;
        result = true;
      }
      else if (operand2.type == operand_deref_idx)
      {
        if (operand2.data.dereference_idx.index_register == register_IX)
          instruction->encoding = encoding_ADD_A_derefIXd;
        else
          instruction->encoding = encoding_ADD_A_derefIYd;

        result = true;
      }
      else
      {
        LOG_INVALID_COMBINATION(instruction);
      }
      break;

    case opcode_ADC:
      if (operand_is_s(&operand2))
      {
        instruction->encoding = encoding_ADC_A_s;
        result = true;
      }
      else
      {
        LOG_INVALID_COMBINATION(instruction);
      }
      break;
    default:
      assert(false); // Should not happen
      break;
    }
  }
  // [HL, ss]
  else if (operand1.type == operand_rr && (operand1.data.rr == register_HL))
  {
    if (operand_is_ss(&operand2))
    {
      if (instruction->opcode == opcode_ADD)
        instruction->encoding = encoding_ADD_HL_ss;
      else
        instruction->encoding = encoding_ADC_HL_ss;

      result = true;
    }
    else
    {
      LOG_INVALID_OPERAND2(instruction);
      result = false;
    }
  }
  // [IX, pp], [IY, rr] Only valid for ADD
  else if (instruction->opcode == opcode_ADD && operand1.type == operand_rr)
  {
    if (operand1.data.rr == register_IX && operand_is_pp(&operand2))
    {
      instruction->encoding = encoding_ADD_IX_pp;
      result = true;
    }
    else if (operand1.data.rr == register_IY && operand_is_rr(&operand2))
    {
      instruction->encoding = encoding_ADD_IY_rr;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
      result = false;
    }
  }
  else
  {
    LOG_INVALID_COMBINATION(instruction);
    result = false;
  }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_SUB_SBC(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (instruction->opcode == opcode_SUB && operand2.type == operand_NA && operand_is_s(&operand1))
  {
    instruction->encoding = encoding_SUB_s;
    result = true;
  }
  else if (instruction->opcode == opcode_SBC && operand1.type == operand_r && operand1.data.r == register_A &&
           operand_is_s(&operand2))
  {
    instruction->encoding = encoding_SBC_A_s;
    result = true;
  }
  // [HL, ss]
  else if (instruction->opcode == opcode_SBC && operand1.type == operand_rr && operand1.data.rr == register_HL &&
           operand_is_ss(&operand2))
  {
    instruction->encoding = encoding_SBC_HL_ss;
    result = true;
  }
  else
  {
    LOG_INVALID_COMBINATION(instruction);
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_LOGIC(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (operand2.type != operand_NA)
  {
    LOG_INVALID_OPERAND2(instruction);
    return false;
  }
  if (!operand_is_s(&operand1))
  {
    switch (instruction->opcode)
    {
    // [s, NA]
    case opcode_AND:
      instruction->encoding = encoding_AND_s;
      break;
    // [s, NA]
    case opcode_OR:
      instruction->encoding = encoding_OR_s;
      break;
    // [s, NA]
    case opcode_XOR:
      instruction->encoding = encoding_XOR_s;
      break;
    // [s, NA]
    case opcode_CP:
      instruction->encoding = encoding_CP_s;
      break;
    default:
      assert(false); // Sould not happen
      break;
    }
    LOG_INVALID_OPERAND1(instruction);
    return false;
  }
  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_INC_DEC(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;

  if (!expect_operand2(instruction, operand_NA))
  {
    return LOG_INVALID_OPERAND2(instruction);
  }

  switch (instruction->opcode)
  {
  case opcode_INC:
    // [m, NA]
    if (operand_is_m(&operand1))
    {
      instruction->encoding = encoding_INC_m;
    }
    // [ss, NA]
    else if (expect_operand1(instruction, operand_rr) && operand_is_ss(&operand1))
    {
      instruction->encoding = encoding_INC_ss;
    }
    // [IX, NA]
    else if (expect_operand1(instruction, operand_rr) && operand1.data.rr == register_IX)
    {
      instruction->encoding = encoding_INC_IX;
    }
    // [IY, NA]
    else if (expect_operand1(instruction, operand_rr) && operand1.data.rr == register_IY)
    {
      instruction->encoding = encoding_INC_IY;
    }
    else
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    break;

  case opcode_DEC:
    // [m, NA]
    if (operand_is_m(&operand1))
    {
      instruction->encoding = encoding_DEC_m;
    }
    // [ss, NA]
    else if (expect_operand1(instruction, operand_rr) && operand_is_ss(&operand1))
    {
      instruction->encoding = encoding_DEC_ss;
    }
    // [IX, NA]
    else if (expect_operand1(instruction, operand_rr) && operand1.data.rr == register_IX)
    {
      instruction->encoding = encoding_DEC_IX;
    }
    // [IY, NA]
    else if (expect_operand1(instruction, operand_rr) && operand1.data.rr == register_IY)
    {
      instruction->encoding = encoding_DEC_IY;
    }
    else
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    break;

  default:
    assert(false); // Should not happen
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_cpu_control(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;

  if (!expect_operand2(instruction, operand_NA))
  {
    return LOG_INVALID_OPERAND1(instruction);
  }

  switch (instruction->opcode)
  {
  case opcode_DAA:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_DAA;
    break;

  case opcode_CPL:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_CPL;
    break;

  case opcode_NEG:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_NEG;
    break;

  case opcode_CCF:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_CCF;
    break;

  case opcode_SCF:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_SCF;
    break;

  case opcode_NOP:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_NOP;
    break;

  case opcode_HALT:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_HALT;
    break;

  case opcode_DI:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_DI;
    break;

  case opcode_EI:
    if (!expect_operand1(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    instruction->encoding = encoding_EI;
    break;

  case opcode_IM:
    if (!expect_operand1(instruction, operand_n))
    {
      return LOG_INVALID_OPERAND1(instruction);
    }

    if (operand1.data.immediate_n > 2)
    {
      return LOG_SYNTAX_ERROR(instruction, "Operand to IM is out of range: %d. Must be in range 0 - 2",
                              operand1.data.immediate_n);
    }

    instruction->encoding = encoding_IM;
    break;

  default:
    assert(false); // Should not happen
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_rotate_shift(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;

  switch (instruction->opcode)
  {
  case opcode_RLCA:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RLCA;
    break;

  case opcode_RLA:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RLA;
    break;

  case opcode_RRCA:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RRCA;
    break;

  case opcode_RRA:
    // [NA, NA]
    instruction->encoding = encoding_RRA;
    break;

  case opcode_RLD:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RLD;
    break;

  case opcode_RRD:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RRD;
    break;

  case opcode_RLC:
    if (!expect_operand2(instruction, operand_NA))
    {
      return LOG_INVALID_OPERAND2(instruction);
    }
    // [r, NA]
    if (expect_operand1(instruction, operand_r))
    {
      instruction->encoding = encoding_RLC_r;
    }
    // [(HL), NA]
    else if (expect_operand1(instruction, operand_deref_HL))
    {
      instruction->encoding = encoding_RLC_derefHL;
    }
    // [(IX+d), NA]
    else if (expect_operand1(instruction, operand_deref_idx) &&
             operand1.data.dereference_idx.index_register == register_IX)
    {
      instruction->encoding = encoding_RLC_derefIXd;
    }
    // [(IY+d), NA]
    else if (expect_operand1(instruction, operand_deref_idx) &&
             operand1.data.dereference_idx.index_register == register_IY)
    {
      instruction->encoding = encoding_RLC_derefIYd;
    }
    else
    {
      return LOG_INVALID_OPERAND1(instruction);
    }
    break;

  // Opcodes with operand type m
  case opcode_RL:
    // [m, NA]
    if (!expect_operand2(instruction, operand_NA) || !operand_is_m(&operand1))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RL_m;
    break;

  case opcode_RRC:
    // [m, NA]
    if (!expect_operand2(instruction, operand_NA) || !operand_is_m(&operand1))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RRC_m;
    break;

  case opcode_RR:
    // [m, NA]
    if (!expect_operand2(instruction, operand_NA) || !operand_is_m(&operand1))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_RR_m;
    break;

  case opcode_SLA:
    // [m, NA]
    if (!expect_operand2(instruction, operand_NA) || !operand_is_m(&operand1))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_SLA_m;
    break;

  case opcode_SRA:
    // [m, NA]
    if (!expect_operand2(instruction, operand_NA) || !operand_is_m(&operand1))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_SRA_m;
    break;

  case opcode_SRL:
    // [m, NA]
    if (!expect_operand2(instruction, operand_NA) || !operand_is_m(&operand1))
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    instruction->encoding = encoding_SRL_m;
    break;

  default:
    assert(false); // Should not happen
    break;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_BIT_SET_RES(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (!expect_operand1(instruction, operand_NA) || !expect_operand2(instruction, operand_NA))
  {
    return LOG_SYNTAX_ERROR(instruction, "operation requires two operands!");
  }

  if (!(expect_operand1(instruction, operand_n) && operand1.data.immediate_n <= 7))
  {
    return LOG_SYNTAX_ERROR(instruction, "Operand 1 must be unsigned integer in range of 0 - 7!");
  }

  switch (instruction->opcode)
  {
  case opcode_BIT:
    // [b, r]
    if (expect_operand2(instruction, operand_r))
    {
      instruction->encoding = encoding_BIT_b_r;
    }
    // [b, (HL)]
    else if (expect_operand2(instruction, operand_deref_HL))
    {
      instruction->encoding = encoding_BIT_b_derefHL;
    }
    // [b, (IX+-d)]
    else if (expect_operand2(instruction, operand_deref_idx) &&
             operand2.data.dereference_idx.index_register == register_IX)
    {
      instruction->encoding = encoding_BIT_b_derefIXd;
    }
    // [b, (IY+-d)]
    else if (expect_operand2(instruction, operand_deref_idx) &&
             operand2.data.dereference_idx.index_register == register_IY)
    {
      instruction->encoding = encoding_BIT_b_derefIYd;
    }
    else
    {
      return LOG_INVALID_OPERAND2(instruction);
    }
    break;

  case opcode_SET:
    // [b, r]
    if (expect_operand2(instruction, operand_r))
    {
      instruction->encoding = encoding_SET_b_r;
    }
    // [b, (HL)]
    else if (expect_operand2(instruction, operand_deref_HL))
    {
      instruction->encoding = encoding_SET_b_derefHL;
    }
    // [b, (IX+-d)]
    else if (expect_operand2(instruction, operand_deref_idx) &&
             operand2.data.dereference_idx.index_register == register_IX)
    {
      instruction->encoding = encoding_SET_b_derefIXd;
    }
    // [b, (IY+-d)]
    else if (expect_operand2(instruction, operand_deref_idx) &&
             operand2.data.dereference_idx.index_register == register_IY)
    {
      instruction->encoding = encoding_SET_b_derefIYd;
    }
    else
    {
      return LOG_INVALID_OPERAND2(instruction);
    }
    break;

  case opcode_RES:
    // [b, m]
    if (operand_is_m(&operand2))
    {
      instruction->encoding = encoding_RES_b_m;
    }
    else
    {
      return LOG_INVALID_OPERAND2(instruction);
    }
    break;

  default:
    assert(false); // Should not happen
    break;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_jump_group(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (instruction->opcode)
  {
  case opcode_JP:
    // [nn, NA]
    if (operand_is_valid_nn(&operand1) && expect_operand2(instruction, operand_NA))
    {
      instruction->encoding = encoding_JP_nn;
    }
    // [(IX), NA]
    else if (expect_operand1(instruction, operand_deref_IX_IY) && expect_operand2(instruction, operand_NA) &&
             operand1.data.rr == register_IX)
    {
      instruction->encoding = encoding_JP_derefIX;
    }
    // [(IY), NA]
    else if (expect_operand1(instruction, operand_deref_IX_IY) && expect_operand2(instruction, operand_NA) &&
             operand1.data.rr == register_IY)
    {
      instruction->encoding = encoding_JP_derefIY;
    }
    // [(HL), NA]
    else if (expect_operand1(instruction, operand_deref_HL) && expect_operand2(instruction, operand_NA))
    {
      instruction->encoding = encoding_JP_derefHL;
    }
    // [(cc), nn]
    else if (operand_is_cc(&operand1) && operand_is_valid_nn(&operand2))
    {
      instruction->encoding = encoding_JP_cc_nn;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_JR:
    // [e, NA]
    if (operand_is_valid_e(&operand1) && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_JR_e;
    }
    else if (expect_operand1(instruction, operand_r) && operand_is_valid_e(&operand2))
    {
      // [C, e]
      if (operand1.data.r == register_C)
      {
        instruction->encoding = encoding_JR_C_e;
      }
      // [NC, e]
      else if (operand1.data.r == register_NC)
      {
        instruction->encoding = encoding_JR_NC_e;
      }
      // [Z, e]
      else if (operand1.data.r == register_Z)
      {
        instruction->encoding = encoding_JR_Z_e;
      }
      // [NZ, e]
      else if (operand1.data.r == register_NZ)
      {
        instruction->encoding = encoding_JR_NZ_e;
      }
      else
      {
        return LOG_INVALID_OPERAND1(instruction);
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_DJNZ:
    if (operand_is_valid_e(&operand1) && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_DJNZ_e;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  default:
    assert(false); // should never happen
    break;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_call_return_group(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (instruction->opcode)
  {
  case opcode_CALL:
    // [nn, NA]
    if (operand_is_valid_nn(&operand1) && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_CALL_nn;
    }
    // [cc, nn]
    else if (operand_is_cc(&operand1) && operand_is_valid_nn(&operand2))
    {
      instruction->encoding = encoding_CALL_cc_nn;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_RET:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_RET;
    }
    // [cc, NA]
    else if (operand_is_cc(&operand1) && expect_operand2(instruction, operand_NA))
    {
      instruction->encoding = encoding_RET_cc;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_RETI:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_RETI;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_RETN:
    // [NA, NA]
    if (operand1.type == operand_NA && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_RETN;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_RST:
    // [n, NA]
    if ((operand1.type == operand_n) && operand2.type == operand_NA)
    {
      uint8_t vector = operand1.data.immediate_n;
      if (vector == 0x00 || vector == 0x08 || vector == 0x10 || vector == 0x18 || vector == 0x20 || vector == 0x28 ||
          vector == 0x30 || vector == 0x38)
      {
        instruction->encoding = encoding_RST_p;
      }
      else
      {
        return LOG_SYNTAX_ERROR(instruction,
                                "Reset vector is not valid! Must be 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30 or 0x38");
      }
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  default:
    assert(false); // Should not happen
    break;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool determine_encoding_io_group(instruction_t *instruction)
{
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (instruction->opcode)
  {
  case opcode_INI:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_INI;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_INIR:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_INIR;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_IND:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_IND;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_INDR:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_INDR;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_OUTI:

    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_OUTI;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_OTIR:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_OTIR;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_OUTD:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_OUTD;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
  case opcode_OTDR:
    // [NA, NA]
    if (expect_operands(instruction, operand_NA, operand_NA))
    {
      instruction->encoding = encoding_OTDR;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_IN:
    // [A, (n)]
    if (expect_operands(instruction, operand_r, operand_deref_n) && operand1.data.r == register_A)
    {
      instruction->encoding = encoding_IN_A_derefn;
    }
    // [r, (C)]
    else if (operand_is_r(&operand1) && expect_operand2(instruction, operand_deref_C))
    {
      instruction->encoding = encoding_IN_r_derefC;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_OUT:
    // [(n), A]
    if (expect_operands(instruction, operand_deref_n, operand_r) && operand2.data.r == register_A)
    {
      instruction->encoding = encoding_OUT_derefn_A;
    }
    // [(C), r]
    else if (expect_operand1(instruction, operand_deref_C) && operand_is_r(&operand2))
    {
      instruction->encoding = encoding_OUT_derefC_r;
    }
    else
    {
      return LOG_INVALID_COMBINATION(instruction);
    }
    break;

  default:
    assert(false); // Should not happen
    break;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_r(operand_t *operand)
{
  // Check if the given type is part of the register group r
  if (operand->type != operand_r)
  {
    return false;
  }

  register_type_t type = operand->data.r;
  if (type == register_A || type == register_B || type == register_C || type == register_D || type == register_E ||
      type == register_H || type == register_L)
  {
    return true;
  }
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_dd(operand_t *operand) { return operand_is_ss(operand); }

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_qq(operand_t *operand)
{
  if (operand->type == operand_r)
  {
    return false;
  }
  register_type_t type = operand->data.r;
  return (type == register_BC || type == register_DE || type == register_HL || type == register_AF);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_cc(operand_t *operand)
{
  if (operand->type == operand_r)
  {
    return false;
  }
  register_type_t type = operand->data.r;
  return (type == register_NZ || type == register_Z || type == register_NC || type == register_C ||
          type == register_PO || type == register_PE || type == register_P || type == register_M);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_ss(operand_t *operand)
{
  if (operand->type != operand_rr)
  {
    return false;
  }
  register_type_t type = operand->data.rr;
  return (type == register_BC || type == register_DE || type == register_HL || type == register_SP);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_pp(operand_t *operand)
{
  if (operand->type != operand_rr)
  {
    return false;
  }

  register_type_t type = operand->data.rr;
  return (type == register_BC || type == register_DE || type == register_IX || type == register_SP);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_rr(operand_t *operand)
{
  if (operand->type != operand_rr)
  {
    return false;
  }

  register_type_t type = operand->data.rr;
  return (type == register_BC || type == register_DE || type == register_IY || type == register_SP);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_s(operand_t *operand)
{
  return (operand_is_r(operand) || (operand->type == operand_n) || (operand->type == operand_deref_HL) ||
          (operand->type == operand_deref_idx));
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_m(operand_t *operand)
{
  return (operand_is_r(operand) || (operand->type == operand_deref_HL) || (operand->type == operand_deref_idx));
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_valid_e(operand_t *operand)
{
  // range is from -126 to +129. Negative side is encoded as operand_e, positvie side is
  // encoded in operand_n since the parser is not aware of previous tokens ther is not way to
  // differentiate between positive e and regular n at parser level
  return ((operand->type == operand_n && operand->data.immediate_n < 130) || operand->type == operand_e);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_valid_nn(operand_t *operand)
{
  return ((operand->type == operand_n || operand->type == operand_nn) || operand->type == operand_symbol);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool operand_is_valid_deref_nn(operand_t *operand)
{
  return ((operand->type == operand_deref_n || operand->type == operand_deref_nn) ||
          operand->type == operand_deref_symbol);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool expect_operand1(instruction_t *instruction, operand_type_t type)
{
  return instruction->operand1.type == type;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool expect_operand2(instruction_t *instruction, operand_type_t type)
{
  return instruction->operand2.type == type;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool expect_operands(instruction_t *instruction, operand_type_t type1, operand_type_t type2)
{
  return (instruction->operand2.type == type1 && instruction->operand2.type == type2);
}
