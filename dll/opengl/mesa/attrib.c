/* $Id: attrib.c,v 1.9 1997/07/24 01:24:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
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
 * $Log: attrib.c,v $
 * Revision 1.9  1997/07/24 01:24:28  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.8  1997/05/28 03:23:09  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.7  1997/04/20 19:49:57  brianp
 * replaced abort() calls with gl_problem()
 *
 * Revision 1.6  1997/04/01 04:18:28  brianp
 * removed #include "draw.h"
 *
 * Revision 1.5  1997/01/28 22:13:02  brianp
 * now there's separate state for CI and RGBA logic op enabled
 *
 * Revision 1.4  1996/10/01 03:30:01  brianp
 * added #include "misc.h"
 *
 * Revision 1.3  1996/09/30 23:55:06  brianp
 * call gl_DrawBuffer() when glPopAttrib(GL_COLOR_BUFFER_BIT)
 *
 * Revision 1.2  1996/09/27 01:24:14  brianp
 * removed unused variables
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "attrib.h"
#include "context.h"
#include "dlist.h"
#include "macros.h"
#include "misc.h"
#include "types.h"
#endif


#define MALLOC_STRUCT(T)  (struct T *) malloc( sizeof(struct T) )



static struct gl_attrib_node *new_attrib_node( GLbitfield kind )
{
   struct gl_attrib_node *an;

   an = (struct gl_attrib_node *) malloc( sizeof(struct gl_attrib_node) );
   if (an) {
      an->kind = kind;
   }
   return an;
}



void gl_PushAttrib( GLcontext* ctx, GLbitfield mask )
{
   struct gl_attrib_node *newnode;
   struct gl_attrib_node *head;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPushAttrib" );
      return;
   }

   if (ctx->AttribStackDepth>=MAX_ATTRIB_STACK_DEPTH) {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_ACCUM_BUFFER_BIT) {
      struct gl_accum_attrib *attr;

      attr = MALLOC_STRUCT( gl_accum_attrib );
      MEMCPY( attr, &ctx->Accum, sizeof(struct gl_accum_attrib) );
      newnode = new_attrib_node( GL_ACCUM_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_colorbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_colorbuffer_attrib );
      MEMCPY( attr, &ctx->Color, sizeof(struct gl_colorbuffer_attrib) );
      newnode = new_attrib_node( GL_COLOR_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_CURRENT_BIT) {
      struct gl_current_attrib *attr;
      attr = MALLOC_STRUCT( gl_current_attrib );
      MEMCPY( attr, &ctx->Current, sizeof(struct gl_current_attrib) );
      newnode = new_attrib_node( GL_CURRENT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_depthbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_depthbuffer_attrib );
      MEMCPY( attr, &ctx->Depth, sizeof(struct gl_depthbuffer_attrib) );
      newnode = new_attrib_node( GL_DEPTH_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_ENABLE_BIT) {
      struct gl_enable_attrib *attr;
      GLuint i;
      attr = MALLOC_STRUCT( gl_enable_attrib );
      /* Copy enable flags from all other attributes into the enable struct. */
      attr->AlphaTest = ctx->Color.AlphaEnabled;
      attr->AutoNormal = ctx->Eval.AutoNormal;
      attr->Blend = ctx->Color.BlendEnabled;
      for (i=0;i<MAX_CLIP_PLANES;i++) {
         attr->ClipPlane[i] = ctx->Transform.ClipEnabled[i];
      }
      attr->ColorMaterial = ctx->Light.ColorMaterialEnabled;
      attr->CullFace = ctx->Polygon.CullFlag;
      attr->DepthTest = ctx->Depth.Test;
      attr->Dither = ctx->Color.DitherFlag;
      attr->Fog = ctx->Fog.Enabled;
      for (i=0;i<MAX_LIGHTS;i++) {
         attr->Light[i] = ctx->Light.Light[i].Enabled;
      }
      attr->Lighting = ctx->Light.Enabled;
      attr->LineSmooth = ctx->Line.SmoothFlag;
      attr->LineStipple = ctx->Line.StippleFlag;
      attr->IndexLogicOp = ctx->Color.IndexLogicOpEnabled;
      attr->ColorLogicOp = ctx->Color.ColorLogicOpEnabled;
      attr->Map1Color4 = ctx->Eval.Map1Color4;
      attr->Map1Index = ctx->Eval.Map1Index;
      attr->Map1Normal = ctx->Eval.Map1Normal;
      attr->Map1TextureCoord1 = ctx->Eval.Map1TextureCoord1;
      attr->Map1TextureCoord2 = ctx->Eval.Map1TextureCoord2;
      attr->Map1TextureCoord3 = ctx->Eval.Map1TextureCoord3;
      attr->Map1TextureCoord4 = ctx->Eval.Map1TextureCoord4;
      attr->Map1Vertex3 = ctx->Eval.Map1Vertex3;
      attr->Map1Vertex4 = ctx->Eval.Map1Vertex4;
      attr->Map2Color4 = ctx->Eval.Map2Color4;
      attr->Map2Index = ctx->Eval.Map2Index;
      attr->Map2Normal = ctx->Eval.Map2Normal;
      attr->Map2TextureCoord1 = ctx->Eval.Map2TextureCoord1;
      attr->Map2TextureCoord2 = ctx->Eval.Map2TextureCoord2;
      attr->Map2TextureCoord3 = ctx->Eval.Map2TextureCoord3;
      attr->Map2TextureCoord4 = ctx->Eval.Map2TextureCoord4;
      attr->Map2Vertex3 = ctx->Eval.Map2Vertex3;
      attr->Map2Vertex4 = ctx->Eval.Map2Vertex4;
      attr->Normalize = ctx->Transform.Normalize;
      attr->PointSmooth = ctx->Point.SmoothFlag;
      attr->PolygonOffsetPoint = ctx->Polygon.OffsetPoint;
      attr->PolygonOffsetLine = ctx->Polygon.OffsetLine;
      attr->PolygonOffsetFill = ctx->Polygon.OffsetFill;
      attr->PolygonSmooth = ctx->Polygon.SmoothFlag;
      attr->PolygonStipple = ctx->Polygon.StippleFlag;
      attr->Scissor = ctx->Scissor.Enabled;
      attr->Stencil = ctx->Stencil.Enabled;
      attr->Texture = ctx->Texture.Enabled;
      attr->TexGen = ctx->Texture.TexGenEnabled;
      newnode = new_attrib_node( GL_ENABLE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_EVAL_BIT) {
      struct gl_eval_attrib *attr;
      attr = MALLOC_STRUCT( gl_eval_attrib );
      MEMCPY( attr, &ctx->Eval, sizeof(struct gl_eval_attrib) );
      newnode = new_attrib_node( GL_EVAL_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_FOG_BIT) {
      struct gl_fog_attrib *attr;
      attr = MALLOC_STRUCT( gl_fog_attrib );
      MEMCPY( attr, &ctx->Fog, sizeof(struct gl_fog_attrib) );
      newnode = new_attrib_node( GL_FOG_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_HINT_BIT) {
      struct gl_hint_attrib *attr;
      attr = MALLOC_STRUCT( gl_hint_attrib );
      MEMCPY( attr, &ctx->Hint, sizeof(struct gl_hint_attrib) );
      newnode = new_attrib_node( GL_HINT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LIGHTING_BIT) {
      struct gl_light_attrib *attr;
      attr = MALLOC_STRUCT( gl_light_attrib );
      MEMCPY( attr, &ctx->Light, sizeof(struct gl_light_attrib) );
      newnode = new_attrib_node( GL_LIGHTING_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LINE_BIT) {
      struct gl_line_attrib *attr;
      attr = MALLOC_STRUCT( gl_line_attrib );
      MEMCPY( attr, &ctx->Line, sizeof(struct gl_line_attrib) );
      newnode = new_attrib_node( GL_LINE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LIST_BIT) {
      struct gl_list_attrib *attr;
      attr = MALLOC_STRUCT( gl_list_attrib );
      MEMCPY( attr, &ctx->List, sizeof(struct gl_list_attrib) );
      newnode = new_attrib_node( GL_LIST_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_PIXEL_MODE_BIT) {
      struct gl_pixel_attrib *attr;
      attr = MALLOC_STRUCT( gl_pixel_attrib );
      MEMCPY( attr, &ctx->Pixel, sizeof(struct gl_pixel_attrib) );
      newnode = new_attrib_node( GL_PIXEL_MODE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POINT_BIT) {
      struct gl_point_attrib *attr;
      attr = MALLOC_STRUCT( gl_point_attrib );
      MEMCPY( attr, &ctx->Point, sizeof(struct gl_point_attrib) );
      newnode = new_attrib_node( GL_POINT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POLYGON_BIT) {
      struct gl_polygon_attrib *attr;
      attr = MALLOC_STRUCT( gl_polygon_attrib );
      MEMCPY( attr, &ctx->Polygon, sizeof(struct gl_polygon_attrib) );
      newnode = new_attrib_node( GL_POLYGON_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POLYGON_STIPPLE_BIT) {
      GLuint *stipple;
      stipple = (GLuint *) malloc( 32*sizeof(GLuint) );
      MEMCPY( stipple, ctx->PolygonStipple, 32*sizeof(GLuint) );
      newnode = new_attrib_node( GL_POLYGON_STIPPLE_BIT );
      newnode->data = stipple;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_SCISSOR_BIT) {
      struct gl_scissor_attrib *attr;
      attr = MALLOC_STRUCT( gl_scissor_attrib );
      MEMCPY( attr, &ctx->Scissor, sizeof(struct gl_scissor_attrib) );
      newnode = new_attrib_node( GL_SCISSOR_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_stencil_attrib *attr;
      attr = MALLOC_STRUCT( gl_stencil_attrib );
      MEMCPY( attr, &ctx->Stencil, sizeof(struct gl_stencil_attrib) );
      newnode = new_attrib_node( GL_STENCIL_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_TEXTURE_BIT) {
      struct gl_texture_attrib *attr;
      attr = MALLOC_STRUCT( gl_texture_attrib );
      MEMCPY( attr, &ctx->Texture, sizeof(struct gl_texture_attrib) );
      newnode = new_attrib_node( GL_TEXTURE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_TRANSFORM_BIT) {
      struct gl_transform_attrib *attr;
      attr = MALLOC_STRUCT( gl_transform_attrib );
      MEMCPY( attr, &ctx->Transform, sizeof(struct gl_transform_attrib) );
      newnode = new_attrib_node( GL_TRANSFORM_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_VIEWPORT_BIT) {
      struct gl_viewport_attrib *attr;
      attr = MALLOC_STRUCT( gl_viewport_attrib );
      MEMCPY( attr, &ctx->Viewport, sizeof(struct gl_viewport_attrib) );
      newnode = new_attrib_node( GL_VIEWPORT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   /* etc... */

   ctx->AttribStack[ctx->AttribStackDepth] = head;
   ctx->AttribStackDepth++;
}



void gl_PopAttrib( GLcontext* ctx )
{
   struct gl_attrib_node *attr, *next;
   struct gl_enable_attrib *enable;
   GLuint i, oldDrawBuffer;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPopAttrib" );
      return;
   }

   if (ctx->AttribStackDepth==0) {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopAttrib" );
      return;
   }

   ctx->AttribStackDepth--;
   attr = ctx->AttribStack[ctx->AttribStackDepth];

   while (attr) {
      switch (attr->kind) {
         case GL_ACCUM_BUFFER_BIT:
            MEMCPY( &ctx->Accum, attr->data, sizeof(struct gl_accum_attrib) );
            break;
         case GL_COLOR_BUFFER_BIT:
	    oldDrawBuffer = ctx->Color.DrawBuffer;
            MEMCPY( &ctx->Color, attr->data,
		    sizeof(struct gl_colorbuffer_attrib) );
	    if (ctx->Color.DrawBuffer != oldDrawBuffer) {
	       gl_DrawBuffer(ctx, ctx->Color.DrawBuffer);
            }
            break;
         case GL_CURRENT_BIT:
            MEMCPY( &ctx->Current, attr->data,
		    sizeof(struct gl_current_attrib) );
            break;
         case GL_DEPTH_BUFFER_BIT:
            MEMCPY( &ctx->Depth, attr->data,
		    sizeof(struct gl_depthbuffer_attrib) );
            break;
         case GL_ENABLE_BIT:
            enable = (struct gl_enable_attrib *) attr->data;
            ctx->Color.AlphaEnabled = enable->AlphaTest;
            ctx->Transform.Normalize = enable->AutoNormal;
            ctx->Color.BlendEnabled = enable->Blend;
	    for (i=0;i<MAX_CLIP_PLANES;i++) {
               ctx->Transform.ClipEnabled[i] = enable->ClipPlane[i];
	    }
	    ctx->Light.ColorMaterialEnabled = enable->ColorMaterial;
	    ctx->Polygon.CullFlag = enable->CullFace;
	    ctx->Depth.Test = enable->DepthTest;
	    ctx->Color.DitherFlag = enable->Dither;
	    ctx->Fog.Enabled = enable->Fog;
	    ctx->Light.Enabled = enable->Lighting;
	    ctx->Line.SmoothFlag = enable->LineSmooth;
	    ctx->Line.StippleFlag = enable->LineStipple;
	    ctx->Color.IndexLogicOpEnabled = enable->IndexLogicOp;
	    ctx->Color.ColorLogicOpEnabled = enable->ColorLogicOp;
	    ctx->Eval.Map1Color4 = enable->Map1Color4;
	    ctx->Eval.Map1Index = enable->Map1Index;
	    ctx->Eval.Map1Normal = enable->Map1Normal;
	    ctx->Eval.Map1TextureCoord1 = enable->Map1TextureCoord1;
	    ctx->Eval.Map1TextureCoord2 = enable->Map1TextureCoord2;
	    ctx->Eval.Map1TextureCoord3 = enable->Map1TextureCoord3;
	    ctx->Eval.Map1TextureCoord4 = enable->Map1TextureCoord4;
	    ctx->Eval.Map1Vertex3 = enable->Map1Vertex3;
	    ctx->Eval.Map1Vertex4 = enable->Map1Vertex4;
	    ctx->Eval.Map2Color4 = enable->Map2Color4;
	    ctx->Eval.Map2Index = enable->Map2Index;
	    ctx->Eval.Map2Normal = enable->Map2Normal;
	    ctx->Eval.Map2TextureCoord1 = enable->Map2TextureCoord1;
	    ctx->Eval.Map2TextureCoord2 = enable->Map2TextureCoord2;
	    ctx->Eval.Map2TextureCoord3 = enable->Map2TextureCoord3;
	    ctx->Eval.Map2TextureCoord4 = enable->Map2TextureCoord4;
	    ctx->Eval.Map2Vertex3 = enable->Map2Vertex3;
	    ctx->Eval.Map2Vertex4 = enable->Map2Vertex4;
	    ctx->Transform.Normalize = enable->Normalize;
	    ctx->Point.SmoothFlag = enable->PointSmooth;
	    ctx->Polygon.OffsetPoint = enable->PolygonOffsetPoint;
	    ctx->Polygon.OffsetLine = enable->PolygonOffsetLine;
	    ctx->Polygon.OffsetFill = enable->PolygonOffsetFill;
            ctx->Polygon.OffsetAny = ctx->Polygon.OffsetPoint ||
                                     ctx->Polygon.OffsetLine ||
                                     ctx->Polygon.OffsetFill;
	    ctx->Polygon.SmoothFlag = enable->PolygonSmooth;
	    ctx->Polygon.StippleFlag = enable->PolygonStipple;
	    ctx->Scissor.Enabled = enable->Scissor;
	    ctx->Stencil.Enabled = enable->Stencil;
	    ctx->Texture.Enabled = enable->Texture;
	    ctx->Texture.TexGenEnabled = enable->TexGen;
            break;
         case GL_EVAL_BIT:
            MEMCPY( &ctx->Eval, attr->data, sizeof(struct gl_eval_attrib) );
            break;
         case GL_FOG_BIT:
            MEMCPY( &ctx->Fog, attr->data, sizeof(struct gl_fog_attrib) );
            break;
         case GL_HINT_BIT:
            MEMCPY( &ctx->Hint, attr->data, sizeof(struct gl_hint_attrib) );
            break;
         case GL_LIGHTING_BIT:
            MEMCPY( &ctx->Light, attr->data, sizeof(struct gl_light_attrib) );
            break;
         case GL_LINE_BIT:
            MEMCPY( &ctx->Line, attr->data, sizeof(struct gl_line_attrib) );
            break;
         case GL_LIST_BIT:
            MEMCPY( &ctx->List, attr->data, sizeof(struct gl_list_attrib) );
            break;
         case GL_PIXEL_MODE_BIT:
            MEMCPY( &ctx->Pixel, attr->data, sizeof(struct gl_pixel_attrib) );
            break;
         case GL_POINT_BIT:
            MEMCPY( &ctx->Point, attr->data, sizeof(struct gl_point_attrib) );
            break;
         case GL_POLYGON_BIT:
            MEMCPY( &ctx->Polygon, attr->data,
		    sizeof(struct gl_polygon_attrib) );
            break;
	 case GL_POLYGON_STIPPLE_BIT:
	    MEMCPY( ctx->PolygonStipple, attr->data, 32*sizeof(GLuint) );
	    break;
         case GL_SCISSOR_BIT:
            MEMCPY( &ctx->Scissor, attr->data,
		    sizeof(struct gl_scissor_attrib) );
            break;
         case GL_STENCIL_BUFFER_BIT:
            MEMCPY( &ctx->Stencil, attr->data,
		    sizeof(struct gl_stencil_attrib) );
            break;
         case GL_TRANSFORM_BIT:
            MEMCPY( &ctx->Transform, attr->data,
		    sizeof(struct gl_transform_attrib) );
            break;
         case GL_TEXTURE_BIT:
            MEMCPY( &ctx->Texture, attr->data,
		    sizeof(struct gl_texture_attrib) );
            break;
         case GL_VIEWPORT_BIT:
            MEMCPY( &ctx->Viewport, attr->data,
		    sizeof(struct gl_viewport_attrib) );
            break;
         default:
            gl_problem( ctx, "Bad attrib flag in PopAttrib");
            break;
      }

      next = attr->next;
      free( (void *) attr->data );
      free( (void *) attr );
      attr = next;
   }

   ctx->NewState = NEW_ALL;
}


#define GL_CLIENT_PACK_BIT (1<<20)
#define GL_CLIENT_UNPACK_BIT (1<<21)


void gl_PushClientAttrib( GLcontext *ctx, GLbitfield mask )
{
   struct gl_attrib_node *newnode;
   struct gl_attrib_node *head;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPushClientAttrib" );
      return;
   }

   if (ctx->ClientAttribStackDepth>=MAX_CLIENT_ATTRIB_STACK_DEPTH) {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushClientAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      struct gl_pixelstore_attrib *attr;
      /* packing attribs */
      attr = MALLOC_STRUCT( gl_pixelstore_attrib );
      MEMCPY( attr, &ctx->Pack, sizeof(struct gl_pixelstore_attrib) );
      newnode = new_attrib_node( GL_CLIENT_PACK_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
      /* unpacking attribs */
      attr = MALLOC_STRUCT( gl_pixelstore_attrib );
      MEMCPY( attr, &ctx->Unpack, sizeof(struct gl_pixelstore_attrib) );
      newnode = new_attrib_node( GL_CLIENT_UNPACK_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }
   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      struct gl_array_attrib *attr;
      attr = MALLOC_STRUCT( gl_array_attrib );
      MEMCPY( attr, &ctx->Array, sizeof(struct gl_array_attrib) );
      newnode = new_attrib_node( GL_CLIENT_VERTEX_ARRAY_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   /* etc... */

   ctx->ClientAttribStack[ctx->ClientAttribStackDepth] = head;
   ctx->ClientAttribStackDepth++;
}




void gl_PopClientAttrib( GLcontext *ctx )
{
   struct gl_attrib_node *attr, *next;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPopClientAttrib" );
      return;
   }

   if (ctx->ClientAttribStackDepth==0) {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopClientAttrib" );
      return;
   }

   ctx->ClientAttribStackDepth--;
   attr = ctx->ClientAttribStack[ctx->ClientAttribStackDepth];

   while (attr) {
      switch (attr->kind) {
         case GL_CLIENT_PACK_BIT:
            MEMCPY( &ctx->Pack, attr->data,
                    sizeof(struct gl_pixelstore_attrib) );
            break;
         case GL_CLIENT_UNPACK_BIT:
            MEMCPY( &ctx->Unpack, attr->data,
                    sizeof(struct gl_pixelstore_attrib) );
            break;
         case GL_CLIENT_VERTEX_ARRAY_BIT:
            MEMCPY( &ctx->Array, attr->data,
		    sizeof(struct gl_array_attrib) );
            break;
         default:
            gl_problem( ctx, "Bad attrib flag in PopClientAttrib");
            break;
      }

      next = attr->next;
      free( (void *) attr->data );
      free( (void *) attr );
      attr = next;
   }

   ctx->NewState = NEW_ALL;
}

