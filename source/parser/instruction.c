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
#define EXPECT_OPERAND(instruction, which, ...) expect_operand(instruction, which, __VA_ARGS__, operand_end)

// Convenience syntax error loging macros
#define LOG_INVALID_COMBINATION(instruction)                                                                           \
  LOG_SYNTAX_ERROR(instruction, "Invalid combination of operands [%s, %s]",                                            \
                   operand_toString(instruction->operand1.type), operand_toString(instruction->operand2.type))

#define LOG_INVALID_OPERAND1(instruction)                                                                              \
  LOG_SYNTAX_ERROR(instruction, "Operand 1 [%s]", operand_toString(instruction->operand1.type))

#define LOG_INVALID_OPERAND2(instruction)                                                                              \
  LOG_SYNTAX_ERROR(instruction, "Operand 2 [%s]", operand_toString(instruction->operand2.type))

typedef enum
{
  op_1,
  op_2
} operand_number_t;

/**************************************************************************************************/
// Static Function Declarations
/**************************************************************************************************/
static bool check_syntax_LD(instruction_t *instruction);
static bool check_syntax_PUSH_POP(instruction_t *instruction);
static bool check_syntax_EX_EXX(instruction_t *instruction);
static bool check_syntax_LDI_LDIR(instruction_t *instruction);
static bool check_syntax_LDD_LDDR(instruction_t *instruction);
static bool check_syntax_CPI_CPIR(instruction_t *instruction);
static bool check_syntax_CPD_CPDR(instruction_t *instruction);
static bool check_syntax_ADD_ADDC(instruction_t *instruction); // 8 bit and 16 bit
static bool check_syntax_SUB_SBC(instruction_t *instruction);
static bool check_syntax_LOGIC(instruction_t *instruction); // AND, OR, XOR, CP
static bool check_syntax_INC_DEC(instruction_t *instruction);
static bool check_syntax_cpu_control(instruction_t *instruction);
static bool check_syntax_rotate_shift(instruction_t *instruction);
static bool check_syntax_BIT_SET_RES(instruction_t *instruction);
static bool check_syntax_jump_group(instruction_t *instruction);
static bool check_syntax_call_return_group(instruction_t *instruction);
static bool check_syntax_io_group(instruction_t *instruction);

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

static bool expect_operand(instruction_t *instruction, operand_number_t which, ...);
static bool expect_operand1(instruction_t *instruction, operand_type_t type);
static bool expect_operand2(instruction_t *instruction, operand_type_t type);
static bool expect_operands(instruction_t *instruction, operand_type_t type1, operand_type_t type2);

/**************************************************************************************************/
// Public function definitions
/**************************************************************************************************/
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
    result = check_syntax_LD(instruction);
    break;

  case opcode_PUSH:
  case opcode_POP:
    result = check_syntax_PUSH_POP(instruction);
    break;

  case opcode_EX:
  case opcode_EXX:
    result = check_syntax_EX_EXX(instruction);
    break;

  case opcode_LDI:
  case opcode_LDIR:
    result = check_syntax_LDI_LDIR(instruction);
    break;

  case opcode_LDD:
  case opcode_LDDR:
    result = check_syntax_LDD_LDDR(instruction);
    break;

  case opcode_CPI:
  case opcode_CPIR:
    result = check_syntax_CPI_CPIR(instruction);
    break;

  case opcode_CPD:
  case opcode_CPDR:
    result = check_syntax_CPD_CPDR(instruction);
    break;

  case opcode_ADD:
  case opcode_ADC:
    result = check_syntax_ADD_ADDC(instruction); // 8 bit and 16 bit
    break;

  case opcode_SUB:
  case opcode_SBC:
    result = check_syntax_SUB_SBC(instruction); // 8 bit and 16 bit
    break;

  case opcode_AND:
  case opcode_OR:
  case opcode_XOR:
  case opcode_CP:
    result = check_syntax_LOGIC(instruction);
    break;

  case opcode_INC:
  case opcode_DEC:
    result = check_syntax_INC_DEC(instruction); // 8 bit and 16 bit
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
    result = check_syntax_cpu_control(instruction);
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
    result = check_syntax_rotate_shift(instruction);
    break;

  case opcode_BIT:
  case opcode_SET:
  case opcode_RES:
    result = check_syntax_BIT_SET_RES(instruction);
    break;

  case opcode_JP:
  case opcode_JR:
  case opcode_DJNZ:
    result = check_syntax_jump_group(instruction);
    break;

  case opcode_CALL:
  case opcode_RET:
  case opcode_RETI:
  case opcode_RETN:
  case opcode_RST:
    result = check_syntax_call_return_group(instruction);
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
    result = check_syntax_io_group(instruction);
    break;
  }

  parser->currentStatement.type = statement_instruction;

  return result;
}

