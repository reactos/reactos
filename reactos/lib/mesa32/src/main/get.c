/**
 * \file get.c
 * State query functions.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.2
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/*
 * Regarding GL_NV_light_max_exponent:
 *
 * Portions of this software may use or implement intellectual
 * property owned and licensed by NVIDIA Corporation. NVIDIA disclaims
 * any and all warranties with respect to such intellectual property,
 * including any use thereof or modifications thereto.
 */

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "extensions.h"
#include "get.h"
#include "macros.h"
#include "mtypes.h"
#include "texcompress.h"
#include "version.h"
#include "math/m_matrix.h"


#define FLOAT_TO_BOOL(X)	( (X)==0.0F ? GL_FALSE : GL_TRUE )
#define INT_TO_BOOL(I)		( (I)==0 ? GL_FALSE : GL_TRUE )
#define ENUM_TO_BOOL(E)		( (E)==0 ? GL_FALSE : GL_TRUE )

#ifdef SPECIALCAST
/* Needed for an Amiga compiler */
#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))
#else
/* all other compilers */
#define ENUM_TO_FLOAT(X) ((GLfloat)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(X))
#endif


/* Check if named extension is enabled, if not generate error and return */

#define CHECK1(E1, str, PNAME)					\
   if (!ctx->Extensions.E1) {					\
      _mesa_error(ctx, GL_INVALID_VALUE,			\
                  "glGet" str "v(0x%x)", (int) PNAME);		\
      return;							\
   }
    
#define CHECK2(E1, E2, str, PNAME)				\
   if (!ctx->Extensions.E1 && !ctx->Extensions.E2) {		\
      _mesa_error(ctx, GL_INVALID_VALUE,			\
                  "glGet" str "v(0x%x)", (int) PNAME);		\
      return;							\
   }
    
#define CHECK_EXTENSION_B(EXTNAME, PNAME)			\
   CHECK1(EXTNAME, "Boolean", PNAME )

#define CHECK_EXTENSION_I(EXTNAME, PNAME)			\
   CHECK1(EXTNAME, "Integer", PNAME )

#define CHECK_EXTENSION_F(EXTNAME, PNAME)			\
   CHECK1(EXTNAME, "Float", PNAME )

#define CHECK_EXTENSION_D(EXTNAME, PNAME)			\
   CHECK1(EXTNAME, "Double", PNAME )

#define CHECK_EXTENSION2_B(EXT1, EXT2, PNAME)			\
   CHECK2(EXT1, EXT2, "Boolean", PNAME)

#define CHECK_EXTENSION2_I(EXT1, EXT2, PNAME)			\
   CHECK2(EXT1, EXT2, "Integer", PNAME)

#define CHECK_EXTENSION2_F(EXT1, EXT2, PNAME)			\
   CHECK2(EXT1, EXT2, "Float", PNAME)

#define CHECK_EXTENSION2_D(EXT1, EXT2, PNAME)			\
   CHECK2(EXT1, EXT2, "Double", PNAME)



static GLenum
pixel_texgen_mode(const GLcontext *ctx)
{
   if (ctx->Pixel.FragmentRgbSource == GL_CURRENT_RASTER_POSITION) {
      if (ctx->Pixel.FragmentAlphaSource == GL_CURRENT_RASTER_POSITION) {
         return GL_RGBA;
      }
      else {
         return GL_RGB;
      }
   }
   else {
      if (ctx->Pixel.FragmentAlphaSource == GL_CURRENT_RASTER_POSITION) {
         return GL_ALPHA;
      }
      else {
         return GL_NONE;
      }
   }
}


/**
 * Get the value(s) of a selected parameter.
 *
 * \param pname parameter to be returned.
 * \param params will hold the value(s) of the speficifed parameter.
 *
 * \sa glGetBooleanv().
 *
 * Tries to get the specified parameter via dd_function_table::GetBooleanv,
 * otherwise gets the specified parameter from the current context, converting
 * it value into GLboolean.
 */
void GLAPIENTRY
_mesa_GetBooleanv( GLenum pname, GLboolean *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   const GLuint clientUnit = ctx->Array.ActiveTexture;
   const GLuint texUnit = ctx->Texture.CurrentUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!params)
      return;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetBooleanv %s\n", _mesa_lookup_enum_by_nr(pname));

   if (!ctx->_CurrentProgram) {
      /* We need this in order to get correct results for
       * GL_OCCLUSION_TEST_RESULT_HP.  There might be other important cases.
       */
      FLUSH_VERTICES(ctx, 0);
   }

   if (ctx->Driver.GetBooleanv
       && (*ctx->Driver.GetBooleanv)(ctx, pname, params))
      return;

   switch (pname) {
      case GL_ACCUM_RED_BITS:
         *params = INT_TO_BOOL(ctx->Visual.accumRedBits);
         break;
      case GL_ACCUM_GREEN_BITS:
         *params = INT_TO_BOOL(ctx->Visual.accumGreenBits);
         break;
      case GL_ACCUM_BLUE_BITS:
         *params = INT_TO_BOOL(ctx->Visual.accumBlueBits);
         break;
      case GL_ACCUM_ALPHA_BITS:
         *params = INT_TO_BOOL(ctx->Visual.accumAlphaBits);
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[0]);
         params[1] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[1]);
         params[2] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[2]);
         params[3] = FLOAT_TO_BOOL(ctx->Accum.ClearColor[3]);
         break;
      case GL_ALPHA_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.AlphaBias);
         break;
      case GL_ALPHA_BITS:
         *params = INT_TO_BOOL(ctx->Visual.alphaBits);
         break;
      case GL_ALPHA_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.AlphaScale);
         break;
      case GL_ALPHA_TEST:
         *params = ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = ENUM_TO_BOOL(ctx->Color.AlphaFunc);
         break;
      case GL_ALPHA_TEST_REF:
         *params = ctx->Color.AlphaRef ? GL_TRUE : GL_FALSE;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = INT_TO_BOOL(ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (ctx->Visual.numAuxBuffers) ? GL_TRUE : GL_FALSE;
         break;
      case GL_BLEND:
         *params = ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = ENUM_TO_BOOL(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC:
         *params = ENUM_TO_BOOL(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_SRC_RGB_EXT:
         *params = ENUM_TO_BOOL(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_DST_RGB_EXT:
         *params = ENUM_TO_BOOL(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC_ALPHA_EXT:
         *params = ENUM_TO_BOOL(ctx->Color.BlendSrcA);
         break;
      case GL_BLEND_DST_ALPHA_EXT:
         *params = ENUM_TO_BOOL(ctx->Color.BlendDstA);
         break;
      case GL_BLEND_EQUATION:
	 *params = ENUM_TO_BOOL( ctx->Color.BlendEquationRGB );
	 break;
      case GL_BLEND_EQUATION_ALPHA_EXT:
	 *params = ENUM_TO_BOOL( ctx->Color.BlendEquationA );
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = FLOAT_TO_BOOL( ctx->Color.BlendColor[0] );
	 params[1] = FLOAT_TO_BOOL( ctx->Color.BlendColor[1] );
	 params[2] = FLOAT_TO_BOOL( ctx->Color.BlendColor[2] );
	 params[3] = FLOAT_TO_BOOL( ctx->Color.BlendColor[3] );
	 break;
      case GL_BLUE_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.BlueBias);
         break;
      case GL_BLUE_BITS:
         *params = INT_TO_BOOL( ctx->Visual.blueBits );
         break;
      case GL_BLUE_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.BlueScale);
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = INT_TO_BOOL(ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         if (ctx->Transform.ClipPlanesEnabled & (1 << (pname - GL_CLIP_PLANE0)))
            *params = GL_TRUE;
         else
            *params = GL_FALSE;
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = ctx->Color.ClearColor[0] ? GL_TRUE : GL_FALSE;
         params[1] = ctx->Color.ClearColor[1] ? GL_TRUE : GL_FALSE;
         params[2] = ctx->Color.ClearColor[2] ? GL_TRUE : GL_FALSE;
         params[3] = ctx->Color.ClearColor[3] ? GL_TRUE : GL_FALSE;
         break;
      case GL_COLOR_MATERIAL:
         *params = ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = ENUM_TO_BOOL(ctx->Light.ColorMaterialFace);
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = ENUM_TO_BOOL(ctx->Light.ColorMaterialMode);
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? GL_TRUE : GL_FALSE;
         params[1] = ctx->Color.ColorMask[GCOMP] ? GL_TRUE : GL_FALSE;
         params[2] = ctx->Color.ColorMask[BCOMP] ? GL_TRUE : GL_FALSE;
         params[3] = ctx->Color.ColorMask[ACOMP] ? GL_TRUE : GL_FALSE;
         break;
      case GL_CULL_FACE:
         *params = ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = ENUM_TO_BOOL(ctx->Polygon.CullFaceMode);
         break;
      case GL_CURRENT_COLOR:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2]);
         params[3] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3]);
         break;
      case GL_CURRENT_INDEX:
	 FLUSH_CURRENT(ctx, 0);
         *params = FLOAT_TO_BOOL(ctx->Current.Index);
         break;
      case GL_CURRENT_NORMAL:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2]);
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = FLOAT_TO_BOOL(ctx->Current.RasterColor[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Current.RasterColor[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Current.RasterColor[2]);
	 params[3] = FLOAT_TO_BOOL(ctx->Current.RasterColor[3]);
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 *params = FLOAT_TO_BOOL(ctx->Current.RasterDistance);
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = FLOAT_TO_BOOL(ctx->Current.RasterIndex);
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = FLOAT_TO_BOOL(ctx->Current.RasterPos[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Current.RasterPos[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Current.RasterPos[2]);
	 params[3] = FLOAT_TO_BOOL(ctx->Current.RasterPos[3]);
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
         params[0] = FLOAT_TO_BOOL(ctx->Current.RasterTexCoords[texUnit][0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.RasterTexCoords[texUnit][1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.RasterTexCoords[texUnit][2]);
         params[3] = FLOAT_TO_BOOL(ctx->Current.RasterTexCoords[texUnit][3]);
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
         *params = ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][0]);
         params[1] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][1]);
         params[2] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][2]);
         params[3] = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][3]);
	 break;
      case GL_DEPTH_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.DepthBias);
	 break;
      case GL_DEPTH_BITS:
	 *params = INT_TO_BOOL(ctx->Visual.depthBits);
	 break;
      case GL_DEPTH_CLEAR_VALUE:
         *params = FLOAT_TO_BOOL(ctx->Depth.Clear);
	 break;
      case GL_DEPTH_FUNC:
         *params = ENUM_TO_BOOL(ctx->Depth.Func);
	 break;
      case GL_DEPTH_RANGE:
         params[0] = FLOAT_TO_BOOL(ctx->Viewport.Near);
         params[1] = FLOAT_TO_BOOL(ctx->Viewport.Far);
	 break;
      case GL_DEPTH_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.DepthScale);
	 break;
      case GL_DEPTH_TEST:
         *params = ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = ctx->Visual.doubleBufferMode;
	 break;
      case GL_DRAW_BUFFER:
	 *params = ENUM_TO_BOOL(ctx->Color.DrawBuffer);
	 break;
      case GL_EDGE_FLAG:
	 FLUSH_CURRENT(ctx, 0);
	 *params = ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         *params = INT_TO_BOOL(ctx->Feedback.BufferSize);
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = INT_TO_BOOL(ctx->Feedback.Type);
         break;
      case GL_FOG:
	 *params = ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
         params[0] = FLOAT_TO_BOOL(ctx->Fog.Color[0]);
         params[1] = FLOAT_TO_BOOL(ctx->Fog.Color[1]);
         params[2] = FLOAT_TO_BOOL(ctx->Fog.Color[2]);
         params[3] = FLOAT_TO_BOOL(ctx->Fog.Color[3]);
	 break;
      case GL_FOG_DENSITY:
         *params = FLOAT_TO_BOOL(ctx->Fog.Density);
	 break;
      case GL_FOG_END:
         *params = FLOAT_TO_BOOL(ctx->Fog.End);
	 break;
      case GL_FOG_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.Fog);
	 break;
      case GL_FOG_INDEX:
	 *params = FLOAT_TO_BOOL(ctx->Fog.Index);
	 break;
      case GL_FOG_MODE:
	 *params = ENUM_TO_BOOL(ctx->Fog.Mode);
	 break;
      case GL_FOG_START:
         *params = FLOAT_TO_BOOL(ctx->Fog.End);
	 break;
      case GL_FRONT_FACE:
	 *params = ENUM_TO_BOOL(ctx->Polygon.FrontFace);
	 break;
      case GL_GREEN_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.GreenBias);
	 break;
      case GL_GREEN_BITS:
         *params = INT_TO_BOOL( ctx->Visual.greenBits );
	 break;
      case GL_GREEN_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.GreenScale);
	 break;
      case GL_INDEX_BITS:
         *params = INT_TO_BOOL( ctx->Visual.indexBits );
	 break;
      case GL_INDEX_CLEAR_VALUE:
	 *params = INT_TO_BOOL(ctx->Color.ClearIndex);
	 break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual.rgbMode ? GL_FALSE : GL_TRUE;
	 break;
      case GL_INDEX_OFFSET:
	 *params = INT_TO_BOOL(ctx->Pixel.IndexOffset);
	 break;
      case GL_INDEX_SHIFT:
	 *params = INT_TO_BOOL(ctx->Pixel.IndexShift);
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = INT_TO_BOOL(ctx->Color.IndexMask);
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[2]);
	 params[3] = FLOAT_TO_BOOL(ctx->Light.Model.Ambient[3]);
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = ENUM_TO_BOOL(ctx->Light.Model.ColorControl);
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.LineSmooth);
	 break;
      case GL_LINE_STIPPLE:
	 *params = ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
	 *params = INT_TO_BOOL(ctx->Line.StipplePattern);
	 break;
      case GL_LINE_STIPPLE_REPEAT:
	 *params = INT_TO_BOOL(ctx->Line.StippleFactor);
	 break;
      case GL_LINE_WIDTH:
	 *params = FLOAT_TO_BOOL(ctx->Line.Width);
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = FLOAT_TO_BOOL(ctx->Const.LineWidthGranularity);
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = FLOAT_TO_BOOL(ctx->Const.MinLineWidthAA);
	 params[1] = FLOAT_TO_BOOL(ctx->Const.MaxLineWidthAA);
         break;
      case GL_ALIASED_LINE_WIDTH_RANGE:
	 params[0] = FLOAT_TO_BOOL(ctx->Const.MinLineWidth);
	 params[1] = FLOAT_TO_BOOL(ctx->Const.MaxLineWidth);
	 break;
      case GL_LIST_BASE:
	 *params = INT_TO_BOOL(ctx->List.ListBase);
	 break;
      case GL_LIST_INDEX:
	 *params = INT_TO_BOOL( ctx->ListState.CurrentListNum );
	 break;
      case GL_LIST_MODE:
         if (!ctx->CompileFlag)
            *params = 0;
         else if (ctx->ExecuteFlag)
            *params = ENUM_TO_BOOL(GL_COMPILE_AND_EXECUTE);
         else
            *params = ENUM_TO_BOOL(GL_COMPILE);
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
	 *params = ENUM_TO_BOOL(ctx->Color.LogicOp);
	 break;
      case GL_MAP1_COLOR_4:
	 *params = ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = FLOAT_TO_BOOL(ctx->Eval.MapGrid1u1);
	 params[1] = FLOAT_TO_BOOL(ctx->Eval.MapGrid1u2);
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = INT_TO_BOOL(ctx->Eval.MapGrid1un);
	 break;
      case GL_MAP1_INDEX:
	 *params = ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2u1);
	 params[1] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2u2);
	 params[2] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2v1);
	 params[3] = FLOAT_TO_BOOL(ctx->Eval.MapGrid2v2);
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = INT_TO_BOOL(ctx->Eval.MapGrid2un);
	 params[1] = INT_TO_BOOL(ctx->Eval.MapGrid2vn);
	 break;
      case GL_MAP2_INDEX:
	 *params = ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = ENUM_TO_BOOL( ctx->Transform.MatrixMode );
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_ATTRIB_STACK_DEPTH);
	 break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = INT_TO_BOOL( MAX_CLIENT_ATTRIB_STACK_DEPTH);
         break;
      case GL_MAX_CLIP_PLANES:
	 *params = INT_TO_BOOL(ctx->Const.MaxClipPlanes);
	 break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = INT_TO_BOOL(ctx->Const.MaxArrayLockSize);
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = INT_TO_BOOL(ctx->Const.MaxArrayLockSize);
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = INT_TO_BOOL(MAX_EVAL_ORDER);
	 break;
      case GL_MAX_LIGHTS:
	 *params = INT_TO_BOOL(ctx->Const.MaxLights);
	 break;
      case GL_MAX_LIST_NESTING:
	 *params = INT_TO_BOOL(MAX_LIST_NESTING);
	 break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_MODELVIEW_STACK_DEPTH);
	 break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_NAME_STACK_DEPTH);
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = INT_TO_BOOL(MAX_PIXEL_MAP_TABLE);
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_PROJECTION_STACK_DEPTH);
	 break;
      case GL_MAX_TEXTURE_SIZE:
         *params = INT_TO_BOOL(1 << (ctx->Const.MaxTextureLevels - 1));
	 break;
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = INT_TO_BOOL(1 << (ctx->Const.Max3DTextureLevels - 1));
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = INT_TO_BOOL(MAX_TEXTURE_STACK_DEPTH);
	 break;
      case GL_MAX_VIEWPORT_DIMS:
	 params[0] = INT_TO_BOOL(MAX_WIDTH);
	 params[1] = INT_TO_BOOL(MAX_HEIGHT);
	 break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = FLOAT_TO_BOOL(ctx->ModelviewMatrixStack.Top->m[i]);
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
         *params = INT_TO_BOOL(ctx->ModelviewMatrixStack.Depth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->Select.NameStackDepth);
	 break;
      case GL_NORMALIZE:
	 *params = ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = INT_TO_BOOL(ctx->Pack.Alignment);
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = INT_TO_BOOL(ctx->Pack.RowLength);
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = INT_TO_BOOL(ctx->Pack.SkipPixels);
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = INT_TO_BOOL(ctx->Pack.SkipRows);
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Pack.ImageHeight;
         break;
      case GL_PACK_INVERT_MESA:
         *params = ctx->Pack.Invert;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.PerspectiveCorrection);
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapAtoAsize);
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapBtoBsize);
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapGtoGsize);
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoAsize);
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoBsize);
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoGsize);
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoIsize);
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapItoRsize);
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapRtoRsize);
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = INT_TO_BOOL(ctx->Pixel.MapStoSsize);
	 break;
      case GL_POINT_SIZE:
	 *params = FLOAT_TO_BOOL(ctx->Point.Size);
	 break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = FLOAT_TO_BOOL(ctx->Const.PointSizeGranularity );
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = FLOAT_TO_BOOL(ctx->Const.MinPointSizeAA);
	 params[1] = FLOAT_TO_BOOL(ctx->Const.MaxPointSizeAA);
         break;
      case GL_ALIASED_POINT_SIZE_RANGE:
	 params[0] = FLOAT_TO_BOOL(ctx->Const.MinPointSize);
	 params[1] = FLOAT_TO_BOOL(ctx->Const.MaxPointSize);
	 break;
      case GL_POINT_SMOOTH:
	 *params = ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.PointSmooth);
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = FLOAT_TO_BOOL(ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = FLOAT_TO_BOOL(ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = FLOAT_TO_BOOL(ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = FLOAT_TO_BOOL(ctx->Point.Params[0]);
	 params[1] = FLOAT_TO_BOOL(ctx->Point.Params[1]);
	 params[2] = FLOAT_TO_BOOL(ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = ENUM_TO_BOOL(ctx->Polygon.FrontMode);
	 params[1] = ENUM_TO_BOOL(ctx->Polygon.BackMode);
	 break;
      case GL_POLYGON_OFFSET_BIAS_EXT:  /* GL_EXT_polygon_offset */
         *params = FLOAT_TO_BOOL( ctx->Polygon.OffsetUnits );
         break;
      case GL_POLYGON_OFFSET_FACTOR:
         *params = FLOAT_TO_BOOL( ctx->Polygon.OffsetFactor );
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = FLOAT_TO_BOOL( ctx->Polygon.OffsetUnits );
         break;
      case GL_POLYGON_SMOOTH:
	 *params = ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = ENUM_TO_BOOL(ctx->Hint.PolygonSmooth);
	 break;
      case GL_POLYGON_STIPPLE:
	 *params = ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = FLOAT_TO_BOOL(ctx->ProjectionMatrixStack.Top->m[i]);
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->ProjectionMatrixStack.Depth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = ENUM_TO_BOOL(ctx->Pixel.ReadBuffer);
	 break;
      case GL_RED_BIAS:
         *params = FLOAT_TO_BOOL(ctx->Pixel.RedBias);
	 break;
      case GL_RED_BITS:
         *params = INT_TO_BOOL( ctx->Visual.redBits );
	 break;
      case GL_RED_SCALE:
         *params = FLOAT_TO_BOOL(ctx->Pixel.RedScale);
	 break;
      case GL_RENDER_MODE:
	 *params = ENUM_TO_BOOL(ctx->RenderMode);
	 break;
      case GL_RESCALE_NORMAL:
         *params = ctx->Transform.RescaleNormals;
         break;
      case GL_RGBA_MODE:
         *params = ctx->Visual.rgbMode;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = INT_TO_BOOL(ctx->Scissor.X);
	 params[1] = INT_TO_BOOL(ctx->Scissor.Y);
	 params[2] = INT_TO_BOOL(ctx->Scissor.Width);
	 params[3] = INT_TO_BOOL(ctx->Scissor.Height);
	 break;
      case GL_SCISSOR_TEST:
	 *params = ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = INT_TO_BOOL(ctx->Select.BufferSize);
         break;
      case GL_SHADE_MODEL:
	 *params = ENUM_TO_BOOL(ctx->Light.ShadeModel);
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
	 *params = INT_TO_BOOL(ctx->Visual.stencilBits);
	 break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = INT_TO_BOOL(ctx->Stencil.Clear);
	 break;
      case GL_STENCIL_FAIL:
	 *params = ENUM_TO_BOOL(ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_FUNC:
	 *params = ENUM_TO_BOOL(ctx->Stencil.Function[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = ENUM_TO_BOOL(ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = ENUM_TO_BOOL(ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_REF:
	 *params = INT_TO_BOOL(ctx->Stencil.Ref[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_TEST:
	 *params = ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = INT_TO_BOOL(ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = INT_TO_BOOL(ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STEREO:
	 *params = ctx->Visual.stereoMode;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = INT_TO_BOOL(ctx->Const.SubPixelBits);
	 break;
      case GL_TEXTURE_1D:
         *params = _mesa_IsEnabled(GL_TEXTURE_1D);
	 break;
      case GL_TEXTURE_2D:
         *params = _mesa_IsEnabled(GL_TEXTURE_2D);
	 break;
      case GL_TEXTURE_3D:
         *params = _mesa_IsEnabled(GL_TEXTURE_3D);
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = INT_TO_BOOL(textureUnit->Current1D->Name);
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = INT_TO_BOOL(textureUnit->Current2D->Name);
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = INT_TO_BOOL(textureUnit->Current3D->Name);
         break;
      case GL_TEXTURE_ENV_COLOR:
         {
            params[0] = FLOAT_TO_BOOL(textureUnit->EnvColor[0]);
            params[1] = FLOAT_TO_BOOL(textureUnit->EnvColor[1]);
            params[2] = FLOAT_TO_BOOL(textureUnit->EnvColor[2]);
            params[3] = FLOAT_TO_BOOL(textureUnit->EnvColor[3]);
         }
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = ENUM_TO_BOOL(textureUnit->EnvMode);
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? GL_TRUE : GL_FALSE;
	 break;
      case GL_TEXTURE_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] =
	       FLOAT_TO_BOOL(ctx->TextureMatrixStack[texUnit].Top->m[i]);
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = INT_TO_BOOL(ctx->TextureMatrixStack[texUnit].Depth + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = INT_TO_BOOL(ctx->Unpack.Alignment);
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = INT_TO_BOOL(ctx->Unpack.RowLength);
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = INT_TO_BOOL(ctx->Unpack.SkipPixels);
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = INT_TO_BOOL(ctx->Unpack.SkipRows);
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Unpack.ImageHeight;
         break;
      case GL_UNPACK_CLIENT_STORAGE_APPLE:
         *params = ctx->Unpack.ClientStorage;
         break;
      case GL_VIEWPORT:
	 params[0] = INT_TO_BOOL(ctx->Viewport.X);
	 params[1] = INT_TO_BOOL(ctx->Viewport.Y);
	 params[2] = INT_TO_BOOL(ctx->Viewport.Width);
	 params[3] = INT_TO_BOOL(ctx->Viewport.Height);
	 break;
      case GL_ZOOM_X:
	 *params = FLOAT_TO_BOOL(ctx->Pixel.ZoomX);
	 break;
      case GL_ZOOM_Y:
	 *params = FLOAT_TO_BOOL(ctx->Pixel.ZoomY);
	 break;
      case GL_VERTEX_ARRAY:
         *params = ctx->Array.Vertex.Enabled;
         break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = INT_TO_BOOL(ctx->Array.Vertex.Size);
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Vertex.Type);
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Vertex.Stride);
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_NORMAL_ARRAY:
         *params = ctx->Array.Normal.Enabled;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Normal.Type);
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Normal.Stride);
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_COLOR_ARRAY:
         *params = ctx->Array.Color.Enabled;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = INT_TO_BOOL(ctx->Array.Color.Size);
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Color.Type);
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Color.Stride);
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_INDEX_ARRAY:
         *params = ctx->Array.Index.Enabled;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.Index.Type);
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.Index.Stride);
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_TEXTURE_COORD_ARRAY:
         *params = ctx->Array.TexCoord[clientUnit].Enabled;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = INT_TO_BOOL(ctx->Array.TexCoord[clientUnit].Size);
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ENUM_TO_BOOL(ctx->Array.TexCoord[clientUnit].Type);
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.TexCoord[clientUnit].Stride);
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;
      case GL_EDGE_FLAG_ARRAY:
         *params = ctx->Array.EdgeFlag.Enabled;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = INT_TO_BOOL(ctx->Array.EdgeFlag.Stride);
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = INT_TO_BOOL(0);
         break;

      /* GL_ARB_multitexture */
      case GL_MAX_TEXTURE_UNITS_ARB:
         CHECK_EXTENSION_B(ARB_multitexture, pname);
         *params = INT_TO_BOOL(MIN2(ctx->Const.MaxTextureImageUnits,
                                    ctx->Const.MaxTextureCoordUnits));
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_B(ARB_multitexture, pname);
         *params = INT_TO_BOOL(GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit);
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_B(ARB_multitexture, pname);
         *params = INT_TO_BOOL(GL_TEXTURE0_ARB + clientUnit);
         break;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION_B(ARB_texture_cube_map, pname);
         *params = _mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB);
         break;
      case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
         CHECK_EXTENSION_B(ARB_texture_cube_map, pname);
         *params = INT_TO_BOOL(textureUnit->CurrentCubeMap->Name);
         break;
      case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
         CHECK_EXTENSION_B(ARB_texture_cube_map, pname);
         *params = INT_TO_BOOL(1 << (ctx->Const.MaxCubeTextureLevels - 1));
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSION_HINT_ARB:
         CHECK_EXTENSION_B(ARB_texture_compression, pname);
         *params = INT_TO_BOOL(ctx->Hint.TextureCompression);
         break;
      case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_B(ARB_texture_compression, pname);
         *params = INT_TO_BOOL(_mesa_get_compressed_formats(ctx, NULL));
         break;
      case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_B(ARB_texture_compression, pname);
         {
            GLint formats[100];
            GLuint i, n;
            n = _mesa_get_compressed_formats(ctx, formats);
            for (i = 0; i < n; i++)
               params[i] = INT_TO_BOOL(formats[i]);
         }
         break;

      /* GL_EXT_compiled_vertex_array */
      case GL_ARRAY_ELEMENT_LOCK_FIRST_EXT:
	 *params = ctx->Array.LockFirst ? GL_TRUE : GL_FALSE;
	 break;
      case GL_ARRAY_ELEMENT_LOCK_COUNT_EXT:
	 *params = ctx->Array.LockCount ? GL_TRUE : GL_FALSE;
	 break;

      /* GL_ARB_transpose_matrix */
      case GL_TRANSPOSE_COLOR_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ColorMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = FLOAT_TO_BOOL(tm[i]);
            }
         }
         break;
      case GL_TRANSPOSE_MODELVIEW_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ModelviewMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = FLOAT_TO_BOOL(tm[i]);
            }
         }
         break;
      case GL_TRANSPOSE_PROJECTION_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ProjectionMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = FLOAT_TO_BOOL(tm[i]);
            }
         }
         break;
      case GL_TRANSPOSE_TEXTURE_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->TextureMatrixStack[texUnit].Top->m);
            for (i=0;i<16;i++) {
               params[i] = FLOAT_TO_BOOL(tm[i]);
            }
         }
         break;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         CHECK_EXTENSION_B(HP_occlusion_test, pname);
         *params = ctx->Depth.OcclusionTest;
         return;
      case GL_OCCLUSION_TEST_RESULT_HP:
         CHECK_EXTENSION_B(HP_occlusion_test, pname);
         if (ctx->Depth.OcclusionTest)
            *params = ctx->OcclusionResult;
         else
            *params = ctx->OcclusionResultSaved;
         /* reset flag now */
         ctx->OcclusionResult = GL_FALSE;
         ctx->OcclusionResultSaved = GL_FALSE;
         return;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         *params = ctx->Pixel.PixelTextureEnabled;
         break;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         *params = ctx->Pixel.PixelTextureEnabled;
         break;
      case GL_PIXEL_TEX_GEN_MODE_SGIX:
         *params = (GLboolean) pixel_texgen_mode(ctx);
         break;

      /* GL_SGI_color_matrix (also in 1.2 imaging) */
      case GL_COLOR_MATRIX_SGI:
         for (i=0;i<16;i++) {
	    params[i] = FLOAT_TO_BOOL(ctx->ColorMatrixStack.Top->m[i]);
	 }
	 break;
      case GL_COLOR_MATRIX_STACK_DEPTH_SGI:
         *params = INT_TO_BOOL(ctx->ColorMatrixStack.Depth + 1);
         break;
      case GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI:
         *params = FLOAT_TO_BOOL(MAX_COLOR_STACK_DEPTH);
         break;
      case GL_POST_COLOR_MATRIX_RED_SCALE_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixScale[0]);
         break;
      case GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixScale[1]);
         break;
      case GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixScale[2]);
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixScale[3]);
         break;
      case GL_POST_COLOR_MATRIX_RED_BIAS_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixBias[0]);
         break;
      case GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixBias[1]);
         break;
      case GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixBias[2]);
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI:
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostColorMatrixBias[3]);
         break;

      /* GL_EXT_convolution (also in 1.2 imaging) */
      case GL_CONVOLUTION_1D_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = ctx->Pixel.Convolution1DEnabled;
         break;
      case GL_CONVOLUTION_2D:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = ctx->Pixel.Convolution2DEnabled;
         break;
      case GL_SEPARABLE_2D:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = ctx->Pixel.Separable2DEnabled;
         break;
      case GL_POST_CONVOLUTION_RED_SCALE_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionScale[0]);
         break;
      case GL_POST_CONVOLUTION_GREEN_SCALE_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionScale[1]);
         break;
      case GL_POST_CONVOLUTION_BLUE_SCALE_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionScale[2]);
         break;
      case GL_POST_CONVOLUTION_ALPHA_SCALE_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionScale[3]);
         break;
      case GL_POST_CONVOLUTION_RED_BIAS_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionBias[0]);
         break;
      case GL_POST_CONVOLUTION_GREEN_BIAS_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionBias[1]);
         break;
      case GL_POST_CONVOLUTION_BLUE_BIAS_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionBias[2]);
         break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS_EXT:
         CHECK_EXTENSION_B(EXT_convolution, pname);
         *params = FLOAT_TO_BOOL(ctx->Pixel.PostConvolutionBias[2]);
         break;

      /* GL_EXT_histogram (also in 1.2 imaging) */
      case GL_HISTOGRAM:
         CHECK_EXTENSION_B(EXT_histogram, pname);
         *params = ctx->Pixel.HistogramEnabled;
	 break;
      case GL_MINMAX:
         CHECK_EXTENSION_B(EXT_histogram, pname);
         *params = ctx->Pixel.MinMaxEnabled;
         break;

      /* GL_SGI_color_table (also in 1.2 imaging */
      case GL_COLOR_TABLE_SGI:
         *params = ctx->Pixel.ColorTableEnabled;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         *params = ctx->Pixel.PostConvolutionColorTableEnabled;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         *params = ctx->Pixel.PostColorMatrixColorTableEnabled;
         break;

      /* GL_SGI_texture_color_table */
      case GL_TEXTURE_COLOR_TABLE_SGI:
         CHECK_EXTENSION_B(SGI_texture_color_table, pname);
         *params = textureUnit->ColorTableEnabled;
         break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION_B(EXT_secondary_color, pname);
	 *params = ctx->Fog.ColorSumEnabled;
	 break;
      case GL_CURRENT_SECONDARY_COLOR_EXT:
         CHECK_EXTENSION_B(EXT_secondary_color, pname);
	 FLUSH_CURRENT(ctx, 0);
         params[0] = INT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0]);
         params[1] = INT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1]);
         params[2] = INT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2]);
         params[3] = INT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3]);
	 break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         CHECK_EXTENSION_B(EXT_secondary_color, pname);
         *params = ctx->Array.SecondaryColor.Enabled;
         break;
      case GL_SECONDARY_COLOR_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_B(EXT_secondary_color, pname);
	 *params = ENUM_TO_BOOL(ctx->Array.SecondaryColor.Type);
	 break;
      case GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_B(EXT_secondary_color, pname);
	 *params = INT_TO_BOOL(ctx->Array.SecondaryColor.Stride);
	 break;
      case GL_SECONDARY_COLOR_ARRAY_SIZE_EXT:
         CHECK_EXTENSION_B(EXT_secondary_color, pname);
	 *params = INT_TO_BOOL(ctx->Array.SecondaryColor.Size);
	 break;

      /* GL_EXT_fog_coord */
      case GL_CURRENT_FOG_COORDINATE_EXT:
         CHECK_EXTENSION_B(EXT_fog_coord, pname);
	 FLUSH_CURRENT(ctx, 0);
	 *params = FLOAT_TO_BOOL(ctx->Current.Attrib[VERT_ATTRIB_FOG][0]);
	 break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         CHECK_EXTENSION_B(EXT_fog_coord, pname);
         *params = ctx->Array.FogCoord.Enabled;
         break;
      case GL_FOG_COORDINATE_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_B(EXT_fog_coord, pname);
	 *params = ENUM_TO_BOOL(ctx->Array.FogCoord.Type);
	 break;
      case GL_FOG_COORDINATE_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_B(EXT_fog_coord, pname);
	 *params = INT_TO_BOOL(ctx->Array.FogCoord.Stride);
	 break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
         CHECK_EXTENSION_B(EXT_fog_coord, pname);
	 *params = ENUM_TO_BOOL(ctx->Fog.FogCoordinateSource);
	 break;

      /* GL_EXT_texture_lod_bias */
      case GL_MAX_TEXTURE_LOD_BIAS_EXT:
         *params = FLOAT_TO_BOOL(ctx->Const.MaxTextureLodBias);
         break;

      /* GL_EXT_texture_filter_anisotropic */
      case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
         CHECK_EXTENSION_B(EXT_texture_filter_anisotropic, pname);
         *params = FLOAT_TO_BOOL(ctx->Const.MaxTextureMaxAnisotropy);
	 break;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = ctx->Multisample.Enabled;
         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = ctx->Multisample.SampleAlphaToCoverage;
         break;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = ctx->Multisample.SampleAlphaToOne;
         break;
      case GL_SAMPLE_COVERAGE_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = ctx->Multisample.SampleCoverage;
         break;
      case GL_SAMPLE_COVERAGE_VALUE_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = FLOAT_TO_BOOL(ctx->Multisample.SampleCoverageValue);
         break;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = ctx->Multisample.SampleCoverageInvert;
         break;
      case GL_SAMPLE_BUFFERS_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = 0; /* XXX fix someday */
         break;
      case GL_SAMPLES_ARB:
         CHECK_EXTENSION_B(ARB_multisample, pname);
         *params = 0; /* XXX fix someday */
         break;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION_B(IBM_rasterpos_clip, pname);
         *params = ctx->Transform.RasterPositionUnclipped;
         break;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION2_B(NV_point_sprite, ARB_point_sprite, pname);
         *params = ctx->Point.PointSprite;
         break;
      case GL_POINT_SPRITE_R_MODE_NV:
         CHECK_EXTENSION_B(NV_point_sprite, pname);
         *params = ENUM_TO_BOOL(ctx->Point.SpriteRMode);
         break;
      case GL_POINT_SPRITE_COORD_ORIGIN:
         CHECK_EXTENSION_B(ARB_point_sprite, pname);
         *params = ENUM_TO_BOOL(ctx->Point.SpriteOrigin);
         break;

      /* GL_SGIS_generate_mipmap */
      case GL_GENERATE_MIPMAP_HINT_SGIS:
         CHECK_EXTENSION_B(SGIS_generate_mipmap, pname);
         *params = ENUM_TO_BOOL(ctx->Hint.GenerateMipmap);
         break;

