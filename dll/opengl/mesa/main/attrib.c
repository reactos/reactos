/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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
 * glEnable()/glDisable() attribute group (GL_ENABLE_BIT).
 */
struct gl_enable_attrib
{
   GLboolean AlphaTest;
   GLboolean AutoNormal;
   GLboolean Blend;
   GLbitfield ClipPlanes;
   GLboolean ColorMaterial;
   GLboolean CullFace;
   GLboolean DepthTest;
   GLboolean Dither;
   GLboolean Fog;
   GLboolean Light[MAX_LIGHTS];
   GLboolean Lighting;
   GLboolean LineSmooth;
   GLboolean LineStipple;
   GLboolean IndexLogicOp;
   GLboolean ColorLogicOp;

   GLboolean Map1Color4;
   GLboolean Map1Index;
   GLboolean Map1Normal;
   GLboolean Map1TextureCoord1;
   GLboolean Map1TextureCoord2;
   GLboolean Map1TextureCoord3;
   GLboolean Map1TextureCoord4;
   GLboolean Map1Vertex3;
   GLboolean Map1Vertex4;
   GLboolean Map2Color4;
   GLboolean Map2Index;
   GLboolean Map2Normal;
   GLboolean Map2TextureCoord1;
   GLboolean Map2TextureCoord2;
   GLboolean Map2TextureCoord3;
   GLboolean Map2TextureCoord4;
   GLboolean Map2Vertex3;
   GLboolean Map2Vertex4;

   GLboolean Normalize;
   GLboolean PixelTexture;
   GLboolean PointSmooth;
   GLboolean PolygonOffsetPoint;
   GLboolean PolygonOffsetLine;
   GLboolean PolygonOffsetFill;
   GLboolean PolygonSmooth;
   GLboolean PolygonStipple;
   GLboolean RescaleNormals;
   GLboolean Scissor;
   GLboolean Stencil;
   GLboolean MultisampleEnabled;      /* GL_ARB_multisample */
   GLboolean SampleAlphaToCoverage;   /* GL_ARB_multisample */
   GLboolean SampleAlphaToOne;        /* GL_ARB_multisample */
   GLboolean SampleCoverage;          /* GL_ARB_multisample */
   GLboolean RasterPositionUnclipped; /* GL_IBM_rasterpos_clip */

   GLbitfield Texture;
   GLbitfield TexGen;

   /* GL_ARB_point_sprite / GL_NV_point_sprite */
   GLboolean PointSprite;
};


/**
 * Node for the attribute stack.
 */
struct gl_attrib_node
{
   GLbitfield kind;
   void *data;
   struct gl_attrib_node *next;
};



/**
 * Special struct for saving/restoring texture state (GL_TEXTURE_BIT)
 */
struct texture_state
{
   struct gl_texture_attrib Texture;  /**< The usual context state */

   /** to save per texture object state (wrap modes, filters, etc): */
   struct gl_texture_object SavedObj[NUM_TEXTURE_TARGETS];

   /**
    * To save references to texture objects (so they don't get accidentally
    * deleted while saved in the attribute stack).
    */
   struct gl_texture_object *SavedTexRef[NUM_TEXTURE_TARGETS];

   /* We need to keep a reference to the shared state.  That's where the
    * default texture objects are kept.  We don't want that state to be
    * freed while the attribute stack contains pointers to any default
    * texture objects.
    */
   struct gl_shared_state *SharedRef;
};


#if FEATURE_attrib_stack


/**
 * Allocate new attribute node of given type/kind.  Attach payload data.
 * Insert it into the linked list named by 'head'.
 */
static void
save_attrib_data(struct gl_attrib_node **head,
                 GLbitfield kind, void *payload)
{
   struct gl_attrib_node *n = MALLOC_STRUCT(gl_attrib_node);
   if (n) {
      n->kind = kind;
      n->data = payload;
      /* insert at head */
      n->next = *head;
      *head = n;
   }
   else {
      /* out of memory! */
   }
}


