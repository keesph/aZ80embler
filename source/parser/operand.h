#ifndef OPERAND_H
#define OPERAND_H

#include "parser/parser.h"
#include "types.h"

#include <stdint.h>

operand_t operand_parse(parser_t *parser);

char *operand_toString(operand_type_t operand);
#endif