#if FEATURE_NV_vertex_program
      case GL_VERTEX_PROGRAM_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = ctx->VertexProgram.Enabled;
         break;
      case GL_VERTEX_PROGRAM_POINT_SIZE_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = ctx->VertexProgram.PointSizeEnabled;
         break;
      case GL_VERTEX_PROGRAM_TWO_SIDE_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = ctx->VertexProgram.TwoSideEnabled;
         break;
      case GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = (ctx->Const.MaxProgramMatrixStackDepth > 0) ? GL_TRUE : GL_FALSE;
         break;
      case GL_MAX_TRACK_MATRICES_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = (ctx->Const.MaxProgramMatrices > 0) ? GL_TRUE : GL_FALSE;
         break;
      case GL_CURRENT_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = GL_TRUE;
         break;
      case GL_CURRENT_MATRIX_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         for (i = 0; i < 16; i++)
            params[i] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[i]);
         break;
      case GL_VERTEX_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         if (ctx->VertexProgram.Current &&
             ctx->VertexProgram.Current->Base.Id != 0)
            *params = GL_TRUE;
         else
            *params = GL_FALSE;
         break;
      case GL_PROGRAM_ERROR_POSITION_NV:
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         *params = (ctx->Program.ErrorPos != 0) ? GL_TRUE : GL_FALSE;
         break;
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
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_VERTEX_ATTRIB_ARRAY0_NV;
            *params = ctx->Array.VertexAttrib[n].Enabled;
         }
         break;
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
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP1_VERTEX_ATTRIB0_4_NV;
            *params = ctx->Eval.Map1Attrib[n];
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
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP2_VERTEX_ATTRIB0_4_NV;
            *params = ctx->Eval.Map2Attrib[n];
         }
         break;
#endif /* FEATURE_NV_vertex_program */

#if FEATURE_NV_fragment_program
      case GL_FRAGMENT_PROGRAM_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         *params = ctx->FragmentProgram.Enabled;
         break;
      case GL_MAX_TEXTURE_COORDS_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         *params = INT_TO_BOOL(ctx->Const.MaxTextureCoordUnits);
         break;
      case GL_MAX_TEXTURE_IMAGE_UNITS_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         *params = INT_TO_BOOL(ctx->Const.MaxTextureImageUnits);
         break;
      case GL_FRAGMENT_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         if (ctx->VertexProgram.Current &&
             ctx->VertexProgram.Current->Base.Id != 0)
            *params = GL_TRUE;
         else
            *params = GL_FALSE;
         break;
      case GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         *params = MAX_NV_FRAGMENT_PROGRAM_PARAMS ? GL_TRUE : GL_FALSE;
         break;
#endif /* FEATURE_NV_fragment_program */

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION_B(NV_texture_rectangle, pname);
         *params = _mesa_IsEnabled(GL_TEXTURE_RECTANGLE_NV);
         break;
      case GL_TEXTURE_BINDING_RECTANGLE_NV:
         CHECK_EXTENSION_B(NV_texture_rectangle, pname);
         *params = INT_TO_BOOL(textureUnit->CurrentRect->Name);
         break;
      case GL_MAX_RECTANGLE_TEXTURE_SIZE_NV:
         CHECK_EXTENSION_B(NV_texture_rectangle, pname);
         *params = INT_TO_BOOL(ctx->Const.MaxTextureRectSize);
         break;

      /* GL_EXT_stencil_two_side */
      case GL_STENCIL_TEST_TWO_SIDE_EXT:
         CHECK_EXTENSION_B(EXT_stencil_two_side, pname);
         *params = ctx->Stencil.TestTwoSide;
         break;
      case GL_ACTIVE_STENCIL_FACE_EXT:
         CHECK_EXTENSION_B(EXT_stencil_two_side, pname);
         *params = ENUM_TO_BOOL(ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT);
         break;

      /* GL_NV_light_max_exponent */
      case GL_MAX_SHININESS_NV:
         *params = FLOAT_TO_BOOL(ctx->Const.MaxShininess);
         break;
      case GL_MAX_SPOT_EXPONENT_NV:
         *params = FLOAT_TO_BOOL(ctx->Const.MaxSpotExponent);
         break;
         
#if FEATURE_ARB_vertex_buffer_object
      case GL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.ArrayBufferObj->Name);
         break;
      case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.Vertex.BufferObj->Name);
         break;
      case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.Normal.BufferObj->Name);
         break;
      case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.Color.BufferObj->Name);
         break;
      case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.Index.BufferObj->Name);
         break;
      case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.TexCoord[clientUnit].BufferObj->Name);
         break;
      case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.EdgeFlag.BufferObj->Name);
         break;
      case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.SecondaryColor.BufferObj->Name);
         break;
      case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.FogCoord.BufferObj->Name);
         break;
      /*case GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB: - not supported */
      case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_B(ARB_vertex_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Array.ElementArrayBufferObj->Name);
         break;
#endif
#if FEATURE_EXT_pixel_buffer_object
      case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_B(EXT_pixel_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Pack.BufferObj->Name);
         break;
      case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_B(EXT_pixel_buffer_object, pname);
         *params = INT_TO_BOOL(ctx->Unpack.BufferObj->Name);
         break;
#endif

#if FEATURE_ARB_vertex_program
      /* GL_NV_vertex_program and GL_ARB_fragment_program define others */
      case GL_MAX_VERTEX_ATTRIBS_ARB:
         CHECK_EXTENSION_B(ARB_vertex_program, pname);
         *params = (ctx->Const.MaxVertexProgramAttribs > 0) ? GL_TRUE : GL_FALSE;
         break;
#endif

#if FEATURE_ARB_fragment_program
      case GL_FRAGMENT_PROGRAM_ARB:
         CHECK_EXTENSION_B(ARB_fragment_program, pname);
         *params = ctx->FragmentProgram.Enabled;
         break;
      case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
         CHECK_EXTENSION_B(ARB_fragment_program, pname);
         params[0] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[0]);
         params[1] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[4]);
         params[2] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[8]);
         params[3] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[12]);
         params[4] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[1]);
         params[5] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[5]);
         params[6] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[9]);
         params[7] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[13]);
         params[8] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[2]);
         params[9] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[6]);
         params[10] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[10]);
         params[11] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[14]);
         params[12] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[3]);
         params[13] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[7]);
         params[14] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[11]);
         params[15] = FLOAT_TO_BOOL(ctx->CurrentStack->Top->m[15]);
         break;
      /* Remaining ARB_fragment_program queries alias with
       * the GL_NV_fragment_program queries.
       */
#endif

      /* GL_EXT_depth_bounds_test */
      case GL_DEPTH_BOUNDS_TEST_EXT:
         CHECK_EXTENSION_B(EXT_depth_bounds_test, pname);
         params[0] = ctx->Depth.BoundsTest;
         break;
      case GL_DEPTH_BOUNDS_EXT:
         CHECK_EXTENSION_B(EXT_depth_bounds_test, pname);
         params[0] = FLOAT_TO_BOOL(ctx->Depth.BoundsMin);
         params[1] = FLOAT_TO_BOOL(ctx->Depth.BoundsMax);
         break;

#if FEATURE_MESA_program_debug
      case GL_FRAGMENT_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_B(MESA_program_debug, pname);
         *params = ctx->FragmentProgram.CallbackEnabled;
         break;
      case GL_VERTEX_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_B(MESA_program_debug, pname);
         *params = ctx->VertexProgram.CallbackEnabled;
         break;
      case GL_FRAGMENT_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_B(MESA_program_debug, pname);
         *params = INT_TO_BOOL(ctx->FragmentProgram.CurrentPosition);
         break;
      case GL_VERTEX_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_B(MESA_program_debug, pname);
         *params = INT_TO_BOOL(ctx->VertexProgram.CurrentPosition);
         break;
#endif

      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetBooleanv(pname=0x%x)", pname);
   }
}


/**
 * Get the value(s) of a selected parameter.
 *
 * \param pname parameter to be returned.
 * \param params will hold the value(s) of the speficifed parameter.
 *
 * \sa glGetDoublev().
 *
 * Tries to get the specified parameter via dd_function_table::GetDoublev,
 * otherwise gets the specified parameter from the current context, converting
 * it value into GLdouble.
 */
