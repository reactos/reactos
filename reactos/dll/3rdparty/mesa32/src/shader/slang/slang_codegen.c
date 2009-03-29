/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008 VMware, Inc.  All Rights Reserved.
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
 * \file slang_codegen.c
 * Generate IR tree from AST.
 * \author Brian Paul
 */


/***
 *** NOTES:
 *** The new_() functions return a new instance of a simple IR node.
 *** The gen_() functions generate larger IR trees from the simple nodes.
 ***/



#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "slang_typeinfo.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_label.h"
#include "slang_mem.h"
#include "slang_simplify.h"
#include "slang_emit.h"
#include "slang_vartable.h"
#include "slang_ir.h"
#include "slang_print.h"


/** Max iterations to unroll */
const GLuint MAX_FOR_LOOP_UNROLL_ITERATIONS = 32;

/** Max for-loop body size (in slang operations) to unroll */
const GLuint MAX_FOR_LOOP_UNROLL_BODY_SIZE = 50;

/** Max for-loop body complexity to unroll.
 * We'll compute complexity as the product of the number of iterations
 * and the size of the body.  So long-ish loops with very simple bodies
 * can be unrolled, as well as short loops with larger bodies.
 */
const GLuint MAX_FOR_LOOP_UNROLL_COMPLEXITY = 256;



static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper);


/**
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
static GLboolean
typeof_operation(const struct slang_assemble_ctx_ *A,
                 slang_operation *op,
                 slang_typeinfo *ti)
{
   return _slang_typeof_operation(op, &A->space, ti, A->atoms, A->log);
}


static GLboolean
is_sampler_type(const slang_fully_specified_type *t)
{
   switch (t->specifier.type) {
   case SLANG_SPEC_SAMPLER1D:
   case SLANG_SPEC_SAMPLER2D:
   case SLANG_SPEC_SAMPLER3D:
   case SLANG_SPEC_SAMPLERCUBE:
   case SLANG_SPEC_SAMPLER1DSHADOW:
   case SLANG_SPEC_SAMPLER2DSHADOW:
   case SLANG_SPEC_SAMPLER2DRECT:
   case SLANG_SPEC_SAMPLER2DRECTSHADOW:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Return the offset (in floats or ints) of the named field within
 * the given struct.  Return -1 if field not found.
 * If field is NULL, return the size of the struct instead.
 */
static GLint
_slang_field_offset(const slang_type_specifier *spec, slang_atom field)
{
   GLint offset = 0;
   GLuint i;
   for (i = 0; i < spec->_struct->fields->num_variables; i++) {
      const slang_variable *v = spec->_struct->fields->variables[i];
      const GLuint sz = _slang_sizeof_type_specifier(&v->type.specifier);
      if (sz > 1) {
         /* types larger than 1 float are register (4-float) aligned */
         offset = (offset + 3) & ~3;
      }
      if (field && v->a_name == field) {
         return offset;
      }
      offset += sz;
   }
   if (field)
      return -1; /* field not found */
   else
      return offset;  /* struct size */
}


/**
 * Return the size (in floats) of the given type specifier.
 * If the size is greater than 4, the size should be a multiple of 4
 * so that the correct number of 4-float registers are allocated.
 * For example, a mat3x2 is size 12 because we want to store the
 * 3 columns in 3 float[4] registers.
 */
GLuint
_slang_sizeof_type_specifier(const slang_type_specifier *spec)
{
   GLuint sz;
   switch (spec->type) {
   case SLANG_SPEC_VOID:
      sz = 0;
      break;
   case SLANG_SPEC_BOOL:
      sz = 1;
      break;
   case SLANG_SPEC_BVEC2:
      sz = 2;
      break;
   case SLANG_SPEC_BVEC3:
      sz = 3;
      break;
   case SLANG_SPEC_BVEC4:
      sz = 4;
      break;
   case SLANG_SPEC_INT:
      sz = 1;
      break;
   case SLANG_SPEC_IVEC2:
      sz = 2;
      break;
   case SLANG_SPEC_IVEC3:
      sz = 3;
      break;
   case SLANG_SPEC_IVEC4:
      sz = 4;
      break;
   case SLANG_SPEC_FLOAT:
      sz = 1;
      break;
   case SLANG_SPEC_VEC2:
      sz = 2;
      break;
   case SLANG_SPEC_VEC3:
      sz = 3;
      break;
   case SLANG_SPEC_VEC4:
      sz = 4;
      break;
   case SLANG_SPEC_MAT2:
      sz = 2 * 4; /* 2 columns (regs) */
      break;
   case SLANG_SPEC_MAT3:
      sz = 3 * 4;
      break;
   case SLANG_SPEC_MAT4:
      sz = 4 * 4;
      break;
   case SLANG_SPEC_MAT23:
      sz = 2 * 4; /* 2 columns (regs) */
      break;
   case SLANG_SPEC_MAT32:
      sz = 3 * 4; /* 3 columns (regs) */
      break;
   case SLANG_SPEC_MAT24:
      sz = 2 * 4;
      break;
   case SLANG_SPEC_MAT42:
      sz = 4 * 4; /* 4 columns (regs) */
      break;
   case SLANG_SPEC_MAT34:
      sz = 3 * 4;
      break;
   case SLANG_SPEC_MAT43:
      sz = 4 * 4; /* 4 columns (regs) */
      break;
   case SLANG_SPEC_SAMPLER1D:
   case SLANG_SPEC_SAMPLER2D:
   case SLANG_SPEC_SAMPLER3D:
   case SLANG_SPEC_SAMPLERCUBE:
   case SLANG_SPEC_SAMPLER1DSHADOW:
   case SLANG_SPEC_SAMPLER2DSHADOW:
   case SLANG_SPEC_SAMPLER2DRECT:
   case SLANG_SPEC_SAMPLER2DRECTSHADOW:
      sz = 1; /* a sampler is basically just an integer index */
      break;
   case SLANG_SPEC_STRUCT:
      sz = _slang_field_offset(spec, 0); /* special use */
      if (sz == 1) {
         /* 1-float structs are actually troublesome to deal with since they
          * might get placed at R.x, R.y, R.z or R.z.  Return size=2 to
          * ensure the object is placed at R.x
          */
         sz = 2;
      }
      else if (sz > 4) {
         sz = (sz + 3) & ~0x3; /* round up to multiple of four */
      }
      break;
   case SLANG_SPEC_ARRAY:
      sz = _slang_sizeof_type_specifier(spec->_array);
      break;
   default:
      _mesa_problem(NULL, "Unexpected type in _slang_sizeof_type_specifier()");
      sz = 0;
   }

   if (sz > 4) {
      /* if size is > 4, it should be a multiple of four */
      assert((sz & 0x3) == 0);
   }
   return sz;
}


/**
 * Query variable/array length (number of elements).
 * This is slightly non-trivial because there are two ways to express
 * arrays: "float x[3]" vs. "float[3] x".
 * \return the length of the array for the given variable, or 0 if not an array
 */
static GLint
_slang_array_length(const slang_variable *var)
{
   if (var->type.array_len > 0) {
      /* Ex: float[4] x; */
      return var->type.array_len;
   }
   if (var->array_len > 0) {
      /* Ex: float x[4]; */
      return var->array_len;
   }
   return 0;
}


/**
 * Compute total size of array give size of element, number of elements.
 * \return size in floats
 */
static GLint
_slang_array_size(GLint elemSize, GLint arrayLen)
{
   GLint total;
   assert(elemSize > 0);
   if (arrayLen > 1) {
      /* round up base type to multiple of 4 */
      total = ((elemSize + 3) & ~0x3) * MAX2(arrayLen, 1);
   }
   else {
      total = elemSize;
   }
   return total;
}



/**
 * Establish the binding between a slang_ir_node and a slang_variable.
 * Then, allocate/attach a slang_ir_storage object to the IR node if needed.
 * The IR node must be a IR_VAR or IR_VAR_DECL node.
 * \param n  the IR node
 * \param var  the variable to associate with the IR node
 */
static void
_slang_attach_storage(slang_ir_node *n, slang_variable *var)
{
   assert(n);
   assert(var);
   assert(n->Opcode == IR_VAR || n->Opcode == IR_VAR_DECL);
   assert(!n->Var || n->Var == var);

   n->Var = var;

   if (!n->Store) {
      /* need to setup storage */
      if (n->Var && n->Var->store) {
         /* node storage info = var storage info */
         n->Store = n->Var->store;
      }
      else {
         /* alloc new storage info */
         n->Store = _slang_new_ir_storage(PROGRAM_UNDEFINED, -7, -5);
#if 0
         printf("%s var=%s Store=%p Size=%d\n", __FUNCTION__,
                (char*) var->a_name,
                (void*) n->Store, n->Store->Size);
#endif
         if (n->Var)
            n->Var->store = n->Store;
         assert(n->Var->store);
      }
   }
}


/**
 * Return the TEXTURE_*_INDEX value that corresponds to a sampler type,
 * or -1 if the type is not a sampler.
 */
static GLint
sampler_to_texture_index(const slang_type_specifier_type type)
{
   switch (type) {
   case SLANG_SPEC_SAMPLER1D:
      return TEXTURE_1D_INDEX;
   case SLANG_SPEC_SAMPLER2D:
      return TEXTURE_2D_INDEX;
   case SLANG_SPEC_SAMPLER3D:
      return TEXTURE_3D_INDEX;
   case SLANG_SPEC_SAMPLERCUBE:
      return TEXTURE_CUBE_INDEX;
   case SLANG_SPEC_SAMPLER1DSHADOW:
      return TEXTURE_1D_INDEX; /* XXX fix */
   case SLANG_SPEC_SAMPLER2DSHADOW:
      return TEXTURE_2D_INDEX; /* XXX fix */
   case SLANG_SPEC_SAMPLER2DRECT:
      return TEXTURE_RECT_INDEX;
   case SLANG_SPEC_SAMPLER2DRECTSHADOW:
      return TEXTURE_RECT_INDEX; /* XXX fix */
   default:
      return -1;
   }
}


#define SWIZZLE_ZWWW MAKE_SWIZZLE4(SWIZZLE_Z, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W)

/**
 * Return the VERT_ATTRIB_* or FRAG_ATTRIB_* value that corresponds to
 * a vertex or fragment program input variable.  Return -1 if the input
 * name is invalid.
 * XXX return size too
 */
static GLint
_slang_input_index(const char *name, GLenum target, GLuint *swizzleOut)
{
   struct input_info {
      const char *Name;
      GLuint Attrib;
      GLuint Swizzle;
   };
   static const struct input_info vertInputs[] = {
      { "gl_Vertex", VERT_ATTRIB_POS, SWIZZLE_NOOP },
      { "gl_Normal", VERT_ATTRIB_NORMAL, SWIZZLE_NOOP },
      { "gl_Color", VERT_ATTRIB_COLOR0, SWIZZLE_NOOP },
      { "gl_SecondaryColor", VERT_ATTRIB_COLOR1, SWIZZLE_NOOP },
      { "gl_FogCoord", VERT_ATTRIB_FOG, SWIZZLE_XXXX },
      { "gl_MultiTexCoord0", VERT_ATTRIB_TEX0, SWIZZLE_NOOP },
      { "gl_MultiTexCoord1", VERT_ATTRIB_TEX1, SWIZZLE_NOOP },
      { "gl_MultiTexCoord2", VERT_ATTRIB_TEX2, SWIZZLE_NOOP },
      { "gl_MultiTexCoord3", VERT_ATTRIB_TEX3, SWIZZLE_NOOP },
      { "gl_MultiTexCoord4", VERT_ATTRIB_TEX4, SWIZZLE_NOOP },
      { "gl_MultiTexCoord5", VERT_ATTRIB_TEX5, SWIZZLE_NOOP },
      { "gl_MultiTexCoord6", VERT_ATTRIB_TEX6, SWIZZLE_NOOP },
      { "gl_MultiTexCoord7", VERT_ATTRIB_TEX7, SWIZZLE_NOOP },
      { NULL, 0, SWIZZLE_NOOP }
   };
   static const struct input_info fragInputs[] = {
      { "gl_FragCoord", FRAG_ATTRIB_WPOS, SWIZZLE_NOOP },
      { "gl_Color", FRAG_ATTRIB_COL0, SWIZZLE_NOOP },
      { "gl_SecondaryColor", FRAG_ATTRIB_COL1, SWIZZLE_NOOP },
      { "gl_TexCoord", FRAG_ATTRIB_TEX0, SWIZZLE_NOOP },
      /* note: we're packing several quantities into the fogcoord vector */
      { "gl_FogFragCoord", FRAG_ATTRIB_FOGC, SWIZZLE_XXXX },
      { "gl_FrontFacing", FRAG_ATTRIB_FOGC, SWIZZLE_YYYY }, /*XXX*/
      { "gl_PointCoord", FRAG_ATTRIB_FOGC, SWIZZLE_ZWWW },
      { NULL, 0, SWIZZLE_NOOP }
   };
   GLuint i;
   const struct input_info *inputs
      = (target == GL_VERTEX_PROGRAM_ARB) ? vertInputs : fragInputs;

   ASSERT(MAX_TEXTURE_COORD_UNITS == 8); /* if this fails, fix vertInputs above */

   for (i = 0; inputs[i].Name; i++) {
      if (strcmp(inputs[i].Name, name) == 0) {
         /* found */
         *swizzleOut = inputs[i].Swizzle;
         return inputs[i].Attrib;
      }
   }
   return -1;
}


/**
 * Return the VERT_RESULT_* or FRAG_RESULT_* value that corresponds to
 * a vertex or fragment program output variable.  Return -1 for an invalid
 * output name.
 */
static GLint
_slang_output_index(const char *name, GLenum target)
{
   struct output_info {
      const char *Name;
      GLuint Attrib;
   };
   static const struct output_info vertOutputs[] = {
      { "gl_Position", VERT_RESULT_HPOS },
      { "gl_FrontColor", VERT_RESULT_COL0 },
      { "gl_BackColor", VERT_RESULT_BFC0 },
      { "gl_FrontSecondaryColor", VERT_RESULT_COL1 },
      { "gl_BackSecondaryColor", VERT_RESULT_BFC1 },
      { "gl_TexCoord", VERT_RESULT_TEX0 },
      { "gl_FogFragCoord", VERT_RESULT_FOGC },
      { "gl_PointSize", VERT_RESULT_PSIZ },
      { NULL, 0 }
   };
   static const struct output_info fragOutputs[] = {
      { "gl_FragColor", FRAG_RESULT_COLR },
      { "gl_FragDepth", FRAG_RESULT_DEPR },
      { "gl_FragData", FRAG_RESULT_DATA0 },
      { NULL, 0 }
   };
   GLuint i;
   const struct output_info *outputs
      = (target == GL_VERTEX_PROGRAM_ARB) ? vertOutputs : fragOutputs;

   for (i = 0; outputs[i].Name; i++) {
      if (strcmp(outputs[i].Name, name) == 0) {
         /* found */
         return outputs[i].Attrib;
      }
   }
   return -1;
}



/**********************************************************************/


/**
 * Map "_asm foo" to IR_FOO, etc.
 */
typedef struct
{
   const char *Name;
   slang_ir_opcode Opcode;
   GLuint HaveRetValue, NumParams;
} slang_asm_info;


static slang_asm_info AsmInfo[] = {
   /* vec4 binary op */
   { "vec4_add", IR_ADD, 1, 2 },
   { "vec4_subtract", IR_SUB, 1, 2 },
   { "vec4_multiply", IR_MUL, 1, 2 },
   { "vec4_dot", IR_DOT4, 1, 2 },
   { "vec3_dot", IR_DOT3, 1, 2 },
   { "vec2_dot", IR_DOT2, 1, 2 },
   { "vec3_nrm", IR_NRM3, 1, 1 },
   { "vec4_nrm", IR_NRM4, 1, 1 },
   { "vec3_cross", IR_CROSS, 1, 2 },
   { "vec4_lrp", IR_LRP, 1, 3 },
   { "vec4_min", IR_MIN, 1, 2 },
   { "vec4_max", IR_MAX, 1, 2 },
   { "vec4_clamp", IR_CLAMP, 1, 3 },
   { "vec4_seq", IR_SEQUAL, 1, 2 },
   { "vec4_sne", IR_SNEQUAL, 1, 2 },
   { "vec4_sge", IR_SGE, 1, 2 },
   { "vec4_sgt", IR_SGT, 1, 2 },
   { "vec4_sle", IR_SLE, 1, 2 },
   { "vec4_slt", IR_SLT, 1, 2 },
   /* vec4 unary */
   { "vec4_move", IR_MOVE, 1, 1 },
   { "vec4_floor", IR_FLOOR, 1, 1 },
   { "vec4_frac", IR_FRAC, 1, 1 },
   { "vec4_abs", IR_ABS, 1, 1 },
   { "vec4_negate", IR_NEG, 1, 1 },
   { "vec4_ddx", IR_DDX, 1, 1 },
   { "vec4_ddy", IR_DDY, 1, 1 },
   /* float binary op */
   { "float_power", IR_POW, 1, 2 },
   /* texture / sampler */
   { "vec4_tex1d", IR_TEX, 1, 2 },
   { "vec4_texb1d", IR_TEXB, 1, 2 },  /* 1d w/ bias */
   { "vec4_texp1d", IR_TEXP, 1, 2 },  /* 1d w/ projection */
   { "vec4_tex2d", IR_TEX, 1, 2 },
   { "vec4_texb2d", IR_TEXB, 1, 2 },  /* 2d w/ bias */
   { "vec4_texp2d", IR_TEXP, 1, 2 },  /* 2d w/ projection */
   { "vec4_tex3d", IR_TEX, 1, 2 },
   { "vec4_texb3d", IR_TEXB, 1, 2 },  /* 3d w/ bias */
   { "vec4_texp3d", IR_TEXP, 1, 2 },  /* 3d w/ projection */
   { "vec4_texcube", IR_TEX, 1, 2 },  /* cubemap */
   { "vec4_tex_rect", IR_TEX, 1, 2 }, /* rectangle */
   { "vec4_texp_rect", IR_TEXP, 1, 2 },/* rectangle w/ projection */

   /* unary op */
   { "ivec4_to_vec4", IR_I_TO_F, 1, 1 }, /* int[4] to float[4] */
   { "vec4_to_ivec4", IR_F_TO_I, 1, 1 },  /* float[4] to int[4] */
   { "float_exp", IR_EXP, 1, 1 },
   { "float_exp2", IR_EXP2, 1, 1 },
   { "float_log2", IR_LOG2, 1, 1 },
   { "float_rsq", IR_RSQ, 1, 1 },
   { "float_rcp", IR_RCP, 1, 1 },
   { "float_sine", IR_SIN, 1, 1 },
   { "float_cosine", IR_COS, 1, 1 },
   { "float_noise1", IR_NOISE1, 1, 1},
   { "float_noise2", IR_NOISE2, 1, 1},
   { "float_noise3", IR_NOISE3, 1, 1},
   { "float_noise4", IR_NOISE4, 1, 1},

   { NULL, IR_NOP, 0, 0 }
};


