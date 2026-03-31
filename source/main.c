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
  LOG_INFO("%d: %s", ++iteration, token_toString(((token_t *)token)->type));
}

static void statement_iterateCb(void *instance, size_t iteration)
{
  (void)iteration;
  statement_t *statement = (statement_t *)instance;
  const char *statementString = parser_statementType_toString(statement->type);

  if (statement->type == statement_label)
  {
    LOG_INFO("[Statement]: %s\t%s:", statementString, statement->label.symbol);
  }
  else if (statement->type == statement_directive)
  {
    LOG_INFO("[Statement]: %s\t%s", statementString, parser_directive_toString(&statement->directive));
  }
  else if (statement->type == statement_instruction)
  {
    const char *opcodeString = parser_opcode_toString(statement->instruction.opcode);
    const char *operand1String = operand_toString(statement->instruction.operand1.type);
    const char *operand2String = operand_toString(statement->instruction.operand2.type);

    if (statement->label.symbol)
    {
      LOG_INFO("[Statement]: %s\t%s:\t%s\t%s, %s", statementString, statement->label.symbol, opcodeString,
               operand1String, operand2String);
    }
    else
    {
      LOG_INFO("[Statement]: %s\t\t%s\t%s, %s", statementString, opcodeString, operand1String, operand2String);
    }
  }
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  bool verbose = true;

  assembler_t assembler = {0};
  // argument_list_t *arguments = get_argument_list(argc, argv);
  // if (!arguments)
  //{
  //   // Error already logged
  //   return -1;
  // }
  assembler.lexer = lexer_initialize();
  if (!assembler.lexer)
  {
    LOG_ERROR("Failed lexer initialization. Aborting!");
    return -1;
  }

  assembler.parser = parser_initialize();
  if (!assembler.parser)
  {
    LOG_ERROR("Failed parser initialization. Aborting!");
    return -1;
  }

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
