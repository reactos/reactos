/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "colormac.h"
#include "colortab.h"
#include "context.h"
#include "enums.h"
#include "extensions.h"
#include "macros.h"
#include "nvfragprog.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "mtypes.h"
#include "math/m_xform.h"
#include "math/m_matrix.h"



#ifdef SPECIALCAST
/* Needed for an Amiga compiler */
#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))
#else
/* all other compilers */
#define ENUM_TO_FLOAT(X) ((GLfloat)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(X))
#endif



void
_mesa_copy_texture_state( const GLcontext *src, GLcontext *dst )
{
   GLuint i;

   ASSERT(src);
   ASSERT(dst);

   dst->Texture.CurrentUnit = src->Texture.CurrentUnit;
   dst->Texture._GenFlags = src->Texture._GenFlags;
   dst->Texture._TexGenEnabled = src->Texture._TexGenEnabled;
   dst->Texture._TexMatEnabled = src->Texture._TexMatEnabled;
   dst->Texture.SharedPalette = src->Texture.SharedPalette;

   /* per-unit state */
   for (i = 0; i < src->Const.MaxTextureUnits; i++) {
      dst->Texture.Unit[i].Enabled = src->Texture.Unit[i].Enabled;
      dst->Texture.Unit[i].EnvMode = src->Texture.Unit[i].EnvMode;
      COPY_4V(dst->Texture.Unit[i].EnvColor, src->Texture.Unit[i].EnvColor);
      dst->Texture.Unit[i].TexGenEnabled = src->Texture.Unit[i].TexGenEnabled;
      dst->Texture.Unit[i].GenModeS = src->Texture.Unit[i].GenModeS;
      dst->Texture.Unit[i].GenModeT = src->Texture.Unit[i].GenModeT;
      dst->Texture.Unit[i].GenModeR = src->Texture.Unit[i].GenModeR;
      dst->Texture.Unit[i].GenModeQ = src->Texture.Unit[i].GenModeQ;
      dst->Texture.Unit[i]._GenBitS = src->Texture.Unit[i]._GenBitS;
      dst->Texture.Unit[i]._GenBitT = src->Texture.Unit[i]._GenBitT;
      dst->Texture.Unit[i]._GenBitR = src->Texture.Unit[i]._GenBitR;
      dst->Texture.Unit[i]._GenBitQ = src->Texture.Unit[i]._GenBitQ;
      dst->Texture.Unit[i]._GenFlags = src->Texture.Unit[i]._GenFlags;
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneS, src->Texture.Unit[i].ObjectPlaneS);
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneT, src->Texture.Unit[i].ObjectPlaneT);
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneR, src->Texture.Unit[i].ObjectPlaneR);
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneQ, src->Texture.Unit[i].ObjectPlaneQ);
      COPY_4V(dst->Texture.Unit[i].EyePlaneS, src->Texture.Unit[i].EyePlaneS);
      COPY_4V(dst->Texture.Unit[i].EyePlaneT, src->Texture.Unit[i].EyePlaneT);
      COPY_4V(dst->Texture.Unit[i].EyePlaneR, src->Texture.Unit[i].EyePlaneR);
      COPY_4V(dst->Texture.Unit[i].EyePlaneQ, src->Texture.Unit[i].EyePlaneQ);
      dst->Texture.Unit[i].LodBias = src->Texture.Unit[i].LodBias;

      /* GL_EXT_texture_env_combine */
      dst->Texture.Unit[i].CombineModeRGB = src->Texture.Unit[i].CombineModeRGB;
      dst->Texture.Unit[i].CombineModeA = src->Texture.Unit[i].CombineModeA;
      COPY_3V(dst->Texture.Unit[i].CombineSourceRGB, src->Texture.Unit[i].CombineSourceRGB);
      COPY_3V(dst->Texture.Unit[i].CombineSourceA, src->Texture.Unit[i].CombineSourceA);
      COPY_3V(dst->Texture.Unit[i].CombineOperandRGB, src->Texture.Unit[i].CombineOperandRGB);
      COPY_3V(dst->Texture.Unit[i].CombineOperandA, src->Texture.Unit[i].CombineOperandA);
      dst->Texture.Unit[i].CombineScaleShiftRGB = src->Texture.Unit[i].CombineScaleShiftRGB;
      dst->Texture.Unit[i].CombineScaleShiftA = src->Texture.Unit[i].CombineScaleShiftA;

      /* texture object state */
      _mesa_copy_texture_object(dst->Texture.Unit[i].Current1D,
                                src->Texture.Unit[i].Current1D);
      _mesa_copy_texture_object(dst->Texture.Unit[i].Current2D,
                                src->Texture.Unit[i].Current2D);
      _mesa_copy_texture_object(dst->Texture.Unit[i].Current3D,
                                src->Texture.Unit[i].Current3D);
      _mesa_copy_texture_object(dst->Texture.Unit[i].CurrentCubeMap,
                                src->Texture.Unit[i].CurrentCubeMap);
      _mesa_copy_texture_object(dst->Texture.Unit[i].CurrentRect,
                                src->Texture.Unit[i].CurrentRect);
   }
}


/*
 * For debugging
 */