void GLAPIENTRY
_mesa_PushAttrib(GLbitfield mask)
{
   struct gl_attrib_node *head;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glPushAttrib %x\n", (int) mask);

   if (ctx->AttribStackDepth >= MAX_ATTRIB_STACK_DEPTH) {
      _mesa_error( ctx, GL_STACK_OVERFLOW, "glPushAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_ACCUM_BUFFER_BIT) {
      struct gl_accum_attrib *attr;
      attr = MALLOC_STRUCT( gl_accum_attrib );
      memcpy( attr, &ctx->Accum, sizeof(struct gl_accum_attrib) );
      save_attrib_data(&head, GL_ACCUM_BUFFER_BIT, attr);
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_colorbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_colorbuffer_attrib );
      memcpy( attr, &ctx->Color, sizeof(struct gl_colorbuffer_attrib) );
      /* push the Draw FBO's DrawBuffer[] state, not ctx->Color.DrawBuffer[] */
      attr->DrawBuffer = ctx->DrawBuffer->ColorDrawBuffer;
      save_attrib_data(&head, GL_COLOR_BUFFER_BIT, attr);
   }

   if (mask & GL_CURRENT_BIT) {
      struct gl_current_attrib *attr;
      FLUSH_CURRENT( ctx, 0 );
      attr = MALLOC_STRUCT( gl_current_attrib );
      memcpy( attr, &ctx->Current, sizeof(struct gl_current_attrib) );
      save_attrib_data(&head, GL_CURRENT_BIT, attr);
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_depthbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_depthbuffer_attrib );
      memcpy( attr, &ctx->Depth, sizeof(struct gl_depthbuffer_attrib) );
      save_attrib_data(&head, GL_DEPTH_BUFFER_BIT, attr);
   }

   if (mask & GL_ENABLE_BIT) {
      struct gl_enable_attrib *attr;
      GLuint i;
      attr = MALLOC_STRUCT( gl_enable_attrib );
      /* Copy enable flags from all other attributes into the enable struct. */
      attr->AlphaTest = ctx->Color.AlphaEnabled;
      attr->AutoNormal = ctx->Eval.AutoNormal;
      attr->Blend = ctx->Color.BlendEnabled;
      attr->ClipPlanes = ctx->Transform.ClipPlanesEnabled;
      attr->ColorMaterial = ctx->Light.ColorMaterialEnabled;
      attr->CullFace = ctx->Polygon.CullFlag;
      attr->DepthTest = ctx->Depth.Test;
      attr->Dither = ctx->Color.DitherFlag;
      attr->Fog = ctx->Fog.Enabled;
      for (i = 0; i < ctx->Const.MaxLights; i++) {
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
      attr->RasterPositionUnclipped = ctx->Transform.RasterPositionUnclipped;
      attr->PointSmooth = ctx->Point.SmoothFlag;
      attr->PointSprite = ctx->Point.PointSprite;
      attr->PolygonOffsetPoint = ctx->Polygon.OffsetPoint;
      attr->PolygonOffsetLine = ctx->Polygon.OffsetLine;
      attr->PolygonOffsetFill = ctx->Polygon.OffsetFill;
      attr->PolygonSmooth = ctx->Polygon.SmoothFlag;
      attr->PolygonStipple = ctx->Polygon.StippleFlag;
      attr->RescaleNormals = ctx->Transform.RescaleNormals;
      attr->Scissor = ctx->Scissor.Enabled;
      attr->Stencil = ctx->Stencil.Enabled;
      attr->MultisampleEnabled = ctx->Multisample.Enabled;
      attr->SampleAlphaToCoverage = ctx->Multisample.SampleAlphaToCoverage;
      attr->SampleAlphaToOne = ctx->Multisample.SampleAlphaToOne;
      attr->SampleCoverage = ctx->Multisample.SampleCoverage;
      attr->Texture = ctx->Texture.Unit.Enabled;
      attr->TexGen = ctx->Texture.Unit.TexGenEnabled;
      save_attrib_data(&head, GL_ENABLE_BIT, attr);
   }

   if (mask & GL_EVAL_BIT) {
      struct gl_eval_attrib *attr;
      attr = MALLOC_STRUCT( gl_eval_attrib );
      memcpy( attr, &ctx->Eval, sizeof(struct gl_eval_attrib) );
      save_attrib_data(&head, GL_EVAL_BIT, attr);
   }

   if (mask & GL_FOG_BIT) {
      struct gl_fog_attrib *attr;
      attr = MALLOC_STRUCT( gl_fog_attrib );
      memcpy( attr, &ctx->Fog, sizeof(struct gl_fog_attrib) );
      save_attrib_data(&head, GL_FOG_BIT, attr);
   }

   if (mask & GL_HINT_BIT) {
      struct gl_hint_attrib *attr;
      attr = MALLOC_STRUCT( gl_hint_attrib );
      memcpy( attr, &ctx->Hint, sizeof(struct gl_hint_attrib) );
      save_attrib_data(&head, GL_HINT_BIT, attr);
   }

   if (mask & GL_LIGHTING_BIT) {
      struct gl_light_attrib *attr;
      FLUSH_CURRENT(ctx, 0);	/* flush material changes */
      attr = MALLOC_STRUCT( gl_light_attrib );
      memcpy( attr, &ctx->Light, sizeof(struct gl_light_attrib) );
      save_attrib_data(&head, GL_LIGHTING_BIT, attr);
   }

   if (mask & GL_LINE_BIT) {
      struct gl_line_attrib *attr;
      attr = MALLOC_STRUCT( gl_line_attrib );
      memcpy( attr, &ctx->Line, sizeof(struct gl_line_attrib) );
      save_attrib_data(&head, GL_LINE_BIT, attr);
   }

   if (mask & GL_LIST_BIT) {
      struct gl_list_attrib *attr;
      attr = MALLOC_STRUCT( gl_list_attrib );
      memcpy( attr, &ctx->List, sizeof(struct gl_list_attrib) );
      save_attrib_data(&head, GL_LIST_BIT, attr);
   }

   if (mask & GL_PIXEL_MODE_BIT) {
      struct gl_pixel_attrib *attr;
      attr = MALLOC_STRUCT( gl_pixel_attrib );
      memcpy( attr, &ctx->Pixel, sizeof(struct gl_pixel_attrib) );
      /* push the Read FBO's ReadBuffer state, not ctx->Pixel.ReadBuffer */
      attr->ReadBuffer = ctx->ReadBuffer->ColorReadBuffer;
      save_attrib_data(&head, GL_PIXEL_MODE_BIT, attr);
   }

   if (mask & GL_POINT_BIT) {
      struct gl_point_attrib *attr;
      attr = MALLOC_STRUCT( gl_point_attrib );
      memcpy( attr, &ctx->Point, sizeof(struct gl_point_attrib) );
      save_attrib_data(&head, GL_POINT_BIT, attr);
   }

   if (mask & GL_POLYGON_BIT) {
      struct gl_polygon_attrib *attr;
      attr = MALLOC_STRUCT( gl_polygon_attrib );
      memcpy( attr, &ctx->Polygon, sizeof(struct gl_polygon_attrib) );
      save_attrib_data(&head, GL_POLYGON_BIT, attr);
   }

   if (mask & GL_POLYGON_STIPPLE_BIT) {
      GLuint *stipple;
      stipple = (GLuint *) MALLOC( 32*sizeof(GLuint) );
      memcpy( stipple, ctx->PolygonStipple, 32*sizeof(GLuint) );
      save_attrib_data(&head, GL_POLYGON_STIPPLE_BIT, stipple);
   }

   if (mask & GL_SCISSOR_BIT) {
      struct gl_scissor_attrib *attr;
      attr = MALLOC_STRUCT( gl_scissor_attrib );
      memcpy( attr, &ctx->Scissor, sizeof(struct gl_scissor_attrib) );
      save_attrib_data(&head, GL_SCISSOR_BIT, attr);
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_stencil_attrib *attr;
      attr = MALLOC_STRUCT( gl_stencil_attrib );
      memcpy( attr, &ctx->Stencil, sizeof(struct gl_stencil_attrib) );
      save_attrib_data(&head, GL_STENCIL_BUFFER_BIT, attr);
   }

   if (mask & GL_TEXTURE_BIT) {
      struct texture_state *texstate = CALLOC_STRUCT(texture_state);
      GLuint tex;

      if (!texstate) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glPushAttrib(GL_TEXTURE_BIT)");
         goto end;
      }

      _mesa_lock_context_textures(ctx);

      /* copy/save the bulk of texture state here */
      memcpy(&texstate->Texture, &ctx->Texture, sizeof(ctx->Texture));

      /* Save references to the currently bound texture objects so they don't
       * accidentally get deleted while referenced in the attribute stack.
       */
      for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
         _mesa_reference_texobj(&texstate->SavedTexRef[tex],
                                ctx->Texture.Unit.CurrentTex[tex]);
      }

      /* copy state/contents of the currently bound texture objects */
      for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
         _mesa_copy_texture_object(&texstate->SavedObj[tex],
                                   ctx->Texture.Unit.CurrentTex[tex]);
      }

      _mesa_reference_shared_state(ctx, &texstate->SharedRef, ctx->Shared);

      _mesa_unlock_context_textures(ctx);

      save_attrib_data(&head, GL_TEXTURE_BIT, texstate);
   }

   if (mask & GL_TRANSFORM_BIT) {
      struct gl_transform_attrib *attr;
      attr = MALLOC_STRUCT( gl_transform_attrib );
      memcpy( attr, &ctx->Transform, sizeof(struct gl_transform_attrib) );
      save_attrib_data(&head, GL_TRANSFORM_BIT, attr);
   }

   if (mask & GL_VIEWPORT_BIT) {
      struct gl_viewport_attrib *attr;
      attr = MALLOC_STRUCT( gl_viewport_attrib );
      memcpy( attr, &ctx->Viewport, sizeof(struct gl_viewport_attrib) );
      save_attrib_data(&head, GL_VIEWPORT_BIT, attr);
   }

   /* GL_ARB_multisample */
   if (mask & GL_MULTISAMPLE_BIT_ARB) {
      struct gl_multisample_attrib *attr;
      attr = MALLOC_STRUCT( gl_multisample_attrib );
      memcpy( attr, &ctx->Multisample, sizeof(struct gl_multisample_attrib) );
      save_attrib_data(&head, GL_MULTISAMPLE_BIT_ARB, attr);
   }

