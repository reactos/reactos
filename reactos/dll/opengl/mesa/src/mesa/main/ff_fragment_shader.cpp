/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 * Copyright © 2010-2011 Intel Corporation
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

extern "C" {
#include "glheader.h"
#include "imports.h"
#include "mtypes.h"
#include "main/uniforms.h"
#include "main/macros.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_cache.h"
#include "program/prog_instruction.h"
#include "program/prog_print.h"
#include "program/prog_statevars.h"
#include "program/programopt.h"
#include "texenvprogram.h"
}
#include "main/uniforms.h"
#include "../glsl/glsl_types.h"
#include "../glsl/ir.h"
#include "../glsl/glsl_symbol_table.h"
#include "../glsl/glsl_parser_extras.h"
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"
#include "../program/ir_to_mesa.h"

/*
 * Note on texture units:
 *
 * The number of texture units supported by fixed-function fragment
 * processing is MAX_TEXTURE_COORD_UNITS, not MAX_TEXTURE_IMAGE_UNITS.
 * That's because there's a one-to-one correspondence between texture
 * coordinates and samplers in fixed-function processing.
 *
 * Since fixed-function vertex processing is limited to MAX_TEXTURE_COORD_UNITS
 * sets of texcoords, so is fixed-function fragment processing.
 *
 * We can safely use ctx->Const.MaxTextureUnits for loop bounds.
 */


struct texenvprog_cache_item
{
   GLuint hash;
   void *key;
   struct gl_shader_program *data;
   struct texenvprog_cache_item *next;
};

static GLboolean
texenv_doing_secondary_color(struct gl_context *ctx)
{
   if (ctx->Light.Enabled &&
       (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR))
      return GL_TRUE;

   if (ctx->Fog.ColorSumEnabled)
      return GL_TRUE;

   return GL_FALSE;
}

struct mode_opt {
#ifdef __GNUC__
   __extension__ GLubyte Source:4;  /**< SRC_x */
   __extension__ GLubyte Operand:3; /**< OPR_x */
#else
   GLubyte Source;  /**< SRC_x */
   GLubyte Operand; /**< OPR_x */
#endif
};

struct state_key {
   GLuint nr_enabled_units:8;
   GLuint enabled_units:8;
   GLuint separate_specular:1;
   GLuint fog_enabled:1;
   GLuint fog_mode:2;          /**< FOG_x */
   GLuint inputs_available:12;
   GLuint num_draw_buffers:4;

   /* NOTE: This array of structs must be last! (see "keySize" below) */
   struct {
      GLuint enabled:1;
      GLuint source_index:4;   /**< TEXTURE_x_INDEX */
      GLuint shadow:1;
      GLuint ScaleShiftRGB:2;
      GLuint ScaleShiftA:2;

      GLuint NumArgsRGB:3;  /**< up to MAX_COMBINER_TERMS */
      GLuint ModeRGB:5;     /**< MODE_x */

      GLuint NumArgsA:3;  /**< up to MAX_COMBINER_TERMS */
      GLuint ModeA:5;     /**< MODE_x */

      struct mode_opt OptRGB[MAX_COMBINER_TERMS];
      struct mode_opt OptA[MAX_COMBINER_TERMS];
   } unit[MAX_TEXTURE_UNITS];
};

#define FOG_LINEAR  0
#define FOG_EXP     1
#define FOG_EXP2    2
#define FOG_UNKNOWN 3

static GLuint translate_fog_mode( GLenum mode )
{
   switch (mode) {
   case GL_LINEAR: return FOG_LINEAR;
   case GL_EXP: return FOG_EXP;
   case GL_EXP2: return FOG_EXP2;
   default: return FOG_UNKNOWN;
   }
}

#define OPR_SRC_COLOR           0
#define OPR_ONE_MINUS_SRC_COLOR 1
#define OPR_SRC_ALPHA           2
#define OPR_ONE_MINUS_SRC_ALPHA	3
#define OPR_ZERO                4
#define OPR_ONE                 5
#define OPR_UNKNOWN             7

static GLuint translate_operand( GLenum operand )
{
   switch (operand) {
   case GL_SRC_COLOR: return OPR_SRC_COLOR;
   case GL_ONE_MINUS_SRC_COLOR: return OPR_ONE_MINUS_SRC_COLOR;
   case GL_SRC_ALPHA: return OPR_SRC_ALPHA;
   case GL_ONE_MINUS_SRC_ALPHA: return OPR_ONE_MINUS_SRC_ALPHA;
   case GL_ZERO: return OPR_ZERO;
   case GL_ONE: return OPR_ONE;
   default:
      assert(0);
      return OPR_UNKNOWN;
   }
}

#define SRC_TEXTURE  0
#define SRC_TEXTURE0 1
#define SRC_TEXTURE1 2
#define SRC_TEXTURE2 3
#define SRC_TEXTURE3 4
#define SRC_TEXTURE4 5
#define SRC_TEXTURE5 6
#define SRC_TEXTURE6 7
#define SRC_TEXTURE7 8
#define SRC_CONSTANT 9
#define SRC_PRIMARY_COLOR 10
#define SRC_PREVIOUS 11
#define SRC_ZERO     12
#define SRC_UNKNOWN  15

static GLuint translate_source( GLenum src )
{
   switch (src) {
   case GL_TEXTURE: return SRC_TEXTURE;
   case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7: return SRC_TEXTURE0 + (src - GL_TEXTURE0);
   case GL_CONSTANT: return SRC_CONSTANT;
   case GL_PRIMARY_COLOR: return SRC_PRIMARY_COLOR;
   case GL_PREVIOUS: return SRC_PREVIOUS;
   case GL_ZERO:
      return SRC_ZERO;
   default:
      assert(0);
      return SRC_UNKNOWN;
   }
}

#define MODE_REPLACE                     0  /* r = a0 */
#define MODE_MODULATE                    1  /* r = a0 * a1 */
#define MODE_ADD                         2  /* r = a0 + a1 */
#define MODE_ADD_SIGNED                  3  /* r = a0 + a1 - 0.5 */
#define MODE_INTERPOLATE                 4  /* r = a0 * a2 + a1 * (1 - a2) */
#define MODE_SUBTRACT                    5  /* r = a0 - a1 */
#define MODE_DOT3_RGB                    6  /* r = a0 . a1 */
#define MODE_DOT3_RGB_EXT                7  /* r = a0 . a1 */
#define MODE_DOT3_RGBA                   8  /* r = a0 . a1 */
#define MODE_DOT3_RGBA_EXT               9  /* r = a0 . a1 */
#define MODE_MODULATE_ADD_ATI           10  /* r = a0 * a2 + a1 */
#define MODE_MODULATE_SIGNED_ADD_ATI    11  /* r = a0 * a2 + a1 - 0.5 */
#define MODE_MODULATE_SUBTRACT_ATI      12  /* r = a0 * a2 - a1 */
#define MODE_ADD_PRODUCTS               13  /* r = a0 * a1 + a2 * a3 */
#define MODE_ADD_PRODUCTS_SIGNED        14  /* r = a0 * a1 + a2 * a3 - 0.5 */
#define MODE_BUMP_ENVMAP_ATI            15  /* special */
#define MODE_UNKNOWN                    16

/**
 * Translate GL combiner state into a MODE_x value
 */
