#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "parser/parser.h"

bool pass_one(statement_list_t *statementList);
bool pass_two(statement_list_t *statementList, bool outputBinary);

#endif
