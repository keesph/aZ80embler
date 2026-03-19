#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H

typedef enum
{
  // General Purpose Single
  register_A,
  register_B,
  register_C,
  register_D,
  register_E,
  register_H,
  register_L,

  // General Purpose Double
  register_BC,
  register_DE,
  register_HL,
  register_AF,

  // Special Purpose
  register_IX,
  register_IXH,
  register_IXL,
  register_IY,
  register_IYH,
  register_IYL,
  register_SP,
  register_I,
  register_R,

  // Flag Registers (C is already defined as normal register)
  register_NZ,
  register_Z,
  register_NC,
  register_PO,
  register_PE,
  register_P,
  register_M
} register_type;

#endif
