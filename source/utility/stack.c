#include "stack.h"
#include "logging/logging.h"
#include "utility/alloc_w.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct stack
{
  void *stackArray;
  size_t dataSize;
  size_t stackSize;
  size_t count;
  unsigned char *stackPointer;
} stack_t;

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
stack_t *stack_initialize(size_t dataSize, size_t stackSize)
{
  if (dataSize == 0 || stackSize == 0)
  {
    LOG_ERROR("Given size for stack initialization was 0");
    return NULL;
  }

  stack_t *newStack = malloc_w(sizeof(stack_t));
  newStack->stackArray = calloc_w(stackSize, dataSize);
  newStack->count = 0;
  newStack->dataSize = dataSize;
  newStack->stackSize = stackSize;
  newStack->stackPointer = newStack->stackArray;

  return newStack;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
void stack_destroy(stack_t *stack)
{
  assert(stack);

  free(stack->stackArray);
  free(stack);
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
bool stack_push(stack_t *stack, void *data)
{
  assert(stack);
  assert(data);

  if (stack->count == stack->stackSize)
  {
    LOG_ERROR("Tried to push to full stack!");
    return false;
  }

  memcpy(stack->stackPointer, data, stack->dataSize);
  stack->stackPointer += stack->dataSize;
  stack->count++;

  return true;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
bool stack_pop(stack_t *stack, void *data)
{
  assert(stack);
  assert(data);

  if (stack->count == 0)
  {
    LOG_ERROR("Tried to pop from empty stack");
    return false;
  }

  stack->stackPointer -= stack->dataSize;
  memcpy(data, stack->stackPointer, stack->dataSize);
  stack->count--;

  return true;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
bool stack_hasData(stack_t *stack)
{
  assert(stack);
  return stack->count > 0;
}