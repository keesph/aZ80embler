#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stddef.h>

typedef struct stack stack_t;

// Note: Does only do shallow copies of pushed data. No ownership is transferred

stack_t *stack_initialize(size_t dataSize, size_t size);
void stack_destroy(stack_t *stack);

bool stack_push(stack_t *stack, void *data);
bool stack_pop(stack_t *stack, void *data);

bool stack_hasData(stack_t *stack);

#endif
