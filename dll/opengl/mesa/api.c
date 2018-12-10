/*
 * Mesa 3-D graphics library
 * Version:  2.5
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef PC_HEADER
#include "all.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include "api.h"
#include "bitmap.h"
#include "context.h"

#include "drawpix.h"

#include "eval.h"
#include "image.h"
#include "macros.h"
#include "matrix.h"
#include "teximage.h"
#include "types.h"
#include "vb.h"
#endif

void APIENTRY _mesa_Accum( GLenum op, GLfloat value )
{
   GET_CONTEXT;
   (*CC->API.Accum)(CC, op, value);
}

void APIENTRY _mesa_AlphaFunc( GLenum func, GLclampf ref )
{
   GET_CONTEXT;
   (*CC->API.AlphaFunc)(CC, func, ref);
}


GLboolean APIENTRY _mesa_AreTexturesResident( GLsizei n, const GLuint *textures,
                                 GLboolean *residences )
{
   GET_CONTEXT;
   return (*CC->API.AreTexturesResident)(CC, n, textures, residences);
}

void APIENTRY _mesa_ArrayElement( GLint i )
{
   GET_CONTEXT;
   (*CC->API.ArrayElement)(CC, i);
}


void APIENTRY _mesa_Begin( GLenum mode )
{
   GET_CONTEXT;
   (*CC->API.Begin)( CC, mode );
}


void APIENTRY _mesa_BindTexture( GLenum target, GLuint texture )
{
   GET_CONTEXT;
   (*CC->API.BindTexture)(CC, target, texture);
}


void APIENTRY _mesa_Bitmap( GLsizei width, GLsizei height,
               GLfloat xorig, GLfloat yorig,
               GLfloat xmove, GLfloat ymove,
               const GLubyte *bitmap )
{
   GET_CONTEXT;
   if (!CC->CompileFlag) {
      /* execute only, try optimized case where no unpacking needed */
      if (   CC->Unpack.LsbFirst==GL_FALSE
          && CC->Unpack.Alignment==1
          && CC->Unpack.RowLength==0
          && CC->Unpack.SkipPixels==0
          && CC->Unpack.SkipRows==0) {
         /* Special case: no unpacking needed */
         struct gl_image image;
         image.Width = width;
         image.Height = height;
         image.Components = 0;
         image.Type = GL_BITMAP;
         image.Format = GL_COLOR_INDEX;
         image.Data = (GLvoid *) bitmap;
         (*CC->Exec.Bitmap)( CC, width, height, xorig, yorig,
                             xmove, ymove, &image );
      }
      else {
         struct gl_image *image;
         image = gl_unpack_bitmap( CC, width, height, bitmap );
         (*CC->Exec.Bitmap)( CC, width, height, xorig, yorig,
                             xmove, ymove, image );
         if (image) {
            gl_free_image( image );
         }
      }
   }
   else {
      /* compile and maybe execute */
      struct gl_image *image;
      image = gl_unpack_bitmap( CC, width, height, bitmap );
      (*CC->API.Bitmap)(CC, width, height, xorig, yorig, xmove, ymove, image );
   }
}


void APIENTRY _mesa_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   GET_CONTEXT;
   (*CC->API.BlendFunc)(CC, sfactor, dfactor);
}


void APIENTRY _mesa_CallList( GLuint list )
{
   GET_CONTEXT;
   (*CC->API.CallList)(CC, list);
}


void APIENTRY _mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists )
{
   GET_CONTEXT;
   (*CC->API.CallLists)(CC, n, type, lists);
}


void APIENTRY _mesa_Clear( GLbitfield mask )
{
   GET_CONTEXT;
   (*CC->API.Clear)(CC, mask);
}


void APIENTRY _mesa_ClearAccum( GLfloat red, GLfloat green,
              GLfloat blue, GLfloat alpha )
{
   GET_CONTEXT;
   (*CC->API.ClearAccum)(CC, red, green, blue, alpha);
}



void APIENTRY _mesa_ClearIndex( GLfloat c )
{
   GET_CONTEXT;
   (*CC->API.ClearIndex)(CC, c);
}


void APIENTRY _mesa_ClearColor( GLclampf red,
              GLclampf green,
              GLclampf blue,
              GLclampf alpha )
{
   GET_CONTEXT;
   (*CC->API.ClearColor)(CC, red, green, blue, alpha);
}


void APIENTRY _mesa_ClearDepth( GLclampd depth )
{
   GET_CONTEXT;
   (*CC->API.ClearDepth)( CC, depth );
}


void APIENTRY _mesa_ClearStencil( GLint s )
{
   GET_CONTEXT;
   (*CC->API.ClearStencil)(CC, s);
}


void APIENTRY _mesa_ClipPlane( GLenum plane, const GLdouble *equation )
{
   GLfloat eq[4];
   GET_CONTEXT;
   eq[0] = (GLfloat) equation[0];
   eq[1] = (GLfloat) equation[1];
   eq[2] = (GLfloat) equation[2];
   eq[3] = (GLfloat) equation[3];
   (*CC->API.ClipPlane)(CC, plane, eq );
}


void APIENTRY _mesa_Color3b( GLbyte red, GLbyte green, GLbyte blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, BYTE_TO_FLOAT(red), BYTE_TO_FLOAT(green),
                       BYTE_TO_FLOAT(blue) );
}


void APIENTRY _mesa_Color3d( GLdouble red, GLdouble green, GLdouble blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, (GLfloat) red, (GLfloat) green, (GLfloat) blue );
}


void APIENTRY _mesa_Color3f( GLfloat red, GLfloat green, GLfloat blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, red, green, blue );
}


void APIENTRY _mesa_Color3i( GLint red, GLint green, GLint blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, INT_TO_FLOAT(red), INT_TO_FLOAT(green),
                       INT_TO_FLOAT(blue) );
}


void APIENTRY _mesa_Color3s( GLshort red, GLshort green, GLshort blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
                       SHORT_TO_FLOAT(blue) );
}


void APIENTRY _mesa_Color3ub( GLubyte red, GLubyte green, GLubyte blue )
{
   GET_CONTEXT;
   (*CC->API.Color4ub)( CC, red, green, blue, 255 );
}


void APIENTRY _mesa_Color3ui( GLuint red, GLuint green, GLuint blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
                       UINT_TO_FLOAT(blue) );
}


void APIENTRY _mesa_Color3us( GLushort red, GLushort green, GLushort blue )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
                       USHORT_TO_FLOAT(blue) );
}


void APIENTRY _mesa_Color4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, BYTE_TO_FLOAT(red), BYTE_TO_FLOAT(green),
                       BYTE_TO_FLOAT(blue), BYTE_TO_FLOAT(alpha) );
}


void APIENTRY _mesa_Color4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, (GLfloat) red, (GLfloat) green,
                       (GLfloat) blue, (GLfloat) alpha );
}


void APIENTRY _mesa_Color4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, red, green, blue, alpha );
}

void APIENTRY _mesa_Color4i( GLint red, GLint green, GLint blue, GLint alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, INT_TO_FLOAT(red), INT_TO_FLOAT(green),
                       INT_TO_FLOAT(blue), INT_TO_FLOAT(alpha) );
}