end:
   ctx->AttribStack[ctx->AttribStackDepth] = head;
   ctx->AttribStackDepth++;
}



static void
pop_enable_group(struct gl_context *ctx, const struct gl_enable_attrib *enable)
{
   GLuint i;

#define TEST_AND_UPDATE(VALUE, NEWVALUE, ENUM)		\
	if ((VALUE) != (NEWVALUE)) {			\
	   _mesa_set_enable( ctx, ENUM, (NEWVALUE) );	\
	}

   TEST_AND_UPDATE(ctx->Color.AlphaEnabled, enable->AlphaTest, GL_ALPHA_TEST);
   if (ctx->Color.BlendEnabled != enable->Blend) {
      _mesa_set_enable(ctx, GL_BLEND, (enable->Blend & 1));
   }

   for (i=0;i<ctx->Const.MaxClipPlanes;i++) {
      const GLuint mask = 1 << i;
      if ((ctx->Transform.ClipPlanesEnabled & mask) != (enable->ClipPlanes & mask))
	  _mesa_set_enable(ctx, (GLenum) (GL_CLIP_PLANE0 + i),
			   !!(enable->ClipPlanes & mask));
   }

   TEST_AND_UPDATE(ctx->Light.ColorMaterialEnabled, enable->ColorMaterial,
                   GL_COLOR_MATERIAL);
   TEST_AND_UPDATE(ctx->Polygon.CullFlag, enable->CullFace, GL_CULL_FACE);
   TEST_AND_UPDATE(ctx->Depth.Test, enable->DepthTest, GL_DEPTH_TEST);
   TEST_AND_UPDATE(ctx->Color.DitherFlag, enable->Dither, GL_DITHER);
   TEST_AND_UPDATE(ctx->Fog.Enabled, enable->Fog, GL_FOG);
   TEST_AND_UPDATE(ctx->Light.Enabled, enable->Lighting, GL_LIGHTING);
   TEST_AND_UPDATE(ctx->Line.SmoothFlag, enable->LineSmooth, GL_LINE_SMOOTH);
   TEST_AND_UPDATE(ctx->Line.StippleFlag, enable->LineStipple,
                   GL_LINE_STIPPLE);
   TEST_AND_UPDATE(ctx->Color.IndexLogicOpEnabled, enable->IndexLogicOp,
                   GL_INDEX_LOGIC_OP);
   TEST_AND_UPDATE(ctx->Color.ColorLogicOpEnabled, enable->ColorLogicOp,
                   GL_COLOR_LOGIC_OP);

   TEST_AND_UPDATE(ctx->Eval.Map1Color4, enable->Map1Color4, GL_MAP1_COLOR_4);
   TEST_AND_UPDATE(ctx->Eval.Map1Index, enable->Map1Index, GL_MAP1_INDEX);
   TEST_AND_UPDATE(ctx->Eval.Map1Normal, enable->Map1Normal, GL_MAP1_NORMAL);
   TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord1, enable->Map1TextureCoord1,
                   GL_MAP1_TEXTURE_COORD_1);
   TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord2, enable->Map1TextureCoord2,
                   GL_MAP1_TEXTURE_COORD_2);
   TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord3, enable->Map1TextureCoord3,
                   GL_MAP1_TEXTURE_COORD_3);
   TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord4, enable->Map1TextureCoord4,
                   GL_MAP1_TEXTURE_COORD_4);
   TEST_AND_UPDATE(ctx->Eval.Map1Vertex3, enable->Map1Vertex3,
                   GL_MAP1_VERTEX_3);
   TEST_AND_UPDATE(ctx->Eval.Map1Vertex4, enable->Map1Vertex4,
                   GL_MAP1_VERTEX_4);

   TEST_AND_UPDATE(ctx->Eval.Map2Color4, enable->Map2Color4, GL_MAP2_COLOR_4);
   TEST_AND_UPDATE(ctx->Eval.Map2Index, enable->Map2Index, GL_MAP2_INDEX);
   TEST_AND_UPDATE(ctx->Eval.Map2Normal, enable->Map2Normal, GL_MAP2_NORMAL);
   TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord1, enable->Map2TextureCoord1,
                   GL_MAP2_TEXTURE_COORD_1);
   TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord2, enable->Map2TextureCoord2,
                   GL_MAP2_TEXTURE_COORD_2);
   TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord3, enable->Map2TextureCoord3,
                   GL_MAP2_TEXTURE_COORD_3);
   TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord4, enable->Map2TextureCoord4,
                   GL_MAP2_TEXTURE_COORD_4);
   TEST_AND_UPDATE(ctx->Eval.Map2Vertex3, enable->Map2Vertex3,
                   GL_MAP2_VERTEX_3);
   TEST_AND_UPDATE(ctx->Eval.Map2Vertex4, enable->Map2Vertex4,
                   GL_MAP2_VERTEX_4);

   TEST_AND_UPDATE(ctx->Eval.AutoNormal, enable->AutoNormal, GL_AUTO_NORMAL);
   TEST_AND_UPDATE(ctx->Transform.Normalize, enable->Normalize, GL_NORMALIZE);
   TEST_AND_UPDATE(ctx->Transform.RescaleNormals, enable->RescaleNormals,
                   GL_RESCALE_NORMAL_EXT);
   TEST_AND_UPDATE(ctx->Transform.RasterPositionUnclipped,
                   enable->RasterPositionUnclipped,
                   GL_RASTER_POSITION_UNCLIPPED_IBM);
   TEST_AND_UPDATE(ctx->Point.SmoothFlag, enable->PointSmooth,
                   GL_POINT_SMOOTH);
   if (ctx->Extensions.NV_point_sprite || ctx->Extensions.ARB_point_sprite) {
      TEST_AND_UPDATE(ctx->Point.PointSprite, enable->PointSprite,
                      GL_POINT_SPRITE_NV);
   }
   TEST_AND_UPDATE(ctx->Polygon.OffsetPoint, enable->PolygonOffsetPoint,
                   GL_POLYGON_OFFSET_POINT);
   TEST_AND_UPDATE(ctx->Polygon.OffsetLine, enable->PolygonOffsetLine,
                   GL_POLYGON_OFFSET_LINE);
   TEST_AND_UPDATE(ctx->Polygon.OffsetFill, enable->PolygonOffsetFill,
                   GL_POLYGON_OFFSET_FILL);
   TEST_AND_UPDATE(ctx->Polygon.SmoothFlag, enable->PolygonSmooth,
                   GL_POLYGON_SMOOTH);
   TEST_AND_UPDATE(ctx->Polygon.StippleFlag, enable->PolygonStipple,
                   GL_POLYGON_STIPPLE);
   TEST_AND_UPDATE(ctx->Scissor.Enabled, enable->Scissor, GL_SCISSOR_TEST);
   TEST_AND_UPDATE(ctx->Stencil.Enabled, enable->Stencil, GL_STENCIL_TEST);
   TEST_AND_UPDATE(ctx->Multisample.Enabled, enable->MultisampleEnabled,
                   GL_MULTISAMPLE_ARB);
   TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToCoverage,
                   enable->SampleAlphaToCoverage,
                   GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
   TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToOne,
                   enable->SampleAlphaToOne,
                   GL_SAMPLE_ALPHA_TO_ONE_ARB);
   TEST_AND_UPDATE(ctx->Multisample.SampleCoverage,
                   enable->SampleCoverage,
                   GL_SAMPLE_COVERAGE_ARB);

   /* texture unit enables */
   {
      const GLbitfield enabled = enable->Texture;
      const GLbitfield genEnabled = enable->TexGen;

      if (ctx->Texture.Unit.Enabled != enabled) {
         _mesa_set_enable(ctx, GL_TEXTURE_1D, !!(enabled & TEXTURE_1D_BIT));
         _mesa_set_enable(ctx, GL_TEXTURE_2D, !!(enabled & TEXTURE_2D_BIT));
         if (ctx->Extensions.ARB_texture_cube_map) {
            _mesa_set_enable(ctx, GL_TEXTURE_CUBE_MAP,
                             !!(enabled & TEXTURE_CUBE_BIT));
         }
      }

      if (ctx->Texture.Unit.TexGenEnabled != genEnabled) {
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_S, !!(genEnabled & S_BIT));
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_T, !!(genEnabled & T_BIT));
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_R, !!(genEnabled & R_BIT));
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_Q, !!(genEnabled & Q_BIT));
      }
   }
}


