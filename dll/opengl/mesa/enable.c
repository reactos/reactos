/* $Id: enable.c,v 1.23 1997/10/29 02:23:54 brianp Exp $ */

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


/*
 * $Log: enable.c,v $
 * Revision 1.23  1997/10/29 02:23:54  brianp
 * added UseGlobalTexturePalette() dd function (David Bucciarelli v20 3dfx)
 *
 * Revision 1.22  1997/10/16 01:59:08  brianp
 * added GL_EXT_shared_texture_palette extension
 *
 * Revision 1.21  1997/07/24 01:25:01  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.20  1997/06/20 02:20:32  brianp
 * replaced Current.IntColor with Current.ByteColor
 *
 * Revision 1.19  1997/05/31 16:57:14  brianp
 * added MESA_NO_RASTER env var support
 *
 * Revision 1.18  1997/05/28 03:24:22  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.17  1997/04/24 01:49:30  brianp
 * call gl_set_color_function() instead of directly setting pointers
 *
 * Revision 1.16  1997/04/12 16:54:05  brianp
 * new NEW_POLYGON state flag
 *
 * Revision 1.15  1997/04/12 16:20:27  brianp
 * removed call to Driver.Dither()
 *
 * Revision 1.14  1997/04/02 03:10:36  brianp
 * changed some #include's
 *
 * Revision 1.13  1997/02/27 19:59:08  brianp
 * issue a warning if enable depth or stencil test without such a buffer
 *
 * Revision 1.12  1997/02/09 19:53:43  brianp
 * now use TEXTURE_xD enable constants
 *
 * Revision 1.11  1997/02/09 18:49:37  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.10  1997/01/28 22:13:42  brianp
 * now there's separate state for CI and RGBA logic op enabled
 *
 * Revision 1.9  1996/12/18 20:00:57  brianp
 * gl_set_material() now takes a bitmask instead of face and pname
 *
 * Revision 1.8  1996/12/11 20:16:49  brianp
 * more work on the GL_COLOR_MATERIAL bug
 *
 * Revision 1.7  1996/12/09 22:51:51  brianp
 * update API Color4f and Color4ub pointers for GL_COLOR_MATERIAL
 *
 * Revision 1.6  1996/12/07 10:21:07  brianp
 * call gl_set_material() instead of gl_Materialfv()
 *
 * Revision 1.5  1996/11/09 03:11:18  brianp
 * added missing GL_EXT_vertex_array caps to gl_enable()
 *
 * Revision 1.4  1996/10/11 03:44:09  brianp
 * added comments for GL_POLYGON_OFFSET_EXT symbol
 *
 * Revision 1.3  1996/09/27 01:26:40  brianp
 * removed unused variables
 *
 * Revision 1.2  1996/09/15 14:17:30  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <string.h>
#include "context.h"
#include "depth.h"
#include "enable.h"
#include "light.h"
#include "dlist.h"
#include "macros.h"
#include "stencil.h"
#include "types.h"
#include "vbfill.h"
#endif



/*
 * Perform glEnable and glDisable calls.
 */