void APIENTRY _mesa_Color4s( GLshort red, GLshort green, GLshort blue, GLshort alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
                       SHORT_TO_FLOAT(blue), SHORT_TO_FLOAT(alpha) );
}

void APIENTRY _mesa_Color4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4ub)( CC, red, green, blue, alpha );
}

void APIENTRY _mesa_Color4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
                       UINT_TO_FLOAT(blue), UINT_TO_FLOAT(alpha) );
}

void APIENTRY _mesa_Color4us( GLushort red, GLushort green, GLushort blue, GLushort alpha )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
                       USHORT_TO_FLOAT(blue), USHORT_TO_FLOAT(alpha) );
}


void APIENTRY _mesa_Color3bv( const GLbyte *v )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
                       BYTE_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_Color3dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, (GLdouble) v[0], (GLdouble) v[1], (GLdouble) v[2] );
}


void APIENTRY _mesa_Color3fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Color3fv)( CC, v );
}


void APIENTRY _mesa_Color3iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
                       INT_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_Color3sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
                       SHORT_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_Color3ubv( const GLubyte *v )
{
   GET_CONTEXT;
   (*CC->API.Color4ub)( CC, v[0], v[1], v[2], 255 );
}


void APIENTRY _mesa_Color3uiv( const GLuint *v )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
                       UINT_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_Color3usv( const GLushort *v )
{
   GET_CONTEXT;
   (*CC->API.Color3f)( CC, USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
                       USHORT_TO_FLOAT(v[2]) );

}


void APIENTRY _mesa_Color4bv( const GLbyte *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
                       BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]) );
}


void APIENTRY _mesa_Color4dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, (GLdouble) v[0], (GLdouble) v[1],
                       (GLdouble) v[2], (GLdouble) v[3] );
}


void APIENTRY _mesa_Color4fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, v[0], v[1], v[2], v[3] );
}


void APIENTRY _mesa_Color4iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
                       INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]) );
}


void APIENTRY _mesa_Color4sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
                       SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]) );
}


void APIENTRY _mesa_Color4ubv( const GLubyte *v )
{
   GET_CONTEXT;
   (*CC->API.Color4ubv)( CC, v );
}


void APIENTRY _mesa_Color4uiv( const GLuint *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
                       UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]) );
}


void APIENTRY _mesa_Color4usv( const GLushort *v )
{
   GET_CONTEXT;
   (*CC->API.Color4f)( CC, USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
                       USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]) );
}


void APIENTRY _mesa_ColorMask( GLboolean red, GLboolean green,
             GLboolean blue, GLboolean alpha )
{
   GET_CONTEXT;
          (*CC->API.ColorMask)(CC, red, green, blue, alpha);
}


void APIENTRY _mesa_ColorMaterial( GLenum face, GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.ColorMaterial)(CC, face, mode);
}


void APIENTRY _mesa_ColorPointer( GLint size, GLenum type, GLsizei stride,
                     const GLvoid *ptr )
{
   GET_CONTEXT;
          (*CC->API.ColorPointer)(CC, size, type, stride, ptr);
}


void APIENTRY _mesa_CopyPixels( GLint x, GLint y, GLsizei width, GLsizei height,
              GLenum type )
{
   GET_CONTEXT;
          (*CC->API.CopyPixels)(CC, x, y, width, height, type);
}


void APIENTRY _mesa_CopyTexImage1D( GLenum target, GLint level,
                                GLenum internalformat,
                                GLint x, GLint y,
                                GLsizei width, GLint border )
{
   GET_CONTEXT;
          (*CC->API.CopyTexImage1D)( CC, target, level, internalformat,
                                 x, y, width, border );
}


void APIENTRY _mesa_CopyTexImage2D( GLenum target, GLint level,
                                GLenum internalformat,
                                GLint x, GLint y,
                                GLsizei width, GLsizei height, GLint border )
{
   GET_CONTEXT;
          (*CC->API.CopyTexImage2D)( CC, target, level, internalformat,
                              x, y, width, height, border );
}


void APIENTRY _mesa_CopyTexSubImage1D( GLenum target, GLint level,
                                   GLint xoffset, GLint x, GLint y,
                                   GLsizei width )
{
   GET_CONTEXT;
          (*CC->API.CopyTexSubImage1D)( CC, target, level, xoffset, x, y, width );
}


void APIENTRY _mesa_CopyTexSubImage2D( GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height )
{
   GET_CONTEXT;
          (*CC->API.CopyTexSubImage2D)( CC, target, level, xoffset, yoffset,
                                 x, y, width, height );
}



void APIENTRY _mesa_CullFace( GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.CullFace)(CC, mode);
}


void APIENTRY _mesa_DepthFunc( GLenum func )
{
   GET_CONTEXT;
          (*CC->API.DepthFunc)( CC, func );
}


void APIENTRY _mesa_DepthMask( GLboolean flag )
{
   GET_CONTEXT;
          (*CC->API.DepthMask)( CC, flag );
}


void APIENTRY _mesa_DepthRange( GLclampd near_val, GLclampd far_val )
{
   GET_CONTEXT;
          (*CC->API.DepthRange)( CC, near_val, far_val );
}


void APIENTRY _mesa_DeleteLists( GLuint list, GLsizei range )
{
   GET_CONTEXT;
          (*CC->API.DeleteLists)(CC, list, range);
}


void APIENTRY _mesa_DeleteTextures( GLsizei n, const GLuint *textures)
{
   GET_CONTEXT;
          (*CC->API.DeleteTextures)(CC, n, textures);
}


void APIENTRY _mesa_Disable( GLenum cap )
{
   GET_CONTEXT;
          (*CC->API.Disable)( CC, cap );
}


void APIENTRY _mesa_DisableClientState( GLenum cap )
{
   GET_CONTEXT;
          (*CC->API.DisableClientState)( CC, cap );
}


void APIENTRY _mesa_DrawArrays( GLenum mode, GLint first, GLsizei count )
{
   GET_CONTEXT;
          (*CC->API.DrawArrays)(CC, mode, first, count);
}


void APIENTRY _mesa_DrawBuffer( GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.DrawBuffer)(CC, mode);
}


void APIENTRY _mesa_DrawElements( GLenum mode, GLsizei count,
                              GLenum type, const GLvoid *indices )
{
   GET_CONTEXT;
          (*CC->API.DrawElements)( CC, mode, count, type, indices );
}


void APIENTRY _mesa_DrawPixels( GLsizei width, GLsizei height,
                            GLenum format, GLenum type, const GLvoid *pixels )
{
   GET_CONTEXT;
          (*CC->API.DrawPixels)( CC, width, height, format, type, pixels );
}


void APIENTRY _mesa_Enable( GLenum cap )
{
   GET_CONTEXT;
          (*CC->API.Enable)( CC, cap );
}


void APIENTRY _mesa_EnableClientState( GLenum cap )
{
   GET_CONTEXT;
          (*CC->API.EnableClientState)( CC, cap );
}


void APIENTRY _mesa_End( void )
{
   GET_CONTEXT;
          (*CC->API.End)( CC );
}


void APIENTRY _mesa_EndList( void )
{
   GET_CONTEXT;
          (*CC->API.EndList)(CC);
}




void APIENTRY _mesa_EvalCoord1d( GLdouble u )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord1f)( CC, (GLfloat) u );
}


void APIENTRY _mesa_EvalCoord1f( GLfloat u )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord1f)( CC, u );
}


