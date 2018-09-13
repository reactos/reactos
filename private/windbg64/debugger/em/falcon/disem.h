/***                                                                  ***/
/***   INTEL CORPORATION PROPRIETARY INFORMATION                      ***/
/***                                                                  ***/
/***   This software is supplied under the terms of a license         ***/
/***   agreement or nondisclosure agreement with Intel Corporation    ***/
/***   and may not be copied or disclosed except in accordance with   ***/
/***   the terms of that agreement.                                   ***/
/***   Copyright (c) 1992,1993,1994,1995  Intel Corporation.          ***/
/***                                                                  ***/

#ifndef EM_DISASM_H
#define EM_DISASM_H

#include "decem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EM_DIS_NO_ERROR=0,
    EM_DIS_NULL_PTR,
    EM_DIS_SHORT_ASCII_INST_BUF,
    EM_DIS_SHORT_BIN_INST_BUF,
    EM_DIS_SHORT_SYMBOL_BUF,
    EM_DIS_UNALIGNED_INST,
    EM_DIS_NO_SYMBOL,
    EM_DIS_INVALID_SLOT_BRANCH_INST,
    EM_DIS_MUST_BE_GROUP_LAST,
    EM_DIS_INVALID_OPCODE,
    EM_DIS_IGNORED_OPCODE,
    EM_DIS_INVALID_SLOT,
    EM_DIS_REGISTER_VALUE_OUT_OF_RANGE,
    EM_DIS_BASE_EQUAL_DEST,
    EM_DIS_EQUAL_DESTS,
    EM_DIS_REGISTER_RESERVED_VALUE,
    EM_DIS_IMMEDIATE_VALUE_OUT_OF_RANGE,
    EM_DIS_IMMEDIATE_INVALID_VALUE,
    EM_DIS_STACK_FRAME_SIZE_OUT_OF_RANGE,
    EM_DIS_LOCALS_SIZE_LARGER_STACK_FRAME,
    EM_DIS_ROTATING_SIZE_LARGER_STACK_FRAME,
    EM_DIS_INVALID_TEMPLATE,
    EM_DIS_MISSING_STOP_BIT,
    EM_DIS_MISPLACED_INSTRUCTION,
    EM_DIS_FIRST_FATAL_ERROR,
    EM_DIS_INVALID_MACHINE_TYPE,
    EM_DIS_INVALID_MACHINE_MODE,
    EM_DIS_INVALID_RADIX,
    EM_DIS_INVALID_STYLE,
    EM_DIS_INTERNAL_ERROR,
    EM_DIS_LAST_ERROR
} EM_Dis_Err;

typedef enum
{
    EM_DIS_RADIX_NO_CHANGE=0,
    EM_DIS_RADIX_BINARY,
    EM_DIS_RADIX_OCTAL,
    EM_DIS_RADIX_DECIMAL,
    EM_DIS_RADIX_HEX,
    EM_DIS_RADIX_LAST
} EM_Dis_Radix;

typedef enum
{
    EM_DIS_STYLE_NO_CHANGE=0,
    EM_DIS_STYLE_USL,
    EM_DIS_STYLE_MASM,
    EM_DIS_STYLE_LAST
} EM_Dis_Style;



typedef enum
{
    EM_DIS_FIELD_NONE,
    EM_DIS_FIELD_ADDR_SYM,
    EM_DIS_FIELD_ADDR_PLUS,
    EM_DIS_FIELD_ADDR_OFFSET,
    EM_DIS_FIELD_ADDR_HEX,
    EM_DIS_FIELD_ADDR_COLON,
    EM_DIS_FIELD_DISP_SYM,
    EM_DIS_FIELD_DISP_PLUS,
    EM_DIS_FIELD_DISP_OFFSET,
    EM_DIS_FIELD_DISP_VALUE,
    EM_DIS_FIELD_MNEM,
    EM_DIS_FIELD_OPER,
    EM_DIS_FIELD_COMMA,
    EM_DIS_FIELD_REMARK,
    EM_DIS_FIELD_OPER_IMM,
    EM_DIS_FIELD_OPER_SEGOVR,
    EM_DIS_FIELD_OPER_COLON,
    EM_DIS_FIELD_OPER_OFS,
    EM_DIS_FIELD_OPER_SCALE,    
    EM_DIS_FIELD_OPER_INDEX,
    EM_DIS_FIELD_OPER_BASE,
    EM_DIS_FIELD_OPER_LPARENT,
    EM_DIS_FIELD_OPER_RPARENT,
    EM_DIS_FIELD_OPER_COMMA,

    EM_DIS_FIELD_PREG_LPARENT,
    EM_DIS_FIELD_PREG_RPARENT,
    EM_DIS_FIELD_PREG_REG,
    EM_DIS_FIELD_BUNDLE_START,
    EM_DIS_FIELD_BUNDLE_END,
    EM_DIS_FIELD_GROUP_SEPERATE,
    EM_DIS_FIELD_EQUAL,
    EM_DIS_FIELD_MEM_BASE,
    EM_DIS_FIELD_MEM_LPARENT,
    EM_DIS_FIELD_MEM_RPARENT,
    EM_DIS_FIELD_RF_LPARENT,
    EM_DIS_FIELD_RF_RPARENT,
    EM_DIS_FIELD_RF_REG_FILE,
    EM_DIS_FIELD_RF_INDEX,
    EM_DIS_FIELD_LESS,
    EM_DIS_FIELD_GREATER,
    EM_DIS_FIELD_VALUE,
    EM_DIS_FIELD_LAST
      
} EM_Dis_Field_Type;
      