static slang_ir_node *
new_node3(slang_ir_opcode op,
          slang_ir_node *c0, slang_ir_node *c1, slang_ir_node *c2)
{
   slang_ir_node *n = (slang_ir_node *) _slang_alloc(sizeof(slang_ir_node));
   if (n) {
      n->Opcode = op;
      n->Children[0] = c0;
      n->Children[1] = c1;
      n->Children[2] = c2;
      n->InstLocation = -1;
   }
   return n;
}

static slang_ir_node *
new_node2(slang_ir_opcode op, slang_ir_node *c0, slang_ir_node *c1)
{
   return new_node3(op, c0, c1, NULL);
}

static slang_ir_node *
new_node1(slang_ir_opcode op, slang_ir_node *c0)
{
   return new_node3(op, c0, NULL, NULL);
}

static slang_ir_node *
new_node0(slang_ir_opcode op)
{
   return new_node3(op, NULL, NULL, NULL);
}


/**
 * Create sequence of two nodes.
 */
static slang_ir_node *
new_seq(slang_ir_node *left, slang_ir_node *right)
{
   if (!left)
      return right;
   if (!right)
      return left;
   return new_node2(IR_SEQ, left, right);
}

static slang_ir_node *
new_label(slang_label *label)
{
   slang_ir_node *n = new_node0(IR_LABEL);
   assert(label);
   if (n)
      n->Label = label;
   return n;
}

static slang_ir_node *
new_float_literal(const float v[4], GLuint size)
{
   slang_ir_node *n = new_node0(IR_FLOAT);
   assert(size <= 4);
   COPY_4V(n->Value, v);
   /* allocate a storage object, but compute actual location (Index) later */
   n->Store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, size);
   return n;
}


static slang_ir_node *
new_not(slang_ir_node *n)
{
   return new_node1(IR_NOT, n);
}


/**
 * Non-inlined function call.
 */
static slang_ir_node *
new_function_call(slang_ir_node *code, slang_label *name)
{
   slang_ir_node *n = new_node1(IR_CALL, code);
   assert(name);
   if (n)
      n->Label = name;
   return n;
}


/**
 * Unconditional jump.
 */
static slang_ir_node *
new_return(slang_label *dest)
{
   slang_ir_node *n = new_node0(IR_RETURN);
   assert(dest);
   if (n)
      n->Label = dest;
   return n;
}


static slang_ir_node *
new_loop(slang_ir_node *body)
{
   return new_node1(IR_LOOP, body);
}


