#include "parser.h"

#include "directive_types.h"
#include "lexer/token.h"
#include "logging/logging.h"
#include "parser/operand.h"
#include "parser/statement.h"
#include "parser/symbol.h"
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
// Type definitions
/**************************************************************************************************/
typedef ListNode token_list_node_t;
typedef LinkedList statement_list_t;

typedef struct
{
  statement_list_t *statementList;
  uint32_t programCounter; // Current offset in the program where new
                           // instructions will be placed

  token_list_t *inputTokenList;        // List of tokens to parse
  token_list_node_t *currentTokenNode; // Current token node being worked on
  // Line parsing state
  token_t tokenLine[MAX_TOKENS_PER_LINE]; // Store tokens of the current line
  uint8_t tokenLineCount;                 // Count of tokens in the current line
  uint32_t currentLine;                   // Count of processed lines
  statement_t currentStatement;           // Is filled while parsing a line

  FILE *objectFile;
  symbol_list_t *internal_symbols;
  symbol_list_t *exported_symbols;
  symbol_list_t *imported_symbols;

} parser_state_t;

typedef enum
{
  parse_line_error,
  parse_line_has_more,
  parse_line_eof
} parse_line_result_t;

/**************************************************************************************************/
// Static Data
/**************************************************************************************************/
static parser_state_t m_parserState;

/**************************************************************************************************/
// Static Function Declarations
/**************************************************************************************************/
static bool pass_one();
static void pass_two();
static void initialize_lists();

// Copy the tokens of the next line into a buffer. Return the token of the
// following line when successful
/**
 * @brief Copy the tokens of the next line into a buffer. Checks for valid
 * syntax
 *
 * @return true
 * @return false
 */
static parse_line_result_t parse_line();

// Return the currently processed token
static token_t *get_token();

// Return true if the current token matches the expected type
static bool expect_token(token_types_t expectedType);

// Adds the current statement to the statement list
static void emit_statement();

// Move to the next token in the list. return NULL if no further entry in list
static token_t *consume_token();

static bool parse_directive(bool labled);
static bool parse_directive_DB();
static bool parse_directive_DW();
static bool parse_directive_DS();
static bool parse_directive_EQU();
static bool parse_directive_ORG();
static bool parse_directive_EXPORT();
static bool parse_directive_IMPORT();
static bool parse_directive_SECTION();
static bool parse_instruction();

/**************************************************************************************************/
// Public Function Definitions
/**************************************************************************************************/
FILE *parser_do_it(token_list_t *tokenlist)
{
  assert(tokenlist);

  memset(&m_parserState, 0, sizeof(parser_state_t));

  initialize_lists();
  m_parserState.inputTokenList = tokenlist;
  m_parserState.currentTokenNode = linkedList_getFirstNode(tokenlist);
  m_parserState.currentLine = 1;

  if (!pass_one())
  {
    LOG_ERROR("Parser failed first pass!");
    abort();
  }

  // Open File and truncate it if already existing
  char *dummyName = "dummy.o";
  m_parserState.objectFile = fopen(dummyName, "w");
  if (!m_parserState.objectFile)
  {
    LOG_ERROR("Could not open Object File %s for writing", dummyName);
    abort();
  }

  return NULL;
}

