#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "utility/linked_list.h"

typedef LinkedList argument_list_t;

extern const char *hex_format;
extern const char *obj_format;

typedef enum
{
  argument_help,
  argument_verbose,
  argument_format,
  argument_input,
  argument_output
} argument_type_t;

typedef struct
{
  argument_type_t type;
  char *parameter;
} argument_t;

argument_list_t *get_argument_list(int count, char *argv[]);

#endif
