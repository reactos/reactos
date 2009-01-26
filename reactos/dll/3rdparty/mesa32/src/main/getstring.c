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



#include "glheader.h"
#include "context.h"
#include "get.h"
#include "version.h"
#include "enums.h"
#include "extensions.h"


/**
 * Examine enabled GL extensions to determine GL version.
 * \return version string
 */
static const char *
compute_version(const GLcontext *ctx)
{
   static const char *version_1_2 = "1.2 Mesa " MESA_VERSION_STRING;
   static const char *version_1_3 = "1.3 Mesa " MESA_VERSION_STRING;
   static const char *version_1_4 = "1.4 Mesa " MESA_VERSION_STRING;
   static const char *version_1_5 = "1.5 Mesa " MESA_VERSION_STRING;
   static const char *version_2_0 = "2.0 Mesa " MESA_VERSION_STRING;
   static const char *version_2_1 = "2.1 Mesa " MESA_VERSION_STRING;

   const GLboolean ver_1_3 = (ctx->Extensions.ARB_multisample &&
                              ctx->Extensions.ARB_multitexture &&
                              ctx->Extensions.ARB_texture_border_clamp &&
                              ctx->Extensions.ARB_texture_compression &&
                              ctx->Extensions.ARB_texture_cube_map &&
                              ctx->Extensions.EXT_texture_env_add &&
                              ctx->Extensions.ARB_texture_env_combine &&
                              ctx->Extensions.ARB_texture_env_dot3);
   const GLboolean ver_1_4 = (ver_1_3 &&
                              ctx->Extensions.ARB_depth_texture &&
                              ctx->Extensions.ARB_shadow &&
                              ctx->Extensions.ARB_texture_env_crossbar &&
                              ctx->Extensions.ARB_texture_mirrored_repeat &&
                              ctx->Extensions.ARB_window_pos &&
                              ctx->Extensions.EXT_blend_color &&
                              ctx->Extensions.EXT_blend_func_separate &&
                              ctx->Extensions.EXT_blend_minmax &&
                              ctx->Extensions.EXT_blend_subtract &&
                              ctx->Extensions.EXT_fog_coord &&
                              ctx->Extensions.EXT_multi_draw_arrays &&
                              ctx->Extensions.EXT_point_parameters &&
                              ctx->Extensions.EXT_secondary_color &&
                              ctx->Extensions.EXT_stencil_wrap &&
                              ctx->Extensions.EXT_texture_lod_bias &&
                              ctx->Extensions.SGIS_generate_mipmap);
   const GLboolean ver_1_5 = (ver_1_4 &&
                              ctx->Extensions.ARB_occlusion_query &&
                              ctx->Extensions.ARB_vertex_buffer_object &&
                              ctx->Extensions.EXT_shadow_funcs);
   const GLboolean ver_2_0 = (ver_1_5 &&
                              ctx->Extensions.ARB_draw_buffers &&
                              ctx->Extensions.ARB_point_sprite &&
                              ctx->Extensions.ARB_shader_objects &&
                              ctx->Extensions.ARB_vertex_shader &&
                              ctx->Extensions.ARB_fragment_shader &&
                              ctx->Extensions.ARB_texture_non_power_of_two &&
                              ctx->Extensions.EXT_blend_equation_separate);
   const GLboolean ver_2_1 = (ver_2_0 &&
                              /*ctx->Extensions.ARB_shading_language_120 &&*/
                              ctx->Extensions.EXT_pixel_buffer_object &&
                              ctx->Extensions.EXT_texture_sRGB);
   if (ver_2_1)
      return version_2_1;
   if (ver_2_0)
      return version_2_0;
   if (ver_1_5)
      return version_1_5;
   if (ver_1_4)
      return version_1_4;
   if (ver_1_3)
      return version_1_3;
   return version_1_2;
}



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
         return (const GLubyte *) compute_version(ctx);
      case GL_EXTENSIONS:
         if (!ctx->Extensions.String)
            ctx->Extensions.String = _mesa_make_extension_string(ctx);
         return (const GLubyte *) ctx->Extensions.String;
#if FEATURE_ARB_shading_language_100
      case GL_SHADING_LANGUAGE_VERSION_ARB:
         if (ctx->Extensions.ARB_shading_language_120)
            return (const GLubyte *) "1.20";
         else if (ctx->Extensions.ARB_shading_language_100)
            return (const GLubyte *) "1.10";
         goto error;
#endif
#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program || \
    FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
      case GL_PROGRAM_ERROR_STRING_NV:
         if (ctx->Extensions.NV_fragment_program ||
             ctx->Extensions.ARB_fragment_program ||
             ctx->Extensions.NV_vertex_program ||
             ctx->Extensions.ARB_vertex_program) {
            return (const GLubyte *) ctx->Program.ErrorString;
         }
         /* FALL-THROUGH */
#endif
#if FEATURE_ARB_shading_language_100
      error:
#endif
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
   const GLuint clientUnit = ctx->Array.ActiveTexture;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!params)
      return;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetPointerv %s\n", _mesa_lookup_enum_by_nr(pname));

   if (ctx->Driver.GetPointerv
       && (*ctx->Driver.GetPointerv)(ctx, pname, params))
      return;

   switch (pname) {
      case GL_VERTEX_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.ArrayObj->Vertex.Ptr;
         break;
      case GL_NORMAL_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.ArrayObj->Normal.Ptr;
         break;
      case GL_COLOR_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.ArrayObj->Color.Ptr;
         break;
      case GL_SECONDARY_COLOR_ARRAY_POINTER_EXT:
         *params = (GLvoid *) ctx->Array.ArrayObj->SecondaryColor.Ptr;
         break;
      case GL_FOG_COORDINATE_ARRAY_POINTER_EXT:
         *params = (GLvoid *) ctx->Array.ArrayObj->FogCoord.Ptr;
         break;
      case GL_INDEX_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.ArrayObj->Index.Ptr;
         break;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.ArrayObj->TexCoord[clientUnit].Ptr;
         break;
      case GL_EDGE_FLAG_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.ArrayObj->EdgeFlag.Ptr;
         break;
      case GL_FEEDBACK_BUFFER_POINTER:
         *params = ctx->Feedback.Buffer;
         break;
      case GL_SELECTION_BUFFER_POINTER:
         *params = ctx->Select.Buffer;
         break;
#if FEATURE_MESA_program_debug
      case GL_FRAGMENT_PROGRAM_CALLBACK_FUNC_MESA:
         if (!ctx->Extensions.MESA_program_debug) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetPointerv");
            return;
         }
         *params = *(GLvoid **) &ctx->FragmentProgram.Callback;
         break;
      case GL_FRAGMENT_PROGRAM_CALLBACK_DATA_MESA:
         if (!ctx->Extensions.MESA_program_debug) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetPointerv");
            return;
         }
         *params = ctx->FragmentProgram.CallbackData;
         break;
      case GL_VERTEX_PROGRAM_CALLBACK_FUNC_MESA:
         if (!ctx->Extensions.MESA_program_debug) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetPointerv");
            return;
         }
         *params = *(GLvoid **) &ctx->VertexProgram.Callback;
         break;
      case GL_VERTEX_PROGRAM_CALLBACK_DATA_MESA:
         if (!ctx->Extensions.MESA_program_debug) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetPointerv");
            return;
         }
         *params = ctx->VertexProgram.CallbackData;
         break;
#endif
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetPointerv" );
         return;
   }
}


/**
 * Returns the current GL error code, or GL_NO_ERROR.
 * \return current error code
 *
 * Returns __GLcontextRec::ErrorValue.
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
   return e;
}