/**************************************************************************************************/
// Static Function Definitions
/**************************************************************************************************/
static bool pass_one()
{
  uint8_t tokenCount = 0;
  while (true)
  {
    if (!parse_line())
    {
      LOG_ERROR("Parser pass 1 failed!");
      return false;
    }

    // Check for EOF
    if (m_parserState.tokenLine[tokenCount].type == token_eof)
    {
      break;
    }

    switch (m_parserState.tokenLine[tokenCount].type)
    {
    case token_directive:

      break;
    case token_label:

      break;

    case token_opcode:

      break;
    default:
      LOG_ERROR("Invalid token at start of line in Pass One!");
      return false;
    }
  }

  LOG_INFO("Finished Pass 1. Parsed %d lines.", m_parserState.currentLine);
  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static void pass_two() {}

/**************************************************************************************************/
/**************************************************************************************************/
static void initialize_lists()
{
  m_parserState.statementList = linkedList_initialize(sizeof(instruction_t), NULL, NULL);
  m_parserState.internal_symbols = linkedList_initialize(sizeof(symbol_t), NULL, NULL);
  m_parserState.exported_symbols = linkedList_initialize(sizeof(symbol_t), NULL, NULL);
  m_parserState.imported_symbols = linkedList_initialize(sizeof(symbol_t), NULL, NULL);

  assert(m_parserState.statementList);
  assert(m_parserState.internal_symbols);
  assert(m_parserState.exported_symbols);
  assert(m_parserState.imported_symbols);
}

/**************************************************************************************************/
/**************************************************************************************************/
static parse_line_result_t parse_line()
{
  // Similar to generating a lexeme in the lexer, this function groups tokens
  // into units that can be assembled and checks on valid syntax
  token_t *token;

  memset(&m_parserState.tokenLine[0], 0, sizeof(m_parserState.tokenLine));
  m_parserState.tokenLineCount = 0;
  memset(&m_parserState.currentStatement, 0, sizeof(statement_t));

  // Handle unexpected token
  if (!expect_token(token_label) && !expect_token(token_opcode) && !expect_token(token_directive) &&
      !expect_token(token_eol) && !expect_token(token_eof))
  {
    LOG_ERROR("[LINE: %d]: Line started with invalid token type!", m_parserState.currentLine);
    return parse_line_error;
  }

  // Handle EOL
  while (expect_token(token_eol))
  {
    token = consume_token();
    if (token == NULL)
    {
      LOG_ERROR("[LINE: %d]: Parser reached end of list without EOF!", m_parserState.currentLine);
      return parse_line_error;
    }
  }

  // Handle EOF
  if (expect_token(token_eof))
  {
    consume_token();
    return parse_line_eof;
  }

  // Handle directives that are not preceeded by label
  if (expect_token(token_directive))
  {
    if (!parse_directive(false))
    {
      return parse_line_error;
    }
    return parse_line_has_more;
  }

  // Handle Label
  if (expect_token(token_label))
  {
    symbol_t *label = &m_parserState.currentStatement.label;
    m_parserState.currentStatement.type = statement_label;

    emit_token();

    // Line with only label?
    if (expect_token(token_eol))
    {
      return true;
    }

    // Handle invalid next token
    if (expect_token(token_eof))
    {
      LOG_ERROR("[LINE: %d]: Unexpected end of file after token definition");
      return false;
    }
  }

  // Handle directives preceeded by token
  if (expect_token(token_directive))
  {
    return parse_directive(true);
  }

  // Handle Operation
  return parse_instruction();
}

/**************************************************************************************************/
/**************************************************************************************************/
static token_t *get_token() { return listNode_getData(m_parserState.currentTokenNode); }

/**************************************************************************************************/
/**************************************************************************************************/
static bool expect_token(token_types_t expectedType)
{
  token_t *token = listNode_getData(m_parserState.currentTokenNode);
  return (token->type == expectedType);
}

/**************************************************************************************************/
/**************************************************************************************************/
static void emit_statement()
{
  linkedList_append(m_parserState.statementList, &m_parserState.currentStatement);
  memset(&m_parserState.currentStatement, 0, sizeof(statement_t));
}

/**************************************************************************************************/
/**************************************************************************************************/
static token_t *consume_token()
{
  if (m_parserState.currentTokenNode == NULL)
  {
    LOG_ERROR("[LINE: %d]: Tried to consume a token, but there was none left!", m_parserState.currentLine);
    abort();
  }
  // Increment line count if EOL is consumed
  token_t *token = listNode_getData(m_parserState.currentTokenNode);
  if (token == NULL)
  {
    LOG_ERROR("TokenNode did not contain any data! Aborting!");
    abort();
  }
  if (token->type == token_eol)
  {
    m_parserState.currentLine++;
  }

  m_parserState.currentTokenNode = listNode_getNext(m_parserState.currentTokenNode);

  if (m_parserState.currentTokenNode == NULL)
  {
    return NULL;
  }
  else
  {
    return listNode_getData(m_parserState.currentTokenNode);
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive(bool labled)
{
  bool result;

  // Get current token from list for processing, then consume from list
  token_t *token = get_token();
  consume_token();

  if (labled)
  {
    m_parserState.currentStatement.type = statement_labeled_directive;
    switch (token->data.directiveType)
    {
    case directive_DB:
      result = parse_directive_DB();
      break;

    case directive_DW:
      result = parse_directive_DW();
      break;

    case directive_DS:
      result = parse_directive_DS();
      break;

    case directive_EQU:
      result = parse_directive_EQU();
      break;
    default:
      LOG_ERROR("[LINE: %d]: Invalid  (labled) directive type", m_parserState.currentLine);
      return false;
    }
  }
  else
  {
    m_parserState.currentStatement.type = statement_directive;
    switch (token->data.directiveType)
    {
    case directive_ORG:
      result = parse_directive_ORG();
      break;

    case directive_EXPORT:
      result = parse_directive_EXPORT();
      break;

    case directive_IMPORT:
      result = parse_directive_IMPORT();
      break;

    case directive_SECTION:
      result = parse_directive_SECTION();
      break;

    case directive_DB:
      result = parse_directive_DB();
      break;

    default:
      LOG_ERROR("[LINE: %d]: Invalid  (unlabled) directive type", m_parserState.currentLine);
      return false;
    }
  }
  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_DB()
{
  if (!expect_token(token_literal_byte))
  {
    LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected "
              "byte literal",
              m_parserState.currentLine);
    return false;
  }

  directive_t *directive = &m_parserState.currentStatement.directive;

  directive->type = directive_DB;
  directive->operand.type = operand_n;
  directive->operand.data.immediate_n = get_token()->data.literal_byte;
  m_parserState.currentStatement.size = 1;

  emit_statement();
  consume_token();

  while (!expect_token(token_eol) && !expect_token(token_eof))
  {
    if (!expect_token(token_comma))
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected comma", m_parserState.currentLine);
      return false;
    }

    // Jump over COMMA token and check if next is as expected
    consume_token();
    if (expect_token(token_literal_byte))
    {
      directive->type = directive_DB;
      directive->operand.type = operand_n;
      directive->operand.data.immediate_n = get_token()->data.literal_byte;
      m_parserState.currentStatement.type = statement_directive;
      m_parserState.currentStatement.size = 1;
      emit_statement();
      consume_token();
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Expected literal after comma!", m_parserState.currentLine);
      abort();
    }
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_DW()
{
  if (!expect_token(token_literal_word) && !expect_token(token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DW. Expected "
              "word literal or symbol",
              m_parserState.currentLine);
    return false;
  }

  directive_t *directive = &m_parserState.currentStatement.directive;

  directive->type = directive_DB;
  if (expect_token(token_symbol))
  {
    directive->operand.type = operand_symbol;
    memcpy(&directive->operand.data.symbol.symbol, get_token()->data.symbol, LABEL_MAX_LENGTH);
  }
  else
  {
    directive->operand.type = operand_nn;
    directive->operand.data.immediate_nn = get_token()->data.literal_word;
  }
  m_parserState.currentStatement.size = 2;
  emit_statement();
  consume_token();

  while (!expect_token(token_eol) && !expect_token(token_eof))
  {
    if (!expect_token(token_comma))
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected comma", m_parserState.currentLine);
      return false;
    }

    // Jump over COMMA token and check if next is as expected
    consume_token();
    if (!expect_token(token_literal_word) && !expect_token(token_symbol))
    {
      if (expect_token(token_symbol))
      {
        directive->operand.type = operand_symbol;
        memcpy(&directive->operand.data.symbol.symbol, get_token()->data.symbol, LABEL_MAX_LENGTH);
      }
      else
      {
        directive->operand.type = operand_nn;
        directive->operand.data.immediate_nn = get_token()->data.literal_word;
      }

      m_parserState.currentStatement.type = statement_directive;
      m_parserState.currentStatement.size = 2;
      emit_statement();
      consume_token();
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Expected literal or symbol after comma!", m_parserState.currentLine);
      abort();
    }
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_DS()
{
  directive_t *directive = &m_parserState.currentStatement.directive;
  bool result;

  consume_token(); // consume "
  if (!expect_token(token_string))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after DS. Expected string!", m_parserState.currentLine);
    return false;
  }
  directive->type = directive_DS;
  directive->operand.type = operand_string;
  strcpy(&directive->operand.data.string_literal[0], &get_token()->data.string[0]);

  m_parserState.currentStatement.size = strlen(directive->operand.data.string_literal);
  emit_statement();
  consume_token(); // consume "
  result = expect_token(token_eol);
  consume_token();

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_EQU()
{
  directive_t *directive = &m_parserState.currentStatement.directive;
  if (!expect_token(token_literal_byte) && !expect_token(token_literal_word))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EQU. Expected byte, word!", m_parserState.currentLine);
    return false;
  }

  token_t *token = get_token();

  directive->type = directive_EQU;
  switch (token->type)
  {
  case token_literal_byte:
    directive->operand.type = operand_n;
    directive->operand.data.immediate_n = token->data.literal_byte;
    m_parserState.currentStatement.size = 1;
    break;
  case token_literal_word:
    directive->operand.type = operand_nn;
    directive->operand.data.immediate_nn = token->data.literal_word;
    m_parserState.currentStatement.size = 2;
    break;
  default:
    // nothing to do. already checked above
    break;
  }
  emit_statement();
  if (!expect_token(token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EQU. expected EOL!", m_parserState.currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_ORG()
{
  directive_t *directive = &m_parserState.currentStatement.directive;

  if (!expect_token(token_literal_byte) && !expect_token(token_literal_word))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after ORG. Expected byte or word!", m_parserState.currentLine);
    return false;
  }
  directive->type = directive_ORG;
  if (expect_token(token_literal_byte))
  {
    directive->operand.type = operand_n;
    directive->operand.data.immediate_n = get_token()->data.literal_byte;
  }
  else
  {
    directive->operand.type = operand_nn;
    directive->operand.data.immediate_nn = get_token()->data.literal_word;
  }

  m_parserState.currentStatement.size = 0; // directive does not result in memory usage
  emit_statement();
  consume_token();
  if (!expect_token(token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after ORG. expected EOL!", m_parserState.currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_EXPORT()
{
  directive_t *directive = &m_parserState.currentStatement.directive;

  if (!expect_token(token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EXPORT. Expected symbol!", m_parserState.currentLine);
    return false;
  }
  directive->type = directive_EXPORT;
  directive->operand.type = operand_symbol;
  strcpy(&directive->operand.data.symbol.symbol[0], &get_token()->data.symbol[0]);
  m_parserState.currentStatement.size = 0; // directive does not result in memory usage
  emit_statement();
  consume_token();
  if (!expect_token(token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EXPORT. expected EOL!", m_parserState.currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_IMPORT()
{
  directive_t *directive = &m_parserState.currentStatement.directive;

  if (!expect_token(token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after IMPORT. Expected symbol!", m_parserState.currentLine);
    return false;
  }
  directive->type = directive_IMPORT;
  directive->operand.type = operand_symbol;
  strcpy(&directive->operand.data.symbol.symbol[0], &get_token()->data.symbol[0]);
  m_parserState.currentStatement.size = 0; // directive does not result in memory usage
  emit_statement();
  consume_token();
  if (!expect_token(token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after IMPORT. expected EOL!", m_parserState.currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_directive_SECTION()
{
  directive_t *directive = &m_parserState.currentStatement.directive;

  if (!expect_token(token_symbol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after SECTION. Expected symbol!", m_parserState.currentLine);
    return false;
  }
  directive->type = directive_SECTION;
  directive->operand.type = operand_symbol;
  strcpy(&directive->operand.data.symbol.symbol[0], &get_token()->data.symbol[0]);
  m_parserState.currentStatement.size = 0; // directive does not result in memory usage
  emit_statement();
  consume_token();
  if (!expect_token(token_eol))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after SECTION. expected EOL!", m_parserState.currentLine);
    return false;
  }

  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_instruction()
{
  instruction_t *instruction = &m_parserState.currentStatement.instruction;
  if (expect_token(token_opcode))
  {
    instruction->opcode = get_token()->data.opcodeType;
    consume_token();
  }
  else
  {
    LOG_ERROR("[LINE: %d]: Invalid token while parsing statement! Expected opcode", m_parserState.currentLine);
  }

  if (expect_token(token_register) || expect_token(token_literal_byte) || expect_token(token_literal_word) ||
      expect_token(token_symbol))
  {
    emit_token();
  }
  else if (expect_token(token_lparenthesis))
  {
    emit_token();

    if (expect_token(token_register) || expect_token(token_literal_word) || expect_token(token_symbol))
    {
      emit_token();
      if (expect_token(token_plus))
      {
        emit_token();
        if (expect_token(token_literal_sbyte) || expect_token(token_literal_byte))
        {
          emit_token();
        }
      }
    }
    if (expect_token(token_rparenthesis))
    {
      emit_token();
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing statement. Expected )");
      return false;
    }
  }
  else if (expect_token(token_comma))
  {
    emit_token();
  }
  else if (expect_token(token_eol) || expect_token(token_eof))
  {
    consume_token();
    return true;
  }
  emit_token();

  // Second operand
  if (expect_token(token_register) || expect_token(token_literal_byte) || expect_token(token_literal_word) ||
      expect_token(token_symbol))
  {
    emit_token();
  }
  else if (expect_token(token_lparenthesis))
  {
    emit_token();

    if (expect_token(token_register) || expect_token(token_literal_word) || expect_token(token_literal_word) ||
        expect_token(token_symbol))
    {
      emit_token();
      if (expect_token(token_plus))
      {
        emit_token();
        if (expect_token(token_literal_sbyte) || expect_token(token_literal_byte))
        {
          emit_token();
        }
      }
    }
    if (expect_token(token_rparenthesis))
    {
      emit_token();
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing statement. Expected )");
      return false;
    }
  }
  else if (expect_token(token_eol) || expect_token(token_eof))
  {
    consume_token();
    return true;
  }
  emit_token();
  return true;
}
