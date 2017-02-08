/**
 * \file enable.c
 * Enable/disable/query GL capabilities.
 */

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
#include "clip.h"
#include "context.h"
#include "enable.h"
#include "light.h"
#include "simple_list.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "enums.h"
#include "api_arrayelt.h"
#include "texstate.h"



#define CHECK_EXTENSION(EXTNAME, CAP)					\
   if (!ctx->Extensions.EXTNAME) {					\
      goto invalid_enum_error;						\
   }


/**
 * Helper to enable/disable client-side state.
 */
static void
client_state(struct gl_context *ctx, GLenum cap, GLboolean state)
{
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
   GLbitfield64 flag;
   GLboolean *var;

   switch (cap) {
      case GL_VERTEX_ARRAY:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled;
         flag = VERT_BIT_POS;
         break;
      case GL_NORMAL_ARRAY:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_NORMAL].Enabled;
         flag = VERT_BIT_NORMAL;
         break;
      case GL_COLOR_ARRAY:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR0].Enabled;
         flag = VERT_BIT_COLOR0;
         break;
      case GL_INDEX_ARRAY:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled;
         flag = VERT_BIT_COLOR_INDEX;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)].Enabled;
         flag = VERT_BIT_TEX(ctx->Array.ActiveTexture);
         break;
      case GL_EDGE_FLAG_ARRAY:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled;
         flag = VERT_BIT_EDGEFLAG;
         break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_FOG].Enabled;
         flag = VERT_BIT_FOG;
         break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_COLOR1].Enabled;
         flag = VERT_BIT_COLOR1;
         break;

#if FEATURE_point_size_array
      case GL_POINT_SIZE_ARRAY_OES:
         var = &arrayObj->VertexAttrib[VERT_ATTRIB_POINT_SIZE].Enabled;
         flag = VERT_BIT_POINT_SIZE;
         break;
#endif

#if FEATURE_NV_vertex_program
      case GL_VERTEX_ATTRIB_ARRAY0_NV:
      case GL_VERTEX_ATTRIB_ARRAY1_NV:
      case GL_VERTEX_ATTRIB_ARRAY2_NV:
      case GL_VERTEX_ATTRIB_ARRAY3_NV:
      case GL_VERTEX_ATTRIB_ARRAY4_NV:
      case GL_VERTEX_ATTRIB_ARRAY5_NV:
      case GL_VERTEX_ATTRIB_ARRAY6_NV:
      case GL_VERTEX_ATTRIB_ARRAY7_NV:
      case GL_VERTEX_ATTRIB_ARRAY8_NV:
      case GL_VERTEX_ATTRIB_ARRAY9_NV:
      case GL_VERTEX_ATTRIB_ARRAY10_NV:
      case GL_VERTEX_ATTRIB_ARRAY11_NV:
      case GL_VERTEX_ATTRIB_ARRAY12_NV:
      case GL_VERTEX_ATTRIB_ARRAY13_NV:
      case GL_VERTEX_ATTRIB_ARRAY14_NV:
      case GL_VERTEX_ATTRIB_ARRAY15_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         {
            GLint n = (GLint) cap - GL_VERTEX_ATTRIB_ARRAY0_NV;
            ASSERT(VERT_ATTRIB_GENERIC(n) < Elements(ctx->Array.ArrayObj->VertexAttrib));
            var = &arrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(n)].Enabled;
            flag = VERT_BIT_GENERIC(n);
         }
         break;
#endif /* FEATURE_NV_vertex_program */

      /* GL_NV_primitive_restart */
      case GL_PRIMITIVE_RESTART_NV:
	 if (!ctx->Extensions.NV_primitive_restart) {
            goto invalid_enum_error;
         }
         var = &ctx->Array.PrimitiveRestart;
         flag = 0;
         break;

      default:
         goto invalid_enum_error;
   }

   if (*var == state)
      return;

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.NewState |= flag;

   _ae_invalidate_state(ctx, _NEW_ARRAY);

   *var = state;

   if (state)
      ctx->Array.ArrayObj->_Enabled |= flag;
   else
      ctx->Array.ArrayObj->_Enabled &= ~flag;

   if (ctx->Driver.Enable) {
      ctx->Driver.Enable( ctx, cap, state );
   }

   return;

invalid_enum_error:
   _mesa_error(ctx, GL_INVALID_ENUM, "gl%sClientState(0x%x)",
               state ? "Enable" : "Disable", cap);
}


/**
 * Enable GL capability.
 * \param cap  state to enable/disable.
 *
 * Get's the current context, assures that we're outside glBegin()/glEnd() and
 * calls client_state().
 */
void GLAPIENTRY
_mesa_EnableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   client_state( ctx, cap, GL_TRUE );
}


/**
 * Disable GL capability.
 * \param cap  state to enable/disable.
 *
 * Get's the current context, assures that we're outside glBegin()/glEnd() and
 * calls client_state().
 */
void GLAPIENTRY
_mesa_DisableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   client_state( ctx, cap, GL_FALSE );
}


#undef CHECK_EXTENSION
#define CHECK_EXTENSION(EXTNAME, CAP)					\
   if (!ctx->Extensions.EXTNAME) {					\
      goto invalid_enum_error;						\
   }

#define CHECK_EXTENSION2(EXT1, EXT2, CAP)				\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2) {		\
      goto invalid_enum_error;						\
   }



/**
 * Return pointer to current texture unit for setting/getting coordinate
 * state.
 * Note that we'll set GL_INVALID_OPERATION and return NULL if the active
 * texture unit is higher than the number of supported coordinate units.
 */
static struct gl_texture_unit *
get_texcoord_unit(struct gl_context *ctx)
{
   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEnable/Disable(texcoord unit)");
      return NULL;
   }
   else {
      return &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   }
}


