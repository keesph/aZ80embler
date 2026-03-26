#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "parser/parser.h"

void instruction_free_callback(void *instructionToFree);

bool instruction_parse(parser_t *parser);

#endif
