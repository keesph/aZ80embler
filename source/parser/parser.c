#include "parser.h"

#include "directive_types.h"
#include "lexer/token.h"
#include "logging/logging.h"
#include "parser/instruction.h"
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
typedef ListNode TokenListNode;
typedef LinkedList IntermediateInstructions;
typedef struct
{
  IntermediateInstructions *InstructionList;
  uint32_t programCounter; // Current offset in the program where new
                           // instructions will be placed

  TokenList *inputTokenList;       // List of tokens to parse
  TokenListNode *currentTokenNode; // Current token node being worked on

  // Line parsing state
  Token tokenLine[MAX_TOKENS_PER_LINE]; // Store tokens of the current line
  uint8_t tokenLineCount;               // Count of tokens in the current line
  uint32_t currentLine;                 // Count of processed lines

  bool passOne_success;
  bool passTwo_success;

  FILE *objectFile;
  SymbolList *internal_symbols;
  SymbolList *exported_symbols;
  SymbolList *imported_symbols;

} parser_state;

/**************************************************************************************************/
// Static Data
/**************************************************************************************************/
static parser_state m_parserState;

/**************************************************************************************************/
// Static Function Declarations
/**************************************************************************************************/
static void pass_one();
static void pass_two();
static void initialize_lists();

// Copy the tokens of the next line into a buffer. Return the token of the
// following line when successful
static bool parseLine();

// Return true if the current token matches the expected type
static bool expectToken(token_type expectedType);

// Adds the current token to this lines token array
static void emitToken();

// Move to the next token in the list. return NULL if no further entry in list
static Token *consumeToken();

static bool readDirective(bool labled);
static bool readDB();
static bool readDW();
static bool readOperation();

/**************************************************************************************************/
// Public Function Definitions
/**************************************************************************************************/
FILE *parser_do_it(TokenList *tokenlist)
{
  assert(tokenlist);

  memset(&m_parserState, 0, sizeof(parser_state));

  initialize_lists();
  m_parserState.inputTokenList = tokenlist;
  m_parserState.currentTokenNode = linkedList_getFirstNode(tokenlist);
  m_parserState.currentLine = 1;

  pass_one();

  if (!m_parserState.passOne_success)
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

  linkedList_iterate(m_parserState.InstructionList, pass_two);
  if (!m_parserState.passTwo_success)
  {
    LOG_ERROR("Parser fialed second pass!");
    abort();
  }

  return NULL;
}

/**************************************************************************************************/
// Static Function Definitions
/**************************************************************************************************/
static void pass_one() {}

/**************************************************************************************************/
/**************************************************************************************************/
static void pass_two() {}

