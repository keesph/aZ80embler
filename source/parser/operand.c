#include "parser/operand.h"

#include "lexer/token.h"
#include "logging/logging.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "types.h"

#include <string.h>

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
            LOG_ERROR("[LINE: %d]: Operand is invalid. Type (C), but no closing parenthesis!", parser->currentLine);
            return operand;
          }
        }
        else
        {
          operand.type = operand_invalid;
          LOG_ERROR("[LINE: %d]: Operand is invalid. Type r, but found a leading parenthesis!", parser->currentLine);
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
          LOG_ERROR("[LINE: %d]: Operand is invalid. Type (rr), but found no closing parenthesis!",
                    parser->currentLine);
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
            LOG_ERROR("[LINE: %d]: Operand is invalid. Expected + or minus after (IX/IY  )", parser->currentLine);
            operand.type = operand_invalid;
            return operand;
          }
          consume_token(parser);
          token = get_token(parser);

          // Check if next token is a literal and if it fits in the offset range
          if (!expect_token(parser, token_literal_byte))
          {
            LOG_ERROR("[LINE: %d]: Operand is invalid. Expected byte literal after index register!",
                      parser->currentLine);
            operand.type = operand_invalid;
            return operand;
          }
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
            LOG_ERROR("[LINE: %d]: Operand is invalid. Index out of range!", parser->currentLine);
            operand.type = operand_invalid;
            return operand;
          }
        }

        consume_token(parser);
        if (!expect_token(parser, token_rparenthesis))
        {
          LOG_ERROR("[LINE: %d]: Operand is invalid. Expected r_parenthesis after (IX/IY    )!", parser->currentLine);
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
      LOG_ERROR("[LINE: %d]: Operand is register!", parser->currentLine);
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
        LOG_ERROR("[LINE: %d]: Operand is invalid! missing closing parenthesis for (nn)", parser->currentLine);
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
      LOG_ERROR("[LINE: %d]: Operand is invalid! Type e can not be dereferenced!", parser->currentLine);
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
      LOG_ERROR("[LINE: %d]: Operand is invalid! Does not match criteria for type e", parser->currentLine);
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
      memcpy(&operand.data.symbol.symbol[0], &token->data.symbol[0], LABEL_MAX_LENGTH);
      consume_token(parser);

      if (!expect_token(parser, token_rparenthesis))
      {
        LOG_ERROR("[LINE: %d]: Operand is invalid! missing closing parenthesis for (symbol)", parser->currentLine);
        operand.type = operand_invalid;
        return operand;
      }
    }
    else
    {
      operand.type = operand_symbol;
      memcpy(&operand.data.symbol.symbol[0], &token->data.symbol[0], LABEL_MAX_LENGTH);
    }
  }

  consume_token(parser); // Move to next token
  return operand;
}
