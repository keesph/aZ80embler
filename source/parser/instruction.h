#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "opcode_types.h"

#include "lexer/token.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_TOKENS_PER_OPERAND 5
#define OPCODE_MAX_LENGTH 4

typedef struct
{
  uint8_t machinecode[OPCODE_MAX_LENGTH];

  opcode_type opcode;
  Token operand1[MAX_TOKENS_PER_OPERAND];
  Token operand2[MAX_TOKENS_PER_OPERAND];

  bool relocatable;
  uint8_t relocate_offset;
} Instruction;

bool opcode_compareCb(void *a, void *b)
{
  Instruction *opA = (Instruction *)a;
  Instruction *opB = (Instruction *)b;

  bool same = true;
  same &= (opA->machinecode[0] == opB->machinecode[0]);
  same &= (opA->machinecode[1] == opB->machinecode[1]);
  same &= (opA->machinecode[2] == opB->machinecode[2]);
  same &= (opA->machinecode[3] == opB->machinecode[3]);
  same &= (opA->relocatable == opB->relocatable);
  same &= (opA->relocate_offset = opB->relocate_offset);
  return same;
}

#endif
