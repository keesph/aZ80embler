#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include "directive_types.h"
#include "lexer/token.h"
#include "opcode_types.h"
#include "register_types.h"

#include <stdint.h>
#include <strings.h>

typedef union
{
  opcode_type opcode;
  register_type reg;
  directive_type directive;
} identifier_type;

typedef struct
{
  const char *name;
  token_type type;
  identifier_type identifer;
} Identifier;

// clang-format off
static const Identifier identifiers[] = {
        // Name           //Token Type              //Identifier Type
    {"A",        token_register,   .identifer.reg=register_A},  
    {"B",        token_register,   .identifer.reg=register_B},  
    {"C",        token_register,   .identifer.reg=register_C},
    {"D",        token_register,   .identifer.reg=register_D},  
    {"E",        token_register,   .identifer.reg=register_E},  
    {"H",        token_register,   .identifer.reg=register_H},
    {"L",        token_register,   .identifer.reg=register_L},  
    {"BC",       token_register,   .identifer.reg=register_BC}, 
    {"DE",       token_register,   .identifer.reg=register_DE},
    {"HL",       token_register,   .identifer.reg=register_HL}, 
    {"AF",      token_register,   .identifer.reg=register_AF}, 
    {"IX",      token_register,   .identifer.reg=register_IX}, 
    {"IXH",      token_register,   .identifer.reg=register_IXH},
    {"IXL",      token_register,   .identifer.reg=register_IXL},
    {"IY",      token_register,   .identifer.reg=register_IY},
    {"IYH",      token_register,   .identifer.reg=register_IYH}, 
    {"IYL",      token_register,   .identifer.reg=register_IYL}, 
    {"SP",      token_register,   .identifer.reg=register_SP}, 
    {"I",      token_register,   .identifer.reg=register_I}, 
    {"R",      token_register,   .identifer.reg=register_R}, 
    {"LD",      token_opcode,     .identifer.opcode=opcode_LD},
    {"PUSH",    token_opcode,     .identifer.opcode=opcode_PUSH}, 
    {"POP",     token_opcode,     .identifer.opcode=opcode_POP},  
    {"EX",      token_opcode,     .identifer.opcode=opcode_EX},
    {"LDI",     token_opcode,     .identifer.opcode=opcode_LDI},  
    {"LDIR",    token_opcode,     .identifer.opcode=opcode_LDIR}, 
    {"LDD",     token_opcode,     .identifer.opcode=opcode_LDD},
    {"LDDR",    token_opcode,     .identifer.opcode=opcode_LDDR}, 
    {"CPI",     token_opcode,     .identifer.opcode=opcode_CPI},  
    {"CPIR",    token_opcode,     .identifer.opcode=opcode_CPIR},
    {"CPD",     token_opcode,     .identifer.opcode=opcode_CPD},    
    {"CPDR",    token_opcode,     .identifer.opcode=opcode_CPDR},
    {"ADD",     token_opcode,     .identifer.opcode=opcode_ADD},  
    {"ADC",     token_opcode,     .identifer.opcode=opcode_ADC},  
    {"SUB",     token_opcode,     .identifer.opcode=opcode_SUB},
    {"SUBC",    token_opcode,     .identifer.opcode=opcode_SUBC}, 
    {"AND",     token_opcode,     .identifer.opcode=opcode_AND},  
    {"OR",      token_opcode,     .identifer.opcode=opcode_OR},
    {"XOR",     token_opcode,     .identifer.opcode=opcode_XOR},  
    {"CP",      token_opcode,     .identifer.opcode=opcode_CP},   
    {"INC",     token_opcode,     .identifer.opcode=opcode_INC},
    {"DEC",     token_opcode,     .identifer.opcode=opcode_DEC},  
    {"DAA",     token_opcode,     .identifer.opcode=opcode_DAA},  
    {"CPL",     token_opcode,     .identifer.opcode=opcode_CPL},
    {"NEG",     token_opcode,     .identifer.opcode=opcode_NEG},  
    {"CCF",     token_opcode,     .identifer.opcode=opcode_CCF},  
    {"SCF",     token_opcode,     .identifer.opcode=opcode_SCF},
    {"NOP",     token_opcode,     .identifer.opcode=opcode_NOP},  
    {"HALT",    token_opcode,     .identifer.opcode=opcode_HALT}, 
    {"DI",      token_opcode,     .identifer.opcode=opcode_DI},
    {"EI",      token_opcode,     .identifer.opcode=opcode_EI},   
    {"IM",      token_opcode,     .identifer.opcode=opcode_IM},   
    {"RLCA",    token_opcode,     .identifer.opcode=opcode_RLCA},
    {"RLA",     token_opcode,     .identifer.opcode=opcode_RLA},  
    {"RRCA",    token_opcode,     .identifer.opcode=opcode_RRCA}, 
    {"RRA",     token_opcode,     .identifer.opcode=opcode_RRA},
    {"RLC",     token_opcode,     .identifer.opcode=opcode_RLC},
    {"RL",      token_opcode,     .identifer.opcode=opcode_RL},
    {"RRC",     token_opcode,     .identifer.opcode=opcode_RRC},
    {"RR",      token_opcode,     .identifer.opcode=opcode_RR},
    {"SLA",     token_opcode,     .identifer.opcode=opcode_SLA},
    {"SRA",     token_opcode,     .identifer.opcode=opcode_SRA},
    {"SRL",     token_opcode,     .identifer.opcode=opcode_SRL},
    {"RLD",     token_opcode,     .identifer.opcode=opcode_RLD},
    {"RRD",     token_opcode,     .identifer.opcode=opcode_RRD},
    {"BIT",     token_opcode,     .identifer.opcode=opcode_BIT},
    {"SET",     token_opcode,     .identifer.opcode=opcode_SET},
    {"RES",     token_opcode,     .identifer.opcode=opcode_RES},
    {"JP",      token_opcode,     .identifer.opcode=opcode_JP},
    {"JR",      token_opcode,     .identifer.opcode=opcode_JR},
    {"DJNZ",    token_opcode,     .identifer.opcode=opcode_DJNZ},
    {"CALL",    token_opcode,     .identifer.opcode=opcode_CALL},
    {"RET",     token_opcode,     .identifer.opcode=opcode_RET},
    {"RETI",    token_opcode,     .identifer.opcode=opcode_RETI},
    {"RETN",    token_opcode,     .identifer.opcode=opcode_RETN},
    {"RST",     token_opcode,     .identifer.opcode=opcode_RST},
    {"IN",      token_opcode,     .identifer.opcode=opcode_IN},
    {"INI",     token_opcode,     .identifer.opcode=opcode_INI},
    {"INIR",    token_opcode,     .identifer.opcode=opcode_INIR},
    {"IND",     token_opcode,     .identifer.opcode=opcode_IND},
    {"INDR",    token_opcode,     .identifer.opcode=opcode_INDR},
    {"OUT",     token_opcode,     .identifer.opcode=opcode_OUT},
    {"OUTI",    token_opcode,     .identifer.opcode=opcode_OUTI},
    {"OTIR",    token_opcode,     .identifer.opcode=opcode_OTIR},
    {"OUTD",    token_opcode,     .identifer.opcode=opcode_OUTD},
    {"OTDR",    token_opcode,     .identifer.opcode=opcode_OTDR},
    {"ORG",     token_directive,  .identifer.directive=directive_ORG},
    {"EXPORT",  token_directive,  .identifer.directive=directive_EXPORT},
    {"IMPORT",  token_directive,  .identifer.directive=directive_IMPORT},
    {"SECTION", token_directive,  .identifer.directive=directive_SECTION},
    {"DB",      token_directive,  .identifer.directive=directive_DB},
    {"DW",      token_directive,  .identifer.directive=directive_DW},
    {"DS",      token_directive,  .identifer.directive=directive_DS},
};

// clang-format on
#define IDENTIFIER_COUNT sizeof(identifiers) / sizeof(identifiers[0])
#endif