void APIENTRY _mesa_EvalCoord1dv( const GLdouble *u )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord1f)( CC, (GLfloat) *u );
}


void APIENTRY _mesa_EvalCoord1fv( const GLfloat *u )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord1f)( CC, (GLfloat) *u );
}


void APIENTRY _mesa_EvalCoord2d( GLdouble u, GLdouble v )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord2f)( CC, (GLfloat) u, (GLfloat) v );
}


void APIENTRY _mesa_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord2f)( CC, u, v );
}


void APIENTRY _mesa_EvalCoord2dv( const GLdouble *u )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord2f)( CC, (GLfloat) u[0], (GLfloat) u[1] );
}


void APIENTRY _mesa_EvalCoord2fv( const GLfloat *u )
{
   GET_CONTEXT;
          (*CC->API.EvalCoord2f)( CC, u[0], u[1] );
}


void APIENTRY _mesa_EvalPoint1( GLint i )
{
   GET_CONTEXT;
          (*CC->API.EvalPoint1)( CC, i );
}


void APIENTRY _mesa_EvalPoint2( GLint i, GLint j )
{
   GET_CONTEXT;
          (*CC->API.EvalPoint2)( CC, i, j );
}


void APIENTRY _mesa_EvalMesh1( GLenum mode, GLint i1, GLint i2 )
{
   GET_CONTEXT;
          (*CC->API.EvalMesh1)( CC, mode, i1, i2 );
}


void APIENTRY _mesa_EdgeFlag( GLboolean flag )
{
   GET_CONTEXT;
          (*CC->API.EdgeFlag)(CC, flag);
}


void APIENTRY _mesa_EdgeFlagv( const GLboolean *flag )
{
   GET_CONTEXT;
          (*CC->API.EdgeFlag)(CC, *flag);
}


void APIENTRY _mesa_EdgeFlagPointer( GLsizei stride, const GLboolean *ptr )
{
   GET_CONTEXT;
          (*CC->API.EdgeFlagPointer)(CC, stride, ptr);
}


void APIENTRY _mesa_EvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
   GET_CONTEXT;
          (*CC->API.EvalMesh2)( CC, mode, i1, i2, j1, j2 );
}


void APIENTRY _mesa_FeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
{
   GET_CONTEXT;
          (*CC->API.FeedbackBuffer)(CC, size, type, buffer);
}


void APIENTRY _mesa_Finish( void )
{
   GET_CONTEXT;
          (*CC->API.Finish)(CC);
}


void APIENTRY _mesa_Flush( void )
{
   GET_CONTEXT;
          (*CC->API.Flush)(CC);
}


void APIENTRY _mesa_Fogf( GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.Fogfv)(CC, pname, &param);
}


void APIENTRY _mesa_Fogi( GLenum pname, GLint param )
{
   GLfloat fparam = (GLfloat) param;
   GET_CONTEXT;
          (*CC->API.Fogfv)(CC, pname, &fparam);
}


void APIENTRY _mesa_Fogfv( GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.Fogfv)(CC, pname, params);
}


void APIENTRY _mesa_Fogiv( GLenum pname, const GLint *params )
{
   GLfloat p[4];
   GET_CONTEXT;

   switch (pname) {
      case GL_FOG_MODE:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_INDEX:
     p[0] = (GLfloat) *params;
     break;
      case GL_FOG_COLOR:
     p[0] = INT_TO_FLOAT( params[0] );
     p[1] = INT_TO_FLOAT( params[1] );
     p[2] = INT_TO_FLOAT( params[2] );
     p[3] = INT_TO_FLOAT( params[3] );
     break;
      default:
         /* Error will be caught later in gl_Fogfv */
         ;
   }
   (*CC->API.Fogfv)( CC, pname, p );
}



void APIENTRY _mesa_FrontFace( GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.FrontFace)(CC, mode);
}


void APIENTRY _mesa_Frustum( GLdouble left, GLdouble right,
                         GLdouble bottom, GLdouble top,
                         GLdouble nearval, GLdouble farval )
{
   GET_CONTEXT;
          (*CC->API.Frustum)(CC, left, right, bottom, top, nearval, farval);
}


GLuint APIENTRY _mesa_GenLists( GLsizei range )
{
   GET_CONTEXT;
          return (*CC->API.GenLists)(CC, range);
}


void APIENTRY _mesa_GenTextures( GLsizei n, GLuint *textures )
{
   GET_CONTEXT;
          (*CC->API.GenTextures)(CC, n, textures);
}


void APIENTRY _mesa_GetBooleanv( GLenum pname, GLboolean *params )
{
   GET_CONTEXT;
          (*CC->API.GetBooleanv)(CC, pname, params);
}


void APIENTRY _mesa_GetClipPlane( GLenum plane, GLdouble *equation )
{
   GET_CONTEXT;
          (*CC->API.GetClipPlane)(CC, plane, equation);
}

void APIENTRY _mesa_GetDoublev( GLenum pname, GLdouble *params )
{
   GET_CONTEXT;
          (*CC->API.GetDoublev)(CC, pname, params);
}


GLenum APIENTRY _mesa_GetError( void )
{
   GET_CONTEXT;
   if (!CC) {
      /* No current context */
      return GL_NO_ERROR;
   }
   return (*CC->API.GetError)(CC);
}


void APIENTRY _mesa_GetFloatv( GLenum pname, GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.GetFloatv)(CC, pname, params);
}


void APIENTRY _mesa_GetIntegerv( GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetIntegerv)(CC, pname, params);
}


void APIENTRY _mesa_GetLightfv( GLenum light, GLenum pname, GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.GetLightfv)(CC, light, pname, params);
}


void APIENTRY _mesa_GetLightiv( GLenum light, GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetLightiv)(CC, light, pname, params);
}


void APIENTRY _mesa_GetMapdv( GLenum target, GLenum query, GLdouble *v )
{
   GET_CONTEXT;
          (*CC->API.GetMapdv)( CC, target, query, v );
}


void APIENTRY _mesa_GetMapfv( GLenum target, GLenum query, GLfloat *v )
{
   GET_CONTEXT;
          (*CC->API.GetMapfv)( CC, target, query, v );
}


void APIENTRY _mesa_GetMapiv( GLenum target, GLenum query, GLint *v )
{
   GET_CONTEXT;
          (*CC->API.GetMapiv)( CC, target, query, v );
}


void APIENTRY _mesa_GetMaterialfv( GLenum face, GLenum pname, GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.GetMaterialfv)(CC, face, pname, params);
}


void APIENTRY _mesa_GetMaterialiv( GLenum face, GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetMaterialiv)(CC, face, pname, params);
}


void APIENTRY _mesa_GetPixelMapfv( GLenum map, GLfloat *values )
{
   GET_CONTEXT;
          (*CC->API.GetPixelMapfv)(CC, map, values);
}


void APIENTRY _mesa_GetPixelMapuiv( GLenum map, GLuint *values )
{
   GET_CONTEXT;
          (*CC->API.GetPixelMapuiv)(CC, map, values);
}


void APIENTRY _mesa_GetPixelMapusv( GLenum map, GLushort *values )
{
   GET_CONTEXT;
          (*CC->API.GetPixelMapusv)(CC, map, values);
}


