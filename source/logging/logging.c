#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

static char *level_toString(log_level level)
{
  switch (level)
  {
  case level_info:
    return "INFO";
    break;

  case level_warning:
    return "WARNING";
    break;

  case level_error:
    return "ERROR";
    break;
  }
}

void log_message(const char *file, const char *function, int line,
                 log_level level, const char *message, ...)
{
  (void)file;
  va_list args;
  va_start(args, message);

  char *levelString = level_toString(level);
  printf("[%s] %s:%d -- ", levelString, function, line);
  vprintf(message, args);
  printf("\n");
  va_end(args);
}
