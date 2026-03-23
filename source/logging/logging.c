#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

static char *level_toString(log_level_t level)
{
  switch (level)
  {
  case level_info:
    return "INFO";
    break;

  case level_error:
    return "ERROR";
    break;
  }
}

void log_program_message(const char *function, int line, log_level_t level, const char *message, ...)
{
  va_list args;
  va_start(args, message);

  char *levelString = level_toString(level);
  printf("[%s] %s:%d -- ", levelString, function, line);
  vprintf(message, args);
  printf("\n");
  va_end(args);
}

static char *processing_toString(processing_message_t type)
{
  switch (type)
  {
  case lexer_error:
    return "LEXER ERROR";
    break;

  case syntax_error:
    return "SYNTAX ERROR";
    break;

  case operand_error:
    return "OEPRAND_ERROR";
    break;
  }
}

void log_source_error(size_t sourceLine, processing_message_t type, const char *message, ...)
{
  va_list args;
  va_start(args, message);

  char *processingString = processing_toString(type);
  printf("[%s - LINE %zu]: ", processingString, sourceLine);
  vprintf(message, args);
  printf("\n");
  va_end(args);
}