/**
 * Pop/restore texture attribute/group state.
 */
static void
pop_texture_group(struct gl_context *ctx, struct texture_state *texstate)
{
   const struct gl_texture_unit *unit;
   GLuint tgt;

   _mesa_lock_context_textures(ctx);

   unit = &texstate->Texture.Unit;

   _mesa_set_enable(ctx, GL_TEXTURE_1D, !!(unit->Enabled & TEXTURE_1D_BIT));
   _mesa_set_enable(ctx, GL_TEXTURE_2D, !!(unit->Enabled & TEXTURE_2D_BIT));

   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, unit->EnvMode);
   _mesa_TexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, unit->EnvColor);
   _mesa_TexGeni(GL_S, GL_TEXTURE_GEN_MODE, unit->GenS.Mode);
   _mesa_TexGeni(GL_T, GL_TEXTURE_GEN_MODE, unit->GenT.Mode);
   _mesa_TexGeni(GL_R, GL_TEXTURE_GEN_MODE, unit->GenR.Mode);
   _mesa_TexGeni(GL_Q, GL_TEXTURE_GEN_MODE, unit->GenQ.Mode);
   _mesa_TexGenfv(GL_S, GL_OBJECT_PLANE, unit->GenS.ObjectPlane);
   _mesa_TexGenfv(GL_T, GL_OBJECT_PLANE, unit->GenT.ObjectPlane);
   _mesa_TexGenfv(GL_R, GL_OBJECT_PLANE, unit->GenR.ObjectPlane);
   _mesa_TexGenfv(GL_Q, GL_OBJECT_PLANE, unit->GenQ.ObjectPlane);
   /* Eye plane done differently to avoid re-transformation */
   {
      struct gl_texture_unit *destUnit = &ctx->Texture.Unit;
      COPY_4FV(destUnit->GenS.EyePlane, unit->GenS.EyePlane);
      COPY_4FV(destUnit->GenT.EyePlane, unit->GenT.EyePlane);
      COPY_4FV(destUnit->GenR.EyePlane, unit->GenR.EyePlane);
      COPY_4FV(destUnit->GenQ.EyePlane, unit->GenQ.EyePlane);
      if (ctx->Driver.TexGen) {
         ctx->Driver.TexGen(ctx, GL_S, GL_EYE_PLANE, unit->GenS.EyePlane);
         ctx->Driver.TexGen(ctx, GL_T, GL_EYE_PLANE, unit->GenT.EyePlane);
         ctx->Driver.TexGen(ctx, GL_R, GL_EYE_PLANE, unit->GenR.EyePlane);
         ctx->Driver.TexGen(ctx, GL_Q, GL_EYE_PLANE, unit->GenQ.EyePlane);
      }
   }
   _mesa_set_enable(ctx, GL_TEXTURE_GEN_S, !!(unit->TexGenEnabled & S_BIT));
   _mesa_set_enable(ctx, GL_TEXTURE_GEN_T, !!(unit->TexGenEnabled & T_BIT));
   _mesa_set_enable(ctx, GL_TEXTURE_GEN_R, !!(unit->TexGenEnabled & R_BIT));
   _mesa_set_enable(ctx, GL_TEXTURE_GEN_Q, !!(unit->TexGenEnabled & Q_BIT));
   _mesa_TexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
      unit->LodBias);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,
      unit->Combine.ModeRGB);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,
      unit->Combine.ModeA);
   {
      const GLuint n = ctx->Extensions.NV_texture_env_combine4 ? 4 : 3;
      GLuint i;
      for (i = 0; i < n; i++) {
         _mesa_TexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB + i,
                       unit->Combine.SourceRGB[i]);
         _mesa_TexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA + i,
                       unit->Combine.SourceA[i]);
         _mesa_TexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB + i,
                       unit->Combine.OperandRGB[i]);
         _mesa_TexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA + i,
                       unit->Combine.OperandA[i]);
      }
   }
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, 1 << unit->Combine.ScaleShiftRGB);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1 << unit->Combine.ScaleShiftA);

   /* Restore texture object state for each target */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      const struct gl_texture_object *obj = NULL;
      const struct gl_sampler_object *samp;
      GLenum target;

      obj = &texstate->SavedObj[tgt];

      /* don't restore state for unsupported targets to prevent
       * raising GL errors.
       */
      if (obj->Target == GL_TEXTURE_CUBE_MAP_ARB &&
          !ctx->Extensions.ARB_texture_cube_map) {
         continue;
      }

      target = obj->Target;

      _mesa_BindTexture(target, obj->Name);

      samp = &obj->Sampler;

      _mesa_TexParameterfv(target, GL_TEXTURE_BORDER_COLOR, samp->BorderColor.f);
      _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, samp->WrapS);
      _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, samp->WrapT);
      _mesa_TexParameteri(target, GL_TEXTURE_WRAP_R, samp->WrapR);
      _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, samp->MinFilter);
      _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, samp->MagFilter);
      _mesa_TexParameterf(target, GL_TEXTURE_PRIORITY, obj->Priority);
      if (ctx->Extensions.EXT_texture_filter_anisotropic) {
         _mesa_TexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                             samp->MaxAnisotropy);
      }
   }

   /* remove saved references to the texture objects */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      _mesa_reference_texobj(&texstate->SavedTexRef[tgt], NULL);
   }

   _mesa_reference_shared_state(ctx, &texstate->SharedRef, NULL);

   _mesa_unlock_context_textures(ctx);
}


