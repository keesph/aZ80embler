#ifndef DIRECTIVE_TYPES_H
#define DIRECTIVE_TYPES_H

typedef enum
{
  directive_ORG,
  directive_EXPORT,
  directive_IMPORT,
  directive_SECTION,
  directive_DB,
  directive_DW,
  directive_DS,
  directive_EQU
} directive_type;

#endif
