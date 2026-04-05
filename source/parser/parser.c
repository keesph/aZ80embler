#include "parser.h"

#include "identifier.h"
#include "parser/directive.h"
#include "parser/instruction.h"
#include "parser_internal.h"

#include "lexer/token.h"
#include "logging/logging.h"
#include "types.h"
#include "utility/alloc_w.h"
#include "utility/linked_list.h"

#include <assert.h>
#include <stdio.h>
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

static void statement_free_callback(void *statementToFree);

/**************************************************************************************************/
// Public Function Definitions
/**************************************************************************************************/
parser_t *parser_initialize()
{
  parser_t *parser = calloc(1, sizeof(parser_t));
  if (!parser)
  {
    LOG_ERROR("Failed to allocate parser object!");
    return NULL;
  }

  parser->statementList = linkedList_initialize(sizeof(statement_t), statement_free_callback, NULL);
  if (!parser->statementList)
  {
    LOG_ERROR("Failed to allocate parser statement list!");
    return NULL;
  }
  return parser;
}

/**************************************************************************************************/
/**************************************************************************************************/
void parser_destroy(parser_t *parser)
{
  assert(parser);

  linkedList_destroy(parser->statementList);
  free(parser);
}

/**************************************************************************************************/
/**************************************************************************************************/
void parser_reset(parser_t *parser)
{
  linkedList_clear(parser->statementList);
  memset(&parser->currentStatement, 0, sizeof(statement_t));
  parser->currentTokenNode = NULL;
  parser->lineNumber = 0;
}

/**************************************************************************************************/
/**************************************************************************************************/
bool parser_do_it(parser_t *parser, token_list_t *list)
{
  assert(parser);
  assert(list);

  parser->statementList = linkedList_initialize(sizeof(statement_t), statement_free_callback, NULL);
  if (!parser->statementList)
  {
    LOG_ERROR("Parser failed. Could not allocate statement list");
    return false;
  }

  parser->currentTokenNode = linkedList_getFirstNode(list);
  parser->lineNumber = 1;

  parse_line_result_t result = parse_line_next;

  while (result == parse_line_next)
  {
    result = parse_line(parser);
  }

  if (result == parse_line_error)
  {
    LOG_ERROR("Parser failed!. Encounterd an error!");
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
statement_list_t *parser_getStatementList(parser_t *parser) { return parser->statementList; }

/**************************************************************************************************/
/**************************************************************************************************/
void parser_statementType_toString(statement_types_t type, char **buffer)
{
  switch (type)
  {
  case statement_undefined:
    *buffer = strdup_w("Undefined");
    break;

  case statement_label:
    *buffer = strdup_w("Label");
    break;

  case statement_directive:
    *buffer = strdup_w("Directive");
    break;

  case statement_instruction:
    *buffer = strdup_w("Instruction");
    break;
  default:
    LOG_ERROR("Invalid statement type! Abort");
    abort();
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
void parser_opcode_toString(opcode_t type, char **buffer)
{
  for (int i = 0; i < (int)(sizeof(identifiers) / sizeof(Identifier)); i++)
  {
    if ((identifiers[i].type == token_opcode) && (identifiers[i].identifier.opcode == type))
    {
      *buffer = strdup_w((char *)identifiers[i].name);
      return;
    }
  }
  LOG_ERROR("Invalid unsupported opcode given. Cant convert to string!");
  abort(); // Should not happen
}

/**************************************************************************************************/
/**************************************************************************************************/
void parser_register_toString(register_type_t type, char **buffer)
{
  for (int i = 0; i < (int)(sizeof(identifiers) / sizeof(Identifier)); i++)
  {
    if ((identifiers[i].type == token_register) && (identifiers[i].identifier.reg == type))
    {
      *buffer = strdup_w((char *)identifiers[i].name);
      return;
    }
  }
  LOG_ERROR("Invalid unsupported register given. Cant convert to string!");
  abort(); // Should not happen
}

/**************************************************************************************************/
/**************************************************************************************************/
void parser_directive_toString(directive_t *dir, char **buffer)
{
  char *directive_type;
  char operand_value[1024] = {0};

  switch (dir->type)
  {
  case directive_ORG:
    directive_type = "ORG";
    break;

  case directive_EXPORT:
    directive_type = "EXPORT";
    break;

  case directive_IMPORT:
    directive_type = "IMPORT";
    break;

  case directive_SECTION:
    directive_type = "SECTION";
    break;
  case directive_DB:
    directive_type = "DB";
    break;

  case directive_DW:
    directive_type = "DW";
    break;

  case directive_DS:
    directive_type = "DS";
    break;

  case directive_EQU:
    directive_type = "EQU";
    break;
  default:
    LOG_ERROR("Invalid directive type! Aborting!");
    abort(); // Should not happen
  }

  switch (dir->operand.type)
  {
  case operand_n:
    snprintf(operand_value, sizeof(operand_value), "%d", dir->operand.data.immediate_n);
    break;

  case operand_nn:
    snprintf(operand_value, sizeof(operand_value), "%d", dir->operand.data.immediate_nn);
    break;

  case operand_symbol:
    strncpy(operand_value, dir->operand.data.symbol.symbol, sizeof(operand_value) - 1);
    break;

  default:
    LOG_ERROR("Invalid operand type for directive! Aborting");
    abort(); // Should not happen
  }
  int len = snprintf(NULL, 0, "%s %s", directive_type, operand_value); // only get string lenght
  *buffer = calloc_w(1, len + 1);
  snprintf(*buffer, len + 1, "%s %s", directive_type, operand_value);
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
    parser->currentStatement.label.symbol = labelToken->data.label;

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

/**************************************************************************************************/
/**************************************************************************************************/
static void statement_free_callback(void *statementToFree)
{
  statement_t *statement = (statement_t *)statementToFree;

  if (statement->type == statement_label && statement->label.symbol)
  {
    free(statement->label.symbol);
  }
  else if (statement->type == statement_directive)
  {
    directive_free_callback(&statement->directive);
  }
  else if (statement->type == statement_instruction)
  {
    instruction_free_callback(&statement->instruction);
  }
}
