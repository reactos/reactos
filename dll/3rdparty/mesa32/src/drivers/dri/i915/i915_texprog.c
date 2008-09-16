/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <strings.h>

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "tnl/t_context.h"
#include "intel_batchbuffer.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_program.h"

static GLuint translate_tex_src_bit( struct i915_fragment_program *p,
				     GLubyte bit )
{
   switch (bit) {
   case TEXTURE_1D_BIT:   return D0_SAMPLE_TYPE_2D;
   case TEXTURE_2D_BIT:   return D0_SAMPLE_TYPE_2D;
   case TEXTURE_RECT_BIT: return D0_SAMPLE_TYPE_2D;
   case TEXTURE_3D_BIT:   return D0_SAMPLE_TYPE_VOLUME;
   case TEXTURE_CUBE_BIT: return D0_SAMPLE_TYPE_CUBE;
   default: i915_program_error(p, "TexSrcBit"); return 0;
   }
}

static GLuint get_source( struct i915_fragment_program *p, 
			  GLenum src, GLuint unit )
{
   switch (src) {
   case GL_TEXTURE: 
      if (p->src_texture == UREG_BAD) {

	 /* TODO: Use D0_CHANNEL_XY where possible.
	  */
	 GLuint dim = translate_tex_src_bit( p, p->ctx->Texture.Unit[unit]._ReallyEnabled);
	 GLuint sampler = i915_emit_decl(p, REG_TYPE_S, unit, dim);
	 GLuint texcoord = i915_emit_decl(p, REG_TYPE_T, unit, D0_CHANNEL_ALL);
	 GLuint tmp = i915_get_temp( p );
	 GLuint op = T0_TEXLD;

	 if (p->VB->TexCoordPtr[unit]->size == 4)
	    op = T0_TEXLDP;

	 p->src_texture = i915_emit_texld( p, tmp, A0_DEST_CHANNEL_ALL, 
					  sampler, texcoord, op );
      }

      return p->src_texture;

      /* Crossbar: */
   case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7: {
      return UREG_BAD;
   }

   case GL_CONSTANT:
      return i915_emit_const4fv( p, p->ctx->Texture.Unit[unit].EnvColor );
   case GL_PRIMARY_COLOR:
      return i915_emit_decl(p, REG_TYPE_T, T_DIFFUSE, D0_CHANNEL_ALL);
   case GL_PREVIOUS:
   default: 
      i915_emit_decl(p, 
		GET_UREG_TYPE(p->src_previous),
		GET_UREG_NR(p->src_previous), D0_CHANNEL_ALL); 
      return p->src_previous;
   }
}
			

static GLuint emit_combine_source( struct i915_fragment_program *p, 
				   GLuint mask,
				   GLuint unit,
				   GLenum source, 
				   GLenum operand )
{
   GLuint arg, src;

   src = get_source(p, source, unit);

   switch (operand) {
   case GL_ONE_MINUS_SRC_COLOR: 
      /* Get unused tmp,
       * Emit tmp = 1.0 + arg.-x-y-z-w
       */
      arg = i915_get_temp( p );
      return i915_emit_arith( p, A0_ADD, arg, mask, 0,
		  swizzle(src, ONE, ONE, ONE, ONE ),
		  negate(src, 1,1,1,1), 0);

   case GL_SRC_ALPHA: 
      if (mask == A0_DEST_CHANNEL_W)
	 return src;
      else
	 return swizzle( src, W, W, W, W );
   case GL_ONE_MINUS_SRC_ALPHA: 
      /* Get unused tmp,
       * Emit tmp = 1.0 + arg.-w-w-w-w
       */
      arg = i915_get_temp( p );
      return i915_emit_arith( p, A0_ADD, arg, mask, 0,
			 swizzle(src, ONE, ONE, ONE, ONE ),
			 negate( swizzle(src,W,W,W,W), 1,1,1,1), 0);
   case GL_SRC_COLOR: 
   default:
      return src;
   }
}



static int nr_args( GLenum mode )
{
   switch (mode) {
   case GL_REPLACE: return 1; 
   case GL_MODULATE: return 2;
   case GL_ADD: return 2;
   case GL_ADD_SIGNED: return 2;
   case GL_INTERPOLATE:	return 3;
   case GL_SUBTRACT: return 2;
   case GL_DOT3_RGB_EXT: return 2;
   case GL_DOT3_RGBA_EXT: return 2;
   case GL_DOT3_RGB: return 2;
   case GL_DOT3_RGBA: return 2;
   default: return 0;
   }
}


