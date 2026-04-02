#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "lexer/token.h"
#include "parser/parser.h"
#include "types.h"
#include "utility/linked_list.h"

#include <stddef.h>

typedef ListNode token_list_node_t;

typedef enum
{
  parse_line_error,
  parse_line_next,
  parse_line_eof
} parse_line_result_t;

/**
 * @brief Struct representing the parser.
 *        The parser is responsible for converting a list of tokens
 *        into a list of instructions while checking for correct syntax
 *
 */
typedef struct parser
{
  statement_list_t *statementList;
  statement_t currentStatement; // Is filled while parsing a line
  size_t lineNumber;            // Count of processed lines

  token_list_node_t *currentTokenNode; // Current token node being worked on
} parser_t;

/**
 * @brief Get the currently processed token objet
 *
 * @param parser
 * @return token_t*
 */
token_t *get_token(parser_t *parser);

/**
 * @brief compare the currenlty processed token type with an expected token type
 *
 * @param parser
 * @param expectedType
 * @return true if match
 * @return false if not
 */
bool expect_token(parser_t *parser, token_types_t expectedType);

/**
 * @brief Adds the parsed statement to the statement list of the parser
 *
 * @param parser
 */
void emit_statement(parser_t *parser);

/**
 * @brief Move the parser to the next token in the list. return NULL if no further entry in list
 *
 * @param parser
 * @return token_t*
 */
token_t *consume_token(parser_t *parser);

#endif
