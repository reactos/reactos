/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file texenv.c
 *
 * glTexEnv-related functions
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/texenv.h"
#include "math/m_xform.h"



void GLAPIENTRY
_mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   GLuint maxUnit;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexEnvfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

#define TE_ERROR(errCode, msg, value)				\
   _mesa_error(ctx, errCode, msg, _mesa_lookup_enum_by_nr(value));

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         {
            GLenum mode = (GLenum) (GLint) *param;
            if (mode == GL_REPLACE_EXT)
               mode = GL_REPLACE;
	    if (texUnit->EnvMode == mode)
	       return;
            if (mode == GL_MODULATE ||
                mode == GL_BLEND ||
                mode == GL_DECAL ||
                mode == GL_REPLACE ||
                (mode == GL_ADD && ctx->Extensions.EXT_texture_env_add) ||
                (mode == GL_COMBINE &&
                 (ctx->Extensions.EXT_texture_env_combine ||
                  ctx->Extensions.ARB_texture_env_combine))) {
               /* legal */
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
	    if (texUnit->Combine.ModeRGB == mode)
	       return;
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
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ModeRGB = mode;
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
	    if (texUnit->Combine.ModeA == mode)
	       return;
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
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ModeA = mode;
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
	    if (texUnit->Combine.SourceRGB[s] == source)
	       return;
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
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.SourceRGB[s] = source;
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
	    if (texUnit->Combine.SourceA[s] == source)
	       return;
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
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.SourceA[s] = source;
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
	    if (texUnit->Combine.OperandRGB[s] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_COLOR:
	    case GL_ONE_MINUS_SRC_COLOR:
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandRGB[s] = operand;
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
	    if (texUnit->Combine.OperandA[pname-GL_OPERAND0_ALPHA] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandA[pname-GL_OPERAND0_ALPHA] = operand;
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
	 if (ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandRGB[2] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_COLOR:           /* ARB combine only */
	    case GL_ONE_MINUS_SRC_COLOR: /* ARB combine only */
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandRGB[2] = operand;
               break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else if (ctx->Extensions.EXT_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandRGB[2] == operand)
	       return;
	    /* operand must be GL_SRC_ALPHA which is the initial value - thus
	       don't need to actually compare the operand to the possible value */
	    else {
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
	 if (ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandA[2] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandA[2] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else if (ctx->Extensions.EXT_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandA[2] == operand)
	       return;
	    /* operand must be GL_SRC_ALPHA which is the initial value - thus
	       don't need to actually compare the operand to the possible value */
	    else {
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
	    if (texUnit->Combine.ScaleShiftRGB == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ScaleShiftRGB = newshift;
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
	    if (texUnit->Combine.ScaleShiftA == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ScaleShiftA = newshift;
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
   GLuint maxUnit;
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnvfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

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
               *params = (GLfloat) texUnit->Combine.ModeRGB;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_COMBINE_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->Combine.ModeA;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE0_RGB:
         case GL_SOURCE1_RGB:
         case GL_SOURCE2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned rgb_idx = pname - GL_SOURCE0_RGB;
               *params = (GLfloat) texUnit->Combine.SourceRGB[rgb_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE0_ALPHA:
         case GL_SOURCE1_ALPHA:
         case GL_SOURCE2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned alpha_idx = pname - GL_SOURCE0_ALPHA;
               *params = (GLfloat) texUnit->Combine.SourceA[alpha_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND0_RGB:
         case GL_OPERAND1_RGB:
         case GL_OPERAND2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_rgb = pname - GL_OPERAND0_RGB;
               *params = (GLfloat) texUnit->Combine.OperandRGB[op_rgb];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND0_ALPHA:
         case GL_OPERAND1_ALPHA:
         case GL_OPERAND2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_alpha = pname - GL_OPERAND0_ALPHA;
               *params = (GLfloat) texUnit->Combine.OperandA[op_alpha];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_RGB_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->Combine.ScaleShiftRGB == 0)
                  *params = 1.0;
               else if (texUnit->Combine.ScaleShiftRGB == 1)
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
               if (texUnit->Combine.ScaleShiftA == 0)
                  *params = 1.0;
               else if (texUnit->Combine.ScaleShiftA == 1)
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
   GLuint maxUnit;
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnviv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

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
               *params = (GLint) texUnit->Combine.ModeRGB;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_COMBINE_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->Combine.ModeA;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE0_RGB:
         case GL_SOURCE1_RGB:
         case GL_SOURCE2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned rgb_idx = pname - GL_SOURCE0_RGB;
               *params = (GLint) texUnit->Combine.SourceRGB[rgb_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE0_ALPHA:
         case GL_SOURCE1_ALPHA:
         case GL_SOURCE2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned alpha_idx = pname - GL_SOURCE0_ALPHA;
               *params = (GLint) texUnit->Combine.SourceA[alpha_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND0_RGB:
         case GL_OPERAND1_RGB:
         case GL_OPERAND2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_rgb = pname - GL_OPERAND0_RGB;
               *params = (GLint) texUnit->Combine.OperandRGB[op_rgb];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND0_ALPHA:
         case GL_OPERAND1_ALPHA:
         case GL_OPERAND2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_alpha = pname - GL_OPERAND0_ALPHA;
               *params = (GLint) texUnit->Combine.OperandA[op_alpha];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_RGB_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->Combine.ScaleShiftRGB == 0)
                  *params = 1;
               else if (texUnit->Combine.ScaleShiftRGB == 1)
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
               if (texUnit->Combine.ScaleShiftA == 0)
                  *params = 1;
               else if (texUnit->Combine.ScaleShiftA == 1)
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