static GLuint translate_mode( GLenum envMode, GLenum mode )
{
   switch (mode) {
   case GL_REPLACE: return MODE_REPLACE;
   case GL_MODULATE: return MODE_MODULATE;
   case GL_ADD:
      if (envMode == GL_COMBINE4_NV)
         return MODE_ADD_PRODUCTS;
      else
         return MODE_ADD;
   case GL_ADD_SIGNED:
      if (envMode == GL_COMBINE4_NV)
         return MODE_ADD_PRODUCTS_SIGNED;
      else
         return MODE_ADD_SIGNED;
   case GL_INTERPOLATE: return MODE_INTERPOLATE;
   case GL_SUBTRACT: return MODE_SUBTRACT;
   case GL_DOT3_RGB: return MODE_DOT3_RGB;
   case GL_DOT3_RGB_EXT: return MODE_DOT3_RGB_EXT;
   case GL_DOT3_RGBA: return MODE_DOT3_RGBA;
   case GL_DOT3_RGBA_EXT: return MODE_DOT3_RGBA_EXT;
   case GL_MODULATE_ADD_ATI: return MODE_MODULATE_ADD_ATI;
   case GL_MODULATE_SIGNED_ADD_ATI: return MODE_MODULATE_SIGNED_ADD_ATI;
   case GL_MODULATE_SUBTRACT_ATI: return MODE_MODULATE_SUBTRACT_ATI;
   case GL_BUMP_ENVMAP_ATI: return MODE_BUMP_ENVMAP_ATI;
   default:
      assert(0);
      return MODE_UNKNOWN;
   }
}


/**
 * Do we need to clamp the results of the given texture env/combine mode?
 * If the inputs to the mode are in [0,1] we don't always have to clamp
 * the results.
 */
static GLboolean
need_saturate( GLuint mode )
{
   switch (mode) {
   case MODE_REPLACE:
   case MODE_MODULATE:
   case MODE_INTERPOLATE:
      return GL_FALSE;
   case MODE_ADD:
   case MODE_ADD_SIGNED:
   case MODE_SUBTRACT:
   case MODE_DOT3_RGB:
   case MODE_DOT3_RGB_EXT:
   case MODE_DOT3_RGBA:
   case MODE_DOT3_RGBA_EXT:
   case MODE_MODULATE_ADD_ATI:
   case MODE_MODULATE_SIGNED_ADD_ATI:
   case MODE_MODULATE_SUBTRACT_ATI:
   case MODE_ADD_PRODUCTS:
   case MODE_ADD_PRODUCTS_SIGNED:
   case MODE_BUMP_ENVMAP_ATI:
      return GL_TRUE;
   default:
      assert(0);
      return GL_FALSE;
   }
}



/**
 * Translate TEXTURE_x_BIT to TEXTURE_x_INDEX.
 */
static GLuint translate_tex_src_bit( GLbitfield bit )
{
   ASSERT(bit);
   return _mesa_ffs(bit) - 1;
}


#define VERT_BIT_TEX_ANY    (0xff << VERT_ATTRIB_TEX0)
#define VERT_RESULT_TEX_ANY (0xff << VERT_RESULT_TEX0)

/**
 * Identify all possible varying inputs.  The fragment program will
 * never reference non-varying inputs, but will track them via state
 * constants instead.
 *
 * This function figures out all the inputs that the fragment program
 * has access to.  The bitmask is later reduced to just those which
 * are actually referenced.
 */
static GLbitfield get_fp_input_mask( struct gl_context *ctx )
{
   /* _NEW_PROGRAM */
   const GLboolean vertexShader =
      (ctx->Shader.CurrentVertexProgram &&
       ctx->Shader.CurrentVertexProgram->LinkStatus &&
       ctx->Shader.CurrentVertexProgram->_LinkedShaders[MESA_SHADER_VERTEX]);
   const GLboolean vertexProgram = ctx->VertexProgram._Enabled;
   GLbitfield fp_inputs = 0x0;

   if (ctx->VertexProgram._Overriden) {
      /* Somebody's messing with the vertex program and we don't have
       * a clue what's happening.  Assume that it could be producing
       * all possible outputs.
       */
      fp_inputs = ~0;
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      /* _NEW_RENDERMODE */
      fp_inputs = (FRAG_BIT_COL0 | FRAG_BIT_TEX0);
   }
   else if (!(vertexProgram || vertexShader)) {
      /* Fixed function vertex logic */
      /* _NEW_ARRAY */
      GLbitfield64 varying_inputs = ctx->varying_vp_inputs;

      /* These get generated in the setup routine regardless of the
       * vertex program:
       */
      /* _NEW_POINT */
      if (ctx->Point.PointSprite)
         varying_inputs |= FRAG_BITS_TEX_ANY;

      /* First look at what values may be computed by the generated
       * vertex program:
       */
      /* _NEW_LIGHT */
      if (ctx->Light.Enabled) {
         fp_inputs |= FRAG_BIT_COL0;

         if (texenv_doing_secondary_color(ctx))
            fp_inputs |= FRAG_BIT_COL1;
      }

      /* _NEW_TEXTURE */
      fp_inputs |= (ctx->Texture._TexGenEnabled |
                    ctx->Texture._TexMatEnabled) << FRAG_ATTRIB_TEX0;

      /* Then look at what might be varying as a result of enabled
       * arrays, etc:
       */
      if (varying_inputs & VERT_BIT_COLOR0)
         fp_inputs |= FRAG_BIT_COL0;
      if (varying_inputs & VERT_BIT_COLOR1)
         fp_inputs |= FRAG_BIT_COL1;

      fp_inputs |= (((varying_inputs & VERT_BIT_TEX_ANY) >> VERT_ATTRIB_TEX0) 
                    << FRAG_ATTRIB_TEX0);

   }
   else {
      /* calculate from vp->outputs */
      struct gl_program *vprog;
      GLbitfield64 vp_outputs;

      /* Choose GLSL vertex shader over ARB vertex program.  Need this
       * since vertex shader state validation comes after fragment state
       * validation (see additional comments in state.c).
       */
      if (vertexShader)
         vprog = ctx->Shader.CurrentVertexProgram->_LinkedShaders[MESA_SHADER_VERTEX]->Program;
      else
         vprog = &ctx->VertexProgram.Current->Base;

      vp_outputs = vprog->OutputsWritten;

      /* These get generated in the setup routine regardless of the
       * vertex program:
       */
      /* _NEW_POINT */
      if (ctx->Point.PointSprite)
         vp_outputs |= FRAG_BITS_TEX_ANY;

      if (vp_outputs & (1 << VERT_RESULT_COL0))
         fp_inputs |= FRAG_BIT_COL0;
      if (vp_outputs & (1 << VERT_RESULT_COL1))
         fp_inputs |= FRAG_BIT_COL1;

      fp_inputs |= (((vp_outputs & VERT_RESULT_TEX_ANY) >> VERT_RESULT_TEX0) 
                    << FRAG_ATTRIB_TEX0);
   }
   
   return fp_inputs;
}


/**
 * Examine current texture environment state and generate a unique
 * key to identify it.
 */