void APIENTRY _mesa_GetPointerv( GLenum pname, GLvoid **params )
{
   GET_CONTEXT;
          (*CC->API.GetPointerv)(CC, pname, params);
}


void APIENTRY _mesa_GetPolygonStipple( GLubyte *mask )
{
   GET_CONTEXT;
          (*CC->API.GetPolygonStipple)(CC, mask);
}


const GLubyte * APIENTRY _mesa_GetString( GLenum name )
{
   GET_CONTEXT;
          return (*CC->API.GetString)(CC, name);
}



void APIENTRY _mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexEnvfv)(CC, target, pname, params);
}


void APIENTRY _mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexEnviv)(CC, target, pname, params);
}


void APIENTRY _mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexGeniv)(CC, coord, pname, params);
}


void APIENTRY _mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexGendv)(CC, coord, pname, params);
}


void APIENTRY _mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexGenfv)(CC, coord, pname, params);
}



void APIENTRY _mesa_GetTexImage( GLenum target, GLint level, GLenum format,
                             GLenum type, GLvoid *pixels )
{
   GET_CONTEXT;
          (*CC->API.GetTexImage)(CC, target, level, format, type, pixels);
}


void APIENTRY _mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                                        GLenum pname, GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexLevelParameterfv)(CC, target, level, pname, params);
}


void APIENTRY _mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                                        GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexLevelParameteriv)(CC, target, level, pname, params);
}




void APIENTRY _mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params)
{
   GET_CONTEXT;
          (*CC->API.GetTexParameterfv)(CC, target, pname, params);
}


void APIENTRY _mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CONTEXT;
          (*CC->API.GetTexParameteriv)(CC, target, pname, params);
}


void APIENTRY _mesa_Hint( GLenum target, GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.Hint)(CC, target, mode);
}


void APIENTRY _mesa_Indexd( GLdouble c )
{
   GET_CONTEXT;
   (*CC->API.Indexf)( CC, (GLfloat) c );
}


void APIENTRY _mesa_Indexf( GLfloat c )
{
   GET_CONTEXT;
   (*CC->API.Indexf)( CC, c );
}


void APIENTRY _mesa_Indexi( GLint c )
{
   GET_CONTEXT;
   (*CC->API.Indexi)( CC, c );
}


void APIENTRY _mesa_Indexs( GLshort c )
{
   GET_CONTEXT;
   (*CC->API.Indexi)( CC, (GLint) c );
}


#ifdef GL_VERSION_1_1
void APIENTRY _mesa_Indexub( GLubyte c )
{
   GET_CONTEXT;
   (*CC->API.Indexi)( CC, (GLint) c );
}
#endif


void APIENTRY _mesa_Indexdv( const GLdouble *c )
{
   GET_CONTEXT;
   (*CC->API.Indexf)( CC, (GLfloat) *c );
}


void APIENTRY _mesa_Indexfv( const GLfloat *c )
{
   GET_CONTEXT;
   (*CC->API.Indexf)( CC, *c );
}


void APIENTRY _mesa_Indexiv( const GLint *c )
{
   GET_CONTEXT;
   (*CC->API.Indexi)( CC, *c );
}


void APIENTRY _mesa_Indexsv( const GLshort *c )
{
   GET_CONTEXT;
   (*CC->API.Indexi)( CC, (GLint) *c );
}


#ifdef GL_VERSION_1_1
void APIENTRY _mesa_Indexubv( const GLubyte *c )
{
   GET_CONTEXT;
   (*CC->API.Indexi)( CC, (GLint) *c );
}
#endif


void APIENTRY _mesa_IndexMask( GLuint mask )
{
   GET_CONTEXT;
   (*CC->API.IndexMask)(CC, mask);
}


void APIENTRY _mesa_IndexPointer( GLenum type, GLsizei stride, const GLvoid *ptr )
{
   GET_CONTEXT;
          (*CC->API.IndexPointer)(CC, type, stride, ptr);
}


void APIENTRY _mesa_InterleavedArrays( GLenum format, GLsizei stride,
                                   const GLvoid *pointer )
{
   GET_CONTEXT;
          (*CC->API.InterleavedArrays)( CC, format, stride, pointer );
}


void APIENTRY _mesa_InitNames( void )
{
   GET_CONTEXT;
          (*CC->API.InitNames)(CC);
}


GLboolean APIENTRY _mesa_IsList( GLuint list )
{
   GET_CONTEXT;
          return (*CC->API.IsList)(CC, list);
}


GLboolean APIENTRY _mesa_IsTexture( GLuint texture )
{
   GET_CONTEXT;
          return (*CC->API.IsTexture)(CC, texture);
}


void APIENTRY _mesa_Lightf( GLenum light, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.Lightfv)( CC, light, pname, &param, 1 );
}



void APIENTRY _mesa_Lighti( GLenum light, GLenum pname, GLint param )
{
   GLfloat fparam = (GLfloat) param;
   GET_CONTEXT;
          (*CC->API.Lightfv)( CC, light, pname, &fparam, 1 );
}



void APIENTRY _mesa_Lightfv( GLenum light, GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.Lightfv)( CC, light, pname, params, 4 );
}



void APIENTRY _mesa_Lightiv( GLenum light, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   GET_CONTEXT;

   switch (pname) {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_POSITION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         fparam[3] = (GLfloat) params[3];
         break;
      case GL_SPOT_DIRECTION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         break;
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* error will be caught later in gl_Lightfv */
         ;
   }
   (*CC->API.Lightfv)( CC, light, pname, fparam, 4 );
}



void APIENTRY _mesa_LightModelf( GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.LightModelfv)( CC, pname, &param );
}


void APIENTRY _mesa_LightModeli( GLenum pname, GLint param )
{
   GLfloat fparam[4];
   GET_CONTEXT;
          fparam[0] = (GLfloat) param;
   (*CC->API.LightModelfv)( CC, pname, fparam );
}


void APIENTRY _mesa_LightModelfv( GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.LightModelfv)( CC, pname, params );
}


void APIENTRY _mesa_LightModeliv( GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   GET_CONTEXT;

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      case GL_LIGHT_MODEL_TWO_SIDE:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* Error will be caught later in gl_LightModelfv */
         ;
   }
   (*CC->API.LightModelfv)( CC, pname, fparam );
}


void APIENTRY _mesa_LineWidth( GLfloat width )
{
   GET_CONTEXT;
          (*CC->API.LineWidth)(CC, width);
}


void APIENTRY _mesa_LineStipple( GLint factor, GLushort pattern )
{
   GET_CONTEXT;
          (*CC->API.LineStipple)(CC, factor, pattern);
}


void APIENTRY _mesa_ListBase( GLuint base )
{
   GET_CONTEXT;
          (*CC->API.ListBase)(CC, base);
}


void APIENTRY _mesa_LoadIdentity( void )
{
   GET_CONTEXT;
          (*CC->API.LoadIdentity)( CC );
}


void APIENTRY _mesa_LoadMatrixd( const GLdouble *m )
{
   GLfloat fm[16];
   GLuint i;
   GET_CONTEXT;

   for (i=0;i<16;i++) {
      fm[i] = (GLfloat) m[i];
   }

   (*CC->API.LoadMatrixf)( CC, fm );
}


