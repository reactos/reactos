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
 * \file texgen.c
 *
 * glTexGen-related functions
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/texgen.h"
#include "math/m_xform.h"



void GLAPIENTRY
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexGen %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(coord),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr((GLenum) (GLint) *params));

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexGen(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLbitfield bits;
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
            COPY_4FV(texUnit->ObjectPlaneS, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
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
	    GLbitfield bitt;
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
            COPY_4FV(texUnit->ObjectPlaneT, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
	    if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
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
	    GLbitfield bitr;
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
	    COPY_4FV(texUnit->ObjectPlaneR, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
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
	    GLbitfield bitq;
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
            COPY_4FV(texUnit->ObjectPlaneQ, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
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
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexGendv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

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
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexGenfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

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
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexGeniv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

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


