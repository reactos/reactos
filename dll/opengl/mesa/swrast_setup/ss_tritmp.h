/*
 * Mesa 3-D graphics library
 * Version:  7.1
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


/**
 * This is where we handle assigning vertex colors based on front/back
 * facing, compute polygon offset and handle glPolygonMode().
 */
static void TAG(triangle)(struct gl_context *ctx, GLuint e0, GLuint e1, GLuint e2 )
{
#if IND & SS_TWOSIDE_BIT
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   GLchan saved_color[3][4] = { { 0 } };
   GLfloat saved_col0[3][4] = { { 0 } };
#endif
   SWvertex *verts = SWSETUP_CONTEXT(ctx)->verts;
   SWvertex *v[3];
#if IND & SS_OFFSET_BIT
   GLfloat z[3];
#endif
   GLfloat oz0, oz1, oz2;
   GLenum mode = GL_FILL;
   GLuint facing = 0;

   v[0] = &verts[e0];
   v[1] = &verts[e1];
   v[2] = &verts[e2];

#if IND & (SS_TWOSIDE_BIT | SS_OFFSET_BIT | SS_UNFILLED_BIT)
   {
      GLfloat ex = v[0]->attrib[FRAG_ATTRIB_WPOS][0] - v[2]->attrib[FRAG_ATTRIB_WPOS][0];
      GLfloat ey = v[0]->attrib[FRAG_ATTRIB_WPOS][1] - v[2]->attrib[FRAG_ATTRIB_WPOS][1];
      GLfloat fx = v[1]->attrib[FRAG_ATTRIB_WPOS][0] - v[2]->attrib[FRAG_ATTRIB_WPOS][0];
      GLfloat fy = v[1]->attrib[FRAG_ATTRIB_WPOS][1] - v[2]->attrib[FRAG_ATTRIB_WPOS][1];
      GLfloat cc  = ex*fy - ey*fx;

#if IND & (SS_TWOSIDE_BIT | SS_UNFILLED_BIT)
      {
	 facing = (cc < 0.0) ^ ctx->Polygon._FrontBit;

#if IND & SS_UNFILLED_BIT
	    mode = facing ? ctx->Polygon.BackMode : ctx->Polygon.FrontMode;
#endif

#if IND & SS_TWOSIDE_BIT
	 if (facing == 1) {
               if (VB->BackfaceColorPtr) {
                  GLfloat (*vbcolor)[4] = VB->BackfaceColorPtr->data;

                  if (swsetup->intColors) {
                     COPY_CHAN4(saved_color[0], v[0]->color);
                     COPY_CHAN4(saved_color[1], v[1]->color);
                     COPY_CHAN4(saved_color[2], v[2]->color);
                  }
                  else {
                     COPY_4V(saved_col0[0], v[0]->attrib[FRAG_ATTRIB_COL]);
                     COPY_4V(saved_col0[1], v[1]->attrib[FRAG_ATTRIB_COL]);
                     COPY_4V(saved_col0[2], v[2]->attrib[FRAG_ATTRIB_COL]);
                  }

                  if (VB->BackfaceColorPtr->stride) {
                     if (swsetup->intColors) {
                        SS_COLOR(v[0]->color, vbcolor[e0]);
                        SS_COLOR(v[1]->color, vbcolor[e1]);
                        SS_COLOR(v[2]->color, vbcolor[e2]);
                     }
                     else {
                        COPY_4V(v[0]->attrib[FRAG_ATTRIB_COL], vbcolor[e0]);
                        COPY_4V(v[1]->attrib[FRAG_ATTRIB_COL], vbcolor[e1]);
                        COPY_4V(v[2]->attrib[FRAG_ATTRIB_COL], vbcolor[e2]);
                     }
                  }
                  else {
                     /* flat shade */
                     if (swsetup->intColors) {
                        SS_COLOR(v[0]->color, vbcolor[0]);
                        SS_COLOR(v[1]->color, vbcolor[0]);
                        SS_COLOR(v[2]->color, vbcolor[0]);
                     }
                     else {
                        COPY_4V(v[0]->attrib[FRAG_ATTRIB_COL], vbcolor[0]);
                        COPY_4V(v[1]->attrib[FRAG_ATTRIB_COL], vbcolor[0]);
                        COPY_4V(v[2]->attrib[FRAG_ATTRIB_COL], vbcolor[0]);
                     }
                  }
               }
	    }
#endif
      }
#endif

#if IND & SS_OFFSET_BIT
      {
         GLfloat offset;
         const GLfloat max = ctx->DrawBuffer->_DepthMaxF;
         /* save original Z values (restored later) */
	 z[0] = v[0]->attrib[FRAG_ATTRIB_WPOS][2];
	 z[1] = v[1]->attrib[FRAG_ATTRIB_WPOS][2];
	 z[2] = v[2]->attrib[FRAG_ATTRIB_WPOS][2];
         /* Note that Z values are already scaled to [0,65535] (for example)
          * so no MRD value is used here.
          */
	 offset = ctx->Polygon.OffsetUnits;
	 if (cc * cc > 1e-16) {
	    const GLfloat ez = z[0] - z[2];
	    const GLfloat fz = z[1] - z[2];
	    const GLfloat oneOverArea = 1.0F / cc;
	    const GLfloat dzdx = FABSF((ey * fz - ez * fy) * oneOverArea);
	    const GLfloat dzdy = FABSF((ez * fx - ex * fz) * oneOverArea);
	    offset += MAX2(dzdx, dzdy) * ctx->Polygon.OffsetFactor;
	 }
         /* new Z values */
         oz0 = CLAMP(v[0]->attrib[FRAG_ATTRIB_WPOS][2] + offset, 0.0F, max);
         oz1 = CLAMP(v[1]->attrib[FRAG_ATTRIB_WPOS][2] + offset, 0.0F, max);
         oz2 = CLAMP(v[2]->attrib[FRAG_ATTRIB_WPOS][2] + offset, 0.0F, max);
      }
#endif
   }
#endif

   if (mode == GL_POINT) {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetPoint) {
	 v[0]->attrib[FRAG_ATTRIB_WPOS][2] = oz0;
	 v[1]->attrib[FRAG_ATTRIB_WPOS][2] = oz1;
	 v[2]->attrib[FRAG_ATTRIB_WPOS][2] = oz2;
      }
      _swsetup_render_tri(ctx, e0, e1, e2, facing, _swsetup_edge_render_point_tri);
   } else if (mode == GL_LINE) {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetLine) {
	 v[0]->attrib[FRAG_ATTRIB_WPOS][2] = oz0;
	 v[1]->attrib[FRAG_ATTRIB_WPOS][2] = oz1;
	 v[2]->attrib[FRAG_ATTRIB_WPOS][2] = oz2;
      }
      _swsetup_render_tri(ctx, e0, e1, e2, facing, _swsetup_edge_render_line_tri);
   }
   else {
#if IND & SS_OFFSET_BIT
      if (ctx->Polygon.OffsetFill) {
	 v[0]->attrib[FRAG_ATTRIB_WPOS][2] = oz0;
	 v[1]->attrib[FRAG_ATTRIB_WPOS][2] = oz1;
	 v[2]->attrib[FRAG_ATTRIB_WPOS][2] = oz2;
      }
#endif
      _swrast_Triangle( ctx, v[0], v[1], v[2] );
   }

   /*
    * Restore original vertex colors, etc.
    */