void
_mesa_print_texunit_state( GLcontext *ctx, GLuint unit )
{
   const struct gl_texture_unit *texUnit = ctx->Texture.Unit + unit;
   _mesa_printf("Texture Unit %d\n", unit);
   _mesa_printf("  GL_TEXTURE_ENV_MODE = %s\n", _mesa_lookup_enum_by_nr(texUnit->EnvMode));
   _mesa_printf("  GL_COMBINE_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineModeRGB));
   _mesa_printf("  GL_COMBINE_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineModeA));
   _mesa_printf("  GL_SOURCE0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceRGB[0]));
   _mesa_printf("  GL_SOURCE1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceRGB[1]));
   _mesa_printf("  GL_SOURCE2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceRGB[2]));
   _mesa_printf("  GL_SOURCE0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceA[0]));
   _mesa_printf("  GL_SOURCE1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceA[1]));
   _mesa_printf("  GL_SOURCE2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceA[2]));
   _mesa_printf("  GL_OPERAND0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandRGB[0]));
   _mesa_printf("  GL_OPERAND1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandRGB[1]));
   _mesa_printf("  GL_OPERAND2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandRGB[2]));
   _mesa_printf("  GL_OPERAND0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandA[0]));
   _mesa_printf("  GL_OPERAND1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandA[1]));
   _mesa_printf("  GL_OPERAND2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandA[2]));
   _mesa_printf("  GL_RGB_SCALE = %d\n", 1 << texUnit->CombineScaleShiftRGB);
   _mesa_printf("  GL_ALPHA_SCALE = %d\n", 1 << texUnit->CombineScaleShiftA);
   _mesa_printf("  GL_TEXTURE_ENV_COLOR = (%f, %f, %f, %f)\n", texUnit->EnvColor[0], texUnit->EnvColor[1], texUnit->EnvColor[2], texUnit->EnvColor[3]);
}



/**********************************************************************/
/*                       Texture Environment                          */
/**********************************************************************/


void GLAPIENTRY
_mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

#define TE_ERROR(errCode, msg, value)				\
   _mesa_error(ctx, errCode, msg, _mesa_lookup_enum_by_nr(value));

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         {
            const GLenum mode = (GLenum) (GLint) *param;
            if (mode == GL_MODULATE ||
                mode == GL_BLEND ||
                mode == GL_DECAL ||
                mode == GL_REPLACE ||
                (mode == GL_ADD && ctx->Extensions.EXT_texture_env_add) ||
                (mode == GL_COMBINE &&
                 (ctx->Extensions.EXT_texture_env_combine ||
                  ctx->Extensions.ARB_texture_env_combine))) {
               /* legal */
               if (texUnit->EnvMode == mode)
                  return;
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texUnit->EnvMode = mode;
            }
            else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
               return;
            }
         }
         break;
      case GL_TEXTURE_ENV_COLOR:
         {
            GLfloat tmp[4];
            tmp[0] = CLAMP( param[0], 0.0F, 1.0F );
            tmp[1] = CLAMP( param[1], 0.0F, 1.0F );
            tmp[2] = CLAMP( param[2], 0.0F, 1.0F );
            tmp[3] = CLAMP( param[3], 0.0F, 1.0F );
            if (TEST_EQ_4V(tmp, texUnit->EnvColor))
               return;
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            COPY_4FV(texUnit->EnvColor, tmp);
         }
         break;
      case GL_COMBINE_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum mode = (GLenum) (GLint) *param;
	    switch (mode) {
	    case GL_REPLACE:
	    case GL_MODULATE:
	    case GL_ADD:
	    case GL_ADD_SIGNED:
	    case GL_INTERPOLATE:
               /* OK */
	       break;
            case GL_SUBTRACT:
               if (!ctx->Extensions.ARB_texture_env_combine) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
                  return;
               }
               break;
	    case GL_DOT3_RGB_EXT:
	    case GL_DOT3_RGBA_EXT:
	       if (!ctx->Extensions.EXT_texture_env_dot3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_DOT3_RGB:
	    case GL_DOT3_RGBA:
	       if (!ctx->Extensions.ARB_texture_env_dot3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_MODULATE_ADD_ATI:
	    case GL_MODULATE_SIGNED_ADD_ATI:
	    case GL_MODULATE_SUBTRACT_ATI:
	       if (!ctx->Extensions.ATI_texture_env_combine3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
	    if (texUnit->CombineModeRGB == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineModeRGB = mode;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
         break;
      case GL_COMBINE_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum mode = (GLenum) (GLint) *param;
            switch (mode) {
	    case GL_REPLACE:
	    case GL_MODULATE:
	    case GL_ADD:
	    case GL_ADD_SIGNED:
	    case GL_INTERPOLATE:
	       /* OK */
	       break;
	    case GL_SUBTRACT:
	       if (!ctx->Extensions.ARB_texture_env_combine) {
		  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_MODULATE_ADD_ATI:
	    case GL_MODULATE_SIGNED_ADD_ATI:
	    case GL_MODULATE_SUBTRACT_ATI:
	       if (!ctx->Extensions.ATI_texture_env_combine3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    default:
	       TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }

	    if (texUnit->CombineModeA == mode)
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineModeA = mode;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_SOURCE0_RGB:
      case GL_SOURCE1_RGB:
      case GL_SOURCE2_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
	     ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum source = (GLenum) (GLint) *param;
	    const GLuint s = pname - GL_SOURCE0_RGB;
            if (source == GL_TEXTURE ||
                source == GL_CONSTANT ||
                source == GL_PRIMARY_COLOR ||
                source == GL_PREVIOUS ||
                (ctx->Extensions.ARB_texture_env_crossbar &&
                 source >= GL_TEXTURE0 &&
                 source < GL_TEXTURE0 + ctx->Const.MaxTextureUnits) ||
                (ctx->Extensions.ATI_texture_env_combine3 &&
                 (source == GL_ZERO || source == GL_ONE))) {
               /* legal */
               if (texUnit->CombineSourceRGB[s] == source)
                  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineSourceRGB[s] = source;
            }
            else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", source);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_SOURCE0_ALPHA:
      case GL_SOURCE1_ALPHA:
      case GL_SOURCE2_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum source = (GLenum) (GLint) *param;
	    const GLuint s = pname - GL_SOURCE0_ALPHA;
            if (source == GL_TEXTURE ||
                source == GL_CONSTANT ||
                source == GL_PRIMARY_COLOR ||
                source == GL_PREVIOUS ||
                (ctx->Extensions.ARB_texture_env_crossbar &&
                 source >= GL_TEXTURE0 &&
                 source < GL_TEXTURE0 + ctx->Const.MaxTextureUnits) ||
		(ctx->Extensions.ATI_texture_env_combine3 &&
                 (source == GL_ZERO || source == GL_ONE))) {
               /* legal */
	       if (texUnit->CombineSourceA[s] == source)
                  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineSourceA[s] = source;
            }
            else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", source);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
	     ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    const GLuint s = pname - GL_OPERAND0_RGB;
	    switch (operand) {
	    case GL_SRC_COLOR:
	    case GL_ONE_MINUS_SRC_COLOR:
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       if (texUnit->CombineOperandRGB[s] == operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandRGB[s] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       if (texUnit->CombineOperandA[pname-GL_OPERAND0_ALPHA] ==
		   operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandA[pname-GL_OPERAND0_ALPHA] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND2_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    switch (operand) {
	    case GL_SRC_COLOR:           /* ARB combine only */
	    case GL_ONE_MINUS_SRC_COLOR: /* ARB combine only */
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       if (texUnit->CombineOperandRGB[2] == operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandRGB[2] = operand;
               break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND2_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       if (texUnit->CombineOperandA[2] == operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandA[2] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_RGB_SCALE:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLuint newshift;
	    if (*param == 1.0) {
	       newshift = 0;
	    }
	    else if (*param == 2.0) {
	       newshift = 1;
	    }
	    else if (*param == 4.0) {
	       newshift = 2;
	    }
	    else {
	       _mesa_error( ctx, GL_INVALID_VALUE,
                            "glTexEnv(GL_RGB_SCALE not 1, 2 or 4)" );
	       return;
	    }
	    if (texUnit->CombineScaleShiftRGB == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineScaleShiftRGB = newshift;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_ALPHA_SCALE:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLuint newshift;
	    if (*param == 1.0) {
	       newshift = 0;
	    }
	    else if (*param == 2.0) {
	       newshift = 1;
	    }
	    else if (*param == 4.0) {
	       newshift = 2;
	    }
	    else {
	       _mesa_error( ctx, GL_INVALID_VALUE,
                            "glTexEnv(GL_ALPHA_SCALE not 1, 2 or 4)" );
	       return;
	    }
	    if (texUnit->CombineScaleShiftA == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineScaleShiftA = newshift;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
	 return;
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      /* GL_EXT_texture_lod_bias */
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target=0x%x)", target );
	 return;
      }
      if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
	 if (texUnit->LodBias == param[0])
	    return;
	 FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->LodBias = param[0];
      }
      else {
         TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	 return;
      }
   }
   else if (target == GL_POINT_SPRITE_NV) {
      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      if (!ctx->Extensions.NV_point_sprite
	  && !ctx->Extensions.ARB_point_sprite) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target=0x%x)", target );
	 return;
      }
      if (pname == GL_COORD_REPLACE_NV) {
         const GLenum value = (GLenum) param[0];
         if (value == GL_TRUE || value == GL_FALSE) {
            /* It's kind of weird to set point state via glTexEnv,
             * but that's what the spec calls for.
             */
            const GLboolean state = (GLboolean) value;
            if (ctx->Point.CoordReplace[ctx->Texture.CurrentUnit] == state)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.CoordReplace[ctx->Texture.CurrentUnit] = state;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexEnv(param=0x%x)", value);
            return;
         }
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname=0x%x)", pname );
         return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target=0x%x)",target );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexEnv %s %s %.1f(%s) ...\n",
                  _mesa_lookup_enum_by_nr(target),
                  _mesa_lookup_enum_by_nr(pname),
                  *param,
                  _mesa_lookup_enum_by_nr((GLenum) (GLint) *param));

   /* Tell device driver about the new texture environment */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, target, pname, param );
   }
}


void GLAPIENTRY
_mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexEnvfv( target, pname, &param );
}



void GLAPIENTRY
_mesa_TexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
   _mesa_TexEnvfv( target, pname, p );
}


void GLAPIENTRY
_mesa_TexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT( param[0] );
      p[1] = INT_TO_FLOAT( param[1] );
      p[2] = INT_TO_FLOAT( param[2] );
      p[3] = INT_TO_FLOAT( param[3] );
   }
   else {
      p[0] = (GLfloat) param[0];
      p[1] = p[2] = p[3] = 0;  /* init to zero, just to be safe */
   }
   _mesa_TexEnvfv( target, pname, p );
}


void GLAPIENTRY
_mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
         case GL_TEXTURE_ENV_MODE:
            *params = ENUM_TO_FLOAT(texUnit->EnvMode);
            break;
         case GL_TEXTURE_ENV_COLOR:
            COPY_4FV( params, texUnit->EnvColor );
            break;
         case GL_COMBINE_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineModeRGB;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_COMBINE_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineModeA;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE0_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineSourceRGB[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE1_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineSourceRGB[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineSourceRGB[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE0_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineSourceA[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE1_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineSourceA[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineSourceA[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND0_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineOperandRGB[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND1_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineOperandRGB[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineOperandRGB[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND0_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineOperandA[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND1_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineOperandA[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->CombineOperandA[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_RGB_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->CombineScaleShiftRGB == 0)
                  *params = 1.0;
               else if (texUnit->CombineScaleShiftRGB == 1)
                  *params = 2.0;
               else
                  *params = 4.0;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
               return;
            }
            break;
         case GL_ALPHA_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->CombineScaleShiftA == 0)
                  *params = 1.0;
               else if (texUnit->CombineScaleShiftA == 1)
                  *params = 2.0;
               else
                  *params = 4.0;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
               return;
            }
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname=0x%x)", pname);
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      /* GL_EXT_texture_lod_bias */
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
	 return;
      }
      if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
         *params = texUnit->LodBias;
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
	 return;
      }
   }
   else if (target == GL_POINT_SPRITE_NV) {
      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      if (!ctx->Extensions.NV_point_sprite
	  && !ctx->Extensions.ARB_point_sprite) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
         return;
      }
      if (pname == GL_COORD_REPLACE_NV) {
         *params = (GLfloat) ctx->Point.CoordReplace[ctx->Texture.CurrentUnit];
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
         return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }
}


void GLAPIENTRY
_mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
         case GL_TEXTURE_ENV_MODE:
            *params = (GLint) texUnit->EnvMode;
            break;
         case GL_TEXTURE_ENV_COLOR:
            params[0] = FLOAT_TO_INT( texUnit->EnvColor[0] );
            params[1] = FLOAT_TO_INT( texUnit->EnvColor[1] );
            params[2] = FLOAT_TO_INT( texUnit->EnvColor[2] );
            params[3] = FLOAT_TO_INT( texUnit->EnvColor[3] );
            break;
         case GL_COMBINE_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineModeRGB;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_COMBINE_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineModeA;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE0_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineSourceRGB[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE1_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineSourceRGB[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineSourceRGB[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE0_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineSourceA[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE1_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineSourceA[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineSourceA[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND0_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineOperandRGB[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND1_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineOperandRGB[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineOperandRGB[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND0_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineOperandA[0];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND1_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineOperandA[1];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->CombineOperandA[2];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_RGB_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->CombineScaleShiftRGB == 0)
                  *params = 1;
               else if (texUnit->CombineScaleShiftRGB == 1)
                  *params = 2;
               else
                  *params = 4;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
               return;
            }
            break;
         case GL_ALPHA_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->CombineScaleShiftA == 0)
                  *params = 1;
               else if (texUnit->CombineScaleShiftA == 1)
                  *params = 2;
               else
                  *params = 4;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
               return;
            }
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname=0x%x)",
                        pname);
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      /* GL_EXT_texture_lod_bias */
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
	 return;
      }
      if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
         *params = (GLint) texUnit->LodBias;
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
	 return;
      }
   }
   else if (target == GL_POINT_SPRITE_NV) {
      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      if (!ctx->Extensions.NV_point_sprite
	  && !ctx->Extensions.ARB_point_sprite) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
         return;
      }
      if (pname == GL_COORD_REPLACE_NV) {
         *params = (GLint) ctx->Point.CoordReplace[ctx->Texture.CurrentUnit];
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
         return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
      return;
   }
}




/**********************************************************************/
/*                       Texture Parameters                           */
/**********************************************************************/

static GLboolean 
_mesa_validate_texture_wrap_mode(GLcontext * ctx,
				 GLenum target, GLenum eparam)
{
   const struct gl_extensions * const e = & ctx->Extensions;

   if (eparam == GL_CLAMP || eparam == GL_CLAMP_TO_EDGE ||
       (eparam == GL_CLAMP_TO_BORDER && e->ARB_texture_border_clamp)) {
      /* any texture target */
      return GL_TRUE;
   }
   else if (target != GL_TEXTURE_RECTANGLE_NV &&
	    (eparam == GL_REPEAT ||
	     (eparam == GL_MIRRORED_REPEAT &&
	      e->ARB_texture_mirrored_repeat) ||
	     (eparam == GL_MIRROR_CLAMP_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (eparam == GL_MIRROR_CLAMP_TO_EDGE_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (eparam == GL_MIRROR_CLAMP_TO_BORDER_EXT &&
	      (e->EXT_texture_mirror_clamp)))) {
      /* non-rectangle texture */
      return GL_TRUE;
   }

   _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
   return GL_FALSE;
}


void GLAPIENTRY
_mesa_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexParameterfv(target, pname, &param);
}


void GLAPIENTRY
_mesa_TexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   GLenum eparam = (GLenum) (GLint) params[0];
   struct gl_texture_object *texObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexParameter %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(target),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr(eparam));


   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->CurrentCubeMap;
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->CurrentRect;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         /* A small optimization */
         if (texObj->MinFilter == eparam)
            return;
         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else if ((eparam==GL_NEAREST_MIPMAP_NEAREST ||
                   eparam==GL_LINEAR_MIPMAP_NEAREST ||
                   eparam==GL_NEAREST_MIPMAP_LINEAR ||
                   eparam==GL_LINEAR_MIPMAP_LINEAR) &&
                  texObj->Target != GL_TEXTURE_RECTANGLE_NV) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         /* A small optimization */
         if (texObj->MagFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MagFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (texObj->WrapS == eparam)
            return;
         if (_mesa_validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapS = eparam;
         }
         else {
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (texObj->WrapT == eparam)
            return;
         if (_mesa_validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapT = eparam;
         }
         else {
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R:
         if (texObj->WrapR == eparam)
            return;
         if (_mesa_validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapR = eparam;
         }
         else {
	    return;
         }
         break;
      case GL_TEXTURE_BORDER_COLOR:
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BorderColor[RCOMP] = params[0];
         texObj->BorderColor[GCOMP] = params[1];
         texObj->BorderColor[BCOMP] = params[2];
         texObj->BorderColor[ACOMP] = params[3];
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[RCOMP], params[0]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[GCOMP], params[1]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[BCOMP], params[2]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[ACOMP], params[3]);
         break;
      case GL_TEXTURE_MIN_LOD:
         if (texObj->MinLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MinLod = params[0];
         break;
      case GL_TEXTURE_MAX_LOD:
         if (texObj->MaxLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLod = params[0];
         break;
      case GL_TEXTURE_BASE_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         if (target == GL_TEXTURE_RECTANGLE_NV && params[0] != 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BaseLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_MAX_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_PRIORITY:
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->Priority = CLAMP( params[0], 0.0F, 1.0F );
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
	    if (params[0] < 1.0) {
	       _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
	       return;
	    }
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MaxAnisotropy = params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_MAX_ANISOTROPY_EXT)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->CompareFlag = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_SGIX)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            GLenum op = (GLenum) params[0];
            if (op == GL_TEXTURE_LEQUAL_R_SGIX ||
                op == GL_TEXTURE_GEQUAL_R_SGIX) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareOperator = op;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(param)");
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                    "glTexParameter(pname=GL_TEXTURE_COMPARE_OPERATOR_SGIX)");
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->ShadowAmbient = CLAMP(params[0], 0.0F, 1.0F);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_SHADOW_AMBIENT_SGIX)");
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_GENERATE_MIPMAP_SGIS)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum mode = (GLenum) params[0];
            if (mode == GL_NONE || mode == GL_COMPARE_R_TO_TEXTURE_ARB) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareMode = mode;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_MODE_ARB: 0x%x)", mode);
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum func = (GLenum) params[0];
            if (func == GL_LEQUAL || func == GL_GEQUAL) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else if (ctx->Extensions.EXT_shadow_funcs &&
                     (func == GL_EQUAL ||
                      func == GL_NOTEQUAL ||
                      func == GL_LESS ||
                      func == GL_GREATER ||
                      func == GL_ALWAYS ||
                      func == GL_NEVER)) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_FUNC_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_FUNC_ARB)");
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            const GLenum result = (GLenum) params[0];
            if (result == GL_LUMINANCE || result == GL_INTENSITY
                || result == GL_ALPHA) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->DepthMode = result;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                          "glTexParameter(bad GL_DEPTH_TEXTURE_MODE_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_DEPTH_TEXTURE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         /* NOTE: this is really part of OpenGL 1.4, not EXT_texture_lod_bias*/
         if (ctx->Extensions.EXT_texture_lod_bias) {
            if (texObj->LodBias != params[0]) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->LodBias = params[0];
            }
         }
         break;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=0x%x)", pname);
         return;
   }

   texObj->Complete = GL_FALSE;

   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}


void GLAPIENTRY
_mesa_TexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   if (pname == GL_TEXTURE_PRIORITY)
      fparam[0] = INT_TO_FLOAT(param);
   else
      fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   _mesa_TexParameterfv(target, pname, fparam);
}


void GLAPIENTRY
_mesa_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      fparam[0] = INT_TO_FLOAT(params[0]);
      fparam[1] = INT_TO_FLOAT(params[1]);
      fparam[2] = INT_TO_FLOAT(params[2]);
      fparam[3] = INT_TO_FLOAT(params[3]);
   }
   else {
      if (pname == GL_TEXTURE_PRIORITY)
         fparam[0] = INT_TO_FLOAT(params[0]);
      else
         fparam[0] = (GLfloat) params[0];
      fparam[1] = fparam[2] = fparam[3] = 0.0F;
   }
   _mesa_TexParameterfv(target, pname, fparam);
}


void GLAPIENTRY
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GLint iparam;
   _mesa_GetTexLevelParameteriv( target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}


static GLuint
tex_image_dimensions(GLcontext *ctx, GLenum target)
{
   switch (target) {
      case GL_TEXTURE_1D:
      case GL_PROXY_TEXTURE_1D:
         return 1;
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_2D:
         return 2;
      case GL_TEXTURE_3D:
      case GL_PROXY_TEXTURE_3D:
         return 3;
      case GL_TEXTURE_CUBE_MAP:
      case GL_PROXY_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map ? 2 : 0;
      case GL_TEXTURE_RECTANGLE_NV:
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle ? 2 : 0;
      default:
         _mesa_problem(ctx, "bad target in _mesa_tex_target_dimensions()");
         return 0;
   }
}


void GLAPIENTRY
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   const struct gl_texture_image *img = NULL;
   GLuint dimensions;
   GLboolean isProxy;
   GLint maxLevels;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* this will catch bad target values */
   dimensions = tex_image_dimensions(ctx, target);  /* 1, 2 or 3 */
   if (dimensions == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
      return;
   }

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      maxLevels = ctx->Const.MaxTextureLevels;
      break;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      maxLevels = ctx->Const.Max3DTextureLevels;
      break;
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_PROXY_TEXTURE_CUBE_MAP:
      maxLevels = ctx->Const.MaxCubeTextureLevels;
      break;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      maxLevels = 1;
      break;
   default:
      _mesa_problem(ctx, "switch in _mesa_GetTexLevelParameter");
      return;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   img = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!img || !img->TexFormat) {
      /* undefined texture image */
      if (pname == GL_TEXTURE_COMPONENTS)
         *params = 1;
      else
         *params = 0;
      return;
   }

   isProxy = (target == GL_PROXY_TEXTURE_1D) ||
             (target == GL_PROXY_TEXTURE_2D) ||
             (target == GL_PROXY_TEXTURE_3D) ||
             (target == GL_PROXY_TEXTURE_CUBE_MAP) ||
             (target == GL_PROXY_TEXTURE_RECTANGLE_NV);

   switch (pname) {
      case GL_TEXTURE_WIDTH:
         *params = img->Width;
         return;
      case GL_TEXTURE_HEIGHT:
         *params = img->Height;
         return;
      case GL_TEXTURE_DEPTH:
         *params = img->Depth;
         return;
      case GL_TEXTURE_INTERNAL_FORMAT:
         *params = img->IntFormat;
         return;
      case GL_TEXTURE_BORDER:
         *params = img->Border;
         return;
      case GL_TEXTURE_RED_SIZE:
         if (img->Format == GL_RGB || img->Format == GL_RGBA)
            *params = img->TexFormat->RedBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_GREEN_SIZE:
         if (img->Format == GL_RGB || img->Format == GL_RGBA)
            *params = img->TexFormat->GreenBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_BLUE_SIZE:
         if (img->Format == GL_RGB || img->Format == GL_RGBA)
            *params = img->TexFormat->BlueBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_ALPHA_SIZE:
         if (img->Format == GL_ALPHA || img->Format == GL_LUMINANCE_ALPHA ||
             img->Format == GL_RGBA)
            *params = img->TexFormat->AlphaBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_INTENSITY_SIZE:
         if (img->Format != GL_INTENSITY)
            *params = 0;
         else if (img->TexFormat->IntensityBits > 0)
            *params = img->TexFormat->IntensityBits;
         else /* intensity probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         return;
      case GL_TEXTURE_LUMINANCE_SIZE:
         if (img->Format != GL_LUMINANCE &&
             img->Format != GL_LUMINANCE_ALPHA)
            *params = 0;
         else if (img->TexFormat->LuminanceBits > 0)
            *params = img->TexFormat->LuminanceBits;
         else /* luminance probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         return;
      case GL_TEXTURE_INDEX_SIZE_EXT:
         if (img->Format == GL_COLOR_INDEX)
            *params = img->TexFormat->IndexBits;
         else
            *params = 0;
         return;
      case GL_DEPTH_BITS:
         /* XXX this isn't in the GL_SGIX_depth_texture spec
          * but seems appropriate.
          */
         if (ctx->Extensions.SGIX_depth_texture)
            *params = img->TexFormat->DepthBits;
         else
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         return;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE:
         if (ctx->Extensions.ARB_texture_compression) {
            if (img->IsCompressed && !isProxy)
               *params = img->CompressedSize;
            else
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glGetTexLevelParameter[if]v(pname)");
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         return;
      case GL_TEXTURE_COMPRESSED:
         if (ctx->Extensions.ARB_texture_compression) {
            *params = (GLint) img->IsCompressed;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         return;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetTexLevelParameter[if]v(pname)");
   }
}



void GLAPIENTRY
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->MagFilter);
	 return;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->MinFilter);
         return;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->WrapS);
         return;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->WrapT);
         return;
      case GL_TEXTURE_WRAP_R:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         return;
      case GL_TEXTURE_BORDER_COLOR:
         params[0] = CLAMP(obj->BorderColor[0], 0.0F, 1.0F);
         params[1] = CLAMP(obj->BorderColor[1], 0.0F, 1.0F);
         params[2] = CLAMP(obj->BorderColor[2], 0.0F, 1.0F);
         params[3] = CLAMP(obj->BorderColor[3], 0.0F, 1.0F);
         return;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = ENUM_TO_FLOAT(resident);
         }
         return;
      case GL_TEXTURE_PRIORITY:
         *params = obj->Priority;
         return;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->MinLod;
         return;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->MaxLod;
         return;
      case GL_TEXTURE_BASE_LEVEL:
         *params = (GLfloat) obj->BaseLevel;
         return;
      case GL_TEXTURE_MAX_LEVEL:
         *params = (GLfloat) obj->MaxLevel;
         return;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = obj->MaxAnisotropy;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareFlag;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareOperator;
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = obj->ShadowAmbient;
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLfloat) obj->GenerateMipmap;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareMode;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareFunc;
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLfloat) obj->DepthMode;
            return;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = obj->LodBias;
            break;
         }
         break;
      default:
         ; /* silence warnings */
   }
   /* If we get here, pname was an unrecognized enum */
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname=0x%x)",
               pname);
}


