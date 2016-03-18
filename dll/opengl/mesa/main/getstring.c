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

#include <precomp.h>

/**
 * Query string-valued state.  The return value should _not_ be freed by
 * the caller.
 *
 * \param name  the state variable to query.
 *
 * \sa glGetString().
 *
 * Tries to get the string from dd_function_table::GetString, otherwise returns
 * the hardcoded strings.
 */
const GLubyte * GLAPIENTRY
_mesa_GetString( GLenum name )
{
   GET_CURRENT_CONTEXT(ctx);
   static const char *vendor = "Brian Paul";
   static const char *renderer = "Mesa";

   if (!ctx)
      return NULL;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   /* this is a required driver function */
   assert(ctx->Driver.GetString);
   {
      /* Give the driver the chance to handle this query */
      const GLubyte *str = (*ctx->Driver.GetString)(ctx, name);
      if (str)
         return str;
   }

   switch (name) {
      case GL_VENDOR:
         return (const GLubyte *) vendor;
      case GL_RENDERER:
         return (const GLubyte *) renderer;
      case GL_VERSION:
         return (const GLubyte *) ctx->VersionString;
      case GL_EXTENSIONS:
         return (const GLubyte *) ctx->Extensions.String;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetString" );
         return (const GLubyte *) 0;
   }
}



/**
 * Return pointer-valued state, such as a vertex array pointer.
 *
 * \param pname  names state to be queried
 * \param params  returns the pointer value
 *
 * \sa glGetPointerv().
 *
 * Tries to get the specified pointer via dd_function_table::GetPointerv,
 * otherwise gets the specified pointer from the current context.
 */
void GLAPIENTRY
_mesa_GetPointerv( GLenum pname, GLvoid **params )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!params)
      return;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetPointerv %s\n", _mesa_lookup_enum_by_nr(pname));

   switch (pname) {
      case GL_VERTEX_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_POS].Ptr;
         break;
      case GL_NORMAL_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_NORMAL].Ptr;
         break;
      case GL_COLOR_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR].Ptr;
         break;
      case GL_FOG_COORDINATE_ARRAY_POINTER_EXT:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_FOG].Ptr;
         break;
      case GL_INDEX_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Ptr;
         break;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_TEX].Ptr;
         break;
      case GL_EDGE_FLAG_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.VertexAttrib[VERT_ATTRIB_EDGEFLAG].Ptr;
         break;
      case GL_FEEDBACK_BUFFER_POINTER:
         *params = ctx->Feedback.Buffer;
         break;
      case GL_SELECTION_BUFFER_POINTER:
         *params = ctx->Select.Buffer;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetPointerv" );
         return;
   }
}


/**
 * Returns the current GL error code, or GL_NO_ERROR.
 * \return current error code
 *
 * Returns __struct gl_contextRec::ErrorValue.
 */
GLenum GLAPIENTRY
_mesa_GetError( void )
{
   GET_CURRENT_CONTEXT(ctx);
   GLenum e = ctx->ErrorValue;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetError <-- %s\n", _mesa_lookup_enum_by_nr(e));

   ctx->ErrorValue = (GLenum) GL_NO_ERROR;
   ctx->ErrorDebugCount = 0;
   return e;
}