/*
 * This function is kind of long just because we have to call a lot
 * of device driver functions to update device driver state.
 *
 * XXX As it is now, most of the pop-code calls immediate-mode Mesa functions
 * in order to restore GL state.  This isn't terribly efficient but it
 * ensures that dirty flags and any derived state gets updated correctly.
 * We could at least check if the value to restore equals the current value
 * and then skip the Mesa call.
 */
void GLAPIENTRY
_mesa_PopAttrib(void)
{
   struct gl_attrib_node *attr, *next;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->AttribStackDepth == 0) {
      _mesa_error( ctx, GL_STACK_UNDERFLOW, "glPopAttrib" );
      return;
   }

   ctx->AttribStackDepth--;
   attr = ctx->AttribStack[ctx->AttribStackDepth];

   while (attr) {

      if (MESA_VERBOSE & VERBOSE_API) {
         _mesa_debug(ctx, "glPopAttrib %s\n",
                     _mesa_lookup_enum_by_nr(attr->kind));
      }

      switch (attr->kind) {
         case GL_ACCUM_BUFFER_BIT:
            {
               const struct gl_accum_attrib *accum;
               accum = (const struct gl_accum_attrib *) attr->data;
               _mesa_ClearAccum(accum->ClearColor[0],
                                accum->ClearColor[1],
                                accum->ClearColor[2],
                                accum->ClearColor[3]);
            }
            break;
         case GL_COLOR_BUFFER_BIT:
            {
               const struct gl_colorbuffer_attrib *color;

               color = (const struct gl_colorbuffer_attrib *) attr->data;
               _mesa_ClearIndex((GLfloat) color->ClearIndex);
               _mesa_ClearColor(color->ClearColor.f[0],
                                color->ClearColor.f[1],
                                color->ClearColor.f[2],
                                color->ClearColor.f[3]);
               _mesa_IndexMask(color->IndexMask);
               _mesa_ColorMask((GLboolean) (color->ColorMask[0] != 0),
                               (GLboolean) (color->ColorMask[1] != 0),
                               (GLboolean) (color->ColorMask[2] != 0),
                               (GLboolean) (color->ColorMask[3] != 0));
               _mesa_DrawBuffer(color->DrawBuffer);
               _mesa_set_enable(ctx, GL_ALPHA_TEST, color->AlphaEnabled);
               _mesa_AlphaFunc(color->AlphaFunc, color->AlphaRef);
               if (ctx->Color.BlendEnabled != color->BlendEnabled) {
                  _mesa_set_enable(ctx, GL_BLEND, (color->BlendEnabled & 1));
               }
               /* set same blend modes for all buffers */
               _mesa_BlendFunc(color->SrcFactor, color->DstFactor);
               _mesa_LogicOp(color->LogicOp);
               _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP,
                                color->ColorLogicOpEnabled);
               _mesa_set_enable(ctx, GL_INDEX_LOGIC_OP,
                                color->IndexLogicOpEnabled);
               _mesa_set_enable(ctx, GL_DITHER, color->DitherFlag);
            }
            break;
         case GL_CURRENT_BIT:
	    FLUSH_CURRENT( ctx, 0 );
            memcpy( &ctx->Current, attr->data,
		    sizeof(struct gl_current_attrib) );
            break;
         case GL_DEPTH_BUFFER_BIT:
            {
               const struct gl_depthbuffer_attrib *depth;
               depth = (const struct gl_depthbuffer_attrib *) attr->data;
               _mesa_DepthFunc(depth->Func);
               _mesa_ClearDepth(depth->Clear);
               _mesa_set_enable(ctx, GL_DEPTH_TEST, depth->Test);
               _mesa_DepthMask(depth->Mask);
            }
            break;
         case GL_ENABLE_BIT:
            {
               const struct gl_enable_attrib *enable;
               enable = (const struct gl_enable_attrib *) attr->data;
               pop_enable_group(ctx, enable);
	       ctx->NewState |= _NEW_ALL;
            }
            break;
         case GL_EVAL_BIT:
            memcpy( &ctx->Eval, attr->data, sizeof(struct gl_eval_attrib) );
	    ctx->NewState |= _NEW_EVAL;
            break;
         case GL_FOG_BIT:
            {
               const struct gl_fog_attrib *fog;
               fog = (const struct gl_fog_attrib *) attr->data;
               _mesa_set_enable(ctx, GL_FOG, fog->Enabled);
               _mesa_Fogfv(GL_FOG_COLOR, fog->Color);
               _mesa_Fogf(GL_FOG_DENSITY, fog->Density);
               _mesa_Fogf(GL_FOG_START, fog->Start);
               _mesa_Fogf(GL_FOG_END, fog->End);
               _mesa_Fogf(GL_FOG_INDEX, fog->Index);
               _mesa_Fogi(GL_FOG_MODE, fog->Mode);
            }
            break;
         case GL_HINT_BIT:
            {
               const struct gl_hint_attrib *hint;
               hint = (const struct gl_hint_attrib *) attr->data;
               _mesa_Hint(GL_PERSPECTIVE_CORRECTION_HINT,
                          hint->PerspectiveCorrection );
               _mesa_Hint(GL_POINT_SMOOTH_HINT, hint->PointSmooth);
               _mesa_Hint(GL_LINE_SMOOTH_HINT, hint->LineSmooth);
               _mesa_Hint(GL_POLYGON_SMOOTH_HINT, hint->PolygonSmooth);
               _mesa_Hint(GL_FOG_HINT, hint->Fog);
               _mesa_Hint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT,
                          hint->ClipVolumeClipping);
            }
            break;
         case GL_LIGHTING_BIT:
            {
               GLuint i;
               const struct gl_light_attrib *light;
               light = (const struct gl_light_attrib *) attr->data;
               /* lighting enable */
               _mesa_set_enable(ctx, GL_LIGHTING, light->Enabled);
               /* per-light state */
               if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top))
                  _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
	       
               for (i = 0; i < ctx->Const.MaxLights; i++) {
                  const struct gl_light *l = &light->Light[i];
                  _mesa_set_enable(ctx, GL_LIGHT0 + i, l->Enabled);
                  _mesa_light(ctx, i, GL_AMBIENT, l->Ambient);
                  _mesa_light(ctx, i, GL_DIFFUSE, l->Diffuse);
                  _mesa_light(ctx, i, GL_SPECULAR, l->Specular );
                  _mesa_light(ctx, i, GL_POSITION, l->EyePosition);
                  _mesa_light(ctx, i, GL_SPOT_DIRECTION, l->SpotDirection);
                  {
                     GLfloat p[4] = { 0 };
                     p[0] = l->SpotExponent;
                     _mesa_light(ctx, i, GL_SPOT_EXPONENT, p);
                  }
                  {
                     GLfloat p[4] = { 0 };
                     p[0] = l->SpotCutoff;
                     _mesa_light(ctx, i, GL_SPOT_CUTOFF, p);
                  }
                  {
                     GLfloat p[4] = { 0 };
                     p[0] = l->ConstantAttenuation;
                     _mesa_light(ctx, i, GL_CONSTANT_ATTENUATION, p);
                  }
                  {
                     GLfloat p[4] = { 0 };
                     p[0] = l->LinearAttenuation;
                     _mesa_light(ctx, i, GL_LINEAR_ATTENUATION, p);
                  }
                  {
                     GLfloat p[4] = { 0 };
                     p[0] = l->QuadraticAttenuation;
                     _mesa_light(ctx, i, GL_QUADRATIC_ATTENUATION, p);
                  }
               }
               /* light model */
               _mesa_LightModelfv(GL_LIGHT_MODEL_AMBIENT,
                                  light->Model.Ambient);
               _mesa_LightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER,
                                 (GLfloat) light->Model.LocalViewer);
               _mesa_LightModelf(GL_LIGHT_MODEL_TWO_SIDE,
                                 (GLfloat) light->Model.TwoSide);
               /* shade model */
               _mesa_ShadeModel(light->ShadeModel);
               /* color material */
               _mesa_ColorMaterial(light->ColorMaterialFace,
                                   light->ColorMaterialMode);
               _mesa_set_enable(ctx, GL_COLOR_MATERIAL,
                                light->ColorMaterialEnabled);
               /* materials */
               memcpy(&ctx->Light.Material, &light->Material,
                      sizeof(struct gl_material));
            }
            break;
         case GL_LINE_BIT:
            {
               const struct gl_line_attrib *line;
               line = (const struct gl_line_attrib *) attr->data;
               _mesa_set_enable(ctx, GL_LINE_SMOOTH, line->SmoothFlag);
               _mesa_set_enable(ctx, GL_LINE_STIPPLE, line->StippleFlag);
               _mesa_LineStipple(line->StippleFactor, line->StipplePattern);
               _mesa_LineWidth(line->Width);
            }
            break;
         case GL_LIST_BIT:
            memcpy( &ctx->List, attr->data, sizeof(struct gl_list_attrib) );
            break;
         case GL_PIXEL_MODE_BIT:
            memcpy( &ctx->Pixel, attr->data, sizeof(struct gl_pixel_attrib) );
            /* XXX what other pixel state needs to be set by function calls? */
            _mesa_ReadBuffer(ctx->Pixel.ReadBuffer);
	    ctx->NewState |= _NEW_PIXEL;
            break;
         case GL_POINT_BIT:
            {
               const struct gl_point_attrib *point;
               point = (const struct gl_point_attrib *) attr->data;
               _mesa_PointSize(point->Size);
               _mesa_set_enable(ctx, GL_POINT_SMOOTH, point->SmoothFlag);
            }
            break;
         case GL_POLYGON_BIT:
            {
               const struct gl_polygon_attrib *polygon;
               polygon = (const struct gl_polygon_attrib *) attr->data;
               _mesa_CullFace(polygon->CullFaceMode);
               _mesa_FrontFace(polygon->FrontFace);
               _mesa_PolygonMode(GL_FRONT, polygon->FrontMode);
               _mesa_PolygonMode(GL_BACK, polygon->BackMode);
               _mesa_PolygonOffset(polygon->OffsetFactor,
                                   polygon->OffsetUnits);
               _mesa_set_enable(ctx, GL_POLYGON_SMOOTH, polygon->SmoothFlag);
               _mesa_set_enable(ctx, GL_POLYGON_STIPPLE, polygon->StippleFlag);
               _mesa_set_enable(ctx, GL_CULL_FACE, polygon->CullFlag);
               _mesa_set_enable(ctx, GL_POLYGON_OFFSET_POINT,
                                polygon->OffsetPoint);
               _mesa_set_enable(ctx, GL_POLYGON_OFFSET_LINE,
                                polygon->OffsetLine);
               _mesa_set_enable(ctx, GL_POLYGON_OFFSET_FILL,
                                polygon->OffsetFill);
            }
            break;
	 case GL_POLYGON_STIPPLE_BIT:
	    memcpy( ctx->PolygonStipple, attr->data, 32*sizeof(GLuint) );
	    ctx->NewState |= _NEW_POLYGONSTIPPLE;
	    if (ctx->Driver.PolygonStipple)
	       ctx->Driver.PolygonStipple( ctx, (const GLubyte *) attr->data );
	    break;
         case GL_SCISSOR_BIT:
            {
               const struct gl_scissor_attrib *scissor;
               scissor = (const struct gl_scissor_attrib *) attr->data;
               _mesa_Scissor(scissor->X, scissor->Y,
                             scissor->Width, scissor->Height);
               _mesa_set_enable(ctx, GL_SCISSOR_TEST, scissor->Enabled);
            }
            break;
         case GL_STENCIL_BUFFER_BIT:
            {
               const struct gl_stencil_attrib *stencil;
               stencil = (const struct gl_stencil_attrib *) attr->data;
               _mesa_set_enable(ctx, GL_STENCIL_TEST, stencil->Enabled);
               _mesa_ClearStencil(stencil->Clear);
               _mesa_StencilFunc(stencil->Function,
                                 stencil->Ref,
                                 stencil->ValueMask);
               _mesa_StencilMask(stencil->WriteMask);
               _mesa_StencilOp(stencil->FailFunc,
                               stencil->ZFailFunc,
                               stencil->ZPassFunc);
            }
            break;
         case GL_TRANSFORM_BIT:
            {
               GLuint i;
               const struct gl_transform_attrib *xform;
               xform = (const struct gl_transform_attrib *) attr->data;
               _mesa_MatrixMode(xform->MatrixMode);
               if (_math_matrix_is_dirty(ctx->ProjectionMatrixStack.Top))
                  _math_matrix_analyse( ctx->ProjectionMatrixStack.Top );

               /* restore clip planes */
               for (i = 0; i < ctx->Const.MaxClipPlanes; i++) {
                  const GLuint mask = 1 << i;
                  const GLfloat *eyePlane = xform->EyeUserPlane[i];
                  COPY_4V(ctx->Transform.EyeUserPlane[i], eyePlane);
                  _mesa_set_enable(ctx, GL_CLIP_PLANE0 + i,
                                   !!(xform->ClipPlanesEnabled & mask));
                  if (ctx->Driver.ClipPlane)
                     ctx->Driver.ClipPlane( ctx, GL_CLIP_PLANE0 + i, eyePlane );
               }

               /* normalize/rescale */
               if (xform->Normalize != ctx->Transform.Normalize)
                  _mesa_set_enable(ctx, GL_NORMALIZE,ctx->Transform.Normalize);
               if (xform->RescaleNormals != ctx->Transform.RescaleNormals)
                  _mesa_set_enable(ctx, GL_RESCALE_NORMAL_EXT,
                                   ctx->Transform.RescaleNormals);
            }
            break;
         case GL_TEXTURE_BIT:
            /* Take care of texture object reference counters */
            {
               struct texture_state *texstate
                  = (struct texture_state *) attr->data;
               pop_texture_group(ctx, texstate);
	       ctx->NewState |= _NEW_TEXTURE;
            }
            break;
         case GL_VIEWPORT_BIT:
            {
               const struct gl_viewport_attrib *vp;
               vp = (const struct gl_viewport_attrib *) attr->data;
               _mesa_Viewport(vp->X, vp->Y, vp->Width, vp->Height);
               _mesa_DepthRange(vp->Near, vp->Far);
            }
            break;
         case GL_MULTISAMPLE_BIT_ARB:
            {
               const struct gl_multisample_attrib *ms;
               ms = (const struct gl_multisample_attrib *) attr->data;

	       TEST_AND_UPDATE(ctx->Multisample.Enabled,
			       ms->Enabled,
			       GL_MULTISAMPLE);

	       TEST_AND_UPDATE(ctx->Multisample.SampleCoverage,
			       ms->SampleCoverage,
			       GL_SAMPLE_COVERAGE);

	       TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToCoverage,
			       ms->SampleAlphaToCoverage,
			       GL_SAMPLE_ALPHA_TO_COVERAGE);

	       TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToOne,
			       ms->SampleAlphaToOne,
			       GL_SAMPLE_ALPHA_TO_ONE);

               _mesa_SampleCoverageARB(ms->SampleCoverageValue,
                                       ms->SampleCoverageInvert);
            }
            break;

         default:
            _mesa_problem( ctx, "Bad attrib flag in PopAttrib");
            break;
      }

      next = attr->next;
      FREE( attr->data );
      FREE( attr );
      attr = next;
   }
}