void GLAPIENTRY
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = (GLint) obj->LodBias;
            break;
         }
         break;
         return;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         return;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         return;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         return;
      case GL_TEXTURE_WRAP_R:
         *params = (GLint) obj->WrapR;
         return;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat b[4];
            b[0] = CLAMP(obj->BorderColor[0], 0.0F, 1.0F);
            b[1] = CLAMP(obj->BorderColor[1], 0.0F, 1.0F);
            b[2] = CLAMP(obj->BorderColor[2], 0.0F, 1.0F);
            b[3] = CLAMP(obj->BorderColor[3], 0.0F, 1.0F);
            params[0] = FLOAT_TO_INT(b[0]);
            params[1] = FLOAT_TO_INT(b[1]);
            params[2] = FLOAT_TO_INT(b[2]);
            params[3] = FLOAT_TO_INT(b[3]);
         }
         return;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = (GLint) resident;
         }
         return;
      case GL_TEXTURE_PRIORITY:
         *params = FLOAT_TO_INT(obj->Priority);
         return;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         return;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         return;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         return;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         return;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = (GLint) obj->MaxAnisotropy;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareFlag;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareOperator;
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = (GLint) FLOAT_TO_INT(obj->ShadowAmbient);
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLint) obj->GenerateMipmap;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareMode;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareFunc;
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLint) obj->DepthMode;
            return;
         }
         break;
      default:
         ; /* silence warnings */
   }
   /* If we get here, pname was an unrecognized enum */
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname=0x%x)", pname);
}




