/*
 * Mesa 3-D graphics library
 * Version:  7.1
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


static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper);


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
      if (sz > 4) {
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
      if (n->Var && n->Var->aux) {
         /* node storage info = var storage info */
         n->Store = (slang_ir_storage *) n->Var->aux;
      }
      else {
         /* alloc new storage info */
         n->Store = _slang_new_ir_storage(PROGRAM_UNDEFINED, -1, -5);
         if (n->Var)
            n->Var->aux = n->Store;
         assert(n->Var->aux);
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
      { "gl_FogFragCoord", FRAG_ATTRIB_FOGC, SWIZZLE_XXXX },
      { "gl_TexCoord", FRAG_ATTRIB_TEX0, SWIZZLE_NOOP },
      { "gl_FrontFacing", FRAG_ATTRIB_FOGC, SWIZZLE_YYYY }, /*XXX*/
      { NULL, 0, SWIZZLE_NOOP }
   };
   GLuint i;
   const struct input_info *inputs
      = (target == GL_VERTEX_PROGRAM_ARB) ? vertInputs : fragInputs;

   ASSERT(MAX_TEXTURE_UNITS == 8); /* if this fails, fix vertInputs above */

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
   { "vec4_texp_rect", IR_TEX, 1, 2 },/* rectangle w/ projection */

   /* unary op */
   { "int_to_float", IR_I_TO_F, 1, 1 },
   { "float_to_int", IR_F_TO_I, 1, 1 },
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
      n->Writemask = WRITEMASK_XYZW;
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
 * Inlined subroutine.
 */
static slang_ir_node *
new_inlined_function_call(slang_ir_node *code, slang_label *name)
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
new_var(slang_assemble_ctx *A, slang_operation *oper, slang_atom name)
{
   slang_ir_node *n;
   slang_variable *var = _slang_locate_variable(oper->locals, name, GL_TRUE);
   if (!var)
      return NULL;

   assert(!oper->var || oper->var == var);

   n = new_node0(IR_VAR);
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


static void
slang_resolve_variable(slang_operation *oper)
{
   if (oper->type == SLANG_OPER_IDENTIFIER && !oper->var) {
      oper->var = _slang_locate_variable(oper->locals, oper->a_id, GL_TRUE);
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
         slang_variable *v = _slang_locate_variable(oper->locals,
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
         v = _slang_locate_variable(oper->locals, id, GL_TRUE);
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
      for (i = 0; i < numArgs; i++) {
         inlined->children[i] = inlined->children[i + 1];
      }
      inlined->num_children--;
   }

   /* now do formal->actual substitutions */
   slang_substitute(A, inlined, numArgs, substOld, substNew, GL_FALSE);

   _slang_free(substOld);
   _slang_free(substNew);

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
   printf("Inline call to %s  (total vars=%d  nparams=%d)\n",
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
   assert(inlined->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE);
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
         /*
         printf("COPY_IN %s from expr\n", (char*)p->a_name);
         */
	 decl->type = SLANG_OPER_VARIABLE_DECL;
         assert(decl->locals);
         decl->locals->outer_scope = inlined->locals;
	 decl->a_id = p->a_name;
	 decl->num_children = 1;
	 decl->children = slang_operation_new(1);

         /* child[0] is the var's initializer */
         slang_operation_copy(&decl->children[0], args + i);

	 numCopyIn++;
      }
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

#if 0
   printf("Done Inline call to %s  (total vars=%d  nparams=%d)\n",
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
      inlined = slang_inline_function_call(A, fun, oper, dest);
      if (inlined && _slang_find_node_type(inlined, SLANG_OPER_RETURN)) {
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
         callOper->type = SLANG_OPER_INLINED_CALL;
         callOper->fun = fun;
         callOper->label = _slang_label_new_unique((char*) fun->header.a_name);
      }
   }

   if (!inlined)
      return NULL;

   /* Replace the function call with the inlined block */
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


static GLuint
make_writemask(const char *field)
{
   GLuint mask = 0x0;
   while (*field) {
      switch (*field) {
      case 'x':
      case 's':
      case 'r':
         mask |= WRITEMASK_X;
         break;
      case 'y':
      case 't':
      case 'g':
         mask |= WRITEMASK_Y;
         break;
      case 'z':
      case 'p':
      case 'b':
         mask |= WRITEMASK_Z;
         break;
      case 'w':
      case 'q':
      case 'a':
         mask |= WRITEMASK_W;
         break;
      default:
         _mesa_problem(NULL, "invalid writemask in make_writemask()");
         return 0;
      }
      field++;
   }
   if (mask == 0x0)
      return WRITEMASK_XYZW;
   else
      return mask;
}


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
       * Children[0], [1] are the operands.
       */
      firstOperand = 0;
   }
   else {
      /* Storage for result (child[0]) is specified.
       * Children[1], [2] are the operands.
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
      GLuint writemask = WRITEMASK_XYZW;
      slang_operation *dest_oper;
      slang_ir_node *n0;

      dest_oper = &oper->children[0];
      while (dest_oper->type == SLANG_OPER_FIELD) {
         /* writemask */
         writemask &= make_writemask((char*) dest_oper->a_id);
         dest_oper = &dest_oper->children[0];
      }

      n0 = _slang_gen_operation(A, dest_oper);
      assert(n0->Var);
      assert(n0->Store);
      assert(!n->Store);
      n->Store = n0->Store;
      n->Writemask = writemask;

      _slang_free(n0);
   }

   return n;
}


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