void GLAPIENTRY
_mesa_GetDoublev( GLenum pname, GLdouble *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   const GLuint clientUnit = ctx->Array.ActiveTexture;
   const GLuint texUnit = ctx->Texture.CurrentUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!params)
      return;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetDoublev %s\n", _mesa_lookup_enum_by_nr(pname));

   if (!ctx->_CurrentProgram) {
      /* We need this in order to get correct results for
       * GL_OCCLUSION_TEST_RESULT_HP.  There might be other important cases.
       */
      FLUSH_VERTICES(ctx, 0);
   }

   if (ctx->Driver.GetDoublev && (*ctx->Driver.GetDoublev)(ctx, pname, params))
      return;

   switch (pname) {
      case GL_ACCUM_RED_BITS:
         *params = (GLdouble) ctx->Visual.accumRedBits;
         break;
      case GL_ACCUM_GREEN_BITS:
         *params = (GLdouble) ctx->Visual.accumGreenBits;
         break;
      case GL_ACCUM_BLUE_BITS:
         *params = (GLdouble) ctx->Visual.accumBlueBits;
         break;
      case GL_ACCUM_ALPHA_BITS:
         *params = (GLdouble) ctx->Visual.accumAlphaBits;
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = (GLdouble) ctx->Accum.ClearColor[0];
         params[1] = (GLdouble) ctx->Accum.ClearColor[1];
         params[2] = (GLdouble) ctx->Accum.ClearColor[2];
         params[3] = (GLdouble) ctx->Accum.ClearColor[3];
         break;
      case GL_ALPHA_BIAS:
         *params = (GLdouble) ctx->Pixel.AlphaBias;
         break;
      case GL_ALPHA_BITS:
         *params = (GLdouble) ctx->Visual.alphaBits;
         break;
      case GL_ALPHA_SCALE:
         *params = (GLdouble) ctx->Pixel.AlphaScale;
         break;
      case GL_ALPHA_TEST:
         *params = (GLdouble) ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = ENUM_TO_DOUBLE(ctx->Color.AlphaFunc);
         break;
      case GL_ALPHA_TEST_REF:
         *params = (GLdouble) ctx->Color.AlphaRef;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = (GLdouble ) (ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = (GLdouble) ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (GLdouble) ctx->Visual.numAuxBuffers;
         break;
      case GL_BLEND:
         *params = (GLdouble) ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_SRC_RGB_EXT:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_DST_RGB_EXT:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC_ALPHA_EXT:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendSrcA);
         break;
      case GL_BLEND_DST_ALPHA_EXT:
         *params = ENUM_TO_DOUBLE(ctx->Color.BlendDstA);
         break;
      case GL_BLEND_EQUATION:
	 *params = ENUM_TO_DOUBLE(ctx->Color.BlendEquationRGB);
	 break;
      case GL_BLEND_EQUATION_ALPHA_EXT:
	 *params = ENUM_TO_DOUBLE(ctx->Color.BlendEquationA);
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = (GLdouble) ctx->Color.BlendColor[0];
	 params[1] = (GLdouble) ctx->Color.BlendColor[1];
	 params[2] = (GLdouble) ctx->Color.BlendColor[2];
	 params[3] = (GLdouble) ctx->Color.BlendColor[3];
	 break;
      case GL_BLUE_BIAS:
         *params = (GLdouble) ctx->Pixel.BlueBias;
         break;
      case GL_BLUE_BITS:
         *params = (GLdouble) ctx->Visual.blueBits;
         break;
      case GL_BLUE_SCALE:
         *params = (GLdouble) ctx->Pixel.BlueScale;
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLdouble) (ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         if (ctx->Transform.ClipPlanesEnabled & (1 << (pname - GL_CLIP_PLANE0)))
            *params = 1.0;
         else
            *params = 0.0;
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = (GLdouble) ctx->Color.ClearColor[0];
         params[1] = (GLdouble) ctx->Color.ClearColor[1];
         params[2] = (GLdouble) ctx->Color.ClearColor[2];
         params[3] = (GLdouble) ctx->Color.ClearColor[3];
         break;
      case GL_COLOR_MATERIAL:
         *params = (GLdouble) ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = ENUM_TO_DOUBLE(ctx->Light.ColorMaterialFace);
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = ENUM_TO_DOUBLE(ctx->Light.ColorMaterialMode);
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? 1.0 : 0.0;
         params[1] = ctx->Color.ColorMask[GCOMP] ? 1.0 : 0.0;
         params[2] = ctx->Color.ColorMask[BCOMP] ? 1.0 : 0.0;
         params[3] = ctx->Color.ColorMask[ACOMP] ? 1.0 : 0.0;
         break;
      case GL_CULL_FACE:
         *params = (GLdouble) ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = ENUM_TO_DOUBLE(ctx->Polygon.CullFaceMode);
         break;
      case GL_CURRENT_COLOR:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0];
         params[1] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1];
         params[2] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2];
         params[3] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3];
         break;
      case GL_CURRENT_INDEX:
	 FLUSH_CURRENT(ctx, 0);
         *params = (GLdouble) ctx->Current.Index;
         break;
      case GL_CURRENT_NORMAL:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0];
         params[1] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1];
         params[2] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2];
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = (GLdouble) ctx->Current.RasterColor[0];
	 params[1] = (GLdouble) ctx->Current.RasterColor[1];
	 params[2] = (GLdouble) ctx->Current.RasterColor[2];
	 params[3] = (GLdouble) ctx->Current.RasterColor[3];
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 params[0] = (GLdouble) ctx->Current.RasterDistance;
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = (GLdouble) ctx->Current.RasterIndex;
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = (GLdouble) ctx->Current.RasterPos[0];
	 params[1] = (GLdouble) ctx->Current.RasterPos[1];
	 params[2] = (GLdouble) ctx->Current.RasterPos[2];
	 params[3] = (GLdouble) ctx->Current.RasterPos[3];
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	 params[0] = (GLdouble) ctx->Current.RasterTexCoords[texUnit][0];
	 params[1] = (GLdouble) ctx->Current.RasterTexCoords[texUnit][1];
	 params[2] = (GLdouble) ctx->Current.RasterTexCoords[texUnit][2];
	 params[3] = (GLdouble) ctx->Current.RasterTexCoords[texUnit][3];
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	 *params = (GLdouble) ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
	 FLUSH_CURRENT(ctx, 0);
	 params[0] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][0];
	 params[1] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][1];
	 params[2] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][2];
	 params[3] = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][3];
	 break;
      case GL_DEPTH_BIAS:
	 *params = (GLdouble) ctx->Pixel.DepthBias;
	 break;
      case GL_DEPTH_BITS:
	 *params = (GLdouble) ctx->Visual.depthBits;
	 break;
      case GL_DEPTH_CLEAR_VALUE:
	 *params = (GLdouble) ctx->Depth.Clear;
	 break;
      case GL_DEPTH_FUNC:
	 *params = ENUM_TO_DOUBLE(ctx->Depth.Func);
	 break;
      case GL_DEPTH_RANGE:
         params[0] = (GLdouble) ctx->Viewport.Near;
         params[1] = (GLdouble) ctx->Viewport.Far;
	 break;
      case GL_DEPTH_SCALE:
	 *params = (GLdouble) ctx->Pixel.DepthScale;
	 break;
      case GL_DEPTH_TEST:
	 *params = (GLdouble) ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = (GLdouble) ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = (GLdouble) ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = (GLdouble) ctx->Visual.doubleBufferMode;
	 break;
      case GL_DRAW_BUFFER:
	 *params = ENUM_TO_DOUBLE(ctx->Color.DrawBuffer);
	 break;
      case GL_EDGE_FLAG:
	 FLUSH_CURRENT(ctx, 0);
	 *params = (GLdouble) ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         *params = (GLdouble) ctx->Feedback.BufferSize;
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Feedback.Type);
         break;
      case GL_FOG:
	 *params = (GLdouble) ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
	 params[0] = (GLdouble) ctx->Fog.Color[0];
	 params[1] = (GLdouble) ctx->Fog.Color[1];
	 params[2] = (GLdouble) ctx->Fog.Color[2];
	 params[3] = (GLdouble) ctx->Fog.Color[3];
	 break;
      case GL_FOG_DENSITY:
	 *params = (GLdouble) ctx->Fog.Density;
	 break;
      case GL_FOG_END:
	 *params = (GLdouble) ctx->Fog.End;
	 break;
      case GL_FOG_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.Fog);
	 break;
      case GL_FOG_INDEX:
	 *params = (GLdouble) ctx->Fog.Index;
	 break;
      case GL_FOG_MODE:
	 *params = ENUM_TO_DOUBLE(ctx->Fog.Mode);
	 break;
      case GL_FOG_START:
	 *params = (GLdouble) ctx->Fog.Start;
	 break;
      case GL_FRONT_FACE:
	 *params = ENUM_TO_DOUBLE(ctx->Polygon.FrontFace);
	 break;
      case GL_GREEN_BIAS:
         *params = (GLdouble) ctx->Pixel.GreenBias;
         break;
      case GL_GREEN_BITS:
         *params = (GLdouble) ctx->Visual.greenBits;
         break;
      case GL_GREEN_SCALE:
         *params = (GLdouble) ctx->Pixel.GreenScale;
         break;
      case GL_INDEX_BITS:
         *params = (GLdouble) ctx->Visual.indexBits;
	 break;
      case GL_INDEX_CLEAR_VALUE:
         *params = (GLdouble) ctx->Color.ClearIndex;
	 break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual.rgbMode ? 0.0 : 1.0;
	 break;
      case GL_INDEX_OFFSET:
	 *params = (GLdouble) ctx->Pixel.IndexOffset;
	 break;
      case GL_INDEX_SHIFT:
	 *params = (GLdouble) ctx->Pixel.IndexShift;
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = (GLdouble) ctx->Color.IndexMask;
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = (GLdouble) ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = (GLdouble) ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = (GLdouble) ctx->Light.Model.Ambient[0];
	 params[1] = (GLdouble) ctx->Light.Model.Ambient[1];
	 params[2] = (GLdouble) ctx->Light.Model.Ambient[2];
	 params[3] = (GLdouble) ctx->Light.Model.Ambient[3];
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = (GLdouble) ctx->Light.Model.ColorControl;
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = (GLdouble) ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = (GLdouble) ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = (GLdouble) ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.LineSmooth);
	 break;
      case GL_LINE_STIPPLE:
	 *params = (GLdouble) ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
         *params = (GLdouble) ctx->Line.StipplePattern;
         break;
      case GL_LINE_STIPPLE_REPEAT:
         *params = (GLdouble) ctx->Line.StippleFactor;
         break;
      case GL_LINE_WIDTH:
	 *params = (GLdouble) ctx->Line.Width;
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = (GLdouble) ctx->Const.LineWidthGranularity;
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = (GLdouble) ctx->Const.MinLineWidthAA;
	 params[1] = (GLdouble) ctx->Const.MaxLineWidthAA;
	 break;
      case GL_ALIASED_LINE_WIDTH_RANGE:
	 params[0] = (GLdouble) ctx->Const.MinLineWidth;
	 params[1] = (GLdouble) ctx->Const.MaxLineWidth;
	 break;
      case GL_LIST_BASE:
	 *params = (GLdouble) ctx->List.ListBase;
	 break;
      case GL_LIST_INDEX:
	 *params = (GLdouble) ctx->ListState.CurrentListNum;
	 break;
      case GL_LIST_MODE:
         if (!ctx->CompileFlag)
            *params = 0.0;
         else if (ctx->ExecuteFlag)
            *params = ENUM_TO_DOUBLE(GL_COMPILE_AND_EXECUTE);
         else
            *params = ENUM_TO_DOUBLE(GL_COMPILE);
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = (GLdouble) ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = (GLdouble) ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
         *params = ENUM_TO_DOUBLE(ctx->Color.LogicOp);
	 break;
      case GL_MAP1_COLOR_4:
	 *params = (GLdouble) ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = (GLdouble) ctx->Eval.MapGrid1u1;
	 params[1] = (GLdouble) ctx->Eval.MapGrid1u2;
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = (GLdouble) ctx->Eval.MapGrid1un;
	 break;
      case GL_MAP1_INDEX:
	 *params = (GLdouble) ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = (GLdouble) ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = (GLdouble) ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = (GLdouble) ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = (GLdouble) ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = (GLdouble) ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = (GLdouble) ctx->Eval.MapGrid2u1;
	 params[1] = (GLdouble) ctx->Eval.MapGrid2u2;
	 params[2] = (GLdouble) ctx->Eval.MapGrid2v1;
	 params[3] = (GLdouble) ctx->Eval.MapGrid2v2;
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = (GLdouble) ctx->Eval.MapGrid2un;
	 params[1] = (GLdouble) ctx->Eval.MapGrid2vn;
	 break;
      case GL_MAP2_INDEX:
	 *params = (GLdouble) ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = (GLdouble) ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = (GLdouble) ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = (GLdouble) ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = (GLdouble) ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = (GLdouble) ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = (GLdouble) ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = ENUM_TO_DOUBLE(ctx->Transform.MatrixMode);
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	 *params = (GLdouble) MAX_ATTRIB_STACK_DEPTH;
	 break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLdouble) MAX_CLIENT_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIP_PLANES:
	 *params = (GLdouble) ctx->Const.MaxClipPlanes;
	 break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = (GLdouble) ctx->Const.MaxArrayLockSize;
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = (GLdouble) ctx->Const.MaxArrayLockSize;
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = (GLdouble) MAX_EVAL_ORDER;
	 break;
      case GL_MAX_LIGHTS:
	 *params = (GLdouble) ctx->Const.MaxLights;
	 break;
      case GL_MAX_LIST_NESTING:
	 *params = (GLdouble) MAX_LIST_NESTING;
	 break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	 *params = (GLdouble) MAX_MODELVIEW_STACK_DEPTH;
	 break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = (GLdouble) MAX_NAME_STACK_DEPTH;
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = (GLdouble) MAX_PIXEL_MAP_TABLE;
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	 *params = (GLdouble) MAX_PROJECTION_STACK_DEPTH;
	 break;
      case GL_MAX_TEXTURE_SIZE:
         *params = (GLdouble) (1 << (ctx->Const.MaxTextureLevels - 1));
	 break;
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = (GLdouble) (1 << (ctx->Const.Max3DTextureLevels - 1));
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = (GLdouble) MAX_TEXTURE_STACK_DEPTH;
	 break;
      case GL_MAX_VIEWPORT_DIMS:
         params[0] = (GLdouble) MAX_WIDTH;
         params[1] = (GLdouble) MAX_HEIGHT;
         break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->ModelviewMatrixStack.Top->m[i];
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = (GLdouble) (ctx->ModelviewMatrixStack.Depth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = (GLdouble) ctx->Select.NameStackDepth;
	 break;
      case GL_NORMALIZE:
	 *params = (GLdouble) ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = (GLdouble) ctx->Pack.Alignment;
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = (GLdouble) ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = (GLdouble) ctx->Pack.RowLength;
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = (GLdouble) ctx->Pack.SkipPixels;
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = (GLdouble) ctx->Pack.SkipRows;
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = (GLdouble) ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = (GLdouble) ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = (GLdouble) ctx->Pack.ImageHeight;
         break;
      case GL_PACK_INVERT_MESA:
         *params = (GLdouble) ctx->Pack.Invert;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.PerspectiveCorrection);
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapAtoAsize;
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapBtoBsize;
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapGtoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoAsize;
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoBsize;
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoIsize;
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapItoRsize;
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapRtoRsize;
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = (GLdouble) ctx->Pixel.MapStoSsize;
	 break;
      case GL_POINT_SIZE:
         *params = (GLdouble) ctx->Point.Size;
         break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = (GLdouble) ctx->Const.PointSizeGranularity;
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = (GLdouble) ctx->Const.MinPointSizeAA;
	 params[1] = (GLdouble) ctx->Const.MaxPointSizeAA;
	 break;
      case GL_ALIASED_POINT_SIZE_RANGE:
	 params[0] = (GLdouble) ctx->Const.MinPointSize;
	 params[1] = (GLdouble) ctx->Const.MaxPointSize;
	 break;
      case GL_POINT_SMOOTH:
	 *params = (GLdouble) ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.PointSmooth);
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = (GLdouble) (ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = (GLdouble) (ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = (GLdouble) (ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = (GLdouble) (ctx->Point.Params[0]);
	 params[1] = (GLdouble) (ctx->Point.Params[1]);
	 params[2] = (GLdouble) (ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = ENUM_TO_DOUBLE(ctx->Polygon.FrontMode);
	 params[1] = ENUM_TO_DOUBLE(ctx->Polygon.BackMode);
	 break;
      case GL_POLYGON_OFFSET_BIAS_EXT:  /* GL_EXT_polygon_offset */
         *params = (GLdouble) ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_OFFSET_FACTOR:
         *params = (GLdouble) ctx->Polygon.OffsetFactor;
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = (GLdouble) ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_SMOOTH:
	 *params = (GLdouble) ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = ENUM_TO_DOUBLE(ctx->Hint.PolygonSmooth);
	 break;
      case GL_POLYGON_STIPPLE:
         *params = (GLdouble) ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->ProjectionMatrixStack.Top->m[i];
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = (GLdouble) (ctx->ProjectionMatrixStack.Depth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = ENUM_TO_DOUBLE(ctx->Pixel.ReadBuffer);
	 break;
      case GL_RED_BIAS:
         *params = (GLdouble) ctx->Pixel.RedBias;
         break;
      case GL_RED_BITS:
         *params = (GLdouble) ctx->Visual.redBits;
         break;
      case GL_RED_SCALE:
         *params = (GLdouble) ctx->Pixel.RedScale;
         break;
      case GL_RENDER_MODE:
	 *params = ENUM_TO_DOUBLE(ctx->RenderMode);
	 break;
      case GL_RESCALE_NORMAL:
         *params = (GLdouble) ctx->Transform.RescaleNormals;
         break;
      case GL_RGBA_MODE:
	 *params = (GLdouble) ctx->Visual.rgbMode;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = (GLdouble) ctx->Scissor.X;
	 params[1] = (GLdouble) ctx->Scissor.Y;
	 params[2] = (GLdouble) ctx->Scissor.Width;
	 params[3] = (GLdouble) ctx->Scissor.Height;
	 break;
      case GL_SCISSOR_TEST:
	 *params = (GLdouble) ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = (GLdouble) ctx->Select.BufferSize;
         break;
      case GL_SHADE_MODEL:
	 *params = ENUM_TO_DOUBLE(ctx->Light.ShadeModel);
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = (GLdouble) ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
         *params = (GLdouble) ctx->Visual.stencilBits;
         break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = (GLdouble) ctx->Stencil.Clear;
	 break;
      case GL_STENCIL_FAIL:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_FUNC:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.Function[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = ENUM_TO_DOUBLE(ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_REF:
	 *params = (GLdouble) ctx->Stencil.Ref[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_TEST:
	 *params = (GLdouble) ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = (GLdouble) ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = (GLdouble) ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace];
	 break;
      case GL_STEREO:
	 *params = (GLdouble) ctx->Visual.stereoMode;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = (GLdouble) ctx->Const.SubPixelBits;
	 break;
      case GL_TEXTURE_1D:
         *params = _mesa_IsEnabled(GL_TEXTURE_1D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_2D:
         *params = _mesa_IsEnabled(GL_TEXTURE_2D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_3D:
         *params = _mesa_IsEnabled(GL_TEXTURE_3D) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = (GLdouble) textureUnit->Current1D->Name;
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = (GLdouble) textureUnit->Current2D->Name;
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = (GLdouble) textureUnit->Current3D->Name;
          break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = (GLdouble) textureUnit->EnvColor[0];
	 params[1] = (GLdouble) textureUnit->EnvColor[1];
	 params[2] = (GLdouble) textureUnit->EnvColor[2];
	 params[3] = (GLdouble) textureUnit->EnvColor[3];
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = ENUM_TO_DOUBLE(textureUnit->EnvMode);
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? 1.0 : 0.0;
	 break;
      case GL_TEXTURE_MATRIX:
         for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->TextureMatrixStack[texUnit].Top->m[i];
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = (GLdouble) (ctx->TextureMatrixStack[texUnit].Depth + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = (GLdouble) ctx->Unpack.Alignment;
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = (GLdouble) ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = (GLdouble) ctx->Unpack.RowLength;
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = (GLdouble) ctx->Unpack.SkipPixels;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = (GLdouble) ctx->Unpack.SkipRows;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = (GLdouble) ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = (GLdouble) ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = (GLdouble) ctx->Unpack.ImageHeight;
         break;
      case GL_UNPACK_CLIENT_STORAGE_APPLE:
         *params = (GLdouble) ctx->Unpack.ClientStorage;
         break;
      case GL_VIEWPORT:
	 params[0] = (GLdouble) ctx->Viewport.X;
	 params[1] = (GLdouble) ctx->Viewport.Y;
	 params[2] = (GLdouble) ctx->Viewport.Width;
	 params[3] = (GLdouble) ctx->Viewport.Height;
	 break;
      case GL_ZOOM_X:
	 *params = (GLdouble) ctx->Pixel.ZoomX;
	 break;
      case GL_ZOOM_Y:
	 *params = (GLdouble) ctx->Pixel.ZoomY;
	 break;
      case GL_VERTEX_ARRAY:
         *params = (GLdouble) ctx->Array.Vertex.Enabled;
         break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = (GLdouble) ctx->Array.Vertex.Size;
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Vertex.Type);
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Vertex.Stride;
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_NORMAL_ARRAY:
         *params = (GLdouble) ctx->Array.Normal.Enabled;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Normal.Type);
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Normal.Stride;
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_COLOR_ARRAY:
         *params = (GLdouble) ctx->Array.Color.Enabled;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = (GLdouble) ctx->Array.Color.Size;
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Color.Type);
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Color.Stride;
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_INDEX_ARRAY:
         *params = (GLdouble) ctx->Array.Index.Enabled;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.Index.Type);
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.Index.Stride;
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         *params = (GLdouble) ctx->Array.TexCoord[clientUnit].Enabled;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = (GLdouble) ctx->Array.TexCoord[clientUnit].Size;
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ENUM_TO_DOUBLE(ctx->Array.TexCoord[clientUnit].Type);
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.TexCoord[clientUnit].Stride;
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_EDGE_FLAG_ARRAY:
         *params = (GLdouble) ctx->Array.EdgeFlag.Enabled;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = (GLdouble) ctx->Array.EdgeFlag.Stride;
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;

      /* GL_ARB_multitexture */
      case GL_MAX_TEXTURE_UNITS_ARB:
         CHECK_EXTENSION_D(ARB_multitexture, pname);
         *params = (GLdouble) MIN2(ctx->Const.MaxTextureImageUnits,
                                   ctx->Const.MaxTextureCoordUnits);
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_D(ARB_multitexture, pname);
         *params = (GLdouble) (GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit);
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_D(ARB_multitexture, pname);
         *params = (GLdouble) (GL_TEXTURE0_ARB + clientUnit);
         break;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION_D(ARB_texture_cube_map, pname);
         *params = (GLdouble) _mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB);
         break;
      case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
         CHECK_EXTENSION_D(ARB_texture_cube_map, pname);
         *params = (GLdouble) textureUnit->CurrentCubeMap->Name;
         break;
      case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
         CHECK_EXTENSION_D(ARB_texture_cube_map, pname);
         *params = (GLdouble) (1 << (ctx->Const.MaxCubeTextureLevels - 1));
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSION_HINT_ARB:
         CHECK_EXTENSION_D(ARB_texture_compression, pname);
         *params = (GLdouble) ctx->Hint.TextureCompression;
         break;
      case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_D(ARB_texture_compression, pname);
         *params = (GLdouble) _mesa_get_compressed_formats(ctx, NULL);
         break;
      case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_D(ARB_texture_compression, pname);
         {
            GLint formats[100];
            GLuint i, n;
            n = _mesa_get_compressed_formats(ctx, formats);
            for (i = 0; i < n; i++)
               params[i] = (GLdouble) formats[i];
         }
         break;

      /* GL_EXT_compiled_vertex_array */
      case GL_ARRAY_ELEMENT_LOCK_FIRST_EXT:
	 *params = (GLdouble) ctx->Array.LockFirst;
	 break;
      case GL_ARRAY_ELEMENT_LOCK_COUNT_EXT:
	 *params = (GLdouble) ctx->Array.LockCount;
	 break;

      /* GL_ARB_transpose_matrix */
      case GL_TRANSPOSE_COLOR_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ColorMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLdouble) tm[i];
            }
         }
         break;
      case GL_TRANSPOSE_MODELVIEW_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ModelviewMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLdouble) tm[i];
            }
         }
         break;
      case GL_TRANSPOSE_PROJECTION_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ProjectionMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLdouble) tm[i];
            }
         }
         break;
      case GL_TRANSPOSE_TEXTURE_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->TextureMatrixStack[texUnit].Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLdouble) tm[i];
            }
         }
         break;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         CHECK_EXTENSION_D(HP_occlusion_test, pname);
         *params = (GLdouble) ctx->Depth.OcclusionTest;
         break;
      case GL_OCCLUSION_TEST_RESULT_HP:
         CHECK_EXTENSION_D(HP_occlusion_test, pname);
         if (ctx->Depth.OcclusionTest)
            *params = (GLdouble) ctx->OcclusionResult;
         else
            *params = (GLdouble) ctx->OcclusionResultSaved;
         /* reset flag now */
         ctx->OcclusionResult = GL_FALSE;
         ctx->OcclusionResultSaved = GL_FALSE;
         break;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         *params = (GLdouble) ctx->Pixel.PixelTextureEnabled;
         break;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         *params = (GLdouble) ctx->Pixel.PixelTextureEnabled;
         break;
      case GL_PIXEL_TEX_GEN_MODE_SGIX:
         *params = (GLdouble) pixel_texgen_mode(ctx);
         break;

      /* GL_SGI_color_matrix (also in 1.2 imaging) */
      case GL_COLOR_MATRIX_SGI:
         for (i=0;i<16;i++) {
	    params[i] = (GLdouble) ctx->ColorMatrixStack.Top->m[i];
	 }
	 break;
      case GL_COLOR_MATRIX_STACK_DEPTH_SGI:
         *params = (GLdouble) (ctx->ColorMatrixStack.Depth + 1);
         break;
      case GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI:
         *params = (GLdouble) MAX_COLOR_STACK_DEPTH;
         break;
      case GL_POST_COLOR_MATRIX_RED_SCALE_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixScale[0];
         break;
      case GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixScale[1];
         break;
      case GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixScale[2];
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixScale[3];
         break;
      case GL_POST_COLOR_MATRIX_RED_BIAS_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixBias[0];
         break;
      case GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixBias[1];
         break;
      case GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixBias[2];
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixBias[3];
         break;

      /* GL_EXT_convolution (also in 1.2 imaging) */
      case GL_CONVOLUTION_1D_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.Convolution1DEnabled;
         break;
      case GL_CONVOLUTION_2D:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.Convolution2DEnabled;
         break;
      case GL_SEPARABLE_2D:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.Separable2DEnabled;
         break;
      case GL_POST_CONVOLUTION_RED_SCALE_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionScale[0];
         break;
      case GL_POST_CONVOLUTION_GREEN_SCALE_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionScale[1];
         break;
      case GL_POST_CONVOLUTION_BLUE_SCALE_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionScale[2];
         break;
      case GL_POST_CONVOLUTION_ALPHA_SCALE_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionScale[3];
         break;
      case GL_POST_CONVOLUTION_RED_BIAS_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionBias[0];
         break;
      case GL_POST_CONVOLUTION_GREEN_BIAS_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionBias[1];
         break;
      case GL_POST_CONVOLUTION_BLUE_BIAS_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionBias[2];
         break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS_EXT:
         CHECK_EXTENSION_D(EXT_convolution, pname);
         *params = (GLdouble) ctx->Pixel.PostConvolutionBias[2];
         break;

      /* GL_EXT_histogram (also in 1.2 imaging) */
      case GL_HISTOGRAM:
         CHECK_EXTENSION_D(EXT_histogram, pname);
         *params = (GLdouble) ctx->Pixel.HistogramEnabled;
	 break;
      case GL_MINMAX:
         CHECK_EXTENSION_D(EXT_histogram, pname);
         *params = (GLdouble) ctx->Pixel.MinMaxEnabled;
         break;

      /* GL_SGI_color_table (also in 1.2 imaging */
      case GL_COLOR_TABLE_SGI:
         *params = (GLdouble) ctx->Pixel.ColorTableEnabled;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         *params = (GLdouble) ctx->Pixel.PostConvolutionColorTableEnabled;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         *params = (GLdouble) ctx->Pixel.PostColorMatrixColorTableEnabled;
         break;

      /* GL_SGI_texture_color_table */
      case GL_TEXTURE_COLOR_TABLE_SGI:
         CHECK_EXTENSION_D(SGI_texture_color_table, pname);
         *params = (GLdouble) textureUnit->ColorTableEnabled;
         break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION_D(EXT_secondary_color, pname);
	 *params = (GLdouble) ctx->Fog.ColorSumEnabled;
	 break;
      case GL_CURRENT_SECONDARY_COLOR_EXT:
         CHECK_EXTENSION_D(EXT_secondary_color, pname);
	 FLUSH_CURRENT(ctx, 0);
         params[0] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0];
         params[1] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1];
         params[2] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2];
         params[3] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3];
	 break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         CHECK_EXTENSION_D(EXT_secondary_color, pname);
         *params = (GLdouble) ctx->Array.SecondaryColor.Enabled;
         break;
      case GL_SECONDARY_COLOR_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_D(EXT_secondary_color, pname);
	 *params = (GLdouble) ctx->Array.SecondaryColor.Type;
	 break;
      case GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_D(EXT_secondary_color, pname);
	 *params = (GLdouble) ctx->Array.SecondaryColor.Stride;
	 break;
      case GL_SECONDARY_COLOR_ARRAY_SIZE_EXT:
         CHECK_EXTENSION_D(EXT_secondary_color, pname);
	 *params = (GLdouble) ctx->Array.SecondaryColor.Size;
	 break;

      /* GL_EXT_fog_coord */
      case GL_CURRENT_FOG_COORDINATE_EXT:
         CHECK_EXTENSION_D(EXT_fog_coord, pname);
	 FLUSH_CURRENT(ctx, 0);
	 *params = (GLdouble) ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
	 break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         CHECK_EXTENSION_D(EXT_fog_coord, pname);
         *params = (GLdouble) ctx->Array.FogCoord.Enabled;
         break;
      case GL_FOG_COORDINATE_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_D(EXT_fog_coord, pname);
	 *params = (GLdouble) ctx->Array.FogCoord.Type;
	 break;
      case GL_FOG_COORDINATE_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_D(EXT_fog_coord, pname);
	 *params = (GLdouble) ctx->Array.FogCoord.Stride;
	 break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
         CHECK_EXTENSION_D(EXT_fog_coord, pname);
	 *params = (GLdouble) ctx->Fog.FogCoordinateSource;
	 break;

      /* GL_EXT_texture_lod_bias */
      case GL_MAX_TEXTURE_LOD_BIAS_EXT:
         *params = (GLdouble) ctx->Const.MaxTextureLodBias;
         break;

      /* GL_EXT_texture_filter_anisotropic */
      case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
         CHECK_EXTENSION_D(EXT_texture_filter_anisotropic, pname);
         *params = (GLdouble) ctx->Const.MaxTextureMaxAnisotropy;
	 break;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = (GLdouble) ctx->Multisample.Enabled;
         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = (GLdouble) ctx->Multisample.SampleAlphaToCoverage;
         break;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = (GLdouble) ctx->Multisample.SampleAlphaToOne;
         break;
      case GL_SAMPLE_COVERAGE_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = (GLdouble) ctx->Multisample.SampleCoverage;
         break;
      case GL_SAMPLE_COVERAGE_VALUE_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = ctx->Multisample.SampleCoverageValue;
         break;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = (GLdouble) ctx->Multisample.SampleCoverageInvert;
         break;
      case GL_SAMPLE_BUFFERS_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = 0.0; /* XXX fix someday */
         break;
      case GL_SAMPLES_ARB:
         CHECK_EXTENSION_D(ARB_multisample, pname);
         *params = 0.0; /* XXX fix someday */
         break;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION_D(IBM_rasterpos_clip, pname);
         *params = (GLdouble) ctx->Transform.RasterPositionUnclipped;
         break;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION2_D(NV_point_sprite, ARB_point_sprite, pname);
         *params = (GLdouble) ctx->Point.PointSprite;
         break;
      case GL_POINT_SPRITE_R_MODE_NV:
         CHECK_EXTENSION_D(NV_point_sprite, pname);
         *params = (GLdouble) ctx->Point.SpriteRMode;
         break;
      case GL_POINT_SPRITE_COORD_ORIGIN:
         CHECK_EXTENSION_D(ARB_point_sprite, pname);
         *params = (GLdouble) ctx->Point.SpriteOrigin;
         break;

      /* GL_SGIS_generate_mipmap */
      case GL_GENERATE_MIPMAP_HINT_SGIS:
         CHECK_EXTENSION_D(SGIS_generate_mipmap, pname);
         *params = (GLdouble) ctx->Hint.GenerateMipmap;
         break;

