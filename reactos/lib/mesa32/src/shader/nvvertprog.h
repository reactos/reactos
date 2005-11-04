/*
 * Mesa 3-D graphics library
 * Version:  6.3.1
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


/* Private vertex program types and constants only used by files
 * related to vertex programs.
 *
 * XXX TO-DO: Rename this file "vertprog.h" since it's not NV-specific.
 */


#ifndef NVVERTPROG_H
#define NVVERTPROG_H


/* Vertex program opcodes */
enum vp_opcode
{
   VP_OPCODE_ABS,
   VP_OPCODE_ADD,
   VP_OPCODE_ARL,
   VP_OPCODE_DP3,
   VP_OPCODE_DP4,
   VP_OPCODE_DPH,
   VP_OPCODE_DST,
   VP_OPCODE_END,		/* Placeholder */
   VP_OPCODE_EX2,		/* ARB only */
   VP_OPCODE_EXP,
   VP_OPCODE_FLR,		/* ARB */
   VP_OPCODE_FRC,		/* ARB */
   VP_OPCODE_LG2,		/* ARB only */
   VP_OPCODE_LIT,
   VP_OPCODE_LOG,
   VP_OPCODE_MAD,
   VP_OPCODE_MAX,
   VP_OPCODE_MIN,
   VP_OPCODE_MOV,
   VP_OPCODE_MUL,
   VP_OPCODE_POW,		/* ARB only */
   VP_OPCODE_PRINT,		/* Mesa only */
   VP_OPCODE_RCC,
   VP_OPCODE_RCP,
   VP_OPCODE_RSQ,
   VP_OPCODE_SGE,
   VP_OPCODE_SLT,
   VP_OPCODE_SUB,
   VP_OPCODE_SWZ,		/* ARB only */
   VP_OPCODE_XPD,		/* ARB only */

   VP_MAX_OPCODE
};



/* Instruction source register */
struct vp_src_register
{
   GLuint File:4;		/* one of the PROGRAM_* register file values */
   GLint Index:9;		/* may be negative for relative addressing */
   GLuint Swizzle:12;
   GLuint Negate:4;		/* ARB requires component-wise negation. */
   GLuint RelAddr:1;
   GLuint pad:2;
};


/* Instruction destination register */
struct vp_dst_register
{
   GLuint File:4;		/* one of the PROGRAM_* register file values */
   GLuint Index:8;
   GLuint WriteMask:4;
   GLuint pad:16;
};


/* Vertex program instruction */
struct vp_instruction
{
   GLshort Opcode;
#if FEATURE_MESA_program_debug
   GLshort StringPos;
#endif
   void *Data;  /* some arbitrary data, only used for PRINT instruction now */
   struct vp_src_register SrcReg[3];
   struct vp_dst_register DstReg;
};


#endif /* VERTPROG_H */
