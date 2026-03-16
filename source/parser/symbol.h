#ifndef SYMBOL_H
#define SYMBOL_H

#include "lexer/token.h"
#include "utility/linked_list.h"

typedef LinkedList symbol_list_t;

typedef struct
{
  bool isResolved;
  char symbol[LABEL_MAX_LENGTH];
  uint16_t value;
} symbol_t;

#endif
