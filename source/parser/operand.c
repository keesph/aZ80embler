#include "parser/operand.h"

#include "lexer/token.h"
#include "logging/logging.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "types.h"

#include <assert.h>
#include <string.h>

#define LOG_MISSING_PARENTHESIS(parser) LOG_OPERAND_ERROR(parser, "Missing closing parenthesis!")

operand_t operand_parse(parser_t *parser)
{
  operand_t operand = {0};

  bool parenthesis_found = false;
  bool index_plus = false;

  if (expect_token(parser, token_lparenthesis))
  {
    parenthesis_found = true;
    consume_token(parser);
  }

  token_t *token = get_token(parser);

  if (expect_token(parser, token_register))
  {
    switch (token->data.registerType)
    {
    case register_A:
    case register_B:
    case register_C:
    case register_D:
    case register_E:
    case register_H:
    case register_L:
    case register_I:
    case register_R:
    // Flags handled same as registers
    case register_NZ:
    case register_Z:
    case register_NC:
    case register_PO:
    case register_PE:
    case register_P:
    case register_M:
      if (parenthesis_found)
      {
        if (token->data.registerType == register_C)
        {
          operand.type = operand_deref_C;
          consume_token(parser);
          if (!expect_token(parser, token_rparenthesis))
          {
            operand.type = operand_invalid;
            LOG_MISSING_PARENTHESIS(parser);
            return operand;
          }
        }
        else
        {
          operand.type = operand_invalid;
          LOG_OPERAND_ERROR(parser, "Operand type r can not be dereferenced!");
          return operand;
        }
      }
      else
      {
        if (token->data.registerType == register_I)
        {
          operand.type = operand_I;
        }
        else if (token->data.registerType == register_R)
        {
          operand.type = operand_R;
        }
        else
        {
          operand.type = operand_r;
        }
        operand.data.r = token->data.registerType;
      }
      break;

    case register_AF:
    case register_BC:
    case register_DE:
    case register_HL:
    case register_SP:
      operand.data.rr = token->data.registerType;
      // (rr)
      if (parenthesis_found)
      {
        consume_token(parser); // Move passed register, check for closing parenthesis
        if (expect_token(parser, token_rparenthesis))
        {
          operand.type = operand_deref_rr;
        }
        else
        {
          operand.type = operand_invalid;
          LOG_MISSING_PARENTHESIS(parser);
          return operand;
        }
      }
      // rr
      else
      {
        operand.type = operand_rr;
      }
      break;

    case register_IX:
    case register_IY:
      // IX, IY, (IX), (IY), (IX+-d), (IY+-d)
      consume_token(parser);
      if (parenthesis_found)
      {
        operand.data.dereference_idx.index_register = token->data.registerType;
        // (IX), (IY)
        if (expect_token(parser, token_rparenthesis))
        {
          operand.type = operand_deref_IX_IY;
        }
        else
        {
          operand.type = operand_deref_idx;
          // Check if index is added or subtracted
          if (expect_token(parser, token_plus))
          {
            index_plus = true;
          }
          else if (expect_token(parser, token_minus))
          {
            index_plus = false;
          }
          else
          {
            LOG_OPERAND_ERROR(parser, "Expecting + or - after IX/IY");
            operand.type = operand_invalid;
            return operand;
          }
          consume_token(parser);
          token = get_token(parser);

          // Check if next token is a literal and if it fits in the offset range, or else if it is a symbol
          // Symbols will be resolved at the assembler stage
          if (expect_token(parser, token_literal_byte))
          {
            if (index_plus && (token->data.literal_byte <= 127))
            {
              operand.data.dereference_idx.index = (int8_t)token->data.literal_byte;
            }
            else if (!index_plus && (token->data.literal_byte <= 128))
            {
              operand.data.dereference_idx.index = (int8_t)(-token->data.literal_byte);
            }
            else
            {
              LOG_OPERAND_ERROR(parser, "Index out of range!");
              operand.type = operand_invalid;
              return operand;
            }
          }
          else if (expect_token(parser, token_symbol))
          {
            operand.data.symbol.symbol = token->data.symbol;
          }
          else
          {
            LOG_OPERAND_ERROR(parser, "Expecting byte literal or symbol after index register!");
            operand.type = operand_invalid;
            return operand;
          }
        }

        consume_token(parser);
        if (!expect_token(parser, token_rparenthesis))
        {
          LOG_MISSING_PARENTHESIS(parser);
          operand.type = operand_invalid;
          return operand;
        }
      }
      // IX, IY
      else
      {
        operand.type = operand_rr;
        operand.data.rr = token->data.registerType;
      }
      break;
    default:
      LOG_OPERAND_ERROR(parser, "Operand is register!");
      operand.type = operand_invalid;
    }
  }
  // Parse (n), (nn), n, nn
  else if (expect_token(parser, token_literal_byte) || expect_token(parser, token_literal_word))
  {
    // (n), (nn)
    if (parenthesis_found)
    {
      if (expect_token(parser, token_literal_byte))
      {
        operand.type = operand_deref_n;
        operand.data.dereference_n = token->data.literal_byte;
      }
      else
      {
        operand.type = operand_deref_nn;
        operand.data.dereference_nn = token->data.literal_word;
      }

      consume_token(parser);
      if (!expect_token(parser, token_rparenthesis))
      {
        LOG_OPERAND_ERROR(parser, "Missing cloding parenthesis!");
        operand.type = operand_invalid;
        return operand;
      }
    }
    // n, nn
    else
    {
      if (expect_token(parser, token_literal_byte))
      {
        operand.type = operand_n;
        operand.data.immediate_n = token->data.literal_byte;
      }
      else
      {
        operand.type = operand_nn;
        operand.data.immediate_nn = token->data.literal_word;
      }
    }
  }
  // Parse special case, negative e
  else if (expect_token(parser, token_minus))
  {
    if (parenthesis_found)
    {
      LOG_OPERAND_ERROR(parser, "Operand type e can not be dereferenced!");
      operand.type = operand_invalid;
      return operand;
    }
    consume_token(parser);
    // Check if operand is a literal byte and fits into the range of -126 - 0
    if (expect_token(parser, token_literal_byte) && get_token(parser)->data.literal_byte <= 126)
    {
      operand.type = operand_e;
      operand.data.relative_offset_e = (int8_t)(-get_token(parser)->data.literal_byte);
    }
    else
    {
      LOG_OPERAND_ERROR(parser, "Does not match criteria for operand type e!");
      operand.type = operand_invalid;
      return operand;
    }
  }
  // Parse (symbol), symbol
  else if (expect_token(parser, token_symbol))
  {
    if (parenthesis_found)
    {
      operand.type = operand_deref_symbol;
      operand.data.symbol.symbol = token->data.symbol;
      consume_token(parser);

      if (!expect_token(parser, token_rparenthesis))
      {
        LOG_MISSING_PARENTHESIS(parser);
        operand.type = operand_invalid;
        return operand;
      }
    }
    else
    {
      operand.type = operand_symbol;
      operand.data.symbol.symbol = token->data.symbol;
    }
  }

  consume_token(parser); // Move to next token
  return operand;
}