static GLboolean args_match( struct gl_texture_unit *texUnit )
{
   int i, nr = nr_args(texUnit->Combine.ModeRGB);

   for (i = 0 ; i < nr ; i++) {
      if (texUnit->Combine.SourceA[i] != texUnit->Combine.SourceRGB[i]) 
	 return GL_FALSE;

      switch(texUnit->Combine.OperandA[i]) {
      case GL_SRC_ALPHA: 
	 switch(texUnit->Combine.OperandRGB[i]) {
	 case GL_SRC_COLOR: 
	 case GL_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      case GL_ONE_MINUS_SRC_ALPHA: 
	 switch(texUnit->Combine.OperandRGB[i]) {
	 case GL_ONE_MINUS_SRC_COLOR: 
	 case GL_ONE_MINUS_SRC_ALPHA: 
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


static GLuint emit_combine( struct i915_fragment_program *p,
			    GLuint dest,
			    GLuint mask,
			    GLuint saturate,
			    GLuint unit,
			    GLenum mode,
			    const GLenum *source,
			    const GLenum *operand)
{
   int tmp, src[3], nr = nr_args(mode);
   int i;

   for (i = 0; i < nr; i++)
      src[i] = emit_combine_source( p, mask, unit, source[i], operand[i] );

   switch (mode) {
   case GL_REPLACE: 
      if (mask == A0_DEST_CHANNEL_ALL && !saturate)
	 return src[0];
      else
	 return i915_emit_arith( p, A0_MOV, dest, mask, saturate, src[0], 0, 0 );
   case GL_MODULATE: 
      return i915_emit_arith( p, A0_MUL, dest, mask, saturate,
			     src[0], src[1], 0 );
   case GL_ADD: 
      return i915_emit_arith( p, A0_ADD, dest, mask, saturate, 
			     src[0], src[1], 0 );
   case GL_ADD_SIGNED:
      /* tmp = arg0 + arg1
       * result = tmp + -.5
       */
      tmp = i915_emit_const1f(p, .5);
      tmp = negate(swizzle(tmp,X,X,X,X),1,1,1,1);
      i915_emit_arith( p, A0_ADD, dest, mask, 0, src[0], src[1], 0 );
      i915_emit_arith( p, A0_ADD, dest, mask, saturate, dest, tmp, 0 );
      return dest;
   case GL_INTERPOLATE:		/* TWO INSTRUCTIONS */
      /* Arg0 * (Arg2) + Arg1 * (1-Arg2)
       *
       * Arg0*Arg2 + Arg1 - Arg1Arg2 
       *
       * tmp = Arg0*Arg2 + Arg1, 
       * result = (-Arg1)Arg2 + tmp 
       */
      tmp = i915_get_temp( p );
      i915_emit_arith( p, A0_MAD, tmp, mask, 0, src[0], src[2], src[1] );
      i915_emit_arith( p, A0_MAD, dest, mask, saturate, 
		      negate(src[1], 1,1,1,1), src[2], tmp );
      return dest;
   case GL_SUBTRACT: 
      /* negate src[1] */
      return i915_emit_arith( p, A0_ADD, dest, mask, saturate, src[0],
			 negate(src[1],1,1,1,1), 0 );

   case GL_DOT3_RGBA:
   case GL_DOT3_RGBA_EXT: 
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGB: {
      GLuint tmp0 = i915_get_temp( p );
      GLuint tmp1 = i915_get_temp( p );
      GLuint neg1 = negate(swizzle(i915_emit_const1f(p, 1),X,X,X,X), 1,1,1,1);
      GLuint two = swizzle(i915_emit_const1f(p, 2),X,X,X,X);
      i915_emit_arith( p, A0_MAD, tmp0, A0_DEST_CHANNEL_ALL, 0, 
		      two, src[0], neg1);
      if (src[0] == src[1])
	 tmp1 = tmp0;
      else
	 i915_emit_arith( p, A0_MAD, tmp1, A0_DEST_CHANNEL_ALL, 0, 
			 two, src[1], neg1);
      i915_emit_arith( p, A0_DP3, dest, mask, saturate, tmp0, tmp1, 0);
      return dest;
   }

   default: 
      return src[0];
   }
}

static GLuint get_dest( struct i915_fragment_program *p, int unit )
{
   if (p->ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      return i915_get_temp( p );
   else if (unit != p->last_tex_stage)
      return i915_get_temp( p );
   else
      return UREG(REG_TYPE_OC, 0);
}
      


static GLuint emit_texenv( struct i915_fragment_program *p, int unit )
{
   struct gl_texture_unit *texUnit = &p->ctx->Texture.Unit[unit];
   GLenum envMode = texUnit->EnvMode;
   struct gl_texture_object *tObj = texUnit->_Current;
   GLenum format = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;
   GLuint saturate = unit < p->last_tex_stage ? A0_DEST_SATURATE : 0;

   switch(envMode) {
   case GL_BLEND: {
      const int cf = get_source(p, GL_PREVIOUS, unit);
      const int cc = get_source(p, GL_CONSTANT, unit);
      const int cs = get_source(p, GL_TEXTURE, unit);
      const int out = get_dest(p, unit);

      if (format == GL_INTENSITY) {
	 /* cv = cf(1 - cs) + cc.cs
	  * cv = cf - cf.cs + cc.cs
	  */
	 /* u[2] = MAD( -cf * cs + cf )
	  * cv   = MAD( cc * cs + u[2] )
	  */
	 
	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, 0, 
			 negate(cf,1,1,1,1), cs, cf );

	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, saturate, 
			 cc, cs, out );

	 return out;
      } else {
	 /* cv = cf(1 - cs) + cc.cs
	  * cv = cf - cf.cs + cc.cs
	  * av =      af.as
	  */
	 /* u[2] = MAD( cf.-x-y-zw * cs.xyzw + cf.xyz0 )
	  * oC   = MAD( cc.xyz0 * cs.xyz0 + u[2].xyzw )
	  */
	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, 0,
			 negate(cf,1,1,1,0),  
			 cs,
			 swizzle(cf,X,Y,Z,ZERO) );


	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, saturate,
			 swizzle(cc,X,Y,Z,ZERO),  
			 swizzle(cs,X,Y,Z,ZERO),
			 out );

	 return out;
      }
   }

   case GL_DECAL: {
      if (format == GL_RGB ||
	  format == GL_RGBA) {
	 int cf = get_source( p, GL_PREVIOUS, unit );
	 int cs = get_source( p, GL_TEXTURE, unit );
	 int out = get_dest(p, unit);
	 
	 /* cv = cf(1-as) + cs.as
	  * cv = cf.(-as) + cf + cs.as
	  * av = af
	  */ 
	 
	 /* u[2] = mad( cf.xyzw * cs.-w-w-w1 + cf.xyz0 )
	  * oc = mad( cs.xyz0 * cs.www0 + u[2].xyzw )
	  */
	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, 0,
			 cf,  
			 negate(swizzle(cs,W,W,W,ONE),1,1,1,0),
			 swizzle(cf,X,Y,Z,ZERO) );
	 
	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, saturate,
			 swizzle(cs,X,Y,Z,ZERO),  
			 swizzle(cs,W,W,W,ZERO),
			 out );
	 return out;
      }
      else {
	 return get_source( p, GL_PREVIOUS, unit );
      }
   }

   case GL_REPLACE: {
      const int cs = get_source( p, GL_TEXTURE, unit );	/* saturated */
      switch (format) {
      case GL_ALPHA: {
	 const int cf = get_source( p, GL_PREVIOUS, unit ); /* saturated */
	 i915_emit_arith( p, A0_MOV, cs, A0_DEST_CHANNEL_XYZ, 0, cf, 0, 0 );
	 return cs;
      }
      case GL_RGB:
      case GL_LUMINANCE: {
	 const int cf = get_source( p, GL_PREVIOUS, unit ); /* saturated */
	 i915_emit_arith( p, A0_MOV, cs, A0_DEST_CHANNEL_W, 0, cf, 0, 0 );
	 return cs;
      }
      default:
	 return cs;
      }
   }

   case GL_MODULATE: {
      const int cf = get_source( p, GL_PREVIOUS, unit );
      const int cs = get_source( p, GL_TEXTURE, unit );
      const int out = get_dest(p, unit);
      switch (format) {
      case GL_ALPHA: 
	 i915_emit_arith( p, A0_MUL, out, A0_DEST_CHANNEL_ALL, saturate,
			 swizzle(cs, ONE, ONE, ONE, W), cf, 0 );
	 break;
      default:
	 i915_emit_arith( p, A0_MUL, out, A0_DEST_CHANNEL_ALL, saturate, 
			 cs, cf, 0 );
	 break;
      }
      return out;
   }
   case GL_ADD: {
      int cf = get_source( p, GL_PREVIOUS, unit );
      int cs = get_source( p, GL_TEXTURE, unit );
      const int out = get_dest( p, unit );

      if (format == GL_INTENSITY) {
	 /* output-color.rgba = add( incoming, u[1] )
	  */
	 i915_emit_arith( p, A0_ADD, out, A0_DEST_CHANNEL_ALL, saturate, 
			 cs, cf, 0 );
	 return out;
      }
      else {
	 /* cv.xyz = cf.xyz + cs.xyz
	  * cv.w   = cf.w * cs.w
	  *
	  * cv.xyzw = MAD( cf.111w * cs.xyzw + cf.xyz0 )
	  */
 	 i915_emit_arith( p, A0_MAD, out, A0_DEST_CHANNEL_ALL, saturate,
			 swizzle(cf,ONE,ONE,ONE,W), 
			 cs,  
			 swizzle(cf,X,Y,Z,ZERO) ); 
	 return out;
      }
      break;
   }
   case GL_COMBINE: {
      GLuint rgb_shift, alpha_shift, out, shift;
      GLuint dest = get_dest(p, unit);

      /* The EXT version of the DOT3 extension does not support the
       * scale factor, but the ARB version (and the version in OpenGL
       * 1.3) does.
       */
      switch (texUnit->Combine.ModeRGB) {
      case GL_DOT3_RGB_EXT:
	 alpha_shift = texUnit->Combine.ScaleShiftA;
	 rgb_shift = 0;
	 break;

      case GL_DOT3_RGBA_EXT:
	 alpha_shift = 0;
	 rgb_shift = 0;
	 break;

      default:
	 rgb_shift = texUnit->Combine.ScaleShiftRGB;
	 alpha_shift = texUnit->Combine.ScaleShiftA;
	 break;
      }


      /* Emit the RGB and A combine ops
       */
      if (texUnit->Combine.ModeRGB == texUnit->Combine.ModeA && 
	  args_match( texUnit )) {
	 out = emit_combine( p, dest, A0_DEST_CHANNEL_ALL, saturate,
			     unit,
			     texUnit->Combine.ModeRGB,
			     texUnit->Combine.SourceRGB,
			     texUnit->Combine.OperandRGB );
      }
      else if (texUnit->Combine.ModeRGB == GL_DOT3_RGBA_EXT ||
	       texUnit->Combine.ModeRGB == GL_DOT3_RGBA) {

	 out = emit_combine( p, dest, A0_DEST_CHANNEL_ALL, saturate,
			     unit,
			     texUnit->Combine.ModeRGB,
			     texUnit->Combine.SourceRGB,
			     texUnit->Combine.OperandRGB );
      }
      else {
	 /* Need to do something to stop from re-emitting identical
	  * argument calculations here:
	  */
	 out = emit_combine( p, dest, A0_DEST_CHANNEL_XYZ, saturate,
			     unit,
			     texUnit->Combine.ModeRGB,
			     texUnit->Combine.SourceRGB,
			     texUnit->Combine.OperandRGB );
	 out = emit_combine( p, dest, A0_DEST_CHANNEL_W, saturate,
			     unit,
			     texUnit->Combine.ModeA,
			     texUnit->Combine.SourceA,
			     texUnit->Combine.OperandA );
      }

      /* Deal with the final shift:
       */
      if (alpha_shift || rgb_shift) {
	 if (rgb_shift == alpha_shift) {
	    shift = i915_emit_const1f(p, 1<<rgb_shift);
	    shift = swizzle(shift,X,X,X,X);
	 }
	 else {
	    shift = i915_emit_const2f(p, 1<<rgb_shift, 1<<alpha_shift);
	    shift = swizzle(shift,X,X,X,Y);
	 }
	 return i915_emit_arith( p, A0_MUL, dest, A0_DEST_CHANNEL_ALL, 
				saturate, out, shift, 0 );
      }

      return out;
   }

   default:
      return get_source(p, GL_PREVIOUS, 0);
   }
}

