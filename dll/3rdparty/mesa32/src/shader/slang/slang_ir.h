/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2005-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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


#include "main/imports.h"
#include "slang_compile.h"
#include "slang_label.h"
#include "main/mtypes.h"


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

   IR_BREAK_IF_TRUE, /**< Children[0] = the condition expression */
   IR_CONT_IF_TRUE,

   IR_COPY,       /**< assignment/copy */
   IR_MOVE,       /**< assembly MOV instruction */

   /* vector ops: */
   IR_ADD,        /**< assembly ADD instruction */
   IR_SUB,
   IR_MUL,
   IR_DIV,
   IR_DOT4,
   IR_DOT3,
   IR_DOT2,
   IR_NRM4,
   IR_NRM3,
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
 * Describes where data/variables are stored in the various register files.
 *
 * In the simple case, the File, Index and Size fields indicate where
 * a variable is stored.  For example, a vec3 variable may be stored
 * as (File=PROGRAM_TEMPORARY, Index=6, Size=3).  Or, File[Index].
 * Or, a program input like color may be stored as
 * (File=PROGRAM_INPUT,Index=3,Size=4);
 *
 * For single-float values, the Swizzle field indicates which component
 * of the vector contains the float.
 *
 * If IsIndirect is set, the storage is accessed through an indirect
 * register lookup.  The value in question will be located at:
 *   File[Index + IndirectFile[IndirectIndex]]
 *
 * This is primary used for indexing arrays.  For example, consider this
 * GLSL code:
 *   uniform int i;
 *   float a[10];
 *   float x = a[i];
 *
 * here, storage for a[i] would be described by (File=PROGRAM_TEMPORAY,
 * Index=aPos, IndirectFile=PROGRAM_UNIFORM, IndirectIndex=iPos), which
 * would mean TEMP[aPos + UNIFORM[iPos]]
 */
struct slang_ir_storage_
{
   enum register_file File;  /**< PROGRAM_TEMPORARY, PROGRAM_INPUT, etc */
   GLint Index;    /**< -1 means unallocated */
   GLint Size;     /**< number of floats or ints */
   GLuint Swizzle; /**< Swizzle AND writemask info */
   GLint RefCount; /**< Used during IR tree delete */

   GLboolean RelAddr; /* we'll remove this eventually */

   GLboolean IsIndirect;
   enum register_file IndirectFile;
   GLint IndirectIndex;
   GLuint IndirectSwizzle;
   GLuint TexTarget;  /**< If File==PROGRAM_SAMPLER, one of TEXTURE_x_INDEX */

   /** If Parent is non-null, Index is relative to parent.
    * The other fields are ignored.
    */
   struct slang_ir_storage_ *Parent;
};

typedef struct slang_ir_storage_ slang_ir_storage;


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
_slang_init_ir_storage(slang_ir_storage *st,
                       enum register_file file, GLint index, GLint size,
                       GLuint swizzle);

extern slang_ir_storage *
_slang_new_ir_storage(enum register_file file, GLint index, GLint size);


extern slang_ir_storage *
_slang_new_ir_storage_swz(enum register_file file, GLint index, GLint size,
                          GLuint swizzle);

extern slang_ir_storage *
_slang_new_ir_storage_relative(GLint index, GLint size,
                               slang_ir_storage *parent);


extern slang_ir_storage *
_slang_new_ir_storage_indirect(enum register_file file,
                               GLint index,
                               GLint size,
                               enum register_file indirectFile,
                               GLint indirectIndex,
                               GLuint indirectSwizzle);

extern slang_ir_storage *
_slang_new_ir_storage_sampler(GLint sampNum, GLuint texTarget, GLint size);


extern void
_slang_copy_ir_storage(slang_ir_storage *dst, const slang_ir_storage *src);


extern void
_slang_free_ir_tree(slang_ir_node *n);


extern void
_slang_print_ir_tree(const slang_ir_node *n, int indent);


#endif /* SLANG_IR_H */