typedef struct
{
    EM_Dis_Field_Type  type;
    char *             first;
    unsigned int       length;
} EM_Dis_Field;

typedef EM_Dis_Field    EM_Dis_Fields[120];

#ifdef WINNT
#define WIN_CDECL __cdecl
#else
#define WIN_CDECL
#endif

/*****************************************************/
/***   Disassembler Library Functions Prototypes   ***/
/*****************************************************/

EM_Dis_Err      WIN_CDECL em_dis_setup(const EM_Decoder_Machine_Type  type,
                                       const EM_Decoder_Machine_Mode  mode,
                                       const unsigned long            print_flags,
                                       const EM_Dis_Radix             radix,
                                       const EM_Dis_Style             style,
                                       EM_Dis_Err                    (*em_client_gen_sym)(U64 ,
                                                                      char *,
                                                                      int *,
                                                                      U64 *));

EM_Dis_Err      em_client_gen_sym(const U64         address,
                               char *           sym_buf,
                               unsigned int *   sym_buf_length,
                               U64 *            offset);

EM_Dis_Err      WIN_CDECL em_dis_inst(const U64 *                   syl_location,
                                      const EM_Decoder_Machine_Mode mode,
                                      const unsigned char *         bin_inst_buf,
                                      const unsigned int            bin_inst_buf_length,
                                      unsigned int *                actual_inst_length,
                                      char *                        ascii_inst_buf,
                                      unsigned int *                ascii_inst_buf_length,
                                      EM_Dis_Fields *               ascii_inst_fields);

EM_Dis_Err      WIN_CDECL em_dis_bundle(const U64 *                   location,
                                        const EM_Decoder_Machine_Mode mach_mode,
                                        const unsigned char *         bin_bundle_buf,
                                        const unsigned int            bin_bundle_buf_length,
                                        char *                        ascii_bundle_buf,
                                        unsigned int *                ascii_bundle_buf_length,
                                        EM_Dis_Fields *               ascii_bundle_fields);

const char* WIN_CDECL em_dis_err_str(EM_Dis_Err error);

void  WIN_CDECL em_dis_get_version(EM_library_version_t *dis_version);

/**********************************************/
/***   Setup Macros                         ***/
/**********************************************/

#define     EM_DIS_RADIX_DEFAULT     EM_DIS_RADIX_HEX
#define     EM_DIS_STYLE_DEFAULT     EM_DIS_STYLE_USL

#define EM_DIS_FLAG_ALIAS_CR         0x00000001
#define EM_DIS_FLAG_ALIAS_AR         0x00000002
#define EM_DIS_FLAG_ALIAS_R          0x00000004
#define EM_DIS_FLAG_ALIAS_F          0x00000008
#define EM_DIS_FLAG_ALIAS_B          0x00000010
#define EM_DIS_FLAG_C_STYLE_RADIX    0x00000100
#define EM_DIS_FLAG_EXCLUDE_ADDRESS  0x00001000
#define EM_DIS_FLAG_EXCLUDE_BUNDLE   0x00002000
#define EM_DIS_FLAG_EXCLUDE_GROUP    0x00004000
#define EM_DIS_FLAG_PRINT_PREDICATE  0x00008000
#define EM_DIS_FLAG_REG_ALIASES  (EM_DIS_FLAG_ALIAS_CR | EM_DIS_FLAG_ALIAS_AR | \
                                  EM_DIS_FLAG_ALIAS_R  | EM_DIS_FLAG_ALIAS_F  | \
                                  EM_DIS_FLAG_ALIAS_B)