void APIENTRY _mesa_LoadMatrixf( const GLfloat *m )
{
   GET_CONTEXT;
          (*CC->API.LoadMatrixf)( CC, m );
}


void APIENTRY _mesa_LoadName( GLuint name )
{
   GET_CONTEXT;
          (*CC->API.LoadName)(CC, name);
}


void APIENTRY _mesa_LogicOp( GLenum opcode )
{
   GET_CONTEXT;
          (*CC->API.LogicOp)(CC, opcode);
}



void APIENTRY _mesa_Map1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride,
                       GLint order, const GLdouble *points )
{
   GLfloat *pnts;
   GLboolean retain;
   GET_CONTEXT;

   pnts = gl_copy_map_points1d( target, stride, order, points );
   retain = CC->CompileFlag;
   (*CC->API.Map1f)( CC, target, u1, u2, stride, order, pnts, retain );
}


void APIENTRY _mesa_Map1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride,
                       GLint order, const GLfloat *points )
{
   GLfloat *pnts;
   GLboolean retain;
   GET_CONTEXT;

   pnts = gl_copy_map_points1f( target, stride, order, points );
   retain = CC->CompileFlag;
   (*CC->API.Map1f)( CC, target, u1, u2, stride, order, pnts, retain );
}


void APIENTRY _mesa_Map2d( GLenum target,
                       GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
                       GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
                       const GLdouble *points )
{
   GLfloat *pnts;
   GLboolean retain;
   GET_CONTEXT;

   pnts = gl_copy_map_points2d( target, ustride, uorder,
                                vstride, vorder, points );
   retain = CC->CompileFlag;
   (*CC->API.Map2f)( CC, target, u1, u2, ustride, uorder,
                     v1, v2, vstride, vorder, pnts, retain );
}


void APIENTRY _mesa_Map2f( GLenum target,
                       GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                       GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
                       const GLfloat *points )
{
   GLfloat *pnts;
   GLboolean retain;
   GET_CONTEXT;

   pnts = gl_copy_map_points2f( target, ustride, uorder,
                                vstride, vorder, points );
   retain = CC->CompileFlag;
   (*CC->API.Map2f)( CC, target, u1, u2, ustride, uorder,
                     v1, v2, vstride, vorder, pnts, retain );
}


void APIENTRY _mesa_MapGrid1d( GLint un, GLdouble u1, GLdouble u2 )
{
   GET_CONTEXT;
          (*CC->API.MapGrid1f)( CC, un, (GLfloat) u1, (GLfloat) u2 );
}


void APIENTRY _mesa_MapGrid1f( GLint un, GLfloat u1, GLfloat u2 )
{
   GET_CONTEXT;
          (*CC->API.MapGrid1f)( CC, un, u1, u2 );
}


void APIENTRY _mesa_MapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                           GLint vn, GLdouble v1, GLdouble v2 )
{
   GET_CONTEXT;
          (*CC->API.MapGrid2f)( CC, un, (GLfloat) u1, (GLfloat) u2,
                         vn, (GLfloat) v1, (GLfloat) v2 );
}


void APIENTRY _mesa_MapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                           GLint vn, GLfloat v1, GLfloat v2 )
{
   GET_CONTEXT;
          (*CC->API.MapGrid2f)( CC, un, u1, u2, vn, v1, v2 );
}


void APIENTRY _mesa_Materialf( GLenum face, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.Materialfv)( CC, face, pname, &param );
}



void APIENTRY _mesa_Materiali( GLenum face, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   GET_CONTEXT;
          fparam[0] = (GLfloat) param;
   (*CC->API.Materialfv)( CC, face, pname, fparam );
}


void APIENTRY _mesa_Materialfv( GLenum face, GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.Materialfv)( CC, face, pname, params );
}


void APIENTRY _mesa_Materialiv( GLenum face, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   GET_CONTEXT;
          switch (pname) {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_EMISSION:
      case GL_AMBIENT_AND_DIFFUSE:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_SHININESS:
         fparam[0] = (GLfloat) params[0];
         break;
      case GL_COLOR_INDEXES:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         break;
      default:
         /* Error will be caught later in gl_Materialfv */
         ;
   }
   (*CC->API.Materialfv)( CC, face, pname, fparam );
}


void APIENTRY _mesa_MatrixMode( GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.MatrixMode)( CC, mode );
}


void APIENTRY _mesa_MultMatrixd( const GLdouble *m )
{
   GLfloat fm[16];
   GLuint i;
   GET_CONTEXT;

   for (i=0;i<16;i++) {
      fm[i] = (GLfloat) m[i];
   }

   (*CC->API.MultMatrixf)( CC, fm );
}


void APIENTRY _mesa_MultMatrixf( const GLfloat *m )
{
   GET_CONTEXT;
          (*CC->API.MultMatrixf)( CC, m );
}


void APIENTRY _mesa_NewList( GLuint list, GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.NewList)(CC, list, mode);
}

void APIENTRY _mesa_Normal3b( GLbyte nx, GLbyte ny, GLbyte nz )
{
   GET_CONTEXT;
   (*CC->API.Normal3f)( CC, BYTE_TO_FLOAT(nx),
                        BYTE_TO_FLOAT(ny), BYTE_TO_FLOAT(nz) );
}


void APIENTRY _mesa_Normal3d( GLdouble nx, GLdouble ny, GLdouble nz )
{
   GLfloat fx, fy, fz;
   GET_CONTEXT;
   if (ABSD(nx)<0.00001)   fx = 0.0F;   else  fx = nx;
   if (ABSD(ny)<0.00001)   fy = 0.0F;   else  fy = ny;
   if (ABSD(nz)<0.00001)   fz = 0.0F;   else  fz = nz;
   (*CC->API.Normal3f)( CC, fx, fy, fz );
}


void APIENTRY _mesa_Normal3f( GLfloat nx, GLfloat ny, GLfloat nz )
{
   GET_CONTEXT;
#ifdef SHORTCUT
   if (CC->CompileFlag) {
      (*CC->Save.Normal3f)( CC, nx, ny, nz );
   }
   else {
      /* Execute */
      CC->Current.Normal[0] = nx;
      CC->Current.Normal[1] = ny;
      CC->Current.Normal[2] = nz;
      CC->VB->MonoNormal = GL_FALSE;
   }
#else
   (*CC->API.Normal3f)( CC, nx, ny, nz );
#endif
}


void APIENTRY _mesa_Normal3i( GLint nx, GLint ny, GLint nz )
{
   GET_CONTEXT;
   (*CC->API.Normal3f)( CC, INT_TO_FLOAT(nx),
                        INT_TO_FLOAT(ny), INT_TO_FLOAT(nz) );
}


void APIENTRY _mesa_Normal3s( GLshort nx, GLshort ny, GLshort nz )
{
   GET_CONTEXT;
   (*CC->API.Normal3f)( CC, SHORT_TO_FLOAT(nx),
                        SHORT_TO_FLOAT(ny), SHORT_TO_FLOAT(nz) );
}