/**
 * Copy gl_pixelstore_attrib from src to dst, updating buffer
 * object refcounts.
 */
static void
copy_pixelstore(struct gl_context *ctx,
                struct gl_pixelstore_attrib *dst,
                const struct gl_pixelstore_attrib *src)
{
   dst->Alignment = src->Alignment;
   dst->RowLength = src->RowLength;
   dst->SkipPixels = src->SkipPixels;
   dst->SkipRows = src->SkipRows;
   dst->ImageHeight = src->ImageHeight;
   dst->SkipImages = src->SkipImages;
   dst->SwapBytes = src->SwapBytes;
   dst->LsbFirst = src->LsbFirst;
   dst->Invert = src->Invert;
}


#define GL_CLIENT_PACK_BIT (1<<20)
#define GL_CLIENT_UNPACK_BIT (1<<21)

/**
 * Copy gl_array_attrib from src to dest.
 * 'dest' must be in an initialized state.
 */
static void
copy_array_attrib(struct gl_context *ctx,
                  struct gl_array_attrib *dest,
                  struct gl_array_attrib *src)
{
    GLuint i;

    /* skip ArrayObj */
    /* skip DefaultArrayObj, Objects */
    dest->LockFirst = src->LockFirst;
    dest->LockCount = src->LockCount;
    /* skip NewState */
    /* skip RebindArrays */