/**********************************************************************/
/*                    Texture Coord Generation                        */
/**********************************************************************/

#if FEATURE_texgen
void GLAPIENTRY
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexGen %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(coord),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr((GLenum) (GLint) *params));

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bits;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bits = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       bits = TEXGEN_EYE_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       bits = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       bits = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_SPHERE_MAP:
	       bits = TEXGEN_SPHERE_MAP;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeS == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeS = mode;
	    texUnit->_GenBitS = bits;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneS, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneS[0] = params[0];
	    texUnit->ObjectPlaneS[1] = params[1];
	    texUnit->ObjectPlaneS[2] = params[2];
	    texUnit->ObjectPlaneS[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];

            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneS, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneS, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bitt;
	    switch (mode) {
               case GL_OBJECT_LINEAR:
                  bitt = TEXGEN_OBJ_LINEAR;
                  break;
               case GL_EYE_LINEAR:
                  bitt = TEXGEN_EYE_LINEAR;
                  break;
               case GL_REFLECTION_MAP_NV:
                  bitt = TEXGEN_REFLECTION_MAP_NV;
                  break;
               case GL_NORMAL_MAP_NV:
                  bitt = TEXGEN_NORMAL_MAP_NV;
                  break;
               case GL_SPHERE_MAP:
                  bitt = TEXGEN_SPHERE_MAP;
                  break;
               default:
                  _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
                  return;
	    }
	    if (texUnit->GenModeT == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeT = mode;
	    texUnit->_GenBitT = bitt;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneT, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneT[0] = params[0];
	    texUnit->ObjectPlaneT[1] = params[1];
	    texUnit->ObjectPlaneT[2] = params[2];
	    texUnit->ObjectPlaneT[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
	    if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneT, tmp))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneT, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bitr;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bitr = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       bitr = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       bitr = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_EYE_LINEAR:
	       bitr = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeR == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeR = mode;
	    texUnit->_GenBitR = bitr;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneR, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneR[0] = params[0];
	    texUnit->ObjectPlaneR[1] = params[1];
	    texUnit->ObjectPlaneR[2] = params[2];
	    texUnit->ObjectPlaneR[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneR, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneR, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bitq;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bitq = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       bitq = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeQ == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeQ = mode;
	    texUnit->_GenBitQ = bitq;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneQ, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneQ[0] = params[0];
	    texUnit->ObjectPlaneQ[1] = params[1];
	    texUnit->ObjectPlaneQ[2] = params[2];
	    texUnit->ObjectPlaneQ[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneQ, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneQ, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(coord)" );
	 return;
   }

   if (ctx->Driver.TexGen)
      ctx->Driver.TexGen( ctx, coord, pname, params );
}


