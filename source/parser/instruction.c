#include "parser/instruction.h"

#include "lexer/token.h"
#include "parser/operand.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"

#include "logging/logging.h"
#include "types.h"

#define EXPECT_OPERAND(instruction, which, ...) expect_operand(instruction, which, __VA_ARGS__, -1)

typedef enum
{
  op_1,
  op_2
} operand_number_t;

// local functions used to parse tokens into instruction and check valid syntax
static bool check_syntax_LD(instruction_t *instruction);
static bool check_syntax_PUSH_POP(instruction_t *instruction);
static bool check_syntax_EX_EXX(instruction_t *instruction);
static bool check_syntax_LDI_LDIR(instruction_t *instruction);
static bool check_syntax_LDD_LDDR(instruction_t *instruction);
static bool check_syntax_CPI_CPIR(instruction_t *instruction);
static bool check_syntax_CPD_CPDR(instruction_t *instruction);
static bool check_syntax_ADD_ADDC(instruction_t *instruction); // 8 bit and 16 bit
static bool check_syntax_SUB_SBC(instruction_t *instruction);
static bool check_syntax_LOGIC(instruction_t *instruction); // AND, OR, XOR
static bool check_syntax_CP(instruction_t *instruction);
static bool check_syntax_INC_DEC(instruction_t *instruction);
static bool check_syntax_cpu_control(instruction_t *instruction);
static bool check_syntax_rotate_shift(instruction_t *instruction);
static bool check_syntax_BIT_SET_RES(instruction_t *instruction);
static bool check_syntax_jump_group(instruction_t *instruction);
static bool check_syntax_call_return_group(instruction_t *instruction);
static bool check_syntax_io_group(instruction_t *instruction);

static bool register_is_r(register_type type);
static bool register_is_dd(register_type type);
static bool register_is_qq(register_type type);
static bool register_is_cc(register_type type);
static bool register_is_ss(register_type type);
static bool register_is_pp(register_type type);
static bool register_is_rr(register_type type); // rr in the standard means something else than operator_rr!

static bool expect_operand(instruction_t *instruction, operand_number_t which, ...);

