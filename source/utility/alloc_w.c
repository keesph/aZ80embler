#include "utility/alloc_w.h"

#include "logging/logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *calloc_w(size_t count, size_t size)
{
  void *new_p = calloc(count, size);
  if (!new_p)
  {
    LOG_ERROR("Failed to allocate memory (%zu bytes)! Aborting", (size * count));
    abort();
  }
  return new_p;
}

void *malloc_w(size_t size)
{
  void *new_p = malloc(size);
  if (!new_p)
  {
    LOG_ERROR("Failed to allocate memory(%zu bytes)! Aborting", size);
    abort();
  }
  return new_p;
}

void *strdup_w(char *string)
{
  void *new_p = strdup(string);
  if (!new_p)
  {
    LOG_ERROR("Failed to duplicate string (length %zu)! Aborting!", strlen(string));
    abort();
  }
  return new_p;
}

int asprintf_w(char **buffer, char *format, ...)
{
  va_list args, argsCopy;

  va_start(args, format);
  va_copy(argsCopy, args);
  int len = vsnprintf(NULL, 0, format, args);
  if (len < 0)
  {
    LOG_ERROR("Failed to get length of formatted string! Invalid, aborting!");
    abort();
  }
  va_end(args);

  *buffer = calloc_w(1, len + 1);
  vsnprintf(*buffer, len + 1, format, argsCopy);

  va_end(argsCopy);
  return len;
}