void GLAPIENTRY
_mesa_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
      p[1] = p[2] = p[3] = 0.0F;
   }
   else {
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
      p[3] = (GLfloat) params[3];
   }
   _mesa_TexGenfv(coord, pname, p);
}


void GLAPIENTRY
_mesa_TexGend(GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   _mesa_TexGenfv( coord, pname, &p );
}


void GLAPIENTRY
_mesa_TexGendv(GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
      p[1] = p[2] = p[3] = 0.0F;
   }
   else {
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
      p[3] = (GLfloat) params[3];
   }
   _mesa_TexGenfv( coord, pname, p );
}


void GLAPIENTRY
_mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   _mesa_TexGenfv(coord, pname, &param);
}


void GLAPIENTRY
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   _mesa_TexGeniv( coord, pname, &param );
}



void GLAPIENTRY
_mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)" );
	 return;
   }
}



void GLAPIENTRY
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)" );
	 return;
   }
}



void GLAPIENTRY
_mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeS;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneS[0];
            params[1] = (GLint) texUnit->ObjectPlaneS[1];
            params[2] = (GLint) texUnit->ObjectPlaneS[2];
            params[3] = (GLint) texUnit->ObjectPlaneS[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneS[0];
            params[1] = (GLint) texUnit->EyePlaneS[1];
            params[2] = (GLint) texUnit->EyePlaneS[2];
            params[3] = (GLint) texUnit->EyePlaneS[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeT;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneT[0];
            params[1] = (GLint) texUnit->ObjectPlaneT[1];
            params[2] = (GLint) texUnit->ObjectPlaneT[2];
            params[3] = (GLint) texUnit->ObjectPlaneT[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneT[0];
            params[1] = (GLint) texUnit->EyePlaneT[1];
            params[2] = (GLint) texUnit->EyePlaneT[2];
            params[3] = (GLint) texUnit->EyePlaneT[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeR;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneR[0];
            params[1] = (GLint) texUnit->ObjectPlaneR[1];
            params[2] = (GLint) texUnit->ObjectPlaneR[2];
            params[3] = (GLint) texUnit->ObjectPlaneR[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneR[0];
            params[1] = (GLint) texUnit->EyePlaneR[1];
            params[2] = (GLint) texUnit->EyePlaneR[2];
            params[3] = (GLint) texUnit->EyePlaneR[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeQ;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneQ[0];
            params[1] = (GLint) texUnit->ObjectPlaneQ[1];
            params[2] = (GLint) texUnit->ObjectPlaneQ[2];
            params[3] = (GLint) texUnit->ObjectPlaneQ[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneQ[0];
            params[1] = (GLint) texUnit->EyePlaneQ[1];
            params[2] = (GLint) texUnit->EyePlaneQ[2];
            params[3] = (GLint) texUnit->EyePlaneQ[3];
         }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)" );
	 return;
   }
}
#endif

/* GL_ARB_multitexture */
void GLAPIENTRY
_mesa_ActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint texUnit = target - GL_TEXTURE0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glActiveTexture %s\n",
                  _mesa_lookup_enum_by_nr(target));

   /* Cater for texture unit 0 is first, therefore use >= */
   if (texUnit >= ctx->Const.MaxTextureUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glActiveTexture(target)");
      return;
   }

   if (ctx->Texture.CurrentUnit == texUnit)
      return;

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   ctx->Texture.CurrentUnit = texUnit;
   if (ctx->Transform.MatrixMode == GL_TEXTURE) {
      /* update current stack pointer */
      ctx->CurrentStack = &ctx->TextureMatrixStack[texUnit];
   }

   if (ctx->Driver.ActiveTexture) {
      (*ctx->Driver.ActiveTexture)( ctx, (GLuint) texUnit );
   }
}


/* GL_ARB_multitexture */
void GLAPIENTRY
_mesa_ClientActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint texUnit = target - GL_TEXTURE0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (texUnit > ctx->Const.MaxTextureUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClientActiveTexture(target)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ActiveTexture = texUnit;
}



/**********************************************************************/
/*                     Pixel Texgen Extensions                        */
/**********************************************************************/

void GLAPIENTRY
_mesa_PixelTexGenSGIX(GLenum mode)
{
   GLenum newRgbSource, newAlphaSource;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (mode) {
      case GL_NONE:
         newRgbSource = GL_PIXEL_GROUP_COLOR_SGIS;
         newAlphaSource = GL_PIXEL_GROUP_COLOR_SGIS;
         break;
      case GL_ALPHA:
         newRgbSource = GL_PIXEL_GROUP_COLOR_SGIS;
         newAlphaSource = GL_CURRENT_RASTER_COLOR;
         break;
      case GL_RGB:
         newRgbSource = GL_CURRENT_RASTER_COLOR;
         newAlphaSource = GL_PIXEL_GROUP_COLOR_SGIS;
         break;
      case GL_RGBA:
         newRgbSource = GL_CURRENT_RASTER_COLOR;
         newAlphaSource = GL_CURRENT_RASTER_COLOR;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glPixelTexGenSGIX(mode)");
         return;
   }

   if (newRgbSource == ctx->Pixel.FragmentRgbSource &&
       newAlphaSource == ctx->Pixel.FragmentAlphaSource)
      return;

   FLUSH_VERTICES(ctx, _NEW_PIXEL);
   ctx->Pixel.FragmentRgbSource = newRgbSource;
   ctx->Pixel.FragmentAlphaSource = newAlphaSource;
}


void GLAPIENTRY
_mesa_PixelTexGenParameterfSGIS(GLenum target, GLfloat value)
{
   _mesa_PixelTexGenParameteriSGIS(target, (GLint) value);
}


void GLAPIENTRY
_mesa_PixelTexGenParameterfvSGIS(GLenum target, const GLfloat *value)
{
   _mesa_PixelTexGenParameteriSGIS(target, (GLint) *value);
}


void GLAPIENTRY
_mesa_PixelTexGenParameteriSGIS(GLenum target, GLint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (value != GL_CURRENT_RASTER_COLOR && value != GL_PIXEL_GROUP_COLOR_SGIS) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelTexGenParameterSGIS(value)");
      return;
   }

   switch (target) {
   case GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS:
      if (ctx->Pixel.FragmentRgbSource == (GLenum) value)
	 return;
      FLUSH_VERTICES(ctx, _NEW_PIXEL);
      ctx->Pixel.FragmentRgbSource = (GLenum) value;
      break;
   case GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS:
      if (ctx->Pixel.FragmentAlphaSource == (GLenum) value)
	 return;
      FLUSH_VERTICES(ctx, _NEW_PIXEL);
      ctx->Pixel.FragmentAlphaSource = (GLenum) value;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelTexGenParameterSGIS(target)");
      return;
   }
}