#if FEATURE_NV_vertex_program
      case GL_VERTEX_PROGRAM_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->VertexProgram.Enabled;
         break;
      case GL_VERTEX_PROGRAM_POINT_SIZE_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->VertexProgram.PointSizeEnabled;
         break;
      case GL_VERTEX_PROGRAM_TWO_SIDE_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->VertexProgram.TwoSideEnabled;
         break;
      case GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->Const.MaxProgramMatrixStackDepth;
         break;
      case GL_MAX_TRACK_MATRICES_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->Const.MaxProgramMatrices;
         break;
      case GL_CURRENT_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->CurrentStack->Depth + 1;
         break;
      case GL_CURRENT_MATRIX_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         for (i = 0; i < 16; i++)
            params[i] = (GLdouble) ctx->CurrentStack->Top->m[i];
         break;
      case GL_VERTEX_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         if (ctx->VertexProgram.Current)
            *params = (GLdouble) ctx->VertexProgram.Current->Base.Id;
         else
            *params = 0.0;
         break;
      case GL_PROGRAM_ERROR_POSITION_NV:
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         *params = (GLdouble) ctx->Program.ErrorPos;
         break;
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
         CHECK_EXTENSION_D(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_VERTEX_ATTRIB_ARRAY0_NV;
            *params = (GLdouble) ctx->Array.VertexAttrib[n].Enabled;
         }
         break;
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
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP1_VERTEX_ATTRIB0_4_NV;
            *params = (GLdouble) ctx->Eval.Map1Attrib[n];
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
         CHECK_EXTENSION_B(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP2_VERTEX_ATTRIB0_4_NV;
            *params = (GLdouble) ctx->Eval.Map2Attrib[n];
         }
         break;
#endif /* FEATURE_NV_vertex_program */

#if FEATURE_NV_fragment_program
      case GL_FRAGMENT_PROGRAM_NV:
         CHECK_EXTENSION_D(NV_fragment_program, pname);
         *params = (GLdouble) ctx->FragmentProgram.Enabled;
         break;
      case GL_MAX_TEXTURE_COORDS_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         *params = (GLdouble) ctx->Const.MaxTextureCoordUnits;
         break;
      case GL_MAX_TEXTURE_IMAGE_UNITS_NV:
         CHECK_EXTENSION_B(NV_fragment_program, pname);
         *params = (GLdouble) ctx->Const.MaxTextureImageUnits;
         break;
      case GL_FRAGMENT_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_D(NV_fragment_program, pname);
         if (ctx->FragmentProgram.Current)
            *params = (GLdouble) ctx->FragmentProgram.Current->Base.Id;
         else
            *params = 0.0;
         break;
      case GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV:
         CHECK_EXTENSION_D(NV_fragment_program, pname);
         *params = (GLdouble) MAX_NV_FRAGMENT_PROGRAM_PARAMS;
         break;
#endif /* FEATURE_NV_fragment_program */

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION_D(NV_texture_rectangle, pname);
         *params = (GLdouble) _mesa_IsEnabled(GL_TEXTURE_RECTANGLE_NV);
         break;
      case GL_TEXTURE_BINDING_RECTANGLE_NV:
         CHECK_EXTENSION_D(NV_texture_rectangle, pname);
         *params = (GLdouble) textureUnit->CurrentRect->Name;
         break;
      case GL_MAX_RECTANGLE_TEXTURE_SIZE_NV:
         CHECK_EXTENSION_D(NV_texture_rectangle, pname);
         *params = (GLdouble) ctx->Const.MaxTextureRectSize;
         break;

      /* GL_EXT_stencil_two_side */
      case GL_STENCIL_TEST_TWO_SIDE_EXT:
         CHECK_EXTENSION_D(EXT_stencil_two_side, pname);
         *params = (GLdouble) ctx->Stencil.TestTwoSide;
         break;
      case GL_ACTIVE_STENCIL_FACE_EXT:
         CHECK_EXTENSION_D(EXT_stencil_two_side, pname);
         *params = (GLdouble) (ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT);
         break;

      /* GL_NV_light_max_exponent */
      case GL_MAX_SHININESS_NV:
         *params = (GLdouble) ctx->Const.MaxShininess;
         break;
      case GL_MAX_SPOT_EXPONENT_NV:
         *params = (GLdouble) ctx->Const.MaxSpotExponent;
         break;

#if FEATURE_ARB_vertex_buffer_object
      case GL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.ArrayBufferObj->Name;
         break;
      case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.Vertex.BufferObj->Name;
         break;
      case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.Normal.BufferObj->Name;
         break;
      case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.Color.BufferObj->Name;
         break;
      case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.Index.BufferObj->Name;
         break;
      case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.TexCoord[clientUnit].BufferObj->Name;
         break;
      case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.EdgeFlag.BufferObj->Name;
         break;
      case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.SecondaryColor.BufferObj->Name;
         break;
      case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.FogCoord.BufferObj->Name;
         break;
      /*case GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB: - not supported */
      case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_D(ARB_vertex_buffer_object, pname);
         *params = (GLdouble) ctx->Array.ElementArrayBufferObj->Name;
         break;
#endif
#if FEATURE_EXT_pixel_buffer_object
      case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_D(EXT_pixel_buffer_object, pname);
         *params = (GLdouble) ctx->Pack.BufferObj->Name;
         break;
      case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_D(EXT_pixel_buffer_object, pname);
         *params = (GLdouble) ctx->Unpack.BufferObj->Name;
         break;
#endif

#if FEATURE_ARB_vertex_program
      /* GL_NV_vertex_program and GL_ARB_fragment_program define others */
      case GL_MAX_VERTEX_ATTRIBS_ARB:
         CHECK_EXTENSION_D(ARB_vertex_program, pname);
         *params = (GLdouble) ctx->Const.MaxVertexProgramAttribs;
         break;
#endif

#if FEATURE_ARB_fragment_program
      case GL_FRAGMENT_PROGRAM_ARB:
         CHECK_EXTENSION_D(ARB_fragment_program, pname);
         *params = (GLdouble) ctx->FragmentProgram.Enabled;
         break;
      case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
         CHECK_EXTENSION_D(ARB_fragment_program, pname);
         params[0] = ctx->CurrentStack->Top->m[0];
         params[1] = ctx->CurrentStack->Top->m[4];
         params[2] = ctx->CurrentStack->Top->m[8];
         params[3] = ctx->CurrentStack->Top->m[12];
         params[4] = ctx->CurrentStack->Top->m[1];
         params[5] = ctx->CurrentStack->Top->m[5];
         params[6] = ctx->CurrentStack->Top->m[9];
         params[7] = ctx->CurrentStack->Top->m[13];
         params[8] = ctx->CurrentStack->Top->m[2];
         params[9] = ctx->CurrentStack->Top->m[6];
         params[10] = ctx->CurrentStack->Top->m[10];
         params[11] = ctx->CurrentStack->Top->m[14];
         params[12] = ctx->CurrentStack->Top->m[3];
         params[13] = ctx->CurrentStack->Top->m[7];
         params[14] = ctx->CurrentStack->Top->m[11];
         params[15] = ctx->CurrentStack->Top->m[15];
         break;
      /* Remaining ARB_fragment_program queries alias with
       * the GL_NV_fragment_program queries.
       */
#endif

      /* GL_EXT_depth_bounds_test */
      case GL_DEPTH_BOUNDS_TEST_EXT:
         CHECK_EXTENSION_D(EXT_depth_bounds_test, pname);
         params[0] = (GLdouble) ctx->Depth.BoundsTest;
         break;
      case GL_DEPTH_BOUNDS_EXT:
         CHECK_EXTENSION_D(EXT_depth_bounds_test, pname);
         params[0] = ctx->Depth.BoundsMin;
         params[1] = ctx->Depth.BoundsMax;
         break;

#if FEATURE_MESA_program_debug
      case GL_FRAGMENT_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_D(MESA_program_debug, pname);
         *params = (GLdouble) ctx->FragmentProgram.CallbackEnabled;
         break;
      case GL_VERTEX_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_D(MESA_program_debug, pname);
         *params = (GLdouble) ctx->VertexProgram.CallbackEnabled;
         break;
      case GL_FRAGMENT_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_D(MESA_program_debug, pname);
         *params = (GLdouble) ctx->FragmentProgram.CurrentPosition;
         break;
      case GL_VERTEX_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_D(MESA_program_debug, pname);
         *params = (GLdouble) ctx->VertexProgram.CurrentPosition;
         break;
#endif

      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetDoublev(pname=0x%x)", pname);
   }
}


/**
 * Get the value(s) of a selected parameter.
 *
 * \param pname parameter to be returned.
 * \param params will hold the value(s) of the speficifed parameter.
 *
 * \sa glGetFloatv().
 *
 * Tries to get the specified parameter via dd_function_table::GetFloatv,
 * otherwise gets the specified parameter from the current context, converting
 * it value into GLfloat.
 */