void APIENTRY _mesa_Normal3bv( const GLbyte *v )
{
   GET_CONTEXT;
   (*CC->API.Normal3f)( CC, BYTE_TO_FLOAT(v[0]),
                        BYTE_TO_FLOAT(v[1]), BYTE_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_Normal3dv( const GLdouble *v )
{
   GLfloat fx, fy, fz;
   GET_CONTEXT;
   if (ABSD(v[0])<0.00001)   fx = 0.0F;   else  fx = v[0];
   if (ABSD(v[1])<0.00001)   fy = 0.0F;   else  fy = v[1];
   if (ABSD(v[2])<0.00001)   fz = 0.0F;   else  fz = v[2];
   (*CC->API.Normal3f)( CC, fx, fy, fz );
}


void APIENTRY _mesa_Normal3fv( const GLfloat *v )
{
   GET_CONTEXT;
#ifdef SHORTCUT
   if (CC->CompileFlag) {
      (*CC->Save.Normal3fv)( CC, v );
   }
   else {
      /* Execute */
      GLfloat *n = CC->Current.Normal;
      n[0] = v[0];
      n[1] = v[1];
      n[2] = v[2];
      CC->VB->MonoNormal = GL_FALSE;
   }
#else
   (*CC->API.Normal3fv)( CC, v );
#endif
}


void APIENTRY _mesa_Normal3iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Normal3f)( CC, INT_TO_FLOAT(v[0]),
                        INT_TO_FLOAT(v[1]), INT_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_Normal3sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Normal3f)( CC, SHORT_TO_FLOAT(v[0]),
                        SHORT_TO_FLOAT(v[1]), SHORT_TO_FLOAT(v[2]) );
}


void APIENTRY _mesa_NormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr )
{
   GET_CONTEXT;
          (*CC->API.NormalPointer)(CC, type, stride, ptr);
}
void APIENTRY _mesa_Ortho( GLdouble left, GLdouble right,
                       GLdouble bottom, GLdouble top,
                       GLdouble nearval, GLdouble farval )
{
   GET_CONTEXT;
          (*CC->API.Ortho)(CC, left, right, bottom, top, nearval, farval);
}


void APIENTRY _mesa_PassThrough( GLfloat token )
{
   GET_CONTEXT;
          (*CC->API.PassThrough)(CC, token);
}


void APIENTRY _mesa_PixelMapfv( GLenum map, GLint mapsize, const GLfloat *values )
{
   GET_CONTEXT;
          (*CC->API.PixelMapfv)( CC, map, mapsize, values );
}


void APIENTRY _mesa_PixelMapuiv( GLenum map, GLint mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLuint i;
   GET_CONTEXT;

   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }
   (*CC->API.PixelMapfv)( CC, map, mapsize, fvalues );
}



void APIENTRY _mesa_PixelMapusv( GLenum map, GLint mapsize, const GLushort *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLuint i;
   GET_CONTEXT;

   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }
   (*CC->API.PixelMapfv)( CC, map, mapsize, fvalues );
}


void APIENTRY _mesa_PixelStoref( GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.PixelStorei)( CC, pname, (GLint) param );
}


void APIENTRY _mesa_PixelStorei( GLenum pname, GLint param )
{
   GET_CONTEXT;
          (*CC->API.PixelStorei)( CC, pname, param );
}


void APIENTRY _mesa_PixelTransferf( GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.PixelTransferf)(CC, pname, param);
}


void APIENTRY _mesa_PixelTransferi( GLenum pname, GLint param )
{
   GET_CONTEXT;
          (*CC->API.PixelTransferf)(CC, pname, (GLfloat) param);
}


void APIENTRY _mesa_PixelZoom( GLfloat xfactor, GLfloat yfactor )
{
   GET_CONTEXT;
          (*CC->API.PixelZoom)(CC, xfactor, yfactor);
}


void APIENTRY _mesa_PointSize( GLfloat size )
{
   GET_CONTEXT;
          (*CC->API.PointSize)(CC, size);
}


void APIENTRY _mesa_PolygonMode( GLenum face, GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.PolygonMode)(CC, face, mode);
}


void APIENTRY _mesa_PolygonOffset( GLfloat factor, GLfloat units )
{
   GET_CONTEXT;
          (*CC->API.PolygonOffset)( CC, factor, units );
}

void APIENTRY _mesa_PolygonStipple( const GLubyte *mask )
{
   GET_CONTEXT;
          (*CC->API.PolygonStipple)(CC, mask);
}


void APIENTRY _mesa_PopAttrib( void )
{
   GET_CONTEXT;
          (*CC->API.PopAttrib)(CC);
}


void APIENTRY _mesa_PopClientAttrib( void )
{
   GET_CONTEXT;
          (*CC->API.PopClientAttrib)(CC);
}


void APIENTRY _mesa_PopMatrix( void )
{
   GET_CONTEXT;
          (*CC->API.PopMatrix)( CC );
}


void APIENTRY _mesa_PopName( void )
{
   GET_CONTEXT;
          (*CC->API.PopName)(CC);
}


void APIENTRY _mesa_PrioritizeTextures( GLsizei n, const GLuint *textures,
                                    const GLclampf *priorities )
{
   GET_CONTEXT;
          (*CC->API.PrioritizeTextures)(CC, n, textures, priorities);
}


void APIENTRY _mesa_PushMatrix( void )
{
   GET_CONTEXT;
          (*CC->API.PushMatrix)( CC );
}


void APIENTRY _mesa_RasterPos2d( GLdouble x, GLdouble y )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos2f( GLfloat x, GLfloat y )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos2i( GLint x, GLint y )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos2s( GLshort x, GLshort y )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos3d( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void APIENTRY _mesa_RasterPos3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void APIENTRY _mesa_RasterPos3i( GLint x, GLint y, GLint z )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void APIENTRY _mesa_RasterPos3s( GLshort x, GLshort y, GLshort z )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void APIENTRY _mesa_RasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y,
                               (GLfloat) z, (GLfloat) w );
}


void APIENTRY _mesa_RasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, x, y, z, w );
}


void APIENTRY _mesa_RasterPos4i( GLint x, GLint y, GLint z, GLint w )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y,
                           (GLfloat) z, (GLfloat) w );
}


void APIENTRY _mesa_RasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y,
                           (GLfloat) z, (GLfloat) w );
}


void APIENTRY _mesa_RasterPos2dv( const GLdouble *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos2fv( const GLfloat *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos2iv( const GLint *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


void APIENTRY _mesa_RasterPos2sv( const GLshort *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


/*** 3 element vector ***/

void APIENTRY _mesa_RasterPos3dv( const GLdouble *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], 1.0F );
}


void APIENTRY _mesa_RasterPos3fv( const GLfloat *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], 1.0F );
}


void APIENTRY _mesa_RasterPos3iv( const GLint *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], 1.0F );
}


void APIENTRY _mesa_RasterPos3sv( const GLshort *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], 1.0F );
}


void APIENTRY _mesa_RasterPos4dv( const GLdouble *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_RasterPos4fv( const GLfloat *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, v[0], v[1], v[2], v[3] );
}


void APIENTRY _mesa_RasterPos4iv( const GLint *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_RasterPos4sv( const GLshort *v )
{
   GET_CONTEXT;
          (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_ReadBuffer( GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.ReadBuffer)( CC, mode );
}


void APIENTRY _mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
           GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CONTEXT;
          (*CC->API.ReadPixels)( CC, x, y, width, height, format, type, pixels );
}


void APIENTRY _mesa_Rectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)( CC, (GLfloat) x1, (GLfloat) y1,
                     (GLfloat) x2, (GLfloat) y2 );
}