void GLAPIENTRY
_mesa_PixelTexGenParameterivSGIS(GLenum target, const GLint *value)
{
  _mesa_PixelTexGenParameteriSGIS(target, *value);
}


void GLAPIENTRY
_mesa_GetPixelTexGenParameterfvSGIS(GLenum target, GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS) {
      *value = (GLfloat) ctx->Pixel.FragmentRgbSource;
   }
   else if (target == GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS) {
      *value = (GLfloat) ctx->Pixel.FragmentAlphaSource;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelTexGenParameterfvSGIS(target)");
   }
}


void GLAPIENTRY
_mesa_GetPixelTexGenParameterivSGIS(GLenum target, GLint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS) {
      *value = (GLint) ctx->Pixel.FragmentRgbSource;
   }
   else if (target == GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS) {
      *value = (GLint) ctx->Pixel.FragmentAlphaSource;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelTexGenParameterivSGIS(target)");
   }
}



/**********************************************************************/
/*****                    State management                        *****/
/**********************************************************************/


/**
 * \note This routine refers to derived texture attribute values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE_MATRIX.  On changes to _NEW_TEXTURE, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_textures(), below.
 *
 * \param ctx GL context.
 */
static void
update_texture_matrices( GLcontext *ctx )
{
   GLuint i;

   ctx->Texture._TexMatEnabled = 0;

   for (i=0; i < ctx->Const.MaxTextureUnits; i++) {
      if (ctx->TextureMatrixStack[i].Top->flags & MAT_DIRTY) {
	 _math_matrix_analyse( ctx->TextureMatrixStack[i].Top );

	 if (ctx->Texture.Unit[i]._ReallyEnabled &&
	     ctx->TextureMatrixStack[i].Top->type != MATRIX_IDENTITY)
	    ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(i);

	 if (ctx->Driver.TextureMatrix)
	    ctx->Driver.TextureMatrix( ctx, i, ctx->TextureMatrixStack[i].Top);
      }
   }
}