static GLuint make_state_key( struct gl_context *ctx,  struct state_key *key )
{
   GLuint i, j;
   GLbitfield inputs_referenced = FRAG_BIT_COL0;
   const GLbitfield inputs_available = get_fp_input_mask( ctx );
   GLuint keySize;

   memset(key, 0, sizeof(*key));

   /* _NEW_TEXTURE */
   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const struct gl_texture_object *texObj = texUnit->_Current;
      const struct gl_tex_env_combine_state *comb = texUnit->_CurrentCombine;
      GLenum format;

      if (!texUnit->_ReallyEnabled || !texUnit->Enabled)
         continue;

      format = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;

      key->unit[i].enabled = 1;
      key->enabled_units |= (1<<i);
      key->nr_enabled_units = i + 1;
      inputs_referenced |= FRAG_BIT_TEX(i);

      key->unit[i].source_index =
         translate_tex_src_bit(texUnit->_ReallyEnabled);

      key->unit[i].shadow =
         ((texObj->Sampler.CompareMode == GL_COMPARE_R_TO_TEXTURE) &&
          ((format == GL_DEPTH_COMPONENT) || 
           (format == GL_DEPTH_STENCIL_EXT)));

      key->unit[i].NumArgsRGB = comb->_NumArgsRGB;
      key->unit[i].NumArgsA = comb->_NumArgsA;

      key->unit[i].ModeRGB =
	 translate_mode(texUnit->EnvMode, comb->ModeRGB);
      key->unit[i].ModeA =
	 translate_mode(texUnit->EnvMode, comb->ModeA);

      key->unit[i].ScaleShiftRGB = comb->ScaleShiftRGB;
      key->unit[i].ScaleShiftA = comb->ScaleShiftA;

      for (j = 0; j < MAX_COMBINER_TERMS; j++) {
         key->unit[i].OptRGB[j].Operand = translate_operand(comb->OperandRGB[j]);
         key->unit[i].OptA[j].Operand = translate_operand(comb->OperandA[j]);
         key->unit[i].OptRGB[j].Source = translate_source(comb->SourceRGB[j]);
         key->unit[i].OptA[j].Source = translate_source(comb->SourceA[j]);
      }

      if (key->unit[i].ModeRGB == MODE_BUMP_ENVMAP_ATI) {
         /* requires some special translation */
         key->unit[i].NumArgsRGB = 2;
         key->unit[i].ScaleShiftRGB = 0;
         key->unit[i].OptRGB[0].Operand = OPR_SRC_COLOR;
         key->unit[i].OptRGB[0].Source = SRC_TEXTURE;
         key->unit[i].OptRGB[1].Operand = OPR_SRC_COLOR;
         key->unit[i].OptRGB[1].Source = texUnit->BumpTarget - GL_TEXTURE0 + SRC_TEXTURE0;
       }
   }

   /* _NEW_LIGHT | _NEW_FOG */
   if (texenv_doing_secondary_color(ctx)) {
      key->separate_specular = 1;
      inputs_referenced |= FRAG_BIT_COL1;
   }

   /* _NEW_FOG */
   if (ctx->Fog.Enabled) {
      key->fog_enabled = 1;
      key->fog_mode = translate_fog_mode(ctx->Fog.Mode);
      inputs_referenced |= FRAG_BIT_FOGC; /* maybe */
   }

   /* _NEW_BUFFERS */
   key->num_draw_buffers = ctx->DrawBuffer->_NumColorDrawBuffers;

   /* _NEW_COLOR */
   if (ctx->Color.AlphaEnabled && key->num_draw_buffers == 0) {
      /* if alpha test is enabled we need to emit at least one color */
      key->num_draw_buffers = 1;
   }

   key->inputs_available = (inputs_available & inputs_referenced);

   /* compute size of state key, ignoring unused texture units */
   keySize = sizeof(*key) - sizeof(key->unit)
      + key->nr_enabled_units * sizeof(key->unit[0]);

   return keySize;
}


/** State used to build the fragment program:
 */
struct texenv_fragment_program {
   struct gl_shader_program *shader_program;
   struct gl_shader *shader;
   struct gl_fragment_program *program;
   exec_list *instructions;
   exec_list *top_instructions;
   void *mem_ctx;
   struct state_key *state;

   GLbitfield alu_temps;	/**< Track texture indirections, see spec. */
   GLbitfield temps_output;	/**< Track texture indirections, see spec. */
   GLbitfield temp_in_use;	/**< Tracks temporary regs which are in use. */
   GLboolean error;

   ir_variable *src_texture[MAX_TEXTURE_COORD_UNITS];
   /* Reg containing each texture unit's sampled texture color,
    * else undef.
    */

   /* Texcoord override from bumpmapping. */
   ir_variable *texcoord_tex[MAX_TEXTURE_COORD_UNITS];

   /* Reg containing texcoord for a texture unit,
    * needed for bump mapping, else undef.
    */

   ir_rvalue *src_previous;	/**< Reg containing color from previous
				 * stage.  May need to be decl'd.
				 */

   GLuint last_tex_stage;	/**< Number of last enabled texture unit */
};

static ir_rvalue *
get_current_attrib(struct texenv_fragment_program *p, GLuint attrib)
{
   ir_variable *current;
   ir_rvalue *val;

   current = p->shader->symbols->get_variable("gl_CurrentAttribFragMESA");
   current->max_array_access = MAX2(current->max_array_access, attrib);
   val = new(p->mem_ctx) ir_dereference_variable(current);
   ir_rvalue *index = new(p->mem_ctx) ir_constant(attrib);
   return new(p->mem_ctx) ir_dereference_array(val, index);
}

static ir_rvalue *
get_gl_Color(struct texenv_fragment_program *p)
{
   if (p->state->inputs_available & FRAG_BIT_COL0) {
      ir_variable *var = p->shader->symbols->get_variable("gl_Color");
      assert(var);
      return new(p->mem_ctx) ir_dereference_variable(var);
   } else {
      return get_current_attrib(p, VERT_ATTRIB_COLOR0);
   }
}

static ir_rvalue *
get_source(struct texenv_fragment_program *p,
	   GLuint src, GLuint unit)
{
   ir_variable *var;
   ir_dereference *deref;

   switch (src) {
   case SRC_TEXTURE: 
      return new(p->mem_ctx) ir_dereference_variable(p->src_texture[unit]);

   case SRC_TEXTURE0:
   case SRC_TEXTURE1:
   case SRC_TEXTURE2:
   case SRC_TEXTURE3:
   case SRC_TEXTURE4:
   case SRC_TEXTURE5:
   case SRC_TEXTURE6:
   case SRC_TEXTURE7: 
      return new(p->mem_ctx)
	 ir_dereference_variable(p->src_texture[src - SRC_TEXTURE0]);

   case SRC_CONSTANT:
      var = p->shader->symbols->get_variable("gl_TextureEnvColor");
      assert(var);
      deref = new(p->mem_ctx) ir_dereference_variable(var);
      var->max_array_access = MAX2(var->max_array_access, unit);
      return new(p->mem_ctx) ir_dereference_array(deref,
						  new(p->mem_ctx) ir_constant(unit));

   case SRC_PRIMARY_COLOR:
      var = p->shader->symbols->get_variable("gl_Color");
      assert(var);
      return new(p->mem_ctx) ir_dereference_variable(var);

   case SRC_ZERO:
      return new(p->mem_ctx) ir_constant(0.0f);

   case SRC_PREVIOUS:
      if (!p->src_previous) {
	 return get_gl_Color(p);
      } else {
	 return p->src_previous->clone(p->mem_ctx, NULL);
      }

   default:
      assert(0);
      return NULL;
   }
}