void APIENTRY _mesa_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)( CC, x1, y1, x2, y2 );
}


void APIENTRY _mesa_Recti( GLint x1, GLint y1, GLint x2, GLint y2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)( CC, (GLfloat) x1, (GLfloat) y1,
                         (GLfloat) x2, (GLfloat) y2 );
}


void APIENTRY _mesa_Rects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)( CC, (GLfloat) x1, (GLfloat) y1,
                     (GLfloat) x2, (GLfloat) y2 );
}


void APIENTRY _mesa_Rectdv( const GLdouble *v1, const GLdouble *v2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)(CC, (GLfloat) v1[0], (GLfloat) v1[1],
                    (GLfloat) v2[0], (GLfloat) v2[1]);
}


void APIENTRY _mesa_Rectfv( const GLfloat *v1, const GLfloat *v2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)(CC, v1[0], v1[1], v2[0], v2[1]);
}


void APIENTRY _mesa_Rectiv( const GLint *v1, const GLint *v2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)( CC, (GLfloat) v1[0], (GLfloat) v1[1],
                     (GLfloat) v2[0], (GLfloat) v2[1] );
}


void APIENTRY _mesa_Rectsv( const GLshort *v1, const GLshort *v2 )
{
   GET_CONTEXT;
          (*CC->API.Rectf)(CC, (GLfloat) v1[0], (GLfloat) v1[1],
        (GLfloat) v2[0], (GLfloat) v2[1]);
}


void APIENTRY _mesa_Scissor( GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CONTEXT;
          (*CC->API.Scissor)(CC, x, y, width, height);
}


GLboolean APIENTRY _mesa_IsEnabled( GLenum cap )
{
   GET_CONTEXT;
          return (*CC->API.IsEnabled)( CC, cap );
}



void APIENTRY _mesa_PushAttrib( GLbitfield mask )
{
   GET_CONTEXT;
          (*CC->API.PushAttrib)(CC, mask);
}


void APIENTRY _mesa_PushClientAttrib( GLbitfield mask )
{
   GET_CONTEXT;
          (*CC->API.PushClientAttrib)(CC, mask);
}


void APIENTRY _mesa_PushName( GLuint name )
{
   GET_CONTEXT;
          (*CC->API.PushName)(CC, name);
}


GLint APIENTRY _mesa_RenderMode( GLenum mode )
{
   GET_CONTEXT;
          return (*CC->API.RenderMode)(CC, mode);
}


void APIENTRY _mesa_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
          (*CC->API.Rotatef)( CC, (GLfloat) angle,
                       (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void APIENTRY _mesa_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
          (*CC->API.Rotatef)( CC, angle, x, y, z );
}


void APIENTRY _mesa_SelectBuffer( GLsizei size, GLuint *buffer )
{
   GET_CONTEXT;
          (*CC->API.SelectBuffer)(CC, size, buffer);
}


void APIENTRY _mesa_Scaled( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
          (*CC->API.Scalef)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void APIENTRY _mesa_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
          (*CC->API.Scalef)( CC, x, y, z );
}


void APIENTRY _mesa_ShadeModel( GLenum mode )
{
   GET_CONTEXT;
          (*CC->API.ShadeModel)(CC, mode);
}


void APIENTRY _mesa_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CONTEXT;
          (*CC->API.StencilFunc)(CC, func, ref, mask);
}


void APIENTRY _mesa_StencilMask( GLuint mask )
{
   GET_CONTEXT;
          (*CC->API.StencilMask)(CC, mask);
}


void APIENTRY _mesa_StencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
   GET_CONTEXT;
          (*CC->API.StencilOp)(CC, fail, zfail, zpass);
}


void APIENTRY _mesa_TexCoord1d( GLdouble s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord1f( GLfloat s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, s, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord1i( GLint s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord1s( GLshort s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord2d( GLdouble s, GLdouble t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) s, (GLfloat) t );
}


void APIENTRY _mesa_TexCoord2f( GLfloat s, GLfloat t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, s, t );
}


void APIENTRY _mesa_TexCoord2i( GLint s, GLint t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) s, (GLfloat) t );
}


void APIENTRY _mesa_TexCoord2s( GLshort s, GLshort t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) s, (GLfloat) t );
}


void APIENTRY _mesa_TexCoord3d( GLdouble s, GLdouble t, GLdouble r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t, (GLfloat) r, 1.0 );
}


void APIENTRY _mesa_TexCoord3f( GLfloat s, GLfloat t, GLfloat r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, s, t, r, 1.0 );
}


void APIENTRY _mesa_TexCoord3i( GLint s, GLint t, GLint r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, 1.0 );
}


void APIENTRY _mesa_TexCoord3s( GLshort s, GLshort t, GLshort r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, 1.0 );
}


void APIENTRY _mesa_TexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, (GLfloat) q );
}


void APIENTRY _mesa_TexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, s, t, r, q );
}


void APIENTRY _mesa_TexCoord4i( GLint s, GLint t, GLint r, GLint q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, (GLfloat) q );
}


void APIENTRY _mesa_TexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, (GLfloat) q );
}


void APIENTRY _mesa_TexCoord1dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) *v, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord1fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, *v, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord1iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, *v, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord1sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) *v, 0.0, 0.0, 1.0 );
}


void APIENTRY _mesa_TexCoord2dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void APIENTRY _mesa_TexCoord2fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, v[0], v[1] );
}


void APIENTRY _mesa_TexCoord2iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void APIENTRY _mesa_TexCoord2sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void APIENTRY _mesa_TexCoord3dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], 1.0 );
}


void APIENTRY _mesa_TexCoord3fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, v[0], v[1], v[2], 1.0 );
}


void APIENTRY _mesa_TexCoord3iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                          (GLfloat) v[2], 1.0 );
}


void APIENTRY _mesa_TexCoord3sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], 1.0 );
}


void APIENTRY _mesa_TexCoord4dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_TexCoord4fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, v[0], v[1], v[2], v[3] );
}


void APIENTRY _mesa_TexCoord4iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_TexCoord4sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_TexCoordPointer( GLint size, GLenum type, GLsizei stride,
                        const GLvoid *ptr )
{
   GET_CONTEXT;
          (*CC->API.TexCoordPointer)(CC, size, type, stride, ptr);
}


void APIENTRY _mesa_TexGend( GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   GET_CONTEXT;
          (*CC->API.TexGenfv)( CC, coord, pname, &p );
}


void APIENTRY _mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.TexGenfv)( CC, coord, pname, &param );
}


void APIENTRY _mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   GLfloat p = (GLfloat) param;
   GET_CONTEXT;
          (*CC->API.TexGenfv)( CC, coord, pname, &p );
}


void APIENTRY _mesa_TexGendv( GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   GET_CONTEXT;
          p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
   (*CC->API.TexGenfv)( CC, coord, pname, p );
}


void APIENTRY _mesa_TexGeniv( GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   GET_CONTEXT;
          p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
   (*CC->API.TexGenfv)( CC, coord, pname, p );
}


void APIENTRY _mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.TexGenfv)( CC, coord, pname, params );
}




void APIENTRY _mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.TexEnvfv)( CC, target, pname, &param );
}



void APIENTRY _mesa_TexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   GET_CONTEXT;
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
          (*CC->API.TexEnvfv)( CC, target, pname, p );
}



