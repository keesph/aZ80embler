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
  identifier_type identifier;
} Identifier;

// clang-format off
static const Identifier identifiers[] = {
        // Name           //Token Type              //Identifier Type
    {"A",        token_register,   .identifier.reg=register_A},  
    {"B",        token_register,   .identifier.reg=register_B},  
    {"C",        token_register,   .identifier.reg=register_C},
    {"D",        token_register,   .identifier.reg=register_D},  
    {"E",        token_register,   .identifier.reg=register_E},  
    {"H",        token_register,   .identifier.reg=register_H},
    {"L",        token_register,   .identifier.reg=register_L},  
    {"BC",       token_register,   .identifier.reg=register_BC}, 
    {"DE",       token_register,   .identifier.reg=register_DE},
    {"HL",       token_register,   .identifier.reg=register_HL}, 
    {"AF",      token_register,   .identifier.reg=register_AF}, 
    {"IX",      token_register,   .identifier.reg=register_IX}, 
    {"IXH",      token_register,   .identifier.reg=register_IXH},
    {"IXL",      token_register,   .identifier.reg=register_IXL},
    {"IY",      token_register,   .identifier.reg=register_IY},
    {"IYH",      token_register,   .identifier.reg=register_IYH}, 
    {"IYL",      token_register,   .identifier.reg=register_IYL}, 
    {"SP",      token_register,   .identifier.reg=register_SP}, 
    {"I",      token_register,   .identifier.reg=register_I}, 
    {"R",      token_register,   .identifier.reg=register_R}, 
    {"LD",      token_opcode,     .identifier.opcode=opcode_LD},
    {"PUSH",    token_opcode,     .identifier.opcode=opcode_PUSH}, 
    {"POP",     token_opcode,     .identifier.opcode=opcode_POP},  
    {"EX",      token_opcode,     .identifier.opcode=opcode_EX},
    {"LDI",     token_opcode,     .identifier.opcode=opcode_LDI},  
    {"LDIR",    token_opcode,     .identifier.opcode=opcode_LDIR}, 
    {"LDD",     token_opcode,     .identifier.opcode=opcode_LDD},
    {"LDDR",    token_opcode,     .identifier.opcode=opcode_LDDR}, 
    {"CPI",     token_opcode,     .identifier.opcode=opcode_CPI},  
    {"CPIR",    token_opcode,     .identifier.opcode=opcode_CPIR},
    {"CPD",     token_opcode,     .identifier.opcode=opcode_CPD},    
    {"CPDR",    token_opcode,     .identifier.opcode=opcode_CPDR},
    {"ADD",     token_opcode,     .identifier.opcode=opcode_ADD},  
    {"ADC",     token_opcode,     .identifier.opcode=opcode_ADC},  
    {"SUB",     token_opcode,     .identifier.opcode=opcode_SUB},
    {"SUBC",    token_opcode,     .identifier.opcode=opcode_SUBC}, 
    {"AND",     token_opcode,     .identifier.opcode=opcode_AND},  
    {"OR",      token_opcode,     .identifier.opcode=opcode_OR},
    {"XOR",     token_opcode,     .identifier.opcode=opcode_XOR},  
    {"CP",      token_opcode,     .identifier.opcode=opcode_CP},   
    {"INC",     token_opcode,     .identifier.opcode=opcode_INC},
    {"DEC",     token_opcode,     .identifier.opcode=opcode_DEC},  
    {"DAA",     token_opcode,     .identifier.opcode=opcode_DAA},  
    {"CPL",     token_opcode,     .identifier.opcode=opcode_CPL},
    {"NEG",     token_opcode,     .identifier.opcode=opcode_NEG},  
    {"CCF",     token_opcode,     .identifier.opcode=opcode_CCF},  
    {"SCF",     token_opcode,     .identifier.opcode=opcode_SCF},
    {"NOP",     token_opcode,     .identifier.opcode=opcode_NOP},  
    {"HALT",    token_opcode,     .identifier.opcode=opcode_HALT}, 
    {"DI",      token_opcode,     .identifier.opcode=opcode_DI},
    {"EI",      token_opcode,     .identifier.opcode=opcode_EI},   
    {"IM",      token_opcode,     .identifier.opcode=opcode_IM},   
    {"RLCA",    token_opcode,     .identifier.opcode=opcode_RLCA},
    {"RLA",     token_opcode,     .identifier.opcode=opcode_RLA},  
    {"RRCA",    token_opcode,     .identifier.opcode=opcode_RRCA}, 
    {"RRA",     token_opcode,     .identifier.opcode=opcode_RRA},
    {"RLC",     token_opcode,     .identifier.opcode=opcode_RLC},
    {"RL",      token_opcode,     .identifier.opcode=opcode_RL},
    {"RRC",     token_opcode,     .identifier.opcode=opcode_RRC},
    {"RR",      token_opcode,     .identifier.opcode=opcode_RR},
    {"SLA",     token_opcode,     .identifier.opcode=opcode_SLA},
    {"SRA",     token_opcode,     .identifier.opcode=opcode_SRA},
    {"SRL",     token_opcode,     .identifier.opcode=opcode_SRL},
    {"RLD",     token_opcode,     .identifier.opcode=opcode_RLD},
    {"RRD",     token_opcode,     .identifier.opcode=opcode_RRD},
    {"BIT",     token_opcode,     .identifier.opcode=opcode_BIT},
    {"SET",     token_opcode,     .identifier.opcode=opcode_SET},
    {"RES",     token_opcode,     .identifier.opcode=opcode_RES},
    {"JP",      token_opcode,     .identifier.opcode=opcode_JP},
    {"JR",      token_opcode,     .identifier.opcode=opcode_JR},
    {"DJNZ",    token_opcode,     .identifier.opcode=opcode_DJNZ},
    {"CALL",    token_opcode,     .identifier.opcode=opcode_CALL},
    {"RET",     token_opcode,     .identifier.opcode=opcode_RET},
    {"RETI",    token_opcode,     .identifier.opcode=opcode_RETI},
    {"RETN",    token_opcode,     .identifier.opcode=opcode_RETN},
    {"RST",     token_opcode,     .identifier.opcode=opcode_RST},
    {"IN",      token_opcode,     .identifier.opcode=opcode_IN},
    {"INI",     token_opcode,     .identifier.opcode=opcode_INI},
    {"INIR",    token_opcode,     .identifier.opcode=opcode_INIR},
    {"IND",     token_opcode,     .identifier.opcode=opcode_IND},
    {"INDR",    token_opcode,     .identifier.opcode=opcode_INDR},
    {"OUT",     token_opcode,     .identifier.opcode=opcode_OUT},
    {"OUTI",    token_opcode,     .identifier.opcode=opcode_OUTI},
    {"OTIR",    token_opcode,     .identifier.opcode=opcode_OTIR},
    {"OUTD",    token_opcode,     .identifier.opcode=opcode_OUTD},
    {"OTDR",    token_opcode,     .identifier.opcode=opcode_OTDR},
    {"ORG",     token_directive,  .identifier.directive=directive_ORG},
    {"EXPORT",  token_directive,  .identifier.directive=directive_EXPORT},
    {"IMPORT",  token_directive,  .identifier.directive=directive_IMPORT},
    {"SECTION", token_directive,  .identifier.directive=directive_SECTION},
    {"DB",      token_directive,  .identifier.directive=directive_DB},
    {"DW",      token_directive,  .identifier.directive=directive_DW},
    {"DS",      token_directive,  .identifier.directive=directive_DS},
};

// clang-format on
#define IDENTIFIER_COUNT sizeof(identifiers) / sizeof(identifiers[0])
#endif