static ir_rvalue *
emit_combine_source(struct texenv_fragment_program *p,
		    GLuint unit,
		    GLuint source,
		    GLuint operand)
{
   ir_rvalue *src;

   src = get_source(p, source, unit);

   switch (operand) {
   case OPR_ONE_MINUS_SRC_COLOR: 
      return new(p->mem_ctx) ir_expression(ir_binop_sub,
					   new(p->mem_ctx) ir_constant(1.0f),
					   src);

   case OPR_SRC_ALPHA:
      return src->type->is_scalar()
	 ? src : (ir_rvalue *) new(p->mem_ctx) ir_swizzle(src, 3, 3, 3, 3, 1);

   case OPR_ONE_MINUS_SRC_ALPHA: {
      ir_rvalue *const scalar = (src->type->is_scalar())
	 ? src : (ir_rvalue *) new(p->mem_ctx) ir_swizzle(src, 3, 3, 3, 3, 1);

      return new(p->mem_ctx) ir_expression(ir_binop_sub,
					   new(p->mem_ctx) ir_constant(1.0f),
					   scalar);
   }

   case OPR_ZERO:
      return new(p->mem_ctx) ir_constant(0.0f);
   case OPR_ONE:
      return new(p->mem_ctx) ir_constant(1.0f);
   case OPR_SRC_COLOR: 
      return src;
   default:
      assert(0);
      return src;
   }
}

/**
 * Check if the RGB and Alpha sources and operands match for the given
 * texture unit's combinder state.  When the RGB and A sources and
 * operands match, we can emit fewer instructions.
 */
static GLboolean args_match( const struct state_key *key, GLuint unit )
{
   GLuint i, numArgs = key->unit[unit].NumArgsRGB;

   for (i = 0; i < numArgs; i++) {
      if (key->unit[unit].OptA[i].Source != key->unit[unit].OptRGB[i].Source) 
	 return GL_FALSE;

      switch (key->unit[unit].OptA[i].Operand) {
      case OPR_SRC_ALPHA: 
	 switch (key->unit[unit].OptRGB[i].Operand) {
	 case OPR_SRC_COLOR: 
	 case OPR_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      case OPR_ONE_MINUS_SRC_ALPHA: 
	 switch (key->unit[unit].OptRGB[i].Operand) {
	 case OPR_ONE_MINUS_SRC_COLOR: 
	 case OPR_ONE_MINUS_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      default: 
	 return GL_FALSE;	/* impossible */
      }
   }

   return GL_TRUE;
}

static ir_rvalue *
smear(struct texenv_fragment_program *p, ir_rvalue *val)
{
   if (!val->type->is_scalar())
      return val;

   return new(p->mem_ctx) ir_swizzle(val, 0, 0, 0, 0, 4);
}

static ir_rvalue *
emit_combine(struct texenv_fragment_program *p,
	     GLuint unit,
	     GLuint nr,
	     GLuint mode,
	     const struct mode_opt *opt)
{
   ir_rvalue *src[MAX_COMBINER_TERMS];
   ir_rvalue *tmp0, *tmp1;
   GLuint i;

   assert(nr <= MAX_COMBINER_TERMS);

   for (i = 0; i < nr; i++)
      src[i] = emit_combine_source( p, unit, opt[i].Source, opt[i].Operand );

   switch (mode) {
   case MODE_REPLACE: 
      return src[0];

   case MODE_MODULATE: 
      return new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[1]);

   case MODE_ADD: 
      return new(p->mem_ctx) ir_expression(ir_binop_add, src[0], src[1]);

   case MODE_ADD_SIGNED:
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_add, src[0], src[1]);
      return new(p->mem_ctx) ir_expression(ir_binop_add, tmp0,
					   new(p->mem_ctx) ir_constant(-0.5f));

   case MODE_INTERPOLATE: 
      /* Arg0 * (Arg2) + Arg1 * (1-Arg2) */
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[2]);

      tmp1 = new(p->mem_ctx) ir_expression(ir_binop_sub,
					   new(p->mem_ctx) ir_constant(1.0f),
					   src[2]->clone(p->mem_ctx, NULL));
      tmp1 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[1], tmp1);

      return new(p->mem_ctx) ir_expression(ir_binop_add, tmp0, tmp1);

   case MODE_SUBTRACT: 
      return new(p->mem_ctx) ir_expression(ir_binop_sub, src[0], src[1]);

   case MODE_DOT3_RGBA:
   case MODE_DOT3_RGBA_EXT: 
   case MODE_DOT3_RGB_EXT:
   case MODE_DOT3_RGB: {
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0],
					   new(p->mem_ctx) ir_constant(2.0f));
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_add, tmp0,
					   new(p->mem_ctx) ir_constant(-1.0f));
      tmp0 = new(p->mem_ctx) ir_swizzle(smear(p, tmp0), 0, 1, 2, 3, 3);

      tmp1 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[1],
					   new(p->mem_ctx) ir_constant(2.0f));
      tmp1 = new(p->mem_ctx) ir_expression(ir_binop_add, tmp1,
					   new(p->mem_ctx) ir_constant(-1.0f));
      tmp1 = new(p->mem_ctx) ir_swizzle(smear(p, tmp1), 0, 1, 2, 3, 3);

      return new(p->mem_ctx) ir_expression(ir_binop_dot, tmp0, tmp1);
   }
   case MODE_MODULATE_ADD_ATI:
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[2]);
      return new(p->mem_ctx) ir_expression(ir_binop_add, tmp0, src[1]);

   case MODE_MODULATE_SIGNED_ADD_ATI:
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[2]);
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_add, tmp0, src[1]);
      return new(p->mem_ctx) ir_expression(ir_binop_add, tmp0,
					   new(p->mem_ctx) ir_constant(-0.5f));

   case MODE_MODULATE_SUBTRACT_ATI:
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[2]);
      return new(p->mem_ctx) ir_expression(ir_binop_sub, tmp0, src[1]);

   case MODE_ADD_PRODUCTS:
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[1]);
      tmp1 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[2], src[3]);
      return new(p->mem_ctx) ir_expression(ir_binop_add, tmp0, tmp1);

   case MODE_ADD_PRODUCTS_SIGNED:
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[0], src[1]);
      tmp1 = new(p->mem_ctx) ir_expression(ir_binop_mul, src[2], src[3]);
      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_add, tmp0, tmp1);
      return new(p->mem_ctx) ir_expression(ir_binop_add, tmp0,
					   new(p->mem_ctx) ir_constant(-0.5f));

   case MODE_BUMP_ENVMAP_ATI:
      /* special - not handled here */
      assert(0);
      return src[0];
   default: 
      assert(0);
      return src[0];
   }
}

static ir_rvalue *
saturate(struct texenv_fragment_program *p, ir_rvalue *val)
{
   val = new(p->mem_ctx) ir_expression(ir_binop_min, val,
				       new(p->mem_ctx) ir_constant(1.0f));
   return new(p->mem_ctx) ir_expression(ir_binop_max, val,
					new(p->mem_ctx) ir_constant(0.0f));
}

/**
 * Generate instructions for one texture unit's env/combiner mode.
 */
