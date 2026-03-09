#ifndef PARSER_H
#define PARSER_H

#include "lexer/token_list.h"

#include <stdio.h>

FILE *parse_module(TokenList *list);

#endif