#define EM_DIS_FLAG_DEFAULT          EM_DIS_FLAG_REG_ALIASES
#define EM_DIS_FLAG_EXCLUDE_ADDRESS_CALLBACK 0x00010000
#define EM_DIS_FLAG_NO_CHANGE        0x80000000

#define EM_DIS_FUNC_NO_CHANGE        (NULL)

/***********************************/
/***   Enviroment Setup Macros   ***/
/***********************************/

#define     EM_DIS_SET_MACHINE_TYPE(type)                                              \
{                                                                                      \
   em_dis_setup((type), EM_DECODER_CPU_NO_CHANGE, EM_DIS_FLAG_NO_CHANGE,               \
             EM_DIS_RADIX_NO_CHANGE, EM_DIS_STYLE_NO_CHANGE, EM_DIS_FUNC_NO_CHANGE);   \
}

#define     EM_DIS_SET_MACHINE_MODE(mode)                                              \
{                                                                                      \
   em_dis_setup(EM_DECODER_MODE_NO_CHANGE, (mode), EM_DIS_FLAG_NO_CHANGE,              \
             EM_DIS_RADIX_NO_CHANGE, EM_DIS_STYLE_NO_CHANGE, EM_DIS_FUNC_NO_CHANGE);   \
}

#define     EM_DIS_SET_PRINT_FLAGS(flags)                                              \
{                                                                                      \
   em_dis_setup(EM_DECODER_MODE_NO_CHANGE, EM_DECODER_CPU_NO_CHANGE, (flags),          \
             EM_DIS_RADIX_NO_CHANGE, EM_DIS_STYLE_NO_CHANGE, EM_DIS_FUNC_NO_CHANGE);   \
}

#define     EM_DIS_SET_RADIX(radix)                                                    \
{                                                                                      \
   em_dis_setup(EM_DECODER_MODE_NO_CHANGE, EM_DECODER_CPU_NO_CHANGE,                   \
             EM_DIS_FLAG_NO_CHANGE, (radix), EM_DIS_STYLE_NO_CHANGE,                   \
             EM_DIS_FUNC_NO_CHANGE);                                                   \
}

#define     EM_DIS_SET_STYLE(style)                                                    \
{                                                                                      \
   em_dis_setup(EM_DECODER_MODE_NO_CHANGE, EM_DECODER_CPU_NO_CHANGE,                   \
             EM_DIS_FLAG_NO_CHANGE, EM_DIS_RADIX_NO_CHANGE, (style),                   \
             EM_DIS_FUNC_NO_CHANGE);                                                   \
}

#define     EM_DIS_SET_CLIENT_FUNC(client_gen_sym)                                     \
{                                                                                      \
   em_dis_setup(EM_DECODER_MODE_NO_CHANGE, EM_DECODER_CPU_NO_CHANGE,                   \
             EM_DIS_FLAG_NO_CHANGE, EM_DIS_RADIX_NO_CHANGE, EM_DIS_STYLE_NO_CHANGE,    \
             (client_gen_sym));                                                        \
}

#define EM_DIS_NEXT(Addr, Size)         \
{                                       \
   U32 syl_size;                        \
   IEL_CONVERT1(syl_size, (Size));      \
   IEL_ADDU((Addr), syl_size, (Addr));  \
}

#define EM_DIS_EM_NEXT(Addr,SlotSize)                   \
{                                                       \
   int slot = EM_IL_GET_SLOT_NO(Addr);                  \
   switch (slot)                                        \
   {                                                    \
     case 0:                                            \
       IEL_INCU(Addr);                                  \
       break;                                           \
     case 1:                                            \
       IEL_INCU(Addr);                                  \
       if ((SlotSize) != 2)                             \
       {                                                \
           break;                                       \
       } /*** else fall-through ***/                    \
     case 2:                                            \
     {                                                  \
         U32 syl_size;                                  \
         IEL_CONVERT1(syl_size, EM_BUNDLE_SIZE-2);      \
         IEL_ADDU((Addr), syl_size, (Addr));            \
     }                                                  \
       break;                                           \
   }                                                    \
}

#ifdef __cplusplus
}
#endif

#endif  /*EM_DISASM_H*/