/**
 * \note This routine refers to derived texture matrix values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE.  On changes to _NEW_TEXTURE_MATRIX, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_texture_matrices, above.
 *
 * \param ctx GL context.
 */
static void
update_texture_state( GLcontext *ctx )
{
   GLuint unit;

   ctx->Texture._EnabledUnits = 0;
   ctx->Texture._GenFlags = 0;
   ctx->Texture._TexMatEnabled = 0;
   ctx->Texture._TexGenEnabled = 0;

   /* Update texture unit state.
    * XXX this loop should probably be broken into separate loops for
    * texture coord units and texture image units.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
      GLuint enableBits;

      texUnit->_Current = NULL;
      texUnit->_ReallyEnabled = 0;
      texUnit->_GenFlags = 0;

      /* Get the bitmask of texture enables */
      if (ctx->FragmentProgram.Enabled && ctx->FragmentProgram.Current) {
         enableBits = ctx->FragmentProgram.Current->TexturesUsed[unit];
      }
      else {
         if (!texUnit->Enabled)
            continue;
         enableBits = texUnit->Enabled;
      }

      /* Look for the highest-priority texture target that's enabled and
       * complete.  That's the one we'll use for texturing.  If we're using
       * a fragment program we're guaranteed that bitcount(enabledBits) <= 1.
       */
      if (enableBits & TEXTURE_CUBE_BIT) {
         struct gl_texture_object *texObj = texUnit->CurrentCubeMap;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE_CUBE_BIT;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (enableBits & TEXTURE_3D_BIT)) {
         struct gl_texture_object *texObj = texUnit->Current3D;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE_3D_BIT;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (enableBits & TEXTURE_RECT_BIT)) {
         struct gl_texture_object *texObj = texUnit->CurrentRect;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE_RECT_BIT;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (enableBits & TEXTURE_2D_BIT)) {
         struct gl_texture_object *texObj = texUnit->Current2D;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE_2D_BIT;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (enableBits & TEXTURE_1D_BIT)) {
         struct gl_texture_object *texObj = texUnit->Current1D;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE_1D_BIT;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled) {
         continue;
      }

      if (texUnit->_ReallyEnabled)
         ctx->Texture._EnabledUnits |= (1 << unit);

      if (texUnit->TexGenEnabled) {
	 if (texUnit->TexGenEnabled & S_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitS;
	 }
	 if (texUnit->TexGenEnabled & T_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitT;
	 }
	 if (texUnit->TexGenEnabled & Q_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitQ;
	 }
	 if (texUnit->TexGenEnabled & R_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitR;
	 }

	 ctx->Texture._TexGenEnabled |= ENABLE_TEXGEN(unit);
	 ctx->Texture._GenFlags |= texUnit->_GenFlags;
      }

      if (ctx->TextureMatrixStack[unit].Top->type != MATRIX_IDENTITY)
	 ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(unit);
   }

   ctx->Texture._EnabledCoordUnits = ctx->Texture._EnabledUnits;
   /* Fragment programs may need texture coordinates but not the
    * corresponding texture images.
    */
   if (ctx->FragmentProgram.Enabled && ctx->FragmentProgram.Current) {
      ctx->Texture._EnabledCoordUnits |=
         (ctx->FragmentProgram.Current->InputsRead >> FRAG_ATTRIB_TEX0);
   }
}


