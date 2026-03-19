#include "parser.h"

#include "directive_types.h"
#include "lexer/token.h"
#include "logging/logging.h"
#include "opcode_types.h"
#include "parser/operand.h"
#include "parser/statement.h"
#include "parser/symbol.h"
#include "register_types.h"
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

  uint32_t currentLine;         // Count of processed lines
  statement_t currentStatement; // Is filled while parsing a line

  FILE *objectFile;
  symbol_list_t *internal_symbols;
  symbol_list_t *exported_symbols;
  symbol_list_t *imported_symbols;

} parser_state_t;

typedef enum
{
  parse_line_error,
  parse_line_next,
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

/**
 * @brief Parses tokens into a list of statements that hold all required information for assembling
 *        Checks for valid syntax, e.g. only valid opcode - operand combinations are parsed into instructions
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

// Compare the last emitted statement with the expected type.
// Needs to be checked for some statements that e.g. require a label in front of them
static bool based_on_statement(statement_types_t expectedType);

// Move to the next token in the list. return NULL if no further entry in list
static token_t *consume_token();

// Local functions used to parse tokens into directives and check valid syntax
static bool parse_directive();
static bool parse_directive_DB();
static bool parse_directive_DW();
static bool parse_directive_DS();
static bool parse_directive_EQU();
static bool parse_directive_ORG();
static bool parse_directive_EXPORT();
static bool parse_directive_IMPORT();
static bool parse_directive_SECTION();

// local functions used to parse tokens into instruction and check valid syntax
static bool parse_opcode_instruction();
static bool parse_opcode_LD();
static bool parse_opcode_PUSH_POP();
static bool parse_opcode_EX_EXX();
static bool parse_opcode_LDI_LDIR();
static bool parse_opcode_LDD_LDDR();
static bool parse_opcode_CPI_CPIR();
static bool parse_opcode_CPD_CPDR();
static bool parse_opcode_ADD_ADDC(); // 8 bit and 16 bit
static bool parse_opcode_SUB_SBC();
static bool parse_opcode_LOGIC(); // AND, OR, XOR
static bool parse_opcode_CP();
static bool parse_opcode_INC_DEC();
static bool parse_opcode_cpu_control();
static bool parse_opcode_rotate_shift();
static bool parse_opcode_BIT_SET_RES();
static bool parse_opcode_jump_group();
static bool parse_opcode_call_return_group();
static bool parse_opcode_io_group();
static bool register_is_r(register_type type);
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
  while (true)
  {
    if (!parse_line())
    {
      LOG_ERROR("Parser pass 1 failed!");
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
  // into statements that can be assembled and checks on valid syntax
  token_t *token;
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

  // Handle Label
  if (expect_token(token_label))
  {
    token_t *labelToken = get_token();
    m_parserState.currentStatement.type = statement_label;
    strcpy(&m_parserState.currentStatement.label.symbol[0], &labelToken->data.label[0]);

    consume_token();

    // Handle invalid eof
    if (expect_token(token_eof))
    {
      LOG_ERROR("[LINE: %d]: Unexpected end of file after token definition", m_parserState.currentLine);
      return parse_line_error;
    }

    // Handle line consisting only of a single label
    if (expect_token(token_eol))
    {
      emit_statement();
      return parse_line_next;
    }
  }

  // Handle directives that are not preceeded by label
  if (expect_token(token_directive))
  {
    if (!parse_directive())
    {
      return parse_line_error;
    }
    return parse_line_next;
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
static bool based_on_statement(statement_types_t expectedType)
{
  // Get the tail of the statement list and check if it even exists or if the list is empty
  ListNode *statementNode = linkedList_getLastNode(m_parserState.statementList);
  if (statementNode == NULL)
  {
    return false;
  }

  // Get the statement from the list node. Check against NULL even though it should not happen (TM).
  statement_t *lastStatement = listNode_getData(statementNode);
  if ((lastStatement == NULL) || (lastStatement->type != expectedType))
  {
    return false;
  }
  return true;
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
static bool parse_directive()
{
  bool result;

  // Get current token from list for processing, then consume from list
  token_t *token = get_token();
  consume_token();

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
  default:
    LOG_ERROR("[LINE: %d]: Invalid directive type", m_parserState.currentLine);
    return false;
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
  if (m_parserState.currentStatement.type != statement_label)
  {
    LOG_ERROR("[LINE: %d]: Tried to parse DW directive, but was not preceeded by label", m_parserState.currentLine);
    return false;
  }
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

      m_parserState.currentStatement.type = statement_labeled_directive;
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
  if (m_parserState.currentStatement.type != statement_label)
  {
    LOG_ERROR("[LINE: %d]: Tried to parse DS directive, but was not preceeded by label", m_parserState.currentLine);
    return false;
  }
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

  m_parserState.currentStatement.type = statement_labeled_directive;
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
  if (m_parserState.currentStatement.type != statement_label)
  {
    LOG_ERROR("[LINE: %d]: Tried to parse EQU directive, but was not preceeded by label", m_parserState.currentLine);
    return false;
  }
  directive_t *directive = &m_parserState.currentStatement.directive;
  if (!expect_token(token_literal_byte) && !expect_token(token_literal_word))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after EQU. Expected byte, word!", m_parserState.currentLine);
    return false;
  }

  token_t *token = get_token();

  directive->type = directive_EQU;
  if (token->type == token_literal_byte)
  {
    m_parserState.currentStatement.label.value = token->data.literal_byte;
  }
  else
  {
    m_parserState.currentStatement.label.value = token->data.literal_word;
  }

  m_parserState.currentStatement.type = statement_labeled_directive;
  emit_statement();
  consume_token();

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
  bool result = false;

  if (!expect_token(token_opcode))
  {
    LOG_ERROR("[LINE: %d]: Error when parsing token. Expected opcode!", m_parserState.currentLine);
    return false;
  }

  // Store current token for switch, then consume to point to 1. operand || EOL
  token_t *token = get_token();
  consume_token();

  instruction_t *instruction = &m_parserState.currentStatement.instruction;
  instruction->opcode = token->data.opcodeType;

  switch (token->data.opcodeType)
  {
  case opcode_LD:
    result = parse_opcode_LD();
    break;

  case opcode_PUSH:
  case opcode_POP:
    result = parse_opcode_PUSH_POP();
    break;

  case opcode_EX:
  case opcode_EXX:
    result = parse_opcode_EX_EXX();
    break;

  case opcode_LDI:
  case opcode_LDIR:
    result = parse_opcode_LDI_LDIR();
    break;

  case opcode_LDD:
  case opcode_LDDR:
    result = parse_opcode_LDD_LDDR();
    break;

  case opcode_CPI:
  case opcode_CPIR:
    result = parse_opcode_CPI_CPIR();
    break;

  case opcode_CPD:
  case opcode_CPDR:
    result = parse_opcode_CPD_CPDR();
    break;

  case opcode_ADD:
  case opcode_ADC:
    result = parse_opcode_ADD_ADDC(); // 8 bit and 16 bit
    break;

  case opcode_SUB:
  case opcode_SBC:
    result = parse_opcode_SUB_SBC(); // 8 bit and 16 bit
    break;

  case opcode_AND:
  case opcode_OR:
  case opcode_XOR:
    result = parse_opcode_LOGIC();
    break;

  case opcode_CP:
    result = parse_opcode_CP();
    break;

  case opcode_INC:
  case opcode_DEC:
    result = parse_opcode_INC_DEC(); // 8 bit and 16 bit
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
    result = parse_opcode_cpu_control();
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
    result = parse_opcode_rotate_shift();
    break;

  case opcode_BIT:
  case opcode_SET:
  case opcode_RES:
    result = parse_opcode_BIT_SET_RES();
    break;

  case opcode_JP:
  case opcode_JR:
  case opcode_DJNZ:
    result = parse_opcode_jump_group();
    break;

  case opcode_CALL:
  case opcode_RET:
  case opcode_RETI:
  case opcode_RETN:
  case opcode_RST:
    result = parse_opcode_call_return_group();
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
    result = parse_opcode_io_group();
    break;
  }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_LD()
{
  bool result = false;

  instruction_t *instruction = &m_parserState.currentStatement.instruction;
  token_t *token = get_token();

  // Parse tokens into instruction struct and do basic plausibility checks. Propper syntax checks are then done on
  // instruction struct

  // Operand is Register?
  if (token->type == token_register && register_is_r(token->data.registerType))
  {
    instruction->operand1.type = operand_r;
    instruction->operand1.data.r = token->data.registerType;
  }
  else if (token->type == token_rparenthesis)
  {
    consume_token();
    if (expect_token(token_literal_byte) || expect_token(token_literal_word))
    {
      instruction->operand1.type = operand_nn;
    }
  }
  else
  {
    LOG_ERROR("[LINE: %d]: Error when parsing operand1 of LD. Expected either register or lparenthesis!",
              m_parserState.currentLine);
    return false;
  }
  consume_token();
  if (!expect_token(token_eol) && !expect_token(token_eof))
  {
    LOG_ERROR("[LINE: %d]: Error when parsing LD. Expected to end with EOL or EOF!", m_parserState.currentLine);
    return false;
  }

  if (expect_token(token_eol) || expect_token(token_eof))
  {
    LOG_ERROR("[LINE: %d]: Invalid EOL or EOF after LD!", m_parserState.currentLine);
    return false;
  }

  // Check expected operand types and structure.
  token_t *operand1 = get_token();
  if (!expect_token(token_register) && !expect_token(token_lparenthesis))
  {
    LOG_ERROR("[LINE: %d]: Invalid operand1 for LD: %s, expected register or (", m_parserState.currentLine,
              token_toString(operand1->type));
    return false;
  }
  consume_token();
  if (operand1->type == token_lparenthesis)
  {
    consume_token();
    if (!expect_token(token_register) && !expect_token(token_literal_word) && !expect_token(token_literal_byte))
    {
      LOG_ERROR("[LINE: %d]: Invalid token after lparenthesis of operand1 for LD: %s, expected register or literal",
                m_parserState.currentLine, token_toString(operand1->type));
      return false;
    }
    consume_token();
    if (!expect_token(token_rparenthesis))
    {
      LOG_ERROR("[LINE: %d]: Invalid token after (operand1 for LD: %s, expected rparenthesis!",
                m_parserState.currentLine, token_toString(operand1->type));
      return false;
    }
  }
  consume_token();
  if (!expect_token(token_comma))
  {
    LOG_ERROR("[LINE: %d]: Invalid token after operand1 for LD: %s, expected comma", m_parserState.currentLine,
              token_toString(operand1->type));
    return false;
  }
  consume_token();
  token_t *operand2 = get_token();
  consume_token();

  if
    switch (token->type)
    {
    case token_register:
      switch (token->data.registerType)
      {
      case register_A:
      case register_B:
      case register_C:
      case register_D:
      case register_E:
      case register_H:
      case register_L:
      }
      break;

    case token_lparenthesis:

      break;

    default:
      LOG_ERROR("[LINE: %d]: Invalid token after LD opcode: %s", m_parserState.currentLine,
                token_toString(token->type));
      break;
    }

  return result;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_PUSH_POP() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_EX_EXX() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_LDI_LDIR() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_LDD_LDDR() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_CPI_CPIR() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_CPD_CPDR() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_ADD_ADDC() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_SUB_SBC() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_LOGIC() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_CP() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_INC_DEC() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_cpu_control() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_rotate_shift() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_BIT_SET_RES() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_jump_group() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_call_return_group() {}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parse_opcode_io_group() {}

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
