#ifndef OPERAND_H
#define OPERAND_H

#include "parser/parser.h"
#include "types.h"

#include <stdint.h>

operand_t operand_parse(parser_t *parser);

#endif