void _mesa_update_texture( GLcontext *ctx, GLuint new_state )
{
   if (new_state & _NEW_TEXTURE_MATRIX)
      update_texture_matrices( ctx );

   if (new_state & (_NEW_TEXTURE | _NEW_PROGRAM))
      update_texture_state( ctx );
}

/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

/**
 * Allocate the proxy textures for the given context.
 * 
 * \param ctx the context to allocate proxies for.
 * 
 * \return GL_TRUE on success, or GL_FALSE on failure
 * 
 * If run out of memory part way through the allocations, clean up and return
 * GL_FALSE.
 */
static GLboolean
alloc_proxy_textures( GLcontext *ctx )
{
   ctx->Texture.Proxy1D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_1D);
   if (!ctx->Texture.Proxy1D)
      goto cleanup;

   ctx->Texture.Proxy2D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_2D);
   if (!ctx->Texture.Proxy2D)
      goto cleanup;

   ctx->Texture.Proxy3D = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_3D);
   if (!ctx->Texture.Proxy3D)
      goto cleanup;

   ctx->Texture.ProxyCubeMap = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_CUBE_MAP_ARB);
   if (!ctx->Texture.ProxyCubeMap)
      goto cleanup;

   ctx->Texture.ProxyRect = (*ctx->Driver.NewTextureObject)(ctx, 0, GL_TEXTURE_RECTANGLE_NV);
   if (!ctx->Texture.ProxyRect)
      goto cleanup;

   return GL_TRUE;

 cleanup:
   if (ctx->Texture.Proxy1D)
      (ctx->Driver.DeleteTexture)(ctx, ctx->Texture.Proxy1D);
   if (ctx->Texture.Proxy2D)
      (ctx->Driver.DeleteTexture)(ctx, ctx->Texture.Proxy2D);
   if (ctx->Texture.Proxy3D)
      (ctx->Driver.DeleteTexture)(ctx, ctx->Texture.Proxy3D);
   if (ctx->Texture.ProxyCubeMap)
      (ctx->Driver.DeleteTexture)(ctx, ctx->Texture.ProxyCubeMap);
   if (ctx->Texture.ProxyRect)
      (ctx->Driver.DeleteTexture)(ctx, ctx->Texture.ProxyRect);
   return GL_FALSE;
}


/**
 * Initialize a texture unit.
 *
 * \param ctx GL context.
 * \param unit texture unit number to be initialized.
 */
static void
init_texture_unit( GLcontext *ctx, GLuint unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   texUnit->EnvMode = GL_MODULATE;
   ASSIGN_4V( texUnit->EnvColor, 0.0, 0.0, 0.0, 0.0 );

   texUnit->CombineModeRGB = GL_MODULATE;
   texUnit->CombineModeA = GL_MODULATE;
   texUnit->CombineSourceRGB[0] = GL_TEXTURE;
   texUnit->CombineSourceRGB[1] = GL_PREVIOUS_EXT;
   texUnit->CombineSourceRGB[2] = GL_CONSTANT_EXT;
   texUnit->CombineSourceA[0] = GL_TEXTURE;
   texUnit->CombineSourceA[1] = GL_PREVIOUS_EXT;
   texUnit->CombineSourceA[2] = GL_CONSTANT_EXT;
   texUnit->CombineOperandRGB[0] = GL_SRC_COLOR;
   texUnit->CombineOperandRGB[1] = GL_SRC_COLOR;
   texUnit->CombineOperandRGB[2] = GL_SRC_ALPHA;
   texUnit->CombineOperandA[0] = GL_SRC_ALPHA;
   texUnit->CombineOperandA[1] = GL_SRC_ALPHA;
   texUnit->CombineOperandA[2] = GL_SRC_ALPHA;
   texUnit->CombineScaleShiftRGB = 0;
   texUnit->CombineScaleShiftA = 0;

   texUnit->TexGenEnabled = 0;
   texUnit->GenModeS = GL_EYE_LINEAR;
   texUnit->GenModeT = GL_EYE_LINEAR;
   texUnit->GenModeR = GL_EYE_LINEAR;
   texUnit->GenModeQ = GL_EYE_LINEAR;
   texUnit->_GenBitS = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitT = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitR = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitQ = TEXGEN_EYE_LINEAR;

   /* Yes, these plane coefficients are correct! */
   ASSIGN_4V( texUnit->ObjectPlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneQ, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneQ, 0.0, 0.0, 0.0, 0.0 );

   texUnit->Current1D = ctx->Shared->Default1D;
   texUnit->Current2D = ctx->Shared->Default2D;
   texUnit->Current3D = ctx->Shared->Default3D;
   texUnit->CurrentCubeMap = ctx->Shared->DefaultCubeMap;
   texUnit->CurrentRect = ctx->Shared->DefaultRect;
}


GLboolean _mesa_init_texture( GLcontext * ctx )
{
   int i;

   assert(MAX_TEXTURE_LEVELS >= MAX_3D_TEXTURE_LEVELS);
   assert(MAX_TEXTURE_LEVELS >= MAX_CUBE_TEXTURE_LEVELS);

   /* Effectively bind the default textures to all texture units */
   ctx->Shared->Default1D->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->Default2D->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->Default3D->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->DefaultCubeMap->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->DefaultRect->RefCount += MAX_TEXTURE_UNITS;

   /* Texture group */
   ctx->Texture.CurrentUnit = 0;      /* multitexture */
   ctx->Texture._EnabledUnits = 0;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      init_texture_unit( ctx, i );
   ctx->Texture.SharedPalette = GL_FALSE;
   _mesa_init_colortable(&ctx->Texture.Palette);

   /* Allocate proxy textures */
   if (!alloc_proxy_textures( ctx ))
      return GL_FALSE;

   return GL_TRUE;
}

void _mesa_free_texture_data( GLcontext *ctx )
{
   int i;

   /* Free proxy texture objects */
   (ctx->Driver.DeleteTexture)(ctx,  ctx->Texture.Proxy1D );
   (ctx->Driver.DeleteTexture)(ctx,  ctx->Texture.Proxy2D );
   (ctx->Driver.DeleteTexture)(ctx,  ctx->Texture.Proxy3D );
   (ctx->Driver.DeleteTexture)(ctx,  ctx->Texture.ProxyCubeMap );
   (ctx->Driver.DeleteTexture)(ctx,  ctx->Texture.ProxyRect );

   for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
      _mesa_free_colortable_data( &ctx->Texture.Unit[i].ColorTable );
}
