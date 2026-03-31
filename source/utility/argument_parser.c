#include "utility/argument_parser.h"

#include "logging/logging.h"
#include "utility/linked_list.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage()
{
  printf("Z80 assembler. Usage: \n");
  printf("az80embler [options] -i inputfile -o outputfile\n");
  printf("Options:\n");
  printf("\t-h\tprint this text\n");
  printf("\t-v\tVerbose. Prints additional output\n");
  printf("\t-f <fmt>\tformat of the output file: [hex, obj] defaults to obj\n");
  printf("\t-i <inp>\tname of the input file\n");
  printf("\t-o <out>\tname of the output file\n");
}

static argument_list_t *fail_parser(argument_list_t *list, char *error)
{
  LOG_ERROR(error);

  print_usage();
  linkedList_destroy(list);

  return NULL;
}

static void free_argument_callback(void *obj)
{
  argument_t *argument = (argument_t *)obj;

  if ((char *)argument->parameter)
  {
    free((char *)argument->parameter);
  }
}

argument_list_t *get_argument_list(int count, char *argv[])
{
  bool foundVerbose = false;
  bool foundFormat = false;
  bool foundInput = false;
  bool foundOutput = false;

  argument_list_t *list = linkedList_initialize(sizeof(argument_t), free_argument_callback, NULL);
  argument_t currentArgument;

  if (!list)
  {
    LOG_ERROR("Failed to allocate program argument list!");
    return NULL;
  }

  char *arg;
  for (int i = 1; i < count; i++) // start at one to ignore program name
  {
    arg = argv[i];
    memset(&currentArgument, 0, sizeof(argument_t));

    if (*arg != '-')
    {
      return fail_parser(list, "Invalid argument");
    }

    // Move to next character in argument
    arg++;

    if (*arg == 'h')
    {
      // Clear the list from any other found entries and append only a help entry to notify about the No-Op
      print_usage();
      linkedList_clear(list);
      currentArgument.type = argument_help;
      linkedList_append(list, &currentArgument);
      return list;
    }
    else if (*arg == 'v')
    {
      if (foundVerbose)
      {
        return fail_parser(list, "Multiple instances of -v is invalid!");
      }
      foundVerbose = true;

      currentArgument.type = argument_verbose;
      linkedList_append(list, &currentArgument);
    }
    else if (*arg == 'f')
    {
      if (foundFormat)
      {
        return fail_parser(list, "Invalid program parameters. Format -f specified more than once!");
      }
      foundFormat = true;
      currentArgument.type = argument_format;

      i++;

      // Check for out of bounds or already next argument without parameter first
      if (i >= count || *argv[i] == '-')
      {
        return fail_parser(list, "Invalid argument. Got -f but no parameter!");
      }

      arg = argv[i];

      currentArgument.parameter = calloc(1, strlen(arg) + 1);
      if (!currentArgument.parameter)
      {
        return fail_parser(list, "Failed to allocate parameter string");
      }

      strcpy(currentArgument.parameter, arg);

      linkedList_append(list, &currentArgument);
    }
    else if (*arg == 'i')
    {
      if (foundInput)
      {
        return fail_parser(list, "Invalid program parameters. -i specified more than once!");
      }
      foundInput = true;
      currentArgument.type = argument_input;

      i++;

      // Check for out of bounds or already next argument without parameter first
      if (i >= count || *argv[i] == '-')
      {
        return fail_parser(list, "Invalid argument. Got -i but no parameter!");
      }

      arg = argv[i];

      currentArgument.parameter = calloc(1, strlen(arg) + 1);
      if (!currentArgument.parameter)
      {
        return fail_parser(list, "Failed to allocate parameter string");
      }

      strcpy(currentArgument.parameter, arg);

      linkedList_append(list, &currentArgument);
    }
    else if (*arg == 'o')
    {
      if (foundOutput)
      {
        return fail_parser(list, "Invalid program parameters. -o specified more than once!");
      }
      foundOutput = true;
      currentArgument.type = argument_output;

      i++;

      // Check for out of bounds or already next argument without parameter first
      if (i >= count || *argv[i] == '-')
      {
        return fail_parser(list, "Invalid argument. Got -o but no parameter!");
      }

      arg = argv[i];

      currentArgument.parameter = calloc(1, strlen(arg) + 1);
      if (!currentArgument.parameter)
      {
        return fail_parser(list, "Failed to allocate parameter string");
      }

      strcpy(currentArgument.parameter, arg);

      linkedList_append(list, &currentArgument);
    }
    else
    {
      return fail_parser(list, "Invalid argument");
    }
  }
  if (!foundInput || !foundOutput)
  {
    return fail_parser(list, "Missing Input/Output file name(s)!");
  }
  return list;
}