    /* skip Name */
    /* skip RefCount */

    for (i = 0; i < Elements(src->VertexAttrib); i++)
        _mesa_copy_client_array(ctx, &dest->VertexAttrib[i], &src->VertexAttrib[i]);

    /* _Enabled must be the same than on push */
    dest->_Enabled = src->_Enabled;
    dest->_MaxElement = src->_MaxElement;

    /* skip ArrayBufferObj */
    /* skip ElementArrayBufferObj */
}

/**
 * Save the content of src to dest.
 */
static void
save_array_attrib(struct gl_context *ctx,
                  struct gl_array_attrib *dest,
                  struct gl_array_attrib *src)
{
   /* And copy all of the rest. */
   copy_array_attrib(ctx, dest, src);

   /* Just reference them here */
   _mesa_reference_buffer_object(ctx, &dest->ElementArrayBufferObj,
                                 src->ElementArrayBufferObj);
}

/**
 * Restore the content of src to dest.
 */
static void
restore_array_attrib(struct gl_context *ctx,
                     struct gl_array_attrib *dest,
                     struct gl_array_attrib *src)
{
    copy_array_attrib(ctx, dest, src);

   if (src->ElementArrayBufferObj->Name == 0
       || _mesa_IsBufferARB(src->ElementArrayBufferObj->Name))
      _mesa_BindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
			  src->ElementArrayBufferObj->Name);

   /* Better safe than sorry?! */
   dest->RebindArrays = GL_TRUE;

   /* FIXME: Should some bits in ctx->Array->NewState also be set
    * FIXME: here?  It seems like it should be set to inclusive-or
    * FIXME: of the old ArrayObj->_Enabled and the new _Enabled.
    * ... just do it.
    */
   dest->NewState |= src->_Enabled | dest->_Enabled;
}


