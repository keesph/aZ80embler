#include "parser/directive.h"

#include "lexer/token.h"
#include "logging/logging.h"
#include "parser_internal.h"
#include "types.h"

#include <string.h>

static bool parse_directive_DB(parser_t *parser);
static bool parse_directive_DW(parser_t *parser);
static bool parse_directive_DS(parser_t *parser);
static bool parse_directive_EQU(parser_t *parser);
static bool parse_directive_ORG(parser_t *parser);
static bool parse_directive_EXPORT(parser_t *parser);
static bool parse_directive_IMPORT(parser_t *parser);
static bool parse_directive_SECTION(parser_t *parser);

bool directive_parse(parser_t *parser)
{
  bool result;

  // Get current token from list for processing, then consume from list
  token_t *token = get_token(parser);
  consume_token(parser);

  switch (token->data.directiveType)
  {
  case directive_DB:
    result = parse_directive_DB(parser);
    break;

  case directive_DW:
    result = parse_directive_DW(parser);
    break;

  case directive_DS:
    result = parse_directive_DS(parser);
    break;

  case directive_EQU:
    result = parse_directive_EQU(parser);
    break;

  case directive_ORG:
    result = parse_directive_ORG(parser);
    break;

  case directive_EXPORT:
    result = parse_directive_EXPORT(parser);
    break;

  case directive_IMPORT:
    result = parse_directive_IMPORT(parser);
    break;

  case directive_SECTION:
    result = parse_directive_SECTION(parser);
    break;
  default:
    LOG_ERROR("[LINE: %d]: Invalid directive type", parser->currentLine);
    return false;
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_DB(parser_t *parser)
{
  if (!expect_token(parser, token_literal_byte))
  {
    LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected "
              "byte literal",
              parser->currentLine);
    return false;
  }

  directive_t *directive = &parser->currentStatement.directive;

  directive->type = directive_DB;
  directive->operand.type = operand_n;
  directive->operand.data.immediate_n = get_token(parser)->data.literal_byte;
  parser->currentStatement.size = 1;

  emit_statement(parser);
  consume_token(parser);

  while (!expect_token(parser, token_eol) && !expect_token(parser, token_eof))
  {
    if (!expect_token(parser, token_comma))
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected comma", parser->currentLine);
      return false;
    }

    // Jump over COMMA token and check if next is as expected
    consume_token(parser);
    if (expect_token(parser, token_literal_byte))
    {
      directive->type = directive_DB;
      directive->operand.type = operand_n;
      directive->operand.data.immediate_n = get_token(parser)->data.literal_byte;
      parser->currentStatement.type = statement_directive;
      parser->currentStatement.size = 1;
      emit_statement(parser);
      consume_token(parser);
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Expected literal after comma!", parser->currentLine);
      return false;
    }
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_DW(parser_t *parser)
{
  // Check if the required label is set for this instruction.
  if (parser->currentStatement.type != statement_label)
  {
    LOG_ERROR("[LINE: %d]: Tried to parse DW directive, but was not preceeded by label", parser->currentLine);
    return false;
  }
  if (!expect_token(parser, token_literal_word) && !expect_token(parser, token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DW. Expected "
              "word literal or symbol",
              parser->currentLine);
    return false;
  }

  directive_t *directive = &parser->currentStatement.directive;

  directive->type = directive_DW;
  if (expect_token(parser, token_symbol))
  {
    directive->operand.type = operand_symbol;
    memcpy(&directive->operand.data.symbol.symbol, get_token(parser)->data.symbol, LABEL_MAX_LENGTH);
  }
  else
  {
    directive->operand.type = operand_nn;
    directive->operand.data.immediate_nn = get_token(parser)->data.literal_word;
  }
  parser->currentStatement.size = 2;
  emit_statement(parser);
  consume_token(parser);

  while (!expect_token(parser, token_eol) && !expect_token(parser, token_eof))
  {
    if (!expect_token(parser, token_comma))
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected comma", parser->currentLine);
      return false;
    }

    // Jump over COMMA token and check if next is as expected
    consume_token(parser);
    if (expect_token(parser, token_literal_word) || expect_token(parser, token_symbol))
    {
      if (expect_token(parser, token_symbol))
      {
        directive->operand.type = operand_symbol;
        memcpy(&directive->operand.data.symbol.symbol, get_token(parser)->data.symbol, LABEL_MAX_LENGTH);
      }
      else
      {
        directive->operand.type = operand_nn;
        directive->operand.data.immediate_nn = get_token(parser)->data.literal_word;
      }

      parser->currentStatement.type = statement_directive;
      parser->currentStatement.size = 2;
      emit_statement(parser);
      consume_token(parser);
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Expected literal or symbol after comma!", parser->currentLine);
      return false;
    }
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_DS(parser_t *parser)
{
  // Check if the required label is set for this instruction.
  if (parser->currentStatement.type != statement_label)
  {
    LOG_ERROR("[LINE: %d]: Tried to parse DS directive, but was not preceeded by label", parser->currentLine);
    return false;
  }
  directive_t *directive = &parser->currentStatement.directive;
  bool result;

  if (!expect_token(parser, token_string))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after DS. Expected string!", parser->currentLine);
    return false;
  }
  directive->type = directive_DS;
  directive->operand.type = operand_string;
  strcpy(&directive->operand.data.string_literal[0], &get_token(parser)->data.string[0]);

  parser->currentStatement.type = statement_directive;
  parser->currentStatement.size = strlen(directive->operand.data.string_literal);
  emit_statement(parser);
  result = expect_token(parser, token_eol);
  consume_token(parser);

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_EQU(parser_t *parser)
{
  // Check if the required label is set for this instruction.
  if (parser->currentStatement.type != statement_label)
  {
    LOG_ERROR("[LINE: %d]: Tried to parse EQU directive, but was not preceeded by label", parser->currentLine);
    return false;
  }
  directive_t *directive = &parser->currentStatement.directive;
  if (!expect_token(parser, token_literal_byte) && !expect_token(parser, token_literal_word))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EQU. Expected byte, word!", parser->currentLine);
    return false;
  }

  token_t *token = get_token(parser);

  directive->type = directive_EQU;
  if (token->type == token_literal_byte)
  {
    parser->currentStatement.label.value = token->data.literal_byte;
  }
  else
  {
    parser->currentStatement.label.value = token->data.literal_word;
  }

  parser->currentStatement.type = statement_directive;
  emit_statement(parser);
  consume_token(parser);

  if (!expect_token(parser, token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EQU. expected EOL!", parser->currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_ORG(parser_t *parser)
{
  directive_t *directive = &parser->currentStatement.directive;

  if (!expect_token(parser, token_literal_byte) && !expect_token(parser, token_literal_word))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after ORG. Expected byte or word!", parser->currentLine);
    return false;
  }
  directive->type = directive_ORG;
  if (expect_token(parser, token_literal_byte))
  {
    directive->operand.type = operand_n;
    directive->operand.data.immediate_n = get_token(parser)->data.literal_byte;
  }
  else
  {
    directive->operand.type = operand_nn;
    directive->operand.data.immediate_nn = get_token(parser)->data.literal_word;
  }

  parser->currentStatement.size = 0; // directive does not result in memory usage
  emit_statement(parser);
  consume_token(parser);
  if (!expect_token(parser, token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after ORG. expected EOL!", parser->currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_EXPORT(parser_t *parser)
{
  directive_t *directive = &parser->currentStatement.directive;

  if (!expect_token(parser, token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EXPORT. Expected symbol!", parser->currentLine);
    return false;
  }
  directive->type = directive_EXPORT;
  directive->operand.type = operand_symbol;
  strcpy(&directive->operand.data.symbol.symbol[0], &get_token(parser)->data.symbol[0]);
  parser->currentStatement.size = 0; // directive does not result in memory usage
  emit_statement(parser);
  consume_token(parser);
  if (!expect_token(parser, token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EXPORT. expected EOL!", parser->currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_IMPORT(parser_t *parser)
{
  directive_t *directive = &parser->currentStatement.directive;

  if (!expect_token(parser, token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after IMPORT. Expected symbol!", parser->currentLine);
    return false;
  }
  directive->type = directive_IMPORT;
  directive->operand.type = operand_symbol;
  strcpy(&directive->operand.data.symbol.symbol[0], &get_token(parser)->data.symbol[0]);
  parser->currentStatement.size = 0; // directive does not result in memory usage
  emit_statement(parser);
  consume_token(parser);
  if (!expect_token(parser, token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after IMPORT. expected EOL!", parser->currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_SECTION(parser_t *parser)
{
  directive_t *directive = &parser->currentStatement.directive;

  if (!expect_token(parser, token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after SECTION. Expected symbol!", parser->currentLine);
    return false;
  }
  directive->type = directive_SECTION;
  directive->operand.type = operand_symbol;
  strcpy(&directive->operand.data.symbol.symbol[0], &get_token(parser)->data.symbol[0]);
  parser->currentStatement.size = 0; // directive does not result in memory usage
  emit_statement(parser);
  consume_token(parser);
  if (!expect_token(parser, token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after SECTION. expected EOL!", parser->currentLine);
    return false;
  }

  return true;
}
