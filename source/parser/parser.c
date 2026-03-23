#include "parser.h"

#include "parser/directive.h"
#include "parser/instruction.h"
#include "parser_internal.h"

#include "lexer/token.h"
#include "logging/logging.h"
#include "types.h"
#include "utility/linked_list.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************************************/
// Constants
/**************************************************************************************************/
#define MAX_TOKENS_PER_LINE 128

/**************************************************************************************************/
// Static Function Declarations
/**************************************************************************************************/
/**
 * @brief Parses tokens into a list of statements that hold all required information for assembling
 *        Checks for valid syntax, e.g. only valid opcode - operand combinations are parsed into instructions
 *
 * @return true
 * @return false
 */
static parse_line_result_t parse_line(parser_t *parser);

/**************************************************************************************************/
// Public Function Definitions
/**************************************************************************************************/
bool parser_do_it(parser_t *parser, token_list_t *tokenlist)
{
  assert(tokenlist);
  assert(parser);

  memset(parser, 0, sizeof(parser_t));

  parser->statementList = linkedList_initialize(sizeof(instruction_t), NULL, NULL);
  if (!parser->statementList)
  {
    LOG_ERROR("Parser failed. Could not allocate statement list");
    return false;
  }

  parser->inputTokenList = tokenlist;
  parser->currentTokenNode = linkedList_getFirstNode(tokenlist);
  parser->lineNumber = 1;

  parse_line_result_t result = parse_line_next;

  while (result == parse_line_next)
  {
    result = parse_line(parser);
  }

  if (result == parse_line_next)
  {
    LOG_ERROR("Parser failed. Process stopped before EOF!");
    return false;
  }
  else if (result == parse_line_error)
  {
    LOG_ERROR("Parser failed!. Encounterd an error!");
    return false;
  }

  return true;
}

/**************************************************************************************************/
// Static Function Definitions
/**************************************************************************************************/
static parse_line_result_t parse_line(parser_t *parser)
{
  // Similar to generating a lexeme in the lexer, this function groups tokens
  // into statements that can be assembled and checks on valid syntax
  token_t *token;
  memset(&parser->currentStatement, 0, sizeof(statement_t));

  // Handle unexpected token
  if (!expect_token(parser, token_label) && !expect_token(parser, token_opcode) &&
      !expect_token(parser, token_directive) && !expect_token(parser, token_eol) && !expect_token(parser, token_eof))
  {
    LOG_ERROR("[LINE: %d]: Line started with invalid token type!", parser->lineNumber);
    return parse_line_error;
  }

  // Handle EOL
  while (expect_token(parser, token_eol))
  {
    token = consume_token(parser);
    if (token == NULL)
    {
      LOG_ERROR("[LINE: %d]: Parser reached end of list without EOF!", parser->lineNumber);
      return parse_line_error;
    }
  }

  // Handle EOF
  if (expect_token(parser, token_eof))
  {
    consume_token(parser);
    return parse_line_eof;
  }

  // Handle Label
  if (expect_token(parser, token_label))
  {
    token_t *labelToken = get_token(parser);
    parser->currentStatement.type = statement_label;
    strcpy(&parser->currentStatement.label.symbol[0], &labelToken->data.label[0]);

    consume_token(parser);

    // Handle invalid eof
    if (expect_token(parser, token_eof))
    {
      LOG_ERROR("[LINE: %d]: Unexpected end of file after token definition", parser->lineNumber);
      return parse_line_error;
    }

    // Handle line consisting only of a single label
    if (expect_token(parser, token_eol))
    {
      emit_statement(parser);
      return parse_line_next;
    }
  }

  if (expect_token(parser, token_directive))
  {
    if (!directive_parse(parser))
    {
      return parse_line_error;
    }
    return parse_line_next;
  }

  // Handle Operation
  return instruction_parse(parser);
}

/**************************************************************************************************/
/**************************************************************************************************/
token_t *get_token(parser_t *parser) { return listNode_getData(parser->currentTokenNode); }

/**************************************************************************************************/
/**************************************************************************************************/
bool expect_token(parser_t *parser, token_types_t expectedType)
{
  token_t *token = listNode_getData(parser->currentTokenNode);
  return (token->type == expectedType);
}

/**************************************************************************************************/
/**************************************************************************************************/
void emit_statement(parser_t *parser)
{
  linkedList_append(parser->statementList, &parser->currentStatement);
  memset(&parser->currentStatement, 0, sizeof(statement_t));
}

/**************************************************************************************************/
/**************************************************************************************************/
token_t *consume_token(parser_t *parser)
{
  if (parser->currentTokenNode == NULL)
  {
    LOG_ERROR("[LINE: %d]: Tried to consume a token, but there was none left!", parser->lineNumber);
    abort();
  }
  // Increment line count if EOL is consumed
  token_t *token = listNode_getData(parser->currentTokenNode);
  if (token == NULL)
  {
    LOG_ERROR("TokenNode did not contain any data! Aborting!");
    abort();
  }
  if (token->type == token_eol)
  {
    parser->lineNumber++;
  }

  parser->currentTokenNode = listNode_getNext(parser->currentTokenNode);

  if (parser->currentTokenNode == NULL)
  {
    return NULL;
  }
  else
  {
    return listNode_getData(parser->currentTokenNode);
  }
}