void GLAPIENTRY
_mesa_PushClientAttrib(GLbitfield mask)
{
   struct gl_attrib_node *head;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->ClientAttribStackDepth >= MAX_CLIENT_ATTRIB_STACK_DEPTH) {
      _mesa_error( ctx, GL_STACK_OVERFLOW, "glPushClientAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute
    * groups specified by the mask.
    */
   head = NULL;

   if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      struct gl_pixelstore_attrib *attr;
      /* packing attribs */
      attr = CALLOC_STRUCT( gl_pixelstore_attrib );
      copy_pixelstore(ctx, attr, &ctx->Pack);
      save_attrib_data(&head, GL_CLIENT_PACK_BIT, attr);
      /* unpacking attribs */
      attr = CALLOC_STRUCT( gl_pixelstore_attrib );
      copy_pixelstore(ctx, attr, &ctx->Unpack);
      save_attrib_data(&head, GL_CLIENT_UNPACK_BIT, attr);
   }

   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      struct gl_array_attrib *attr;
      attr = CALLOC_STRUCT( gl_array_attrib );
      _mesa_init_varray(ctx, attr);
      save_array_attrib(ctx, attr, &ctx->Array);
      save_attrib_data(&head, GL_CLIENT_VERTEX_ARRAY_BIT, attr);
   }

   ctx->ClientAttribStack[ctx->ClientAttribStackDepth] = head;
   ctx->ClientAttribStackDepth++;
}




void GLAPIENTRY
_mesa_PopClientAttrib(void)
{
   struct gl_attrib_node *node, *next;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->ClientAttribStackDepth == 0) {
      _mesa_error( ctx, GL_STACK_UNDERFLOW, "glPopClientAttrib" );
      return;
   }

   ctx->ClientAttribStackDepth--;
   node = ctx->ClientAttribStack[ctx->ClientAttribStackDepth];

   while (node) {
      switch (node->kind) {
         case GL_CLIENT_PACK_BIT:
            {
               struct gl_pixelstore_attrib *store =
                  (struct gl_pixelstore_attrib *) node->data;
               copy_pixelstore(ctx, &ctx->Pack, store);
            }
	    ctx->NewState |= _NEW_PACKUNPACK;
            break;
         case GL_CLIENT_UNPACK_BIT:
            {
               struct gl_pixelstore_attrib *store =
                  (struct gl_pixelstore_attrib *) node->data;
               copy_pixelstore(ctx, &ctx->Unpack, store);
            }
	    ctx->NewState |= _NEW_PACKUNPACK;
            break;
         case GL_CLIENT_VERTEX_ARRAY_BIT: {
	    struct gl_array_attrib * attr =
	      (struct gl_array_attrib *) node->data;
            restore_array_attrib(ctx, &ctx->Array, attr);
            _mesa_free_varray_data(ctx, attr);
	    ctx->NewState |= _NEW_ARRAY;
            break;
	 }
         default:
            _mesa_problem( ctx, "Bad attrib flag in PopClientAttrib");
            break;
      }

      next = node->next;
      FREE( node->data );
      FREE( node );
      node = next;
   }
}


void
_mesa_init_attrib_dispatch(struct _glapi_table *disp)
{
   SET_PopAttrib(disp, _mesa_PopAttrib);
   SET_PushAttrib(disp, _mesa_PushAttrib);
   SET_PopClientAttrib(disp, _mesa_PopClientAttrib);
   SET_PushClientAttrib(disp, _mesa_PushClientAttrib);
}


#endif /* FEATURE_attrib_stack */


/**
 * Free any attribute state data that might be attached to the context.
 */
void
_mesa_free_attrib_data(struct gl_context *ctx)
{
   while (ctx->AttribStackDepth > 0) {
      struct gl_attrib_node *attr, *next;

      ctx->AttribStackDepth--;
      attr = ctx->AttribStack[ctx->AttribStackDepth];

      while (attr) {
         if (attr->kind == GL_TEXTURE_BIT) {
            struct texture_state *texstate = (struct texture_state*)attr->data;
            GLuint tgt;
            /* clear references to the saved texture objects */
            for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
               _mesa_reference_texobj(&texstate->SavedTexRef[tgt], NULL);
            }
            _mesa_reference_shared_state(ctx, &texstate->SharedRef, NULL);
         }
         else {
            /* any other chunks of state that requires special handling? */
         }

         next = attr->next;
         free(attr->data);
         free(attr);
         attr = next;
      }
   }
}


void _mesa_init_attrib( struct gl_context *ctx )
{
   /* Renderer and client attribute stacks */
   ctx->AttribStackDepth = 0;
   ctx->ClientAttribStackDepth = 0;
}
