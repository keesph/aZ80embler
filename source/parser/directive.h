#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#include "parser/parser.h"

void directive_free_callback(void *directiveToFree);

bool directive_parse(parser_t *parser);

#endif