void GLAPIENTRY
_mesa_GetFloatv( GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   const GLuint clientUnit = ctx->Array.ActiveTexture;
   const GLuint texUnit = ctx->Texture.CurrentUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!params)
      return;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetFloatv %s\n", _mesa_lookup_enum_by_nr(pname));

   if (!ctx->_CurrentProgram) {
      /* We need this in order to get correct results for
       * GL_OCCLUSION_TEST_RESULT_HP.  There might be other important cases.
       */
      FLUSH_VERTICES(ctx, 0);
   }

   if (ctx->Driver.GetFloatv && (*ctx->Driver.GetFloatv)(ctx, pname, params))
      return;

   switch (pname) {
      case GL_ACCUM_RED_BITS:
         *params = (GLfloat) ctx->Visual.accumRedBits;
         break;
      case GL_ACCUM_GREEN_BITS:
         *params = (GLfloat) ctx->Visual.accumGreenBits;
         break;
      case GL_ACCUM_BLUE_BITS:
         *params = (GLfloat) ctx->Visual.accumBlueBits;
         break;
      case GL_ACCUM_ALPHA_BITS:
         *params = (GLfloat) ctx->Visual.accumAlphaBits;
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = ctx->Accum.ClearColor[0];
         params[1] = ctx->Accum.ClearColor[1];
         params[2] = ctx->Accum.ClearColor[2];
         params[3] = ctx->Accum.ClearColor[3];
         break;
      case GL_ALPHA_BIAS:
         *params = ctx->Pixel.AlphaBias;
         break;
      case GL_ALPHA_BITS:
         *params = (GLfloat) ctx->Visual.alphaBits;
         break;
      case GL_ALPHA_SCALE:
         *params = ctx->Pixel.AlphaScale;
         break;
      case GL_ALPHA_TEST:
         *params = (GLfloat) ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = ENUM_TO_FLOAT(ctx->Color.AlphaFunc);
         break;
      case GL_ALPHA_TEST_REF:
         *params = (GLfloat) ctx->Color.AlphaRef;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = (GLfloat) (ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = (GLfloat) ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (GLfloat) ctx->Visual.numAuxBuffers;
         break;
      case GL_BLEND:
         *params = (GLfloat) ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_SRC_RGB_EXT:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendSrcRGB);
         break;
      case GL_BLEND_DST_RGB_EXT:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendDstRGB);
         break;
      case GL_BLEND_SRC_ALPHA_EXT:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendSrcA);
         break;
      case GL_BLEND_DST_ALPHA_EXT:
         *params = ENUM_TO_FLOAT(ctx->Color.BlendDstA);
         break;
      case GL_BLEND_EQUATION:
	 *params = ENUM_TO_FLOAT(ctx->Color.BlendEquationRGB);
	 break;
      case GL_BLEND_EQUATION_ALPHA_EXT:
	 *params = ENUM_TO_FLOAT(ctx->Color.BlendEquationA);
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = ctx->Color.BlendColor[0];
	 params[1] = ctx->Color.BlendColor[1];
	 params[2] = ctx->Color.BlendColor[2];
	 params[3] = ctx->Color.BlendColor[3];
	 break;
      case GL_BLUE_BIAS:
         *params = ctx->Pixel.BlueBias;
         break;
      case GL_BLUE_BITS:
         *params = (GLfloat) ctx->Visual.blueBits;
         break;
      case GL_BLUE_SCALE:
         *params = ctx->Pixel.BlueScale;
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLfloat) (ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         if (ctx->Transform.ClipPlanesEnabled & (1 << (pname - GL_CLIP_PLANE0)))
            *params = 1.0;
         else
            *params = 0.0;
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = ctx->Color.ClearColor[0];
         params[1] = ctx->Color.ClearColor[1];
         params[2] = ctx->Color.ClearColor[2];
         params[3] = ctx->Color.ClearColor[3];
         break;
      case GL_COLOR_MATERIAL:
         *params = (GLfloat) ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = ENUM_TO_FLOAT(ctx->Light.ColorMaterialFace);
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = ENUM_TO_FLOAT(ctx->Light.ColorMaterialMode);
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? 1.0F : 0.0F;
         params[1] = ctx->Color.ColorMask[GCOMP] ? 1.0F : 0.0F;
         params[2] = ctx->Color.ColorMask[BCOMP] ? 1.0F : 0.0F;
         params[3] = ctx->Color.ColorMask[ACOMP] ? 1.0F : 0.0F;
         break;
      case GL_CULL_FACE:
         *params = (GLfloat) ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = ENUM_TO_FLOAT(ctx->Polygon.CullFaceMode);
         break;
      case GL_CURRENT_COLOR:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0];
         params[1] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1];
         params[2] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2];
         params[3] = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3];
         break;
      case GL_CURRENT_INDEX:
	 FLUSH_CURRENT(ctx, 0);
         *params = (GLfloat) ctx->Current.Index;
         break;
      case GL_CURRENT_NORMAL:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0];
         params[1] = ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1];
         params[2] = ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2];
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = ctx->Current.RasterColor[0];
	 params[1] = ctx->Current.RasterColor[1];
	 params[2] = ctx->Current.RasterColor[2];
	 params[3] = ctx->Current.RasterColor[3];
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 params[0] = ctx->Current.RasterDistance;
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = ctx->Current.RasterIndex;
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = ctx->Current.RasterPos[0];
	 params[1] = ctx->Current.RasterPos[1];
	 params[2] = ctx->Current.RasterPos[2];
	 params[3] = ctx->Current.RasterPos[3];
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	 params[0] = ctx->Current.RasterTexCoords[texUnit][0];
	 params[1] = ctx->Current.RasterTexCoords[texUnit][1];
	 params[2] = ctx->Current.RasterTexCoords[texUnit][2];
	 params[3] = ctx->Current.RasterTexCoords[texUnit][3];
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	 *params = (GLfloat) ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
	 FLUSH_CURRENT(ctx, 0);
	 params[0] = (GLfloat) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][0];
	 params[1] = (GLfloat) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][1];
	 params[2] = (GLfloat) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][2];
	 params[3] = (GLfloat) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][3];
	 break;
      case GL_DEPTH_BIAS:
	 *params = (GLfloat) ctx->Pixel.DepthBias;
	 break;
      case GL_DEPTH_BITS:
	 *params = (GLfloat) ctx->Visual.depthBits;
	 break;
      case GL_DEPTH_CLEAR_VALUE:
	 *params = (GLfloat) ctx->Depth.Clear;
	 break;
      case GL_DEPTH_FUNC:
	 *params = ENUM_TO_FLOAT(ctx->Depth.Func);
	 break;
      case GL_DEPTH_RANGE:
         params[0] = (GLfloat) ctx->Viewport.Near;
         params[1] = (GLfloat) ctx->Viewport.Far;
	 break;
      case GL_DEPTH_SCALE:
	 *params = (GLfloat) ctx->Pixel.DepthScale;
	 break;
      case GL_DEPTH_TEST:
	 *params = (GLfloat) ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = (GLfloat) ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = (GLfloat) ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = (GLfloat) ctx->Visual.doubleBufferMode;
	 break;
      case GL_DRAW_BUFFER:
	 *params = ENUM_TO_FLOAT(ctx->Color.DrawBuffer);
	 break;
      case GL_EDGE_FLAG:
	 FLUSH_CURRENT(ctx, 0);
	 *params = (GLfloat) ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         *params = (GLfloat) ctx->Feedback.BufferSize;
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Feedback.Type);
         break;
      case GL_FOG:
	 *params = (GLfloat) ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
	 params[0] = ctx->Fog.Color[0];
	 params[1] = ctx->Fog.Color[1];
	 params[2] = ctx->Fog.Color[2];
	 params[3] = ctx->Fog.Color[3];
	 break;
      case GL_FOG_DENSITY:
	 *params = ctx->Fog.Density;
	 break;
      case GL_FOG_END:
	 *params = ctx->Fog.End;
	 break;
      case GL_FOG_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.Fog);
	 break;
      case GL_FOG_INDEX:
	 *params = ctx->Fog.Index;
	 break;
      case GL_FOG_MODE:
	 *params = ENUM_TO_FLOAT(ctx->Fog.Mode);
	 break;
      case GL_FOG_START:
	 *params = ctx->Fog.Start;
	 break;
      case GL_FRONT_FACE:
	 *params = ENUM_TO_FLOAT(ctx->Polygon.FrontFace);
	 break;
      case GL_GREEN_BIAS:
         *params = (GLfloat) ctx->Pixel.GreenBias;
         break;
      case GL_GREEN_BITS:
         *params = (GLfloat) ctx->Visual.greenBits;
         break;
      case GL_GREEN_SCALE:
         *params = (GLfloat) ctx->Pixel.GreenScale;
         break;
      case GL_INDEX_BITS:
         *params = (GLfloat) ctx->Visual.indexBits;
	 break;
      case GL_INDEX_CLEAR_VALUE:
         *params = (GLfloat) ctx->Color.ClearIndex;
	 break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual.rgbMode ? 0.0F : 1.0F;
	 break;
      case GL_INDEX_OFFSET:
	 *params = (GLfloat) ctx->Pixel.IndexOffset;
	 break;
      case GL_INDEX_SHIFT:
	 *params = (GLfloat) ctx->Pixel.IndexShift;
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = (GLfloat) ctx->Color.IndexMask;
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = (GLfloat) ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = (GLfloat) ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = ctx->Light.Model.Ambient[0];
	 params[1] = ctx->Light.Model.Ambient[1];
	 params[2] = ctx->Light.Model.Ambient[2];
	 params[3] = ctx->Light.Model.Ambient[3];
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = ENUM_TO_FLOAT(ctx->Light.Model.ColorControl);
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = (GLfloat) ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = (GLfloat) ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = (GLfloat) ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.LineSmooth);
	 break;
      case GL_LINE_STIPPLE:
	 *params = (GLfloat) ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
         *params = (GLfloat) ctx->Line.StipplePattern;
         break;
      case GL_LINE_STIPPLE_REPEAT:
         *params = (GLfloat) ctx->Line.StippleFactor;
         break;
      case GL_LINE_WIDTH:
	 *params = (GLfloat) ctx->Line.Width;
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = (GLfloat) ctx->Const.LineWidthGranularity;
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = (GLfloat) ctx->Const.MinLineWidthAA;
	 params[1] = (GLfloat) ctx->Const.MaxLineWidthAA;
	 break;
      case GL_ALIASED_LINE_WIDTH_RANGE:
	 params[0] = (GLfloat) ctx->Const.MinLineWidth;
	 params[1] = (GLfloat) ctx->Const.MaxLineWidth;
	 break;
      case GL_LIST_BASE:
	 *params = (GLfloat) ctx->List.ListBase;
	 break;
      case GL_LIST_INDEX:
	 *params = (GLfloat) ctx->ListState.CurrentListNum;
	 break;
      case GL_LIST_MODE:
         if (!ctx->CompileFlag)
            *params = 0.0F;
         else if (ctx->ExecuteFlag)
            *params = ENUM_TO_FLOAT(GL_COMPILE_AND_EXECUTE);
         else
            *params = ENUM_TO_FLOAT(GL_COMPILE);
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = (GLfloat) ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = (GLfloat) ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
         *params = ENUM_TO_FLOAT(ctx->Color.LogicOp);
	 break;
      case GL_MAP1_COLOR_4:
	 *params = (GLfloat) ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = ctx->Eval.MapGrid1u1;
	 params[1] = ctx->Eval.MapGrid1u2;
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = (GLfloat) ctx->Eval.MapGrid1un;
	 break;
      case GL_MAP1_INDEX:
	 *params = (GLfloat) ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = (GLfloat) ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = (GLfloat) ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = (GLfloat) ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = (GLfloat) ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = (GLfloat) ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = ctx->Eval.MapGrid2u1;
	 params[1] = ctx->Eval.MapGrid2u2;
	 params[2] = ctx->Eval.MapGrid2v1;
	 params[3] = ctx->Eval.MapGrid2v2;
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = (GLfloat) ctx->Eval.MapGrid2un;
	 params[1] = (GLfloat) ctx->Eval.MapGrid2vn;
	 break;
      case GL_MAP2_INDEX:
	 *params = (GLfloat) ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = (GLfloat) ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = (GLfloat) ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = (GLfloat) ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = (GLfloat) ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = (GLfloat) ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = ENUM_TO_FLOAT(ctx->Transform.MatrixMode);
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	 *params = (GLfloat) MAX_ATTRIB_STACK_DEPTH;
	 break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLfloat) MAX_CLIENT_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIP_PLANES:
	 *params = (GLfloat) ctx->Const.MaxClipPlanes;
	 break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = (GLfloat) ctx->Const.MaxArrayLockSize;
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = (GLfloat) ctx->Const.MaxArrayLockSize;
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = (GLfloat) MAX_EVAL_ORDER;
	 break;
      case GL_MAX_LIGHTS:
	 *params = (GLfloat) ctx->Const.MaxLights;
	 break;
      case GL_MAX_LIST_NESTING:
	 *params = (GLfloat) MAX_LIST_NESTING;
	 break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	 *params = (GLfloat) MAX_MODELVIEW_STACK_DEPTH;
	 break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = (GLfloat) MAX_NAME_STACK_DEPTH;
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = (GLfloat) MAX_PIXEL_MAP_TABLE;
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	 *params = (GLfloat) MAX_PROJECTION_STACK_DEPTH;
	 break;
      case GL_MAX_TEXTURE_SIZE:
         *params = (GLfloat) (1 << (ctx->Const.MaxTextureLevels - 1));
	 break;
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = (GLfloat) (1 << (ctx->Const.Max3DTextureLevels - 1));
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = (GLfloat) MAX_TEXTURE_STACK_DEPTH;
	 break;
      case GL_MAX_VIEWPORT_DIMS:
         params[0] = (GLfloat) MAX_WIDTH;
         params[1] = (GLfloat) MAX_HEIGHT;
         break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = ctx->ModelviewMatrixStack.Top->m[i];
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = (GLfloat) (ctx->ModelviewMatrixStack.Depth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = (GLfloat) ctx->Select.NameStackDepth;
	 break;
      case GL_NORMALIZE:
	 *params = (GLfloat) ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = (GLfloat) ctx->Pack.Alignment;
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = (GLfloat) ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = (GLfloat) ctx->Pack.RowLength;
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = (GLfloat) ctx->Pack.SkipPixels;
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = (GLfloat) ctx->Pack.SkipRows;
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = (GLfloat) ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = (GLfloat) ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = (GLfloat) ctx->Pack.ImageHeight;
         break;
      case GL_PACK_INVERT_MESA:
         *params = (GLfloat) ctx->Pack.Invert;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.PerspectiveCorrection);
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapAtoAsize;
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapBtoBsize;
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapGtoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoAsize;
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoBsize;
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoIsize;
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapItoRsize;
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapRtoRsize;
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = (GLfloat) ctx->Pixel.MapStoSsize;
	 break;
      case GL_POINT_SIZE:
         *params = (GLfloat) ctx->Point.Size;
         break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = (GLfloat) ctx->Const.PointSizeGranularity;
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = (GLfloat) ctx->Const.MinPointSizeAA;
	 params[1] = (GLfloat) ctx->Const.MaxPointSizeAA;
	 break;
      case GL_ALIASED_POINT_SIZE_RANGE:
	 params[0] = (GLfloat) ctx->Const.MinPointSize;
	 params[1] = (GLfloat) ctx->Const.MaxPointSize;
	 break;
      case GL_POINT_SMOOTH:
	 *params = (GLfloat) ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.PointSmooth);
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = (GLfloat) (ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = (GLfloat) (ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = (GLfloat) (ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = (GLfloat) (ctx->Point.Params[0]);
	 params[1] = (GLfloat) (ctx->Point.Params[1]);
	 params[2] = (GLfloat) (ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = ENUM_TO_FLOAT(ctx->Polygon.FrontMode);
	 params[1] = ENUM_TO_FLOAT(ctx->Polygon.BackMode);
	 break;
#ifdef GL_EXT_polygon_offset
      case GL_POLYGON_OFFSET_BIAS_EXT:
         *params = ctx->Polygon.OffsetUnits;
         break;
#endif
      case GL_POLYGON_OFFSET_FACTOR:
         *params = ctx->Polygon.OffsetFactor;
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_SMOOTH:
	 *params = (GLfloat) ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = ENUM_TO_FLOAT(ctx->Hint.PolygonSmooth);
	 break;
      case GL_POLYGON_STIPPLE:
         *params = (GLfloat) ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = ctx->ProjectionMatrixStack.Top->m[i];
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = (GLfloat) (ctx->ProjectionMatrixStack.Depth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = ENUM_TO_FLOAT(ctx->Pixel.ReadBuffer);
	 break;
      case GL_RED_BIAS:
         *params = ctx->Pixel.RedBias;
         break;
      case GL_RED_BITS:
         *params = (GLfloat) ctx->Visual.redBits;
         break;
      case GL_RED_SCALE:
         *params = ctx->Pixel.RedScale;
         break;
      case GL_RENDER_MODE:
	 *params = ENUM_TO_FLOAT(ctx->RenderMode);
	 break;
      case GL_RESCALE_NORMAL:
         *params = (GLfloat) ctx->Transform.RescaleNormals;
         break;
      case GL_RGBA_MODE:
	 *params = (GLfloat) ctx->Visual.rgbMode;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = (GLfloat) ctx->Scissor.X;
	 params[1] = (GLfloat) ctx->Scissor.Y;
	 params[2] = (GLfloat) ctx->Scissor.Width;
	 params[3] = (GLfloat) ctx->Scissor.Height;
	 break;
      case GL_SCISSOR_TEST:
	 *params = (GLfloat) ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = (GLfloat) ctx->Select.BufferSize;
         break;
      case GL_SHADE_MODEL:
	 *params = ENUM_TO_FLOAT(ctx->Light.ShadeModel);
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = (GLfloat) ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
         *params = (GLfloat) ctx->Visual.stencilBits;
         break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = (GLfloat) ctx->Stencil.Clear;
	 break;
      case GL_STENCIL_FAIL:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_FUNC:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.Function[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = ENUM_TO_FLOAT(ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace]);
	 break;
      case GL_STENCIL_REF:
	 *params = (GLfloat) ctx->Stencil.Ref[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_TEST:
	 *params = (GLfloat) ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = (GLfloat) ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = (GLfloat) ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace];
	 break;
      case GL_STEREO:
	 *params = (GLfloat) ctx->Visual.stereoMode;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = (GLfloat) ctx->Const.SubPixelBits;
	 break;
      case GL_TEXTURE_1D:
         *params = _mesa_IsEnabled(GL_TEXTURE_1D) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_2D:
         *params = _mesa_IsEnabled(GL_TEXTURE_2D) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_3D:
         *params = _mesa_IsEnabled(GL_TEXTURE_3D) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = (GLfloat) textureUnit->Current1D->Name;
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = (GLfloat) textureUnit->Current2D->Name;
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = (GLfloat) textureUnit->Current2D->Name;
          break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = textureUnit->EnvColor[0];
	 params[1] = textureUnit->EnvColor[1];
	 params[2] = textureUnit->EnvColor[2];
	 params[3] = textureUnit->EnvColor[3];
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = ENUM_TO_FLOAT(textureUnit->EnvMode);
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? 1.0F : 0.0F;
	 break;
      case GL_TEXTURE_MATRIX:
         for (i=0;i<16;i++) {
	    params[i] = ctx->TextureMatrixStack[texUnit].Top->m[i];
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = (GLfloat) (ctx->TextureMatrixStack[texUnit].Depth + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = (GLfloat) ctx->Unpack.Alignment;
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = (GLfloat) ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = (GLfloat) ctx->Unpack.RowLength;
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = (GLfloat) ctx->Unpack.SkipPixels;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = (GLfloat) ctx->Unpack.SkipRows;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = (GLfloat) ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = (GLfloat) ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = (GLfloat) ctx->Unpack.ImageHeight;
         break;
      case GL_UNPACK_CLIENT_STORAGE_APPLE:
         *params = (GLfloat) ctx->Unpack.ClientStorage;
         break;
      case GL_VIEWPORT:
	 params[0] = (GLfloat) ctx->Viewport.X;
	 params[1] = (GLfloat) ctx->Viewport.Y;
	 params[2] = (GLfloat) ctx->Viewport.Width;
	 params[3] = (GLfloat) ctx->Viewport.Height;
	 break;
      case GL_ZOOM_X:
	 *params = (GLfloat) ctx->Pixel.ZoomX;
	 break;
      case GL_ZOOM_Y:
	 *params = (GLfloat) ctx->Pixel.ZoomY;
	 break;
      case GL_VERTEX_ARRAY:
         *params = (GLfloat) ctx->Array.Vertex.Enabled;
         break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = (GLfloat) ctx->Array.Vertex.Size;
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Vertex.Type);
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Vertex.Stride;
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_NORMAL_ARRAY:
         *params = (GLfloat) ctx->Array.Normal.Enabled;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Normal.Type);
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Normal.Stride;
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_COLOR_ARRAY:
         *params = (GLfloat) ctx->Array.Color.Enabled;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = (GLfloat) ctx->Array.Color.Size;
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Color.Type);
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Color.Stride;
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_INDEX_ARRAY:
         *params = (GLfloat) ctx->Array.Index.Enabled;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.Index.Type);
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.Index.Stride;
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         *params = (GLfloat) ctx->Array.TexCoord[clientUnit].Enabled;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = (GLfloat) ctx->Array.TexCoord[clientUnit].Size;
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ENUM_TO_FLOAT(ctx->Array.TexCoord[clientUnit].Type);
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.TexCoord[clientUnit].Stride;
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;
      case GL_EDGE_FLAG_ARRAY:
         *params = (GLfloat) ctx->Array.EdgeFlag.Enabled;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = (GLfloat) ctx->Array.EdgeFlag.Stride;
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = 0.0;
         break;

      /* GL_ARB_multitexture */
      case GL_MAX_TEXTURE_UNITS_ARB:
         CHECK_EXTENSION_F(ARB_multitexture, pname);
         *params = (GLfloat) MIN2(ctx->Const.MaxTextureImageUnits,
                                  ctx->Const.MaxTextureCoordUnits);
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_F(ARB_multitexture, pname);
         *params = (GLfloat) (GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit);
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_F(ARB_multitexture, pname);
         *params = (GLfloat) (GL_TEXTURE0_ARB + clientUnit);
         break;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION_F(ARB_texture_cube_map, pname);
         *params = (GLfloat) _mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB);
         break;
      case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
         CHECK_EXTENSION_F(ARB_texture_cube_map, pname);
         *params = (GLfloat) textureUnit->CurrentCubeMap->Name;
         break;
      case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
         CHECK_EXTENSION_F(ARB_texture_cube_map, pname);
         *params = (GLfloat) (1 << (ctx->Const.MaxCubeTextureLevels - 1));
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSION_HINT_ARB:
         CHECK_EXTENSION_F(ARB_texture_compression, pname);
         *params = (GLfloat) ctx->Hint.TextureCompression;
         break;
      case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_F(ARB_texture_compression, pname);
         *params = (GLfloat) _mesa_get_compressed_formats(ctx, NULL);
         break;
      case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_F(ARB_texture_compression, pname);
         {
            GLint formats[100];
            GLuint i, n;
            n = _mesa_get_compressed_formats(ctx, formats);
            for (i = 0; i < n; i++)
               params[i] = (GLfloat) formats[i];
         }
         break;

      /* GL_EXT_compiled_vertex_array */
      case GL_ARRAY_ELEMENT_LOCK_FIRST_EXT:
         CHECK_EXTENSION_F(EXT_compiled_vertex_array, pname);
	 *params = (GLfloat) ctx->Array.LockFirst;
	 break;
      case GL_ARRAY_ELEMENT_LOCK_COUNT_EXT:
         CHECK_EXTENSION_F(EXT_compiled_vertex_array, pname);
	 *params = (GLfloat) ctx->Array.LockCount;
	 break;

      /* GL_ARB_transpose_matrix */
      case GL_TRANSPOSE_COLOR_MATRIX_ARB:
         _math_transposef(params, ctx->ColorMatrixStack.Top->m);
         break;
      case GL_TRANSPOSE_MODELVIEW_MATRIX_ARB:
         _math_transposef(params, ctx->ModelviewMatrixStack.Top->m);
         break;
      case GL_TRANSPOSE_PROJECTION_MATRIX_ARB:
         _math_transposef(params, ctx->ProjectionMatrixStack.Top->m);
         break;
      case GL_TRANSPOSE_TEXTURE_MATRIX_ARB:
         _math_transposef(params, ctx->TextureMatrixStack[texUnit].Top->m);
         break;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         CHECK_EXTENSION_F(HP_occlusion_test, pname);
         *params = (GLfloat) ctx->Depth.OcclusionTest;
         break;
      case GL_OCCLUSION_TEST_RESULT_HP:
         CHECK_EXTENSION_F(HP_occlusion_test, pname);
         if (ctx->Depth.OcclusionTest)
            *params = (GLfloat) ctx->OcclusionResult;
         else
            *params = (GLfloat) ctx->OcclusionResultSaved;
         /* reset flag now */
         ctx->OcclusionResult = GL_FALSE;
         ctx->OcclusionResultSaved = GL_FALSE;
         break;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         *params = (GLfloat) ctx->Pixel.PixelTextureEnabled;
         break;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         *params = (GLfloat) ctx->Pixel.PixelTextureEnabled;
         break;
      case GL_PIXEL_TEX_GEN_MODE_SGIX:
         *params = (GLfloat) pixel_texgen_mode(ctx);
         break;

      /* GL_SGI_color_matrix (also in 1.2 imaging) */
      case GL_COLOR_MATRIX_SGI:
         for (i=0;i<16;i++) {
	    params[i] = ctx->ColorMatrixStack.Top->m[i];
	 }
	 break;
      case GL_COLOR_MATRIX_STACK_DEPTH_SGI:
         *params = (GLfloat) (ctx->ColorMatrixStack.Depth + 1);
         break;
      case GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI:
         *params = (GLfloat) MAX_COLOR_STACK_DEPTH;
         break;
      case GL_POST_COLOR_MATRIX_RED_SCALE_SGI:
         *params = ctx->Pixel.PostColorMatrixScale[0];
         break;
      case GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI:
         *params = ctx->Pixel.PostColorMatrixScale[1];
         break;
      case GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI:
         *params = ctx->Pixel.PostColorMatrixScale[2];
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI:
         *params = ctx->Pixel.PostColorMatrixScale[3];
         break;
      case GL_POST_COLOR_MATRIX_RED_BIAS_SGI:
         *params = ctx->Pixel.PostColorMatrixBias[0];
         break;
      case GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI:
         *params = ctx->Pixel.PostColorMatrixBias[1];
         break;
      case GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI:
         *params = ctx->Pixel.PostColorMatrixBias[2];
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI:
         *params = ctx->Pixel.PostColorMatrixBias[3];
         break;

      /* GL_EXT_convolution (also in 1.2 imaging) */
      case GL_CONVOLUTION_1D_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = (GLfloat) ctx->Pixel.Convolution1DEnabled;
         break;
      case GL_CONVOLUTION_2D:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = (GLfloat) ctx->Pixel.Convolution2DEnabled;
         break;
      case GL_SEPARABLE_2D:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = (GLfloat) ctx->Pixel.Separable2DEnabled;
         break;
      case GL_POST_CONVOLUTION_RED_SCALE_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionScale[0];
         break;
      case GL_POST_CONVOLUTION_GREEN_SCALE_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionScale[1];
         break;
      case GL_POST_CONVOLUTION_BLUE_SCALE_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionScale[2];
         break;
      case GL_POST_CONVOLUTION_ALPHA_SCALE_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionScale[3];
         break;
      case GL_POST_CONVOLUTION_RED_BIAS_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionBias[0];
         break;
      case GL_POST_CONVOLUTION_GREEN_BIAS_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionBias[1];
         break;
      case GL_POST_CONVOLUTION_BLUE_BIAS_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionBias[2];
         break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS_EXT:
         CHECK_EXTENSION_F(EXT_convolution, pname);
         *params = ctx->Pixel.PostConvolutionBias[2];
         break;

      /* GL_EXT_histogram (also in 1.2 imaging) */
      case GL_HISTOGRAM:
         CHECK_EXTENSION_F(EXT_histogram, pname);
         *params = (GLfloat) ctx->Pixel.HistogramEnabled;
	 break;
      case GL_MINMAX:
         CHECK_EXTENSION_F(EXT_histogram, pname);
         *params = (GLfloat) ctx->Pixel.MinMaxEnabled;
         break;

      /* GL_SGI_color_table (also in 1.2 imaging */
      case GL_COLOR_TABLE_SGI:
         *params = (GLfloat) ctx->Pixel.ColorTableEnabled;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         *params = (GLfloat) ctx->Pixel.PostConvolutionColorTableEnabled;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         *params = (GLfloat) ctx->Pixel.PostColorMatrixColorTableEnabled;
         break;

      /* GL_SGI_texture_color_table */
      case GL_TEXTURE_COLOR_TABLE_SGI:
         CHECK_EXTENSION_F(SGI_texture_color_table, pname);
         *params = (GLfloat) textureUnit->ColorTableEnabled;
         break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION_F(EXT_secondary_color, pname);
	 *params = (GLfloat) ctx->Fog.ColorSumEnabled;
	 break;
      case GL_CURRENT_SECONDARY_COLOR_EXT:
         CHECK_EXTENSION_F(EXT_secondary_color, pname);
	 FLUSH_CURRENT(ctx, 0);
         params[0] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0];
         params[1] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1];
         params[2] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2];
         params[3] = ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3];
	 break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         CHECK_EXTENSION_F(EXT_secondary_color, pname);
         *params = (GLfloat) ctx->Array.SecondaryColor.Enabled;
         break;
      case GL_SECONDARY_COLOR_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_F(EXT_secondary_color, pname);
	 *params = (GLfloat) ctx->Array.SecondaryColor.Type;
	 break;
      case GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_F(EXT_secondary_color, pname);
	 *params = (GLfloat) ctx->Array.SecondaryColor.Stride;
	 break;
      case GL_SECONDARY_COLOR_ARRAY_SIZE_EXT:
         CHECK_EXTENSION_F(EXT_secondary_color, pname);
	 *params = (GLfloat) ctx->Array.SecondaryColor.Size;
	 break;

      /* GL_EXT_fog_coord */
      case GL_CURRENT_FOG_COORDINATE_EXT:
         CHECK_EXTENSION_F(EXT_fog_coord, pname);
	 FLUSH_CURRENT(ctx, 0);
	 *params = (GLfloat) ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
	 break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         CHECK_EXTENSION_F(EXT_fog_coord, pname);
         *params = (GLfloat) ctx->Array.FogCoord.Enabled;
         break;
      case GL_FOG_COORDINATE_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_F(EXT_fog_coord, pname);
	 *params = (GLfloat) ctx->Array.FogCoord.Type;
	 break;
      case GL_FOG_COORDINATE_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_F(EXT_fog_coord, pname);
	 *params = (GLfloat) ctx->Array.FogCoord.Stride;
	 break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
         CHECK_EXTENSION_F(EXT_fog_coord, pname);
	 *params = (GLfloat) ctx->Fog.FogCoordinateSource;
	 break;

      /* GL_EXT_texture_lod_bias */
      case GL_MAX_TEXTURE_LOD_BIAS_EXT:
         *params = ctx->Const.MaxTextureLodBias;
         break;

      /* GL_EXT_texture_filter_anisotropic */
      case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
         CHECK_EXTENSION_F(EXT_texture_filter_anisotropic, pname);
         *params = ctx->Const.MaxTextureMaxAnisotropy;
	 break;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = (GLfloat) ctx->Multisample.Enabled;
         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = (GLfloat) ctx->Multisample.SampleAlphaToCoverage;
         break;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = (GLfloat) ctx->Multisample.SampleAlphaToOne;
         break;
      case GL_SAMPLE_COVERAGE_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = (GLfloat) ctx->Multisample.SampleCoverage;
         break;
      case GL_SAMPLE_COVERAGE_VALUE_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = ctx->Multisample.SampleCoverageValue;
         break;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = (GLfloat) ctx->Multisample.SampleCoverageInvert;
         break;
      case GL_SAMPLE_BUFFERS_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = 0.0; /* XXX fix someday */
         break;
      case GL_SAMPLES_ARB:
         CHECK_EXTENSION_F(ARB_multisample, pname);
         *params = 0.0; /* XXX fix someday */
         break;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION_F(IBM_rasterpos_clip, pname);
         *params = (GLfloat) ctx->Transform.RasterPositionUnclipped;
         break;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION2_F(NV_point_sprite, ARB_point_sprite, pname);
         *params = (GLfloat) ctx->Point.PointSprite;
         break;
      case GL_POINT_SPRITE_R_MODE_NV:
         CHECK_EXTENSION_F(NV_point_sprite, pname);
         *params = (GLfloat) ctx->Point.SpriteRMode;
         break;
      case GL_POINT_SPRITE_COORD_ORIGIN:
         CHECK_EXTENSION_F(ARB_point_sprite, pname);
         *params = (GLfloat) ctx->Point.SpriteOrigin;
         break;

      /* GL_SGIS_generate_mipmap */
      case GL_GENERATE_MIPMAP_HINT_SGIS:
         CHECK_EXTENSION_F(SGIS_generate_mipmap, pname);
         *params = (GLfloat) ctx->Hint.GenerateMipmap;
         break;