static ir_rvalue *
emit_texenv(struct texenv_fragment_program *p, GLuint unit)
{
   const struct state_key *key = p->state;
   GLboolean rgb_saturate, alpha_saturate;
   GLuint rgb_shift, alpha_shift;

   if (!key->unit[unit].enabled) {
      return get_source(p, SRC_PREVIOUS, 0);
   }
   if (key->unit[unit].ModeRGB == MODE_BUMP_ENVMAP_ATI) {
      /* this isn't really a env stage delivering a color and handled elsewhere */
      return get_source(p, SRC_PREVIOUS, 0);
   }
   
   switch (key->unit[unit].ModeRGB) {
   case MODE_DOT3_RGB_EXT:
      alpha_shift = key->unit[unit].ScaleShiftA;
      rgb_shift = 0;
      break;
   case MODE_DOT3_RGBA_EXT:
      alpha_shift = 0;
      rgb_shift = 0;
      break;
   default:
      rgb_shift = key->unit[unit].ScaleShiftRGB;
      alpha_shift = key->unit[unit].ScaleShiftA;
      break;
   }
   
   /* If we'll do rgb/alpha shifting don't saturate in emit_combine().
    * We don't want to clamp twice.
    */
   if (rgb_shift)
      rgb_saturate = GL_FALSE;  /* saturate after rgb shift */
   else if (need_saturate(key->unit[unit].ModeRGB))
      rgb_saturate = GL_TRUE;
   else
      rgb_saturate = GL_FALSE;

   if (alpha_shift)
      alpha_saturate = GL_FALSE;  /* saturate after alpha shift */
   else if (need_saturate(key->unit[unit].ModeA))
      alpha_saturate = GL_TRUE;
   else
      alpha_saturate = GL_FALSE;

   ir_variable *temp_var = new(p->mem_ctx) ir_variable(glsl_type::vec4_type,
						       "texenv_combine",
						       ir_var_temporary);
   p->instructions->push_tail(temp_var);

   ir_dereference *deref;
   ir_assignment *assign;
   ir_rvalue *val;

   /* Emit the RGB and A combine ops
    */
   if (key->unit[unit].ModeRGB == key->unit[unit].ModeA &&
       args_match(key, unit)) {
      val = emit_combine(p, unit,
			 key->unit[unit].NumArgsRGB,
			 key->unit[unit].ModeRGB,
			 key->unit[unit].OptRGB);
      val = smear(p, val);
      if (rgb_saturate)
	 val = saturate(p, val);

      deref = new(p->mem_ctx) ir_dereference_variable(temp_var);
      assign = new(p->mem_ctx) ir_assignment(deref, val);
      p->instructions->push_tail(assign);
   }
   else if (key->unit[unit].ModeRGB == MODE_DOT3_RGBA_EXT ||
	    key->unit[unit].ModeRGB == MODE_DOT3_RGBA) {
      ir_rvalue *val = emit_combine(p, unit,
				    key->unit[unit].NumArgsRGB,
				    key->unit[unit].ModeRGB,
				    key->unit[unit].OptRGB);
      val = smear(p, val);
      if (rgb_saturate)
	 val = saturate(p, val);
      deref = new(p->mem_ctx) ir_dereference_variable(temp_var);
      assign = new(p->mem_ctx) ir_assignment(deref, val);
      p->instructions->push_tail(assign);
   }
   else {
      /* Need to do something to stop from re-emitting identical
       * argument calculations here:
       */
      val = emit_combine(p, unit,
			 key->unit[unit].NumArgsRGB,
			 key->unit[unit].ModeRGB,
			 key->unit[unit].OptRGB);
      val = smear(p, val);
      val = new(p->mem_ctx) ir_swizzle(val, 0, 1, 2, 3, 3);
      if (rgb_saturate)
	 val = saturate(p, val);
      deref = new(p->mem_ctx) ir_dereference_variable(temp_var);
      assign = new(p->mem_ctx) ir_assignment(deref, val, NULL, WRITEMASK_XYZ);
      p->instructions->push_tail(assign);

      val = emit_combine(p, unit,
			 key->unit[unit].NumArgsA,
			 key->unit[unit].ModeA,
			 key->unit[unit].OptA);
      val = smear(p, val);
      val = new(p->mem_ctx) ir_swizzle(val, 3, 3, 3, 3, 1);
      if (alpha_saturate)
	 val = saturate(p, val);
      deref = new(p->mem_ctx) ir_dereference_variable(temp_var);
      assign = new(p->mem_ctx) ir_assignment(deref, val, NULL, WRITEMASK_W);
      p->instructions->push_tail(assign);
   }

   deref = new(p->mem_ctx) ir_dereference_variable(temp_var);

   /* Deal with the final shift:
    */
   if (alpha_shift || rgb_shift) {
      ir_constant *shift;

      if (rgb_shift == alpha_shift) {
	 shift = new(p->mem_ctx) ir_constant((float)(1 << rgb_shift));
      }
      else {
	 float const_data[4] = {
	    1 << rgb_shift,
	    1 << rgb_shift,
	    1 << rgb_shift,
	    1 << alpha_shift
	 };
	 shift = new(p->mem_ctx) ir_constant(glsl_type::vec4_type,
					     (ir_constant_data *)const_data);
      }

      return saturate(p, new(p->mem_ctx) ir_expression(ir_binop_mul,
						       deref, shift));
   }
   else
      return deref;
}


/**
 * Generate instruction for getting a texture source term.
 */
static void load_texture( struct texenv_fragment_program *p, GLuint unit )
{
   ir_dereference *deref;
   ir_assignment *assign;

   if (p->src_texture[unit])
      return;

   const GLuint texTarget = p->state->unit[unit].source_index;
   ir_rvalue *texcoord;

   if (!(p->state->inputs_available & (FRAG_BIT_TEX0 << unit))) {
      texcoord = get_current_attrib(p, VERT_ATTRIB_TEX0 + unit);
   } else if (p->texcoord_tex[unit]) {
      texcoord = new(p->mem_ctx) ir_dereference_variable(p->texcoord_tex[unit]);
   } else {
      ir_variable *tc_array = p->shader->symbols->get_variable("gl_TexCoord");
      assert(tc_array);
      texcoord = new(p->mem_ctx) ir_dereference_variable(tc_array);
      ir_rvalue *index = new(p->mem_ctx) ir_constant(unit);
      texcoord = new(p->mem_ctx) ir_dereference_array(texcoord, index);
      tc_array->max_array_access = MAX2(tc_array->max_array_access, unit);
   }

   if (!p->state->unit[unit].enabled) {
      p->src_texture[unit] = new(p->mem_ctx) ir_variable(glsl_type::vec4_type,
							 "dummy_tex",
							 ir_var_temporary);
      p->instructions->push_tail(p->src_texture[unit]);

      deref = new(p->mem_ctx) ir_dereference_variable(p->src_texture[unit]);
      assign = new(p->mem_ctx) ir_assignment(deref,
					     new(p->mem_ctx) ir_constant(0.0f));
      p->instructions->push_tail(assign);
      return ;
   }

   const glsl_type *sampler_type = NULL;
   int coords = 0;

   switch (texTarget) {
   case TEXTURE_1D_INDEX:
      if (p->state->unit[unit].shadow)
	 sampler_type = p->shader->symbols->get_type("sampler1DShadow");
      else
	 sampler_type = p->shader->symbols->get_type("sampler1D");
      coords = 1;
      break;
   case TEXTURE_1D_ARRAY_INDEX:
      if (p->state->unit[unit].shadow)
	 sampler_type = p->shader->symbols->get_type("sampler1DArrayShadow");
      else
	 sampler_type = p->shader->symbols->get_type("sampler1DArray");
      coords = 2;
      break;
   case TEXTURE_2D_INDEX:
      if (p->state->unit[unit].shadow)
	 sampler_type = p->shader->symbols->get_type("sampler2DShadow");
      else
	 sampler_type = p->shader->symbols->get_type("sampler2D");
      coords = 2;
      break;
   case TEXTURE_2D_ARRAY_INDEX:
      if (p->state->unit[unit].shadow)
	 sampler_type = p->shader->symbols->get_type("sampler2DArrayShadow");
      else
	 sampler_type = p->shader->symbols->get_type("sampler2DArray");
      coords = 3;
      break;
   case TEXTURE_RECT_INDEX:
      if (p->state->unit[unit].shadow)
	 sampler_type = p->shader->symbols->get_type("sampler2DRectShadow");
      else
	 sampler_type = p->shader->symbols->get_type("sampler2DRect");
      coords = 2;
      break;
   case TEXTURE_3D_INDEX:
      assert(!p->state->unit[unit].shadow);
      sampler_type = p->shader->symbols->get_type("sampler3D");
      coords = 3;
      break;
   case TEXTURE_CUBE_INDEX:
      if (p->state->unit[unit].shadow)
	 sampler_type = p->shader->symbols->get_type("samplerCubeShadow");
      else
	 sampler_type = p->shader->symbols->get_type("samplerCube");
      coords = 3;
      break;
   case TEXTURE_EXTERNAL_INDEX:
      assert(!p->state->unit[unit].shadow);
      sampler_type = p->shader->symbols->get_type("samplerExternalOES");
      coords = 2;
      break;
   }

   p->src_texture[unit] = new(p->mem_ctx) ir_variable(glsl_type::vec4_type,
						      "tex",
						      ir_var_temporary);
   p->instructions->push_tail(p->src_texture[unit]);

   ir_texture *tex = new(p->mem_ctx) ir_texture(ir_tex);


   char *sampler_name = ralloc_asprintf(p->mem_ctx, "sampler_%d", unit);
   ir_variable *sampler = new(p->mem_ctx) ir_variable(sampler_type,
						      sampler_name,
						      ir_var_uniform);
   p->top_instructions->push_head(sampler);
   deref = new(p->mem_ctx) ir_dereference_variable(sampler);
   tex->set_sampler(deref, glsl_type::vec4_type);

   tex->coordinate = new(p->mem_ctx) ir_swizzle(texcoord, 0, 1, 2, 3, coords);

   if (p->state->unit[unit].shadow) {
      texcoord = texcoord->clone(p->mem_ctx, NULL);
      tex->shadow_comparitor = new(p->mem_ctx) ir_swizzle(texcoord,
							  coords, 0, 0, 0,
							  1);
      coords++;
   }

   texcoord = texcoord->clone(p->mem_ctx, NULL);
   tex->projector = new(p->mem_ctx) ir_swizzle(texcoord, 3, 0, 0, 0, 1);

   deref = new(p->mem_ctx) ir_dereference_variable(p->src_texture[unit]);
   assign = new(p->mem_ctx) ir_assignment(deref, tex);
   p->instructions->push_tail(assign);
}

