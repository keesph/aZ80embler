#include "utility/argument_parser.h"

#include "logging/logging.h"
#include "utility/linked_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *hex_format = "hex";
const char *obj_format = "obj";

static void print_usage()
{
  printf("Z80 assembler. Usage: \n");
  printf("az80embler [options] input_file\n");
  printf("Options:\n");
  printf("\t-h\tprint this text\n");
  printf("\t-v\tVerbose. Prints additional output\n");
  printf("\t-f=<fmt>,--format=<fmt>\tformat of the output file: [hex, obj] defaults to obj\n");
  printf("\t-o=<out>,--output=<out>\tName of the output file. Defaults to input_file.hex\n");
}

static void free_argument_callback(void *obj)
{
  argument_t *argument = (argument_t *)obj;

  if (argument->type == argument_parameter && argument->data.parameter.key == 'o')
  {
    // allocated output file name
    free(argument->data.parameter.value);
  }
  else if (argument->type == argument_positional && argument->data.positional)
  {
    // allocated argument string
    free(argument->data.positional);
  }
}

argument_list_t *get_argument_list(int count, char *argv[])
{
  bool has_dedicated_out = false;
  bool has_inputName = false;

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
    // Short flags/parameter
    if (*arg == '-' && *(arg + 1) != '-')
    {
      if (*(arg + 1) == '\0')
      {
        printf("Invalid command line argument. Leading '-' but nothing else!\n");
        print_usage();
        linkedList_destroy(list);
        return NULL;
      }
      arg++;
      // Short parameter
      if (*(arg + 1) == '=')
      {
        currentArgument.type = argument_parameter;
        if (*arg == 'f')
        {
          currentArgument.data.parameter.key = *arg;
          arg += 2; // Jump over '=' to parameter value
          if (strcmp((arg), hex_format) == 0)
          {
            currentArgument.data.parameter.value = (char *)hex_format;
            linkedList_append(list, &currentArgument);
          }
          else if (strcmp(arg, obj_format) == 0)
          {
            currentArgument.data.parameter.value = (char *)obj_format;
            linkedList_append(list, &currentArgument);
          }
          else
          {
            printf("Invalid command line argument %c.\n", *arg);
            print_usage();
            linkedList_destroy(list);
            return NULL;
          }
        }
        else if (*arg == 'o')
        {
          has_dedicated_out = true;
          currentArgument.data.parameter.key = *arg;
          arg += 2;                                                          // Jump over '=' to parameter value
          currentArgument.data.parameter.value = calloc(1, strlen(arg) + 1); // +1 for terminator
          if (!currentArgument.data.parameter.value)
          {
            LOG_ERROR("Failed to allocate parameter string!");
            linkedList_destroy(list);
            return NULL;
          }
          strcpy(currentArgument.data.parameter.value, arg);
          linkedList_append(list, &currentArgument);
        }
        else
        {
          printf("Invalid command line argument %c.\n", *argv[i]);
          print_usage();
          linkedList_destroy(list);
          return NULL;
        }
      }
      // Flag(s)
      else
      {
        currentArgument.type = argument_flag;

        while (*arg != '\0')
        {
          if (*arg == 'h' || *arg == 'v')
          {
            currentArgument.data.flag = *arg;
            linkedList_append(list, &currentArgument);
          }
          else
          {
            printf("Invalid command line argument: %c\n", *arg);
            print_usage();
            linkedList_destroy(list);
            return NULL;
          }
          arg++;
        }
      }
    }
    // Long parameter
    else if (*arg == '-' && *(arg + 1) == '-')
    {
      currentArgument.type = argument_parameter;
      arg += 2;
      char *value = strchr(arg, '='); // get pointer to = between key and value
      if (!value)
      {
        printf("Invalid command line argument!\n");
        print_usage();
        linkedList_destroy(list);
        return NULL;
      }
      *value = '\0'; // Separate key and value
      value++;       // Set pointer to start of value string
      if (strcmp(arg, "format") == 0)
      {
        currentArgument.data.parameter.key = 'f';
        if (strcmp(value, hex_format) == 0)
        {
          currentArgument.data.parameter.value = (char *)hex_format;
          linkedList_append(list, &currentArgument);
        }
        else if (strcmp(value, obj_format) == 0)
        {
          currentArgument.data.parameter.value = (char *)obj_format;
          linkedList_append(list, &currentArgument);
        }
        else
        {
          printf("Invalid command line argument!\n");
          print_usage();
          linkedList_destroy(list);
          return NULL;
        }
      }
      else if (strcmp(arg, "output") == 0)
      {
        has_dedicated_out = true;
        currentArgument.data.parameter.key = 'o';
        currentArgument.data.parameter.value = calloc(1, strlen(value) + 1); // +1 for terminator

        if (!currentArgument.data.parameter.value)
        {
          LOG_ERROR("Failed to allocate parameter string!");
          linkedList_destroy(list);
          return NULL;
        }
        strcpy(currentArgument.data.parameter.value, value);
        linkedList_append(list, &currentArgument);
      }
      else
      {
        printf("Invalid command line argument!\n");
        print_usage();
        linkedList_destroy(list);
        return NULL;
      }
    }
    // Positional input name
    else
    {
      has_inputName = true;

      // If no dedicated output file name was give, generate dummy equaling input file name
      if (!has_dedicated_out)
      {
        currentArgument.type = argument_parameter;
        currentArgument.data.parameter.key = 'o';
        currentArgument.data.parameter.value = calloc(1, strlen(arg) + 1); // +1 for terminator
        if (!currentArgument.data.parameter.value)
        {
          LOG_ERROR("Failed to allocate output file name string!");
          linkedList_destroy(list);
          return NULL;
        }
        strcpy(currentArgument.data.parameter.value, arg);
        linkedList_append(list, &currentArgument);
      }
      currentArgument.type = argument_positional;
      currentArgument.data.positional = calloc(1, strlen(arg) + 1); // +1 for terminator
      if (!currentArgument.data.positional)
      {
        LOG_ERROR("Failed to allocate output file name string!");
        linkedList_destroy(list);
        return NULL;
      }
      strcpy(currentArgument.data.positional, arg);
      linkedList_append(list, &currentArgument);
    }
  }
  if (!has_inputName)
  {
    printf("Missing Input file name!\n");
    print_usage();
    linkedList_destroy(list);
    return NULL;
  }
  return list;
}
