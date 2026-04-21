#ifndef TEST_PARSER_H
#define TEST_PARSER_H

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/parser_internal.h"

extern parser_t *parser;
extern lexer_state_t *lexer;
extern FILE *parser_test_file;

void test_arithmetic(void);
void test_BIT(void);
void test_call_return(void);
void test_EX(void);
void test_gparith_cpucontrol(void);
void test_input_output(void);
void test_jump(void);
void test_LD(void);
void test_push_pop(void);
void test_rotate_shift(void);

#endif