static void
load_texenv_source(struct texenv_fragment_program *p,
		   GLuint src, GLuint unit)
{
   switch (src) {
   case SRC_TEXTURE:
      load_texture(p, unit);
      break;

   case SRC_TEXTURE0:
   case SRC_TEXTURE1:
   case SRC_TEXTURE2:
   case SRC_TEXTURE3:
   case SRC_TEXTURE4:
   case SRC_TEXTURE5:
   case SRC_TEXTURE6:
   case SRC_TEXTURE7:       
      load_texture(p, src - SRC_TEXTURE0);
      break;
      
   default:
      /* not a texture src - do nothing */
      break;
   }
}


/**
 * Generate instructions for loading all texture source terms.
 */
static GLboolean
load_texunit_sources( struct texenv_fragment_program *p, GLuint unit )
{
   const struct state_key *key = p->state;
   GLuint i;

   for (i = 0; i < key->unit[unit].NumArgsRGB; i++) {
      load_texenv_source( p, key->unit[unit].OptRGB[i].Source, unit );
   }

   for (i = 0; i < key->unit[unit].NumArgsA; i++) {
      load_texenv_source( p, key->unit[unit].OptA[i].Source, unit );
   }

   return GL_TRUE;
}

/**
 * Generate instructions for loading bump map textures.
 */
static void
load_texunit_bumpmap( struct texenv_fragment_program *p, GLuint unit )
{
   const struct state_key *key = p->state;
   GLuint bumpedUnitNr = key->unit[unit].OptRGB[1].Source - SRC_TEXTURE0;
   ir_rvalue *bump;
   ir_rvalue *texcoord;
   ir_variable *rot_mat_0_var, *rot_mat_1_var;
   ir_dereference_variable *rot_mat_0, *rot_mat_1;

   rot_mat_0_var = p->shader->symbols->get_variable("gl_BumpRotMatrix0MESA");
   rot_mat_1_var = p->shader->symbols->get_variable("gl_BumpRotMatrix1MESA");
   rot_mat_0 = new(p->mem_ctx) ir_dereference_variable(rot_mat_0_var);
   rot_mat_1 = new(p->mem_ctx) ir_dereference_variable(rot_mat_1_var);

   ir_variable *tc_array = p->shader->symbols->get_variable("gl_TexCoord");
   assert(tc_array);
   texcoord = new(p->mem_ctx) ir_dereference_variable(tc_array);
   ir_rvalue *index = new(p->mem_ctx) ir_constant(bumpedUnitNr);
   texcoord = new(p->mem_ctx) ir_dereference_array(texcoord, index);
   tc_array->max_array_access = MAX2(tc_array->max_array_access, unit);

   load_texenv_source( p, unit + SRC_TEXTURE0, unit );

   /* Apply rot matrix and add coords to be available in next phase.
    * dest = Arg1 + (Arg0.xx * rotMat0) + (Arg0.yy * rotMat1)
    * note only 2 coords are affected the rest are left unchanged (mul by 0)
    */
   ir_dereference *deref;
   ir_assignment *assign;
   ir_rvalue *bump_x, *bump_y;

   texcoord = smear(p, texcoord);

   /* bump_texcoord = texcoord */
   ir_variable *bumped = new(p->mem_ctx) ir_variable(texcoord->type,
						     "bump_texcoord",
						     ir_var_temporary);
   p->instructions->push_tail(bumped);

   deref = new(p->mem_ctx) ir_dereference_variable(bumped);
   assign = new(p->mem_ctx) ir_assignment(deref, texcoord);
   p->instructions->push_tail(assign);

   /* bump_texcoord.xy += arg0.x * rotmat0 + arg0.y * rotmat1 */
   bump = get_source(p, key->unit[unit].OptRGB[0].Source, unit);
   bump_x = new(p->mem_ctx) ir_swizzle(bump, 0, 0, 0, 0, 1);
   bump = bump->clone(p->mem_ctx, NULL);
   bump_y = new(p->mem_ctx) ir_swizzle(bump, 1, 0, 0, 0, 1);

   bump_x = new(p->mem_ctx) ir_expression(ir_binop_mul, bump_x, rot_mat_0);
   bump_y = new(p->mem_ctx) ir_expression(ir_binop_mul, bump_y, rot_mat_1);

   ir_expression *expr;
   expr = new(p->mem_ctx) ir_expression(ir_binop_add, bump_x, bump_y);

   deref = new(p->mem_ctx) ir_dereference_variable(bumped);
   expr = new(p->mem_ctx) ir_expression(ir_binop_add,
					new(p->mem_ctx) ir_swizzle(deref,
								   0, 1, 1, 1,
								   2),
					expr);

   deref = new(p->mem_ctx) ir_dereference_variable(bumped);
   assign = new(p->mem_ctx) ir_assignment(deref, expr, NULL, WRITEMASK_XY);
   p->instructions->push_tail(assign);

   p->texcoord_tex[bumpedUnitNr] = bumped;
}