static slang_ir_node *
new_break(slang_ir_node *loopNode)
{
   slang_ir_node *n = new_node0(IR_BREAK);
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   if (n) {
      /* insert this node at head of linked list */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


/**
 * Make new IR_BREAK_IF_TRUE.
 */
static slang_ir_node *
new_break_if_true(slang_ir_node *loopNode, slang_ir_node *cond)
{
   slang_ir_node *n;
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   n = new_node1(IR_BREAK_IF_TRUE, cond);
   if (n) {
      /* insert this node at head of linked list */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


/**
 * Make new IR_CONT_IF_TRUE node.
 */
static slang_ir_node *
new_cont_if_true(slang_ir_node *loopNode, slang_ir_node *cond)
{
   slang_ir_node *n;
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   n = new_node1(IR_CONT_IF_TRUE, cond);
   if (n) {
      /* insert this node at head of linked list */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


static slang_ir_node *
new_cond(slang_ir_node *n)
{
   slang_ir_node *c = new_node1(IR_COND, n);
   return c;
}


static slang_ir_node *
new_if(slang_ir_node *cond, slang_ir_node *ifPart, slang_ir_node *elsePart)
{
   return new_node3(IR_IF, cond, ifPart, elsePart);
}


/**
 * New IR_VAR node - a reference to a previously declared variable.
 */
static slang_ir_node *
new_var(slang_assemble_ctx *A, slang_variable *var)
{
   slang_ir_node *n = new_node0(IR_VAR);
   if (n) {
      _slang_attach_storage(n, var);
   }
   return n;
}


/**
 * Check if the given function is really just a wrapper for a
 * basic assembly instruction.
 */
static GLboolean
slang_is_asm_function(const slang_function *fun)
{
   if (fun->body->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE &&
       fun->body->num_children == 1 &&
       fun->body->children[0].type == SLANG_OPER_ASM) {
      return GL_TRUE;
   }
   return GL_FALSE;
}


static GLboolean
_slang_is_noop(const slang_operation *oper)
{
   if (!oper ||
       oper->type == SLANG_OPER_VOID ||
       (oper->num_children == 1 && oper->children[0].type == SLANG_OPER_VOID))
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 * Recursively search tree for a node of the given type.
 */
static slang_operation *
_slang_find_node_type(slang_operation *oper, slang_operation_type type)
{
   GLuint i;
   if (oper->type == type)
      return oper;
   for (i = 0; i < oper->num_children; i++) {
      slang_operation *p = _slang_find_node_type(&oper->children[i], type);
      if (p)
         return p;
   }
   return NULL;
}


/**
 * Count the number of operations of the given time rooted at 'oper'.
 */
static GLuint
_slang_count_node_type(slang_operation *oper, slang_operation_type type)
{
   GLuint i, count = 0;
   if (oper->type == type) {
      return 1;
   }
   for (i = 0; i < oper->num_children; i++) {
      count += _slang_count_node_type(&oper->children[i], type);
   }
   return count;
}


/**
 * Check if the 'return' statement found under 'oper' is a "tail return"
 * that can be no-op'd.  For example:
 *
 * void func(void)
 * {
 *    .. do something ..
 *    return;   // this is a no-op
 * }
 *
 * This is used when determining if a function can be inlined.  If the
 * 'return' is not the last statement, we can't inline the function since
 * we still need the semantic behaviour of the 'return' but we don't want
 * to accidentally return from the _calling_ function.  We'd need to use an
 * unconditional branch, but we don't have such a GPU instruction (not
 * always, at least).
 */
static GLboolean
_slang_is_tail_return(const slang_operation *oper)
{
   GLuint k = oper->num_children;

   while (k > 0) {
      const slang_operation *last = &oper->children[k - 1];
      if (last->type == SLANG_OPER_RETURN)
         return GL_TRUE;
      else if (last->type == SLANG_OPER_IDENTIFIER ||
               last->type == SLANG_OPER_LABEL)
         k--; /* try prev child */
      else if (last->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE ||
               last->type == SLANG_OPER_BLOCK_NEW_SCOPE)
         /* try sub-children */
         return _slang_is_tail_return(last);
      else
         break;
   }

   return GL_FALSE;
}


static void
slang_resolve_variable(slang_operation *oper)
{
   if (oper->type == SLANG_OPER_IDENTIFIER && !oper->var) {
      oper->var = _slang_variable_locate(oper->locals, oper->a_id, GL_TRUE);
   }
}


/**
 * Replace particular variables (SLANG_OPER_IDENTIFIER) with new expressions.
 */
static void
slang_substitute(slang_assemble_ctx *A, slang_operation *oper,
                 GLuint substCount, slang_variable **substOld,
		 slang_operation **substNew, GLboolean isLHS)
{
   switch (oper->type) {
   case SLANG_OPER_VARIABLE_DECL:
      {
         slang_variable *v = _slang_variable_locate(oper->locals,
                                                    oper->a_id, GL_TRUE);
         assert(v);
         if (v->initializer && oper->num_children == 0) {
            /* set child of oper to copy of initializer */
            oper->num_children = 1;
            oper->children = slang_operation_new(1);
            slang_operation_copy(&oper->children[0], v->initializer);
         }
         if (oper->num_children == 1) {
            /* the initializer */
            slang_substitute(A, &oper->children[0], substCount,
                             substOld, substNew, GL_FALSE);
         }
      }
      break;
   case SLANG_OPER_IDENTIFIER:
      assert(oper->num_children == 0);
      if (1/**!isLHS XXX FIX */) {
         slang_atom id = oper->a_id;
         slang_variable *v;
	 GLuint i;
         v = _slang_variable_locate(oper->locals, id, GL_TRUE);
	 if (!v) {
            _mesa_problem(NULL, "var %s not found!\n", (char *) oper->a_id);
            return;
	 }

	 /* look for a substitution */
	 for (i = 0; i < substCount; i++) {
	    if (v == substOld[i]) {
               /* OK, replace this SLANG_OPER_IDENTIFIER with a new expr */
#if 0 /* DEBUG only */
	       if (substNew[i]->type == SLANG_OPER_IDENTIFIER) {
                  assert(substNew[i]->var);
                  assert(substNew[i]->var->a_name);
		  printf("Substitute %s with %s in id node %p\n",
			 (char*)v->a_name, (char*) substNew[i]->var->a_name,
			 (void*) oper);
               }
	       else {
		  printf("Substitute %s with %f in id node %p\n",
			 (char*)v->a_name, substNew[i]->literal[0],
			 (void*) oper);
               }
#endif
	       slang_operation_copy(oper, substNew[i]);
	       break;
	    }
	 }
      }
      break;

   case SLANG_OPER_RETURN:
      /* do return replacement here too */
      assert(oper->num_children == 0 || oper->num_children == 1);
      if (oper->num_children == 1 && !_slang_is_noop(&oper->children[0])) {
         /* replace:
          *   return expr;
          * with:
          *   __retVal = expr;
          *   return;
          * then do substitutions on the assignment.
          */
         slang_operation *blockOper, *assignOper, *returnOper;

         /* check if function actually has a return type */
         assert(A->CurFunction);
         if (A->CurFunction->header.type.specifier.type == SLANG_SPEC_VOID) {
            slang_info_log_error(A->log, "illegal return expression");
            return;
         }

         blockOper = slang_operation_new(1);
         blockOper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;
         blockOper->num_children = 2;
         blockOper->locals->outer_scope = oper->locals->outer_scope;
         blockOper->children = slang_operation_new(2);
         assignOper = blockOper->children + 0;
         returnOper = blockOper->children + 1;

         assignOper->type = SLANG_OPER_ASSIGN;
         assignOper->num_children = 2;
         assignOper->locals->outer_scope = blockOper->locals;
         assignOper->children = slang_operation_new(2);
         assignOper->children[0].type = SLANG_OPER_IDENTIFIER;
         assignOper->children[0].a_id = slang_atom_pool_atom(A->atoms, "__retVal");
         assignOper->children[0].locals->outer_scope = assignOper->locals;

         slang_operation_copy(&assignOper->children[1],
                              &oper->children[0]);

         returnOper->type = SLANG_OPER_RETURN; /* return w/ no value */
         assert(returnOper->num_children == 0);

         /* do substitutions on the "__retVal = expr" sub-tree */
         slang_substitute(A, assignOper,
                          substCount, substOld, substNew, GL_FALSE);

         /* install new code */
         slang_operation_copy(oper, blockOper);
         slang_operation_destruct(blockOper);
      }
      else {
         /* check if return value was expected */
         assert(A->CurFunction);
         if (A->CurFunction->header.type.specifier.type != SLANG_SPEC_VOID) {
            slang_info_log_error(A->log, "return statement requires an expression");
            return;
         }
      }
      break;

   case SLANG_OPER_ASSIGN:
   case SLANG_OPER_SUBSCRIPT:
      /* special case:
       * child[0] can't have substitutions but child[1] can.
       */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      slang_substitute(A, &oper->children[1],
                       substCount, substOld, substNew, GL_FALSE);
      break;
   case SLANG_OPER_FIELD:
      /* XXX NEW - test */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      break;
   default:
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) 
            slang_substitute(A, &oper->children[i],
                             substCount, substOld, substNew, GL_FALSE);
      }
   }
}


/**
 * Produce inline code for a call to an assembly instruction.
 * This is typically used to compile a call to a built-in function like this:
 *
 * vec4 mix(const vec4 x, const vec4 y, const vec4 a)
 * {
 *    __asm vec4_lrp __retVal, a, y, x;
 * }
 *
 *
 * A call to
 *     r = mix(p1, p2, p3);
 *
 * Becomes:
 *
 *              mov
 *             /   \
 *            r   vec4_lrp
 *                 /  |  \
 *                p3  p2  p1
 *
 * We basically translate a SLANG_OPER_CALL into a SLANG_OPER_ASM.
 */
static slang_operation *
slang_inline_asm_function(slang_assemble_ctx *A,
                          slang_function *fun, slang_operation *oper)
{
   const GLuint numArgs = oper->num_children;
   GLuint i;
   slang_operation *inlined;
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   slang_variable **substOld;
   slang_operation **substNew;

   ASSERT(slang_is_asm_function(fun));
   ASSERT(fun->param_count == numArgs + haveRetValue);

   /*
   printf("Inline %s as %s\n",
          (char*) fun->header.a_name,
          (char*) fun->body->children[0].a_id);
   */

   /*
    * We'll substitute formal params with actual args in the asm call.
    */
   substOld = (slang_variable **)
      _slang_alloc(numArgs * sizeof(slang_variable *));
   substNew = (slang_operation **)
      _slang_alloc(numArgs * sizeof(slang_operation *));
   for (i = 0; i < numArgs; i++) {
      substOld[i] = fun->parameters->variables[i];
      substNew[i] = oper->children + i;
   }

   /* make a copy of the code to inline */
   inlined = slang_operation_new(1);
   slang_operation_copy(inlined, &fun->body->children[0]);
   if (haveRetValue) {
      /* get rid of the __retVal child */
      inlined->num_children--;
      for (i = 0; i < inlined->num_children; i++) {
         inlined->children[i] = inlined->children[i + 1];
      }
   }

   /* now do formal->actual substitutions */
   slang_substitute(A, inlined, numArgs, substOld, substNew, GL_FALSE);

   _slang_free(substOld);
   _slang_free(substNew);

#if 0
   printf("+++++++++++++ inlined asm function %s +++++++++++++\n",
          (char *) fun->header.a_name);
   slang_print_tree(inlined, 3);
   printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
#endif

   return inlined;
}


/**
 * Inline the given function call operation.
 * Return a new slang_operation that corresponds to the inlined code.
 */
static slang_operation *
slang_inline_function_call(slang_assemble_ctx * A, slang_function *fun,
			   slang_operation *oper, slang_operation *returnOper)
{
   typedef enum {
      SUBST = 1,
      COPY_IN,
      COPY_OUT
   } ParamMode;
   ParamMode *paramMode;
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   const GLuint numArgs = oper->num_children;
   const GLuint totalArgs = numArgs + haveRetValue;
   slang_operation *args = oper->children;
   slang_operation *inlined, *top;
   slang_variable **substOld;
   slang_operation **substNew;
   GLuint substCount, numCopyIn, i;
   slang_function *prevFunction;
   slang_variable_scope *newScope = NULL;

   /* save / push */
   prevFunction = A->CurFunction;
   A->CurFunction = fun;

   /*assert(oper->type == SLANG_OPER_CALL); (or (matrix) multiply, etc) */
   assert(fun->param_count == totalArgs);

   /* allocate temporary arrays */
   paramMode = (ParamMode *)
      _slang_alloc(totalArgs * sizeof(ParamMode));
   substOld = (slang_variable **)
      _slang_alloc(totalArgs * sizeof(slang_variable *));
   substNew = (slang_operation **)
      _slang_alloc(totalArgs * sizeof(slang_operation *));

#if 0
   printf("\nInline call to %s  (total vars=%d  nparams=%d)\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);
#endif

   if (haveRetValue && !returnOper) {
      /* Create 3-child comma sequence for inlined code:
       * child[0]:  declare __resultTmp
       * child[1]:  inlined function body
       * child[2]:  __resultTmp
       */
      slang_operation *commaSeq;
      slang_operation *declOper = NULL;
      slang_variable *resultVar;

      commaSeq = slang_operation_new(1);
      commaSeq->type = SLANG_OPER_SEQUENCE;
      assert(commaSeq->locals);
      commaSeq->locals->outer_scope = oper->locals->outer_scope;
      commaSeq->num_children = 3;
      commaSeq->children = slang_operation_new(3);
      /* allocate the return var */
      resultVar = slang_variable_scope_grow(commaSeq->locals);
      /*
      printf("Alloc __resultTmp in scope %p for retval of calling %s\n",
             (void*)commaSeq->locals, (char *) fun->header.a_name);
      */

      resultVar->a_name = slang_atom_pool_atom(A->atoms, "__resultTmp");
      resultVar->type = fun->header.type; /* XXX copy? */
      resultVar->isTemp = GL_TRUE;

      /* child[0] = __resultTmp declaration */
      declOper = &commaSeq->children[0];
      declOper->type = SLANG_OPER_VARIABLE_DECL;
      declOper->a_id = resultVar->a_name;
      declOper->locals->outer_scope = commaSeq->locals;

      /* child[1] = function body */
      inlined = &commaSeq->children[1];
      inlined->locals->outer_scope = commaSeq->locals;

      /* child[2] = __resultTmp reference */
      returnOper = &commaSeq->children[2];
      returnOper->type = SLANG_OPER_IDENTIFIER;
      returnOper->a_id = resultVar->a_name;
      returnOper->locals->outer_scope = commaSeq->locals;

      top = commaSeq;
   }
   else {
      top = inlined = slang_operation_new(1);
      /* XXXX this may be inappropriate!!!! */
      inlined->locals->outer_scope = oper->locals->outer_scope;
   }


   assert(inlined->locals);

   /* Examine the parameters, look for inout/out params, look for possible
    * substitutions, etc:
    *    param type      behaviour
    *     in             copy actual to local
    *     const in       substitute param with actual
    *     out            copy out
    */
   substCount = 0;
   for (i = 0; i < totalArgs; i++) {
      slang_variable *p = fun->parameters->variables[i];
      /*
      printf("Param %d: %s %s \n", i,
             slang_type_qual_string(p->type.qualifier),
	     (char *) p->a_name);
      */
      if (p->type.qualifier == SLANG_QUAL_INOUT ||
	  p->type.qualifier == SLANG_QUAL_OUT) {
	 /* an output param */
         slang_operation *arg;
         if (i < numArgs)
            arg = &args[i];
         else
            arg = returnOper;
	 paramMode[i] = SUBST;

	 if (arg->type == SLANG_OPER_IDENTIFIER)
            slang_resolve_variable(arg);

         /* replace parameter 'p' with argument 'arg' */
	 substOld[substCount] = p;
	 substNew[substCount] = arg; /* will get copied */
	 substCount++;
      }
      else if (p->type.qualifier == SLANG_QUAL_CONST) {
	 /* a constant input param */
	 if (args[i].type == SLANG_OPER_IDENTIFIER ||
	     args[i].type == SLANG_OPER_LITERAL_FLOAT) {
	    /* replace all occurances of this parameter variable with the
	     * actual argument variable or a literal.
	     */
	    paramMode[i] = SUBST;
            slang_resolve_variable(&args[i]);
	    substOld[substCount] = p;
	    substNew[substCount] = &args[i]; /* will get copied */
	    substCount++;
	 }
	 else {
	    paramMode[i] = COPY_IN;
	 }
      }
      else {
	 paramMode[i] = COPY_IN;
      }
      assert(paramMode[i]);
   }

   /* actual code inlining: */
   slang_operation_copy(inlined, fun->body);

   /*** XXX review this */
   assert(inlined->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE ||
          inlined->type == SLANG_OPER_BLOCK_NEW_SCOPE);
   inlined->type = SLANG_OPER_BLOCK_NEW_SCOPE;

#if 0
   printf("======================= orig body code ======================\n");
   printf("=== params scope = %p\n", (void*) fun->parameters);
   slang_print_tree(fun->body, 8);
   printf("======================= copied code =========================\n");
   slang_print_tree(inlined, 8);
#endif

   /* do parameter substitution in inlined code: */
   slang_substitute(A, inlined, substCount, substOld, substNew, GL_FALSE);

#if 0
   printf("======================= subst code ==========================\n");
   slang_print_tree(inlined, 8);
   printf("=============================================================\n");
#endif

   /* New prolog statements: (inserted before the inlined code)
    * Copy the 'in' arguments.
    */
   numCopyIn = 0;
   for (i = 0; i < numArgs; i++) {
      if (paramMode[i] == COPY_IN) {
	 slang_variable *p = fun->parameters->variables[i];
	 /* declare parameter 'p' */
	 slang_operation *decl = slang_operation_insert(&inlined->num_children,
							&inlined->children,
							numCopyIn);

	 decl->type = SLANG_OPER_VARIABLE_DECL;
         assert(decl->locals);
         decl->locals->outer_scope = inlined->locals;
	 decl->a_id = p->a_name;
	 decl->num_children = 1;
	 decl->children = slang_operation_new(1);

         /* child[0] is the var's initializer */
         slang_operation_copy(&decl->children[0], args + i);

         /* add parameter 'p' to the local variable scope here */
         {
            slang_variable *pCopy = slang_variable_scope_grow(inlined->locals);
            pCopy->type = p->type;
            pCopy->a_name = p->a_name;
            pCopy->array_len = p->array_len;
         }

         newScope = inlined->locals;
	 numCopyIn++;
      }
   }

   /* Now add copies of the function's local vars to the new variable scope */
   for (i = totalArgs; i < fun->parameters->num_variables; i++) {
      slang_variable *p = fun->parameters->variables[i];
      slang_variable *pCopy = slang_variable_scope_grow(inlined->locals);
      pCopy->type = p->type;
      pCopy->a_name = p->a_name;
      pCopy->array_len = p->array_len;
   }


   /* New epilog statements:
    * 1. Create end of function label to jump to from return statements.
    * 2. Copy the 'out' parameter vars
    */
   {
      slang_operation *lab = slang_operation_insert(&inlined->num_children,
                                                    &inlined->children,
                                                    inlined->num_children);
      lab->type = SLANG_OPER_LABEL;
      lab->label = A->curFuncEndLabel;
   }

   for (i = 0; i < totalArgs; i++) {
      if (paramMode[i] == COPY_OUT) {
	 const slang_variable *p = fun->parameters->variables[i];
	 /* actualCallVar = outParam */
	 /*if (i > 0 || !haveRetValue)*/
	 slang_operation *ass = slang_operation_insert(&inlined->num_children,
						       &inlined->children,
						       inlined->num_children);
	 ass->type = SLANG_OPER_ASSIGN;
	 ass->num_children = 2;
         ass->locals->outer_scope = inlined->locals;
	 ass->children = slang_operation_new(2);
	 ass->children[0] = args[i]; /*XXX copy */
	 ass->children[1].type = SLANG_OPER_IDENTIFIER;
	 ass->children[1].a_id = p->a_name;
         ass->children[1].locals->outer_scope = ass->locals;
      }
   }

   _slang_free(paramMode);
   _slang_free(substOld);
   _slang_free(substNew);

   /* Update scoping to use the new local vars instead of the
    * original function's vars.  This is especially important
    * for nested inlining.
    */
   if (newScope)
      slang_replace_scope(inlined, fun->parameters, newScope);

#if 0
   printf("Done Inline call to %s  (total vars=%d  nparams=%d)\n\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);
   slang_print_tree(top, 0);
#endif

   /* pop */
   A->CurFunction = prevFunction;

   return top;
}


static slang_ir_node *
_slang_gen_function_call(slang_assemble_ctx *A, slang_function *fun,
                         slang_operation *oper, slang_operation *dest)
{
   slang_ir_node *n;
   slang_operation *inlined;
   slang_label *prevFuncEndLabel;
   char name[200];

   prevFuncEndLabel = A->curFuncEndLabel;
   sprintf(name, "__endOfFunc_%s_", (char *) fun->header.a_name);
   A->curFuncEndLabel = _slang_label_new(name);
   assert(A->curFuncEndLabel);

   if (slang_is_asm_function(fun) && !dest) {
      /* assemble assembly function - tree style */
      inlined = slang_inline_asm_function(A, fun, oper);
   }
   else {
      /* non-assembly function */
      /* We always generate an "inline-able" block of code here.
       * We may either:
       *  1. insert the inline code
       *  2. Generate a call to the "inline" code as a subroutine
       */


      slang_operation *ret = NULL;

      inlined = slang_inline_function_call(A, fun, oper, dest);
      if (!inlined)
         return NULL;

      ret = _slang_find_node_type(inlined, SLANG_OPER_RETURN);
      if (ret) {
         /* check if this is a "tail" return */
         if (_slang_count_node_type(inlined, SLANG_OPER_RETURN) == 1 &&
             _slang_is_tail_return(inlined)) {
            /* The only RETURN is the last stmt in the function, no-op it
             * and inline the function body.
             */
            ret->type = SLANG_OPER_NONE;
         }
         else {
            slang_operation *callOper;
            /* The function we're calling has one or more 'return' statements.
             * So, we can't truly inline this function because we need to
             * implement 'return' with RET (and CAL).
             * Nevertheless, we performed "inlining" to make a new instance
             * of the function body to deal with static register allocation.
             *
             * XXX check if there's one 'return' and if it's the very last
             * statement in the function - we can optimize that case.
             */
            assert(inlined->type == SLANG_OPER_BLOCK_NEW_SCOPE ||
                   inlined->type == SLANG_OPER_SEQUENCE);

            if (_slang_function_has_return_value(fun) && !dest) {
               assert(inlined->children[0].type == SLANG_OPER_VARIABLE_DECL);
               assert(inlined->children[2].type == SLANG_OPER_IDENTIFIER);
               callOper = &inlined->children[1];
            }
            else {
               callOper = inlined;
            }
            callOper->type = SLANG_OPER_NON_INLINED_CALL;
            callOper->fun = fun;
            callOper->label = _slang_label_new_unique((char*) fun->header.a_name);
         }
      }
   }

   if (!inlined)
      return NULL;

   /* Replace the function call with the inlined block (or new CALL stmt) */
   slang_operation_destruct(oper);
   *oper = *inlined;
   _slang_free(inlined);

#if 0
   assert(inlined->locals);
   printf("*** Inlined code for call to %s:\n",
          (char*) fun->header.a_name);
   slang_print_tree(oper, 10);
   printf("\n");
#endif

   n = _slang_gen_operation(A, oper);

   /*_slang_label_delete(A->curFuncEndLabel);*/
   A->curFuncEndLabel = prevFuncEndLabel;

   return n;
}


static slang_asm_info *
slang_find_asm_info(const char *name)
{
   GLuint i;
   for (i = 0; AsmInfo[i].Name; i++) {
      if (_mesa_strcmp(AsmInfo[i].Name, name) == 0) {
         return AsmInfo + i;
      }
   }
   return NULL;
}


/**
 * Some write-masked assignments are simple, but others are hard.
 * Simple example:
 *    vec3 v;
 *    v.xy = vec2(a, b);
 * Hard example:
 *    vec3 v;
 *    v.zy = vec2(a, b);
 * this gets transformed/swizzled into:
 *    v.zy = vec2(a, b).*yx*         (* = don't care)
 * This function helps to determine simple vs. non-simple.
 */
static GLboolean
_slang_simple_writemask(GLuint writemask, GLuint swizzle)
{
   switch (writemask) {
   case WRITEMASK_X:
      return GET_SWZ(swizzle, 0) == SWIZZLE_X;
   case WRITEMASK_Y:
      return GET_SWZ(swizzle, 1) == SWIZZLE_Y;
   case WRITEMASK_Z:
      return GET_SWZ(swizzle, 2) == SWIZZLE_Z;
   case WRITEMASK_W:
      return GET_SWZ(swizzle, 3) == SWIZZLE_W;
   case WRITEMASK_XY:
      return (GET_SWZ(swizzle, 0) == SWIZZLE_X)
         && (GET_SWZ(swizzle, 1) == SWIZZLE_Y);
   case WRITEMASK_XYZ:
      return (GET_SWZ(swizzle, 0) == SWIZZLE_X)
         && (GET_SWZ(swizzle, 1) == SWIZZLE_Y)
         && (GET_SWZ(swizzle, 2) == SWIZZLE_Z);
   case WRITEMASK_XYZW:
      return swizzle == SWIZZLE_NOOP;
   default:
      return GL_FALSE;
   }
}


/**
 * Convert the given swizzle into a writemask.  In some cases this
 * is trivial, in other cases, we'll need to also swizzle the right
 * hand side to put components in the right places.
 * See comment above for more info.
 * XXX this function could be simplified and should probably be renamed.
 * \param swizzle  the incoming swizzle
 * \param writemaskOut  returns the writemask
 * \param swizzleOut  swizzle to apply to the right-hand-side
 * \return GL_FALSE for simple writemasks, GL_TRUE for non-simple
 */
static GLboolean
swizzle_to_writemask(slang_assemble_ctx *A, GLuint swizzle,
                     GLuint *writemaskOut, GLuint *swizzleOut)
{
   GLuint mask = 0x0, newSwizzle[4];
   GLint i, size;

   /* make new dst writemask, compute size */
   for (i = 0; i < 4; i++) {
      const GLuint swz = GET_SWZ(swizzle, i);
      if (swz == SWIZZLE_NIL) {
         /* end */
         break;
      }
      assert(swz >= 0 && swz <= 3);

      if (swizzle != SWIZZLE_XXXX &&
          swizzle != SWIZZLE_YYYY &&
          swizzle != SWIZZLE_ZZZZ &&
          swizzle != SWIZZLE_WWWW &&
          (mask & (1 << swz))) {
         /* a channel can't be specified twice (ex: ".xyyz") */
         slang_info_log_error(A->log, "Invalid writemask '%s'",
                              _mesa_swizzle_string(swizzle, 0, 0));
         return GL_FALSE;
      }

      mask |= (1 << swz);
   }
   assert(mask <= 0xf);
   size = i;  /* number of components in mask/swizzle */

   *writemaskOut = mask;

   /* make new src swizzle, by inversion */
   for (i = 0; i < 4; i++) {
      newSwizzle[i] = i; /*identity*/
   }
   for (i = 0; i < size; i++) {
      const GLuint swz = GET_SWZ(swizzle, i);
      newSwizzle[swz] = i;
   }
   *swizzleOut = MAKE_SWIZZLE4(newSwizzle[0],
                               newSwizzle[1],
                               newSwizzle[2],
                               newSwizzle[3]);

   if (_slang_simple_writemask(mask, *swizzleOut)) {
      if (size >= 1)
         assert(GET_SWZ(*swizzleOut, 0) == SWIZZLE_X);
      if (size >= 2)
         assert(GET_SWZ(*swizzleOut, 1) == SWIZZLE_Y);
      if (size >= 3)
         assert(GET_SWZ(*swizzleOut, 2) == SWIZZLE_Z);
      if (size >= 4)
         assert(GET_SWZ(*swizzleOut, 3) == SWIZZLE_W);
      return GL_TRUE;
   }
   else
      return GL_FALSE;
}


#if 0 /* not used, but don't remove just yet */
/**
 * Recursively traverse 'oper' to produce a swizzle mask in the event
 * of any vector subscripts and swizzle suffixes.
 * Ex:  for "vec4 v",  "v[2].x" resolves to v.z
 */
static GLuint
resolve_swizzle(const slang_operation *oper)
{
   if (oper->type == SLANG_OPER_FIELD) {
      /* writemask from .xyzw suffix */
      slang_swizzle swz;
      if (_slang_is_swizzle((char*) oper->a_id, 4, &swz)) {
         GLuint swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                                        swz.swizzle[1],
                                        swz.swizzle[2],
                                        swz.swizzle[3]);
         GLuint child_swizzle = resolve_swizzle(&oper->children[0]);
         GLuint s = _slang_swizzle_swizzle(child_swizzle, swizzle);
         return s;
      }
      else
         return SWIZZLE_XYZW;
   }
   else if (oper->type == SLANG_OPER_SUBSCRIPT &&
            oper->children[1].type == SLANG_OPER_LITERAL_INT) {
      /* writemask from [index] */
      GLuint child_swizzle = resolve_swizzle(&oper->children[0]);
      GLuint i = (GLuint) oper->children[1].literal[0];
      GLuint swizzle;
      GLuint s;
      switch (i) {
      case 0:
         swizzle = SWIZZLE_XXXX;
         break;
      case 1:
         swizzle = SWIZZLE_YYYY;
         break;
      case 2:
         swizzle = SWIZZLE_ZZZZ;
         break;
      case 3:
         swizzle = SWIZZLE_WWWW;
         break;
      default:
         swizzle = SWIZZLE_XYZW;
      }
      s = _slang_swizzle_swizzle(child_swizzle, swizzle);
      return s;
   }
   else {
      return SWIZZLE_XYZW;
   }
}
#endif


#if 0
/**
 * Recursively descend through swizzle nodes to find the node's storage info.
 */
static slang_ir_storage *
get_store(const slang_ir_node *n)
{
   if (n->Opcode == IR_SWIZZLE) {
      return get_store(n->Children[0]);
   }
   return n->Store;
}
#endif


/**
 * Generate IR tree for an asm instruction/operation such as:
 *    __asm vec4_dot __retVal.x, v1, v2;
 */
static slang_ir_node *
_slang_gen_asm(slang_assemble_ctx *A, slang_operation *oper,
               slang_operation *dest)
{
   const slang_asm_info *info;
   slang_ir_node *kids[3], *n;
   GLuint j, firstOperand;

   assert(oper->type == SLANG_OPER_ASM);

   info = slang_find_asm_info((char *) oper->a_id);
   if (!info) {
      _mesa_problem(NULL, "undefined __asm function %s\n",
                    (char *) oper->a_id);
      assert(info);
   }
   assert(info->NumParams <= 3);

   if (info->NumParams == oper->num_children) {
      /* Storage for result is not specified.
       * Children[0], [1], [2] are the operands.
       */
      firstOperand = 0;
   }
   else {
      /* Storage for result (child[0]) is specified.
       * Children[1], [2], [3] are the operands.
       */
      firstOperand = 1;
   }

   /* assemble child(ren) */
   kids[0] = kids[1] = kids[2] = NULL;
   for (j = 0; j < info->NumParams; j++) {
      kids[j] = _slang_gen_operation(A, &oper->children[firstOperand + j]);
      if (!kids[j])
         return NULL;
   }

   n = new_node3(info->Opcode, kids[0], kids[1], kids[2]);

   if (firstOperand) {
      /* Setup n->Store to be a particular location.  Otherwise, storage
       * for the result (a temporary) will be allocated later.
       */
      slang_operation *dest_oper;
      slang_ir_node *n0;

      dest_oper = &oper->children[0];

      n0 = _slang_gen_operation(A, dest_oper);
      if (!n0)
         return NULL;

      assert(!n->Store);
      n->Store = n0->Store;

      assert(n->Store->File != PROGRAM_UNDEFINED || n->Store->Parent);

      _slang_free(n0);
   }

   return n;
}


#if 0
static void
print_funcs(struct slang_function_scope_ *scope, const char *name)
{
   GLuint i;
   for (i = 0; i < scope->num_functions; i++) {
      slang_function *f = &scope->functions[i];
      if (!name || strcmp(name, (char*) f->header.a_name) == 0)
          printf("  %s (%d args)\n", name, f->param_count);

   }
   if (scope->outer_scope)
      print_funcs(scope->outer_scope, name);
}
#endif


/**
 * Find a function of the given name, taking 'numArgs' arguments.
 * This is the function we'll try to call when there is no exact match
 * between function parameters and call arguments.
 *
 * XXX we should really create a list of candidate functions and try
 * all of them...
 */
static slang_function *
_slang_find_function_by_argc(slang_function_scope *scope,
                             const char *name, int numArgs)
{
   while (scope) {
      GLuint i;
      for (i = 0; i < scope->num_functions; i++) {
         slang_function *f = &scope->functions[i];
         if (strcmp(name, (char*) f->header.a_name) == 0) {
            int haveRetValue = _slang_function_has_return_value(f);
            if (numArgs == f->param_count - haveRetValue)
               return f;
         }
      }
      scope = scope->outer_scope;
   }

   return NULL;
}


static slang_function *
_slang_find_function_by_max_argc(slang_function_scope *scope,
                                 const char *name)
{
   slang_function *maxFunc = NULL;
   GLuint maxArgs = 0;

   while (scope) {
      GLuint i;
      for (i = 0; i < scope->num_functions; i++) {
         slang_function *f = &scope->functions[i];
         if (strcmp(name, (char*) f->header.a_name) == 0) {
            if (f->param_count > maxArgs) {
               maxArgs = f->param_count;
               maxFunc = f;
            }
         }
      }
      scope = scope->outer_scope;
   }

   return maxFunc;
}


/**
 * Generate a new slang_function which is a constructor for a user-defined
 * struct type.
 */
static slang_function *
_slang_make_struct_constructor(slang_assemble_ctx *A, slang_struct *str)
{
   const GLint numFields = str->fields->num_variables;
   slang_function *fun = slang_function_new(SLANG_FUNC_CONSTRUCTOR);

   /* function header (name, return type) */
   fun->header.a_name = str->a_name;
   fun->header.type.qualifier = SLANG_QUAL_NONE;
   fun->header.type.specifier.type = SLANG_SPEC_STRUCT;
   fun->header.type.specifier._struct = str;

   /* function parameters (= struct's fields) */
   {
      GLint i;
      for (i = 0; i < numFields; i++) {
         /*
         printf("Field %d: %s\n", i, (char*) str->fields->variables[i]->a_name);
         */
         slang_variable *p = slang_variable_scope_grow(fun->parameters);
         *p = *str->fields->variables[i]; /* copy the variable and type */
         p->type.qualifier = SLANG_QUAL_CONST;
      }
      fun->param_count = fun->parameters->num_variables;
   }

   /* Add __retVal to params */
   {
      slang_variable *p = slang_variable_scope_grow(fun->parameters);
      slang_atom a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);
      p->a_name = a_retVal;
      p->type = fun->header.type;
      p->type.qualifier = SLANG_QUAL_OUT;
      fun->param_count++;
   }

   /* function body is:
    *    block:
    *       declare T;
    *       T.f1 = p1;
    *       T.f2 = p2;
    *       ...
    *       T.fn = pn;
    *       return T;
    */
   {
      slang_variable_scope *scope;
      slang_variable *var;
      GLint i;

      fun->body = slang_operation_new(1);
      fun->body->type = SLANG_OPER_BLOCK_NEW_SCOPE;
      fun->body->num_children = numFields + 2;
      fun->body->children = slang_operation_new(numFields + 2);

      scope = fun->body->locals;
      scope->outer_scope = fun->parameters;

      /* create local var 't' */
      var = slang_variable_scope_grow(scope);
      var->a_name = slang_atom_pool_atom(A->atoms, "t");
      var->type = fun->header.type;

      /* declare t */
      {
         slang_operation *decl;

         decl = &fun->body->children[0];
         decl->type = SLANG_OPER_VARIABLE_DECL;
         decl->locals = _slang_variable_scope_new(scope);
         decl->a_id = var->a_name;
      }

      /* assign params to fields of t */
      for (i = 0; i < numFields; i++) {
         slang_operation *assign = &fun->body->children[1 + i];

         assign->type = SLANG_OPER_ASSIGN;
         assign->locals = _slang_variable_scope_new(scope);
         assign->num_children = 2;
         assign->children = slang_operation_new(2);
         
         {
            slang_operation *lhs = &assign->children[0];

            lhs->type = SLANG_OPER_FIELD;
            lhs->locals = _slang_variable_scope_new(scope);
            lhs->num_children = 1;
            lhs->children = slang_operation_new(1);
            lhs->a_id = str->fields->variables[i]->a_name;

            lhs->children[0].type = SLANG_OPER_IDENTIFIER;
            lhs->children[0].a_id = var->a_name;
            lhs->children[0].locals = _slang_variable_scope_new(scope);

#if 0
            lhs->children[1].num_children = 1;
            lhs->children[1].children = slang_operation_new(1);
            lhs->children[1].children[0].type = SLANG_OPER_IDENTIFIER;
            lhs->children[1].children[0].a_id = str->fields->variables[i]->a_name;
            lhs->children[1].children->locals = _slang_variable_scope_new(scope);
#endif
         }

         {
            slang_operation *rhs = &assign->children[1];

            rhs->type = SLANG_OPER_IDENTIFIER;
            rhs->locals = _slang_variable_scope_new(scope);
            rhs->a_id = str->fields->variables[i]->a_name;
         }         
      }

      /* return t; */
      {
         slang_operation *ret = &fun->body->children[numFields + 1];

         ret->type = SLANG_OPER_RETURN;
         ret->locals = _slang_variable_scope_new(scope);
         ret->num_children = 1;
         ret->children = slang_operation_new(1);
         ret->children[0].type = SLANG_OPER_IDENTIFIER;
         ret->children[0].a_id = var->a_name;
         ret->children[0].locals = _slang_variable_scope_new(scope);
      }
   }
   /*
   slang_print_function(fun, 1);
   */
   return fun;
}


/**
 * Find/create a function (constructor) for the given structure name.
 */
static slang_function *
_slang_locate_struct_constructor(slang_assemble_ctx *A, const char *name)
{
   unsigned int i;
   for (i = 0; i < A->space.structs->num_structs; i++) {
      slang_struct *str = &A->space.structs->structs[i];
      if (strcmp(name, (const char *) str->a_name) == 0) {
         /* found a structure type that matches the function name */
         if (!str->constructor) {
            /* create the constructor function now */
            str->constructor = _slang_make_struct_constructor(A, str);
         }
         return str->constructor;
      }
   }
   return NULL;
}


/**
 * Generate a new slang_function to satisfy a call to an array constructor.
 * Ex:  float[3](1., 2., 3.)
 */
static slang_function *
_slang_make_array_constructor(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_type_specifier_type baseType;
   slang_function *fun;
   int num_elements;

   fun = slang_function_new(SLANG_FUNC_CONSTRUCTOR);
   if (!fun)
      return NULL;

   baseType = slang_type_specifier_type_from_string((char *) oper->a_id);

   num_elements = oper->num_children;

   /* function header, return type */
   {
      fun->header.a_name = oper->a_id;
      fun->header.type.qualifier = SLANG_QUAL_NONE;
      fun->header.type.specifier.type = SLANG_SPEC_ARRAY;
      fun->header.type.specifier._array =
         slang_type_specifier_new(baseType, NULL, NULL);
      fun->header.type.array_len = num_elements;
   }

   /* function parameters (= number of elements) */
   {
      GLint i;
      for (i = 0; i < num_elements; i++) {
         /*
         printf("Field %d: %s\n", i, (char*) str->fields->variables[i]->a_name);
         */
         slang_variable *p = slang_variable_scope_grow(fun->parameters);
         char name[10];
         _mesa_snprintf(name, sizeof(name), "p%d", i);
         p->a_name = slang_atom_pool_atom(A->atoms, name);
         p->type.qualifier = SLANG_QUAL_CONST;
         p->type.specifier.type = baseType;
      }
      fun->param_count = fun->parameters->num_variables;
   }

   /* Add __retVal to params */
   {
      slang_variable *p = slang_variable_scope_grow(fun->parameters);
      slang_atom a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);
      p->a_name = a_retVal;
      p->type = fun->header.type;
      p->type.qualifier = SLANG_QUAL_OUT;
      p->type.specifier.type = baseType;
      fun->param_count++;
   }

   /* function body is:
    *    block:
    *       declare T;
    *       T[0] = p0;
    *       T[1] = p1;
    *       ...
    *       T[n] = pn;
    *       return T;
    */
   {
      slang_variable_scope *scope;
      slang_variable *var;
      GLint i;

      fun->body = slang_operation_new(1);
      fun->body->type = SLANG_OPER_BLOCK_NEW_SCOPE;
      fun->body->num_children = num_elements + 2;
      fun->body->children = slang_operation_new(num_elements + 2);

      scope = fun->body->locals;
      scope->outer_scope = fun->parameters;

      /* create local var 't' */
      var = slang_variable_scope_grow(scope);
      var->a_name = slang_atom_pool_atom(A->atoms, "ttt");
      var->type = fun->header.type;/*XXX copy*/

      /* declare t */
      {
         slang_operation *decl;

         decl = &fun->body->children[0];
         decl->type = SLANG_OPER_VARIABLE_DECL;
         decl->locals = _slang_variable_scope_new(scope);
         decl->a_id = var->a_name;
      }

      /* assign params to elements of t */
      for (i = 0; i < num_elements; i++) {
         slang_operation *assign = &fun->body->children[1 + i];

         assign->type = SLANG_OPER_ASSIGN;
         assign->locals = _slang_variable_scope_new(scope);
         assign->num_children = 2;
         assign->children = slang_operation_new(2);
         
         {
            slang_operation *lhs = &assign->children[0];

            lhs->type = SLANG_OPER_SUBSCRIPT;
            lhs->locals = _slang_variable_scope_new(scope);
            lhs->num_children = 2;
            lhs->children = slang_operation_new(2);
 
            lhs->children[0].type = SLANG_OPER_IDENTIFIER;
            lhs->children[0].a_id = var->a_name;
            lhs->children[0].locals = _slang_variable_scope_new(scope);

            lhs->children[1].type = SLANG_OPER_LITERAL_INT;
            lhs->children[1].literal[0] = (GLfloat) i;
         }

         {
            slang_operation *rhs = &assign->children[1];

            rhs->type = SLANG_OPER_IDENTIFIER;
            rhs->locals = _slang_variable_scope_new(scope);
            rhs->a_id = fun->parameters->variables[i]->a_name;
         }         
      }

      /* return t; */
      {
         slang_operation *ret = &fun->body->children[num_elements + 1];

         ret->type = SLANG_OPER_RETURN;
         ret->locals = _slang_variable_scope_new(scope);
         ret->num_children = 1;
         ret->children = slang_operation_new(1);
         ret->children[0].type = SLANG_OPER_IDENTIFIER;
         ret->children[0].a_id = var->a_name;
         ret->children[0].locals = _slang_variable_scope_new(scope);
      }
   }

   /*
   slang_print_function(fun, 1);
   */

   return fun;
}


static GLboolean
_slang_is_vec_mat_type(const char *name)
{
   static const char *vecmat_types[] = {
      "float", "int", "bool",
      "vec2", "vec3", "vec4",
      "ivec2", "ivec3", "ivec4",
      "bvec2", "bvec3", "bvec4",
      "mat2", "mat3", "mat4",
      "mat2x3", "mat2x4", "mat3x2", "mat3x4", "mat4x2", "mat4x3",
      NULL
   };
   int i;
   for (i = 0; vecmat_types[i]; i++)
      if (_mesa_strcmp(name, vecmat_types[i]) == 0)
         return GL_TRUE;
   return GL_FALSE;
}


/**
 * Assemble a function call, given a particular function name.
 * \param name  the function's name (operators like '*' are possible).
 */
static slang_ir_node *
_slang_gen_function_call_name(slang_assemble_ctx *A, const char *name,
                              slang_operation *oper, slang_operation *dest)
{
   slang_operation *params = oper->children;
   const GLuint param_count = oper->num_children;
   slang_atom atom;
   slang_function *fun;
   slang_ir_node *n;

   atom = slang_atom_pool_atom(A->atoms, name);
   if (atom == SLANG_ATOM_NULL)
      return NULL;

   if (oper->array_constructor) {
      /* this needs special handling */
      fun = _slang_make_array_constructor(A, oper);
   }
   else {
      /* Try to find function by name and exact argument type matching */
      GLboolean error = GL_FALSE;
      fun = _slang_function_locate(A->space.funcs, atom, params, param_count,
                                   &A->space, A->atoms, A->log, &error);
      if (error) {
         slang_info_log_error(A->log,
                              "Function '%s' not found (check argument types)",
                              name);
         return NULL;
      }
   }

   if (!fun) {
      /* Next, try locating a constructor function for a user-defined type */
      fun = _slang_locate_struct_constructor(A, name);
   }

   /*
    * At this point, some heuristics are used to try to find a function
    * that matches the calling signature by means of casting or "unrolling"
    * of constructors.
    */

   if (!fun && _slang_is_vec_mat_type(name)) {
      /* Next, if this call looks like a vec() or mat() constructor call,
       * try "unwinding" the args to satisfy a constructor.
       */
      fun = _slang_find_function_by_max_argc(A->space.funcs, name);
      if (fun) {
         if (!_slang_adapt_call(oper, fun, &A->space, A->atoms, A->log)) {
            slang_info_log_error(A->log,
                                 "Function '%s' not found (check argument types)",
                                 name);
            return NULL;
         }
      }
   }

   if (!fun && _slang_is_vec_mat_type(name)) {
      /* Next, try casting args to the types of the formal parameters */
      int numArgs = oper->num_children;
      fun = _slang_find_function_by_argc(A->space.funcs, name, numArgs);
      if (!fun || !_slang_cast_func_params(oper, fun, &A->space, A->atoms, A->log)) {
         slang_info_log_error(A->log,
                              "Function '%s' not found (check argument types)",
                              name);
         return NULL;
      }
      assert(fun);
   }

   if (!fun) {
      slang_info_log_error(A->log,
                           "Function '%s' not found (check argument types)",
                           name);
      return NULL;
   }
   if (!fun->body) {
      slang_info_log_error(A->log,
                           "Function '%s' prototyped but not defined.  "
                           "Separate compilation units not supported.",
                           name);
      return NULL;
   }

   /* type checking to be sure function's return type matches 'dest' type */
   if (dest) {
      slang_typeinfo t0;

      slang_typeinfo_construct(&t0);
      typeof_operation(A, dest, &t0);

      if (!slang_type_specifier_equal(&t0.spec, &fun->header.type.specifier)) {
         slang_info_log_error(A->log,
                              "Incompatible type returned by call to '%s'",
                              name);
         return NULL;
      }
   }

   n = _slang_gen_function_call(A, fun, oper, dest);

   if (n && !n->Store && !dest
       && fun->header.type.specifier.type != SLANG_SPEC_VOID) {
      /* setup n->Store for the result of the function call */
      GLint size = _slang_sizeof_type_specifier(&fun->header.type.specifier);
      n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, size);
      /*printf("Alloc storage for function result, size %d \n", size);*/
   }

   if (oper->array_constructor) {
      /* free the temporary array constructor function now */
      slang_function_destruct(fun);
   }

   return n;
}


static slang_ir_node *
_slang_gen_method_call(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_atom *a_length = slang_atom_pool_atom(A->atoms, "length");
   slang_ir_node *n;
   slang_variable *var;

   /* NOTE: In GLSL 1.20, there's only one kind of method
    * call: array.length().  Anything else is an error.
    */
   if (oper->a_id != a_length) {
      slang_info_log_error(A->log,
                           "Undefined method call '%s'", (char *) oper->a_id);
      return NULL;
   }

   /* length() takes no arguments */
   if (oper->num_children > 0) {
      slang_info_log_error(A->log, "Invalid arguments to length() method");
      return NULL;
   }

   /* lookup the object/variable */
   var = _slang_variable_locate(oper->locals, oper->a_obj, GL_TRUE);
   if (!var || var->type.specifier.type != SLANG_SPEC_ARRAY) {
      slang_info_log_error(A->log,
                           "Undefined object '%s'", (char *) oper->a_obj);
      return NULL;
   }

   /* Create a float/literal IR node encoding the array length */
   n = new_node0(IR_FLOAT);
   if (n) {
      n->Value[0] = (float) _slang_array_length(var);
      n->Store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, 1);
   }
   return n;
}


static GLboolean
_slang_is_constant_cond(const slang_operation *oper, GLboolean *value)
{
   if (oper->type == SLANG_OPER_LITERAL_FLOAT ||
       oper->type == SLANG_OPER_LITERAL_INT ||
       oper->type == SLANG_OPER_LITERAL_BOOL) {
      if (oper->literal[0])
         *value = GL_TRUE;
      else
         *value = GL_FALSE;
      return GL_TRUE;
   }
   else if (oper->type == SLANG_OPER_EXPRESSION &&
            oper->num_children == 1) {
      return _slang_is_constant_cond(&oper->children[0], value);
   }
   return GL_FALSE;
}


/**
 * Test if an operation is a scalar or boolean.
 */
static GLboolean
_slang_is_scalar_or_boolean(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_typeinfo type;
   GLint size;

   slang_typeinfo_construct(&type);
   typeof_operation(A, oper, &type);
   size = _slang_sizeof_type_specifier(&type.spec);
   slang_typeinfo_destruct(&type);
   return size == 1;
}


/**
 * Test if an operation is boolean.
 */
static GLboolean
_slang_is_boolean(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_typeinfo type;
   GLboolean isBool;

   slang_typeinfo_construct(&type);
   typeof_operation(A, oper, &type);
   isBool = (type.spec.type == SLANG_SPEC_BOOL);
   slang_typeinfo_destruct(&type);
   return isBool;
}


/**
 * Generate loop code using high-level IR_LOOP instruction
 */
static slang_ir_node *
_slang_gen_while(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * LOOP:
    *    BREAK if !expr (child[0])
    *    body code (child[1])
    */
   slang_ir_node *prevLoop, *loop, *breakIf, *body;
   GLboolean isConst, constTrue;

   /* type-check expression */
   if (!_slang_is_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log, "scalar/boolean expression expected for 'while'");
      return NULL;
   }

   /* Check if loop condition is a constant */
   isConst = _slang_is_constant_cond(&oper->children[0], &constTrue);

   if (isConst && !constTrue) {
      /* loop is never executed! */
      return new_node0(IR_NOP);
   }

   loop = new_loop(NULL);

   /* save old, push new loop */
   prevLoop = A->CurLoop;
   A->CurLoop = loop;

   if (isConst && constTrue) {
      /* while(nonzero constant), no conditional break */
      breakIf = NULL;
   }
   else {
      slang_ir_node *cond
         = new_cond(new_not(_slang_gen_operation(A, &oper->children[0])));
      breakIf = new_break_if_true(A->CurLoop, cond);
   }
   body = _slang_gen_operation(A, &oper->children[1]);
   loop->Children[0] = new_seq(breakIf, body);

   /* Do infinite loop detection */
   /* loop->List is head of linked list of break/continue nodes */
   if (!loop->List && isConst && constTrue) {
      /* infinite loop detected */
      A->CurLoop = prevLoop; /* clean-up */
      slang_info_log_error(A->log, "Infinite loop detected!");
      return NULL;
   }

   /* pop loop, restore prev */
   A->CurLoop = prevLoop;

   return loop;
}


/**
 * Generate IR tree for a do-while loop using high-level LOOP, IF instructions.
 */
static slang_ir_node *
_slang_gen_do(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * LOOP:
    *    body code (child[0])
    *    tail code:
    *       BREAK if !expr (child[1])
    */
   slang_ir_node *prevLoop, *loop;
   GLboolean isConst, constTrue;

   /* type-check expression */
   if (!_slang_is_boolean(A, &oper->children[1])) {
      slang_info_log_error(A->log, "scalar/boolean expression expected for 'do/while'");
      return NULL;
   }

   loop = new_loop(NULL);

   /* save old, push new loop */
   prevLoop = A->CurLoop;
   A->CurLoop = loop;

   /* loop body: */
   loop->Children[0] = _slang_gen_operation(A, &oper->children[0]);

   /* Check if loop condition is a constant */
   isConst = _slang_is_constant_cond(&oper->children[1], &constTrue);
   if (isConst && constTrue) {
      /* do { } while(1)   ==> no conditional break */
      loop->Children[1] = NULL; /* no tail code */
   }
   else {
      slang_ir_node *cond
         = new_cond(new_not(_slang_gen_operation(A, &oper->children[1])));
      loop->Children[1] = new_break_if_true(A->CurLoop, cond);
   }

   /* XXX we should do infinite loop detection, as above */

   /* pop loop, restore prev */
   A->CurLoop = prevLoop;

   return loop;
}


/**
 * Recursively count the number of operations rooted at 'oper'.
 * This gives some kind of indication of the size/complexity of an operation.
 */
static GLuint
sizeof_operation(const slang_operation *oper)
{
   if (oper) {
      GLuint count = 1; /* me */
      GLuint i;
      for (i = 0; i < oper->num_children; i++) {
         count += sizeof_operation(&oper->children[i]);
      }
      return count;
   }
   else {
      return 0;
   }
}


/**
 * Determine if a for-loop can be unrolled.
 * At this time, only a rather narrow class of for loops can be unrolled.
 * See code for details.
 * When a loop can't be unrolled because it's too large we'll emit a
 * message to the log.
 */
static GLboolean
_slang_can_unroll_for_loop(slang_assemble_ctx * A, const slang_operation *oper)
{
   GLuint bodySize;
   GLint start, end;
   const char *varName;
   slang_atom varId;

   assert(oper->type == SLANG_OPER_FOR);
   assert(oper->num_children == 4);

   /* children[0] must be either "int i=constant" or "i=constant" */
   if (oper->children[0].type == SLANG_OPER_BLOCK_NO_NEW_SCOPE) {
      slang_variable *var;

      if (oper->children[0].children[0].type != SLANG_OPER_VARIABLE_DECL)
         return GL_FALSE;

      varId = oper->children[0].children[0].a_id;

      var = _slang_variable_locate(oper->children[0].children[0].locals,
                                   varId, GL_TRUE);
      if (!var)
         return GL_FALSE;
      if (!var->initializer)
         return GL_FALSE;
      if (var->initializer->type != SLANG_OPER_LITERAL_INT)
         return GL_FALSE;
      start = (GLint) var->initializer->literal[0];
   }
   else if (oper->children[0].type == SLANG_OPER_EXPRESSION) {
      if (oper->children[0].children[0].type != SLANG_OPER_ASSIGN)
         return GL_FALSE;
      if (oper->children[0].children[0].children[0].type != SLANG_OPER_IDENTIFIER)
         return GL_FALSE;
      if (oper->children[0].children[0].children[1].type != SLANG_OPER_LITERAL_INT)
         return GL_FALSE;

      varId = oper->children[0].children[0].children[0].a_id;

      start = (GLint) oper->children[0].children[0].children[1].literal[0];
   }
   else {
      return GL_FALSE;
   }

   /* children[1] must be "i<constant" */
   if (oper->children[1].type != SLANG_OPER_EXPRESSION)
      return GL_FALSE;
   if (oper->children[1].children[0].type != SLANG_OPER_LESS)
      return GL_FALSE;
   if (oper->children[1].children[0].children[0].type != SLANG_OPER_IDENTIFIER)
      return GL_FALSE;
   if (oper->children[1].children[0].children[1].type != SLANG_OPER_LITERAL_INT)
      return GL_FALSE;

   end = (GLint) oper->children[1].children[0].children[1].literal[0];

   /* children[2] must be "i++" or "++i" */
   if (oper->children[2].type != SLANG_OPER_POSTINCREMENT &&
       oper->children[2].type != SLANG_OPER_PREINCREMENT)
      return GL_FALSE;
   if (oper->children[2].children[0].type != SLANG_OPER_IDENTIFIER)
      return GL_FALSE;

   /* make sure the same variable name is used in all places */
   if ((oper->children[1].children[0].children[0].a_id != varId) ||
       (oper->children[2].children[0].a_id != varId))
      return GL_FALSE;

   varName = (const char *) varId;

   /* children[3], the loop body, can't be too large */
   bodySize = sizeof_operation(&oper->children[3]);
   if (bodySize > MAX_FOR_LOOP_UNROLL_BODY_SIZE) {
      slang_info_log_print(A->log,
                           "Note: 'for (%s ... )' body is too large/complex"
                           " to unroll",
                           varName);
      return GL_FALSE;
   }

   if (start >= end)
      return GL_FALSE; /* degenerate case */

   if (end - start > MAX_FOR_LOOP_UNROLL_ITERATIONS) {
      slang_info_log_print(A->log,
                           "Note: 'for (%s=%d; %s<%d; ++%s)' is too"
                           " many iterations to unroll",
                           varName, start, varName, end, varName);
      return GL_FALSE;
   }

   if ((end - start) * bodySize > MAX_FOR_LOOP_UNROLL_COMPLEXITY) {
      slang_info_log_print(A->log,
                           "Note: 'for (%s=%d; %s<%d; ++%s)' will generate"
                           " too much code to unroll",
                           varName, start, varName, end, varName);
      return GL_FALSE;
   }

   return GL_TRUE; /* we can unroll the loop */
}


/**
 * Unroll a for-loop.
 * First we determine the number of iterations to unroll.
 * Then for each iteration:
 *   make a copy of the loop body
 *   replace instances of the loop variable with the current iteration value
 *   generate IR code for the body
 * \return pointer to generated IR code or NULL if error, out of memory, etc.
 */
static slang_ir_node *
_slang_unroll_for_loop(slang_assemble_ctx * A, const slang_operation *oper)
{
   GLint start, end, iter;
   slang_ir_node *n, *root = NULL;
   slang_atom varId;

   if (oper->children[0].type == SLANG_OPER_BLOCK_NO_NEW_SCOPE) {
      /* for (int i=0; ... */
      slang_variable *var;

      varId = oper->children[0].children[0].a_id;
      var = _slang_variable_locate(oper->children[0].children[0].locals,
                                   varId, GL_TRUE);
      start = (GLint) var->initializer->literal[0];
   }
   else {
      /* for (i=0; ... */
      varId = oper->children[0].children[0].children[0].a_id;
      start = (GLint) oper->children[0].children[0].children[1].literal[0];
   }

   end = (GLint) oper->children[1].children[0].children[1].literal[0];

   for (iter = start; iter < end; iter++) {
      slang_operation *body;

      /* make a copy of the loop body */
      body = slang_operation_new(1);
      if (!body)
         return NULL;

      if (!slang_operation_copy(body, &oper->children[3]))
         return NULL;

      /* in body, replace instances of 'varId' with literal 'iter' */
      {
         slang_variable *oldVar;
         slang_operation *newOper;

         oldVar = _slang_variable_locate(oper->locals, varId, GL_TRUE);
         if (!oldVar) {
            /* undeclared loop variable */
            slang_operation_delete(body);
            return NULL;
         }

         newOper = slang_operation_new(1);
         newOper->type = SLANG_OPER_LITERAL_INT;
         newOper->literal_size = 1;
         newOper->literal[0] = iter;

         /* replace instances of the loop variable with newOper */
         slang_substitute(A, body, 1, &oldVar, &newOper, GL_FALSE);
      }

      /* do IR codegen for body */
      n = _slang_gen_operation(A, body);
      root = new_seq(root, n);

      slang_operation_delete(body);
   }

   return root;
}


/**
 * Generate IR for a for-loop.  Unrolling will be done when possible.
 */
static slang_ir_node *
_slang_gen_for(slang_assemble_ctx * A, const slang_operation *oper)
{
   GLboolean unroll = _slang_can_unroll_for_loop(A, oper);

   if (unroll) {
      slang_ir_node *code = _slang_unroll_for_loop(A, oper);
      if (code)
         return code;
   }

   /* conventional for-loop code generation */
   {
      /*
       * init code (child[0])
       * LOOP:
       *    BREAK if !expr (child[1])
       *    body code (child[3])
       *    tail code:
       *       incr code (child[2])   // XXX continue here
       */
      slang_ir_node *prevLoop, *loop, *cond, *breakIf, *body, *init, *incr;
      init = _slang_gen_operation(A, &oper->children[0]);
      loop = new_loop(NULL);

      /* save old, push new loop */
      prevLoop = A->CurLoop;
      A->CurLoop = loop;

      cond = new_cond(new_not(_slang_gen_operation(A, &oper->children[1])));
      breakIf = new_break_if_true(A->CurLoop, cond);
      body = _slang_gen_operation(A, &oper->children[3]);
      incr = _slang_gen_operation(A, &oper->children[2]);

      loop->Children[0] = new_seq(breakIf, body);
      loop->Children[1] = incr;  /* tail code */

      /* pop loop, restore prev */
      A->CurLoop = prevLoop;

      return new_seq(init, loop);
   }
}


static slang_ir_node *
_slang_gen_continue(slang_assemble_ctx * A, const slang_operation *oper)
{
   slang_ir_node *n, *loopNode;
   assert(oper->type == SLANG_OPER_CONTINUE);
   loopNode = A->CurLoop;
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   n = new_node0(IR_CONT);
   if (n) {
      n->Parent = loopNode;
      /* insert this node at head of linked list */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


/**
 * Determine if the given operation is of a specific type.
 */
static GLboolean
is_operation_type(const slang_operation *oper, slang_operation_type type)
{
   if (oper->type == type)
      return GL_TRUE;
   else if ((oper->type == SLANG_OPER_BLOCK_NEW_SCOPE ||
             oper->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE) &&
            oper->num_children == 1)
      return is_operation_type(&oper->children[0], type);
   else
      return GL_FALSE;
}


/**
 * Generate IR tree for an if/then/else conditional using high-level
 * IR_IF instruction.
 */
static slang_ir_node *
_slang_gen_if(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * eval expr (child[0])
    * IF expr THEN
    *    if-body code
    * ELSE
    *    else-body code
    * ENDIF
    */
   const GLboolean haveElseClause = !_slang_is_noop(&oper->children[2]);
   slang_ir_node *ifNode, *cond, *ifBody, *elseBody;
   GLboolean isConst, constTrue;

   /* type-check expression */
   if (!_slang_is_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log, "boolean expression expected for 'if'");
      return NULL;
   }

   if (!_slang_is_scalar_or_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log, "scalar/boolean expression expected for 'if'");
      return NULL;
   }

   isConst = _slang_is_constant_cond(&oper->children[0], &constTrue);
   if (isConst) {
      if (constTrue) {
         /* if (true) ... */
         return _slang_gen_operation(A, &oper->children[1]);
      }
      else {
         /* if (false) ... */
         return _slang_gen_operation(A, &oper->children[2]);
      }
   }

   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = new_cond(cond);

   if (is_operation_type(&oper->children[1], SLANG_OPER_BREAK)
       && !haveElseClause) {
      /* Special case: generate a conditional break */
      ifBody = new_break_if_true(A->CurLoop, cond);
      return ifBody;
   }
   else if (is_operation_type(&oper->children[1], SLANG_OPER_CONTINUE)
            && !haveElseClause) {
      /* Special case: generate a conditional break */
      ifBody = new_cont_if_true(A->CurLoop, cond);
      return ifBody;
   }
   else {
      /* general case */
      ifBody = _slang_gen_operation(A, &oper->children[1]);
      if (haveElseClause)
         elseBody = _slang_gen_operation(A, &oper->children[2]);
      else
         elseBody = NULL;
      ifNode = new_if(cond, ifBody, elseBody);
      return ifNode;
   }
}



static slang_ir_node *
_slang_gen_not(slang_assemble_ctx * A, const slang_operation *oper)
{
   slang_ir_node *n;

   assert(oper->type == SLANG_OPER_NOT);

   /* type-check expression */
   if (!_slang_is_scalar_or_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log,
                           "scalar/boolean expression expected for '!'");
      return NULL;
   }

   n = _slang_gen_operation(A, &oper->children[0]);
   if (n)
      return new_not(n);
   else
      return NULL;
}