/**************************************************************************************************/
// Static function definitions
/**************************************************************************************************/
static bool check_syntax_LD(instruction_t *instruction)
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
static bool check_syntax_PUSH_POP(instruction_t *instruction)
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
static bool check_syntax_EX_EXX(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (instruction->opcode == opcode_EX)
  {
    if (operand2.type == operand_NA)
    {
      LOG_INVALID_OPERAND2(instruction);
    }
    else if ((operand1.type == operand_rr && operand1.data.rr == register_DE) &&
             (operand2.type == operand_rr && operand2.data.rr == register_HL))
    {
      instruction->encoding = encoding_EX_DE_HL;
      result = true;
    }
    else if ((operand1.type == operand_rr && operand1.data.rr == register_AF) &&
             (operand2.type == operand_rr && operand2.data.rr == register_AF))
    {
      instruction->encoding = encoding_EX_AF_AF;
      result = true;
    }
    else if ((operand1.type == operand_deref_rr && operand1.data.rr == register_SP) &&
             (operand2.type == operand_rr &&
              (operand2.data.rr == register_HL || operand2.data.rr == register_IX || operand2.data.rr == register_IY)))
    {
      // Set instruction encoding
      if (operand2.data.rr == register_HL)
        instruction->encoding = encoding_EX_derefSP_HL;
      else if (operand2.data.rr == register_IX)
        instruction->encoding = encoding_EX_derefSP_IX;
      else
        instruction->encoding = encoding_EX_derefSP_IY;

      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
  }
  else
  {
    if (operand1.type == operand_NA && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_EXX;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
  }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_LDI_LDIR(instruction_t *instruction)
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
static bool check_syntax_LDD_LDDR(instruction_t *instruction)
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
static bool check_syntax_CPI_CPIR(instruction_t *instruction)
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
static bool check_syntax_CPD_CPDR(instruction_t *instruction)
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
static bool check_syntax_ADD_ADDC(instruction_t *instruction)
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
static bool check_syntax_SUB_SBC(instruction_t *instruction)
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
static bool check_syntax_LOGIC(instruction_t *instruction)
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
    case opcode_AND:
      instruction->encoding = encoding_AND_s;
      break;
    case opcode_OR:
      instruction->encoding = encoding_OR_s;
      break;
    case opcode_XOR:
      instruction->encoding = encoding_XOR_s;
      break;
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
static bool check_syntax_INC_DEC(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;
  if (operand2.type != operand_NA)
  {
    LOG_INVALID_OPERAND2(instruction);
    return false;
  }

  if (instruction->opcode == opcode_INC &&
      (operand1.type == operand_r || operand1.type == operand_deref_HL || operand1.type == operand_deref_idx))
  {
    if (instruction->opcode == opcode_INC)
    {
      if (operand1.type == operand_r)
        instruction->encoding = encoding_INC_r;
      else if (operand1.type == operand_deref_HL)
        instruction->encoding = encoding_INC_derefHL;
      else if (operand1.type == operand_deref_idx && operand1.data.dereference_idx.index_register == register_IX)
        instruction->encoding = encoding_INC_derefIXd;
      else
        instruction->encoding = encoding_INC_derefIYd;
    }

    result = true;
  }
  else if (instruction->opcode == opcode_INC && operand1.type == operand_rr &&
           (operand_is_ss(&operand1) || operand1.data.rr == register_IX || operand1.data.rr == register_IY))
  {
    if (operand_is_ss(&operand1))
      instruction->encoding = encoding_INC_ss;
    else if (operand1.data.rr == register_IX)
      instruction->encoding = encoding_INC_IX;
    else
      instruction->encoding = encoding_INC_IY;
    result = true;
  }
  else if (instruction->opcode == opcode_DEC)
  {
    if (operand_is_m(&operand1))
    {
      instruction->encoding = encoding_DEC_m;
      result = true;
    }
    else if (operand_is_ss(&operand1))
    {
      instruction->encoding = encoding_DEC_ss;
      result = true;
    }
    else if (operand1.type == operand_rr && operand1.data.rr == register_IX)
    {
      instruction->encoding = encoding_DEC_IX;
      result = true;
    }
    else if (operand1.type == operand_rr && operand1.data.rr == register_IY)
    {
      instruction->encoding = encoding_DEC_IY;
      result = true;
    }
    else
    {
      LOG_INVALID_OPERAND1(instruction);
    }
  }
  else
  {
    LOG_INVALID_OPERAND1(instruction);
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_cpu_control(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (operand2.type != operand_NA)
  {
    LOG_INVALID_OPERAND1(instruction);
    return result;
  }
  if (instruction->opcode == opcode_IM && operand1.type == operand_n)
  {
    if (operand1.data.immediate_n <= 2)
    {
      result = true;
    }
    else
    {
      LOG_SYNTAX_ERROR(instruction, "IM operand must be in range 0 - 2!");
    }
  }
  else
  {
    if (operand1.type == operand_NA)
    {
      result = true;
    }
    else
    {
      LOG_INVALID_OPERAND1(instruction);
    }
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_rotate_shift(instruction_t *instruction)
{
  bool result = false;
  opcode_type opcode = instruction->opcode;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (instruction->opcode)
  {
  // Opcodes without operands
  case opcode_RLCA:
  case opcode_RLA:
  case opcode_RRCA:
  case opcode_RRA:
  case opcode_RLD:
  case opcode_RRD:
    if (operand1.type != operand_NA || operand2.type != operand_NA)
    {
      LOG_SYNTAX_ERROR(instruction, "No operands supported for this operation!");
      result = false;
    }
    else
    {
      // Set instruction encoding
      if (opcode == opcode_RLCA)
        instruction->encoding = encoding_RLCA;
      else if (opcode == opcode_RLA)
        instruction->encoding = encoding_RLA;
      else if (opcode == opcode_RRCA)
        instruction->encoding = encoding_RRCA;
      else if (opcode == opcode_RRA)
        instruction->encoding = encoding_RRA;
      else if (opcode == opcode_RLD)
        instruction->encoding = encoding_RLD;
      else
        instruction->encoding = encoding_RRD;

      result = true;
    }
    break;

  case opcode_RLC:
    if (operand2.type != operand_NA)
    {
      LOG_INVALID_OPERAND2(instruction);
      result = false;
    }
    else if (operand1.type == operand_r || operand1.type == operand_deref_HL || operand1.type == operand_deref_idx)
    {
      // Set instruction encoding
      if (operand1.type == operand_deref_HL)
        instruction->encoding = encoding_RLC_derefHL;
      else if (operand1.data.dereference_idx.index_register == register_IX)
        instruction->encoding = encoding_RLC_derefIXd;
      else
        instruction->encoding = encoding_RLC_derefIYd;
      result = true;
    }
    else
    {
      LOG_INVALID_OPERAND1(instruction);
      result = false;
    }
    break;

  // Opcodes with operand type m
  case opcode_RL:
  case opcode_RRC:
  case opcode_RR:
  case opcode_SLA:
  case opcode_SRA:
  case opcode_SRL:
    if (operand2.type == operand_NA && operand_is_m(&operand1))
    {
      // Set instruction encoding
      if (opcode == opcode_RL)
        instruction->encoding = encoding_RL_m;
      else if (opcode == opcode_RRC)
        instruction->encoding = encoding_RRC_m;
      else if (opcode == opcode_RR)
        instruction->encoding = encoding_RR_m;
      else if (opcode == opcode_SLA)
        instruction->encoding = encoding_SLA_m;
      else if (opcode == opcode_SRA)
        instruction->encoding = encoding_SRA_m;
      else
        instruction->encoding = encoding_SRL_m;

      result = true;
    }
    else
    {
      LOG_INVALID_OPERAND1(instruction);
      result = false;
    }
    break;
  default:
    assert(false); // Should not happen
    break;
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_BIT_SET_RES(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  if (operand1.type == operand_NA || operand2.type == operand_NA)
  {
    LOG_SYNTAX_ERROR(instruction, "operation requires two operands!");
    return false;
  }

  if (operand1.type != operand_n || operand1.data.immediate_n > 7)
  {
    LOG_SYNTAX_ERROR(instruction, "Operand 1 must be in range of 0 - 7!");
    return false;
  }

  opcode_type opcode = instruction->opcode;
  if (operand_is_m(&operand2))
  {
    if (opcode == opcode_BIT)
    {
      if (operand2.type == operand_r)
        instruction->encoding = encoding_BIT_b_r;
      else if (operand2.type == operand_deref_HL)
        instruction->encoding = encoding_BIT_b_derefHL;
      else if (operand2.data.dereference_idx.index_register == register_IX)
        instruction->encoding = encoding_BIT_b_derefIXd;
      else
        instruction->encoding = encoding_BIT_b_derefIYd;
    }
    else if (opcode_SET)
    {
      if (operand2.type == operand_r)
        instruction->encoding = encoding_SET_b_r;
      else if (operand2.type == operand_deref_HL)
        instruction->encoding = encoding_SET_b_derefHL;
      else if (operand2.data.dereference_idx.index_register == register_IX)
        instruction->encoding = encoding_SET_b_derefIXd;
      else
        instruction->encoding = encoding_SET_b_derefIYd;
    }
    else
    {
      instruction->encoding = encoding_RES_b_m;
    }

    result = true;
  }
  else
  {
    LOG_INVALID_OPERAND2(instruction);
    result = false;
  }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_jump_group(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (instruction->opcode)
  {
  case opcode_JP:
    if (operand2.type == operand_NA &&
        (operand_is_valid_nn(&operand1) || operand1.type == operand_deref_IX_IY || operand1.type == operand_deref_HL))
    {
      // Set instruction encoding
      if (operand_is_valid_nn(&operand1))
        instruction->encoding = encoding_JP_nn;
      else if (operand1.type == operand_deref_IX_IY && operand1.data.rr == register_IX)
        instruction->encoding = encoding_JP_derefIX;
      else if (operand1.type == operand_deref_IX_IY && operand1.data.rr == register_IY)
        instruction->encoding = encoding_JP_derefIY;
      else
        instruction->encoding = encoding_JP_derefHL;
      result = true;
    }
    else if (operand_is_cc(&operand1) && operand_is_valid_nn(&operand2))
    {
      instruction->encoding = encoding_JP_cc_nn;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_JR:
    // JR e
    if (operand_is_valid_e(&operand1) && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_JR_e;
      result = true;
    }
    else if ((operand1.type == operand_r && (operand1.data.r == register_C || operand1.data.r == register_NC ||
                                             operand1.data.r == register_Z || operand1.data.r == register_NZ)) &&
             operand_is_valid_e(&operand2))
    {
      // Set instruction encoding
      if (operand1.data.r == register_C)
        instruction->encoding = encoding_JR_C_e;
      else if (operand1.data.r == register_NC)
        instruction->encoding = encoding_JR_NC_e;
      else if (operand1.data.r == register_Z)
        instruction->encoding = encoding_JR_Z_e;
      else
        instruction->encoding = encoding_JR_NZ_e;

      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_DJNZ:
    if (operand_is_valid_e(&operand1) && operand2.type == operand_NA)
    {
      instruction->encoding = encoding_DJNZ_e;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  default:
    assert(false); // should never happen
    break;
  }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_call_return_group(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (instruction->opcode)
  {
  case opcode_CALL:
    if (operand2.type == operand_NA && operand_is_valid_nn(&operand1))
    {
      instruction->encoding = encoding_CALL_nn;
      result = true;
    }
    else if (operand_is_cc(&operand1) && operand_is_valid_nn(&operand2))
    {
      instruction->encoding = encoding_CALL_cc_nn;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_RET:
    if ((operand1.type == operand_NA && operand2.type == operand_NA) || (operand_is_cc(&operand1)))
    {
      if (operand1.type == operand_NA)
        instruction->encoding = encoding_RET;
      else
        instruction->encoding = encoding_RET_cc;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_RETI:
  case opcode_RETN:
    if (operand1.type == operand_NA && operand2.type == operand_NA)
    {
      if (instruction->opcode == opcode_RETI)
        instruction->encoding = encoding_RETI;
      else
        instruction->encoding = encoding_RETN;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_RST:

    if (operand2.type == operand_NA && (operand1.type == operand_n))
    {
      uint8_t vector = operand1.data.immediate_n;
      if (vector == 0x00 || vector == 0x08 || vector == 0x10 || vector == 0x18 || vector == 0x20 || vector == 0x28 ||
          vector == 0x30 || vector == 0x38)
      {
        instruction->encoding = encoding_RST_p;
        result = true;
      }
      else
      {
        LOG_SYNTAX_ERROR(instruction,
                         "Reset vector is not valid! Must be 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30 or 0x38");
      }
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

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_io_group(instruction_t *instruction)
{
  bool result = false;
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;
  opcode_type opcode = instruction->opcode;

  switch (opcode)
  {
  case opcode_INI:
  case opcode_INIR:
  case opcode_IND:
  case opcode_INDR:
  case opcode_OUTI:
  case opcode_OTIR:
  case opcode_OUTD:
  case opcode_OTDR:
    if (operand1.type == operand_NA && operand2.type == operand_NA)
    {
      // Set instruction encoding
      if (opcode == opcode_INI)
        instruction->encoding = encoding_INI;
      else if (opcode == opcode_INIR)
        instruction->encoding = encoding_INIR;
      else if (opcode == opcode_IND)
        instruction->encoding = encoding_IND;
      else if (opcode == opcode_INDR)
        instruction->encoding = encoding_INDR;
      else if (opcode == opcode_OUTI)
        instruction->encoding = encoding_OUTI;
      else if (opcode == opcode_OTIR)
        instruction->encoding = encoding_OTIR;
      else if (opcode == opcode_OUTD)
        instruction->encoding = encoding_OUTD;
      else
        instruction->encoding = encoding_OTDR;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_IN:
    if ((operand1.type == operand_r && operand1.data.r == register_A && operand2.type == operand_deref_n) ||
        (operand_is_r(&operand1) && operand2.type == operand_deref_C))
    {
      if (operand2.type == operand_deref_n)
        instruction->encoding = encoding_IN_A_derefn;
      else if (operand2.type == operand_deref_C)
        instruction->encoding = encoding_IN_r_derefC;
      result = true;
    }
    else
    {
      LOG_INVALID_COMBINATION(instruction);
    }
    break;

  case opcode_OUT:

    if ((operand2.type == operand_r && operand2.data.r == register_A && operand1.type == operand_deref_n) ||
        (operand_is_r(&operand2) && operand1.type == operand_deref_C))
    {
      if (operand1.type == operand_deref_n)
        instruction->encoding = encoding_OUT_derefn_A;
      else
        instruction->encoding = encoding_OUT_derefC_r;
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

  return result;
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
static bool expect_operand(instruction_t *instruction, operand_number_t which, ...)
{
  va_list args;
  va_start(args, which);

  bool matched = false;
  operand_type_t currentType;
  if (which == op_1)
  {
    currentType = instruction->operand1.type;
  }
  else
  {
    currentType = instruction->operand2.type;
  }

  operand_type_t expected;
  while ((expected = va_arg(args, operand_type_t)) != operand_end)
  {
    if (expected == currentType)
    {
      matched = true;
      break;
    }
  }
  va_end(args);

  return matched;
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