/**
 * Applies the fog calculations.
 *
 * This is basically like the ARB_fragment_prorgam fog options.  Note
 * that ffvertex_prog.c produces fogcoord for us when
 * GL_FOG_COORDINATE_EXT is set to GL_FRAGMENT_DEPTH_EXT.
 */
static ir_rvalue *
emit_fog_instructions(struct texenv_fragment_program *p,
		      ir_rvalue *fragcolor)
{
   struct state_key *key = p->state;
   ir_rvalue *f, *temp;
   ir_variable *params, *oparams;
   ir_variable *fogcoord;
   ir_assignment *assign;

   /* Temporary storage for the whole fog result.  Fog calculations
    * only affect rgb so we're hanging on to the .a value of fragcolor
    * this way.
    */
   ir_variable *fog_result = new(p->mem_ctx) ir_variable(glsl_type::vec4_type,
							 "fog_result",
							 ir_var_auto);
   p->instructions->push_tail(fog_result);
   temp = new(p->mem_ctx) ir_dereference_variable(fog_result);
   assign = new(p->mem_ctx) ir_assignment(temp, fragcolor);
   p->instructions->push_tail(assign);

   temp = new(p->mem_ctx) ir_dereference_variable(fog_result);
   fragcolor = new(p->mem_ctx) ir_swizzle(temp, 0, 1, 2, 3, 3);

   oparams = p->shader->symbols->get_variable("gl_FogParamsOptimizedMESA");
   fogcoord = p->shader->symbols->get_variable("gl_FogFragCoord");
   params = p->shader->symbols->get_variable("gl_Fog");
   f = new(p->mem_ctx) ir_dereference_variable(fogcoord);

   ir_variable *f_var = new(p->mem_ctx) ir_variable(glsl_type::float_type,
						    "fog_factor", ir_var_auto);
   p->instructions->push_tail(f_var);

   switch (key->fog_mode) {
   case FOG_LINEAR:
      /* f = (end - z) / (end - start)
       *
       * gl_MesaFogParamsOptimized gives us (-1 / (end - start)) and
       * (end / (end - start)) so we can generate a single MAD.
       */
      temp = new(p->mem_ctx) ir_dereference_variable(oparams);
      temp = new(p->mem_ctx) ir_swizzle(temp, 0, 0, 0, 0, 1);
      f = new(p->mem_ctx) ir_expression(ir_binop_mul, f, temp);

      temp = new(p->mem_ctx) ir_dereference_variable(oparams);
      temp = new(p->mem_ctx) ir_swizzle(temp, 1, 0, 0, 0, 1);
      f = new(p->mem_ctx) ir_expression(ir_binop_add, f, temp);
      break;
   case FOG_EXP:
      /* f = e^(-(density * fogcoord))
       *
       * gl_MesaFogParamsOptimized gives us density/ln(2) so we can
       * use EXP2 which is generally the native instruction without
       * having to do any further math on the fog density uniform.
       */
      temp = new(p->mem_ctx) ir_dereference_variable(oparams);
      temp = new(p->mem_ctx) ir_swizzle(temp, 2, 0, 0, 0, 1);
      f = new(p->mem_ctx) ir_expression(ir_binop_mul, f, temp);
      f = new(p->mem_ctx) ir_expression(ir_unop_neg, f);
      f = new(p->mem_ctx) ir_expression(ir_unop_exp2, f);
      break;
   case FOG_EXP2:
      /* f = e^(-(density * fogcoord)^2)
       *
       * gl_MesaFogParamsOptimized gives us density/sqrt(ln(2)) so we
       * can do this like FOG_EXP but with a squaring after the
       * multiply by density.
       */
      ir_variable *temp_var = new(p->mem_ctx) ir_variable(glsl_type::float_type,
							  "fog_temp",
							  ir_var_auto);
      p->instructions->push_tail(temp_var);

      temp = new(p->mem_ctx) ir_dereference_variable(oparams);
      temp = new(p->mem_ctx) ir_swizzle(temp, 3, 0, 0, 0, 1);
      f = new(p->mem_ctx) ir_expression(ir_binop_mul,
					f, temp);

      temp = new(p->mem_ctx) ir_dereference_variable(temp_var);
      ir_assignment *assign = new(p->mem_ctx) ir_assignment(temp, f);
      p->instructions->push_tail(assign);

      f = new(p->mem_ctx) ir_dereference_variable(temp_var);
      temp = new(p->mem_ctx) ir_dereference_variable(temp_var);
      f = new(p->mem_ctx) ir_expression(ir_binop_mul, f, temp);
      f = new(p->mem_ctx) ir_expression(ir_unop_neg, f);
      f = new(p->mem_ctx) ir_expression(ir_unop_exp2, f);
      break;
   }

   f = saturate(p, f);

   temp = new(p->mem_ctx) ir_dereference_variable(f_var);
   assign = new(p->mem_ctx) ir_assignment(temp, f);
   p->instructions->push_tail(assign);

   f = new(p->mem_ctx) ir_dereference_variable(f_var);
   f = new(p->mem_ctx) ir_expression(ir_binop_sub,
				     new(p->mem_ctx) ir_constant(1.0f),
				     f);
   temp = new(p->mem_ctx) ir_dereference_variable(params);
   temp = new(p->mem_ctx) ir_dereference_record(temp, "color");
   temp = new(p->mem_ctx) ir_swizzle(temp, 0, 1, 2, 3, 3);
   temp = new(p->mem_ctx) ir_expression(ir_binop_mul, temp, f);

   f = new(p->mem_ctx) ir_dereference_variable(f_var);
   f = new(p->mem_ctx) ir_expression(ir_binop_mul, fragcolor, f);
   f = new(p->mem_ctx) ir_expression(ir_binop_add, temp, f);

   ir_dereference *deref = new(p->mem_ctx) ir_dereference_variable(fog_result);
   assign = new(p->mem_ctx) ir_assignment(deref, f, NULL, WRITEMASK_XYZ);
   p->instructions->push_tail(assign);

   return new(p->mem_ctx) ir_dereference_variable(fog_result);
}

