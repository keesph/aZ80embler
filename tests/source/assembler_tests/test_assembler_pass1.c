#include "unity.h"

#include "test_assembler.h"

#include "assembler/assembler.h"
#include "types.h"
#include "utility/linked_list.h"

#include <stdio.h>

static void test_pass_one(void)
{

  assembler_t assembler = {0};
  assembler_initialize(&assembler);
}

void test_assembler_pass1() { RUN_TEST(); }