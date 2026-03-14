#ifndef LOGGING_H
#define LOGGING_H

typedef enum
{
  level_info,
  level_warning,
  level_error
} log_level;

void log_message(const char *function, int line, log_level level,
                 const char *message, ...);

#define LOG_INFO(i, ...)                                                       \
  log_message(__func__, __LINE__, level_info, i __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING(w, ...)                                                    \
  log_message(__func__, __LINE__, level_warning, w __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(e, ...)                                                      \
  log_message(__func__, __LINE__, level_error, e __VA_OPT__(, ) __VA_ARGS__)

#endif