#if FEATURE_NV_vertex_program
      case GL_VERTEX_PROGRAM_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->VertexProgram.Enabled;
         break;
      case GL_VERTEX_PROGRAM_POINT_SIZE_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->VertexProgram.PointSizeEnabled;
         break;
      case GL_VERTEX_PROGRAM_TWO_SIDE_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->VertexProgram.TwoSideEnabled;
         break;
      case GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->Const.MaxProgramMatrixStackDepth;
         break;
      case GL_MAX_TRACK_MATRICES_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->Const.MaxProgramMatrices;
         break;
      case GL_CURRENT_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->CurrentStack->Depth + 1;
         break;
      case GL_CURRENT_MATRIX_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         for (i = 0; i < 16; i++)
            params[i] = ctx->CurrentStack->Top->m[i];
         break;
      case GL_VERTEX_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         if (ctx->VertexProgram.Current)
            *params = (GLfloat) ctx->VertexProgram.Current->Base.Id;
         else
            *params = 0.0;
         break;
      case GL_PROGRAM_ERROR_POSITION_NV:
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         *params = (GLfloat) ctx->Program.ErrorPos;
         break;
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
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_VERTEX_ATTRIB_ARRAY0_NV;
            *params = (GLfloat) ctx->Array.VertexAttrib[n].Enabled;
         }
         break;
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
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP1_VERTEX_ATTRIB0_4_NV;
            *params = (GLfloat) ctx->Eval.Map1Attrib[n];
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
         CHECK_EXTENSION_F(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP2_VERTEX_ATTRIB0_4_NV;
            *params = (GLfloat) ctx->Eval.Map2Attrib[n];
         }
         break;
#endif /* FEATURE_NV_vertex_program */

#if FEATURE_NV_fragment_program
      case GL_FRAGMENT_PROGRAM_NV:
         CHECK_EXTENSION_F(NV_fragment_program, pname);
         *params = (GLfloat) ctx->FragmentProgram.Enabled;
         break;
      case GL_MAX_TEXTURE_COORDS_NV:
         CHECK_EXTENSION_F(NV_fragment_program, pname);
         *params = (GLfloat) ctx->Const.MaxTextureCoordUnits;
         break;
      case GL_MAX_TEXTURE_IMAGE_UNITS_NV:
         CHECK_EXTENSION_F(NV_fragment_program, pname);
         *params = (GLfloat) ctx->Const.MaxTextureImageUnits;
         break;
      case GL_FRAGMENT_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_F(NV_fragment_program, pname);
         if (ctx->FragmentProgram.Current)
            *params = (GLfloat) ctx->FragmentProgram.Current->Base.Id;
         else
            *params = 0.0;
         break;
      case GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV:
         CHECK_EXTENSION_F(NV_fragment_program, pname);
         *params = (GLfloat) MAX_NV_FRAGMENT_PROGRAM_PARAMS;
         break;
#endif /* FEATURE_NV_fragment_program */

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION_F(NV_texture_rectangle, pname);
         *params = (GLfloat) _mesa_IsEnabled(GL_TEXTURE_RECTANGLE_NV);
         break;
      case GL_TEXTURE_BINDING_RECTANGLE_NV:
         CHECK_EXTENSION_F(NV_texture_rectangle, pname);
         *params = (GLfloat) textureUnit->CurrentRect->Name;
         break;
      case GL_MAX_RECTANGLE_TEXTURE_SIZE_NV:
         CHECK_EXTENSION_F(NV_texture_rectangle, pname);
         *params = (GLfloat) ctx->Const.MaxTextureRectSize;
         break;

      /* GL_EXT_stencil_two_side */
      case GL_STENCIL_TEST_TWO_SIDE_EXT:
         CHECK_EXTENSION_F(EXT_stencil_two_side, pname);
         *params = (GLfloat) ctx->Stencil.TestTwoSide;
         break;
      case GL_ACTIVE_STENCIL_FACE_EXT:
         CHECK_EXTENSION_F(EXT_stencil_two_side, pname);
         *params = (GLfloat) (ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT);
         break;

      /* GL_NV_light_max_exponent */
      case GL_MAX_SHININESS_NV:
         *params = ctx->Const.MaxShininess;
         break;
      case GL_MAX_SPOT_EXPONENT_NV:
         *params = ctx->Const.MaxSpotExponent;
         break;

#if FEATURE_ARB_vertex_buffer_object
      case GL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.ArrayBufferObj->Name;
         break;
      case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.Vertex.BufferObj->Name;
         break;
      case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.Normal.BufferObj->Name;
         break;
      case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.Color.BufferObj->Name;
         break;
      case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.Index.BufferObj->Name;
         break;
      case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.TexCoord[clientUnit].BufferObj->Name;
         break;
      case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.EdgeFlag.BufferObj->Name;
         break;
      case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.SecondaryColor.BufferObj->Name;
         break;
      case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.FogCoord.BufferObj->Name;
         break;
      /*case GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB: - not supported */
      case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_F(ARB_vertex_buffer_object, pname);
         *params = (GLfloat) ctx->Array.ElementArrayBufferObj->Name;
         break;
#endif
#if FEATURE_EXT_pixel_buffer_object
      case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_F(EXT_pixel_buffer_object, pname);
         *params = (GLfloat) ctx->Pack.BufferObj->Name;
         break;
      case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_F(EXT_pixel_buffer_object, pname);
         *params = (GLfloat) ctx->Unpack.BufferObj->Name;
         break;
#endif

#if FEATURE_ARB_vertex_program
      /* GL_NV_vertex_program and GL_ARB_fragment_program define others */
      case GL_MAX_VERTEX_ATTRIBS_ARB:
         CHECK_EXTENSION_F(ARB_vertex_program, pname);
         *params = (GLfloat) ctx->Const.MaxVertexProgramAttribs;
         break;
#endif

#if FEATURE_ARB_fragment_program
      case GL_FRAGMENT_PROGRAM_ARB:
         CHECK_EXTENSION_F(ARB_fragment_program, pname);
         *params = (GLfloat) ctx->FragmentProgram.Enabled;
         break;
      case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
         CHECK_EXTENSION_F(ARB_fragment_program, pname);
         params[0] = ctx->CurrentStack->Top->m[0];
         params[1] = ctx->CurrentStack->Top->m[4];
         params[2] = ctx->CurrentStack->Top->m[8];
         params[3] = ctx->CurrentStack->Top->m[12];
         params[4] = ctx->CurrentStack->Top->m[1];
         params[5] = ctx->CurrentStack->Top->m[5];
         params[6] = ctx->CurrentStack->Top->m[9];
         params[7] = ctx->CurrentStack->Top->m[13];
         params[8] = ctx->CurrentStack->Top->m[2];
         params[9] = ctx->CurrentStack->Top->m[6];
         params[10] = ctx->CurrentStack->Top->m[10];
         params[11] = ctx->CurrentStack->Top->m[14];
         params[12] = ctx->CurrentStack->Top->m[3];
         params[13] = ctx->CurrentStack->Top->m[7];
         params[14] = ctx->CurrentStack->Top->m[11];
         params[15] = ctx->CurrentStack->Top->m[15];
         break;
      /* Remaining ARB_fragment_program queries alias with
       * the GL_NV_fragment_program queries.
       */
#endif

      /* GL_EXT_depth_bounds_test */
      case GL_DEPTH_BOUNDS_TEST_EXT:
         CHECK_EXTENSION_F(EXT_depth_bounds_test, pname);
         params[0] = (GLfloat) ctx->Depth.BoundsTest;
         break;
      case GL_DEPTH_BOUNDS_EXT:
         CHECK_EXTENSION_F(EXT_depth_bounds_test, pname);
         params[0] = ctx->Depth.BoundsMin;
         params[1] = ctx->Depth.BoundsMax;
         break;

#if FEATURE_MESA_program_debug
      case GL_FRAGMENT_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_F(MESA_program_debug, pname);
         *params = (GLfloat) ctx->FragmentProgram.CallbackEnabled;
         break;
      case GL_VERTEX_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_F(MESA_program_debug, pname);
         *params = (GLfloat) ctx->VertexProgram.CallbackEnabled;
         break;
      case GL_FRAGMENT_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_F(MESA_program_debug, pname);
         *params = (GLfloat) ctx->FragmentProgram.CurrentPosition;
         break;
      case GL_VERTEX_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_F(MESA_program_debug, pname);
         *params = (GLfloat) ctx->VertexProgram.CurrentPosition;
         break;
#endif

      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetFloatv(0x%x)", pname);
   }
}


/**
 * Get the value(s) of a selected parameter.
 *
 * \param pname parameter to be returned.
 * \param params will hold the value(s) of the speficifed parameter.
 *
 * \sa glGetIntegerv().
 *
 * Tries to get the specified parameter via dd_function_table::GetIntegerv,
 * otherwise gets the specified parameter from the current context, converting
 * it value into GLinteger.
 */
