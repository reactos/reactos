/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "tgsi_info.h"

static const struct tgsi_opcode_info opcode_info[TGSI_OPCODE_LAST] =
{
   { 1, 1, 0, 0, 0, 0, "ARL", TGSI_OPCODE_ARL },
   { 1, 1, 0, 0, 0, 0, "MOV", TGSI_OPCODE_MOV },
   { 1, 1, 0, 0, 0, 0, "LIT", TGSI_OPCODE_LIT },
   { 1, 1, 0, 0, 0, 0, "RCP", TGSI_OPCODE_RCP },
   { 1, 1, 0, 0, 0, 0, "RSQ", TGSI_OPCODE_RSQ },
   { 1, 1, 0, 0, 0, 0, "EXP", TGSI_OPCODE_EXP },
   { 1, 1, 0, 0, 0, 0, "LOG", TGSI_OPCODE_LOG },
   { 1, 2, 0, 0, 0, 0, "MUL", TGSI_OPCODE_MUL },
   { 1, 2, 0, 0, 0, 0, "ADD", TGSI_OPCODE_ADD },
   { 1, 2, 0, 0, 0, 0, "DP3", TGSI_OPCODE_DP3 },
   { 1, 2, 0, 0, 0, 0, "DP4", TGSI_OPCODE_DP4 },
   { 1, 2, 0, 0, 0, 0, "DST", TGSI_OPCODE_DST },
   { 1, 2, 0, 0, 0, 0, "MIN", TGSI_OPCODE_MIN },
   { 1, 2, 0, 0, 0, 0, "MAX", TGSI_OPCODE_MAX },
   { 1, 2, 0, 0, 0, 0, "SLT", TGSI_OPCODE_SLT },
   { 1, 2, 0, 0, 0, 0, "SGE", TGSI_OPCODE_SGE },
   { 1, 3, 0, 0, 0, 0, "MAD", TGSI_OPCODE_MAD },
   { 1, 2, 0, 0, 0, 0, "SUB", TGSI_OPCODE_SUB },
   { 1, 3, 0, 0, 0, 0, "LRP", TGSI_OPCODE_LRP },
   { 1, 3, 0, 0, 0, 0, "CND", TGSI_OPCODE_CND },
   { 0, 0, 0, 0, 0, 0, "", 20 },      /* removed */
   { 1, 3, 0, 0, 0, 0, "DP2A", TGSI_OPCODE_DP2A },
   { 0, 0, 0, 0, 0, 0, "", 22 },      /* removed */
   { 0, 0, 0, 0, 0, 0, "", 23 },      /* removed */
   { 1, 1, 0, 0, 0, 0, "FRC", TGSI_OPCODE_FRC },
   { 1, 3, 0, 0, 0, 0, "CLAMP", TGSI_OPCODE_CLAMP },
   { 1, 1, 0, 0, 0, 0, "FLR", TGSI_OPCODE_FLR },
   { 1, 1, 0, 0, 0, 0, "ROUND", TGSI_OPCODE_ROUND },
   { 1, 1, 0, 0, 0, 0, "EX2", TGSI_OPCODE_EX2 },
   { 1, 1, 0, 0, 0, 0, "LG2", TGSI_OPCODE_LG2 },
   { 1, 2, 0, 0, 0, 0, "POW", TGSI_OPCODE_POW },
   { 1, 2, 0, 0, 0, 0, "XPD", TGSI_OPCODE_XPD },
   { 0, 0, 0, 0, 0, 0, "", 32 },      /* removed */
   { 1, 1, 0, 0, 0, 0, "ABS", TGSI_OPCODE_ABS },
   { 1, 1, 0, 0, 0, 0, "RCC", TGSI_OPCODE_RCC },
   { 1, 2, 0, 0, 0, 0, "DPH", TGSI_OPCODE_DPH },
   { 1, 1, 0, 0, 0, 0, "COS", TGSI_OPCODE_COS },
   { 1, 1, 0, 0, 0, 0, "DDX", TGSI_OPCODE_DDX },
   { 1, 1, 0, 0, 0, 0, "DDY", TGSI_OPCODE_DDY },
   { 0, 0, 0, 0, 0, 0, "KILP", TGSI_OPCODE_KILP },
   { 1, 1, 0, 0, 0, 0, "PK2H", TGSI_OPCODE_PK2H },
   { 1, 1, 0, 0, 0, 0, "PK2US", TGSI_OPCODE_PK2US },
   { 1, 1, 0, 0, 0, 0, "PK4B", TGSI_OPCODE_PK4B },
   { 1, 1, 0, 0, 0, 0, "PK4UB", TGSI_OPCODE_PK4UB },
   { 1, 2, 0, 0, 0, 0, "RFL", TGSI_OPCODE_RFL },
   { 1, 2, 0, 0, 0, 0, "SEQ", TGSI_OPCODE_SEQ },
   { 1, 2, 0, 0, 0, 0, "SFL", TGSI_OPCODE_SFL },
   { 1, 2, 0, 0, 0, 0, "SGT", TGSI_OPCODE_SGT },
   { 1, 1, 0, 0, 0, 0, "SIN", TGSI_OPCODE_SIN },
   { 1, 2, 0, 0, 0, 0, "SLE", TGSI_OPCODE_SLE },
   { 1, 2, 0, 0, 0, 0, "SNE", TGSI_OPCODE_SNE },
   { 1, 2, 0, 0, 0, 0, "STR", TGSI_OPCODE_STR },
   { 1, 2, 1, 0, 0, 0, "TEX", TGSI_OPCODE_TEX },
   { 1, 4, 1, 0, 0, 0, "TXD", TGSI_OPCODE_TXD },
   { 1, 2, 1, 0, 0, 0, "TXP", TGSI_OPCODE_TXP },
   { 1, 1, 0, 0, 0, 0, "UP2H", TGSI_OPCODE_UP2H },
   { 1, 1, 0, 0, 0, 0, "UP2US", TGSI_OPCODE_UP2US },
   { 1, 1, 0, 0, 0, 0, "UP4B", TGSI_OPCODE_UP4B },
   { 1, 1, 0, 0, 0, 0, "UP4UB", TGSI_OPCODE_UP4UB },
   { 1, 3, 0, 0, 0, 0, "X2D", TGSI_OPCODE_X2D },
   { 1, 1, 0, 0, 0, 0, "ARA", TGSI_OPCODE_ARA },
   { 1, 1, 0, 0, 0, 0, "ARR", TGSI_OPCODE_ARR },
   { 0, 1, 0, 0, 0, 0, "BRA", TGSI_OPCODE_BRA },
   { 0, 0, 0, 1, 0, 0, "CAL", TGSI_OPCODE_CAL },
   { 0, 0, 0, 0, 0, 0, "RET", TGSI_OPCODE_RET },
   { 1, 1, 0, 0, 0, 0, "SSG", TGSI_OPCODE_SSG },
   { 1, 3, 0, 0, 0, 0, "CMP", TGSI_OPCODE_CMP },
   { 1, 1, 0, 0, 0, 0, "SCS", TGSI_OPCODE_SCS },
   { 1, 2, 1, 0, 0, 0, "TXB", TGSI_OPCODE_TXB },
   { 1, 1, 0, 0, 0, 0, "NRM", TGSI_OPCODE_NRM },
   { 1, 2, 0, 0, 0, 0, "DIV", TGSI_OPCODE_DIV },
   { 1, 2, 0, 0, 0, 0, "DP2", TGSI_OPCODE_DP2 },
   { 1, 2, 1, 0, 0, 0, "TXL", TGSI_OPCODE_TXL },
   { 0, 0, 0, 0, 0, 0, "BRK", TGSI_OPCODE_BRK },
   { 0, 1, 0, 1, 0, 1, "IF", TGSI_OPCODE_IF },
   { 1, 1, 0, 0, 0, 1, "", 75 },      /* removed */
   { 0, 1, 0, 0, 0, 1, "", 76 },      /* removed */
   { 0, 0, 0, 1, 1, 1, "ELSE", TGSI_OPCODE_ELSE },
   { 0, 0, 0, 0, 1, 0, "ENDIF", TGSI_OPCODE_ENDIF },
   { 1, 0, 0, 0, 1, 0, "", 79 },      /* removed */
   { 0, 0, 0, 0, 1, 0, "", 80 },      /* removed */
   { 0, 1, 0, 0, 0, 0, "PUSHA", TGSI_OPCODE_PUSHA },
   { 1, 0, 0, 0, 0, 0, "POPA", TGSI_OPCODE_POPA },
   { 1, 1, 0, 0, 0, 0, "CEIL", TGSI_OPCODE_CEIL },
   { 1, 1, 0, 0, 0, 0, "I2F", TGSI_OPCODE_I2F },
   { 1, 1, 0, 0, 0, 0, "NOT", TGSI_OPCODE_NOT },
   { 1, 1, 0, 0, 0, 0, "TRUNC", TGSI_OPCODE_TRUNC },
   { 1, 2, 0, 0, 0, 0, "SHL", TGSI_OPCODE_SHL },
   { 0, 0, 0, 0, 0, 0, "", 88 },      /* removed */
   { 1, 2, 0, 0, 0, 0, "AND", TGSI_OPCODE_AND },
   { 1, 2, 0, 0, 0, 0, "OR", TGSI_OPCODE_OR },
   { 1, 2, 0, 0, 0, 0, "MOD", TGSI_OPCODE_MOD },
   { 1, 2, 0, 0, 0, 0, "XOR", TGSI_OPCODE_XOR },
   { 1, 3, 0, 0, 0, 0, "SAD", TGSI_OPCODE_SAD },
   { 1, 2, 1, 0, 0, 0, "TXF", TGSI_OPCODE_TXF },
   { 1, 2, 1, 0, 0, 0, "TXQ", TGSI_OPCODE_TXQ },
   { 0, 0, 0, 0, 0, 0, "CONT", TGSI_OPCODE_CONT },
   { 0, 0, 0, 0, 0, 0, "EMIT", TGSI_OPCODE_EMIT },
   { 0, 0, 0, 0, 0, 0, "ENDPRIM", TGSI_OPCODE_ENDPRIM },
   { 0, 0, 0, 1, 0, 1, "BGNLOOP", TGSI_OPCODE_BGNLOOP },
   { 0, 0, 0, 0, 0, 1, "BGNSUB", TGSI_OPCODE_BGNSUB },
   { 0, 0, 0, 1, 1, 0, "ENDLOOP", TGSI_OPCODE_ENDLOOP },
   { 0, 0, 0, 0, 1, 0, "ENDSUB", TGSI_OPCODE_ENDSUB },
   { 0, 0, 0, 0, 0, 0, "", 103 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "", 104 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "", 105 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "", 106 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "NOP", TGSI_OPCODE_NOP },
   { 0, 0, 0, 0, 0, 0, "", 108 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "", 109 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "", 110 },     /* removed */
   { 0, 0, 0, 0, 0, 0, "", 111 },     /* removed */
   { 1, 1, 0, 0, 0, 0, "NRM4", TGSI_OPCODE_NRM4 },
   { 0, 1, 0, 0, 0, 0, "CALLNZ", TGSI_OPCODE_CALLNZ },
   { 0, 1, 0, 0, 0, 0, "IFC", TGSI_OPCODE_IFC },
   { 0, 1, 0, 0, 0, 0, "BREAKC", TGSI_OPCODE_BREAKC },
   { 0, 1, 0, 0, 0, 0, "KIL", TGSI_OPCODE_KIL },
   { 0, 0, 0, 0, 0, 0, "END", TGSI_OPCODE_END },
   { 0, 0, 0, 0, 0, 0, "", 118 },     /* removed */
   { 1, 1, 0, 0, 0, 0, "F2I", TGSI_OPCODE_F2I },
   { 1, 2, 0, 0, 0, 0, "IDIV", TGSI_OPCODE_IDIV },
   { 1, 2, 0, 0, 0, 0, "IMAX", TGSI_OPCODE_IMAX },
   { 1, 2, 0, 0, 0, 0, "IMIN", TGSI_OPCODE_IMIN },
   { 1, 1, 0, 0, 0, 0, "INEG", TGSI_OPCODE_INEG },
   { 1, 2, 0, 0, 0, 0, "ISGE", TGSI_OPCODE_ISGE },
   { 1, 2, 0, 0, 0, 0, "ISHR", TGSI_OPCODE_ISHR },
   { 1, 2, 0, 0, 0, 0, "ISLT", TGSI_OPCODE_ISLT },
   { 1, 1, 0, 0, 0, 0, "F2U", TGSI_OPCODE_F2U },
   { 1, 1, 0, 0, 0, 0, "U2F", TGSI_OPCODE_U2F },
   { 1, 2, 0, 0, 0, 0, "UADD", TGSI_OPCODE_UADD },
   { 1, 2, 0, 0, 0, 0, "UDIV", TGSI_OPCODE_UDIV },
   { 1, 3, 0, 0, 0, 0, "UMAD", TGSI_OPCODE_UMAD },
   { 1, 2, 0, 0, 0, 0, "UMAX", TGSI_OPCODE_UMAX },
   { 1, 2, 0, 0, 0, 0, "UMIN", TGSI_OPCODE_UMIN },
   { 1, 2, 0, 0, 0, 0, "UMOD", TGSI_OPCODE_UMOD },
   { 1, 2, 0, 0, 0, 0, "UMUL", TGSI_OPCODE_UMUL },
   { 1, 2, 0, 0, 0, 0, "USEQ", TGSI_OPCODE_USEQ },
   { 1, 2, 0, 0, 0, 0, "USGE", TGSI_OPCODE_USGE },
   { 1, 2, 0, 0, 0, 0, "USHR", TGSI_OPCODE_USHR },
   { 1, 2, 0, 0, 0, 0, "USLT", TGSI_OPCODE_USLT },
   { 1, 2, 0, 0, 0, 0, "USNE", TGSI_OPCODE_USNE },
   { 0, 1, 0, 0, 0, 0, "SWITCH", TGSI_OPCODE_SWITCH },
   { 0, 1, 0, 0, 0, 0, "CASE", TGSI_OPCODE_CASE },
   { 0, 0, 0, 0, 0, 0, "DEFAULT", TGSI_OPCODE_DEFAULT },
   { 0, 0, 0, 0, 0, 0, "ENDSWITCH", TGSI_OPCODE_ENDSWITCH },

   { 1, 2, 0, 0, 0, 0, "LOAD",        TGSI_OPCODE_LOAD },
   { 1, 2, 0, 0, 0, 0, "LOAD_MS",     TGSI_OPCODE_LOAD_MS },
   { 1, 3, 0, 0, 0, 0, "SAMPLE",      TGSI_OPCODE_SAMPLE },
   { 1, 4, 0, 0, 0, 0, "SAMPLE_B",    TGSI_OPCODE_SAMPLE_B },
   { 1, 4, 0, 0, 0, 0, "SAMPLE_C",    TGSI_OPCODE_SAMPLE_C },
   { 1, 4, 0, 0, 0, 0, "SAMPLE_C_LZ", TGSI_OPCODE_SAMPLE_C_LZ },
   { 1, 5, 0, 0, 0, 0, "SAMPLE_D",    TGSI_OPCODE_SAMPLE_D },
   { 1, 3, 0, 0, 0, 0, "SAMPLE_L",    TGSI_OPCODE_SAMPLE_L },
   { 1, 3, 0, 0, 0, 0, "GATHER4",     TGSI_OPCODE_GATHER4 },
   { 1, 2, 0, 0, 0, 0, "RESINFO",     TGSI_OPCODE_RESINFO },
   { 1, 2, 0, 0, 0, 0, "SAMPLE_POS",  TGSI_OPCODE_SAMPLE_POS },
   { 1, 2, 0, 0, 0, 0, "SAMPLE_INFO", TGSI_OPCODE_SAMPLE_INFO },
   
   { 1, 1, 0, 0, 0, 0, "UARL", TGSI_OPCODE_UARL },
   { 1, 3, 0, 0, 0, 0, "UCMP", TGSI_OPCODE_UCMP },
   { 1, 1, 0, 0, 0, 0, "IABS", TGSI_OPCODE_IABS },
   { 1, 1, 0, 0, 0, 0, "ISSG", TGSI_OPCODE_ISSG },
};

const struct tgsi_opcode_info *
tgsi_get_opcode_info( uint opcode )
{
   static boolean firsttime = 1;

   if (firsttime) {
      unsigned i;
      firsttime = 0;
      for (i = 0; i < Elements(opcode_info); i++)
         assert(opcode_info[i].opcode == i);
   }
   
   if (opcode < TGSI_OPCODE_LAST)
      return &opcode_info[opcode];

   assert( 0 );
   return NULL;
}


const char *
tgsi_get_opcode_name( uint opcode )
{
   const struct tgsi_opcode_info *info = tgsi_get_opcode_info(opcode);
   return info->mnemonic;
}


const char *
tgsi_get_processor_name( uint processor )
{
   switch (processor) {
   case TGSI_PROCESSOR_VERTEX:
      return "vertex shader";
   case TGSI_PROCESSOR_FRAGMENT:
      return "fragment shader";
   case TGSI_PROCESSOR_GEOMETRY:
      return "geometry shader";
   default:
      return "unknown shader type!";
   }
}