static void emit_program_fini( struct i915_fragment_program *p )
{
   int cf = get_source( p, GL_PREVIOUS, 0 );
   int out = UREG( REG_TYPE_OC, 0 );

   if (p->ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
      /* Emit specular add.
       */
      GLuint s = i915_emit_decl(p, REG_TYPE_T, T_SPECULAR, D0_CHANNEL_ALL);
      i915_emit_arith( p, A0_ADD, out, A0_DEST_CHANNEL_ALL, 0, cf, 
		  swizzle(s, X,Y,Z,ZERO), 0 );
   }
   else if (cf != out) {
      /* Will wind up in here if no texture enabled or a couple of
       * other scenarios (GL_REPLACE for instance).
       */
      i915_emit_arith( p, A0_MOV, out, A0_DEST_CHANNEL_ALL, 0, cf, 0, 0 );
   }
}


static void i915EmitTextureProgram( i915ContextPtr i915 )
{
   GLcontext *ctx = &i915->intel.ctx;
   struct i915_fragment_program *p = &i915->tex_program;
   GLuint unit;

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   i915_init_program( i915, p );

   if (ctx->Texture._EnabledUnits) {
      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits ; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	    p->last_tex_stage = unit;
	 }

      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	    p->src_previous = emit_texenv( p, unit );
	    p->src_texture = UREG_BAD;
	    p->temp_flag = 0xffff000;
	    p->temp_flag |= 1 << GET_UREG_NR(p->src_previous);
	 }
   }

   emit_program_fini( p );

   i915_fini_program( p );
   i915_upload_program( i915, p );

   p->translated = 1;
}