void GLAPIENTRY
_mesa_GetIntegerv( GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   const GLuint clientUnit = ctx->Array.ActiveTexture;
   const GLuint texUnit = ctx->Texture.CurrentUnit;
   const struct gl_texture_unit *textureUnit = &ctx->Texture.Unit[texUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!params)
      return;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetIntegerv %s\n", _mesa_lookup_enum_by_nr(pname));

   if (!ctx->_CurrentProgram) {
      /* We need this in order to get correct results for
       * GL_OCCLUSION_TEST_RESULT_HP.  There might be other important cases.
       */
      FLUSH_VERTICES(ctx, 0);
   }

   if (ctx->Driver.GetIntegerv
       && (*ctx->Driver.GetIntegerv)(ctx, pname, params))
      return;

   switch (pname) {
      case GL_ACCUM_RED_BITS:
         *params = (GLint) ctx->Visual.accumRedBits;
         break;
      case GL_ACCUM_GREEN_BITS:
         *params = (GLint) ctx->Visual.accumGreenBits;
         break;
      case GL_ACCUM_BLUE_BITS:
         *params = (GLint) ctx->Visual.accumBlueBits;
         break;
      case GL_ACCUM_ALPHA_BITS:
         *params = (GLint) ctx->Visual.accumAlphaBits;
         break;
      case GL_ACCUM_CLEAR_VALUE:
         params[0] = FLOAT_TO_INT( ctx->Accum.ClearColor[0] );
         params[1] = FLOAT_TO_INT( ctx->Accum.ClearColor[1] );
         params[2] = FLOAT_TO_INT( ctx->Accum.ClearColor[2] );
         params[3] = FLOAT_TO_INT( ctx->Accum.ClearColor[3] );
         break;
      case GL_ALPHA_BIAS:
         *params = (GLint) ctx->Pixel.AlphaBias;
         break;
      case GL_ALPHA_BITS:
         *params = ctx->Visual.alphaBits;
         break;
      case GL_ALPHA_SCALE:
         *params = (GLint) ctx->Pixel.AlphaScale;
         break;
      case GL_ALPHA_TEST:
         *params = (GLint) ctx->Color.AlphaEnabled;
         break;
      case GL_ALPHA_TEST_REF:
         *params = FLOAT_TO_INT(ctx->Color.AlphaRef);
         break;
      case GL_ALPHA_TEST_FUNC:
         *params = (GLint) ctx->Color.AlphaFunc;
         break;
      case GL_ATTRIB_STACK_DEPTH:
         *params = (GLint) (ctx->AttribStackDepth);
         break;
      case GL_AUTO_NORMAL:
         *params = (GLint) ctx->Eval.AutoNormal;
         break;
      case GL_AUX_BUFFERS:
         *params = (GLint) ctx->Visual.numAuxBuffers;
         break;
      case GL_BLEND:
         *params = (GLint) ctx->Color.BlendEnabled;
         break;
      case GL_BLEND_DST:
         *params = (GLint) ctx->Color.BlendDstRGB;
         break;
      case GL_BLEND_SRC:
         *params = (GLint) ctx->Color.BlendSrcRGB;
         break;
      case GL_BLEND_SRC_RGB_EXT:
         *params = (GLint) ctx->Color.BlendSrcRGB;
         break;
      case GL_BLEND_DST_RGB_EXT:
         *params = (GLint) ctx->Color.BlendDstRGB;
         break;
      case GL_BLEND_SRC_ALPHA_EXT:
         *params = (GLint) ctx->Color.BlendSrcA;
         break;
      case GL_BLEND_DST_ALPHA_EXT:
         *params = (GLint) ctx->Color.BlendDstA;
         break;
      case GL_BLEND_EQUATION:
	 *params = (GLint) ctx->Color.BlendEquationRGB;
	 break;
      case GL_BLEND_EQUATION_ALPHA_EXT:
	 *params = (GLint) ctx->Color.BlendEquationA;
	 break;
      case GL_BLEND_COLOR_EXT:
	 params[0] = FLOAT_TO_INT( ctx->Color.BlendColor[0] );
	 params[1] = FLOAT_TO_INT( ctx->Color.BlendColor[1] );
	 params[2] = FLOAT_TO_INT( ctx->Color.BlendColor[2] );
	 params[3] = FLOAT_TO_INT( ctx->Color.BlendColor[3] );
	 break;
      case GL_BLUE_BIAS:
         *params = (GLint) ctx->Pixel.BlueBias;
         break;
      case GL_BLUE_BITS:
         *params = (GLint) ctx->Visual.blueBits;
         break;
      case GL_BLUE_SCALE:
         *params = (GLint) ctx->Pixel.BlueScale;
         break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLint) (ctx->ClientAttribStackDepth);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         if (ctx->Transform.ClipPlanesEnabled & (1 << (pname - GL_CLIP_PLANE0)))
            *params = 1;
         else
            *params = 0;
         break;
      case GL_COLOR_CLEAR_VALUE:
         params[0] = FLOAT_TO_INT( (ctx->Color.ClearColor[0]) );
         params[1] = FLOAT_TO_INT( (ctx->Color.ClearColor[1]) );
         params[2] = FLOAT_TO_INT( (ctx->Color.ClearColor[2]) );
         params[3] = FLOAT_TO_INT( (ctx->Color.ClearColor[3]) );
         break;
      case GL_COLOR_MATERIAL:
         *params = (GLint) ctx->Light.ColorMaterialEnabled;
         break;
      case GL_COLOR_MATERIAL_FACE:
         *params = (GLint) ctx->Light.ColorMaterialFace;
         break;
      case GL_COLOR_MATERIAL_PARAMETER:
         *params = (GLint) ctx->Light.ColorMaterialMode;
         break;
      case GL_COLOR_WRITEMASK:
         params[0] = ctx->Color.ColorMask[RCOMP] ? 1 : 0;
         params[1] = ctx->Color.ColorMask[GCOMP] ? 1 : 0;
         params[2] = ctx->Color.ColorMask[BCOMP] ? 1 : 0;
         params[3] = ctx->Color.ColorMask[ACOMP] ? 1 : 0;
         break;
      case GL_CULL_FACE:
         *params = (GLint) ctx->Polygon.CullFlag;
         break;
      case GL_CULL_FACE_MODE:
         *params = (GLint) ctx->Polygon.CullFaceMode;
         break;
      case GL_CURRENT_COLOR:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0]);
         params[1] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1]);
         params[2] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2]);
         params[3] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3]);
         break;
      case GL_CURRENT_INDEX:
	 FLUSH_CURRENT(ctx, 0);
         *params = (GLint) ctx->Current.Index;
         break;
      case GL_CURRENT_NORMAL:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_NORMAL][0]);
         params[1] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_NORMAL][1]);
         params[2] = FLOAT_TO_INT(ctx->Current.Attrib[VERT_ATTRIB_NORMAL][2]);
         break;
      case GL_CURRENT_RASTER_COLOR:
	 params[0] = FLOAT_TO_INT( ctx->Current.RasterColor[0] );
	 params[1] = FLOAT_TO_INT( ctx->Current.RasterColor[1] );
	 params[2] = FLOAT_TO_INT( ctx->Current.RasterColor[2] );
	 params[3] = FLOAT_TO_INT( ctx->Current.RasterColor[3] );
	 break;
      case GL_CURRENT_RASTER_DISTANCE:
	 params[0] = (GLint) ctx->Current.RasterDistance;
	 break;
      case GL_CURRENT_RASTER_INDEX:
	 *params = (GLint) ctx->Current.RasterIndex;
	 break;
      case GL_CURRENT_RASTER_POSITION:
	 params[0] = (GLint) ctx->Current.RasterPos[0];
	 params[1] = (GLint) ctx->Current.RasterPos[1];
	 params[2] = (GLint) ctx->Current.RasterPos[2];
	 params[3] = (GLint) ctx->Current.RasterPos[3];
	 break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	 params[0] = (GLint) ctx->Current.RasterTexCoords[texUnit][0];
	 params[1] = (GLint) ctx->Current.RasterTexCoords[texUnit][1];
	 params[2] = (GLint) ctx->Current.RasterTexCoords[texUnit][2];
	 params[3] = (GLint) ctx->Current.RasterTexCoords[texUnit][3];
	 break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	 *params = (GLint) ctx->Current.RasterPosValid;
	 break;
      case GL_CURRENT_TEXTURE_COORDS:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = (GLint) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][0];
         params[1] = (GLint) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][1];
         params[2] = (GLint) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][2];
         params[3] = (GLint) ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texUnit][3];
	 break;
      case GL_DEPTH_BIAS:
         *params = (GLint) ctx->Pixel.DepthBias;
	 break;
      case GL_DEPTH_BITS:
	 *params = ctx->Visual.depthBits;
	 break;
      case GL_DEPTH_CLEAR_VALUE:
         *params = (GLint) ctx->Depth.Clear;
	 break;
      case GL_DEPTH_FUNC:
         *params = (GLint) ctx->Depth.Func;
	 break;
      case GL_DEPTH_RANGE:
         params[0] = (GLint) ctx->Viewport.Near;
         params[1] = (GLint) ctx->Viewport.Far;
	 break;
      case GL_DEPTH_SCALE:
         *params = (GLint) ctx->Pixel.DepthScale;
	 break;
      case GL_DEPTH_TEST:
         *params = (GLint) ctx->Depth.Test;
	 break;
      case GL_DEPTH_WRITEMASK:
	 *params = (GLint) ctx->Depth.Mask;
	 break;
      case GL_DITHER:
	 *params = (GLint) ctx->Color.DitherFlag;
	 break;
      case GL_DOUBLEBUFFER:
	 *params = (GLint) ctx->Visual.doubleBufferMode;
	 break;
      case GL_DRAW_BUFFER:
	 *params = (GLint) ctx->Color.DrawBuffer;
	 break;
      case GL_EDGE_FLAG:
	 FLUSH_CURRENT(ctx, 0);
	 *params = (GLint) ctx->Current.EdgeFlag;
	 break;
      case GL_FEEDBACK_BUFFER_SIZE:
         *params = ctx->Feedback.BufferSize;
         break;
      case GL_FEEDBACK_BUFFER_TYPE:
         *params = ctx->Feedback.Type;
         break;
      case GL_FOG:
	 *params = (GLint) ctx->Fog.Enabled;
	 break;
      case GL_FOG_COLOR:
	 params[0] = FLOAT_TO_INT( ctx->Fog.Color[0] );
	 params[1] = FLOAT_TO_INT( ctx->Fog.Color[1] );
	 params[2] = FLOAT_TO_INT( ctx->Fog.Color[2] );
	 params[3] = FLOAT_TO_INT( ctx->Fog.Color[3] );
	 break;
      case GL_FOG_DENSITY:
	 *params = (GLint) ctx->Fog.Density;
	 break;
      case GL_FOG_END:
	 *params = (GLint) ctx->Fog.End;
	 break;
      case GL_FOG_HINT:
	 *params = (GLint) ctx->Hint.Fog;
	 break;
      case GL_FOG_INDEX:
	 *params = (GLint) ctx->Fog.Index;
	 break;
      case GL_FOG_MODE:
	 *params = (GLint) ctx->Fog.Mode;
	 break;
      case GL_FOG_START:
	 *params = (GLint) ctx->Fog.Start;
	 break;
      case GL_FRONT_FACE:
	 *params = (GLint) ctx->Polygon.FrontFace;
	 break;
      case GL_GREEN_BIAS:
         *params = (GLint) ctx->Pixel.GreenBias;
         break;
      case GL_GREEN_BITS:
         *params = (GLint) ctx->Visual.greenBits;
         break;
      case GL_GREEN_SCALE:
         *params = (GLint) ctx->Pixel.GreenScale;
         break;
      case GL_INDEX_BITS:
         *params = (GLint) ctx->Visual.indexBits;
         break;
      case GL_INDEX_CLEAR_VALUE:
         *params = (GLint) ctx->Color.ClearIndex;
         break;
      case GL_INDEX_MODE:
	 *params = ctx->Visual.rgbMode ? 0 : 1;
	 break;
      case GL_INDEX_OFFSET:
	 *params = ctx->Pixel.IndexOffset;
	 break;
      case GL_INDEX_SHIFT:
	 *params = ctx->Pixel.IndexShift;
	 break;
      case GL_INDEX_WRITEMASK:
	 *params = (GLint) ctx->Color.IndexMask;
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 *params = (GLint) ctx->Light.Light[pname-GL_LIGHT0].Enabled;
	 break;
      case GL_LIGHTING:
	 *params = (GLint) ctx->Light.Enabled;
	 break;
      case GL_LIGHT_MODEL_AMBIENT:
	 params[0] = FLOAT_TO_INT( ctx->Light.Model.Ambient[0] );
	 params[1] = FLOAT_TO_INT( ctx->Light.Model.Ambient[1] );
	 params[2] = FLOAT_TO_INT( ctx->Light.Model.Ambient[2] );
	 params[3] = FLOAT_TO_INT( ctx->Light.Model.Ambient[3] );
	 break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         params[0] = (GLint) ctx->Light.Model.ColorControl;
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 *params = (GLint) ctx->Light.Model.LocalViewer;
	 break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	 *params = (GLint) ctx->Light.Model.TwoSide;
	 break;
      case GL_LINE_SMOOTH:
	 *params = (GLint) ctx->Line.SmoothFlag;
	 break;
      case GL_LINE_SMOOTH_HINT:
	 *params = (GLint) ctx->Hint.LineSmooth;
	 break;
      case GL_LINE_STIPPLE:
	 *params = (GLint) ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STIPPLE_PATTERN:
         *params = (GLint) ctx->Line.StipplePattern;
         break;
      case GL_LINE_STIPPLE_REPEAT:
         *params = (GLint) ctx->Line.StippleFactor;
         break;
      case GL_LINE_WIDTH:
	 *params = (GLint) ctx->Line.Width;
	 break;
      case GL_LINE_WIDTH_GRANULARITY:
	 *params = (GLint) ctx->Const.LineWidthGranularity;
	 break;
      case GL_LINE_WIDTH_RANGE:
	 params[0] = (GLint) ctx->Const.MinLineWidthAA;
	 params[1] = (GLint) ctx->Const.MaxLineWidthAA;
	 break;
      case GL_ALIASED_LINE_WIDTH_RANGE:
	 params[0] = (GLint) ctx->Const.MinLineWidth;
	 params[1] = (GLint) ctx->Const.MaxLineWidth;
	 break;
      case GL_LIST_BASE:
	 *params = (GLint) ctx->List.ListBase;
	 break;
      case GL_LIST_INDEX:
	 *params = (GLint) ctx->ListState.CurrentListNum;
	 break;
      case GL_LIST_MODE:
         if (!ctx->CompileFlag)
            *params = 0;
         else if (ctx->ExecuteFlag)
            *params = (GLint) GL_COMPILE_AND_EXECUTE;
         else
            *params = (GLint) GL_COMPILE;
	 break;
      case GL_INDEX_LOGIC_OP:
	 *params = (GLint) ctx->Color.IndexLogicOpEnabled;
	 break;
      case GL_COLOR_LOGIC_OP:
	 *params = (GLint) ctx->Color.ColorLogicOpEnabled;
	 break;
      case GL_LOGIC_OP_MODE:
         *params = (GLint) ctx->Color.LogicOp;
         break;
      case GL_MAP1_COLOR_4:
	 *params = (GLint) ctx->Eval.Map1Color4;
	 break;
      case GL_MAP1_GRID_DOMAIN:
	 params[0] = (GLint) ctx->Eval.MapGrid1u1;
	 params[1] = (GLint) ctx->Eval.MapGrid1u2;
	 break;
      case GL_MAP1_GRID_SEGMENTS:
	 *params = (GLint) ctx->Eval.MapGrid1un;
	 break;
      case GL_MAP1_INDEX:
	 *params = (GLint) ctx->Eval.Map1Index;
	 break;
      case GL_MAP1_NORMAL:
	 *params = (GLint) ctx->Eval.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 *params = (GLint) ctx->Eval.Map1TextureCoord1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 *params = (GLint) ctx->Eval.Map1TextureCoord2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 *params = (GLint) ctx->Eval.Map1TextureCoord3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 *params = (GLint) ctx->Eval.Map1TextureCoord4;
	 break;
      case GL_MAP1_VERTEX_3:
	 *params = (GLint) ctx->Eval.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
	 *params = (GLint) ctx->Eval.Map1Vertex4;
	 break;
      case GL_MAP2_COLOR_4:
	 *params = (GLint) ctx->Eval.Map2Color4;
	 break;
      case GL_MAP2_GRID_DOMAIN:
	 params[0] = (GLint) ctx->Eval.MapGrid2u1;
	 params[1] = (GLint) ctx->Eval.MapGrid2u2;
	 params[2] = (GLint) ctx->Eval.MapGrid2v1;
	 params[3] = (GLint) ctx->Eval.MapGrid2v2;
	 break;
      case GL_MAP2_GRID_SEGMENTS:
	 params[0] = (GLint) ctx->Eval.MapGrid2un;
	 params[1] = (GLint) ctx->Eval.MapGrid2vn;
	 break;
      case GL_MAP2_INDEX:
	 *params = (GLint) ctx->Eval.Map2Index;
	 break;
      case GL_MAP2_NORMAL:
	 *params = (GLint) ctx->Eval.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
	 *params = (GLint) ctx->Eval.Map2TextureCoord1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 *params = (GLint) ctx->Eval.Map2TextureCoord2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 *params = (GLint) ctx->Eval.Map2TextureCoord3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 *params = (GLint) ctx->Eval.Map2TextureCoord4;
	 break;
      case GL_MAP2_VERTEX_3:
	 *params = (GLint) ctx->Eval.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
	 *params = (GLint) ctx->Eval.Map2Vertex4;
	 break;
      case GL_MAP_COLOR:
	 *params = (GLint) ctx->Pixel.MapColorFlag;
	 break;
      case GL_MAP_STENCIL:
	 *params = (GLint) ctx->Pixel.MapStencilFlag;
	 break;
      case GL_MATRIX_MODE:
	 *params = (GLint) ctx->Transform.MatrixMode;
	 break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
         *params = (GLint) MAX_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
         *params = (GLint) MAX_CLIENT_ATTRIB_STACK_DEPTH;
         break;
      case GL_MAX_CLIP_PLANES:
         *params = (GLint) ctx->Const.MaxClipPlanes;
         break;
      case GL_MAX_ELEMENTS_VERTICES:  /* GL_VERSION_1_2 */
         *params = (GLint) ctx->Const.MaxArrayLockSize;
         break;
      case GL_MAX_ELEMENTS_INDICES:   /* GL_VERSION_1_2 */
         *params = (GLint) ctx->Const.MaxArrayLockSize;
         break;
      case GL_MAX_EVAL_ORDER:
	 *params = (GLint) MAX_EVAL_ORDER;
	 break;
      case GL_MAX_LIGHTS:
         *params = (GLint) ctx->Const.MaxLights;
         break;
      case GL_MAX_LIST_NESTING:
         *params = (GLint) MAX_LIST_NESTING;
         break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
         *params = (GLint) MAX_MODELVIEW_STACK_DEPTH;
         break;
      case GL_MAX_NAME_STACK_DEPTH:
	 *params = (GLint) MAX_NAME_STACK_DEPTH;
	 break;
      case GL_MAX_PIXEL_MAP_TABLE:
	 *params = (GLint) MAX_PIXEL_MAP_TABLE;
	 break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
         *params = (GLint) MAX_PROJECTION_STACK_DEPTH;
         break;
      case GL_MAX_TEXTURE_SIZE:
         *params = (1 << (ctx->Const.MaxTextureLevels - 1));
	 break;
      case GL_MAX_3D_TEXTURE_SIZE:
         *params = (1 << (ctx->Const.Max3DTextureLevels - 1));
	 break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	 *params = (GLint) MAX_TEXTURE_STACK_DEPTH;
	 break;
      case GL_MAX_VIEWPORT_DIMS:
         params[0] = (GLint) MAX_WIDTH;
         params[1] = (GLint) MAX_HEIGHT;
         break;
      case GL_MODELVIEW_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->ModelviewMatrixStack.Top->m[i];
	 }
	 break;
      case GL_MODELVIEW_STACK_DEPTH:
	 *params = (GLint) (ctx->ModelviewMatrixStack.Depth + 1);
	 break;
      case GL_NAME_STACK_DEPTH:
	 *params = (GLint) ctx->Select.NameStackDepth;
	 break;
      case GL_NORMALIZE:
	 *params = (GLint) ctx->Transform.Normalize;
	 break;
      case GL_PACK_ALIGNMENT:
	 *params = ctx->Pack.Alignment;
	 break;
      case GL_PACK_LSB_FIRST:
	 *params = (GLint) ctx->Pack.LsbFirst;
	 break;
      case GL_PACK_ROW_LENGTH:
	 *params = ctx->Pack.RowLength;
	 break;
      case GL_PACK_SKIP_PIXELS:
	 *params = ctx->Pack.SkipPixels;
	 break;
      case GL_PACK_SKIP_ROWS:
	 *params = ctx->Pack.SkipRows;
	 break;
      case GL_PACK_SWAP_BYTES:
	 *params = (GLint) ctx->Pack.SwapBytes;
	 break;
      case GL_PACK_SKIP_IMAGES_EXT:
         *params = ctx->Pack.SkipImages;
         break;
      case GL_PACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Pack.ImageHeight;
         break;
      case GL_PACK_INVERT_MESA:
         *params = ctx->Pack.Invert;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	 *params = (GLint) ctx->Hint.PerspectiveCorrection;
	 break;
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	 *params = ctx->Pixel.MapAtoAsize;
	 break;
      case GL_PIXEL_MAP_B_TO_B_SIZE:
	 *params = ctx->Pixel.MapBtoBsize;
	 break;
      case GL_PIXEL_MAP_G_TO_G_SIZE:
	 *params = ctx->Pixel.MapGtoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_A_SIZE:
	 *params = ctx->Pixel.MapItoAsize;
	 break;
      case GL_PIXEL_MAP_I_TO_B_SIZE:
	 *params = ctx->Pixel.MapItoBsize;
	 break;
      case GL_PIXEL_MAP_I_TO_G_SIZE:
	 *params = ctx->Pixel.MapItoGsize;
	 break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
	 *params = ctx->Pixel.MapItoIsize;
	 break;
      case GL_PIXEL_MAP_I_TO_R_SIZE:
	 *params = ctx->Pixel.MapItoRsize;
	 break;
      case GL_PIXEL_MAP_R_TO_R_SIZE:
	 *params = ctx->Pixel.MapRtoRsize;
	 break;
      case GL_PIXEL_MAP_S_TO_S_SIZE:
	 *params = ctx->Pixel.MapStoSsize;
	 break;
      case GL_POINT_SIZE:
         *params = (GLint) ctx->Point.Size;
         break;
      case GL_POINT_SIZE_GRANULARITY:
	 *params = (GLint) ctx->Const.PointSizeGranularity;
	 break;
      case GL_POINT_SIZE_RANGE:
	 params[0] = (GLint) ctx->Const.MinPointSizeAA;
	 params[1] = (GLint) ctx->Const.MaxPointSizeAA;
	 break;
      case GL_ALIASED_POINT_SIZE_RANGE:
	 params[0] = (GLint) ctx->Const.MinPointSize;
	 params[1] = (GLint) ctx->Const.MaxPointSize;
	 break;
      case GL_POINT_SMOOTH:
	 *params = (GLint) ctx->Point.SmoothFlag;
	 break;
      case GL_POINT_SMOOTH_HINT:
	 *params = (GLint) ctx->Hint.PointSmooth;
	 break;
      case GL_POINT_SIZE_MIN_EXT:
	 *params = (GLint) (ctx->Point.MinSize);
	 break;
      case GL_POINT_SIZE_MAX_EXT:
	 *params = (GLint) (ctx->Point.MaxSize);
	 break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
	 *params = (GLint) (ctx->Point.Threshold);
	 break;
      case GL_DISTANCE_ATTENUATION_EXT:
	 params[0] = (GLint) (ctx->Point.Params[0]);
	 params[1] = (GLint) (ctx->Point.Params[1]);
	 params[2] = (GLint) (ctx->Point.Params[2]);
	 break;
      case GL_POLYGON_MODE:
	 params[0] = (GLint) ctx->Polygon.FrontMode;
	 params[1] = (GLint) ctx->Polygon.BackMode;
	 break;
      case GL_POLYGON_OFFSET_BIAS_EXT: /* GL_EXT_polygon_offset */
         *params = (GLint) ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_OFFSET_FACTOR:
         *params = (GLint) ctx->Polygon.OffsetFactor;
         break;
      case GL_POLYGON_OFFSET_UNITS:
         *params = (GLint) ctx->Polygon.OffsetUnits;
         break;
      case GL_POLYGON_SMOOTH:
	 *params = (GLint) ctx->Polygon.SmoothFlag;
	 break;
      case GL_POLYGON_SMOOTH_HINT:
	 *params = (GLint) ctx->Hint.PolygonSmooth;
	 break;
      case GL_POLYGON_STIPPLE:
         *params = (GLint) ctx->Polygon.StippleFlag;
	 break;
      case GL_PROJECTION_MATRIX:
	 for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->ProjectionMatrixStack.Top->m[i];
	 }
	 break;
      case GL_PROJECTION_STACK_DEPTH:
	 *params = (GLint) (ctx->ProjectionMatrixStack.Depth + 1);
	 break;
      case GL_READ_BUFFER:
	 *params = (GLint) ctx->Pixel.ReadBuffer;
	 break;
      case GL_RED_BIAS:
         *params = (GLint) ctx->Pixel.RedBias;
         break;
      case GL_RED_BITS:
         *params = (GLint) ctx->Visual.redBits;
         break;
      case GL_RED_SCALE:
         *params = (GLint) ctx->Pixel.RedScale;
         break;
      case GL_RENDER_MODE:
	 *params = (GLint) ctx->RenderMode;
	 break;
      case GL_RESCALE_NORMAL:
         *params = (GLint) ctx->Transform.RescaleNormals;
         break;
      case GL_RGBA_MODE:
	 *params = (GLint) ctx->Visual.rgbMode;
	 break;
      case GL_SCISSOR_BOX:
	 params[0] = (GLint) ctx->Scissor.X;
	 params[1] = (GLint) ctx->Scissor.Y;
	 params[2] = (GLint) ctx->Scissor.Width;
	 params[3] = (GLint) ctx->Scissor.Height;
	 break;
      case GL_SCISSOR_TEST:
	 *params = (GLint) ctx->Scissor.Enabled;
	 break;
      case GL_SELECTION_BUFFER_SIZE:
         *params = (GLint) ctx->Select.BufferSize;
         break;
      case GL_SHADE_MODEL:
	 *params = (GLint) ctx->Light.ShadeModel;
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         *params = (GLint) ctx->Texture.SharedPalette;
         break;
      case GL_STENCIL_BITS:
         *params = ctx->Visual.stencilBits;
         break;
      case GL_STENCIL_CLEAR_VALUE:
	 *params = (GLint) ctx->Stencil.Clear;
	 break;
      case GL_STENCIL_FAIL:
	 *params = (GLint) ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_FUNC:
	 *params = (GLint) ctx->Stencil.Function[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	 *params = (GLint) ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	 *params = (GLint) ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_REF:
	 *params = (GLint) ctx->Stencil.Ref[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_TEST:
	 *params = (GLint) ctx->Stencil.Enabled;
	 break;
      case GL_STENCIL_VALUE_MASK:
	 *params = (GLint) ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace];
	 break;
      case GL_STENCIL_WRITEMASK:
	 *params = (GLint) ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace];
	 break;
      case GL_STEREO:
	 *params = (GLint) ctx->Visual.stereoMode;
	 break;
      case GL_SUBPIXEL_BITS:
	 *params = ctx->Const.SubPixelBits;
	 break;
      case GL_TEXTURE_1D:
         *params = _mesa_IsEnabled(GL_TEXTURE_1D) ? 1 : 0;
	 break;
      case GL_TEXTURE_2D:
         *params = _mesa_IsEnabled(GL_TEXTURE_2D) ? 1 : 0;
	 break;
      case GL_TEXTURE_3D:
         *params = _mesa_IsEnabled(GL_TEXTURE_3D) ? 1 : 0;
	 break;
      case GL_TEXTURE_BINDING_1D:
         *params = textureUnit->Current1D->Name;
          break;
      case GL_TEXTURE_BINDING_2D:
         *params = textureUnit->Current2D->Name;
          break;
      case GL_TEXTURE_BINDING_3D:
         *params = textureUnit->Current3D->Name;
          break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = FLOAT_TO_INT( textureUnit->EnvColor[0] );
	 params[1] = FLOAT_TO_INT( textureUnit->EnvColor[1] );
	 params[2] = FLOAT_TO_INT( textureUnit->EnvColor[2] );
	 params[3] = FLOAT_TO_INT( textureUnit->EnvColor[3] );
	 break;
      case GL_TEXTURE_ENV_MODE:
	 *params = (GLint) textureUnit->EnvMode;
	 break;
      case GL_TEXTURE_GEN_S:
	 *params = (textureUnit->TexGenEnabled & S_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_GEN_T:
	 *params = (textureUnit->TexGenEnabled & T_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_GEN_R:
	 *params = (textureUnit->TexGenEnabled & R_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_GEN_Q:
	 *params = (textureUnit->TexGenEnabled & Q_BIT) ? 1 : 0;
	 break;
      case GL_TEXTURE_MATRIX:
         for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->TextureMatrixStack[texUnit].Top->m[i];
	 }
	 break;
      case GL_TEXTURE_STACK_DEPTH:
	 *params = (GLint) (ctx->TextureMatrixStack[texUnit].Depth + 1);
	 break;
      case GL_UNPACK_ALIGNMENT:
	 *params = ctx->Unpack.Alignment;
	 break;
      case GL_UNPACK_LSB_FIRST:
	 *params = (GLint) ctx->Unpack.LsbFirst;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 *params = ctx->Unpack.RowLength;
	 break;
      case GL_UNPACK_SKIP_PIXELS:
	 *params = ctx->Unpack.SkipPixels;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 *params = ctx->Unpack.SkipRows;
	 break;
      case GL_UNPACK_SWAP_BYTES:
	 *params = (GLint) ctx->Unpack.SwapBytes;
	 break;
      case GL_UNPACK_SKIP_IMAGES_EXT:
         *params = ctx->Unpack.SkipImages;
         break;
      case GL_UNPACK_IMAGE_HEIGHT_EXT:
         *params = ctx->Unpack.ImageHeight;
         break;
      case GL_UNPACK_CLIENT_STORAGE_APPLE:
         *params = ctx->Unpack.ClientStorage;
         break;
      case GL_VIEWPORT:
         params[0] = (GLint) ctx->Viewport.X;
         params[1] = (GLint) ctx->Viewport.Y;
         params[2] = (GLint) ctx->Viewport.Width;
         params[3] = (GLint) ctx->Viewport.Height;
         break;
      case GL_ZOOM_X:
	 *params = (GLint) ctx->Pixel.ZoomX;
	 break;
      case GL_ZOOM_Y:
	 *params = (GLint) ctx->Pixel.ZoomY;
	 break;
      case GL_VERTEX_ARRAY:
         *params = (GLint) ctx->Array.Vertex.Enabled;
         break;
      case GL_VERTEX_ARRAY_SIZE:
         *params = ctx->Array.Vertex.Size;
         break;
      case GL_VERTEX_ARRAY_TYPE:
         *params = ctx->Array.Vertex.Type;
         break;
      case GL_VERTEX_ARRAY_STRIDE:
         *params = ctx->Array.Vertex.Stride;
         break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_NORMAL_ARRAY:
         *params = (GLint) ctx->Array.Normal.Enabled;
         break;
      case GL_NORMAL_ARRAY_TYPE:
         *params = ctx->Array.Normal.Type;
         break;
      case GL_NORMAL_ARRAY_STRIDE:
         *params = ctx->Array.Normal.Stride;
         break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_COLOR_ARRAY:
         *params = (GLint) ctx->Array.Color.Enabled;
         break;
      case GL_COLOR_ARRAY_SIZE:
         *params = ctx->Array.Color.Size;
         break;
      case GL_COLOR_ARRAY_TYPE:
         *params = ctx->Array.Color.Type;
         break;
      case GL_COLOR_ARRAY_STRIDE:
         *params = ctx->Array.Color.Stride;
         break;
      case GL_COLOR_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_INDEX_ARRAY:
         *params = (GLint) ctx->Array.Index.Enabled;
         break;
      case GL_INDEX_ARRAY_TYPE:
         *params = ctx->Array.Index.Type;
         break;
      case GL_INDEX_ARRAY_STRIDE:
         *params = ctx->Array.Index.Stride;
         break;
      case GL_INDEX_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         *params = (GLint) ctx->Array.TexCoord[clientUnit].Enabled;
         break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
         *params = ctx->Array.TexCoord[clientUnit].Size;
         break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
         *params = ctx->Array.TexCoord[clientUnit].Type;
         break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
         *params = ctx->Array.TexCoord[clientUnit].Stride;
         break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
         *params = 0;
         break;
      case GL_EDGE_FLAG_ARRAY:
         *params = (GLint) ctx->Array.EdgeFlag.Enabled;
         break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
         *params = ctx->Array.EdgeFlag.Stride;
         break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
         *params = 0;
         break;

      /* GL_ARB_multitexture */
      case GL_MAX_TEXTURE_UNITS_ARB:
         CHECK_EXTENSION_I(ARB_multitexture, pname);
         *params = MIN2(ctx->Const.MaxTextureImageUnits,
                        ctx->Const.MaxTextureCoordUnits);
         break;
      case GL_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_I(ARB_multitexture, pname);
         *params = GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit;
         break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
         CHECK_EXTENSION_I(ARB_multitexture, pname);
         *params = GL_TEXTURE0_ARB + clientUnit;
         break;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION_I(ARB_texture_cube_map, pname);
         *params = (GLint) _mesa_IsEnabled(GL_TEXTURE_CUBE_MAP_ARB);
         break;
      case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
         CHECK_EXTENSION_I(ARB_texture_cube_map, pname);
         *params = textureUnit->CurrentCubeMap->Name;
         break;
      case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
         CHECK_EXTENSION_I(ARB_texture_cube_map, pname);
         *params = (1 << (ctx->Const.MaxCubeTextureLevels - 1));
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSION_HINT_ARB:
         CHECK_EXTENSION_I(ARB_texture_compression, pname);
         *params = (GLint) ctx->Hint.TextureCompression;
         break;
      case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_I(ARB_texture_compression, pname);
         *params = (GLint) _mesa_get_compressed_formats(ctx, NULL);
         break;
      case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
         CHECK_EXTENSION_I(ARB_texture_compression, pname);
         (void) _mesa_get_compressed_formats(ctx, params);
         break;

      /* GL_EXT_compiled_vertex_array */
      case GL_ARRAY_ELEMENT_LOCK_FIRST_EXT:
         CHECK_EXTENSION_I(EXT_compiled_vertex_array, pname);
	 *params = ctx->Array.LockFirst;
	 break;
      case GL_ARRAY_ELEMENT_LOCK_COUNT_EXT:
         CHECK_EXTENSION_I(EXT_compiled_vertex_array, pname);
	 *params = ctx->Array.LockCount;
	 break;

      /* GL_ARB_transpose_matrix */
      case GL_TRANSPOSE_COLOR_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ColorMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLint) tm[i];
            }
         }
         break;
      case GL_TRANSPOSE_MODELVIEW_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ModelviewMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLint) tm[i];
            }
         }
         break;
      case GL_TRANSPOSE_PROJECTION_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->ProjectionMatrixStack.Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLint) tm[i];
            }
         }
         break;
      case GL_TRANSPOSE_TEXTURE_MATRIX_ARB:
         {
            GLfloat tm[16];
            GLuint i;
            _math_transposef(tm, ctx->TextureMatrixStack[texUnit].Top->m);
            for (i=0;i<16;i++) {
               params[i] = (GLint) tm[i];
            }
         }
         break;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         CHECK_EXTENSION_I(HP_occlusion_test, pname);
         *params = (GLint) ctx->Depth.OcclusionTest;
         break;
      case GL_OCCLUSION_TEST_RESULT_HP:
         CHECK_EXTENSION_I(HP_occlusion_test, pname);
         if (ctx->Depth.OcclusionTest)
            *params = (GLint) ctx->OcclusionResult;
         else
            *params = (GLint) ctx->OcclusionResultSaved;
         /* reset flag now */
         ctx->OcclusionResult = GL_FALSE;
         ctx->OcclusionResultSaved = GL_FALSE;
         break;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         CHECK_EXTENSION_I(SGIS_pixel_texture, pname);
         *params = (GLint) ctx->Pixel.PixelTextureEnabled;
         break;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         CHECK_EXTENSION_I(SGIX_pixel_texture, pname);
         *params = (GLint) ctx->Pixel.PixelTextureEnabled;
         break;
      case GL_PIXEL_TEX_GEN_MODE_SGIX:
         CHECK_EXTENSION_I(SGIX_pixel_texture, pname);
         *params = (GLint) pixel_texgen_mode(ctx);
         break;

      /* GL_SGI_color_matrix (also in 1.2 imaging) */
      case GL_COLOR_MATRIX_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         for (i=0;i<16;i++) {
	    params[i] = (GLint) ctx->ColorMatrixStack.Top->m[i];
	 }
	 break;
      case GL_COLOR_MATRIX_STACK_DEPTH_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = ctx->ColorMatrixStack.Depth + 1;
         break;
      case GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = MAX_COLOR_STACK_DEPTH;
         break;
      case GL_POST_COLOR_MATRIX_RED_SCALE_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixScale[0];
         break;
      case GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixScale[1];
         break;
      case GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixScale[2];
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixScale[3];
         break;
      case GL_POST_COLOR_MATRIX_RED_BIAS_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixBias[0];
         break;
      case GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixBias[1];
         break;
      case GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixBias[2];
         break;
      case GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI:
         CHECK_EXTENSION_I(SGI_color_matrix, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixBias[3];
         break;

      /* GL_EXT_convolution (also in 1.2 imaging) */
      case GL_CONVOLUTION_1D_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.Convolution1DEnabled;
         break;
      case GL_CONVOLUTION_2D:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.Convolution2DEnabled;
         break;
      case GL_SEPARABLE_2D:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.Separable2DEnabled;
         break;
      case GL_POST_CONVOLUTION_RED_SCALE_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionScale[0];
         break;
      case GL_POST_CONVOLUTION_GREEN_SCALE_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionScale[1];
         break;
      case GL_POST_CONVOLUTION_BLUE_SCALE_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionScale[2];
         break;
      case GL_POST_CONVOLUTION_ALPHA_SCALE_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionScale[3];
         break;
      case GL_POST_CONVOLUTION_RED_BIAS_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionBias[0];
         break;
      case GL_POST_CONVOLUTION_GREEN_BIAS_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionBias[1];
         break;
      case GL_POST_CONVOLUTION_BLUE_BIAS_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionBias[2];
         break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS_EXT:
         CHECK_EXTENSION_I(EXT_convolution, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionBias[2];
         break;

      /* GL_EXT_histogram (also in 1.2 imaging) */
      case GL_HISTOGRAM:
         CHECK_EXTENSION_I(EXT_histogram, pname);
         *params = (GLint) ctx->Pixel.HistogramEnabled;
	 break;
      case GL_MINMAX:
         CHECK_EXTENSION_I(EXT_histogram, pname);
         *params = (GLint) ctx->Pixel.MinMaxEnabled;
         break;

      /* GL_SGI_color_table (also in 1.2 imaging */
      case GL_COLOR_TABLE_SGI:
         CHECK_EXTENSION_I(SGI_color_table, pname);
         *params = (GLint) ctx->Pixel.ColorTableEnabled;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         CHECK_EXTENSION_I(SGI_color_table, pname);
         *params = (GLint) ctx->Pixel.PostConvolutionColorTableEnabled;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         CHECK_EXTENSION_I(SGI_color_table, pname);
         *params = (GLint) ctx->Pixel.PostColorMatrixColorTableEnabled;
         break;

      /* GL_SGI_texture_color_table */
      case GL_TEXTURE_COLOR_TABLE_SGI:
         CHECK_EXTENSION_I(SGI_texture_color_table, pname);
         *params = (GLint) textureUnit->ColorTableEnabled;
         break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION_I(EXT_secondary_color, pname);
	 *params = (GLint) ctx->Fog.ColorSumEnabled;
	 break;
      case GL_CURRENT_SECONDARY_COLOR_EXT:
         CHECK_EXTENSION_I(EXT_secondary_color, pname);
	 FLUSH_CURRENT(ctx, 0);
         params[0] = FLOAT_TO_INT( (ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0]) );
         params[1] = FLOAT_TO_INT( (ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1]) );
         params[2] = FLOAT_TO_INT( (ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2]) );
         params[3] = FLOAT_TO_INT( (ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3]) );
	 break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         CHECK_EXTENSION_I(EXT_secondary_color, pname);
         *params = (GLint) ctx->Array.SecondaryColor.Enabled;
         break;
      case GL_SECONDARY_COLOR_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_I(EXT_secondary_color, pname);
	 *params = (GLint) ctx->Array.SecondaryColor.Type;
	 break;
      case GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_I(EXT_secondary_color, pname);
	 *params = (GLint) ctx->Array.SecondaryColor.Stride;
	 break;
      case GL_SECONDARY_COLOR_ARRAY_SIZE_EXT:
         CHECK_EXTENSION_I(EXT_secondary_color, pname);
	 *params = (GLint) ctx->Array.SecondaryColor.Size;
	 break;

      /* GL_EXT_fog_coord */
      case GL_CURRENT_FOG_COORDINATE_EXT:
         CHECK_EXTENSION_I(EXT_fog_coord, pname);
         FLUSH_CURRENT(ctx, 0);
         *params = (GLint) ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
	 break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         CHECK_EXTENSION_I(EXT_fog_coord, pname);
         *params = (GLint) ctx->Array.FogCoord.Enabled;
         break;
      case GL_FOG_COORDINATE_ARRAY_TYPE_EXT:
         CHECK_EXTENSION_I(EXT_fog_coord, pname);
         *params = (GLint) ctx->Array.FogCoord.Type;
	 break;
      case GL_FOG_COORDINATE_ARRAY_STRIDE_EXT:
         CHECK_EXTENSION_I(EXT_fog_coord, pname);
         *params = (GLint) ctx->Array.FogCoord.Stride;
	 break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
         CHECK_EXTENSION_I(EXT_fog_coord, pname);
	 *params = (GLint) ctx->Fog.FogCoordinateSource;
	 break;

      /* GL_EXT_texture_lod_bias */
      case GL_MAX_TEXTURE_LOD_BIAS_EXT:
         *params = (GLint) ctx->Const.MaxTextureLodBias;
         break;

      /* GL_EXT_texture_filter_anisotropic */
      case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
         CHECK_EXTENSION_I(EXT_texture_filter_anisotropic, pname);
         *params = (GLint) ctx->Const.MaxTextureMaxAnisotropy;
	 break;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = (GLint) ctx->Multisample.Enabled;
         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = (GLint) ctx->Multisample.SampleAlphaToCoverage;
         break;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = (GLint) ctx->Multisample.SampleAlphaToOne;
         break;
      case GL_SAMPLE_COVERAGE_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = (GLint) ctx->Multisample.SampleCoverage;
         break;
      case GL_SAMPLE_COVERAGE_VALUE_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = (GLint) ctx->Multisample.SampleCoverageValue;
         break;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = (GLint) ctx->Multisample.SampleCoverageInvert;
         break;
      case GL_SAMPLE_BUFFERS_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = 0; /* XXX fix someday */
         break;
      case GL_SAMPLES_ARB:
         CHECK_EXTENSION_I(ARB_multisample, pname);
         *params = 0; /* XXX fix someday */
         break;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION_I(IBM_rasterpos_clip, pname);
         *params = (GLint) ctx->Transform.RasterPositionUnclipped;
         break;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION2_I(NV_point_sprite, ARB_point_sprite, pname);
         *params = (GLint) ctx->Point.PointSprite;
         break;
      case GL_POINT_SPRITE_R_MODE_NV:
         CHECK_EXTENSION_I(NV_point_sprite, pname);
         *params = (GLint) ctx->Point.SpriteRMode;
         break;
      case GL_POINT_SPRITE_COORD_ORIGIN:
         CHECK_EXTENSION_I(ARB_point_sprite, pname);
         *params = (GLint) ctx->Point.SpriteOrigin;
         break;

      /* GL_SGIS_generate_mipmap */
      case GL_GENERATE_MIPMAP_HINT_SGIS:
         CHECK_EXTENSION_I(SGIS_generate_mipmap, pname);
         *params = (GLint) ctx->Hint.GenerateMipmap;
         break;

