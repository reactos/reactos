/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
   VP_OPCODE_MOV,
   VP_OPCODE_LIT,
   VP_OPCODE_RCP,
   VP_OPCODE_RSQ,
   VP_OPCODE_EXP,
   VP_OPCODE_LOG,
   VP_OPCODE_MUL,
   VP_OPCODE_ADD,
   VP_OPCODE_DP3,
   VP_OPCODE_DP4,
   VP_OPCODE_DST,
   VP_OPCODE_MIN,
   VP_OPCODE_MAX,
   VP_OPCODE_SLT,
   VP_OPCODE_SGE,
   VP_OPCODE_MAD,
   VP_OPCODE_ARL,
   VP_OPCODE_DPH,
   VP_OPCODE_RCC,
   VP_OPCODE_SUB,
   VP_OPCODE_ABS,
   VP_OPCODE_END,
   /* Additional opcodes for GL_ARB_vertex_program */ 
   VP_OPCODE_FLR,
   VP_OPCODE_FRC,
   VP_OPCODE_EX2,
   VP_OPCODE_LG2,
   VP_OPCODE_POW,
   VP_OPCODE_XPD,
   VP_OPCODE_SWZ
};



/* Instruction source register */
struct vp_src_register
{
   enum register_file File;  /* which register file */
   GLint Index;              /* index into register file */
   GLubyte Swizzle[4]; /* Each value is 0,1,2,3 for x,y,z,w or */
                       /* SWIZZLE_ZERO or SWIZZLE_ONE for VP_OPCODE_SWZ. */
   GLboolean Negate;
   GLboolean RelAddr;
};


/* Instruction destination register */
struct vp_dst_register
{
   enum register_file File;  /* which register file */
   GLint Index;              /* index into register file */
   GLboolean WriteMask[4];
};


/* Vertex program instruction */
struct vp_instruction
{
   enum vp_opcode Opcode;
   struct vp_src_register SrcReg[3];
   struct vp_dst_register DstReg;
#if FEATURE_MESA_program_debug
   GLint StringPos;
#endif
};


#endif /* VERTPROG_H */