void i915ValidateTextureProgram( i915ContextPtr i915 )
{
   intelContextPtr intel = &i915->intel;
   GLcontext *ctx = &intel->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   int i, offset;
   GLuint s4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_VFMT_MASK;
   GLuint s2 = S2_TEXCOORD_NONE;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   intel->vertex_attr_count = 0;
   intel->coloroffset = 0;
   intel->specoffset = 0;
   offset = 0;

   if (i915->current_program) {
      i915->current_program->on_hardware = 0;
      i915->current_program->params_uptodate = 0;
   }

   if (i915->vertex_fog == I915_FOG_PIXEL) {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, S4_VFMT_XYZW, 16 );
      RENDERINPUTS_CLEAR( index_bitset, _TNL_ATTRIB_FOG );
   }
   else if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, S4_VFMT_XYZW, 16 );
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, S4_VFMT_XYZ, 12 );
   }

   /* How undefined is undefined? */
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_POINTSIZE )) {
      EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F, S4_VFMT_POINT_WIDTH, 4 );
   }
      
   intel->coloroffset = offset / 4;
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, S4_VFMT_COLOR, 4 );
            
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 intel->specoffset = offset / 4;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, S4_VFMT_SPEC_FOG, 3 );
      } else 
	 EMIT_PAD( 3 );
      
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG ))
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, S4_VFMT_SPEC_FOG, 1 );
      else
	 EMIT_PAD( 1 );
   }

   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      for (i = 0; i < 8; i++) {
	 if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX(i) )) {
	    int sz = VB->TexCoordPtr[i]->size;
	    
	    s2 &= ~S2_TEXCOORD_FMT(i, S2_TEXCOORD_FMT0_MASK);
	    s2 |= S2_TEXCOORD_FMT(i, SZ_TO_HW(sz));

	    EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_SZ(sz), 0, sz * 4 );
	 }
      }
   }

   /* Only need to change the vertex emit code if there has been a
    * statechange to a new hardware vertex format:
    */
   if (s2 != i915->state.Ctx[I915_CTXREG_LIS2] ||
       s4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
    
      I915_STATECHANGE( i915, I915_UPLOAD_CTX );

      i915->tex_program.translated = 0;

      /* Must do this *after* statechange, so as not to affect
       * buffered vertices reliant on the old state:
       */
      intel->vertex_size = _tnl_install_attrs( ctx, 
					       intel->vertex_attrs, 
					       intel->vertex_attr_count,
					       intel->ViewportMatrix.m, 0 ); 

      intel->vertex_size >>= 2;

      i915->state.Ctx[I915_CTXREG_LIS2] = s2;
      i915->state.Ctx[I915_CTXREG_LIS4] = s4;

      assert(intel->vtbl.check_vertex_size( intel, intel->vertex_size ));
   }

   if (!i915->tex_program.translated ||
       i915->last_ReallyEnabled != ctx->Texture._EnabledUnits) {
      i915EmitTextureProgram( i915 );      
      i915->last_ReallyEnabled = ctx->Texture._EnabledUnits;
   }
}