static void
emit_instructions(struct texenv_fragment_program *p)
{
   struct state_key *key = p->state;
   GLuint unit;

   if (key->enabled_units) {
      /* Zeroth pass - bump map textures first */
      for (unit = 0; unit < key->nr_enabled_units; unit++) {
	 if (key->unit[unit].enabled &&
             key->unit[unit].ModeRGB == MODE_BUMP_ENVMAP_ATI) {
	    load_texunit_bumpmap(p, unit);
	 }
      }

      /* First pass - to support texture_env_crossbar, first identify
       * all referenced texture sources and emit texld instructions
       * for each:
       */
      for (unit = 0; unit < key->nr_enabled_units; unit++)
	 if (key->unit[unit].enabled) {
	    load_texunit_sources(p, unit);
	    p->last_tex_stage = unit;
	 }

      /* Second pass - emit combine instructions to build final color:
       */
      for (unit = 0; unit < key->nr_enabled_units; unit++) {
	 if (key->unit[unit].enabled) {
	    p->src_previous = emit_texenv(p, unit);
	 }
      }
   }

   ir_rvalue *cf = get_source(p, SRC_PREVIOUS, 0);
   ir_dereference_variable *deref;
   ir_assignment *assign;

   if (key->separate_specular) {
      ir_rvalue *tmp0;
      ir_variable *spec_result = new(p->mem_ctx) ir_variable(glsl_type::vec4_type,
							    "specular_add",
							    ir_var_temporary);

      p->instructions->push_tail(spec_result);

      deref = new(p->mem_ctx) ir_dereference_variable(spec_result);
      assign = new(p->mem_ctx) ir_assignment(deref, cf);
      p->instructions->push_tail(assign);

      deref = new(p->mem_ctx) ir_dereference_variable(spec_result);
      tmp0 = new(p->mem_ctx) ir_swizzle(deref, 0, 1, 2, 3, 3);

      ir_rvalue *secondary;
      if (p->state->inputs_available & FRAG_BIT_COL1) {
	 ir_variable *var =
	    p->shader->symbols->get_variable("gl_SecondaryColor");
	 assert(var);
	 secondary = new(p->mem_ctx) ir_dereference_variable(var);
      } else {
	 secondary = get_current_attrib(p, VERT_ATTRIB_COLOR1);
      }
      secondary = new(p->mem_ctx) ir_swizzle(secondary, 0, 1, 2, 3, 3);

      tmp0 = new(p->mem_ctx) ir_expression(ir_binop_add, tmp0, secondary);

      deref = new(p->mem_ctx) ir_dereference_variable(spec_result);
      assign = new(p->mem_ctx) ir_assignment(deref, tmp0, NULL, WRITEMASK_XYZ);
      p->instructions->push_tail(assign);

      cf = new(p->mem_ctx) ir_dereference_variable(spec_result);
   }

   if (key->fog_enabled) {
      cf = emit_fog_instructions(p, cf);
   }

   ir_variable *frag_color = p->shader->symbols->get_variable("gl_FragColor");
   assert(frag_color);
   deref = new(p->mem_ctx) ir_dereference_variable(frag_color);
   assign = new(p->mem_ctx) ir_assignment(deref, cf);
   p->instructions->push_tail(assign);
}

/**
 * Generate a new fragment program which implements the context's
 * current texture env/combine mode.
 */
static struct gl_shader_program *
create_new_program(struct gl_context *ctx, struct state_key *key)
{
   struct texenv_fragment_program p;
   unsigned int unit;
   _mesa_glsl_parse_state *state;

   memset(&p, 0, sizeof(p));
   p.mem_ctx = ralloc_context(NULL);
   p.shader = ctx->Driver.NewShader(ctx, 0, GL_FRAGMENT_SHADER);
   p.shader->ir = new(p.shader) exec_list;
   state = new(p.shader) _mesa_glsl_parse_state(ctx, GL_FRAGMENT_SHADER,
						p.shader);
   p.shader->symbols = state->symbols;
   p.top_instructions = p.shader->ir;
   p.instructions = p.shader->ir;
   p.state = key;
   p.shader_program = ctx->Driver.NewShaderProgram(ctx, 0);

   /* Tell the linker to ignore the fact that we're building a
    * separate shader, in case we're in a GLES2 context that would
    * normally reject that.  The real problem is that we're building a
    * fixed function program in a GLES2 context at all, but that's a
    * big mess to clean up.
    */
   p.shader_program->InternalSeparateShader = GL_TRUE;

   state->language_version = 130;
   if (ctx->Extensions.OES_EGL_image_external)
      state->OES_EGL_image_external_enable = true;
   _mesa_glsl_initialize_types(state);
   _mesa_glsl_initialize_variables(p.instructions, state);

   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      p.src_texture[unit] = NULL;
      p.texcoord_tex[unit] = NULL;
   }

   p.src_previous = NULL;

   p.last_tex_stage = 0;

   ir_function *main_f = new(p.mem_ctx) ir_function("main");
   p.instructions->push_tail(main_f);
   state->symbols->add_function(main_f);

   ir_function_signature *main_sig =
      new(p.mem_ctx) ir_function_signature(p.shader->symbols->get_type("void"));
   main_sig->is_defined = true;
   main_f->add_signature(main_sig);

   p.instructions = &main_sig->body;
   if (key->num_draw_buffers)
      emit_instructions(&p);

   validate_ir_tree(p.shader->ir);

   while (do_common_optimization(p.shader->ir, false, false, 32))
      ;
   reparent_ir(p.shader->ir, p.shader->ir);

   p.shader->CompileStatus = true;
   p.shader->Version = state->language_version;
   p.shader->num_builtins_to_link = state->num_builtins_to_link;
   p.shader_program->Shaders =
      (gl_shader **)malloc(sizeof(*p.shader_program->Shaders));
   p.shader_program->Shaders[0] = p.shader;
   p.shader_program->NumShaders = 1;

   _mesa_glsl_link_shader(ctx, p.shader_program);

   /* Set the sampler uniforms, and relink to get them into the linked
    * program.
    */
   struct gl_shader *const fs =
      p.shader_program->_LinkedShaders[MESA_SHADER_FRAGMENT];
   struct gl_program *const fp = fs->Program;

   _mesa_generate_parameters_list_for_uniforms(p.shader_program, fs,
					       fp->Parameters);

   _mesa_associate_uniform_storage(ctx, p.shader_program, fp->Parameters);

   for (unsigned int i = 0; i < MAX_TEXTURE_UNITS; i++) {
      /* Enough space for 'sampler_999\0'.
       */
      char name[12];

      snprintf(name, sizeof(name), "sampler_%d", i);

      int loc = _mesa_get_uniform_location(ctx, p.shader_program, name);
      if (loc != -1) {
	 unsigned base;
	 unsigned idx;

	 /* Avoid using _mesa_uniform() because it flags state
	  * updates, so if we're generating this shader_program in a
	  * state update, we end up recursing.  Instead, just set the
	  * value, which is picked up at re-link.
	  */
	 _mesa_uniform_split_location_offset(loc, &base, &idx);
	 assert(idx == 0);

	 struct gl_uniform_storage *const storage =
	    &p.shader_program->UniformStorage[base];

	 /* Update the storage, the SamplerUnits in the shader program, and
	  * the SamplerUnits in the assembly shader.
	  */
	 storage->storage[idx].i = i;
	 fp->SamplerUnits[storage->sampler] = i;
	 p.shader_program->SamplerUnits[storage->sampler] = i;
	 _mesa_propagate_uniforms_to_driver_storage(storage, 0, 1);
      }
   }
   _mesa_update_shader_textures_used(p.shader_program, fp);
   (void) ctx->Driver.ProgramStringNotify(ctx, fp->Target, fp);

   if (!p.shader_program->LinkStatus)
      _mesa_problem(ctx, "Failed to link fixed function fragment shader: %s\n",
		    p.shader_program->InfoLog);

   ralloc_free(p.mem_ctx);
   return p.shader_program;
}

extern "C" {

/**
 * Return a fragment program which implements the current
 * fixed-function texture, fog and color-sum operations.
 */
struct gl_shader_program *
_mesa_get_fixed_func_fragment_program(struct gl_context *ctx)
{
   struct gl_shader_program *shader_program;
   struct state_key key;
   GLuint keySize;

   keySize = make_state_key(ctx, &key);

   shader_program = (struct gl_shader_program *)
      _mesa_search_program_cache(ctx->FragmentProgram.Cache,
                                 &key, keySize);

   if (!shader_program) {
      shader_program = create_new_program(ctx, &key);

      _mesa_shader_cache_insert(ctx, ctx->FragmentProgram.Cache,
				&key, keySize, shader_program);
   }

   return shader_program;
}

}