static slang_ir_node *
_slang_gen_xor(slang_assemble_ctx * A, const slang_operation *oper)
{
   slang_ir_node *n1, *n2;

   assert(oper->type == SLANG_OPER_LOGICALXOR);

   if (!_slang_is_scalar_or_boolean(A, &oper->children[0]) ||
       !_slang_is_scalar_or_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log,
                           "scalar/boolean expressions expected for '^^'");
      return NULL;
   }

   n1 = _slang_gen_operation(A, &oper->children[0]);
   if (!n1)
      return NULL;
   n2 = _slang_gen_operation(A, &oper->children[1]);
   if (!n2)
      return NULL;
   return new_node2(IR_NOTEQUAL, n1, n2);
}


/**
 * Generate IR node for storage of a temporary of given size.
 */
static slang_ir_node *
_slang_gen_temporary(GLint size)
{
   slang_ir_storage *store;
   slang_ir_node *n = NULL;

   store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -2, size);
   if (store) {
      n = new_node0(IR_VAR_DECL);
      if (n) {
         n->Store = store;
      }
      else {
         _slang_free(store);
      }
   }
   return n;
}


/**
 * Generate program constants for an array.
 * Ex: const vec2[3] v = vec2[3](vec2(1,1), vec2(2,2), vec2(3,3));
 * This will allocate and initialize three vector constants, storing
 * the array in constant memory, not temporaries like a non-const array.
 * This can also be used for uniform array initializers.
 * \return GL_TRUE for success, GL_FALSE if failure (semantic error, etc).
 */