#if FEATURE_NV_vertex_program
      case GL_VERTEX_PROGRAM_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = (GLint) ctx->VertexProgram.Enabled;
         break;
      case GL_VERTEX_PROGRAM_POINT_SIZE_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = (GLint) ctx->VertexProgram.PointSizeEnabled;
         break;
      case GL_VERTEX_PROGRAM_TWO_SIDE_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = (GLint) ctx->VertexProgram.TwoSideEnabled;
         break;
      case GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = ctx->Const.MaxProgramMatrixStackDepth;
         break;
      case GL_MAX_TRACK_MATRICES_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = ctx->Const.MaxProgramMatrices;
         break;
      case GL_CURRENT_MATRIX_STACK_DEPTH_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = ctx->CurrentStack->Depth + 1;
         break;
      case GL_CURRENT_MATRIX_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         for (i = 0; i < 16; i++)
            params[i] = (GLint) ctx->CurrentStack->Top->m[i];
         break;
      case GL_VERTEX_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         if (ctx->VertexProgram.Current)
            *params = (GLint) ctx->VertexProgram.Current->Base.Id;
         else
            *params = 0;
         break;
      case GL_PROGRAM_ERROR_POSITION_NV:
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         *params = (GLint) ctx->Program.ErrorPos;
         break;
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
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_VERTEX_ATTRIB_ARRAY0_NV;
            *params = (GLint) ctx->Array.VertexAttrib[n].Enabled;
         }
         break;
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
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP1_VERTEX_ATTRIB0_4_NV;
            *params = (GLint) ctx->Eval.Map1Attrib[n];
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
         CHECK_EXTENSION_I(NV_vertex_program, pname);
         {
            GLuint n = (GLuint) pname - GL_MAP2_VERTEX_ATTRIB0_4_NV;
            *params = (GLint) ctx->Eval.Map2Attrib[n];
         }
         break;
#endif /* FEATURE_NV_vertex_program */

#if FEATURE_NV_fragment_program
      case GL_FRAGMENT_PROGRAM_NV:
         CHECK_EXTENSION_I(NV_fragment_program, pname);
         *params = (GLint) ctx->FragmentProgram.Enabled;
         break;
      case GL_MAX_TEXTURE_COORDS_NV:
         CHECK_EXTENSION_I(NV_fragment_program, pname);
         *params = (GLint) ctx->Const.MaxTextureCoordUnits;
         break;
      case GL_MAX_TEXTURE_IMAGE_UNITS_NV:
         CHECK_EXTENSION_I(NV_fragment_program, pname);
         *params = (GLint) ctx->Const.MaxTextureImageUnits;
         break;
      case GL_FRAGMENT_PROGRAM_BINDING_NV:
         CHECK_EXTENSION_I(NV_fragment_program, pname);
         if (ctx->FragmentProgram.Current)
            *params = (GLint) ctx->FragmentProgram.Current->Base.Id;
         else
            *params = 0;
         break;
      case GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV:
         CHECK_EXTENSION_I(NV_fragment_program, pname);
         *params = MAX_NV_FRAGMENT_PROGRAM_PARAMS;
         break;
#endif /* FEATURE_NV_fragment_program */

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION_I(NV_texture_rectangle, pname);
         *params = (GLint) _mesa_IsEnabled(GL_TEXTURE_RECTANGLE_NV);
         break;
      case GL_TEXTURE_BINDING_RECTANGLE_NV:
         CHECK_EXTENSION_I(NV_texture_rectangle, pname);
         *params = (GLint) textureUnit->CurrentRect->Name;
         break;
      case GL_MAX_RECTANGLE_TEXTURE_SIZE_NV:
         CHECK_EXTENSION_I(NV_texture_rectangle, pname);
         *params = (GLint) ctx->Const.MaxTextureRectSize;
         break;

      /* GL_EXT_stencil_two_side */
      case GL_STENCIL_TEST_TWO_SIDE_EXT:
         CHECK_EXTENSION_I(EXT_stencil_two_side, pname);
         *params = (GLint) ctx->Stencil.TestTwoSide;
         break;
      case GL_ACTIVE_STENCIL_FACE_EXT:
         CHECK_EXTENSION_I(EXT_stencil_two_side, pname);
         *params = (GLint) (ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT);
         break;

      /* GL_NV_light_max_exponent */
      case GL_MAX_SHININESS_NV:
         *params = (GLint) ctx->Const.MaxShininess;
         break;
      case GL_MAX_SPOT_EXPONENT_NV:
         *params = (GLint) ctx->Const.MaxSpotExponent;
         break;

#if FEATURE_ARB_vertex_buffer_object
      case GL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.ArrayBufferObj->Name;
         break;
      case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.Vertex.BufferObj->Name;
         break;
      case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.Normal.BufferObj->Name;
         break;
      case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.Color.BufferObj->Name;
         break;
      case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.Index.BufferObj->Name;
         break;
      case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.TexCoord[clientUnit].BufferObj->Name;
         break;
      case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.EdgeFlag.BufferObj->Name;
         break;
      case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.SecondaryColor.BufferObj->Name;
         break;
      case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.FogCoord.BufferObj->Name;
         break;
      /*case GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB: - not supported */
      case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
         CHECK_EXTENSION_I(ARB_vertex_buffer_object, pname);
         *params = (GLint) ctx->Array.ElementArrayBufferObj->Name;
         break;
#endif
#if FEATURE_EXT_pixel_buffer_object
      case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_I(EXT_pixel_buffer_object, pname);
         *params = (GLint) ctx->Pack.BufferObj->Name;
         break;
      case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
         CHECK_EXTENSION_I(EXT_pixel_buffer_object, pname);
         *params = (GLint) ctx->Unpack.BufferObj->Name;
         break;
#endif

#if FEATURE_ARB_vertex_program
      /* GL_NV_vertex_program and GL_ARB_fragment_program define others */
      case GL_MAX_VERTEX_ATTRIBS_ARB:
         CHECK_EXTENSION_I(ARB_vertex_program, pname);
         *params = (GLint) ctx->Const.MaxVertexProgramAttribs;
         break;
#endif

#if FEATURE_ARB_fragment_program
      case GL_FRAGMENT_PROGRAM_ARB:
         CHECK_EXTENSION_I(ARB_fragment_program, pname);
         *params = ctx->FragmentProgram.Enabled;
         break;
      case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
         CHECK_EXTENSION_I(ARB_fragment_program, pname);
         params[0] = (GLint) ctx->CurrentStack->Top->m[0];
         params[1] = (GLint) ctx->CurrentStack->Top->m[4];
         params[2] = (GLint) ctx->CurrentStack->Top->m[8];
         params[3] = (GLint) ctx->CurrentStack->Top->m[12];
         params[4] = (GLint) ctx->CurrentStack->Top->m[1];
         params[5] = (GLint) ctx->CurrentStack->Top->m[5];
         params[6] = (GLint) ctx->CurrentStack->Top->m[9];
         params[7] = (GLint) ctx->CurrentStack->Top->m[13];
         params[8] = (GLint) ctx->CurrentStack->Top->m[2];
         params[9] = (GLint) ctx->CurrentStack->Top->m[6];
         params[10] = (GLint) ctx->CurrentStack->Top->m[10];
         params[11] = (GLint) ctx->CurrentStack->Top->m[14];
         params[12] = (GLint) ctx->CurrentStack->Top->m[3];
         params[13] = (GLint) ctx->CurrentStack->Top->m[7];
         params[14] = (GLint) ctx->CurrentStack->Top->m[11];
         params[15] = (GLint) ctx->CurrentStack->Top->m[15];
         break;
      /* Remaining ARB_fragment_program queries alias with
       * the GL_NV_fragment_program queries.
       */
#endif

      /* GL_EXT_depth_bounds_test */
      case GL_DEPTH_BOUNDS_TEST_EXT:
         CHECK_EXTENSION_I(EXT_depth_bounds_test, pname);
         params[0] = ctx->Depth.BoundsTest;
         break;
      case GL_DEPTH_BOUNDS_EXT:
         CHECK_EXTENSION_I(EXT_depth_bounds_test, pname);
         params[0] = (GLint) ctx->Depth.BoundsMin;
         params[1] = (GLint) ctx->Depth.BoundsMax;
         break;

#if FEATURE_MESA_program_debug
      case GL_FRAGMENT_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_I(MESA_program_debug, pname);
         *params = (GLint) ctx->FragmentProgram.CallbackEnabled;
         break;
      case GL_VERTEX_PROGRAM_CALLBACK_MESA:
         CHECK_EXTENSION_I(MESA_program_debug, pname);
         *params = (GLint) ctx->VertexProgram.CallbackEnabled;
         break;
      case GL_FRAGMENT_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_I(MESA_program_debug, pname);
         *params = (GLint) ctx->FragmentProgram.CurrentPosition;
         break;
      case GL_VERTEX_PROGRAM_POSITION_MESA:
         CHECK_EXTENSION_I(MESA_program_debug, pname);
         *params = (GLint) ctx->VertexProgram.CurrentPosition;
         break;
#endif

      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetIntegerv(pname=0x%x)", pname);
   }
}


/**
 * Get the address of a selected pointer.
 *
 * \param pname array or buffer to be returned.
 * \param params will hold the pointer speficifed by \p pname.
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
         *params = (GLvoid *) ctx->Array.Vertex.Ptr;
         break;
      case GL_NORMAL_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.Normal.Ptr;
         break;
      case GL_COLOR_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.Color.Ptr;
         break;
      case GL_SECONDARY_COLOR_ARRAY_POINTER_EXT:
         *params = (GLvoid *) ctx->Array.SecondaryColor.Ptr;
         break;
      case GL_FOG_COORDINATE_ARRAY_POINTER_EXT:
         *params = (GLvoid *) ctx->Array.FogCoord.Ptr;
         break;
      case GL_INDEX_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.Index.Ptr;
         break;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.TexCoord[clientUnit].Ptr;
         break;
      case GL_EDGE_FLAG_ARRAY_POINTER:
         *params = (GLvoid *) ctx->Array.EdgeFlag.Ptr;
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
 * Get a string describing the current GL connection.
 *
 * \param name name symbolic constant.
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
   static const char *version_1_2 = "1.2 Mesa " MESA_VERSION_STRING;
   static const char *version_1_3 = "1.3 Mesa " MESA_VERSION_STRING;
   static const char *version_1_4 = "1.4 Mesa " MESA_VERSION_STRING;
   static const char *version_1_5 = "1.5 Mesa " MESA_VERSION_STRING;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   /* this is a required driver function */
   assert(ctx->Driver.GetString);
   {
      const GLubyte *str = (*ctx->Driver.GetString)(ctx, name);
      if (str)
         return str;

      switch (name) {
         case GL_VENDOR:
            return (const GLubyte *) vendor;
         case GL_RENDERER:
            return (const GLubyte *) renderer;
         case GL_VERSION:
            if (ctx->Extensions.ARB_multisample &&
                ctx->Extensions.ARB_multitexture &&
                ctx->Extensions.ARB_texture_border_clamp &&
                ctx->Extensions.ARB_texture_compression &&
                ctx->Extensions.ARB_texture_cube_map &&
                ctx->Extensions.EXT_texture_env_add &&
                ctx->Extensions.ARB_texture_env_combine &&
                ctx->Extensions.ARB_texture_env_dot3) {
               if (ctx->Extensions.ARB_depth_texture &&
                   ctx->Extensions.ARB_shadow &&
                   ctx->Extensions.ARB_texture_env_crossbar &&
                   ctx->Extensions.ARB_texture_mirrored_repeat &&
                   ctx->Extensions.ARB_window_pos &&
                   ctx->Extensions.EXT_blend_color &&
                   ctx->Extensions.EXT_blend_func_separate &&
                   ctx->Extensions.EXT_blend_logic_op &&
                   ctx->Extensions.EXT_blend_minmax &&
                   ctx->Extensions.EXT_blend_subtract &&
                   ctx->Extensions.EXT_fog_coord &&
                   ctx->Extensions.EXT_multi_draw_arrays &&
                   ctx->Extensions.EXT_point_parameters && /*aka ARB*/
                   ctx->Extensions.EXT_secondary_color &&
                   ctx->Extensions.EXT_stencil_wrap &&
                   ctx->Extensions.EXT_texture_lod_bias &&
                   ctx->Extensions.SGIS_generate_mipmap) {
                  if (ctx->Extensions.ARB_occlusion_query &&
                      ctx->Extensions.ARB_vertex_buffer_object &&
                      ctx->Extensions.EXT_shadow_funcs) {
                     return (const GLubyte *) version_1_5;
                  }
                  else {
                     return (const GLubyte *) version_1_4;
                  }
               }
               else {
                  return (const GLubyte *) version_1_3;
               }
            }
            else {
               return (const GLubyte *) version_1_2;
            }
         case GL_EXTENSIONS:
            if (!ctx->Extensions.String)
               ctx->Extensions.String = _mesa_make_extension_string(ctx);
            return (const GLubyte *) ctx->Extensions.String;
#if FEATURE_NV_fragment_program
         case GL_PROGRAM_ERROR_STRING_NV:
            if (ctx->Extensions.NV_fragment_program) {
               return (const GLubyte *) ctx->Program.ErrorString;
            }
            /* FALL-THROUGH */
#endif
         default:
            _mesa_error( ctx, GL_INVALID_ENUM, "glGetString" );
            return (const GLubyte *) 0;
      }
   }
}


/**
 * Execute a glGetError() command.
 *
 * \return error number.
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
