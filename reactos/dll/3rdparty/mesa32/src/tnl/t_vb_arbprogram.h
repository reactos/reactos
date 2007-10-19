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

/**
 * \file t_arb_program.c
 * Compile vertex programs to an intermediate representation.
 * Execute vertex programs over a buffer of vertices.
 * \author Keith Whitwell, Brian Paul
 */


#ifndef _T_VB_ARBPROGRAM_H_
#define _T_VB_ARBPROGRAM_H_


/* New, internal instructions:
 */
#define RSW        (VP_MAX_OPCODE)
#define MSK        (VP_MAX_OPCODE+1)
#define REL        (VP_MAX_OPCODE+2)

/**
 * Register files for vertex programs
 */
#define FILE_REG         0  /* temporaries */
#define FILE_LOCAL_PARAM 1  /* local parameters */
#define FILE_ENV_PARAM   2  /* global parameters */
#define FILE_STATE_PARAM 3  /* GL state references */

#define REG_ARG0   0
#define REG_ARG1   1
#define REG_ARG2   2
#define REG_RES    3
#define REG_ADDR   4
#define REG_TMP0   5
#define REG_TMP11  16
#define REG_OUT0   17
#define REG_OUT14  31
#define REG_IN0    32
#define REG_IN31   63
#define REG_ID     64		/* 0,0,0,1 */
#define REG_ONES   65		/* 1,1,1,1 */
#define REG_SWZ    66		/* -1,1,0,0 */
#define REG_NEG    67		/* -1,-1,-1,-1 */
#define REG_LIT    68           /* 1,0,0,1 */
#define REG_LIT2    69           /* 1,0,0,1 */
#define REG_SCRATCH 70		/* internal temporary */
#define REG_UNDEF  127		/* special case - never used */
#define REG_MAX    128
#define REG_INVALID ~0

/* ARB_vp instructions are broken down into one or more of the
 * following micro-instructions, each representable in a 32 bit packed
 * structure.
 */
struct reg {
   GLuint file:2;
   GLuint idx:7;
};


union instruction {
   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file0:2;
      GLuint idx0:7;
      GLuint file1:2;
      GLuint idx1:7;
      GLuint pad:3;
   } alu;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file0:2;
      GLuint idx0:7;
      GLuint neg:4;
      GLuint swz:8;		/* xyzw only */
   } rsw;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file:2;
      GLuint idx:7;
      GLuint mask:4;
      GLuint pad:1;
   } msk;

   GLuint dword;
};

#define RSW_NOOP ((0<<0) | (1<<2) | (2<<4) | (3<<6))
#define GET_RSW(swz, idx)      (((swz) >> ((idx)*2)) & 0x3)


struct input {
   GLuint idx;
   GLfloat *data;
   GLuint stride;
   GLuint size;
};

struct output {
   GLuint idx;
   GLfloat *data;
};



/*--------------------------------------------------------------------------- */
#if defined(USE_SSE_ASM)
#ifdef NO_FAST_MATH
#define RESTORE_FPU (DEFAULT_X86_FPU)
#define RND_NEG_FPU (DEFAULT_X86_FPU | 0x400)
#else
#define RESTORE_FPU (FAST_X86_FPU)
#define RND_NEG_FPU (FAST_X86_FPU | 0x400)
#endif
#else
#define RESTORE_FPU 0
#define RND_NEG_FPU 0
#endif


/*!
 * Private storage for the vertex program pipeline stage.
 */
struct arb_vp_machine {
   GLfloat (*File[4])[4];	/* All values referencable from the program. */

   struct input input[_TNL_ATTRIB_MAX];
   GLuint nr_inputs;

   struct output output[15];
   GLuint nr_outputs;

   GLvector4f attribs[VERT_RESULT_MAX]; /**< result vectors. */
   GLvector4f ndcCoords;              /**< normalized device coords */
   GLubyte *clipmask;                 /**< clip flags */
   GLubyte ormask, andmask;           /**< for clipping */

   GLuint vtx_nr;		/**< loop counter */

   struct vertex_buffer *VB;
   GLcontext *ctx;

   GLshort fpucntl_rnd_neg;	/* constant value */
   GLshort fpucntl_restore;	/* constant value */

   GLboolean try_codegen;
};

struct tnl_compiled_program {
   union instruction instructions[1024];
   GLint nr_instructions;
   void (*compiled_func)( struct arb_vp_machine * ); /**< codegen'd program */
};

void _tnl_program_string_change( struct vertex_program * );
void _tnl_program_destroy( struct vertex_program * );

void _tnl_disassem_vba_insn( union instruction op );

GLboolean _tnl_sse_codegen_vertex_program(struct tnl_compiled_program *p);

#endif
