/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

#include <precomp.h>

#if FEATURE_texgen


/**
 * Return texgen state for given coordinate
 */
static struct gl_texgen *
get_texgen(struct gl_texture_unit *texUnit, GLenum coord)
{
   switch (coord) {
   case GL_S:
      return &texUnit->GenS;
   case GL_T:
      return &texUnit->GenT;
   case GL_R:
      return &texUnit->GenR;
   case GL_Q:
      return &texUnit->GenQ;
   default:
      return NULL;
   }
}


void GLAPIENTRY
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texgen *texgen;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexGen %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(coord),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr((GLenum) (GLint) *params));

   texUnit = &ctx->Texture.Unit;

   texgen = get_texgen(texUnit, coord);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexGen(coord)");
      return;
   }

   switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      {
         GLenum mode = (GLenum) (GLint) params[0];
         GLbitfield bit = 0x0;
         if (texgen->Mode == mode)
            return;
         switch (mode) {
         case GL_OBJECT_LINEAR:
            bit = TEXGEN_OBJ_LINEAR;
            break;
         case GL_EYE_LINEAR:
            bit = TEXGEN_EYE_LINEAR;
            break;
         case GL_SPHERE_MAP:
            if (coord == GL_S || coord == GL_T)
               bit = TEXGEN_SPHERE_MAP;
            break;
         case GL_REFLECTION_MAP_NV:
            if (coord != GL_Q)
               bit = TEXGEN_REFLECTION_MAP_NV;
            break;
         case GL_NORMAL_MAP_NV:
            if (coord != GL_Q)
               bit = TEXGEN_NORMAL_MAP_NV;
            break;
         default:
            ; /* nop */
         }
         if (!bit) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texgen->Mode = mode;
         texgen->_ModeBit = bit;
      }
      break;

   case GL_OBJECT_PLANE:
      {
         if (TEST_EQ_4V(texgen->ObjectPlane, params))
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         COPY_4FV(texgen->ObjectPlane, params);
      }
      break;

   case GL_EYE_PLANE:
      {
         GLfloat tmp[4];
         /* Transform plane equation by the inverse modelview matrix */
         if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
            _math_matrix_analyse(ctx->ModelviewMatrixStack.Top);
         }
         _mesa_transform_vector(tmp, params,
                                ctx->ModelviewMatrixStack.Top->inv);
         if (TEST_EQ_4V(texgen->EyePlane, tmp))
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         COPY_4FV(texgen->EyePlane, tmp);
      }
      break;

   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
      return;
   }

   if (ctx->Driver.TexGen)
      ctx->Driver.TexGen( ctx, coord, pname, params );
}


static void GLAPIENTRY
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


static void GLAPIENTRY
_mesa_TexGend(GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0F;
   _mesa_TexGenfv( coord, pname, p );
}

#if FEATURE_ES1

void GLAPIENTRY
_es_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
   ASSERT(coord == GL_TEXTURE_GEN_STR_OES);
   _mesa_GetTexGenfv(GL_S, pname, params);
}


void GLAPIENTRY
_es_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
   ASSERT(coord == GL_TEXTURE_GEN_STR_OES);
   /* set S, T, and R at the same time */
   _mesa_TexGenf(GL_S, pname, param);
   _mesa_TexGenf(GL_T, pname, param);
   _mesa_TexGenf(GL_R, pname, param);
}


void GLAPIENTRY
_es_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
   ASSERT(coord == GL_TEXTURE_GEN_STR_OES);
   /* set S, T, and R at the same time */
   _mesa_TexGenfv(GL_S, pname, params);
   _mesa_TexGenfv(GL_T, pname, params);
   _mesa_TexGenfv(GL_R, pname, params);
}

#endif

static void GLAPIENTRY
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
   GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0F;
   _mesa_TexGenfv(coord, pname, p);
}


void GLAPIENTRY
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   GLint p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0;
   _mesa_TexGeniv( coord, pname, p );
}



static void GLAPIENTRY
_mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texgen *texgen;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texUnit = &ctx->Texture.Unit;

   texgen = get_texgen(texUnit, coord);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)");
      return;
   }

   switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      params[0] = ENUM_TO_DOUBLE(texgen->Mode);
      break;
   case GL_OBJECT_PLANE:
      COPY_4V(params, texgen->ObjectPlane);
      break;
   case GL_EYE_PLANE:
      COPY_4V(params, texgen->EyePlane);
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
   }
}



void GLAPIENTRY
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texgen *texgen;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texUnit = &ctx->Texture.Unit;

   texgen = get_texgen(texUnit, coord);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)");
      return;
   }

   switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      params[0] = ENUM_TO_FLOAT(texgen->Mode);
      break;
   case GL_OBJECT_PLANE:
      COPY_4V(params, texgen->ObjectPlane);
      break;
   case GL_EYE_PLANE:
      COPY_4V(params, texgen->EyePlane);
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
   }
}



static void GLAPIENTRY
_mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texgen *texgen;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texUnit = &ctx->Texture.Unit;

   texgen = get_texgen(texUnit, coord);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)");
      return;
   }

   switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      params[0] = texgen->Mode;
      break;
   case GL_OBJECT_PLANE:
      params[0] = (GLint) texgen->ObjectPlane[0];
      params[1] = (GLint) texgen->ObjectPlane[1];
      params[2] = (GLint) texgen->ObjectPlane[2];
      params[3] = (GLint) texgen->ObjectPlane[3];
      break;
   case GL_EYE_PLANE:
      params[0] = (GLint) texgen->EyePlane[0];
      params[1] = (GLint) texgen->EyePlane[1];
      params[2] = (GLint) texgen->EyePlane[2];
      params[3] = (GLint) texgen->EyePlane[3];
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
   }
}


void
_mesa_init_texgen_dispatch(struct _glapi_table *disp)
{
   SET_GetTexGendv(disp, _mesa_GetTexGendv);
   SET_GetTexGenfv(disp, _mesa_GetTexGenfv);
   SET_GetTexGeniv(disp, _mesa_GetTexGeniv);
   SET_TexGend(disp, _mesa_TexGend);
   SET_TexGendv(disp, _mesa_TexGendv);
   SET_TexGenf(disp, _mesa_TexGenf);
   SET_TexGenfv(disp, _mesa_TexGenfv);
   SET_TexGeni(disp, _mesa_TexGeni);
   SET_TexGeniv(disp, _mesa_TexGeniv);
}


#endif /* FEATURE_texgen */