/**************************************************************************************************/
/**************************************************************************************************/
static void initialize_lists()
{
  m_parserState.InstructionList =
      linkedList_initialize(sizeof(Instruction), NULL, NULL);
  m_parserState.internal_symbols =
      linkedList_initialize(sizeof(Symbol), NULL, NULL);
  m_parserState.exported_symbols =
      linkedList_initialize(sizeof(Symbol), NULL, NULL);
  m_parserState.imported_symbols =
      linkedList_initialize(sizeof(Symbol), NULL, NULL);

  assert(m_parserState.InstructionList);
  assert(m_parserState.internal_symbols);
  assert(m_parserState.exported_symbols);
  assert(m_parserState.imported_symbols);
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool parseLine()
{
  // Similar to generating a lexeme in the lexer, this function groups tokens
  // into units that can be assembled
  Token *token;

  memset(&m_parserState.tokenLine[0], 0, sizeof(m_parserState.tokenLine));
  m_parserState.tokenLineCount = 0;

  // Handle unexpected token
  if (!expectToken(token_label) && !expectToken(token_opcode) &&
      !expectToken(token_directive) && !expectToken(token_eol) &&
      !expectToken(token_eof))
  {
    LOG_ERROR("[LINE: %d]: Line started with invalid token type: %s",
              m_parserState.currentLine, token_toString(token->type));
    return false;
  }

  // Handle EOL
  while (expectToken(token_eol))
  {
    token = consumeToken();
    if (token == NULL)
    {
      LOG_ERROR("[LINE: %d]: Parser reached end of list without EOF!",
                m_parserState.currentLine);
      return false;
    }
  }

  // Handle EOF
  if (expectToken(token_eof))
  {
    emitToken();
    return true;
  }

  // Handle directives that are not preceeded by label
  if (expectToken(token_directive))
  {
    return readDirective(false);
  }

  // Handle Label
  if (expectToken(token_label))
  {
    emitToken();

    // Line with only label?
    if (expectToken(token_eol))
    {
      return true;
    }

    // Handle invalid next token
    if (expectToken(token_eof))
    {
      LOG_ERROR("[LINE: %d]: Unexpected end of file after token definition");
      return false;
    }
  }

  // Handle directives preceeded by token
  if (expectToken(token_directive))
  {
    return readDirective(true);
  }

  // Handle Operation
  return readOperation();
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool expectToken(token_type expectedType)
{
  Token *token = listNode_getData(m_parserState.currentTokenNode);
  return (token->type == expectedType);
}

/**************************************************************************************************/
/**************************************************************************************************/
static void emitToken()
{
  if (m_parserState.tokenLineCount >= MAX_TOKENS_PER_LINE)
  {
    LOG_ERROR("[LINE: %d]: Line consists of more tokens than allowed!",
              m_parserState.currentLine);
    abort();
  }
  Token *token = listNode_getData(m_parserState.currentTokenNode);
  memcpy(&m_parserState.tokenLine[m_parserState.tokenLineCount++], token,
         sizeof(Token));
  consumeToken();
}

/**************************************************************************************************/
/**************************************************************************************************/
static Token *consumeToken()
{
  if (m_parserState.currentTokenNode == NULL)
  {
    LOG_ERROR("[LINE: %d]: Tried to consume a token, but there was none left!",
              m_parserState.currentLine);
    abort();
  }
  // Increment line count if EOL is consumed
  Token *token = listNode_getData(m_parserState.currentTokenNode);
  if (token == NULL)
  {
    LOG_ERROR("TokenNode did not contain any data! Aborting!");
    abort();
  }
  if (token->type == token_eol)
  {
    m_parserState.currentLine++;
  }

  m_parserState.currentTokenNode =
      listNode_getNext(m_parserState.currentTokenNode);

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
static bool readDirective(bool labled)
{
  bool result;
  Token *token = listNode_getData(m_parserState.currentTokenNode);

  emitToken();

  if (labled)
  {
    if (((token->data.directiveType != directive_DB)) &&
        (token->data.directiveType != directive_DW) &&
        (token->data.directiveType != directive_DS) &&
        (token->data.directiveType != directive_EQU))
    {
      LOG_ERROR("[LINE: %d]: Invalid  (labled) directive type",
                m_parserState.currentLine);
      return false;
    }
    if (token->data.directiveType == directive_DB)
    {
      consumeToken();
      return readDB();
    }
    else if (token->data.directiveType == directive_DW)
    {
      consumeToken();
      return readDW();
    }
    else if (token->data.directiveType == directive_DS)
    {
      consumeToken();
      if (!expectToken(token_string))
      {
        LOG_ERROR("[LINE: %d]: Invalid token after DS. Expected string!",
                  m_parserState.currentLine);
        return false;
      }
      emitToken();
      result = expectToken(token_eol);
      consumeToken();
      return result;
    }
    else // EQU
    {
      consumeToken();
      if (!(expectToken(token_literal_byte) || expectToken(token_literal_word)))
      {
        LOG_ERROR("[LINE: %d]: Invalid token after DS. Expected byte or word "
                  "literal!",
                  m_parserState.currentLine);
        return false;
      }
      emitToken();
      result = expectToken(token_eol);
      consumeToken();
      return result;
    }
  }
  return true;
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool readDB()
{
  if (!expectToken(token_literal_byte))
  {
    LOG_ERROR("[LINE: %d]: Invalid token when parsing directive DB. Expected "
              "byte literal",
              m_parserState.currentLine);
    return false;
  }
  emitToken();
  if (expectToken(token_eol) || expectToken(token_eof))
  {
    consumeToken();
    return true;
  }
  else if (!expectToken(token_comma))
  {
    LOG_ERROR(
        "[LINE: %d]: Invalid token when parsing directive DB. Expected comma",
        m_parserState.currentLine);
    return false;
  }
  emitToken();
  if (expectToken(token_literal_byte))
  {
    emitToken();
  }
  else
  {
    LOG_ERROR("[LINE: %d]: Expected literal after comma!",
              m_parserState.currentLine);
    abort();
  }

  if (expectToken(token_eol) || expectToken(token_eof))
  {
    consumeToken();
    return true;
  }
  else
  {
    return readDB();
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool readDW()
{
  if (!expectToken(token_label))
  {
    LOG_ERROR(
        "[LINE: %d]: Invalid token when parsing directive DW. Expected label",
        m_parserState.currentLine);
    return false;
  }
  emitToken();
  if (expectToken(token_eol) || expectToken(token_eof))
  {
    consumeToken();
    return true;
  }
  else if (!expectToken(token_comma))
  {
    LOG_ERROR(
        "[LINE: %d]: Invalid token when parsing directive DW. Expected comma",
        m_parserState.currentLine);
    return false;
  }
  emitToken();
  if (expectToken(token_literal_word))
  {
    emitToken();
  }
  else
  {
    LOG_ERROR("[LINE: %d]: Expected literal after comma!",
              m_parserState.currentLine);
    abort();
  }
  if (expectToken(token_eol) || expectToken(token_eof))
  {
    consumeToken();
    return true;
  }
  else
  {
    return readDB();
  }
}

/**************************************************************************************************/
/**************************************************************************************************/
static bool readOperation()
{
  if (expectToken(token_opcode))
  {
    emitToken();
  }
  else
  {
    LOG_ERROR(
        "[LINE: %d]: Invalid token while parsing statement! Expected opcode",
        m_parserState.currentLine);
  }

  if (expectToken(token_register) || expectToken(token_literal_byte) ||
      expectToken(token_literal_word) || expectToken(token_symbol))
  {
    emitToken();
  }
  else if (expectToken(token_lparenthesis))
  {
    emitToken();

    if (expectToken(token_register) || expectToken(token_literal_word) ||
        expectToken(token_symbol))
    {
      emitToken();
      if (expectToken(token_plus))
      {
        emitToken();
        if (expectToken(token_literal_sbyte) || expectToken(token_literal_byte))
        {
          emitToken();
        }
      }
    }
    if (expectToken(token_rparenthesis))
    {
      emitToken();
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing statement. Expected )");
      return false;
    }
  }
  else if (expectToken(token_comma))
  {
    emitToken();
  }
  else if (expectToken(token_eol) || expectToken(token_eof))
  {
    consumeToken();
    return true;
  }
  emitToken();

  // Second operand
  if (expectToken(token_register) || expectToken(token_literal_byte) ||
      expectToken(token_literal_word) || expectToken(token_symbol))
  {
    emitToken();
  }
  else if (expectToken(token_lparenthesis))
  {
    emitToken();

    if (expectToken(token_register) || expectToken(token_literal_word) ||
        expectToken(token_literal_word) || expectToken(token_symbol))
    {
      emitToken();
      if (expectToken(token_plus))
      {
        emitToken();
        if (expectToken(token_literal_sbyte) || expectToken(token_literal_byte))
        {
          emitToken();
        }
      }
    }
    if (expectToken(token_rparenthesis))
    {
      emitToken();
    }
    else
    {
      LOG_ERROR("[LINE: %d]: Invalid token when parsing statement. Expected )");
      return false;
    }
  }
  else if (expectToken(token_eol) || expectToken(token_eof))
  {
    consumeToken();
    return true;
  }
  emitToken();
  return true;
}