static void gl_enable( GLcontext* ctx, GLenum cap, GLboolean state )
{
   GLuint p;

   if (INSIDE_BEGIN_END(ctx)) {
      if (state) {
	 gl_error( ctx, GL_INVALID_OPERATION, "glEnable" );
      }
      else {
	 gl_error( ctx, GL_INVALID_OPERATION, "glDisable" );
      }
      return;
   }

   switch (cap) {
      case GL_ALPHA_TEST:
         if (ctx->Color.AlphaEnabled!=state) {
            ctx->Color.AlphaEnabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_AUTO_NORMAL:
	 ctx->Eval.AutoNormal = state;
	 break;
      case GL_BLEND:
         if (ctx->Color.BlendEnabled!=state) {
            ctx->Color.BlendEnabled = state;
            /* The following needed to accomodate 1.0 RGB logic op blending */
            ctx->Color.ColorLogicOpEnabled = GL_FALSE;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
	 ctx->Transform.ClipEnabled[cap-GL_CLIP_PLANE0] = state;
	 /* Check if any clip planes enabled */
         ctx->Transform.AnyClip = GL_FALSE;
         for (p=0;p<MAX_CLIP_PLANES;p++) {
            if (ctx->Transform.ClipEnabled[p]) {
               ctx->Transform.AnyClip = GL_TRUE;
               break;
            }
         }
	 break;
      case GL_COLOR_MATERIAL:
         if (ctx->Light.ColorMaterialEnabled!=state) {
            ctx->Light.ColorMaterialEnabled = state;
            if (state) {
               GLfloat color[4];
               color[0] = ctx->Current.ByteColor[0] * ctx->Visual->InvRedScale;
               color[1] = ctx->Current.ByteColor[1] * ctx->Visual->InvGreenScale;
               color[2] = ctx->Current.ByteColor[2] * ctx->Visual->InvBlueScale;
               color[3] = ctx->Current.ByteColor[3] * ctx->Visual->InvAlphaScale;
               /* update material with current color */
               gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
            }
            gl_set_color_function(ctx);
            ctx->NewState |= NEW_LIGHTING;
         }
	 break;
      case GL_CULL_FACE:
         if (ctx->Polygon.CullFlag!=state) {
            ctx->Polygon.CullFlag = state;
            ctx->NewState |= NEW_POLYGON;
         }
	 break;
      case GL_DEPTH_TEST:
         if (state && ctx->Visual->DepthBits==0) {
            gl_warning(ctx,"glEnable(GL_DEPTH_TEST) but no depth buffer");
            return;
         }
	 if (ctx->Depth.Test!=state) {
            ctx->Depth.Test = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
         break;
      case GL_DITHER:
         if (ctx->NoDither) {
            /* MESA_NO_DITHER env var */
            state = GL_FALSE;
         }
         if (ctx->Color.DitherFlag!=state) {
            ctx->Color.DitherFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_FOG:
	 if (ctx->Fog.Enabled!=state) {
            ctx->Fog.Enabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
         ctx->Light.Light[cap-GL_LIGHT0].Enabled = state;
         ctx->NewState |= NEW_LIGHTING;
         break;
      case GL_LIGHTING:
         if (ctx->Light.Enabled!=state) {
            ctx->Light.Enabled = state;
            ctx->NewState |= NEW_LIGHTING;
         }
         break;
      case GL_LINE_SMOOTH:
	 if (ctx->Line.SmoothFlag!=state) {
            ctx->Line.SmoothFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_LINE_STIPPLE:
	 if (ctx->Line.StippleFlag!=state) {
            ctx->Line.StippleFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_INDEX_LOGIC_OP:
         if (ctx->Color.IndexLogicOpEnabled!=state) {
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 ctx->Color.IndexLogicOpEnabled = state;
	 break;
      case GL_COLOR_LOGIC_OP:
         if (ctx->Color.ColorLogicOpEnabled!=state) {
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 ctx->Color.ColorLogicOpEnabled = state;
	 break;
      case GL_MAP1_COLOR_4:
	 ctx->Eval.Map1Color4 = state;
	 break;
      case GL_MAP1_INDEX:
	 ctx->Eval.Map1Index = state;
	 break;
      case GL_MAP1_NORMAL:
	 ctx->Eval.Map1Normal = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 ctx->Eval.Map1TextureCoord1 = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 ctx->Eval.Map1TextureCoord2 = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 ctx->Eval.Map1TextureCoord3 = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 ctx->Eval.Map1TextureCoord4 = state;
	 break;
      case GL_MAP1_VERTEX_3:
	 ctx->Eval.Map1Vertex3 = state;
	 break;
      case GL_MAP1_VERTEX_4:
	 ctx->Eval.Map1Vertex4 = state;
	 break;
      case GL_MAP2_COLOR_4:
	 ctx->Eval.Map2Color4 = state;
	 break;
      case GL_MAP2_INDEX:
	 ctx->Eval.Map2Index = state;
	 break;
      case GL_MAP2_NORMAL:
	 ctx->Eval.Map2Normal = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_1: 
	 ctx->Eval.Map2TextureCoord1 = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 ctx->Eval.Map2TextureCoord2 = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 ctx->Eval.Map2TextureCoord3 = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 ctx->Eval.Map2TextureCoord4 = state;
	 break;
      case GL_MAP2_VERTEX_3:
	 ctx->Eval.Map2Vertex3 = state;
	 break;
      case GL_MAP2_VERTEX_4:
	 ctx->Eval.Map2Vertex4 = state;
	 break;
      case GL_NORMALIZE:
	 ctx->Transform.Normalize = state;
	 break;
      case GL_POINT_SMOOTH:
	 if (ctx->Point.SmoothFlag!=state) {
            ctx->Point.SmoothFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_POLYGON_SMOOTH:
	 if (ctx->Polygon.SmoothFlag!=state) {
            ctx->Polygon.SmoothFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_POLYGON_STIPPLE:
	 if (ctx->Polygon.StippleFlag!=state) {
            ctx->Polygon.StippleFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_POLYGON_OFFSET_POINT:
         if (ctx->Polygon.OffsetPoint!=state) {
            ctx->Polygon.OffsetPoint = state;
            ctx->NewState |= NEW_POLYGON;
         }
         break;
      case GL_POLYGON_OFFSET_LINE:
         if (ctx->Polygon.OffsetLine!=state) {
            ctx->Polygon.OffsetLine = state;
            ctx->NewState |= NEW_POLYGON;
         }
         break;
      case GL_POLYGON_OFFSET_FILL:
      /*case GL_POLYGON_OFFSET_EXT:*/
         if (ctx->Polygon.OffsetFill!=state) {
            ctx->Polygon.OffsetFill = state;
            ctx->NewState |= NEW_POLYGON;
         }
         break;
      case GL_SCISSOR_TEST:
         if (ctx->Scissor.Enabled!=state) {
            ctx->Scissor.Enabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_STENCIL_TEST:
	 if (state && ctx->Visual->StencilBits==0) {
            gl_warning(ctx, "glEnable(GL_STENCIL_TEST) but no stencil buffer");
            return;
	 }
	 if (ctx->Stencil.Enabled!=state) {
            ctx->Stencil.Enabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_TEXTURE_1D:
         if (ctx->Visual->RGBAflag) {
            /* texturing only works in RGB mode */
            if (state) {
               ctx->Texture.Enabled |= TEXTURE_1D;
            }
            else {
               ctx->Texture.Enabled &= (~TEXTURE_1D);
            }
            ctx->NewState |= (NEW_RASTER_OPS | NEW_TEXTURING);
         }
	 break;
      case GL_TEXTURE_2D:
         if (ctx->Visual->RGBAflag) {
            /* texturing only works in RGB mode */
            if (state) {
               ctx->Texture.Enabled |= TEXTURE_2D;
            }
            else {
               ctx->Texture.Enabled &= (~TEXTURE_2D);
            }
            ctx->NewState |= (NEW_RASTER_OPS | NEW_TEXTURING);
         }
	 break;
      case GL_TEXTURE_GEN_Q:
         if (state) {
            ctx->Texture.TexGenEnabled |= Q_BIT;
         }
         else {
            ctx->Texture.TexGenEnabled &= ~Q_BIT;
         }
         ctx->NewState |= NEW_TEXTURING;
	 break;
      case GL_TEXTURE_GEN_R:
         if (state) {
            ctx->Texture.TexGenEnabled |= R_BIT;
         }
         else {
            ctx->Texture.TexGenEnabled &= ~R_BIT;
         }
         ctx->NewState |= NEW_TEXTURING;
	 break;
      case GL_TEXTURE_GEN_S:
	 if (state) {
            ctx->Texture.TexGenEnabled |= S_BIT;
         }
         else {
            ctx->Texture.TexGenEnabled &= ~S_BIT;
         }
         ctx->NewState |= NEW_TEXTURING;
	 break;
      case GL_TEXTURE_GEN_T:
         if (state) {
            ctx->Texture.TexGenEnabled |= T_BIT;
         }
         else {
            ctx->Texture.TexGenEnabled &= ~T_BIT;
         }
         ctx->NewState |= NEW_TEXTURING;
	 break;

      /*
       * CLIENT STATE!!!
       */
      case GL_VERTEX_ARRAY:
         ctx->Array.VertexEnabled = state;
         break;
      case GL_NORMAL_ARRAY:
         ctx->Array.NormalEnabled = state;
         break;
      case GL_COLOR_ARRAY:
         ctx->Array.ColorEnabled = state;
         break;
      case GL_INDEX_ARRAY:
         ctx->Array.IndexEnabled = state;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         ctx->Array.TexCoordEnabled = state;
         break;
      case GL_EDGE_FLAG_ARRAY:
         ctx->Array.EdgeFlagEnabled = state;
         break;

      default:
	 if (state) {
	    gl_error( ctx, GL_INVALID_ENUM, "glEnable" );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glDisable" );
	 }
         break;
   }
}




void gl_Enable( GLcontext* ctx, GLenum cap )
{
   gl_enable( ctx, cap, GL_TRUE );
}



void gl_Disable( GLcontext* ctx, GLenum cap )
{
   gl_enable( ctx, cap, GL_FALSE );
}



GLboolean gl_IsEnabled( GLcontext* ctx, GLenum cap )
{
   switch (cap) {
      case GL_ALPHA_TEST:
         return ctx->Color.AlphaEnabled;
      case GL_AUTO_NORMAL:
	 return ctx->Eval.AutoNormal;
      case GL_BLEND:
         return ctx->Color.BlendEnabled;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
	 return ctx->Transform.ClipEnabled[cap-GL_CLIP_PLANE0];
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
      /*case GL_POLYGON_OFFSET_EXT:*/
	 return ctx->Polygon.OffsetFill;
      case GL_SCISSOR_TEST:
	 return ctx->Scissor.Enabled;
      case GL_STENCIL_TEST:
	 return ctx->Stencil.Enabled;
      case GL_TEXTURE_1D:
	 return (ctx->Texture.Enabled & TEXTURE_1D) ? GL_TRUE : GL_FALSE;
      case GL_TEXTURE_2D:
	 return (ctx->Texture.Enabled & TEXTURE_2D) ? GL_TRUE : GL_FALSE;
      case GL_TEXTURE_GEN_Q:
	 return (ctx->Texture.TexGenEnabled & Q_BIT) ? GL_TRUE : GL_FALSE;
      case GL_TEXTURE_GEN_R:
	 return (ctx->Texture.TexGenEnabled & R_BIT) ? GL_TRUE : GL_FALSE;
      case GL_TEXTURE_GEN_S:
	 return (ctx->Texture.TexGenEnabled & S_BIT) ? GL_TRUE : GL_FALSE;
      case GL_TEXTURE_GEN_T:
	 return (ctx->Texture.TexGenEnabled & T_BIT) ? GL_TRUE : GL_FALSE;

      /*
       * CLIENT STATE!!!
       */
      case GL_VERTEX_ARRAY:
         return ctx->Array.VertexEnabled;
      case GL_NORMAL_ARRAY:
         return ctx->Array.NormalEnabled;
      case GL_COLOR_ARRAY:
         return ctx->Array.ColorEnabled;
      case GL_INDEX_ARRAY:
         return ctx->Array.IndexEnabled;
      case GL_TEXTURE_COORD_ARRAY:
         return ctx->Array.TexCoordEnabled;
      case GL_EDGE_FLAG_ARRAY:
         return ctx->Array.EdgeFlagEnabled;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glIsEnabled" );
	 return GL_FALSE;
   }
}




void gl_client_state( GLcontext *ctx, GLenum cap, GLboolean state )
{
   switch (cap) {
      case GL_VERTEX_ARRAY:
         ctx->Array.VertexEnabled = state;
         break;
      case GL_NORMAL_ARRAY:
         ctx->Array.NormalEnabled = state;
         break;
      case GL_COLOR_ARRAY:
         ctx->Array.ColorEnabled = state;
         break;
      case GL_INDEX_ARRAY:
         ctx->Array.IndexEnabled = state;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         ctx->Array.TexCoordEnabled = state;
         break;
      case GL_EDGE_FLAG_ARRAY:
         ctx->Array.EdgeFlagEnabled = state;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glEnable/DisableClientState" );
   }
}



void gl_EnableClientState( GLcontext *ctx, GLenum cap )
{
   gl_client_state( ctx, cap, GL_TRUE );
}



void gl_DisableClientState( GLcontext *ctx, GLenum cap )
{
   gl_client_state( ctx, cap, GL_FALSE );
}