void APIENTRY _mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   GET_CONTEXT;
          (*CC->API.TexEnvfv)( CC, target, pname, param );
}



void APIENTRY _mesa_TexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   GET_CONTEXT;
   p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
   p[3] = INT_TO_FLOAT( param[3] );
          (*CC->API.TexEnvfv)( CC, target, pname, p );
}


void APIENTRY _mesa_TexImage1D( GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLint border,
                            GLenum format, GLenum type, const GLvoid *pixels )
{
   struct gl_image *teximage;
   GET_CONTEXT;
          teximage = gl_unpack_image( CC, width, 1, format, type, pixels );
   (*CC->API.TexImage1D)( CC, target, level, internalformat,
                          width, border, format, type, teximage );
}



void APIENTRY _mesa_TexImage2D( GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type, const GLvoid *pixels )
{
  struct gl_image *teximage;

  GET_CONTEXT;

  teximage = gl_unpack_image( CC, width, height, format, type, pixels );
  (*CC->API.TexImage2D)( CC, target, level, internalformat,
             width, height, border, format, type, teximage );
}


void APIENTRY _mesa_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
          (*CC->API.TexParameterfv)( CC, target, pname, &param );
}


void APIENTRY _mesa_TexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   GET_CONTEXT;
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
          (*CC->API.TexParameterfv)( CC, target, pname, fparam );
}


void APIENTRY _mesa_TexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
          (*CC->API.TexParameterfv)( CC, target, pname, params );
}


void APIENTRY _mesa_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   GET_CONTEXT;
          if (pname==GL_TEXTURE_BORDER_COLOR) {
      p[0] = INT_TO_FLOAT( params[0] );
      p[1] = INT_TO_FLOAT( params[1] );
      p[2] = INT_TO_FLOAT( params[2] );
      p[3] = INT_TO_FLOAT( params[3] );
   }
   else {
      p[0] = (GLfloat) params[0];
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
      p[3] = (GLfloat) params[3];
   }
   (*CC->API.TexParameterfv)( CC, target, pname, p );
}


void APIENTRY _mesa_TexSubImage1D( GLenum target, GLint level, GLint xoffset,
                               GLsizei width, GLenum format,
                               GLenum type, const GLvoid *pixels )
{
   struct gl_image *image;
   GET_CONTEXT;
   image = gl_unpack_texsubimage( CC, width, 1, format, type, pixels );
   (*CC->API.TexSubImage1D)( CC, target, level, xoffset, width,
                             format, type, image );
}


void APIENTRY _mesa_TexSubImage2D( GLenum target, GLint level,
                               GLint xoffset, GLint yoffset,
                               GLsizei width, GLsizei height,
                               GLenum format, GLenum type,
                               const GLvoid *pixels )
{
   struct gl_image *image;
   GET_CONTEXT;
   image = gl_unpack_texsubimage( CC, width, height, format, type, pixels );
   (*CC->API.TexSubImage2D)( CC, target, level, xoffset, yoffset,
                             width, height, format, type, image );
}


void APIENTRY _mesa_Translated( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   (*CC->API.Translatef)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void APIENTRY _mesa_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   (*CC->API.Translatef)( CC, x, y, z );
}


void APIENTRY _mesa_Vertex2d( GLdouble x, GLdouble y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) x, (GLfloat) y );
}


void APIENTRY _mesa_Vertex2f( GLfloat x, GLfloat y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, x, y );
}


void APIENTRY _mesa_Vertex2i( GLint x, GLint y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) x, (GLfloat) y );
}


void APIENTRY _mesa_Vertex2s( GLshort x, GLshort y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) x, (GLfloat) y );
}


void APIENTRY _mesa_Vertex3d( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void APIENTRY _mesa_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, x, y, z );
}


void APIENTRY _mesa_Vertex3i( GLint x, GLint y, GLint z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void APIENTRY _mesa_Vertex3s( GLshort x, GLshort y, GLshort z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void APIENTRY _mesa_Vertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) x, (GLfloat) y,
                            (GLfloat) z, (GLfloat) w );
}


void APIENTRY _mesa_Vertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, x, y, z, w );
}


void APIENTRY _mesa_Vertex4i( GLint x, GLint y, GLint z, GLint w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) x, (GLfloat) y,
                            (GLfloat) z, (GLfloat) w );
}


void APIENTRY _mesa_Vertex4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) x, (GLfloat) y,
                            (GLfloat) z, (GLfloat) w );
}


void APIENTRY _mesa_Vertex2dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void APIENTRY _mesa_Vertex2fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, v[0], v[1] );
}


void APIENTRY _mesa_Vertex2iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void APIENTRY _mesa_Vertex2sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void APIENTRY _mesa_Vertex3dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}


void APIENTRY _mesa_Vertex3fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3fv)( CC, v );
}


void APIENTRY _mesa_Vertex3iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}


void APIENTRY _mesa_Vertex3sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}


void APIENTRY _mesa_Vertex4dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                            (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_Vertex4fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, v[0], v[1], v[2], v[3] );
}


void APIENTRY _mesa_Vertex4iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                            (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_Vertex4sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                            (GLfloat) v[2], (GLfloat) v[3] );
}


void APIENTRY _mesa_VertexPointer( GLint size, GLenum type, GLsizei stride,
                               const GLvoid *ptr )
{
   GET_CONTEXT;
   (*CC->API.VertexPointer)(CC, size, type, stride, ptr);
}


void APIENTRY _mesa_Viewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CONTEXT;
   (*CC->API.Viewport)( CC, x, y, width, height );
}

/* GL_EXT_paletted_texture */

void APIENTRY _mesa_ColorTableEXT( GLenum target, GLenum internalFormat,
                               GLsizei width, GLenum format, GLenum type,
                               const GLvoid *table )
{
   struct gl_image *image;
   GET_CONTEXT;
   image = gl_unpack_image( CC, width, 1, format, type, table );
   (*CC->API.ColorTable)( CC, target, internalFormat, image );
   if (image->RefCount == 0)
      gl_free_image(image);
}


void APIENTRY _mesa_ColorSubTableEXT( GLenum target, GLsizei start, GLsizei count,
                                  GLenum format, GLenum type,
                                  const GLvoid *data )
{
   struct gl_image *image;
   GET_CONTEXT;
   image = gl_unpack_image( CC, count, 1, format, type, data );
   (*CC->API.ColorSubTable)( CC, target, start, image );
   if (image->RefCount == 0)
      gl_free_image(image);
}

void APIENTRY _mesa_GetColorTableEXT( GLenum target, GLenum format,
                                  GLenum type, GLvoid *table )
{
   GET_CONTEXT;
   (*CC->API.GetColorTable)(CC, target, format, type, table);
}


void APIENTRY _mesa_GetColorTableParameterivEXT( GLenum target, GLenum pname,
                                             GLint *params )
{
   GET_CONTEXT;
   (*CC->API.GetColorTableParameteriv)(CC, target, pname, params);
}


void APIENTRY _mesa_GetColorTableParameterfvEXT( GLenum target, GLenum pname,
                                             GLfloat *params )
{
   GLint iparams;
   _mesa_GetColorTableParameterivEXT( target, pname, &iparams );
   *params = (GLfloat) iparams;
}
/* End GL_EXT_paletted_texture */