static GLboolean
make_constant_array(slang_assemble_ctx *A,
                    slang_variable *var,
                    slang_operation *initializer)
{
   struct gl_program *prog = A->program;
   const GLenum datatype = _slang_gltype_from_specifier(&var->type.specifier);
   const char *varName = (char *) var->a_name;
   const GLuint numElements = initializer->num_children;
   GLint size;
   GLuint i, j;
   GLfloat *values;

   if (!var->store) {
      var->store = _slang_new_ir_storage(PROGRAM_UNDEFINED, -6, -6);
   }
   size = var->store->Size;

   assert(var->type.qualifier == SLANG_QUAL_CONST ||
          var->type.qualifier == SLANG_QUAL_UNIFORM);
   assert(initializer->type == SLANG_OPER_CALL);
   assert(initializer->array_constructor);

   values = (GLfloat *) _mesa_malloc(numElements * 4 * sizeof(GLfloat));

   /* convert constructor params into ordinary floats */
   for (i = 0; i < numElements; i++) {
      const slang_operation *op = &initializer->children[i];
      if (op->type != SLANG_OPER_LITERAL_FLOAT) {
         /* unsupported type for this optimization */
         free(values);
         return GL_FALSE;
      }
      for (j = 0; j < op->literal_size; j++) {
         values[i * 4 + j] = op->literal[j];
      }
      for ( ; j < 4; j++) {
         values[i * 4 + j] = 0.0f;
      }
   }

   /* slightly different paths for constants vs. uniforms */
   if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      var->store->File = PROGRAM_UNIFORM;
      var->store->Index = _mesa_add_uniform(prog->Parameters, varName,
                                            size, datatype, values);
   }
   else {
      var->store->File = PROGRAM_CONSTANT;
      var->store->Index = _mesa_add_named_constant(prog->Parameters, varName,
                                                   values, size);
   }
   assert(var->store->Size == size);

   _mesa_free(values);

   return GL_TRUE;
}



