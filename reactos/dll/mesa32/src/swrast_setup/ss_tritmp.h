/*
 * Mesa 3-D graphics library
 * Version:  6.1
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


static void TAG(triangle)(GLcontext *ctx, GLuint e0, GLuint e1, GLuint e2 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   SWvertex *verts = SWSETUP_CONTEXT(ctx)->verts;
   SWvertex *v[3];
   GLfloat z[3];
   GLfloat offset;
   GLenum mode = GL_FILL;
   GLuint facing = 0;
   GLchan saved_color[3][4];
   GLchan saved_spec[3][4];
   GLfloat saved_index[3];

   v[0] = &verts[e0];
   v[1] = &verts[e1];
   v[2] = &verts[e2];


   if (IND & (SS_TWOSIDE_BIT | SS_OFFSET_BIT | SS_UNFILLED_BIT))
   {
      GLfloat ex = v[0]->win[0] - v[2]->win[0];
      GLfloat ey = v[0]->win[1] - v[2]->win[1];
      GLfloat fx = v[1]->win[0] - v[2]->win[0];
      GLfloat fy = v[1]->win[1] - v[2]->win[1];
      GLfloat cc  = ex*fy - ey*fx;

      if (IND & (SS_TWOSIDE_BIT | SS_UNFILLED_BIT))
      {
	 facing = (cc < 0.0) ^ ctx->Polygon._FrontBit;
         if (ctx->Stencil.TestTwoSide)
            ctx->_Facing = facing; /* for 2-sided stencil test */

	 if (IND & SS_UNFILLED_BIT)
	    mode = facing ? ctx->Polygon.BackMode : ctx->Polygon.FrontMode;

	 if (facing == 1) {
	    if (IND & SS_TWOSIDE_BIT) {
	       if (IND & SS_RGBA_BIT) {
		  GLfloat (*vbcolor)[4] = VB->ColorPtr[1]->data;

		  COPY_CHAN4(saved_color[0], v[0]->color);
		  COPY_CHAN4(saved_color[1], v[1]->color);
		  COPY_CHAN4(saved_color[2], v[2]->color);

		  if (VB->ColorPtr[1]->stride) {
		     SS_COLOR(v[0]->color, vbcolor[e0]);
		     SS_COLOR(v[1]->color, vbcolor[e1]);
		     SS_COLOR(v[2]->color, vbcolor[e2]);
		  }
		  else {
		     SS_COLOR(v[0]->color, vbcolor[0]);
		     SS_COLOR(v[1]->color, vbcolor[0]);
		     SS_COLOR(v[2]->color, vbcolor[0]);
		  }

		  if (VB->SecondaryColorPtr[1]) {
		     GLfloat (*vbspec)[4] = VB->SecondaryColorPtr[1]->data;

		     COPY_CHAN4(saved_spec[0], v[0]->specular);
		     COPY_CHAN4(saved_spec[1], v[1]->specular);
		     COPY_CHAN4(saved_spec[2], v[2]->specular);

		     if (VB->SecondaryColorPtr[1]->stride) {
			SS_SPEC(v[0]->specular, vbspec[e0]);
			SS_SPEC(v[1]->specular, vbspec[e1]);
			SS_SPEC(v[2]->specular, vbspec[e2]);
		     }
		     else {
			SS_SPEC(v[0]->specular, vbspec[0]);
			SS_SPEC(v[1]->specular, vbspec[0]);
			SS_SPEC(v[2]->specular, vbspec[0]);
		     }
		  }
	       } else {
		  GLfloat *vbindex = (GLfloat *)VB->IndexPtr[1]->data;
		  saved_index[0] = v[0]->index;
		  saved_index[1] = v[1]->index;
		  saved_index[2] = v[2]->index;
		  
		  SS_IND(v[0]->index, (GLuint) vbindex[e0]);
		  SS_IND(v[1]->index, (GLuint) vbindex[e1]);
		  SS_IND(v[2]->index, (GLuint) vbindex[e2]);
	       }
	    }
	 }
      }

      if (IND & SS_OFFSET_BIT)
      {
	 offset = ctx->Polygon.OffsetUnits * ctx->DrawBuffer->_MRD;
	 z[0] = v[0]->win[2];
	 z[1] = v[1]->win[2];
	 z[2] = v[2]->win[2];
	 if (cc * cc > 1e-16) {
	    const GLfloat ez = z[0] - z[2];
	    const GLfloat fz = z[1] - z[2];
	    const GLfloat oneOverArea = 1.0F / cc;
	    const GLfloat dzdx = FABSF((ey * fz - ez * fy) * oneOverArea);
	    const GLfloat dzdy = FABSF((ez * fx - ex * fz) * oneOverArea);
	    offset += MAX2(dzdx, dzdy) * ctx->Polygon.OffsetFactor;
            /* Unfortunately, we need to clamp to prevent negative Zs below.
             * Technically, we should do the clamping per-fragment.
             */
            offset = MAX2(offset, -v[0]->win[2]);
            offset = MAX2(offset, -v[1]->win[2]);
            offset = MAX2(offset, -v[2]->win[2]);
	 }
      }
   }

   if (mode == GL_POINT) {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetPoint) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      _swsetup_render_point_tri( ctx, e0, e1, e2, facing );
   } else if (mode == GL_LINE) {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetLine) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      _swsetup_render_line_tri( ctx, e0, e1, e2, facing );
   } else {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetFill) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      _swrast_Triangle( ctx, v[0], v[1], v[2] );
   }

   if (IND & SS_OFFSET_BIT) {
      v[0]->win[2] = z[0];
      v[1]->win[2] = z[1];
      v[2]->win[2] = z[2];
   }

   if (IND & SS_TWOSIDE_BIT) {
      if (facing == 1) {
	 if (IND & SS_RGBA_BIT) {
	    COPY_CHAN4(v[0]->color, saved_color[0]);
	    COPY_CHAN4(v[1]->color, saved_color[1]);
	    COPY_CHAN4(v[2]->color, saved_color[2]);
	    if (VB->SecondaryColorPtr[1]) {
	       COPY_CHAN4(v[0]->specular, saved_spec[0]);
	       COPY_CHAN4(v[1]->specular, saved_spec[1]);
	       COPY_CHAN4(v[2]->specular, saved_spec[2]);
	    }
	 } else {
	    v[0]->index = saved_index[0];
	    v[1]->index = saved_index[1];
	    v[2]->index = saved_index[2];
	 }
      }
   }
}



/* Need to fixup edgeflags when decomposing to triangles:
 */
static void TAG(quadfunc)( GLcontext *ctx, GLuint v0,
		       GLuint v1, GLuint v2, GLuint v3 )
{
   if (IND & SS_UNFILLED_BIT) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
      GLubyte ef1 = VB->EdgeFlag[v1];
      GLubyte ef3 = VB->EdgeFlag[v3];
      VB->EdgeFlag[v1] = 0;
      TAG(triangle)( ctx, v0, v1, v3 );
      VB->EdgeFlag[v1] = ef1;
      VB->EdgeFlag[v3] = 0;
      TAG(triangle)( ctx, v1, v2, v3 );
      VB->EdgeFlag[v3] = ef3;
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
