/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* Private fragment program types and constants only used by files
 * related to fragment programs.
 *
 * XXX TO-DO: Rename this file "fragprog.h" since it's not NV-specific.
 */


#ifndef NVFRAGPROG_H
#define NVFRAGPROG_H

#include "config.h"
#include "mtypes.h"

/* output registers */
#define FRAG_OUTPUT_COLR  0
#define FRAG_OUTPUT_COLH  1
#define FRAG_OUTPUT_DEPR  2


/* condition codes */
#define COND_GT  1  /* greater than zero */
#define COND_EQ  2  /* equal to zero */
#define COND_LT  3  /* less than zero */
#define COND_UN  4  /* unordered (NaN) */
#define COND_GE  5  /* greater then or equal to zero */
#define COND_LE  6  /* less then or equal to zero */
#define COND_NE  7  /* not equal to zero */
#define COND_TR  8  /* always true */
#define COND_FL  9  /* always false */


/* instruction precision */
#define FLOAT32  0x1
#define FLOAT16  0x2
#define FIXED12  0x4


/* Fragment program instruction opcodes */
enum fp_opcode {
   FP_OPCODE_ABS,		/* ARB_f_p only */
   FP_OPCODE_ADD,
   FP_OPCODE_CMP,		/* ARB_f_p only */
   FP_OPCODE_COS,
   FP_OPCODE_DDX,		/* NV_f_p only */
   FP_OPCODE_DDY,		/* NV_f_p only */
   FP_OPCODE_DP3,
   FP_OPCODE_DP4,
   FP_OPCODE_DPH,		/* ARB_f_p only */
   FP_OPCODE_DST,
   FP_OPCODE_END,		/* private opcode */
   FP_OPCODE_EX2,
   FP_OPCODE_FLR,
   FP_OPCODE_FRC,
   FP_OPCODE_KIL,		/* ARB_f_p only */
   FP_OPCODE_KIL_NV,		/* NV_f_p only */
   FP_OPCODE_LG2,
   FP_OPCODE_LIT,
   FP_OPCODE_LRP,
   FP_OPCODE_MAD,
   FP_OPCODE_MAX,
   FP_OPCODE_MIN,
   FP_OPCODE_MOV,
   FP_OPCODE_MUL,
   FP_OPCODE_PK2H,		/* NV_f_p only */
   FP_OPCODE_PK2US,		/* NV_f_p only */
   FP_OPCODE_PK4B,		/* NV_f_p only */
   FP_OPCODE_PK4UB,		/* NV_f_p only */
   FP_OPCODE_POW,
   FP_OPCODE_PRINT,		/* Mesa only */
   FP_OPCODE_RCP,
   FP_OPCODE_RFL,		/* NV_f_p only */
   FP_OPCODE_RSQ,
   FP_OPCODE_SCS,		/* ARB_f_p only */
   FP_OPCODE_SEQ,		/* NV_f_p only */
   FP_OPCODE_SFL,		/* NV_f_p only */
   FP_OPCODE_SGE,		/* NV_f_p only */
   FP_OPCODE_SGT,		/* NV_f_p only */
   FP_OPCODE_SIN,
   FP_OPCODE_SLE,		/* NV_f_p only */
   FP_OPCODE_SLT,
   FP_OPCODE_SNE,		/* NV_f_p only */
   FP_OPCODE_STR,		/* NV_f_p only */
   FP_OPCODE_SUB,
   FP_OPCODE_SWZ,		/* ARB_f_p only */
   FP_OPCODE_TEX,
   FP_OPCODE_TXB,		/* ARB_f_p only */
   FP_OPCODE_TXD,		/* NV_f_p only */
   FP_OPCODE_TXP,		/* ARB_f_p only */
   FP_OPCODE_TXP_NV,		/* NV_f_p only */
   FP_OPCODE_UP2H,		/* NV_f_p only */
   FP_OPCODE_UP2US,		/* NV_f_p only */
   FP_OPCODE_UP4B,		/* NV_f_p only */
   FP_OPCODE_UP4UB,		/* NV_f_p only */
   FP_OPCODE_X2D,		/* NV_f_p only - 2d mat mul */
   FP_OPCODE_XPD		/* ARB_f_p only - cross product */
};


/* Instruction source register */
struct fp_src_register
{
   GLuint File:4;
   GLuint Index:8;
   GLuint Swizzle:12;
   GLuint NegateBase:4; /* ARB: negate/extended negate.
			   NV: negate before absolute value? */
   GLuint Abs:1;        /* NV: take absolute value? */
   GLuint NegateAbs:1;  /* NV: negate after absolute value? */
};


/* Instruction destination register */
struct fp_dst_register
{
   GLuint File:4;
   GLuint Index:8;
   GLuint WriteMask:4;
   GLuint CondMask:4;		/* NV: enough bits? */
   GLuint CondSwizzle:12;	/* NV: enough bits? */
};


/* Fragment program instruction */
struct fp_instruction
{
   GLuint Opcode:6;
   GLuint Saturate:1;	
   GLuint UpdateCondRegister:1;	/* NV */
   GLuint Precision:2;    /* NV: unused/unneeded? */
   GLuint TexSrcUnit:4;   /* texture unit for TEX, TXD, TXP instructions */
   GLuint TexSrcIdx:3;    /* TEXTURE_1D,2D,3D,CUBE,RECT_INDEX source target */

#if FEATURE_MESA_program_debug
   GLint StringPos:15;		/* enough bits? */
#endif

   void *Data;  /* some arbitrary data, only used for PRINT instruction now */
   struct fp_src_register SrcReg[3];
   struct fp_dst_register DstReg;
};


#endif
