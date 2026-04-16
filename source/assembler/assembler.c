#include "assembler/assembler.h"

#include "lexer/lexer.h"
#include "logging/logging.h"
#include "parser/parser.h"
#include "types.h"
#include "utility/alloc_w.h"
#include "utility/linked_list.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static bool symbol_compareCb(void *s1, void *s2);
static void symbol_freeCb(void *s);

static size_t getInstructionSize(instruction_t instruction);

static size_t programCounter;

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
void assembler_initialize(assembler_t *assembler)
{
  assembler = calloc_w(1, sizeof(assembler_t));

  assembler->symbolList = linkedList_initialize(sizeof(symbol_t), symbol_freeCb, symbol_compareCb);
  assembler->importedSymbols = linkedList_initialize(sizeof(symbol_t), symbol_freeCb, symbol_compareCb);
  assembler->exportedSymbols = linkedList_initialize(sizeof(symbol_t), symbol_freeCb, symbol_compareCb);

  assembler->lexer = lexer_initialize();
  assembler->parser = parser_initialize();
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
bool assembler_pass_one(assembler_t *assembler)
{
  statement_list_t *statementList = parser_getStatementList(assembler->parser);
  statement_t *currentStatement = listNode_getData(linkedList_getFirstNode(statementList));

  symbol_t newSymbol;

  while (currentStatement != NULL)
  {
    switch (currentStatement->type)
    {
    case statement_label:
      ListNode *symbolNode = linkedList_find(assembler->symbolList, currentStatement->label.symbol);

      // if the symbol is already present in the list, it can now be resolved
      if (symbolNode != NULL)
      {
        symbol_t *symbol = listNode_getData(symbolNode);
        if (symbol->isResolved)
        {
          return LOG_ASSEMBLER_ERROR(currentStatement, "Symbol %s defined twice!", symbol->symbol);
        }

        symbol->value = assembler->programCounter;
        symbol->isResolved = true;
      }
      else
      {
        newSymbol.isResolved = true;
        newSymbol.value = assembler->programCounter;
        newSymbol.symbol = strdup_w(currentStatement->label.symbol);
        linkedList_append(assembler->symbolList, &newSymbol);
      }
      break;

    case statement_directive:

      break;

    case statement_instruction:

      break;

    case statement_undefined:
    default:
      assert(false); // Should not happen!
      break;
    }
  }
  return false;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
bool assembler_pass_two(statement_list_t *statementList, bool outputBinary)
{
  (void)statementList;
  (void)outputBinary;
  return false;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
static bool symbol_compareCb(void *s1, void *s2)
{
  char *symbol1 = (char *)s1;
  char *symbol2 = (char *)s2;

  return (strcmp(symbol1, symbol2) == 0);
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
static void symbol_freeCb(void *s)
{
  symbol_t *symbol = (symbol_t *)s;
  free(symbol->symbol);
  free(symbol);
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
size_t getInstructionSize(instruction_t instruction)
{
  size_t size = 0;
  switch (instruction.encoding)
  {
  case encoding_LD_r_r:
  case encoding_LD_r_derefHL:
  case encoding_LD_derefHL_r:
  case encoding_LD_A_derefBC:
  case encoding_LD_A_derefDE:
  case encoding_LD_derefBC_A:
  case encoding_LD_derefDE_A:
  case encoding_LD_SP_HL:
  case encoding_PUSH_qq:
  case encoding_POP_qq:
  case encoding_EX_DE_HL:
  case encoding_EX_AF_AF:
  case encoding_EX_derefSP_HL:
  case encoding_EXX:
  case encoding_ADD_A_r:
  case encoding_ADD_A_derefHL:
  case encoding_ADD_HL_ss:
  case encoding_INC_ss:
  case encoding_DEC_ss:
  case encoding_DAA:
  case encoding_CPL:
  case encoding_CCF:
  case encoding_SCF:
  case encoding_NOP:
  case encoding_HALT:
  case encoding_DI:
  case encoding_EI:
  case encoding_RLCA:
  case encoding_RLA:
  case encoding_RRCA:
  case encoding_RRA:
  case encoding_JP_derefHL:
  case encoding_RET:
  case encoding_RET_cc:
  case encoding_RST_p:
    size = 1;
    break;

  case encoding_LD_r_n:
  case encoding_LD_derefHL_n:
  case encoding_LD_A_I:
  case encoding_LD_A_R:
  case encoding_LD_I_A:
  case encoding_LD_R_A:
  case encoding_LD_SP_IX:
  case encoding_LD_SP_IY:
  case encoding_PUSH_IX:
  case encoding_PUSH_IY:
  case encoding_POP_IX:
  case encoding_POP_IY:
  case encoding_EX_derefSP_IX:
  case encoding_EX_derefSP_IY:
  case encoding_LDI:
  case encoding_LDIR:
  case encoding_LDD:
  case encoding_LDDR:
  case encoding_CPI:
  case encoding_CPIR:
  case encoding_CPD:
  case encoding_CPDR:
  case encoding_ADD_IX_pp:
  case encoding_ADD_IY_rr:
  case encoding_ADC_HL_ss:
  case encoding_SBC_HL_ss:
  case encoding_INC_IX:
  case encoding_INC_IY:
  case encoding_DEC_IX:
  case encoding_DEC_IY:
  case encoding_NEG:
  case encoding_IM:
  case encoding_RLC_r:
  case encoding_RLC_derefHL:
  case encoding_RLD:
  case encoding_RRD:
  case encoding_BIT_b_r:
  case encoding_BIT_b_derefHL:
  case encoding_SET_b_r:
  case encoding_SET_b_derefHL:
  case encoding_JR_e:
  case encoding_JR_C_e:
  case encoding_JR_NC_e:
  case encoding_JR_Z_e:
  case encoding_JR_NZ_e:
  case encoding_DJNZ_e:
  case encoding_JP_derefIX:
  case encoding_JP_derefIY:
  case encoding_RETI:
  case encoding_RETN:
  case encoding_IN_A_derefn:
  case encoding_IN_r_derefC:
  case encoding_INI:
  case encoding_INIR:
  case encoding_IND:
  case encoding_INDR:
  case encoding_OUT_derefn_A:
  case encoding_OUT_derefC_r:
  case encoding_OUTI:
  case encoding_OTIR:
  case encoding_OUTD:
  case encoding_OTDR:
    size = 2;
    break;

  case encoding_LD_r_derefIXd:
  case encoding_LD_r_derefIYd:
  case encoding_LD_derefIXd_r:
  case encoding_LD_derefIYd_r:
  case encoding_LD_A_derefnn:
  case encoding_LD_derefnn_A:
  case encoding_LD_dd_nn:
  case encoding_LD_HL_derefnn:
  case encoding_LD_derefnn_HL:
  case encoding_ADD_A_n:
  case encoding_ADD_A_derefIXd:
  case encoding_ADD_A_derefIYd:
  case encoding_JP_nn:
  case encoding_JP_cc_nn:
  case encoding_CALL_nn:
  case encoding_CALL_cc_nn:
    size = 3;
    break;

  case encoding_LD_derefIXd_n:
  case encoding_LD_derefIYd_n:
  case encoding_LD_IX_nn:
  case encoding_LD_IY_nn:
  case encoding_LD_dd_derefnn:
  case encoding_LD_IX_derefnn:
  case encoding_LD_IY_derefnn:
  case encoding_LD_derefnn_dd:
  case encoding_LD_derefnn_IX:
  case encoding_LD_derefnn_IY:
  case encoding_RLC_derefIXd:
  case encoding_RLC_derefIYd:
  case encoding_BIT_b_derefIXd:
  case encoding_BIT_b_derefIYd:
  case encoding_SET_b_derefIXd:
  case encoding_SET_b_derefIYd:
    size = 4;
    break;

  case encoding_ADC_A_s:
  case encoding_SBC_A_s:
    switch (instruction.operand2.type)
    {
    case operand_r:
    case operand_deref_HL:
      size = 1;
      break;

    case operand_n:
      size = 2;
      break;

    case operand_deref_idx:
      size = 3;
      break;

    default:
      assert(false); // Should not happen!
    }
    break;

  case encoding_SUB_s:
  case encoding_AND_s:
  case encoding_OR_s:
  case encoding_XOR_s:
  case encoding_CP_s:
    switch (instruction.operand1.type)
    {
    case operand_r:
    case operand_deref_HL:
      size = 1;
      break;

    case operand_n:
      size = 2;
      break;

    case operand_deref_idx:
      size = 3;
      break;

    default:
      assert(false); // Should not happen!
    }
    break;

  case encoding_INC_m:
  case encoding_DEC_m:
    switch (instruction.operand1.type)
    {
    case operand_r:
    case operand_deref_HL:
      size = 1;
      break;

    case operand_deref_idx:
      size = 3;
      break;

    default:
      assert(false); // Should not happen
    }
    break;

  case encoding_RL_m:
  case encoding_RRC_m:
  case encoding_RR_m:
  case encoding_SLA_m:
  case encoding_SRA_m:
  case encoding_SRL_m:
    switch (instruction.operand1.type)
    {
    case operand_r:
    case operand_deref_HL:
      size = 2;
      break;

    case operand_deref_idx:
      size = 4;
      break;

    default:
      assert(false); // Should not happen!
    }
    break;

  case encoding_RES_b_m:
    switch (instruction.operand2.type)
    {
    case operand_r:
    case operand_deref_HL:
      size = 2;
      break;

    case operand_deref_idx:
      size = 4;
      break;

    default:
      assert(false); // Should not happen!
    }
    break;
  }
  return size;
}