char *operand_toString(operand_type_t operand)
{
  char *result = "";
  switch (operand)
  {
  case operand_NA:
    result = "NO_OPERAND";
    break;

  case operand_invalid:
    result = "INVALID_OPERAND";
    break;

  case operand_r:
    result = "r";
    break;

  case operand_I:
    result = "I";
    break;

  case operand_R:
    result = "R";
    break;

  case operand_rr:
    result = "rr";
    break;

  case operand_deref_HL:
    result = "(HL)";
    break;

  case operand_deref_rr:
    result = "(rr)";
    break;

  case operand_deref_IX_IY:
    result = "(IXY)";
    break;

  case operand_deref_idx:
    result = "(IXY+d)";
    break;

  case operand_deref_C:
    result = "(C)";
    break;

  case operand_n:
    result = "n";
    break;

  case operand_nn:
    result = "nn";
    break;

  case operand_deref_n:
    result = "(n)";
    break;

  case operand_deref_nn:
    result = "(nn)";
    break;

  case operand_b:
    result = "b";
    break;

  case operand_e:
    result = "e";
    break;

  case operand_cc:
    result = "cc";
    break;

  case operand_symbol:
    result = "SYMBOL";
    break;

  case operand_deref_symbol:
    result = "(SYMBOL)";
    break;

  case operand_string:
    result = "STRING";
    break;
  default:
    assert(false); // Should not happen
    break;
  }
  return result;
}