#if IND & SS_OFFSET_BIT
   {
      v[0]->attrib[FRAG_ATTRIB_WPOS][2] = z[0];
      v[1]->attrib[FRAG_ATTRIB_WPOS][2] = z[1];
      v[2]->attrib[FRAG_ATTRIB_WPOS][2] = z[2];
   }
#endif

#if IND & SS_TWOSIDE_BIT
   {
      if (facing == 1) {
	if (VB->BackfaceColorPtr) {
	  if (swsetup->intColors) {
	    COPY_CHAN4(v[0]->color, saved_color[0]);
	    COPY_CHAN4(v[1]->color, saved_color[1]);
	    COPY_CHAN4(v[2]->color, saved_color[2]);
	  }
	  else {
	    COPY_4V(v[0]->attrib[FRAG_ATTRIB_COL], saved_col0[0]);
	    COPY_4V(v[1]->attrib[FRAG_ATTRIB_COL], saved_col0[1]);
	    COPY_4V(v[2]->attrib[FRAG_ATTRIB_COL], saved_col0[2]);
	  }
	}
      }
   }
#endif
}



/* Need to fixup edgeflags when decomposing to triangles:
 */
static void TAG(quadfunc)( struct gl_context *ctx, GLuint v0,
		       GLuint v1, GLuint v2, GLuint v3 )
{
   if (IND & SS_UNFILLED_BIT) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
      if (VB->EdgeFlag) { /* XXX this test shouldn't be needed (bug 12614) */
         GLubyte ef1 = VB->EdgeFlag[v1];
         GLubyte ef3 = VB->EdgeFlag[v3];
         VB->EdgeFlag[v1] = 0;
         TAG(triangle)( ctx, v0, v1, v3 );
         VB->EdgeFlag[v1] = ef1;
         VB->EdgeFlag[v3] = 0;
         TAG(triangle)( ctx, v1, v2, v3 );
         VB->EdgeFlag[v3] = ef3;
      }
   } else {
      TAG(triangle)( ctx, v0, v1, v3 );
      TAG(triangle)( ctx, v1, v2, v3 );
   }
}




static void TAG(init)( void )
{
   tri_tab[IND] = TAG(triangle);
   quad_tab[IND] = TAG(quadfunc);
}


#undef IND
#undef TAG
