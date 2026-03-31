#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"
#include "types.h"

typedef struct parser parser_t;
typedef LinkedList statement_list_t;

/**
 * @brief Initializes the parser state object
 *
 * @return parser_t*
 */
parser_t *parser_initialize();

/**
 * @brief Converts a given list of tokens into a list of intermediate instruction representations
 *
 * @param parser
 * @param tokenlist
 * @return true if success, false if not
 */
bool parser_do_it(parser_t *parser, token_list_t *list);

/**
 * @brief Returns the result of the parsing process
 *
 * @param parser
 * @return statement_list_t*
 */
statement_list_t *parser_getStatementList(parser_t *parser);

/**
 * @brief Returns the string representation of the given statement type
 *
 * @param type
 * @return char*
 */
const char *parser_statementType_toString(statement_types_t type);

/**
 * @brief Returns the string representation of the given opcode
 *
 * @param type
 * @return char*
 */
const char *parser_opcode_toString(opcode_t type);

/**
 * @brief Returns the string representation of the given directive
 *
 * @param type
 * @return char*
 */
char *parser_directive_toString(directive_t *dir);
#endif
