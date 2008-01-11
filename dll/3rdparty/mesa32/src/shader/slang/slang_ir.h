/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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
 * \file slang_ir.h
 * Mesa GLSL Intermediate Representation tree types and constants.
 * \author Brian Paul
 */


#ifndef SLANG_IR_H
#define SLANG_IR_H


#include "imports.h"
#include "slang_compile.h"
#include "slang_label.h"
#include "mtypes.h"


/**
 * Intermediate Representation opcodes
 */
typedef enum
{
   IR_NOP = 0,
   IR_SEQ,     /* sequence (eval left, then right) */
   IR_SCOPE,   /* new variable scope (one child) */

   IR_LABEL,   /* target of a jump or cjump */

   IR_COND,    /* conditional expression/predicate */

   IR_IF,      /* high-level IF/then/else */
               /* Children[0] = conditional expression */
               /* Children[1] = if-true part */
               /* Children[2] = if-else part, or NULL */

   IR_BEGIN_SUB, /* begin subroutine */
   IR_END_SUB,   /* end subroutine */
   IR_RETURN,    /* return from subroutine */
   IR_CALL,      /* call subroutine */

   IR_LOOP,      /* high-level loop-begin / loop-end */
                 /* Children[0] = loop body */
                 /* Children[1] = loop tail code, or NULL */

   IR_CONT,      /* continue loop */
                 /* n->Parent = ptr to parent IR_LOOP Node */
   IR_BREAK,     /* break loop */

   IR_BREAK_IF_TRUE,
   IR_CONT_IF_TRUE,
                 /* Children[0] = the condition expression */

   IR_MOVE,

   /* vector ops: */
   IR_ADD,
   IR_SUB,
   IR_MUL,
   IR_DIV,
   IR_DOT4,
   IR_DOT3,
   IR_CROSS,   /* vec3 cross product */
   IR_LRP,
   IR_CLAMP,
   IR_MIN,
   IR_MAX,
   IR_SEQUAL,  /* Set if args are equal (vector) */
   IR_SNEQUAL, /* Set if args are not equal (vector) */
   IR_SGE,     /* Set if greater or equal (vector) */
   IR_SGT,     /* Set if greater than (vector) */
   IR_SLE,     /* Set if less or equal (vector) */
   IR_SLT,     /* Set if less than (vector) */
   IR_POW,     /* x^y */
   IR_EXP,     /* e^x */
   IR_EXP2,    /* 2^x */
   IR_LOG2,    /* log base 2 */
   IR_RSQ,     /* 1/sqrt() */
   IR_RCP,     /* reciprocol */
   IR_FLOOR,
   IR_FRAC,
   IR_ABS,     /* absolute value */
   IR_NEG,     /* negate */
   IR_DDX,     /* derivative w.r.t. X */
   IR_DDY,     /* derivative w.r.t. Y */
   IR_SIN,     /* sine */
   IR_COS,     /* cosine */
   IR_NOISE1,  /* noise(x) */
   IR_NOISE2,  /* noise(x, y) */
   IR_NOISE3,  /* noise(x, y, z) */
   IR_NOISE4,  /* noise(x, y, z, w) */

   IR_EQUAL,   /* boolean equality */
   IR_NOTEQUAL,/* boolean inequality */
   IR_NOT,     /* boolean not */

   IR_VAR,     /* variable reference */
   IR_VAR_DECL,/* var declaration */

   IR_ELEMENT, /* array element */
   IR_FIELD,   /* struct field */
   IR_SWIZZLE, /* swizzled storage access */

   IR_TEX,     /* texture lookup */
   IR_TEXB,    /* texture lookup with LOD bias */
   IR_TEXP,    /* texture lookup with projection */

   IR_FLOAT,
   IR_I_TO_F,  /* int[4] to float[4] conversion */
   IR_F_TO_I,  /* float[4] to int[4] conversion */

   IR_KILL     /* fragment kill/discard */
} slang_ir_opcode;


/**
 * Describes where data storage is allocated.
 */
struct _slang_ir_storage
{
   enum register_file File;  /**< PROGRAM_TEMPORARY, PROGRAM_INPUT, etc */
   GLint Index;  /**< -1 means unallocated */
   GLint Size;  /**< number of floats */
   GLuint Swizzle;
   GLint RefCount; /**< Used during IR tree delete */
};

typedef struct _slang_ir_storage slang_ir_storage;


/**
 * Intermediate Representation (IR) tree node
 * Basically a binary tree, but IR_LRP and IR_CLAMP have three children.
 */
typedef struct slang_ir_node_
{
   slang_ir_opcode Opcode;
   struct slang_ir_node_ *Children[3];
   slang_ir_storage *Store;  /**< location of result of this operation */
   GLint InstLocation;  /**< Location of instruction emitted for this node */

   /** special fields depending on Opcode: */
   const char *Field;  /**< If Opcode == IR_FIELD */
   int FieldOffset;  /**< If Opcode == IR_FIELD */
   GLuint Writemask;  /**< If Opcode == IR_MOVE */
   GLfloat Value[4];    /**< If Opcode == IR_FLOAT */
   slang_variable *Var;  /**< If Opcode == IR_VAR or IR_VAR_DECL */
   struct slang_ir_node_ *List;  /**< For various linked lists */
   struct slang_ir_node_ *Parent;  /**< Pointer to logical parent (ie. loop) */
   slang_label *Label;  /**< Used for branches */
} slang_ir_node;



/**
 * Assembly and IR info
 */
typedef struct
{
   slang_ir_opcode IrOpcode;
   const char *IrName;
   gl_inst_opcode InstOpcode;
   GLuint ResultSize, NumParams;
} slang_ir_info;



extern const slang_ir_info *
_slang_ir_info(slang_ir_opcode opcode);


extern void
_slang_free_ir_tree(slang_ir_node *n);


extern void
_slang_print_ir_tree(const slang_ir_node *n, int indent);


#endif /* SLANG_IR_H */