/**
 * Generate IR node for allocating/declaring a variable (either a local or
 * a global).
 * Generally, this involves allocating an slang_ir_storage instance for the
 * variable, choosing a register file (temporary, constant, etc).
 * For ordinary variables we do not yet allocate storage though.  We do that
 * when we find the first actual use of the variable to avoid allocating temp
 * regs that will never get used.
 * At this time, uniforms are always allocated space in this function.
 *
 * \param initializer  Optional initializer expression for the variable.
 */
static slang_ir_node *
_slang_gen_var_decl(slang_assemble_ctx *A, slang_variable *var,
                    slang_operation *initializer)
{
   const char *varName = (const char *) var->a_name;
   const GLenum datatype = _slang_gltype_from_specifier(&var->type.specifier);
   slang_ir_node *varDecl, *n;
   slang_ir_storage *store;
   GLint arrayLen, size, totalSize;  /* if array then totalSize > size */
   enum register_file file;

   /*assert(!var->declared);*/
   var->declared = GL_TRUE;

   /* determine GPU register file for simple cases */
   if (is_sampler_type(&var->type)) {
      file = PROGRAM_SAMPLER;
   }
   else if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      file = PROGRAM_UNIFORM;
   }
   else {
      file = PROGRAM_TEMPORARY;
   }

   totalSize = size = _slang_sizeof_type_specifier(&var->type.specifier);
   if (size <= 0) {
      slang_info_log_error(A->log, "invalid declaration for '%s'", varName);
      return NULL;
   }

   arrayLen = _slang_array_length(var);
   totalSize = _slang_array_size(size, arrayLen);

   /* Allocate IR node for the declaration */
   varDecl = new_node0(IR_VAR_DECL);
   if (!varDecl)
      return NULL;

   _slang_attach_storage(varDecl, var); /* undefined storage at first */
   assert(var->store);
   assert(varDecl->Store == var->store);
   assert(varDecl->Store);
   assert(varDecl->Store->Index < 0);
   store = var->store;

   assert(store == varDecl->Store);


   /* Fill in storage fields which we now know.  store->Index/Swizzle may be
    * set for some cases below.  Otherwise, store->Index/Swizzle will be set
    * during code emit.
    */
   store->File = file;
   store->Size = totalSize;

   /* if there's an initializer, generate IR for the expression */
   if (initializer) {
      slang_ir_node *varRef, *init;

      if (var->type.qualifier == SLANG_QUAL_CONST) {
         /* if the variable is const, the initializer must be a const
          * expression as well.
          */
#if 0
         if (!_slang_is_constant_expr(initializer)) {
            slang_info_log_error(A->log,
                                 "initializer for %s not constant", varName);
            return NULL;
         }
#endif
      }

      /* IR for the variable we're initializing */
      varRef = new_var(A, var);
      if (!varRef) {
         slang_info_log_error(A->log, "out of memory");
         return NULL;
      }

      /* constant-folding, etc here */
      _slang_simplify(initializer, &A->space, A->atoms); 

      /* look for simple constant-valued variables and uniforms */
      if (var->type.qualifier == SLANG_QUAL_CONST ||
          var->type.qualifier == SLANG_QUAL_UNIFORM) {

         if (initializer->type == SLANG_OPER_CALL &&
             initializer->array_constructor) {
            /* array initializer */
            if (make_constant_array(A, var, initializer))
               return varRef;
         }
         else if (initializer->type == SLANG_OPER_LITERAL_FLOAT ||
                  initializer->type == SLANG_OPER_LITERAL_INT) {
            /* simple float/vector initializer */
            if (store->File == PROGRAM_UNIFORM) {
               store->Index = _mesa_add_uniform(A->program->Parameters,
                                                varName,
                                                totalSize, datatype,
                                                initializer->literal);
               store->Swizzle = _slang_var_swizzle(size, 0);
               return varRef;
            }
#if 0
            else {
               store->File = PROGRAM_CONSTANT;
               store->Index = _mesa_add_named_constant(A->program->Parameters,
                                                       varName,
                                                       initializer->literal,
                                                       totalSize);
               store->Swizzle = _slang_var_swizzle(size, 0);
               return varRef;
            }
#endif
         }
      }

      /* IR for initializer */
      init = _slang_gen_operation(A, initializer);
      if (!init)
         return NULL;

      /* XXX remove this when type checking is added above */
      if (init->Store && init->Store->Size != totalSize) {
         slang_info_log_error(A->log, "invalid assignment (wrong types)");
         return NULL;
      }

      /* assign RHS to LHS */
      n = new_node2(IR_COPY, varRef, init);
      n = new_seq(varDecl, n);
   }
   else {
      /* no initializer */
      n = varDecl;
   }

   if (store->File == PROGRAM_UNIFORM && store->Index < 0) {
      /* always need to allocate storage for uniforms at this point */
      store->Index = _mesa_add_uniform(A->program->Parameters, varName,
                                       totalSize, datatype, NULL);
      store->Swizzle = _slang_var_swizzle(size, 0);
   }

#if 0
   printf("%s var %p %s  store=%p index=%d size=%d\n",
          __FUNCTION__, (void *) var, (char *) varName,
          (void *) store, store->Index, store->Size);
#endif

   return n;
}


/**
 * Generate code for a selection expression:   b ? x : y
 * XXX In some cases we could implement a selection expression
 * with an LRP instruction (use the boolean as the interpolant).
 * Otherwise, we use an IF/ELSE/ENDIF construct.
 */
static slang_ir_node *
_slang_gen_select(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_ir_node *cond, *ifNode, *trueExpr, *falseExpr, *trueNode, *falseNode;
   slang_ir_node *tmpDecl, *tmpVar, *tree;
   slang_typeinfo type0, type1, type2;
   int size, isBool, isEqual;

   assert(oper->type == SLANG_OPER_SELECT);
   assert(oper->num_children == 3);

   /* type of children[0] must be boolean */
   slang_typeinfo_construct(&type0);
   typeof_operation(A, &oper->children[0], &type0);
   isBool = (type0.spec.type == SLANG_SPEC_BOOL);
   slang_typeinfo_destruct(&type0);
   if (!isBool) {
      slang_info_log_error(A->log, "selector type is not boolean");
      return NULL;
   }

   slang_typeinfo_construct(&type1);
   slang_typeinfo_construct(&type2);
   typeof_operation(A, &oper->children[1], &type1);
   typeof_operation(A, &oper->children[2], &type2);
   isEqual = slang_type_specifier_equal(&type1.spec, &type2.spec);
   slang_typeinfo_destruct(&type1);
   slang_typeinfo_destruct(&type2);
   if (!isEqual) {
      slang_info_log_error(A->log, "incompatible types for ?: operator");
      return NULL;
   }

   /* size of x or y's type */
   size = _slang_sizeof_type_specifier(&type1.spec);
   assert(size > 0);

   /* temporary var */
   tmpDecl = _slang_gen_temporary(size);

   /* the condition (child 0) */
   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = new_cond(cond);

   /* if-true body (child 1) */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;
   trueExpr = _slang_gen_operation(A, &oper->children[1]);
   trueNode = new_node2(IR_COPY, tmpVar, trueExpr);

   /* if-false body (child 2) */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;
   falseExpr = _slang_gen_operation(A, &oper->children[2]);
   falseNode = new_node2(IR_COPY, tmpVar, falseExpr);

   ifNode = new_if(cond, trueNode, falseNode);

   /* tmp var value */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;

   tree = new_seq(ifNode, tmpVar);
   tree = new_seq(tmpDecl, tree);

   /*_slang_print_ir_tree(tree, 10);*/
   return tree;
}


/**
 * Generate code for &&.
 */
static slang_ir_node *
_slang_gen_logical_and(slang_assemble_ctx *A, slang_operation *oper)
{
   /* rewrite "a && b" as  "a ? b : false" */
   slang_operation *select;
   slang_ir_node *n;

   select = slang_operation_new(1);
   select->type = SLANG_OPER_SELECT;
   select->num_children = 3;
   select->children = slang_operation_new(3);

   slang_operation_copy(&select->children[0], &oper->children[0]);
   slang_operation_copy(&select->children[1], &oper->children[1]);
   select->children[2].type = SLANG_OPER_LITERAL_BOOL;
   ASSIGN_4V(select->children[2].literal, 0, 0, 0, 0); /* false */
   select->children[2].literal_size = 1;

   n = _slang_gen_select(A, select);
   return n;
}


/**
 * Generate code for ||.
 */
static slang_ir_node *
_slang_gen_logical_or(slang_assemble_ctx *A, slang_operation *oper)
{
   /* rewrite "a || b" as  "a ? true : b" */
   slang_operation *select;
   slang_ir_node *n;

   select = slang_operation_new(1);
   select->type = SLANG_OPER_SELECT;
   select->num_children = 3;
   select->children = slang_operation_new(3);

   slang_operation_copy(&select->children[0], &oper->children[0]);
   select->children[1].type = SLANG_OPER_LITERAL_BOOL;
   ASSIGN_4V(select->children[1].literal, 1, 1, 1, 1); /* true */
   select->children[1].literal_size = 1;
   slang_operation_copy(&select->children[2], &oper->children[1]);

   n = _slang_gen_select(A, select);
   return n;
}


/**
 * Generate IR tree for a return statement.
 */
static slang_ir_node *
_slang_gen_return(slang_assemble_ctx * A, slang_operation *oper)
{
   const GLboolean haveReturnValue
      = (oper->num_children == 1 && oper->children[0].type != SLANG_OPER_VOID);

   /* error checking */
   assert(A->CurFunction);
   if (haveReturnValue &&
       A->CurFunction->header.type.specifier.type == SLANG_SPEC_VOID) {
      slang_info_log_error(A->log, "illegal return expression");
      return NULL;
   }
   else if (!haveReturnValue &&
            A->CurFunction->header.type.specifier.type != SLANG_SPEC_VOID) {
      slang_info_log_error(A->log, "return statement requires an expression");
      return NULL;
   }

   if (!haveReturnValue) {
      return new_return(A->curFuncEndLabel);
   }
   else {
      /*
       * Convert from:
       *   return expr;
       * To:
       *   __retVal = expr;
       *   return;  // goto __endOfFunction
       */
      slang_operation *assign;
      slang_atom a_retVal;
      slang_ir_node *n;

      a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);

#if 1 /* DEBUG */
      {
         slang_variable *v =
            _slang_variable_locate(oper->locals, a_retVal, GL_TRUE);
         if (!v) {
            /* trying to return a value in a void-valued function */
            return NULL;
         }
      }
#endif

      assign = slang_operation_new(1);
      assign->type = SLANG_OPER_ASSIGN;
      assign->num_children = 2;
      assign->children = slang_operation_new(2);
      /* lhs (__retVal) */
      assign->children[0].type = SLANG_OPER_IDENTIFIER;
      assign->children[0].a_id = a_retVal;
      assign->children[0].locals->outer_scope = assign->locals;
      /* rhs (expr) */
      /* XXX we might be able to avoid this copy someday */
      slang_operation_copy(&assign->children[1], &oper->children[0]);

      /* assemble the new code */
      n = new_seq(_slang_gen_operation(A, assign),
                  new_return(A->curFuncEndLabel));

      slang_operation_delete(assign);
      return n;
   }
}


#if 0
/**
 * Determine if the given operation/expression is const-valued.
 */
static GLboolean
_slang_is_constant_expr(const slang_operation *oper)
{
   slang_variable *var;
   GLuint i;

   switch (oper->type) {
   case SLANG_OPER_IDENTIFIER:
      var = _slang_variable_locate(oper->locals, oper->a_id, GL_TRUE);
      if (var && var->type.qualifier == SLANG_QUAL_CONST)
         return GL_TRUE;
      return GL_FALSE;
   default:
      for (i = 0; i < oper->num_children; i++) {
         if (!_slang_is_constant_expr(&oper->children[i]))
            return GL_FALSE;
      }
      return GL_TRUE;
   }
}
#endif


/**
 * Check if an assignment of type t1 to t0 is legal.
 * XXX more cases needed.
 */
