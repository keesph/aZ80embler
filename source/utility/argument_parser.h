#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "utility/linked_list.h"

typedef LinkedList argument_list_t;

extern const char *hex_format;
extern const char *obj_format;

typedef enum
{
  argument_flag,
  argument_parameter,
  argument_positional
} argument_type_t;

typedef struct
{
  char key;
  char *value;
} parameter_t;

typedef union
{
  char flag;
  parameter_t parameter;
  char *positional;
} argument_data_t;

typedef struct
{
  argument_type_t type;
  argument_data_t data;
} argument_t;

argument_list_t *get_argument_list(int count, char *argv[]);

#endif
