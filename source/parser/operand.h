#ifndef OPERAND_H
#define OPERAND_H

#include "parser/parser.h"
#include "types.h"

#include <stdint.h>

operand_t operand_parse(parser_t *parser);

void operand_toString(operand_t *operand, char **buffer);
#endif