static GLboolean
_slang_assignment_compatible(slang_assemble_ctx *A,
                             slang_operation *op0,
                             slang_operation *op1)
{
   slang_typeinfo t0, t1;
   GLuint sz0, sz1;

   if (op0->type == SLANG_OPER_POSTINCREMENT ||
       op0->type == SLANG_OPER_POSTDECREMENT) {
      return GL_FALSE;
   }

   slang_typeinfo_construct(&t0);
   typeof_operation(A, op0, &t0);

   slang_typeinfo_construct(&t1);
   typeof_operation(A, op1, &t1);

   sz0 = _slang_sizeof_type_specifier(&t0.spec);
   sz1 = _slang_sizeof_type_specifier(&t1.spec);

#if 1
   if (sz0 != sz1) {
      /*printf("assignment size mismatch %u vs %u\n", sz0, sz1);*/
      return GL_FALSE;
   }
#endif

   if (t0.spec.type == SLANG_SPEC_STRUCT &&
       t1.spec.type == SLANG_SPEC_STRUCT &&
       t0.spec._struct->a_name != t1.spec._struct->a_name)
      return GL_FALSE;

   if (t0.spec.type == SLANG_SPEC_FLOAT &&
       t1.spec.type == SLANG_SPEC_BOOL)
      return GL_FALSE;

#if 0 /* not used just yet - causes problems elsewhere */
   if (t0.spec.type == SLANG_SPEC_INT &&
       t1.spec.type == SLANG_SPEC_FLOAT)
      return GL_FALSE;
#endif

   if (t0.spec.type == SLANG_SPEC_BOOL &&
       t1.spec.type == SLANG_SPEC_FLOAT)
      return GL_FALSE;

   if (t0.spec.type == SLANG_SPEC_BOOL &&
       t1.spec.type == SLANG_SPEC_INT)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Generate IR tree for a local variable declaration.
 * Basically do some error checking and call _slang_gen_var_decl().
 */
static slang_ir_node *
_slang_gen_declaration(slang_assemble_ctx *A, slang_operation *oper)
{
   const char *varName = (char *) oper->a_id;
   slang_variable *var;
   slang_ir_node *varDecl;
   slang_operation *initializer;

   assert(oper->type == SLANG_OPER_VARIABLE_DECL);
   assert(oper->num_children <= 1);

   /* lookup the variable by name */
   var = _slang_variable_locate(oper->locals, oper->a_id, GL_TRUE);
   if (!var)
      return NULL;  /* "shouldn't happen" */

   if (var->type.qualifier == SLANG_QUAL_ATTRIBUTE ||
       var->type.qualifier == SLANG_QUAL_VARYING ||
       var->type.qualifier == SLANG_QUAL_UNIFORM) {
      /* can't declare attribute/uniform vars inside functions */
      slang_info_log_error(A->log,
                "local variable '%s' cannot be an attribute/uniform/varying",
                varName);
      return NULL;
   }

#if 0
   if (v->declared) {
      slang_info_log_error(A->log, "variable '%s' redeclared", varName);
      return NULL;
   }
#endif

   /* check if the var has an initializer */
   if (oper->num_children > 0) {
      assert(oper->num_children == 1);
      initializer = &oper->children[0];
   }
   else if (var->initializer) {
      initializer = var->initializer;
   }
   else {
      initializer = NULL;
   }

   if (initializer) {
      /* check/compare var type and initializer type */
      if (!_slang_assignment_compatible(A, oper, initializer)) {
         slang_info_log_error(A->log, "incompatible types in assignment");
         return NULL;
      }         
   }
   else {
      if (var->type.qualifier == SLANG_QUAL_CONST) {
         slang_info_log_error(A->log,
                       "const-qualified variable '%s' requires initializer",
                       varName);
         return NULL;
      }
   }

   /* Generate IR node */
   varDecl = _slang_gen_var_decl(A, var, initializer);
   if (!varDecl)
      return NULL;

   return varDecl;
}


/**
 * Generate IR tree for a reference to a variable (such as in an expression).
 * This is different from a variable declaration.
 */
static slang_ir_node *
_slang_gen_variable(slang_assemble_ctx * A, slang_operation *oper)
{
   /* If there's a variable associated with this oper (from inlining)
    * use it.  Otherwise, use the oper's var id.
    */
   slang_atom name = oper->var ? oper->var->a_name : oper->a_id;
   slang_variable *var = _slang_variable_locate(oper->locals, name, GL_TRUE);
   slang_ir_node *n;
   if (!var) {
      slang_info_log_error(A->log, "undefined variable '%s'", (char *) name);
      return NULL;
   }
   assert(var->declared);
   n = new_var(A, var);
   return n;
}



/**
 * Return the number of components actually named by the swizzle.
 * Recall that swizzles may have undefined/don't-care values.
 */
static GLuint
swizzle_size(GLuint swizzle)
{
   GLuint size = 0, i;
   for (i = 0; i < 4; i++) {
      GLuint swz = GET_SWZ(swizzle, i);
      size += (swz >= 0 && swz <= 3);
   }
   return size;
}


static slang_ir_node *
_slang_gen_swizzle(slang_ir_node *child, GLuint swizzle)
{
   slang_ir_node *n = new_node1(IR_SWIZZLE, child);
   assert(child);
   if (n) {
      assert(!n->Store);
      n->Store = _slang_new_ir_storage_relative(0,
                                                swizzle_size(swizzle),
                                                child->Store);
      n->Store->Swizzle = swizzle;
   }
   return n;
}


static GLboolean
is_store_writable(const slang_assemble_ctx *A, const slang_ir_storage *store)
{
   while (store->Parent)
      store = store->Parent;

   if (!(store->File == PROGRAM_OUTPUT ||
         store->File == PROGRAM_TEMPORARY ||
         (store->File == PROGRAM_VARYING &&
          A->program->Target == GL_VERTEX_PROGRAM_ARB))) {
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}


/**
 * Walk up an IR storage path to compute the final swizzle.
 * This is used when we find an expression such as "foo.xz.yx".
 */
static GLuint
root_swizzle(const slang_ir_storage *st)
{
   GLuint swizzle = st->Swizzle;
   while (st->Parent) {
      st = st->Parent;
      swizzle = _slang_swizzle_swizzle(st->Swizzle, swizzle);
   }
   return swizzle;
}


/**
 * Generate IR tree for an assignment (=).
 */
static slang_ir_node *
_slang_gen_assignment(slang_assemble_ctx * A, slang_operation *oper)
{
   if (oper->children[0].type == SLANG_OPER_IDENTIFIER) {
      /* Check that var is writeable */
      slang_variable *var
         = _slang_variable_locate(oper->children[0].locals,
                                  oper->children[0].a_id, GL_TRUE);
      if (!var) {
         slang_info_log_error(A->log, "undefined variable '%s'",
                              (char *) oper->children[0].a_id);
         return NULL;
      }
      if (var->type.qualifier == SLANG_QUAL_CONST ||
          var->type.qualifier == SLANG_QUAL_ATTRIBUTE ||
          var->type.qualifier == SLANG_QUAL_UNIFORM ||
          (var->type.qualifier == SLANG_QUAL_VARYING &&
           A->program->Target == GL_FRAGMENT_PROGRAM_ARB)) {
         slang_info_log_error(A->log,
                              "illegal assignment to read-only variable '%s'",
                              (char *) oper->children[0].a_id);
         return NULL;
      }
   }

   if (oper->children[0].type == SLANG_OPER_IDENTIFIER &&
       oper->children[1].type == SLANG_OPER_CALL) {
      /* Special case of:  x = f(a, b)
       * Replace with f(a, b, x)  (where x == hidden __retVal out param)
       *
       * XXX this could be even more effective if we could accomodate
       * cases such as "v.x = f();"  - would help with typical vertex
       * transformation.
       */
      slang_ir_node *n;
      n = _slang_gen_function_call_name(A,
                                      (const char *) oper->children[1].a_id,
                                      &oper->children[1], &oper->children[0]);
      return n;
   }
   else {
      slang_ir_node *n, *lhs, *rhs;

      /* lhs and rhs type checking */
      if (!_slang_assignment_compatible(A,
                                        &oper->children[0],
                                        &oper->children[1])) {
         slang_info_log_error(A->log, "incompatible types in assignment");
         return NULL;
      }

      lhs = _slang_gen_operation(A, &oper->children[0]);
      if (!lhs) {
         return NULL;
      }

      if (!lhs->Store) {
         slang_info_log_error(A->log,
                              "invalid left hand side for assignment");
         return NULL;
      }

      /* check that lhs is writable */
      if (!is_store_writable(A, lhs->Store)) {
         slang_info_log_error(A->log,
                              "illegal assignment to read-only l-value");
         return NULL;
      }

      rhs = _slang_gen_operation(A, &oper->children[1]);
      if (lhs && rhs) {
         /* convert lhs swizzle into writemask */
         const GLuint swizzle = root_swizzle(lhs->Store);
         GLuint writemask, newSwizzle = 0x0;
         if (!swizzle_to_writemask(A, swizzle, &writemask, &newSwizzle)) {
            /* Non-simple writemask, need to swizzle right hand side in
             * order to put components into the right place.
             */
            rhs = _slang_gen_swizzle(rhs, newSwizzle);
         }
         n = new_node2(IR_COPY, lhs, rhs);
         return n;
      }
      else {
         return NULL;
      }
   }
}


/**
 * Generate IR tree for referencing a field in a struct (or basic vector type)
 */
static slang_ir_node *
_slang_gen_struct_field(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_typeinfo ti;

   /* type of struct */
   slang_typeinfo_construct(&ti);
   typeof_operation(A, &oper->children[0], &ti);

   if (_slang_type_is_vector(ti.spec.type)) {
      /* the field should be a swizzle */
      const GLuint rows = _slang_type_dim(ti.spec.type);
      slang_swizzle swz;
      slang_ir_node *n;
      GLuint swizzle;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         slang_info_log_error(A->log, "Bad swizzle");
         return NULL;
      }
      swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                              swz.swizzle[1],
                              swz.swizzle[2],
                              swz.swizzle[3]);

      n = _slang_gen_operation(A, &oper->children[0]);
      /* create new parent node with swizzle */
      if (n)
         n = _slang_gen_swizzle(n, swizzle);
      return n;
   }
   else if (   ti.spec.type == SLANG_SPEC_FLOAT
            || ti.spec.type == SLANG_SPEC_INT
            || ti.spec.type == SLANG_SPEC_BOOL) {
      const GLuint rows = 1;
      slang_swizzle swz;
      slang_ir_node *n;
      GLuint swizzle;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         slang_info_log_error(A->log, "Bad swizzle");
      }
      swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                              swz.swizzle[1],
                              swz.swizzle[2],
                              swz.swizzle[3]);
      n = _slang_gen_operation(A, &oper->children[0]);
      /* create new parent node with swizzle */
      n = _slang_gen_swizzle(n, swizzle);
      return n;
   }
   else {
      /* the field is a structure member (base.field) */
      /* oper->children[0] is the base */
      /* oper->a_id is the field name */
      slang_ir_node *base, *n;
      slang_typeinfo field_ti;
      GLint fieldSize, fieldOffset = -1;

      /* type of field */
      slang_typeinfo_construct(&field_ti);
      typeof_operation(A, oper, &field_ti);

      fieldSize = _slang_sizeof_type_specifier(&field_ti.spec);
      if (fieldSize > 0)
         fieldOffset = _slang_field_offset(&ti.spec, oper->a_id);

      if (fieldSize == 0 || fieldOffset < 0) {
         const char *structName;
         if (ti.spec._struct)
            structName = (char *) ti.spec._struct->a_name;
         else
            structName = "unknown";
         slang_info_log_error(A->log,
                              "\"%s\" is not a member of struct \"%s\"",
                              (char *) oper->a_id, structName);
         return NULL;
      }
      assert(fieldSize >= 0);

      base = _slang_gen_operation(A, &oper->children[0]);
      if (!base) {
         /* error msg should have already been logged */
         return NULL;
      }

      n = new_node1(IR_FIELD, base);
      if (!n)
         return NULL;

      n->Field = (char *) oper->a_id;

      /* Store the field's offset in storage->Index */
      n->Store = _slang_new_ir_storage(base->Store->File,
                                       fieldOffset,
                                       fieldSize);

      return n;
   }
}


/**
 * Gen code for array indexing.
 */
static slang_ir_node *
_slang_gen_array_element(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_typeinfo array_ti;

   /* get array's type info */
   slang_typeinfo_construct(&array_ti);
   typeof_operation(A, &oper->children[0], &array_ti);

   if (_slang_type_is_vector(array_ti.spec.type)) {
      /* indexing a simple vector type: "vec4 v; v[0]=p;" */
      /* translate the index into a swizzle/writemask: "v.x=p" */
      const GLuint max = _slang_type_dim(array_ti.spec.type);
      GLint index;
      slang_ir_node *n;

      index = (GLint) oper->children[1].literal[0];
      if (oper->children[1].type != SLANG_OPER_LITERAL_INT ||
          index >= (GLint) max) {
#if 0
         slang_info_log_error(A->log, "Invalid array index for vector type");
         printf("type = %d\n", oper->children[1].type);
         printf("index = %d, max = %d\n", index, max);
         printf("array = %s\n", (char*)oper->children[0].a_id);
         printf("index = %s\n", (char*)oper->children[1].a_id);
         return NULL;
#else
         index = 0;
#endif
      }

      n = _slang_gen_operation(A, &oper->children[0]);
      if (n) {
         /* use swizzle to access the element */
         GLuint swizzle = MAKE_SWIZZLE4(SWIZZLE_X + index,
                                        SWIZZLE_NIL,
                                        SWIZZLE_NIL,
                                        SWIZZLE_NIL);
         n = _slang_gen_swizzle(n, swizzle);
      }
      assert(n->Store);
      return n;
   }
   else {
      /* conventional array */
      slang_typeinfo elem_ti;
      slang_ir_node *elem, *array, *index;
      GLint elemSize, arrayLen;

      /* size of array element */
      slang_typeinfo_construct(&elem_ti);
      typeof_operation(A, oper, &elem_ti);
      elemSize = _slang_sizeof_type_specifier(&elem_ti.spec);

      if (_slang_type_is_matrix(array_ti.spec.type))
         arrayLen = _slang_type_dim(array_ti.spec.type);
      else
         arrayLen = array_ti.array_len;

      slang_typeinfo_destruct(&array_ti);
      slang_typeinfo_destruct(&elem_ti);

      if (elemSize <= 0) {
         /* unknown var or type */
         slang_info_log_error(A->log, "Undefined variable or type");
         return NULL;
      }

      array = _slang_gen_operation(A, &oper->children[0]);
      index = _slang_gen_operation(A, &oper->children[1]);
      if (array && index) {
         /* bounds check */
         GLint constIndex = -1;
         if (index->Opcode == IR_FLOAT) {
            constIndex = (int) index->Value[0];
            if (constIndex < 0 || constIndex >= arrayLen) {
               slang_info_log_error(A->log,
                                "Array index out of bounds (index=%d size=%d)",
                                 constIndex, arrayLen);
               _slang_free_ir_tree(array);
               _slang_free_ir_tree(index);
               return NULL;
            }
         }

         if (!array->Store) {
            slang_info_log_error(A->log, "Invalid array");
            return NULL;
         }

         elem = new_node2(IR_ELEMENT, array, index);

         /* The storage info here will be updated during code emit */
         elem->Store = _slang_new_ir_storage(array->Store->File,
                                             array->Store->Index,
                                             elemSize);
         elem->Store->Swizzle = _slang_var_swizzle(elemSize, 0);
         return elem;
      }
      else {
         _slang_free_ir_tree(array);
         _slang_free_ir_tree(index);
         return NULL;
      }
   }
}


static slang_ir_node *
_slang_gen_compare(slang_assemble_ctx *A, slang_operation *oper,
                   slang_ir_opcode opcode)
{
   slang_typeinfo t0, t1;
   slang_ir_node *n;
   
   slang_typeinfo_construct(&t0);
   typeof_operation(A, &oper->children[0], &t0);

   slang_typeinfo_construct(&t1);
   typeof_operation(A, &oper->children[0], &t1);

   if (t0.spec.type == SLANG_SPEC_ARRAY ||
       t1.spec.type == SLANG_SPEC_ARRAY) {
      slang_info_log_error(A->log, "Illegal array comparison");
      return NULL;
   }

   if (oper->type != SLANG_OPER_EQUAL &&
       oper->type != SLANG_OPER_NOTEQUAL) {
      /* <, <=, >, >= can only be used with scalars */
      if ((t0.spec.type != SLANG_SPEC_INT &&
           t0.spec.type != SLANG_SPEC_FLOAT) ||
          (t1.spec.type != SLANG_SPEC_INT &&
           t1.spec.type != SLANG_SPEC_FLOAT)) {
         slang_info_log_error(A->log, "Incompatible type(s) for inequality operator");
         return NULL;
      }
   }

   n =  new_node2(opcode,
                  _slang_gen_operation(A, &oper->children[0]),
                  _slang_gen_operation(A, &oper->children[1]));

   /* result is a bool (size 1) */
   n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, 1);

   return n;
}


#if 0
static void
print_vars(slang_variable_scope *s)
{
   int i;
   printf("vars: ");
   for (i = 0; i < s->num_variables; i++) {
      printf("%s %d, \n",
             (char*) s->variables[i]->a_name,
             s->variables[i]->declared);
   }

   printf("\n");
}
#endif


#if 0
static void
_slang_undeclare_vars(slang_variable_scope *locals)
{
   if (locals->num_variables > 0) {
      int i;
      for (i = 0; i < locals->num_variables; i++) {
         slang_variable *v = locals->variables[i];
         printf("undeclare %s at %p\n", (char*) v->a_name, v);
         v->declared = GL_FALSE;
      }
   }
}
#endif


/**
 * Generate IR tree for a slang_operation (AST node)
 */
