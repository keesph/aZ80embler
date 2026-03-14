#ifndef SYMBOL_H
#define SYMBOL_H

#include "lexer/token.h"
#include "utility/linked_list.h"

typedef LinkedList SymbolList;

typedef struct
{
  char symbol[LABEL_MAX_LENGTH];
  uint16_t address;
} Symbol;

#endif