#include "assembler/assembler.h"
#include "lexer/lexer.h"
#include "lexer/token.h"

#include "logging/logging.h"
#include "parser/operand.h"
#include "parser/parser.h"
#include "types.h"
#include "utility/argument_parser.h"
#include "utility/linked_list.h"

static void token_iterateCb(void *token, size_t iteration)
{
  char *string;
  token_toString(((token_t *)token)->type, &string);
  LOG_INFO("%d: %s", ++iteration, string);
  free(string);
}

static void statement_iterateCb(void *instance, size_t iteration)
{
  (void)iteration;
  char *statementString;
  char *directiveString;
  char *opcodeString;
  char *operand1String;
  char *operand2String;
  statement_t *statement = (statement_t *)instance;

  parser_statementType_toString(statement->type, &statementString);

  if (statement->type == statement_label)
  {
    LOG_INFO("[Statement]: %s\t%s:", statementString, statement->label.symbol);
  }
  else if (statement->type == statement_directive)
  {
    parser_directive_toString(&statement->directive, &directiveString);
    LOG_INFO("[Statement]: %s\t%s", statementString, directiveString);
    free(directiveString);
  }
  else if (statement->type == statement_instruction)
  {
    parser_opcode_toString(statement->instruction.opcode, &opcodeString);
    operand_toString(&statement->instruction.operand1, &operand1String);
    operand_toString(&statement->instruction.operand2, &operand2String);

    if (statement->label.symbol)
    {
      LOG_INFO("[Statement]: %s\t%s:\t%s\t%s, %s", statementString, statement->label.symbol, opcodeString,
               operand1String, operand2String);
    }
    else
    {
      LOG_INFO("[Statement]: %s\t\t%s\t%s, %s", statementString, opcodeString, operand1String, operand2String);
    }

    free(opcodeString);
    free(operand1String);
    free(operand2String);
  }
  else
  {
    LOG_ERROR("Undefined Statement!");
  }
  free(statementString);
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  bool verbose = true;

  assembler_t *assembler;
  assembler_initialize(assembler);

  FILE *sourceFile = sourceFile = fopen("testfile.txt", "r");

  if (!sourceFile)
  {
    LOG_ERROR("Failed to open input file. Aborting!");
    return -1;
  }

  if (!lexer_tokenize(assembler.lexer, sourceFile))
  {
    LOG_ERROR("Lexer failed! Aborting!");
    return -1;
  }

  token_list_t *list = lexer_getTokenList(assembler.lexer);
  if (verbose)
  {
    // Output all found tokens
    int count = linkedList_count(list);
    LOG_INFO("Tokenized File. Got %d tokens", count);
    linkedList_iterate(list, token_iterateCb);
  }

  if (!parser_do_it(assembler.parser, list))
  {
    LOG_ERROR("Parser failed! Aborting!");
    return -1;
  }
  statement_list_t *statementList = parser_getStatementList(assembler.parser);
  if (verbose)
  {
    LOG_INFO("Parsed File. Got %d statements", linkedList_count(statementList));
    linkedList_iterate(statementList, statement_iterateCb);
  }

  return 0;
}