static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper)
{
   switch (oper->type) {
   case SLANG_OPER_BLOCK_NEW_SCOPE:
      {
         slang_ir_node *n;

         _slang_push_var_table(A->vartable);

         oper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE; /* temp change */
         n = _slang_gen_operation(A, oper);
         oper->type = SLANG_OPER_BLOCK_NEW_SCOPE; /* restore */

         _slang_pop_var_table(A->vartable);

         /*_slang_undeclare_vars(oper->locals);*/
         /*print_vars(oper->locals);*/

         if (n)
            n = new_node1(IR_SCOPE, n);
         return n;
      }
      break;

   case SLANG_OPER_BLOCK_NO_NEW_SCOPE:
      /* list of operations */
      if (oper->num_children > 0)
      {
         slang_ir_node *n, *tree = NULL;
         GLuint i;

         for (i = 0; i < oper->num_children; i++) {
            n = _slang_gen_operation(A, &oper->children[i]);
            if (!n) {
               _slang_free_ir_tree(tree);
               return NULL; /* error must have occured */
            }
            tree = new_seq(tree, n);
         }

         return tree;
      }
      else {
         return new_node0(IR_NOP);
      }

   case SLANG_OPER_EXPRESSION:
      return _slang_gen_operation(A, &oper->children[0]);

   case SLANG_OPER_FOR:
      return _slang_gen_for(A, oper);
   case SLANG_OPER_DO:
      return _slang_gen_do(A, oper);
   case SLANG_OPER_WHILE:
      return _slang_gen_while(A, oper);
   case SLANG_OPER_BREAK:
      if (!A->CurLoop) {
         slang_info_log_error(A->log, "'break' not in loop");
         return NULL;
      }
      return new_break(A->CurLoop);
   case SLANG_OPER_CONTINUE:
      if (!A->CurLoop) {
         slang_info_log_error(A->log, "'continue' not in loop");
         return NULL;
      }
      return _slang_gen_continue(A, oper);
   case SLANG_OPER_DISCARD:
      return new_node0(IR_KILL);

   case SLANG_OPER_EQUAL:
      return _slang_gen_compare(A, oper, IR_EQUAL);
   case SLANG_OPER_NOTEQUAL:
      return _slang_gen_compare(A, oper, IR_NOTEQUAL);
   case SLANG_OPER_GREATER:
      return _slang_gen_compare(A, oper, IR_SGT);
   case SLANG_OPER_LESS:
      return _slang_gen_compare(A, oper, IR_SLT);
   case SLANG_OPER_GREATEREQUAL:
      return _slang_gen_compare(A, oper, IR_SGE);
   case SLANG_OPER_LESSEQUAL:
      return _slang_gen_compare(A, oper, IR_SLE);
   case SLANG_OPER_ADD:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+", oper, NULL);
	 return n;
      }
   case SLANG_OPER_SUBTRACT:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case SLANG_OPER_MULTIPLY:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
         n = _slang_gen_function_call_name(A, "*", oper, NULL);
	 return n;
      }
   case SLANG_OPER_DIVIDE:
      {
         slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/", oper, NULL);
	 return n;
      }
   case SLANG_OPER_MINUS:
      {
         slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case SLANG_OPER_PLUS:
      /* +expr   --> do nothing */
      return _slang_gen_operation(A, &oper->children[0]);
   case SLANG_OPER_VARIABLE_DECL:
      return _slang_gen_declaration(A, oper);
   case SLANG_OPER_ASSIGN:
      return _slang_gen_assignment(A, oper);
   case SLANG_OPER_ADDASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+=", oper, NULL);
	 return n;
      }
   case SLANG_OPER_SUBASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-=", oper, NULL);
	 return n;
      }
      break;
   case SLANG_OPER_MULASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "*=", oper, NULL);
	 return n;
      }
   case SLANG_OPER_DIVASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/=", oper, NULL);
	 return n;
      }
   case SLANG_OPER_LOGICALAND:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_logical_and(A, oper);
	 return n;
      }
   case SLANG_OPER_LOGICALOR:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_logical_or(A, oper);
	 return n;
      }
   case SLANG_OPER_LOGICALXOR:
      return _slang_gen_xor(A, oper);
   case SLANG_OPER_NOT:
      return _slang_gen_not(A, oper);
   case SLANG_OPER_SELECT:  /* b ? x : y */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 3);
	 n = _slang_gen_select(A, oper);
	 return n;
      }

   case SLANG_OPER_ASM:
      return _slang_gen_asm(A, oper, NULL);
   case SLANG_OPER_CALL:
      return _slang_gen_function_call_name(A, (const char *) oper->a_id,
                                           oper, NULL);
   case SLANG_OPER_METHOD:
      return _slang_gen_method_call(A, oper);
   case SLANG_OPER_RETURN:
      return _slang_gen_return(A, oper);
   case SLANG_OPER_LABEL:
      return new_label(oper->label);
   case SLANG_OPER_IDENTIFIER:
      return _slang_gen_variable(A, oper);
   case SLANG_OPER_IF:
      return _slang_gen_if(A, oper);
   case SLANG_OPER_FIELD:
      return _slang_gen_struct_field(A, oper);
   case SLANG_OPER_SUBSCRIPT:
      return _slang_gen_array_element(A, oper);
   case SLANG_OPER_LITERAL_FLOAT:
      /* fall-through */
   case SLANG_OPER_LITERAL_INT:
      /* fall-through */
   case SLANG_OPER_LITERAL_BOOL:
      return new_float_literal(oper->literal, oper->literal_size);

   case SLANG_OPER_POSTINCREMENT:   /* var++ */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__postIncr", oper, NULL);
	 return n;
      }
   case SLANG_OPER_POSTDECREMENT:   /* var-- */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__postDecr", oper, NULL);
	 return n;
      }
   case SLANG_OPER_PREINCREMENT:   /* ++var */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "++", oper, NULL);
	 return n;
      }
   case SLANG_OPER_PREDECREMENT:   /* --var */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "--", oper, NULL);
	 return n;
      }

   case SLANG_OPER_NON_INLINED_CALL:
   case SLANG_OPER_SEQUENCE:
      {
         slang_ir_node *tree = NULL;
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            slang_ir_node *n = _slang_gen_operation(A, &oper->children[i]);
            tree = new_seq(tree, n);
            if (n)
               tree->Store = n->Store;
         }
         if (oper->type == SLANG_OPER_NON_INLINED_CALL) {
            tree = new_function_call(tree, oper->label);
         }
         return tree;
      }

   case SLANG_OPER_NONE:
   case SLANG_OPER_VOID:
      /* returning NULL here would generate an error */
      return new_node0(IR_NOP);

   default:
      _mesa_problem(NULL, "bad node type %d in _slang_gen_operation",
                    oper->type);
      return new_node0(IR_NOP);
   }

   return NULL;
}


/**
 * Check if the given type specifier is a rectangular texture sampler.
 */
static GLboolean
is_rect_sampler_spec(const slang_type_specifier *spec)
{
   while (spec->_array) {
      spec = spec->_array;
   }
   return spec->type == SLANG_SPEC_SAMPLER2DRECT ||
          spec->type == SLANG_SPEC_SAMPLER2DRECTSHADOW;
}



/**
 * Called by compiler when a global variable has been parsed/compiled.
 * Here we examine the variable's type to determine what kind of register
 * storage will be used.
 *
 * A uniform such as "gl_Position" will become the register specification
 * (PROGRAM_OUTPUT, VERT_RESULT_HPOS).  Or, uniform "gl_FogFragCoord"
 * will be (PROGRAM_INPUT, FRAG_ATTRIB_FOGC).
 *
 * Samplers are interesting.  For "uniform sampler2D tex;" we'll specify
 * (PROGRAM_SAMPLER, index) where index is resolved at link-time to an
 * actual texture unit (as specified by the user calling glUniform1i()).
 */
GLboolean
_slang_codegen_global_variable(slang_assemble_ctx *A, slang_variable *var,
                               slang_unit_type type)
{
   struct gl_program *prog = A->program;
   const char *varName = (char *) var->a_name;
   GLboolean success = GL_TRUE;
   slang_ir_storage *store = NULL;
   int dbg = 0;
   const GLenum datatype = _slang_gltype_from_specifier(&var->type.specifier);
   const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
   const GLint arrayLen = _slang_array_length(var);
   const GLint totalSize = _slang_array_size(size, arrayLen);
   GLint texIndex = sampler_to_texture_index(var->type.specifier.type);

   /* check for sampler2D arrays */
   if (texIndex == -1 && var->type.specifier._array)
      texIndex = sampler_to_texture_index(var->type.specifier._array->type);

   if (texIndex != -1) {
      /* This is a texture sampler variable...
       * store->File = PROGRAM_SAMPLER
       * store->Index = sampler number (0..7, typically)
       * store->Size = texture type index (1D, 2D, 3D, cube, etc)
       */
      if (var->initializer) {
         slang_info_log_error(A->log, "illegal assignment to '%s'", varName);
         return GL_FALSE;
      }
#if FEATURE_es2_glsl /* XXX should use FEATURE_texture_rect */
      /* disallow rect samplers */
      if (is_rect_sampler_spec(&var->type.specifier)) {
         slang_info_log_error(A->log, "invalid sampler type for '%s'", varName);
         return GL_FALSE;
      }
#else
      (void) is_rect_sampler_spec; /* silence warning */
#endif
      {
         GLint sampNum = _mesa_add_sampler(prog->Parameters, varName, datatype);
         store = _slang_new_ir_storage_sampler(sampNum, texIndex, totalSize);

         /* If we have a sampler array, then we need to allocate the 
	  * additional samplers to ensure we don't allocate them elsewhere.
	  * We can't directly use _mesa_add_sampler() as that checks the
	  * varName and gets a match, so we call _mesa_add_parameter()
	  * directly and use the last sampler number from the call above.
	  */
	 if (arrayLen > 0) {
	    GLint a = arrayLen - 1;
	    GLint i;
	    for (i = 0; i < a; i++) {
               GLfloat value = (GLfloat)(i + sampNum + 1);
               (void) _mesa_add_parameter(prog->Parameters, PROGRAM_SAMPLER,
                                 varName, 1, datatype, &value, NULL, 0x0);
	    }
	 }
      }
      if (dbg) printf("SAMPLER ");
   }
   else if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      /* Uniform variable */
      const GLuint swizzle = _slang_var_swizzle(totalSize, 0);

      if (prog) {
         /* user-defined uniform */
         if (datatype == GL_NONE) {
            if (var->type.specifier.type == SLANG_SPEC_STRUCT) {
               /* temporary work-around */
               GLenum datatype = GL_FLOAT;
               GLint uniformLoc = _mesa_add_uniform(prog->Parameters, varName,
                                                    totalSize, datatype, NULL);
               store = _slang_new_ir_storage_swz(PROGRAM_UNIFORM, uniformLoc,
                                                 totalSize, swizzle);

               /* XXX what we need to do is unroll the struct into its
                * basic types, creating a uniform variable for each.
                * For example:
                * struct foo {
                *   vec3 a;
                *   vec4 b;
                * };
                * uniform foo f;
                *
                * Should produce uniforms:
                * "f.a"  (GL_FLOAT_VEC3)
                * "f.b"  (GL_FLOAT_VEC4)
                */

               if (var->initializer) {
                  slang_info_log_error(A->log,
                     "unsupported initializer for uniform '%s'", varName);
                  return GL_FALSE;
               }
            }
            else {
               slang_info_log_error(A->log,
                                    "invalid datatype for uniform variable %s",
                                    varName);
               return GL_FALSE;
            }
         }
         else {
            /* non-struct uniform */
            if (!_slang_gen_var_decl(A, var, var->initializer))
               return GL_FALSE;
            store = var->store;
         }
      }
      else {
         /* pre-defined uniform, like gl_ModelviewMatrix */
         /* We know it's a uniform, but don't allocate storage unless
          * it's really used.
          */
         store = _slang_new_ir_storage_swz(PROGRAM_STATE_VAR, -1,
                                           totalSize, swizzle);
      }
      if (dbg) printf("UNIFORM (sz %d) ", totalSize);
   }
   else if (var->type.qualifier == SLANG_QUAL_VARYING) {
      /* varyings must be float, vec or mat */
      if (!_slang_type_is_float_vec_mat(var->type.specifier.type) &&
          var->type.specifier.type != SLANG_SPEC_ARRAY) {
         slang_info_log_error(A->log,
                              "varying '%s' must be float/vector/matrix",
                              varName);
         return GL_FALSE;
      }

      if (var->initializer) {
         slang_info_log_error(A->log, "illegal initializer for varying '%s'",
                              varName);
         return GL_FALSE;
      }

      if (prog) {
         /* user-defined varying */
         GLbitfield flags;
         GLint varyingLoc;
         GLuint swizzle;

         flags = 0x0;
         if (var->type.centroid == SLANG_CENTROID)
            flags |= PROG_PARAM_BIT_CENTROID;
         if (var->type.variant == SLANG_INVARIANT)
            flags |= PROG_PARAM_BIT_INVARIANT;

         varyingLoc = _mesa_add_varying(prog->Varying, varName,
                                        totalSize, flags);
         swizzle = _slang_var_swizzle(size, 0);
         store = _slang_new_ir_storage_swz(PROGRAM_VARYING, varyingLoc,
                                           totalSize, swizzle);
      }
      else {
         /* pre-defined varying, like gl_Color or gl_TexCoord */
         if (type == SLANG_UNIT_FRAGMENT_BUILTIN) {
            /* fragment program input */
            GLuint swizzle;
            GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB,
                                             &swizzle);
            assert(index >= 0);
            assert(index < FRAG_ATTRIB_MAX);
            store = _slang_new_ir_storage_swz(PROGRAM_INPUT, index,
                                              size, swizzle);
         }
         else {
            /* vertex program output */
            GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
            GLuint swizzle = _slang_var_swizzle(size, 0);
            assert(index >= 0);
            assert(index < VERT_RESULT_MAX);
            assert(type == SLANG_UNIT_VERTEX_BUILTIN);
            store = _slang_new_ir_storage_swz(PROGRAM_OUTPUT, index,
                                              size, swizzle);
         }
         if (dbg) printf("V/F ");
      }
      if (dbg) printf("VARYING ");
   }
   else if (var->type.qualifier == SLANG_QUAL_ATTRIBUTE) {
      GLuint swizzle;
      GLint index;
      /* attributes must be float, vec or mat */
      if (!_slang_type_is_float_vec_mat(var->type.specifier.type)) {
         slang_info_log_error(A->log,
                              "attribute '%s' must be float/vector/matrix",
                              varName);
         return GL_FALSE;
      }

      if (prog) {
         /* user-defined vertex attribute */
         const GLint attr = -1; /* unknown */
         swizzle = _slang_var_swizzle(size, 0);
         index = _mesa_add_attribute(prog->Attributes, varName,
                                     size, datatype, attr);
         assert(index >= 0);
         index = VERT_ATTRIB_GENERIC0 + index;
      }
      else {
         /* pre-defined vertex attrib */
         index = _slang_input_index(varName, GL_VERTEX_PROGRAM_ARB, &swizzle);
         assert(index >= 0);
      }
      store = _slang_new_ir_storage_swz(PROGRAM_INPUT, index, size, swizzle);
      if (dbg) printf("ATTRIB ");
   }
   else if (var->type.qualifier == SLANG_QUAL_FIXEDINPUT) {
      GLuint swizzle = SWIZZLE_XYZW; /* silence compiler warning */
      GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB,
                                       &swizzle);
      store = _slang_new_ir_storage_swz(PROGRAM_INPUT, index, size, swizzle);
      if (dbg) printf("INPUT ");
   }
   else if (var->type.qualifier == SLANG_QUAL_FIXEDOUTPUT) {
      if (type == SLANG_UNIT_VERTEX_BUILTIN) {
         GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
      }
      else {
         GLint index = _slang_output_index(varName, GL_FRAGMENT_PROGRAM_ARB);
         GLint specialSize = 4; /* treat all fragment outputs as float[4] */
         assert(type == SLANG_UNIT_FRAGMENT_BUILTIN);
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, specialSize);
      }
      if (dbg) printf("OUTPUT ");
   }
   else if (var->type.qualifier == SLANG_QUAL_CONST && !prog) {
      /* pre-defined global constant, like gl_MaxLights */
      store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, size);
      if (dbg) printf("CONST ");
   }
   else {
      /* ordinary variable (may be const) */
      slang_ir_node *n;

      /* IR node to declare the variable */
      n = _slang_gen_var_decl(A, var, var->initializer);

      /* emit GPU instructions */
      success = _slang_emit_code(n, A->vartable, A->program, A->pragmas, GL_FALSE, A->log);

      _slang_free_ir_tree(n);
   }

   if (dbg) printf("GLOBAL VAR %s  idx %d\n", (char*) var->a_name,
                   store ? store->Index : -2);

   if (store)
      var->store = store;  /* save var's storage info */

   var->declared = GL_TRUE;

   return success;
}


/**
 * Produce an IR tree from a function AST (fun->body).
 * Then call the code emitter to convert the IR tree into gl_program
 * instructions.
 */
GLboolean
_slang_codegen_function(slang_assemble_ctx * A, slang_function * fun)
{
   slang_ir_node *n;
   GLboolean success = GL_TRUE;

   if (_mesa_strcmp((char *) fun->header.a_name, "main") != 0) {
      /* we only really generate code for main, all other functions get
       * inlined or codegen'd upon an actual call.
       */
#if 0
      /* do some basic error checking though */
      if (fun->header.type.specifier.type != SLANG_SPEC_VOID) {
         /* check that non-void functions actually return something */
         slang_operation *op
            = _slang_find_node_type(fun->body, SLANG_OPER_RETURN);
         if (!op) {
            slang_info_log_error(A->log,
                                 "function \"%s\" has no return statement",
                                 (char *) fun->header.a_name);
            printf(
                   "function \"%s\" has no return statement\n",
                   (char *) fun->header.a_name);
            return GL_FALSE;
         }
      }
#endif
      return GL_TRUE;  /* not an error */
   }

#if 0
   printf("\n*********** codegen_function %s\n", (char *) fun->header.a_name);
   slang_print_function(fun, 1);
#endif

   /* should have been allocated earlier: */
   assert(A->program->Parameters );
   assert(A->program->Varying);
   assert(A->vartable);
   A->CurLoop = NULL;
   A->CurFunction = fun;

   /* fold constant expressions, etc. */
   _slang_simplify(fun->body, &A->space, A->atoms);

#if 0
   printf("\n*********** simplified %s\n", (char *) fun->header.a_name);
   slang_print_function(fun, 1);
#endif

   /* Create an end-of-function label */
   A->curFuncEndLabel = _slang_label_new("__endOfFunc__main");

   /* push new vartable scope */
   _slang_push_var_table(A->vartable);

   /* Generate IR tree for the function body code */
   n = _slang_gen_operation(A, fun->body);
   if (n)
      n = new_node1(IR_SCOPE, n);

   /* pop vartable, restore previous */
   _slang_pop_var_table(A->vartable);

   if (!n) {
      /* XXX record error */
      return GL_FALSE;
   }

   /* append an end-of-function-label to IR tree */
   n = new_seq(n, new_label(A->curFuncEndLabel));

   /*_slang_label_delete(A->curFuncEndLabel);*/
   A->curFuncEndLabel = NULL;

#if 0
   printf("************* New AST for %s *****\n", (char*)fun->header.a_name);
   slang_print_function(fun, 1);
#endif
#if 0
   printf("************* IR for %s *******\n", (char*)fun->header.a_name);
   _slang_print_ir_tree(n, 0);
#endif
#if 0
   printf("************* End codegen function ************\n\n");
#endif

   /* Emit program instructions */
   success = _slang_emit_code(n, A->vartable, A->program, A->pragmas, GL_TRUE, A->log);
   _slang_free_ir_tree(n);

   /* free codegen context */
   /*
   _mesa_free(A->codegen);
   */

   return success;
}