bool instruction_parse(parser_t *parser)
{
  bool result = false;

  if (!expect_token(parser, token_opcode))
  {
    LOG_ERROR("[LINE: %d]: Error when parsing token. Expected opcode!", parser->currentLine);
    return false;
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
      LOG_ERROR("[LINE: %d]: Error when parsing Opcode 1. Invlaid operand!", parser->currentLine);
      return false;
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
        LOG_ERROR("[LINE: %d]: Error when parsing Opcode 2. Invlaid operand!", parser->currentLine);
        return false;
      }
      instruction->operand2 = operand;
    }

    if (!expect_token(parser, token_eof) && !expect_token(parser, token_eol))
    {
      LOG_ERROR("[LINE: %d]: Error when parsing instruction. Expected EOF or EOL!", parser->currentLine);
      return false;
    }
  }
  instruction->sourceLine = parser->currentLine;

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
    result = check_syntax_LOGIC(instruction);
    break;

  case opcode_CP:
    result = check_syntax_CP(instruction);
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
/**************************************************************************************************/
static bool check_syntax_LD(instruction_t *instruction)
{
  bool result = false;
  if (instruction->operand1.type == operand_NA || instruction->operand2.type == operand_NA)
  {
    LOG_ERROR("[LINE: %d], Expetec two operands for opcode LD", instruction->sourceLine);
  }
  operand_t operand1 = instruction->operand1;
  operand_t operand2 = instruction->operand2;

  switch (operand1.type)
  {
  case operand_r:
    // [r, r], [r, n], [r, (IX+d)], [r, (IY+d)], [r, (HL))]
    if (EXPECT_OPERAND(instruction, op_2, operand_r, operand_n, operand_deref_idx, operand_deref_HL))
    {
      result = true;
    }
    // [A, (BC)], [A, (DE)], [A, (nn)], [A, I], [A, R]
    else if (operand1.data.r == register_A)
    {
      if (operand2.type == operand_deref_rr && (operand2.data.rr == register_BC || operand2.data.rr == register_DE))
      {
        result = true;
      }
      else if (EXPECT_OPERAND(instruction, op_2, operand_deref_nn, operand_I, operand_R))
      {
        result = true;
      }
      else
      {
        LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                  instruction->sourceLine);
      }
    }
    break;

  case operand_deref_HL:
    result = EXPECT_OPERAND(instruction, op_2, operand_r, operand_n);
    if (!result)
    {
      LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                instruction->sourceLine);
    }
    break;

  case operand_deref_rr:
    // [(HL), r], [(HL), n], [(BC), A], [(DE), A]
    if (((operand1.data.rr == register_BC || operand1.data.rr == register_DE) &&
         (operand2.type == operand_r && operand2.data.r == register_A)))
    {
      result = true;
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                instruction->sourceLine);
    }
    break;

  case operand_deref_idx:
    // [(IX+d), r], [(IY+d), r], [(IX+d), n], [(IY+d), n]
    result = EXPECT_OPERAND(instruction, op_2, operand_r, operand_n);
    if (!result)
    {
      LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                instruction->sourceLine);
    }
    break;

  case operand_rr:
    // [dd, nn], [dd, (nn)]
    if (register_is_dd(operand1.data.rr) && (operand2.type == operand_nn || operand2.type == operand_deref_nn))
    {
      result = true;
    }
    // [IX/IY, nn, (nn)]
    else if ((operand1.data.rr == register_IX || operand1.data.rr == register_IY) &&
             (operand2.type == operand_nn || operand2.type == operand_deref_nn))
    {
      result = true;
    }
    // [HL, (nn)]
    else if (operand1.data.rr == register_HL && operand2.type == operand_deref_nn)
    {
      result = true;
    }
    // [SP, HL/IX/IY]
    else if (operand1.data.rr == register_SP &&
             (operand2.type == operand_rr &&
              (operand2.data.rr == register_IX || operand2.data.rr == register_IY || operand2.data.rr == register_HL)))
    {
      result = true;
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                instruction->sourceLine);
    }
    break;

  case operand_R:
  case operand_I:
    // [R/I, A]
    result = (operand2.type == operand_r && operand2.data.r == register_A);
    if (result)
    {
      LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                instruction->sourceLine);
    }
    break;

  case operand_deref_nn:
    // [(nn), A]
    if (operand2.type == operand_r && operand2.data.r == register_A)
    {
      result = true;
    }
    // [(nn), IX/IY/dd]
    else if (operand2.type == operand_rr &&
             (register_is_dd(operand2.data.rr) || operand2.data.rr == register_IX || operand2.data.rr == register_IY))
    {
      result = true;
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand2 is not supported in combination with Operand1!",
                instruction->sourceLine);
    }
  default:
    LOG_ERROR("[LINE: %d]: Invalid Syntax! Operand1 is not supported for LD!", instruction->sourceLine);
    break;
  }
  if (!result)
  {
    LOG_ERROR("[LINE: %d]: Invalid Syntax! Combination of operands is not allowed!", instruction->sourceLine);
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_PUSH_POP(instruction_t *instruction)
{
  bool result = false;

  if (instruction->operand2.type != operand_NA)
  {
    LOG_ERROR("[LINE: %d]: PUSH/POP does not support second operand!");
    return result;
  }

  operand_t operand1 = instruction->operand1;
  if (instruction->operand1.type == operand_rr &&
      (register_is_qq(operand1.data.rr) || operand1.data.rr == register_IX || operand1.data.rr == register_IY))
  {
    result = true;
  }
  else
  {
    LOG_ERROR("[LINE: %d]: PUSH/POP - Invalid operand. Supported: qq, IX, IY!");
  }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_EX_EXX(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_LDI_LDIR(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_LDD_LDDR(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_CPI_CPIR(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_CPD_CPDR(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_ADD_ADDC(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_SUB_SBC(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_LOGIC(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_CP(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_INC_DEC(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_cpu_control(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_rotate_shift(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_BIT_SET_RES(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_jump_group(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_call_return_group(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool check_syntax_io_group(instruction_t *instruction)
{
  (void)parser;
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_r(register_type type)
{
  // Check if the given type is part of the register group r
  if (type == register_A || type == register_B || type == register_C || type == register_D || type == register_E ||
      type == register_H || type == register_L)
  {
    return true;
  }
  return false;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_dd(register_type type)
{
  return (type == register_BC || type == register_DE || type == register_HL || type == register_SP);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_qq(register_type type)
{
  return (type == register_BC || type == register_DE || type == register_HL || type == register_AF);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_cc(register_type type)
{
  return (type == register_NZ || type == register_Z || type == register_NC || type == register_C ||
          type == register_PO || type == register_PE || type == register_P || type == register_M);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_ss(register_type type)
{
  return (type == register_BC || type == register_DE || type == register_HL || type == register_SP);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_pp(register_type type)
{
  return (type == register_BC || type == register_DE || type == register_IX || type == register_SP);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool register_is_rr(register_type type)
{
  return (type == register_BC || type == register_DE || type == register_IY || type == register_SP);
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
  while ((expected = va_arg(args, operand_type_t)) != -1)
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