/**
 * Return first function in the scope that has the given name.
 * This is the function we'll try to call when there is no exact match
 * between function parameters and call arguments.
 *
 * XXX we should really create a list of candidate functions and try
 * all of them...
 */
static slang_function *
_slang_first_function(struct slang_function_scope_ *scope, const char *name)
{
   GLuint i;
   for (i = 0; i < scope->num_functions; i++) {
      slang_function *f = &scope->functions[i];
      if (strcmp(name, (char*) f->header.a_name) == 0)
         return f;
   }
   if (scope->outer_scope)
      return _slang_first_function(scope->outer_scope, name);
   return NULL;
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

   atom = slang_atom_pool_atom(A->atoms, name);
   if (atom == SLANG_ATOM_NULL)
      return NULL;

   /*
    * Use 'name' to find the function to call
    */
   fun = _slang_locate_function(A->space.funcs, atom, params, param_count,
				&A->space, A->atoms, A->log);
   if (!fun) {
      /* A function with exactly the right parameters/types was not found.
       * Try adapting the parameters.
       */
      fun = _slang_first_function(A->space.funcs, name);
      if (!fun || !_slang_adapt_call(oper, fun, &A->space, A->atoms, A->log)) {
         slang_info_log_error(A->log, "Function '%s' not found (check argument types)", name);
         return NULL;
      }
      assert(fun);
   }

   return _slang_gen_function_call(A, fun, oper, dest);
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
   _slang_typeof_operation(A, oper, &type);
   size = _slang_sizeof_type_specifier(&type.spec);
   slang_typeinfo_destruct(&type);
   return size == 1;
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
   if (!_slang_is_scalar_or_boolean(A, &oper->children[0])) {
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
   if (!_slang_is_scalar_or_boolean(A, &oper->children[1])) {
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
 * Generate for-loop using high-level IR_LOOP instruction.
 */
static slang_ir_node *
_slang_gen_for(slang_assemble_ctx * A, const slang_operation *oper)
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

   if (is_operation_type(&oper->children[1], SLANG_OPER_BREAK)) {
      /* Special case: generate a conditional break */
      ifBody = new_break_if_true(A->CurLoop, cond);
      if (haveElseClause) {
         elseBody = _slang_gen_operation(A, &oper->children[2]);
         return new_seq(ifBody, elseBody);
      }
      return ifBody;
   }
   else if (is_operation_type(&oper->children[1], SLANG_OPER_CONTINUE)) {
      /* Special case: generate a conditional break */
      ifBody = new_cont_if_true(A->CurLoop, cond);
      if (haveElseClause) {
         elseBody = _slang_gen_operation(A, &oper->children[2]);
         return new_seq(ifBody, elseBody);
      }
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

   store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, size);
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
 * Generate IR node for allocating/declaring a variable.
 */
static slang_ir_node *
_slang_gen_var_decl(slang_assemble_ctx *A, slang_variable *var)
{
   slang_ir_node *n;
   assert(!is_sampler_type(&var->type));
   n = new_node0(IR_VAR_DECL);
   if (n) {
      _slang_attach_storage(n, var);

      assert(var->aux);
      assert(n->Store == var->aux);
      assert(n->Store);
      assert(n->Store->Index < 0);

      n->Store->File = PROGRAM_TEMPORARY;
      n->Store->Size = _slang_sizeof_type_specifier(&n->Var->type.specifier);
      assert(n->Store->Size > 0);
   }
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
   slang_typeinfo type;
   int size;

   assert(oper->type == SLANG_OPER_SELECT);
   assert(oper->num_children == 3);

   /* size of x or y's type */
   slang_typeinfo_construct(&type);
   _slang_typeof_operation(A, &oper->children[1], &type);
   size = _slang_sizeof_type_specifier(&type.spec);
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
   trueNode = new_node2(IR_MOVE, tmpVar, trueExpr);

   /* if-false body (child 2) */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;
   falseExpr = _slang_gen_operation(A, &oper->children[2]);
   falseNode = new_node2(IR_MOVE, tmpVar, falseExpr);

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
         slang_variable *v
            = _slang_locate_variable(oper->locals, a_retVal, GL_TRUE);
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


/**
 * Generate IR tree for a variable declaration.
 */
static slang_ir_node *
_slang_gen_declaration(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_ir_node *n;
   slang_ir_node *varDecl;
   slang_variable *v;
   const char *varName = (char *) oper->a_id;

   assert(oper->num_children == 0 || oper->num_children == 1);

   v = _slang_locate_variable(oper->locals, oper->a_id, GL_TRUE);
   assert(v);

   varDecl = _slang_gen_var_decl(A, v);

   if (oper->num_children > 0) {
      /* child is initializer */
      slang_ir_node *var, *init, *rhs;
      assert(oper->num_children == 1);
      var = new_var(A, oper, oper->a_id);
      if (!var) {
         slang_info_log_error(A->log, "undefined variable '%s'", varName);
         return NULL;
      }
      /* XXX make copy of this initializer? */
      rhs = _slang_gen_operation(A, &oper->children[0]);
      if (!rhs)
         return NULL;  /* must have found an error */
      init = new_node2(IR_MOVE, var, rhs);
      /*assert(rhs->Opcode != IR_SEQ);*/
      n = new_seq(varDecl, init);
   }
   else if (v->initializer) {
      slang_ir_node *var, *init, *rhs;
      var = new_var(A, oper, oper->a_id);
      if (!var) {
         slang_info_log_error(A->log, "undefined variable '%s'", varName);
         return NULL;
      }
#if 0
      /* XXX make copy of this initializer? */
      {
         slang_operation dup;
         slang_operation_construct(&dup);
         slang_operation_copy(&dup, v->initializer);
         _slang_simplify(&dup, &A->space, A->atoms); 
         rhs = _slang_gen_operation(A, &dup);
      }
#else
      _slang_simplify(v->initializer, &A->space, A->atoms); 
      rhs = _slang_gen_operation(A, v->initializer);
#endif
      if (!rhs)
         return NULL;

      assert(rhs);
      init = new_node2(IR_MOVE, var, rhs);
      /*
        assert(rhs->Opcode != IR_SEQ);
      */
      n = new_seq(varDecl, init);
   }
   else {
      n = varDecl;
   }
   return n;
}


/**
 * Generate IR tree for a variable (such as in an expression).
 */
static slang_ir_node *
_slang_gen_variable(slang_assemble_ctx * A, slang_operation *oper)
{
   /* If there's a variable associated with this oper (from inlining)
    * use it.  Otherwise, use the oper's var id.
    */
   slang_atom aVar = oper->var ? oper->var->a_name : oper->a_id;
   slang_ir_node *n = new_var(A, oper, aVar);
   if (!n) {
      slang_info_log_error(A->log, "undefined variable '%s'", (char *) aVar);
      return NULL;
   }
   return n;
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
 * \param swizzle  the incoming swizzle
 * \param writemaskOut  returns the writemask
 * \param swizzleOut  swizzle to apply to the right-hand-side
 * \return GL_FALSE for simple writemasks, GL_TRUE for non-simple
 */
static GLboolean
swizzle_to_writemask(GLuint swizzle,
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


static slang_ir_node *
_slang_gen_swizzle(slang_ir_node *child, GLuint swizzle)
{
   slang_ir_node *n = new_node1(IR_SWIZZLE, child);
   assert(child);
   if (n) {
      n->Store = _slang_new_ir_storage(PROGRAM_UNDEFINED, -1, -1);
      n->Store->Swizzle = swizzle;
   }
   return n;
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
         = _slang_locate_variable(oper->children[0].locals,
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
      lhs = _slang_gen_operation(A, &oper->children[0]);

      if (lhs) {
         if (!(lhs->Store->File == PROGRAM_OUTPUT ||
               lhs->Store->File == PROGRAM_TEMPORARY ||
               (lhs->Store->File == PROGRAM_VARYING &&
                A->program->Target == GL_VERTEX_PROGRAM_ARB) ||
               lhs->Store->File == PROGRAM_UNDEFINED)) {
            slang_info_log_error(A->log,
                                 "illegal assignment to read-only l-value");
            return NULL;
         }
      }

      rhs = _slang_gen_operation(A, &oper->children[1]);
      if (lhs && rhs) {
         /* convert lhs swizzle into writemask */
         GLuint writemask, newSwizzle;
         if (!swizzle_to_writemask(lhs->Store->Swizzle,
                                   &writemask, &newSwizzle)) {
            /* Non-simple writemask, need to swizzle right hand side in
             * order to put components into the right place.
             */
            rhs = _slang_gen_swizzle(rhs, newSwizzle);
         }
         n = new_node2(IR_MOVE, lhs, rhs);
         n->Writemask = writemask;
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
_slang_gen_field(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_typeinfo ti;

   /* type of struct */
   slang_typeinfo_construct(&ti);
   _slang_typeof_operation(A, &oper->children[0], &ti);

   if (_slang_type_is_vector(ti.spec.type)) {
      /* the field should be a swizzle */
      const GLuint rows = _slang_type_dim(ti.spec.type);
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
      if (n)
         n = _slang_gen_swizzle(n, swizzle);
      return n;
   }
   else if (   ti.spec.type == SLANG_SPEC_FLOAT
            || ti.spec.type == SLANG_SPEC_INT) {
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
      _slang_typeof_operation(A, oper, &field_ti);

      fieldSize = _slang_sizeof_type_specifier(&field_ti.spec);
      if (fieldSize > 0)
         fieldOffset = _slang_field_offset(&ti.spec, oper->a_id);

      if (fieldSize == 0 || fieldOffset < 0) {
         slang_info_log_error(A->log,
                              "\"%s\" is not a member of struct \"%s\"",
                              (char *) oper->a_id,
                              (char *) ti.spec._struct->a_name);
         return NULL;
      }
      assert(fieldSize >= 0);

      base = _slang_gen_operation(A, &oper->children[0]);
      if (!base) {
         /* error msg should have already been logged */
         return NULL;
      }

      n = new_node1(IR_FIELD, base);
      if (n) {
         n->Field = (char *) oper->a_id;
         n->FieldOffset = fieldOffset;
         assert(n->FieldOffset >= 0);
         n->Store = _slang_new_ir_storage(base->Store->File,
                                          base->Store->Index,
                                          fieldSize);
      }
      return n;

#if 0
      _mesa_problem(NULL, "glsl structs/fields not supported yet");
      return NULL;
#endif
   }
}


/**
 * Gen code for array indexing.
 */
static slang_ir_node *
_slang_gen_subscript(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_typeinfo array_ti;

   /* get array's type info */
   slang_typeinfo_construct(&array_ti);
   _slang_typeof_operation(A, &oper->children[0], &array_ti);

   if (_slang_type_is_vector(array_ti.spec.type)) {
      /* indexing a simple vector type: "vec4 v; v[0]=p;" */
      /* translate the index into a swizzle/writemask: "v.x=p" */
      const GLuint max = _slang_type_dim(array_ti.spec.type);
      GLint index;
      slang_ir_node *n;

      index = (GLint) oper->children[1].literal[0];
      if (oper->children[1].type != SLANG_OPER_LITERAL_INT ||
          index >= max) {
         slang_info_log_error(A->log, "Invalid array index for vector type");
         return NULL;
      }

      n = _slang_gen_operation(A, &oper->children[0]);
      if (n) {
         /* use swizzle to access the element */
         GLuint swizzle = MAKE_SWIZZLE4(SWIZZLE_X + index,
                                        SWIZZLE_NIL,
                                        SWIZZLE_NIL,
                                        SWIZZLE_NIL);
         n = _slang_gen_swizzle(n, swizzle);
         /*n->Store = _slang_clone_ir_storage_swz(n->Store, */
         n->Writemask = WRITEMASK_X << index;
      }
      return n;
   }
   else {
      /* conventional array */
      slang_typeinfo elem_ti;
      slang_ir_node *elem, *array, *index;
      GLint elemSize, arrayLen;

      /* size of array element */
      slang_typeinfo_construct(&elem_ti);
      _slang_typeof_operation(A, oper, &elem_ti);
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
         if (index->Opcode == IR_FLOAT &&
             ((int) index->Value[0] < 0 ||
              (int) index->Value[0] >= arrayLen)) {
            slang_info_log_error(A->log,
                                "Array index out of bounds (index=%d size=%d)",
                                 (int) index->Value[0], arrayLen);
            _slang_free_ir_tree(array);
            _slang_free_ir_tree(index);
            return NULL;
         }

         elem = new_node2(IR_ELEMENT, array, index);
         elem->Store = _slang_new_ir_storage(array->Store->File,
                                             array->Store->Index,
                                             elemSize);
         /* XXX try to do some array bounds checking here */
         return elem;
      }
      else {
         _slang_free_ir_tree(array);
         _slang_free_ir_tree(index);
         return NULL;
      }
   }
}


/**
 * Look for expressions such as:  gl_ModelviewMatrix * gl_Vertex
 * and replace with this:  gl_Vertex * gl_ModelviewMatrixTranpose
 * Since matrices are stored in column-major order, the second form of
 * multiplication is much more efficient (just 4 dot products).
 */
static void
_slang_check_matmul_optimization(slang_assemble_ctx *A, slang_operation *oper)
{
   static const struct {
      const char *orig;
      const char *tranpose;
   } matrices[] = {
      {"gl_ModelViewMatrix", "gl_ModelViewMatrixTranspose"},
      {"gl_ProjectionMatrix", "gl_ProjectionMatrixTranspose"},
      {"gl_ModelViewProjectionMatrix", "gl_ModelViewProjectionMatrixTranspose"},
      {"gl_TextureMatrix", "gl_TextureMatrixTranspose"},
      {"gl_NormalMatrix", "__NormalMatrixTranspose"},
      { NULL, NULL }
   };

   assert(oper->type == SLANG_OPER_MULTIPLY);
   if (oper->children[0].type == SLANG_OPER_IDENTIFIER) {
      GLuint i;
      for (i = 0; matrices[i].orig; i++) {
         if (oper->children[0].a_id
             == slang_atom_pool_atom(A->atoms, matrices[i].orig)) {
            /*
            _mesa_printf("Replace %s with %s\n",
                         matrices[i].orig, matrices[i].tranpose);
            */
            assert(oper->children[0].type == SLANG_OPER_IDENTIFIER);
            oper->children[0].a_id
               = slang_atom_pool_atom(A->atoms, matrices[i].tranpose);
            /* finally, swap the operands */
            _slang_operation_swap(&oper->children[0], &oper->children[1]);
            return;
         }
      }
   }
}


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
            tree = tree ? new_seq(tree, n) : n;
         }

#if 00
         if (oper->locals->num_variables > 0) {
            int i;
            /*
            printf("\n****** Deallocate vars in scope!\n");
            */
            for (i = 0; i < oper->locals->num_variables; i++) {
               slang_variable *v = oper->locals->variables + i;
               if (v->aux) {
                  slang_ir_storage *store = (slang_ir_storage *) v->aux;
                  /*
                  printf("  Deallocate var %s\n", (char*) v->a_name);
                  */
                  assert(store->File == PROGRAM_TEMPORARY);
                  assert(store->Index >= 0);
                  _slang_free_temp(A->vartable, store->Index, store->Size);
               }
            }
         }
#endif
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
      return new_node2(IR_EQUAL,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case SLANG_OPER_NOTEQUAL:
      return new_node2(IR_NOTEQUAL,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case SLANG_OPER_GREATER:
      return new_node2(IR_SGT,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case SLANG_OPER_LESS:
      return new_node2(IR_SLT,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case SLANG_OPER_GREATEREQUAL:
      return new_node2(IR_SGE,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case SLANG_OPER_LESSEQUAL:
      return new_node2(IR_SLE,
                       _slang_gen_operation(A, &oper->children[0]),
                       _slang_gen_operation(A, &oper->children[1]));
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
         _slang_check_matmul_optimization(A, oper);
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
	 n = _slang_gen_function_call_name(A, "+=", oper, &oper->children[0]);
	 return n;
      }
   case SLANG_OPER_SUBASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-=", oper, &oper->children[0]);
	 return n;
      }
      break;
   case SLANG_OPER_MULASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "*=", oper, &oper->children[0]);
	 return n;
      }
   case SLANG_OPER_DIVASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/=", oper, &oper->children[0]);
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
   case SLANG_OPER_RETURN:
      return _slang_gen_return(A, oper);
   case SLANG_OPER_LABEL:
      return new_label(oper->label);
   case SLANG_OPER_IDENTIFIER:
      return _slang_gen_variable(A, oper);
   case SLANG_OPER_IF:
      return _slang_gen_if(A, oper);
   case SLANG_OPER_FIELD:
      return _slang_gen_field(A, oper);
   case SLANG_OPER_SUBSCRIPT:
      return _slang_gen_subscript(A, oper);
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

   case SLANG_OPER_INLINED_CALL:
   case SLANG_OPER_SEQUENCE:
      {
         slang_ir_node *tree = NULL;
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            slang_ir_node *n = _slang_gen_operation(A, &oper->children[i]);
            tree = tree ? new_seq(tree, n) : n;
         }
         if (oper->type == SLANG_OPER_INLINED_CALL) {
            tree = new_inlined_function_call(tree, oper->label);
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
   const GLint texIndex = sampler_to_texture_index(var->type.specifier.type);

   if (texIndex != -1) {
      /* Texture sampler:
       * store->File = PROGRAM_SAMPLER
       * store->Index = sampler uniform location
       * store->Size = texture type index (1D, 2D, 3D, cube, etc)
       */
      GLint samplerUniform
         = _mesa_add_sampler(prog->Parameters, varName, datatype);
      store = _slang_new_ir_storage(PROGRAM_SAMPLER, samplerUniform, texIndex);
      if (dbg) printf("SAMPLER ");
   }
   else if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      /* Uniform variable */
      const GLint size = _slang_sizeof_type_specifier(&var->type.specifier)
                         * MAX2(var->array_len, 1);
      if (prog) {
         /* user-defined uniform */
         if (datatype == GL_NONE) {
            if (var->type.specifier.type == SLANG_SPEC_STRUCT) {
               _mesa_problem(NULL, "user-declared uniform structs not supported yet");
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
            }
            else {
               slang_info_log_error(A->log,
                                    "invalid datatype for uniform variable %s",
                                    (char *) var->a_name);
            }
            return GL_FALSE;
         }
         else {
            GLint uniformLoc = _mesa_add_uniform(prog->Parameters, varName,
                                                 size, datatype);
            store = _slang_new_ir_storage(PROGRAM_UNIFORM, uniformLoc, size);
         }
      }
      else {
         /* pre-defined uniform, like gl_ModelviewMatrix */
         /* We know it's a uniform, but don't allocate storage unless
          * it's really used.
          */
         store = _slang_new_ir_storage(PROGRAM_STATE_VAR, -1, size);
      }
      if (dbg) printf("UNIFORM (sz %d) ", size);
   }
   else if (var->type.qualifier == SLANG_QUAL_VARYING) {
      const GLint size = 4; /* XXX fix */
      if (prog) {
         /* user-defined varying */
         GLint varyingLoc = _mesa_add_varying(prog->Varying, varName, size);
         store = _slang_new_ir_storage(PROGRAM_VARYING, varyingLoc, size);
      }
      else {
         /* pre-defined varying, like gl_Color or gl_TexCoord */
         if (type == SLANG_UNIT_FRAGMENT_BUILTIN) {
            GLuint swizzle;
            GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB,
                                             &swizzle);
            assert(index >= 0);
            store = _slang_new_ir_storage(PROGRAM_INPUT, index, size);
            store->Swizzle = swizzle;
            assert(index < FRAG_ATTRIB_MAX);
         }
         else {
            GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
            assert(index >= 0);
            assert(type == SLANG_UNIT_VERTEX_BUILTIN);
            store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
            assert(index < VERT_RESULT_MAX);
         }
         if (dbg) printf("V/F ");
      }
      if (dbg) printf("VARYING ");
   }
   else if (var->type.qualifier == SLANG_QUAL_ATTRIBUTE) {
      if (prog) {
         /* user-defined vertex attribute */
         const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
         const GLint attr = -1; /* unknown */
         GLint index = _mesa_add_attribute(prog->Attributes, varName,
                                           size, attr);
         assert(index >= 0);
         store = _slang_new_ir_storage(PROGRAM_INPUT,
                                       VERT_ATTRIB_GENERIC0 + index, size);
      }
      else {
         /* pre-defined vertex attrib */
         GLuint swizzle;
         GLint index = _slang_input_index(varName, GL_VERTEX_PROGRAM_ARB,
                                          &swizzle);
         GLint size = 4; /* XXX? */
         assert(index >= 0);
         store = _slang_new_ir_storage(PROGRAM_INPUT, index, size);
         store->Swizzle = swizzle;
      }
      if (dbg) printf("ATTRIB ");
   }
   else if (var->type.qualifier == SLANG_QUAL_FIXEDINPUT) {
      GLuint swizzle = SWIZZLE_XYZW; /* silence compiler warning */
      GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB,
                                       &swizzle);
      GLint size = 4; /* XXX? */
      store = _slang_new_ir_storage(PROGRAM_INPUT, index, size);
      store->Swizzle = swizzle;
      if (dbg) printf("INPUT ");
   }
   else if (var->type.qualifier == SLANG_QUAL_FIXEDOUTPUT) {
      if (type == SLANG_UNIT_VERTEX_BUILTIN) {
         GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
         GLint size = 4; /* XXX? */
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
      }
      else {
         GLint index = _slang_output_index(varName, GL_FRAGMENT_PROGRAM_ARB);
         GLint size = 4; /* XXX? */
         assert(type == SLANG_UNIT_FRAGMENT_BUILTIN);
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
      }
      if (dbg) printf("OUTPUT ");
   }
   else if (var->type.qualifier == SLANG_QUAL_CONST && !prog) {
      /* pre-defined global constant, like gl_MaxLights */
      const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
      store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, size);
      if (dbg) printf("CONST ");
   }
   else {
      /* ordinary variable (may be const) */
      slang_ir_node *n;

      /* IR node to declare the variable */
      n = _slang_gen_var_decl(A, var);

      /* IR code for the var's initializer, if present */
      if (var->initializer) {
         slang_ir_node *lhs, *rhs, *init;

         /* Generate IR_MOVE instruction to initialize the variable */
         lhs = new_node0(IR_VAR);
         lhs->Var = var;
         lhs->Store = n->Store;

         /* constant folding, etc */
         _slang_simplify(var->initializer, &A->space, A->atoms);

         rhs = _slang_gen_operation(A, var->initializer);
         assert(rhs);
         init = new_node2(IR_MOVE, lhs, rhs);
         n = new_seq(n, init);
      }

      success = _slang_emit_code(n, A->vartable, A->program, GL_FALSE, A->log);

      _slang_free_ir_tree(n);
   }

   if (dbg) printf("GLOBAL VAR %s  idx %d\n", (char*) var->a_name,
                   store ? store->Index : -2);

   if (store)
      var->aux = store;  /* save var's storage info */

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
       * inlined.
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
   success = _slang_emit_code(n, A->vartable, A->program, GL_TRUE, A->log);
   _slang_free_ir_tree(n);

   /* free codegen context */
   /*
   _mesa_free(A->codegen);
   */

   return success;
}

