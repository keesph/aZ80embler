#ifndef LOGGING_H
#define LOGGING_H

#include <stddef.h>

typedef enum
{
  level_info,
  level_error
} log_level_t;

// Used for logging events related to program execution
void log_program_message(const char *function, int line, log_level_t level, const char *message, ...);

#define LOG_INFO(i, ...) log_program_message(__func__, __LINE__, level_info, i __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(e, ...) log_program_message(__func__, __LINE__, level_error, e __VA_OPT__(, ) __VA_ARGS__)

typedef enum
{
  lexer_error,
  syntax_error,
  operand_error,
  assembler_error,
} processing_message_t;

// Used for logging errors in the processed source code
bool log_source_error(size_t sourceLine, processing_message_t type, const char *message, ...);

#define LOG_LEXER_ERROR(state, msg, ...)                                                                               \
  log_source_error((state)->lineNumber, lexer_error, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOG_SYNTAX_ERROR(state, msg, ...)                                                                              \
  log_source_error((state)->lineNumber, syntax_error, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOG_OPERAND_ERROR(state, msg, ...)                                                                             \
  log_source_error((state)->lineNumber, operand_error, msg __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ASSEMBLER_ERROR(state, msg, ...)                                                                           \
  log_source_error((state)->lineNumber, assembler_error, msg __VA_OPT__(, ) __VA_ARGS__)

#endif
