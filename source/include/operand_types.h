#ifndef OPERAND_TYPES_H
#define OPERAND_TYPES_H

typedef enum
{
  operand_r,        // Register A, B, C, D, E, E, H, L
  operand_deref_HL, // (HL)
  operand_deref_IX, // (IX+d)
  operand_deref_IY, // (IY+d)
  operand_n,        // 1 byte unsigned integer 0 - 255
  operand_nn,       // 2 byte unsigned integer 0 - 65535
  operand_d,        // one byte signed integer -128 - +127
  operand_b,        // one bit expression in the range 0 - 7 (7 = MSB, 0 = LSB)
  operand_e,  // one byte signed integer -126 - + 129 for relative jump offsets
  operand_cc, // Status in the Flag register (NZ, Z, NC, C, PO, PE, P, M)
  operand_qq, // Register Pairs BC, DE, HL, SP
  operand_ss, // Register Pairs BC, DE, IX or SP
  operand_rr, // Register Pairs BC, DE, IY or SP
  operand_s,  // any of r, n, (HL), (IX+d) or (IY+d)
  operand_m   // any of r, (HL), (IX+d) or (IY+d)
} operand_type;

#endif