/**
 * Helper function to enable or disable a texture target.
 * \param bit  one of the TEXTURE_x_BIT values
 * \return GL_TRUE if state is changing or GL_FALSE if no change
 */
static GLboolean
enable_texture(struct gl_context *ctx, GLboolean state, GLbitfield texBit)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   const GLbitfield newenabled = state
      ? (texUnit->Enabled | texBit) : (texUnit->Enabled & ~texBit);

   if (texUnit->Enabled == newenabled)
       return GL_FALSE;

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   texUnit->Enabled = newenabled;
   return GL_TRUE;
}


/**
 * Helper function to enable or disable state.
 *
 * \param ctx GL context.
 * \param cap  the state to enable/disable
 * \param state whether to enable or disable the specified capability.
 *
 * Updates the current context and flushes the vertices as needed. For
 * capabilities associated with extensions it verifies that those extensions
 * are effectivly present before updating. Notifies the driver via
 * dd_function_table::Enable.
 */
void
_mesa_set_enable(struct gl_context *ctx, GLenum cap, GLboolean state)
{
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "%s %s (newstate is %x)\n",
                  state ? "glEnable" : "glDisable",
                  _mesa_lookup_enum_by_nr(cap),
                  ctx->NewState);

   switch (cap) {
      case GL_ALPHA_TEST:
         if (ctx->Color.AlphaEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.AlphaEnabled = state;
         break;
      case GL_AUTO_NORMAL:
         if (ctx->Eval.AutoNormal == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.AutoNormal = state;
         break;
      case GL_BLEND:
         {
            GLbitfield newEnabled =
               state * ((1 << ctx->Const.MaxDrawBuffers) - 1);
            if (newEnabled != ctx->Color.BlendEnabled) {
               FLUSH_VERTICES(ctx, _NEW_COLOR);
               ctx->Color.BlendEnabled = newEnabled;
            }
         }
         break;
#if FEATURE_userclip
      case GL_CLIP_DISTANCE0:
      case GL_CLIP_DISTANCE1:
      case GL_CLIP_DISTANCE2:
      case GL_CLIP_DISTANCE3:
      case GL_CLIP_DISTANCE4:
      case GL_CLIP_DISTANCE5:
      case GL_CLIP_DISTANCE6:
      case GL_CLIP_DISTANCE7:
         {
            const GLuint p = cap - GL_CLIP_DISTANCE0;

            if (p >= ctx->Const.MaxClipPlanes)
               goto invalid_enum_error;

            if ((ctx->Transform.ClipPlanesEnabled & (1 << p))
                == ((GLuint) state << p))
               return;

            FLUSH_VERTICES(ctx, _NEW_TRANSFORM);

            if (state) {
               ctx->Transform.ClipPlanesEnabled |= (1 << p);
               _mesa_update_clip_plane(ctx, p);
            }
            else {
               ctx->Transform.ClipPlanesEnabled &= ~(1 << p);
            }               
         }
         break;
#endif
      case GL_COLOR_MATERIAL:
         if (ctx->Light.ColorMaterialEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT);
         FLUSH_CURRENT(ctx, 0);
         ctx->Light.ColorMaterialEnabled = state;
         if (state) {
            _mesa_update_color_material( ctx,
                                  ctx->Current.Attrib[VERT_ATTRIB_COLOR0] );
         }
         break;
      case GL_CULL_FACE:
         if (ctx->Polygon.CullFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.CullFlag = state;
         break;
      case GL_DEPTH_TEST:
         if (ctx->Depth.Test == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_DEPTH);
         ctx->Depth.Test = state;
         break;
      case GL_DITHER:
         if (ctx->Color.DitherFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.DitherFlag = state;
         break;
      case GL_FOG:
         if (ctx->Fog.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.Enabled = state;
         break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
         if (ctx->Light.Light[cap-GL_LIGHT0].Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT);
         ctx->Light.Light[cap-GL_LIGHT0].Enabled = state;
         if (state) {
            insert_at_tail(&ctx->Light.EnabledList,
                           &ctx->Light.Light[cap-GL_LIGHT0]);
         }
         else {
            remove_from_list(&ctx->Light.Light[cap-GL_LIGHT0]);
         }
         break;
      case GL_LIGHTING:
         if (ctx->Light.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT);
         ctx->Light.Enabled = state;
         if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
            ctx->_TriangleCaps |= DD_TRI_LIGHT_TWOSIDE;
         else
            ctx->_TriangleCaps &= ~DD_TRI_LIGHT_TWOSIDE;
         break;
      case GL_LINE_SMOOTH:
         if (ctx->Line.SmoothFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LINE);
         ctx->Line.SmoothFlag = state;
         ctx->_TriangleCaps ^= DD_LINE_SMOOTH;
         break;
      case GL_LINE_STIPPLE:
         if (ctx->Line.StippleFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LINE);
         ctx->Line.StippleFlag = state;
         ctx->_TriangleCaps ^= DD_LINE_STIPPLE;
         break;
      case GL_INDEX_LOGIC_OP:
         if (ctx->Color.IndexLogicOpEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.IndexLogicOpEnabled = state;
         break;
      case GL_COLOR_LOGIC_OP:
         if (ctx->Color.ColorLogicOpEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.ColorLogicOpEnabled = state;
         break;
      case GL_MAP1_COLOR_4:
         if (ctx->Eval.Map1Color4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Color4 = state;
         break;
      case GL_MAP1_INDEX:
         if (ctx->Eval.Map1Index == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Index = state;
         break;
      case GL_MAP1_NORMAL:
         if (ctx->Eval.Map1Normal == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Normal = state;
         break;
      case GL_MAP1_TEXTURE_COORD_1:
         if (ctx->Eval.Map1TextureCoord1 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord1 = state;
         break;
      case GL_MAP1_TEXTURE_COORD_2:
         if (ctx->Eval.Map1TextureCoord2 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord2 = state;
         break;
      case GL_MAP1_TEXTURE_COORD_3:
         if (ctx->Eval.Map1TextureCoord3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord3 = state;
         break;
      case GL_MAP1_TEXTURE_COORD_4:
         if (ctx->Eval.Map1TextureCoord4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord4 = state;
         break;
      case GL_MAP1_VERTEX_3:
         if (ctx->Eval.Map1Vertex3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Vertex3 = state;
         break;
      case GL_MAP1_VERTEX_4:
         if (ctx->Eval.Map1Vertex4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Vertex4 = state;
         break;
      case GL_MAP2_COLOR_4:
         if (ctx->Eval.Map2Color4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Color4 = state;
         break;
      case GL_MAP2_INDEX:
         if (ctx->Eval.Map2Index == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Index = state;
         break;
      case GL_MAP2_NORMAL:
         if (ctx->Eval.Map2Normal == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Normal = state;
         break;
      case GL_MAP2_TEXTURE_COORD_1:
         if (ctx->Eval.Map2TextureCoord1 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord1 = state;
         break;
      case GL_MAP2_TEXTURE_COORD_2:
         if (ctx->Eval.Map2TextureCoord2 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord2 = state;
         break;
      case GL_MAP2_TEXTURE_COORD_3:
         if (ctx->Eval.Map2TextureCoord3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord3 = state;
         break;
      case GL_MAP2_TEXTURE_COORD_4:
         if (ctx->Eval.Map2TextureCoord4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord4 = state;
         break;
      case GL_MAP2_VERTEX_3:
         if (ctx->Eval.Map2Vertex3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Vertex3 = state;
         break;
      case GL_MAP2_VERTEX_4:
         if (ctx->Eval.Map2Vertex4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Vertex4 = state;
         break;
      case GL_NORMALIZE:
         if (ctx->Transform.Normalize == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         ctx->Transform.Normalize = state;
         break;
      case GL_POINT_SMOOTH:
         if (ctx->Point.SmoothFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.SmoothFlag = state;
         ctx->_TriangleCaps ^= DD_POINT_SMOOTH;
         break;
      case GL_POLYGON_SMOOTH:
         if (ctx->Polygon.SmoothFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.SmoothFlag = state;
         ctx->_TriangleCaps ^= DD_TRI_SMOOTH;
         break;
      case GL_POLYGON_STIPPLE:
         if (ctx->Polygon.StippleFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.StippleFlag = state;
         ctx->_TriangleCaps ^= DD_TRI_STIPPLE;
         break;
      case GL_POLYGON_OFFSET_POINT:
         if (ctx->Polygon.OffsetPoint == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.OffsetPoint = state;
         break;
      case GL_POLYGON_OFFSET_LINE:
         if (ctx->Polygon.OffsetLine == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.OffsetLine = state;
         break;
      case GL_POLYGON_OFFSET_FILL:
         if (ctx->Polygon.OffsetFill == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.OffsetFill = state;
         break;
      case GL_RESCALE_NORMAL_EXT:
         if (ctx->Transform.RescaleNormals == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         ctx->Transform.RescaleNormals = state;
         break;
      case GL_SCISSOR_TEST:
         if (ctx->Scissor.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_SCISSOR);
         ctx->Scissor.Enabled = state;
         break;
      case GL_STENCIL_TEST:
         if (ctx->Stencil.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_STENCIL);
         ctx->Stencil.Enabled = state;
         break;
      case GL_TEXTURE_1D:
         if (!enable_texture(ctx, state, TEXTURE_1D_BIT)) {
            return;
         }
         break;
      case GL_TEXTURE_2D:
         if (!enable_texture(ctx, state, TEXTURE_2D_BIT)) {
            return;
         }
         break;
      case GL_TEXTURE_3D:
         if (!enable_texture(ctx, state, TEXTURE_3D_BIT)) {
            return;
         }
         break;
      case GL_TEXTURE_GEN_S:
      case GL_TEXTURE_GEN_T:
      case GL_TEXTURE_GEN_R:
      case GL_TEXTURE_GEN_Q:
         {
            struct gl_texture_unit *texUnit = get_texcoord_unit(ctx);
            if (texUnit) {
               GLbitfield coordBit = S_BIT << (cap - GL_TEXTURE_GEN_S);
               GLbitfield newenabled = texUnit->TexGenEnabled & ~coordBit;
               if (state)
                  newenabled |= coordBit;
               if (texUnit->TexGenEnabled == newenabled)
                  return;
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texUnit->TexGenEnabled = newenabled;
            }
         }
         break;

#if FEATURE_ES1
      case GL_TEXTURE_GEN_STR_OES:
	 /* disable S, T, and R at the same time */
	 {
            struct gl_texture_unit *texUnit = get_texcoord_unit(ctx);
            if (texUnit) {
               GLuint newenabled =
		  texUnit->TexGenEnabled & ~STR_BITS;
               if (state)
                  newenabled |= STR_BITS;
               if (texUnit->TexGenEnabled == newenabled)
                  return;
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texUnit->TexGenEnabled = newenabled;
            }
         }
         break;
#endif

      /* client-side state */
      case GL_VERTEX_ARRAY:
      case GL_NORMAL_ARRAY:
      case GL_COLOR_ARRAY:
      case GL_INDEX_ARRAY:
      case GL_TEXTURE_COORD_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
      case GL_FOG_COORDINATE_ARRAY_EXT:
      case GL_SECONDARY_COLOR_ARRAY_EXT:
      case GL_POINT_SIZE_ARRAY_OES:
         client_state( ctx, cap, state );
         return;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION(ARB_texture_cube_map, cap);
         if (!enable_texture(ctx, state, TEXTURE_CUBE_BIT)) {
            return;
         }
         break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION2(EXT_secondary_color, ARB_vertex_program, cap);
         if (ctx->Fog.ColorSumEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.ColorSumEnabled = state;
         break;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         if (ctx->Multisample.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.Enabled = state;
         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         if (ctx->Multisample.SampleAlphaToCoverage == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleAlphaToCoverage = state;
         break;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         if (ctx->Multisample.SampleAlphaToOne == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleAlphaToOne = state;
         break;
      case GL_SAMPLE_COVERAGE_ARB:
         if (ctx->Multisample.SampleCoverage == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleCoverage = state;
         break;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         if (ctx->Multisample.SampleCoverageInvert == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleCoverageInvert = state;
         break;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION(IBM_rasterpos_clip, cap);
         if (ctx->Transform.RasterPositionUnclipped == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         ctx->Transform.RasterPositionUnclipped = state;
         break;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION2(NV_point_sprite, ARB_point_sprite, cap);
         if (ctx->Point.PointSprite == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.PointSprite = state;
         break;

#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
      case GL_VERTEX_PROGRAM_ARB:
         CHECK_EXTENSION2(ARB_vertex_program, NV_vertex_program, cap);
         if (ctx->VertexProgram.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PROGRAM); 
         ctx->VertexProgram.Enabled = state;
         break;
      case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
         CHECK_EXTENSION2(ARB_vertex_program, NV_vertex_program, cap);
         if (ctx->VertexProgram.PointSizeEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PROGRAM);
         ctx->VertexProgram.PointSizeEnabled = state;
         break;
      case GL_VERTEX_PROGRAM_TWO_SIDE_ARB:
         CHECK_EXTENSION2(ARB_vertex_program, NV_vertex_program, cap);
         if (ctx->VertexProgram.TwoSideEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PROGRAM); 
         ctx->VertexProgram.TwoSideEnabled = state;
         break;
#endif
#if FEATURE_NV_vertex_program
      case GL_MAP1_VERTEX_ATTRIB0_4_NV:
      case GL_MAP1_VERTEX_ATTRIB1_4_NV:
      case GL_MAP1_VERTEX_ATTRIB2_4_NV:
      case GL_MAP1_VERTEX_ATTRIB3_4_NV:
      case GL_MAP1_VERTEX_ATTRIB4_4_NV:
      case GL_MAP1_VERTEX_ATTRIB5_4_NV:
      case GL_MAP1_VERTEX_ATTRIB6_4_NV:
      case GL_MAP1_VERTEX_ATTRIB7_4_NV:
      case GL_MAP1_VERTEX_ATTRIB8_4_NV:
      case GL_MAP1_VERTEX_ATTRIB9_4_NV:
      case GL_MAP1_VERTEX_ATTRIB10_4_NV:
      case GL_MAP1_VERTEX_ATTRIB11_4_NV:
      case GL_MAP1_VERTEX_ATTRIB12_4_NV:
      case GL_MAP1_VERTEX_ATTRIB13_4_NV:
      case GL_MAP1_VERTEX_ATTRIB14_4_NV:
      case GL_MAP1_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         {
            const GLuint map = (GLuint) (cap - GL_MAP1_VERTEX_ATTRIB0_4_NV);
            FLUSH_VERTICES(ctx, _NEW_EVAL);
            ctx->Eval.Map1Attrib[map] = state;
         }
         break;
      case GL_MAP2_VERTEX_ATTRIB0_4_NV:
      case GL_MAP2_VERTEX_ATTRIB1_4_NV:
      case GL_MAP2_VERTEX_ATTRIB2_4_NV:
      case GL_MAP2_VERTEX_ATTRIB3_4_NV:
      case GL_MAP2_VERTEX_ATTRIB4_4_NV:
      case GL_MAP2_VERTEX_ATTRIB5_4_NV:
      case GL_MAP2_VERTEX_ATTRIB6_4_NV:
      case GL_MAP2_VERTEX_ATTRIB7_4_NV:
      case GL_MAP2_VERTEX_ATTRIB8_4_NV:
      case GL_MAP2_VERTEX_ATTRIB9_4_NV:
      case GL_MAP2_VERTEX_ATTRIB10_4_NV:
      case GL_MAP2_VERTEX_ATTRIB11_4_NV:
      case GL_MAP2_VERTEX_ATTRIB12_4_NV:
      case GL_MAP2_VERTEX_ATTRIB13_4_NV:
      case GL_MAP2_VERTEX_ATTRIB14_4_NV:
      case GL_MAP2_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         {
            const GLuint map = (GLuint) (cap - GL_MAP2_VERTEX_ATTRIB0_4_NV);
            FLUSH_VERTICES(ctx, _NEW_EVAL);
            ctx->Eval.Map2Attrib[map] = state;
         }
         break;
#endif /* FEATURE_NV_vertex_program */

#if FEATURE_NV_fragment_program
      case GL_FRAGMENT_PROGRAM_NV:
         CHECK_EXTENSION(NV_fragment_program, cap);
         if (ctx->FragmentProgram.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PROGRAM);
         ctx->FragmentProgram.Enabled = state;
         break;
#endif /* FEATURE_NV_fragment_program */

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION(NV_texture_rectangle, cap);
         if (!enable_texture(ctx, state, TEXTURE_RECT_BIT)) {
            return;
         }
         break;

      /* GL_EXT_stencil_two_side */
      case GL_STENCIL_TEST_TWO_SIDE_EXT:
         CHECK_EXTENSION(EXT_stencil_two_side, cap);
         if (ctx->Stencil.TestTwoSide == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_STENCIL);
         ctx->Stencil.TestTwoSide = state;
         if (state) {
            ctx->Stencil._BackFace = 2;
            ctx->_TriangleCaps |= DD_TRI_TWOSTENCIL;
         } else {
            ctx->Stencil._BackFace = 1;
            ctx->_TriangleCaps &= ~DD_TRI_TWOSTENCIL;
         }
         break;

#if FEATURE_ARB_fragment_program
      case GL_FRAGMENT_PROGRAM_ARB:
         CHECK_EXTENSION(ARB_fragment_program, cap);
         if (ctx->FragmentProgram.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PROGRAM);
         ctx->FragmentProgram.Enabled = state;
         break;
#endif /* FEATURE_ARB_fragment_program */

      /* GL_EXT_depth_bounds_test */
      case GL_DEPTH_BOUNDS_TEST_EXT:
         CHECK_EXTENSION(EXT_depth_bounds_test, cap);
         if (ctx->Depth.BoundsTest == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_DEPTH);
         ctx->Depth.BoundsTest = state;
         break;

      case GL_DEPTH_CLAMP:
         if (ctx->Transform.DepthClamp == state)
            return;
	 CHECK_EXTENSION(ARB_depth_clamp, cap);
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
	 ctx->Transform.DepthClamp = state;
	 break;

#if FEATURE_ATI_fragment_shader
      case GL_FRAGMENT_SHADER_ATI:
        CHECK_EXTENSION(ATI_fragment_shader, cap);
	if (ctx->ATIFragmentShader.Enabled == state)
	  return;
	FLUSH_VERTICES(ctx, _NEW_PROGRAM);
	ctx->ATIFragmentShader.Enabled = state;
        break;
#endif

      /* GL_MESA_texture_array */
      case GL_TEXTURE_1D_ARRAY_EXT:
         CHECK_EXTENSION(MESA_texture_array, cap);
         if (!enable_texture(ctx, state, TEXTURE_1D_ARRAY_BIT)) {
            return;
         }
         break;

      case GL_TEXTURE_2D_ARRAY_EXT:
         CHECK_EXTENSION(MESA_texture_array, cap);
         if (!enable_texture(ctx, state, TEXTURE_2D_ARRAY_BIT)) {
            return;
         }
         break;

      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
	 CHECK_EXTENSION(ARB_seamless_cube_map, cap);
	 if (ctx->Texture.CubeMapSeamless != state) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    ctx->Texture.CubeMapSeamless = state;
	 }
	 break;

#if FEATURE_EXT_transform_feedback
      case GL_RASTERIZER_DISCARD:
	 CHECK_EXTENSION(EXT_transform_feedback, cap);
         if (ctx->RasterDiscard != state) {
            FLUSH_VERTICES(ctx, _NEW_RASTERIZER_DISCARD);
            ctx->RasterDiscard = state;
         }
         break;
#endif

      /* GL 3.1 primitive restart.  Note: this enum is different from
       * GL_PRIMITIVE_RESTART_NV (which is client state).
       */
      case GL_PRIMITIVE_RESTART:
         if (ctx->VersionMajor * 10 + ctx->VersionMinor < 31) {
            goto invalid_enum_error;
         }
         if (ctx->Array.PrimitiveRestart != state) {
            FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
            ctx->Array.PrimitiveRestart = state;
         }
         break;

      /* GL3.0 - GL_framebuffer_sRGB */
      case GL_FRAMEBUFFER_SRGB_EXT:
         CHECK_EXTENSION(EXT_framebuffer_sRGB, cap);
         FLUSH_VERTICES(ctx, _NEW_BUFFERS);
         ctx->Color.sRGBEnabled = state;
         break;

      /* GL_OES_EGL_image_external */
      case GL_TEXTURE_EXTERNAL_OES:
         CHECK_EXTENSION(OES_EGL_image_external, cap);
         if (!enable_texture(ctx, state, TEXTURE_EXTERNAL_BIT)) {
            return;
         }
         break;

      default:
         goto invalid_enum_error;
   }

   if (ctx->Driver.Enable) {
      ctx->Driver.Enable( ctx, cap, state );
   }

   return;

invalid_enum_error:
   _mesa_error(ctx, GL_INVALID_ENUM, "gl%s(0x%x)",
               state ? "Enable" : "Disable", cap);
}


/**
 * Enable GL capability.  Called by glEnable()
 * \param cap  state to enable.
 */
void GLAPIENTRY
_mesa_Enable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_set_enable( ctx, cap, GL_TRUE );
}


/**
 * Disable GL capability.  Called by glDisable()
 * \param cap  state to disable.
 */
void GLAPIENTRY
_mesa_Disable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_set_enable( ctx, cap, GL_FALSE );
}



/**
 * Enable/disable an indexed state var.
 */
void
_mesa_set_enablei(struct gl_context *ctx, GLenum cap,
                  GLuint index, GLboolean state)
{
   ASSERT(state == 0 || state == 1);
   switch (cap) {
   case GL_BLEND:
      if (!ctx->Extensions.EXT_draw_buffers2) {
         goto invalid_enum_error;
      }
      if (index >= ctx->Const.MaxDrawBuffers) {
         _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%u)",
                     state ? "glEnableIndexed" : "glDisableIndexed", index);
         return;
      }
      if (((ctx->Color.BlendEnabled >> index) & 1) != state) {
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         if (state)
            ctx->Color.BlendEnabled |= (1 << index);
         else
            ctx->Color.BlendEnabled &= ~(1 << index);
      }
      break;
   default:
      goto invalid_enum_error;
   }
   return;

invalid_enum_error:
    _mesa_error(ctx, GL_INVALID_ENUM, "%s(cap=%s)",
                state ? "glEnablei" : "glDisablei",
                _mesa_lookup_enum_by_nr(cap));
}


void GLAPIENTRY
_mesa_DisableIndexed( GLenum cap, GLuint index )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   _mesa_set_enablei(ctx, cap, index, GL_FALSE);
}


void GLAPIENTRY
_mesa_EnableIndexed( GLenum cap, GLuint index )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   _mesa_set_enablei(ctx, cap, index, GL_TRUE);
}


GLboolean GLAPIENTRY
_mesa_IsEnabledIndexed( GLenum cap, GLuint index )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);
   switch (cap) {
   case GL_BLEND:
      if (index >= ctx->Const.MaxDrawBuffers) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glIsEnabledIndexed(index=%u)",
                     index);
         return GL_FALSE;
      }
      return (ctx->Color.BlendEnabled >> index) & 1;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glIsEnabledIndexed(cap=%s)",
                  _mesa_lookup_enum_by_nr(cap));
      return GL_FALSE;
   }
}




#undef CHECK_EXTENSION
#define CHECK_EXTENSION(EXTNAME)			\
   if (!ctx->Extensions.EXTNAME) {			\
      goto invalid_enum_error;				\
   }

#undef CHECK_EXTENSION2
#define CHECK_EXTENSION2(EXT1, EXT2)				\
   if (!ctx->Extensions.EXT1 && !ctx->Extensions.EXT2) {	\
      goto invalid_enum_error;					\
   }


/**
 * Helper function to determine whether a texture target is enabled.
 */
static GLboolean
is_texture_enabled(struct gl_context *ctx, GLbitfield bit)
{
   const struct gl_texture_unit *const texUnit =
       &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   return (texUnit->Enabled & bit) ? GL_TRUE : GL_FALSE;
}


/**
 * Return simple enable/disable state.
 *
 * \param cap  state variable to query.
 *
 * Returns the state of the specified capability from the current GL context.
 * For the capabilities associated with extensions verifies that those
 * extensions are effectively present before reporting.
 */
GLboolean GLAPIENTRY
_mesa_IsEnabled( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   switch (cap) {
      case GL_ALPHA_TEST:
         return ctx->Color.AlphaEnabled;
      case GL_AUTO_NORMAL:
	 return ctx->Eval.AutoNormal;
      case GL_BLEND:
         return ctx->Color.BlendEnabled & 1;  /* return state for buffer[0] */
      case GL_CLIP_DISTANCE0:
      case GL_CLIP_DISTANCE1:
      case GL_CLIP_DISTANCE2:
      case GL_CLIP_DISTANCE3:
      case GL_CLIP_DISTANCE4:
      case GL_CLIP_DISTANCE5:
      case GL_CLIP_DISTANCE6:
      case GL_CLIP_DISTANCE7: {
         const GLuint p = cap - GL_CLIP_DISTANCE0;

         if (p >= ctx->Const.MaxClipPlanes)
            goto invalid_enum_error;

	 return (ctx->Transform.ClipPlanesEnabled >> p) & 1;
      }
      case GL_COLOR_MATERIAL:
	 return ctx->Light.ColorMaterialEnabled;
      case GL_CULL_FACE:
         return ctx->Polygon.CullFlag;
      case GL_DEPTH_TEST:
         return ctx->Depth.Test;
      case GL_DITHER:
	 return ctx->Color.DitherFlag;
      case GL_FOG:
	 return ctx->Fog.Enabled;
      case GL_LIGHTING:
         return ctx->Light.Enabled;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
         return ctx->Light.Light[cap-GL_LIGHT0].Enabled;
      case GL_LINE_SMOOTH:
	 return ctx->Line.SmoothFlag;
      case GL_LINE_STIPPLE:
	 return ctx->Line.StippleFlag;
      case GL_INDEX_LOGIC_OP:
	 return ctx->Color.IndexLogicOpEnabled;
      case GL_COLOR_LOGIC_OP:
	 return ctx->Color.ColorLogicOpEnabled;
      case GL_MAP1_COLOR_4:
	 return ctx->Eval.Map1Color4;
      case GL_MAP1_INDEX:
	 return ctx->Eval.Map1Index;
      case GL_MAP1_NORMAL:
	 return ctx->Eval.Map1Normal;
      case GL_MAP1_TEXTURE_COORD_1:
	 return ctx->Eval.Map1TextureCoord1;
      case GL_MAP1_TEXTURE_COORD_2:
	 return ctx->Eval.Map1TextureCoord2;
      case GL_MAP1_TEXTURE_COORD_3:
	 return ctx->Eval.Map1TextureCoord3;
      case GL_MAP1_TEXTURE_COORD_4:
	 return ctx->Eval.Map1TextureCoord4;
      case GL_MAP1_VERTEX_3:
	 return ctx->Eval.Map1Vertex3;
      case GL_MAP1_VERTEX_4:
	 return ctx->Eval.Map1Vertex4;
      case GL_MAP2_COLOR_4:
	 return ctx->Eval.Map2Color4;
      case GL_MAP2_INDEX:
	 return ctx->Eval.Map2Index;
      case GL_MAP2_NORMAL:
	 return ctx->Eval.Map2Normal;
      case GL_MAP2_TEXTURE_COORD_1:
	 return ctx->Eval.Map2TextureCoord1;
      case GL_MAP2_TEXTURE_COORD_2:
	 return ctx->Eval.Map2TextureCoord2;
      case GL_MAP2_TEXTURE_COORD_3:
	 return ctx->Eval.Map2TextureCoord3;
      case GL_MAP2_TEXTURE_COORD_4:
	 return ctx->Eval.Map2TextureCoord4;
      case GL_MAP2_VERTEX_3:
	 return ctx->Eval.Map2Vertex3;
      case GL_MAP2_VERTEX_4:
	 return ctx->Eval.Map2Vertex4;
      case GL_NORMALIZE:
	 return ctx->Transform.Normalize;
      case GL_POINT_SMOOTH:
	 return ctx->Point.SmoothFlag;
      case GL_POLYGON_SMOOTH:
	 return ctx->Polygon.SmoothFlag;
      case GL_POLYGON_STIPPLE:
	 return ctx->Polygon.StippleFlag;
      case GL_POLYGON_OFFSET_POINT:
	 return ctx->Polygon.OffsetPoint;
      case GL_POLYGON_OFFSET_LINE:
	 return ctx->Polygon.OffsetLine;
      case GL_POLYGON_OFFSET_FILL:
	 return ctx->Polygon.OffsetFill;
      case GL_RESCALE_NORMAL_EXT:
         return ctx->Transform.RescaleNormals;
      case GL_SCISSOR_TEST:
	 return ctx->Scissor.Enabled;
      case GL_STENCIL_TEST:
	 return ctx->Stencil.Enabled;
      case GL_TEXTURE_1D:
         return is_texture_enabled(ctx, TEXTURE_1D_BIT);
      case GL_TEXTURE_2D:
         return is_texture_enabled(ctx, TEXTURE_2D_BIT);
      case GL_TEXTURE_3D:
         return is_texture_enabled(ctx, TEXTURE_3D_BIT);
      case GL_TEXTURE_GEN_S:
      case GL_TEXTURE_GEN_T:
      case GL_TEXTURE_GEN_R:
      case GL_TEXTURE_GEN_Q:
         {
            const struct gl_texture_unit *texUnit = get_texcoord_unit(ctx);
            if (texUnit) {
               GLbitfield coordBit = S_BIT << (cap - GL_TEXTURE_GEN_S);
               return (texUnit->TexGenEnabled & coordBit) ? GL_TRUE : GL_FALSE;
            }
         }
         return GL_FALSE;
#if FEATURE_ES1
      case GL_TEXTURE_GEN_STR_OES:
	 {
            const struct gl_texture_unit *texUnit = get_texcoord_unit(ctx);
            if (texUnit) {
               return (texUnit->TexGenEnabled & STR_BITS) == STR_BITS
                  ? GL_TRUE : GL_FALSE;
            }
         }
#endif

      /* client-side state */
      case GL_VERTEX_ARRAY:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled != 0);
      case GL_NORMAL_ARRAY:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_NORMAL].Enabled != 0);
      case GL_COLOR_ARRAY:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR0].Enabled != 0);
      case GL_INDEX_ARRAY:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Enabled != 0);
      case GL_TEXTURE_COORD_ARRAY:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)]
                 .Enabled != 0);
      case GL_EDGE_FLAG_ARRAY:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_EDGEFLAG].Enabled != 0);
      case GL_FOG_COORDINATE_ARRAY_EXT:
         CHECK_EXTENSION(EXT_fog_coord);
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_FOG].Enabled != 0);
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         CHECK_EXTENSION(EXT_secondary_color);
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_COLOR1].Enabled != 0);
#if FEATURE_point_size_array
      case GL_POINT_SIZE_ARRAY_OES:
         return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POINT_SIZE].Enabled != 0);
#endif

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION(ARB_texture_cube_map);
         return is_texture_enabled(ctx, TEXTURE_CUBE_BIT);

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION2(EXT_secondary_color, ARB_vertex_program);
         return ctx->Fog.ColorSumEnabled;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         return ctx->Multisample.Enabled;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         return ctx->Multisample.SampleAlphaToCoverage;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         return ctx->Multisample.SampleAlphaToOne;
      case GL_SAMPLE_COVERAGE_ARB:
         return ctx->Multisample.SampleCoverage;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         return ctx->Multisample.SampleCoverageInvert;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION(IBM_rasterpos_clip);
         return ctx->Transform.RasterPositionUnclipped;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION2(NV_point_sprite, ARB_point_sprite)
         return ctx->Point.PointSprite;

#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
      case GL_VERTEX_PROGRAM_ARB:
         CHECK_EXTENSION2(ARB_vertex_program, NV_vertex_program);
         return ctx->VertexProgram.Enabled;
      case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
         CHECK_EXTENSION2(ARB_vertex_program, NV_vertex_program);
         return ctx->VertexProgram.PointSizeEnabled;
      case GL_VERTEX_PROGRAM_TWO_SIDE_ARB:
         CHECK_EXTENSION2(ARB_vertex_program, NV_vertex_program);
         return ctx->VertexProgram.TwoSideEnabled;
#endif
#if FEATURE_NV_vertex_program
      case GL_VERTEX_ATTRIB_ARRAY0_NV:
      case GL_VERTEX_ATTRIB_ARRAY1_NV:
      case GL_VERTEX_ATTRIB_ARRAY2_NV:
      case GL_VERTEX_ATTRIB_ARRAY3_NV:
      case GL_VERTEX_ATTRIB_ARRAY4_NV:
      case GL_VERTEX_ATTRIB_ARRAY5_NV:
      case GL_VERTEX_ATTRIB_ARRAY6_NV:
      case GL_VERTEX_ATTRIB_ARRAY7_NV:
      case GL_VERTEX_ATTRIB_ARRAY8_NV:
      case GL_VERTEX_ATTRIB_ARRAY9_NV:
      case GL_VERTEX_ATTRIB_ARRAY10_NV:
      case GL_VERTEX_ATTRIB_ARRAY11_NV:
      case GL_VERTEX_ATTRIB_ARRAY12_NV:
      case GL_VERTEX_ATTRIB_ARRAY13_NV:
      case GL_VERTEX_ATTRIB_ARRAY14_NV:
      case GL_VERTEX_ATTRIB_ARRAY15_NV:
         CHECK_EXTENSION(NV_vertex_program);
         {
            GLint n = (GLint) cap - GL_VERTEX_ATTRIB_ARRAY0_NV;
            ASSERT(VERT_ATTRIB_GENERIC(n) < Elements(ctx->Array.ArrayObj->VertexAttrib));
            return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC(n)].Enabled != 0);
         }
      case GL_MAP1_VERTEX_ATTRIB0_4_NV:
      case GL_MAP1_VERTEX_ATTRIB1_4_NV:
      case GL_MAP1_VERTEX_ATTRIB2_4_NV:
      case GL_MAP1_VERTEX_ATTRIB3_4_NV:
      case GL_MAP1_VERTEX_ATTRIB4_4_NV:
      case GL_MAP1_VERTEX_ATTRIB5_4_NV:
      case GL_MAP1_VERTEX_ATTRIB6_4_NV:
      case GL_MAP1_VERTEX_ATTRIB7_4_NV:
      case GL_MAP1_VERTEX_ATTRIB8_4_NV:
      case GL_MAP1_VERTEX_ATTRIB9_4_NV:
      case GL_MAP1_VERTEX_ATTRIB10_4_NV:
      case GL_MAP1_VERTEX_ATTRIB11_4_NV:
      case GL_MAP1_VERTEX_ATTRIB12_4_NV:
      case GL_MAP1_VERTEX_ATTRIB13_4_NV:
      case GL_MAP1_VERTEX_ATTRIB14_4_NV:
      case GL_MAP1_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program);
         {
            const GLuint map = (GLuint) (cap - GL_MAP1_VERTEX_ATTRIB0_4_NV);
            return ctx->Eval.Map1Attrib[map];
         }
      case GL_MAP2_VERTEX_ATTRIB0_4_NV:
      case GL_MAP2_VERTEX_ATTRIB1_4_NV:
      case GL_MAP2_VERTEX_ATTRIB2_4_NV:
      case GL_MAP2_VERTEX_ATTRIB3_4_NV:
      case GL_MAP2_VERTEX_ATTRIB4_4_NV:
      case GL_MAP2_VERTEX_ATTRIB5_4_NV:
      case GL_MAP2_VERTEX_ATTRIB6_4_NV:
      case GL_MAP2_VERTEX_ATTRIB7_4_NV:
      case GL_MAP2_VERTEX_ATTRIB8_4_NV:
      case GL_MAP2_VERTEX_ATTRIB9_4_NV:
      case GL_MAP2_VERTEX_ATTRIB10_4_NV:
      case GL_MAP2_VERTEX_ATTRIB11_4_NV:
      case GL_MAP2_VERTEX_ATTRIB12_4_NV:
      case GL_MAP2_VERTEX_ATTRIB13_4_NV:
      case GL_MAP2_VERTEX_ATTRIB14_4_NV:
      case GL_MAP2_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program);
         {
            const GLuint map = (GLuint) (cap - GL_MAP2_VERTEX_ATTRIB0_4_NV);
            return ctx->Eval.Map2Attrib[map];
         }
#endif /* FEATURE_NV_vertex_program */

#if FEATURE_NV_fragment_program
      case GL_FRAGMENT_PROGRAM_NV:
         CHECK_EXTENSION(NV_fragment_program);
         return ctx->FragmentProgram.Enabled;
#endif /* FEATURE_NV_fragment_program */

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION(NV_texture_rectangle);
         return is_texture_enabled(ctx, TEXTURE_RECT_BIT);

      /* GL_EXT_stencil_two_side */
      case GL_STENCIL_TEST_TWO_SIDE_EXT:
         CHECK_EXTENSION(EXT_stencil_two_side);
         return ctx->Stencil.TestTwoSide;

#if FEATURE_ARB_fragment_program
      case GL_FRAGMENT_PROGRAM_ARB:
         return ctx->FragmentProgram.Enabled;
#endif /* FEATURE_ARB_fragment_program */

      /* GL_EXT_depth_bounds_test */
      case GL_DEPTH_BOUNDS_TEST_EXT:
         CHECK_EXTENSION(EXT_depth_bounds_test);
         return ctx->Depth.BoundsTest;

      /* GL_ARB_depth_clamp */
      case GL_DEPTH_CLAMP:
         CHECK_EXTENSION(ARB_depth_clamp);
         return ctx->Transform.DepthClamp;

#if FEATURE_ATI_fragment_shader
      case GL_FRAGMENT_SHADER_ATI:
	 CHECK_EXTENSION(ATI_fragment_shader);
	 return ctx->ATIFragmentShader.Enabled;
#endif /* FEATURE_ATI_fragment_shader */

      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
	 CHECK_EXTENSION(ARB_seamless_cube_map);
	 return ctx->Texture.CubeMapSeamless;

#if FEATURE_EXT_transform_feedback
      case GL_RASTERIZER_DISCARD:
	 CHECK_EXTENSION(EXT_transform_feedback);
         return ctx->RasterDiscard;
#endif

      /* GL_NV_primitive_restart */
      case GL_PRIMITIVE_RESTART_NV:
	 if (!ctx->Extensions.NV_primitive_restart) {
            goto invalid_enum_error;
         }
         return ctx->Array.PrimitiveRestart;

      /* GL 3.1 primitive restart */
      case GL_PRIMITIVE_RESTART:
         if (ctx->VersionMajor * 10 + ctx->VersionMinor < 31) {
            goto invalid_enum_error;
         }
         return ctx->Array.PrimitiveRestart;

      /* GL3.0 - GL_framebuffer_sRGB */
      case GL_FRAMEBUFFER_SRGB_EXT:
	 CHECK_EXTENSION(EXT_framebuffer_sRGB);
	 return ctx->Color.sRGBEnabled;

      /* GL_OES_EGL_image_external */
      case GL_TEXTURE_EXTERNAL_OES:
	 CHECK_EXTENSION(OES_EGL_image_external);
         return is_texture_enabled(ctx, TEXTURE_EXTERNAL_BIT);

      default:
         goto invalid_enum_error;
   }

   return GL_FALSE;

invalid_enum_error:
   _mesa_error(ctx, GL_INVALID_ENUM, "glIsEnabled(0x%x)", (int) cap);
   return GL_FALSE;
}
