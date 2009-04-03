/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


/* Template for building functions to plug into the driver interface
 * of t_vb_render.c:
 *     ctx->Driver.QuadFunc
 *     ctx->Driver.TriangleFunc
 *     ctx->Driver.LineFunc
 *     ctx->Driver.PointsFunc
 *
 * DO_TWOSIDE:   Plug back-color values from the VB into backfacing triangles,
 *               and restore vertices afterwards.
 * DO_OFFSET:    Calculate offset for triangles and adjust vertices.  Restore
 *               vertices after rendering.
 * DO_FLAT:      For hardware without native flatshading, copy provoking colors
 *               into the other vertices.  Restore after rendering.
 * DO_UNFILLED:  Decompose triangles to lines and points where appropriate.
 * DO_TWOSTENCIL:Gross hack for two-sided stencil.
 *
 * HAVE_RGBA: Vertices have rgba values (otherwise index values).
 * HAVE_SPEC: Vertices have secondary rgba values.
 *
 * VERT_X(v): Alias for vertex x value.
 * VERT_Y(v): Alias for vertex y value.
 * VERT_Z(v): Alias for vertex z value.
 * DEPTH_SCALE: Scale for constant offset.
 * REVERSE_DEPTH: Viewport depth range reversed.
 *
 * VERTEX: Hardware vertex type.
 * GET_VERTEX(n): Retreive vertex with index n.
 * AREA_IS_CCW(a): Return true if triangle with signed area a is ccw.
 *
 * VERT_SET_RGBA: Assign vertex rgba from VB color.
 * VERT_COPY_RGBA: Copy vertex rgba another vertex.
 * VERT_SAVE_RGBA: Save vertex rgba to a local variable.
 * VERT_RESTORE_RGBA: Restore vertex rgba from a local variable.
 *   --> Similar for IND and SPEC.
 *
 * LOCAL_VARS(n): (At least) define local vars for save/restore rgba.
 *
 */

#if HAVE_RGBA
#define VERT_SET_IND( v, c ) (void) c
#define VERT_COPY_IND( v0, v1 )
#define VERT_SAVE_IND( idx )
#define VERT_RESTORE_IND( idx )
#if HAVE_BACK_COLORS
#define VERT_SET_RGBA( v, c )
#endif
#else
#define VERT_SET_RGBA( v, c ) (void) c
#define VERT_COPY_RGBA( v0, v1 )
#define VERT_SAVE_RGBA( idx )
#define VERT_RESTORE_RGBA( idx )
#if HAVE_BACK_COLORS
#define VERT_SET_IND( v, c )
#endif
#endif

#if !HAVE_SPEC
#define VERT_SET_SPEC( v, c ) (void) c
#define VERT_COPY_SPEC( v0, v1 )
#define VERT_SAVE_SPEC( idx )
#define VERT_RESTORE_SPEC( idx )
#if HAVE_BACK_COLORS
#define VERT_COPY_SPEC1( v )
#endif
#else
#if HAVE_BACK_COLORS
#define VERT_SET_SPEC( v, c )
#endif
#endif

#if !HAVE_BACK_COLORS
#define VERT_COPY_SPEC1( v )
#define VERT_COPY_IND1( v )
#define VERT_COPY_RGBA1( v )
#endif

#ifndef INSANE_VERTICES
#define VERT_SET_Z(v,val) VERT_Z(v) = val
#define VERT_Z_ADD(v,val) VERT_Z(v) += val
#endif

#ifndef REVERSE_DEPTH
#define REVERSE_DEPTH 0
#endif

/* disable twostencil for un-aware drivers */
#ifndef HAVE_STENCIL_TWOSIDE
#define HAVE_STENCIL_TWOSIDE 0
#endif
#ifndef DO_TWOSTENCIL
#define DO_TWOSTENCIL 0
#endif
#ifndef SETUP_STENCIL
#define SETUP_STENCIL(f)
#endif
#ifndef UNSET_STENCIL
#define UNSET_STENCIL(f)
#endif

#if DO_TRI
static void TAG(triangle)( GLcontext *ctx, GLuint e0, GLuint e1, GLuint e2 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   VERTEX *v[3];
   GLfloat offset = 0;
   GLfloat z[3];
   GLenum mode = GL_FILL;
   GLuint facing = 0;
   LOCAL_VARS(3);

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   v[0] = (VERTEX *)GET_VERTEX(e0);
   v[1] = (VERTEX *)GET_VERTEX(e1);
   v[2] = (VERTEX *)GET_VERTEX(e2);

   if (DO_TWOSIDE || DO_OFFSET || DO_UNFILLED || DO_TWOSTENCIL)
   {
      GLfloat ex = VERT_X(v[0]) - VERT_X(v[2]);
      GLfloat ey = VERT_Y(v[0]) - VERT_Y(v[2]);
      GLfloat fx = VERT_X(v[1]) - VERT_X(v[2]);
      GLfloat fy = VERT_Y(v[1]) - VERT_Y(v[2]);
      GLfloat cc = ex*fy - ey*fx;

      if (DO_TWOSIDE || DO_UNFILLED || DO_TWOSTENCIL)
      {
	 facing = AREA_IS_CCW( cc ) ^ ctx->Polygon._FrontBit;

	 if (DO_UNFILLED) {
	    if (facing) {
	       mode = ctx->Polygon.BackMode;
	       if (ctx->Polygon.CullFlag &&
		   ctx->Polygon.CullFaceMode != GL_FRONT) {
		  return;
	       }
	    } else {
	       mode = ctx->Polygon.FrontMode;
	       if (ctx->Polygon.CullFlag &&
		   ctx->Polygon.CullFaceMode != GL_BACK) {
		  return;
	       }
	    }
	 }

	 if (DO_TWOSIDE && facing == 1)
	 {
	    if (HAVE_RGBA) {
	       if (HAVE_BACK_COLORS) {
		  if (!DO_FLAT) {
		     VERT_SAVE_RGBA( 0 );
		     VERT_SAVE_RGBA( 1 );
		     VERT_COPY_RGBA1( v[0] );
		     VERT_COPY_RGBA1( v[1] );
		  }
		  VERT_SAVE_RGBA( 2 );
		  VERT_COPY_RGBA1( v[2] );
		  if (HAVE_SPEC) {
		     if (!DO_FLAT) {
			VERT_SAVE_SPEC( 0 );
			VERT_SAVE_SPEC( 1 );
			VERT_COPY_SPEC1( v[0] );
			VERT_COPY_SPEC1( v[1] );
		     }
		     VERT_SAVE_SPEC( 2 );
		     VERT_COPY_SPEC1( v[2] );
		  }
	       }
	       else {
		  GLfloat (*vbcolor)[4] = VB->ColorPtr[1]->data;
		  (void) vbcolor;

		  if (!DO_FLAT) {
		     VERT_SAVE_RGBA( 0 );
		     VERT_SAVE_RGBA( 1 );
		  }
		  VERT_SAVE_RGBA( 2 );

		  if (VB->ColorPtr[1]->stride) {
		     ASSERT(VB->ColorPtr[1]->stride == 4*sizeof(GLfloat));

		     if (!DO_FLAT) {		  
			VERT_SET_RGBA( v[0], vbcolor[e0] );
			VERT_SET_RGBA( v[1], vbcolor[e1] );
		     }
		     VERT_SET_RGBA( v[2], vbcolor[e2] );
		  }
		  else {
		     if (!DO_FLAT) {		  
			VERT_SET_RGBA( v[0], vbcolor[0] );
			VERT_SET_RGBA( v[1], vbcolor[0] );
		     }
		     VERT_SET_RGBA( v[2], vbcolor[0] );
		  }

		  if (HAVE_SPEC && VB->SecondaryColorPtr[1]) {
		     GLfloat (*vbspec)[4] = VB->SecondaryColorPtr[1]->data;
		     ASSERT(VB->SecondaryColorPtr[1]->stride == 4*sizeof(GLfloat));

		     if (!DO_FLAT) {
			VERT_SAVE_SPEC( 0 );
			VERT_SAVE_SPEC( 1 );
			VERT_SET_SPEC( v[0], vbspec[e0] );
			VERT_SET_SPEC( v[1], vbspec[e1] );
		     }
		     VERT_SAVE_SPEC( 2 );
		     VERT_SET_SPEC( v[2], vbspec[e2] );
		  }
	       }
	    }
	    else {
	       GLfloat (*vbindex) = (GLfloat *)VB->IndexPtr[1]->data;
	       if (!DO_FLAT) {
		  VERT_SAVE_IND( 0 );
		  VERT_SAVE_IND( 1 );
		  VERT_SET_IND( v[0], vbindex[e0] );
		  VERT_SET_IND( v[1], vbindex[e1] );
	       }
	       VERT_SAVE_IND( 2 );
	       VERT_SET_IND( v[2], vbindex[e2] );
	    }
	 }
      }


      if (DO_OFFSET)
      {
	 offset = ctx->Polygon.OffsetUnits * DEPTH_SCALE;
	 z[0] = VERT_Z(v[0]);
	 z[1] = VERT_Z(v[1]);
	 z[2] = VERT_Z(v[2]);
	 if (cc * cc > 1e-16) {
	    GLfloat ic	= 1.0 / cc;
	    GLfloat ez	= z[0] - z[2];
	    GLfloat fz	= z[1] - z[2];
	    GLfloat a	= ey*fz - ez*fy;
	    GLfloat b	= ez*fx - ex*fz;
	    GLfloat ac	= a * ic;
	    GLfloat bc	= b * ic;
	    if ( ac < 0.0f ) ac = -ac;
	    if ( bc < 0.0f ) bc = -bc;
	    offset += MAX2( ac, bc ) * ctx->Polygon.OffsetFactor / ctx->DrawBuffer->_MRD;
	 }
	 offset *= ctx->DrawBuffer->_MRD * (REVERSE_DEPTH ? -1.0 : 1.0);
      }
   }

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_SAVE_RGBA( 0 );
	 VERT_SAVE_RGBA( 1 );
	 VERT_COPY_RGBA( v[0], v[2] );
	 VERT_COPY_RGBA( v[1], v[2] );
	 if (HAVE_SPEC && VB->SecondaryColorPtr[0]) {
	    VERT_SAVE_SPEC( 0 );
	    VERT_SAVE_SPEC( 1 );
	    VERT_COPY_SPEC( v[0], v[2] );
	    VERT_COPY_SPEC( v[1], v[2] );
	 }
      }
      else {
	 VERT_SAVE_IND( 0 );
	 VERT_SAVE_IND( 1 );
	 VERT_COPY_IND( v[0], v[2] );
	 VERT_COPY_IND( v[1], v[2] );
      }
   }

   if (mode == GL_POINT) {
      if (DO_OFFSET && ctx->Polygon.OffsetPoint) {
	 VERT_Z_ADD(v[0], offset);
	 VERT_Z_ADD(v[1], offset);
	 VERT_Z_ADD(v[2], offset);
      }
      if (DO_TWOSTENCIL && !HAVE_STENCIL_TWOSIDE && ctx->Stencil.TestTwoSide) {
         SETUP_STENCIL(facing);
         UNFILLED_TRI( ctx, GL_POINT, e0, e1, e2 );
         UNSET_STENCIL(facing);
      } else {
         UNFILLED_TRI( ctx, GL_POINT, e0, e1, e2 );
      }
   } else if (mode == GL_LINE) {
      if (DO_OFFSET && ctx->Polygon.OffsetLine) {
	 VERT_Z_ADD(v[0], offset);
	 VERT_Z_ADD(v[1], offset);
	 VERT_Z_ADD(v[2], offset);
      }
      if (DO_TWOSTENCIL && !HAVE_STENCIL_TWOSIDE && ctx->Stencil.TestTwoSide) {
         SETUP_STENCIL(facing);
         UNFILLED_TRI( ctx, GL_LINE, e0, e1, e2 );
         UNSET_STENCIL(facing);
      } else {
         UNFILLED_TRI( ctx, GL_LINE, e0, e1, e2 );
      }
   } else {
      if (DO_OFFSET && ctx->Polygon.OffsetFill) {
	 VERT_Z_ADD(v[0], offset);
	 VERT_Z_ADD(v[1], offset);
	 VERT_Z_ADD(v[2], offset);
      }
      if (DO_UNFILLED)
	 RASTERIZE( GL_TRIANGLES );
      if (DO_TWOSTENCIL && !HAVE_STENCIL_TWOSIDE && ctx->Stencil.TestTwoSide) {
         SETUP_STENCIL(facing);
         TRI( v[0], v[1], v[2] );
         UNSET_STENCIL(facing);
      } else {
         TRI( v[0], v[1], v[2] );
      }
   }

   if (DO_OFFSET)
   {
      VERT_SET_Z(v[0], z[0]);
      VERT_SET_Z(v[1], z[1]);
      VERT_SET_Z(v[2], z[2]);
   }

   if (DO_TWOSIDE && facing == 1)
   {
      if (HAVE_RGBA) {
	 if (!DO_FLAT) {
	    VERT_RESTORE_RGBA( 0 );
	    VERT_RESTORE_RGBA( 1 );
	 }
	 VERT_RESTORE_RGBA( 2 );
	 if (HAVE_SPEC) {
	    if (!DO_FLAT) {
	       VERT_RESTORE_SPEC( 0 );
	       VERT_RESTORE_SPEC( 1 );
	    }
	    VERT_RESTORE_SPEC( 2 );
	 }
      }
      else {
	 if (!DO_FLAT) {
	    VERT_RESTORE_IND( 0 );
	    VERT_RESTORE_IND( 1 );
	 }
	 VERT_RESTORE_IND( 2 );
      }
   }


   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_RGBA( 0 );
	 VERT_RESTORE_RGBA( 1 );
	 if (HAVE_SPEC && VB->SecondaryColorPtr[0]) {
	    VERT_RESTORE_SPEC( 0 );
	    VERT_RESTORE_SPEC( 1 );
	 }
      }
      else {
	 VERT_RESTORE_IND( 0 );
	 VERT_RESTORE_IND( 1 );
      }
   }
}
#endif

#if DO_QUAD
#if DO_FULL_QUAD
static void TAG(quadr)( GLcontext *ctx,
		       GLuint e0, GLuint e1, GLuint e2, GLuint e3 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   VERTEX *v[4];
   GLfloat offset = 0;
   GLfloat z[4];
   GLenum mode = GL_FILL;
   GLuint facing = 0;
   LOCAL_VARS(4);

   v[0] = (VERTEX *)GET_VERTEX(e0);
   v[1] = (VERTEX *)GET_VERTEX(e1);
   v[2] = (VERTEX *)GET_VERTEX(e2);
   v[3] = (VERTEX *)GET_VERTEX(e3);

   if (DO_TWOSIDE || DO_OFFSET || DO_UNFILLED || DO_TWOSTENCIL)
   {
      GLfloat ex = VERT_X(v[2]) - VERT_X(v[0]);
      GLfloat ey = VERT_Y(v[2]) - VERT_Y(v[0]);
      GLfloat fx = VERT_X(v[3]) - VERT_X(v[1]);
      GLfloat fy = VERT_Y(v[3]) - VERT_Y(v[1]);
      GLfloat cc = ex*fy - ey*fx;

      if (DO_TWOSIDE || DO_UNFILLED || DO_TWOSTENCIL)
      {
	 facing = AREA_IS_CCW( cc ) ^ ctx->Polygon._FrontBit;

	 if (DO_UNFILLED) {
	    if (facing) {
	       mode = ctx->Polygon.BackMode;
	       if (ctx->Polygon.CullFlag &&
		   ctx->Polygon.CullFaceMode != GL_FRONT) {
		  return;
	       }
	    } else {
	       mode = ctx->Polygon.FrontMode;
	       if (ctx->Polygon.CullFlag &&
		   ctx->Polygon.CullFaceMode != GL_BACK) {
		  return;
	       }
	    }
	 }

	 if (DO_TWOSIDE && facing == 1)
	 {
	    if (HAVE_RGBA) {
	       GLfloat (*vbcolor)[4] = VB->ColorPtr[1]->data;
	       (void)vbcolor;

	       if (HAVE_BACK_COLORS) {
                  if (!DO_FLAT) {
                     VERT_SAVE_RGBA( 0 );
                     VERT_SAVE_RGBA( 1 );
                     VERT_SAVE_RGBA( 2 );
		     VERT_COPY_RGBA1( v[0] );
		     VERT_COPY_RGBA1( v[1] );
		     VERT_COPY_RGBA1( v[2] );
		  }
		  VERT_SAVE_RGBA( 3 );
		  VERT_COPY_RGBA1( v[3] );
		  if (HAVE_SPEC) {
                     if (!DO_FLAT) {
                        VERT_SAVE_SPEC( 0 );
                        VERT_SAVE_SPEC( 1 );
                        VERT_SAVE_SPEC( 2 );
			VERT_COPY_SPEC1( v[0] );
			VERT_COPY_SPEC1( v[1] );
			VERT_COPY_SPEC1( v[2] );
		     }
		     VERT_SAVE_SPEC( 3 );
		     VERT_COPY_SPEC1( v[3] );
		  }
	       }
	       else {
	          if (!DO_FLAT) {
		     VERT_SAVE_RGBA( 0 );
		     VERT_SAVE_RGBA( 1 );
		     VERT_SAVE_RGBA( 2 );
		  }
	          VERT_SAVE_RGBA( 3 );

		  if (VB->ColorPtr[1]->stride) {
		     if (!DO_FLAT) {
			VERT_SET_RGBA( v[0], vbcolor[e0] );
			VERT_SET_RGBA( v[1], vbcolor[e1] );
			VERT_SET_RGBA( v[2], vbcolor[e2] );
		     }
		     VERT_SET_RGBA( v[3], vbcolor[e3] );
		  }
		  else {
		     if (!DO_FLAT) {
			VERT_SET_RGBA( v[0], vbcolor[0] );
			VERT_SET_RGBA( v[1], vbcolor[0] );
			VERT_SET_RGBA( v[2], vbcolor[0] );
		     }
		     VERT_SET_RGBA( v[3], vbcolor[0] );
		  }

	          if (HAVE_SPEC && VB->SecondaryColorPtr[1]) {
		     GLfloat (*vbspec)[4] = VB->SecondaryColorPtr[1]->data;
		     ASSERT(VB->SecondaryColorPtr[1]->stride==4*sizeof(GLfloat));

		     if (!DO_FLAT) {
		        VERT_SAVE_SPEC( 0 );
		        VERT_SAVE_SPEC( 1 );
		        VERT_SAVE_SPEC( 2 );
		        VERT_SET_SPEC( v[0], vbspec[e0] );
		        VERT_SET_SPEC( v[1], vbspec[e1] );
		        VERT_SET_SPEC( v[2], vbspec[e2] );
		     }
		     VERT_SAVE_SPEC( 3 );
		     VERT_SET_SPEC( v[3], vbspec[e3] );
	          }
	       }
	    }
	    else {
	       GLfloat *vbindex = (GLfloat *)VB->IndexPtr[1]->data;
	       if (!DO_FLAT) {
		  VERT_SAVE_IND( 0 );
		  VERT_SAVE_IND( 1 );
		  VERT_SAVE_IND( 2 );
		  VERT_SET_IND( v[0], vbindex[e0] );
		  VERT_SET_IND( v[1], vbindex[e1] );
		  VERT_SET_IND( v[2], vbindex[e2] );
	       }
	       VERT_SAVE_IND( 3 );
	       VERT_SET_IND( v[3], vbindex[e3] );
	    }
	 }
      }


      if (DO_OFFSET)
      {
	 offset = ctx->Polygon.OffsetUnits * DEPTH_SCALE;
	 z[0] = VERT_Z(v[0]);
	 z[1] = VERT_Z(v[1]);
	 z[2] = VERT_Z(v[2]);
	 z[3] = VERT_Z(v[3]);
	 if (cc * cc > 1e-16) {
	    GLfloat ez = z[2] - z[0];
	    GLfloat fz = z[3] - z[1];
	    GLfloat a	= ey*fz - ez*fy;
	    GLfloat b	= ez*fx - ex*fz;
	    GLfloat ic	= 1.0 / cc;
	    GLfloat ac	= a * ic;
	    GLfloat bc	= b * ic;
	    if ( ac < 0.0f ) ac = -ac;
	    if ( bc < 0.0f ) bc = -bc;
	    offset += MAX2( ac, bc ) * ctx->Polygon.OffsetFactor / ctx->DrawBuffer->_MRD;
	 }
	 offset *= ctx->DrawBuffer->_MRD * (REVERSE_DEPTH ? -1.0 : 1.0);
      }
   }

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_SAVE_RGBA( 0 );
	 VERT_SAVE_RGBA( 1 );
	 VERT_SAVE_RGBA( 2 );
	 VERT_COPY_RGBA( v[0], v[3] );
	 VERT_COPY_RGBA( v[1], v[3] );
	 VERT_COPY_RGBA( v[2], v[3] );
	 if (HAVE_SPEC && VB->SecondaryColorPtr[0]) {
	    VERT_SAVE_SPEC( 0 );
	    VERT_SAVE_SPEC( 1 );
	    VERT_SAVE_SPEC( 2 );
	    VERT_COPY_SPEC( v[0], v[3] );
	    VERT_COPY_SPEC( v[1], v[3] );
	    VERT_COPY_SPEC( v[2], v[3] );
	 }
      }
      else {
	 VERT_SAVE_IND( 0 );
	 VERT_SAVE_IND( 1 );
	 VERT_SAVE_IND( 2 );
	 VERT_COPY_IND( v[0], v[3] );
	 VERT_COPY_IND( v[1], v[3] );
	 VERT_COPY_IND( v[2], v[3] );
      }
   }

   if (mode == GL_POINT) {
      if (( DO_OFFSET) && ctx->Polygon.OffsetPoint) {
	 VERT_Z_ADD(v[0], offset);
	 VERT_Z_ADD(v[1], offset);
	 VERT_Z_ADD(v[2], offset);
	 VERT_Z_ADD(v[3], offset);
      }
      if (DO_TWOSTENCIL && !HAVE_STENCIL_TWOSIDE && ctx->Stencil.TestTwoSide) {
         SETUP_STENCIL(facing);
         UNFILLED_QUAD( ctx, GL_POINT, e0, e1, e2, e3 );
         UNSET_STENCIL(facing);
      } else {
         UNFILLED_QUAD( ctx, GL_POINT, e0, e1, e2, e3 );
      }
   } else if (mode == GL_LINE) {
      if (DO_OFFSET && ctx->Polygon.OffsetLine) {
	 VERT_Z_ADD(v[0], offset);
	 VERT_Z_ADD(v[1], offset);
	 VERT_Z_ADD(v[2], offset);
	 VERT_Z_ADD(v[3], offset);
      }
      if (DO_TWOSTENCIL && !HAVE_STENCIL_TWOSIDE && ctx->Stencil.TestTwoSide) {
         SETUP_STENCIL(facing);
         UNFILLED_QUAD( ctx, GL_LINE, e0, e1, e2, e3 );
         UNSET_STENCIL(facing);
      } else {
         UNFILLED_QUAD( ctx, GL_LINE, e0, e1, e2, e3 );
      }
   } else {
      if (DO_OFFSET && ctx->Polygon.OffsetFill) {
	 VERT_Z_ADD(v[0], offset);
	 VERT_Z_ADD(v[1], offset);
	 VERT_Z_ADD(v[2], offset);
	 VERT_Z_ADD(v[3], offset);
      }
      RASTERIZE( GL_QUADS );
      if (DO_TWOSTENCIL && !HAVE_STENCIL_TWOSIDE && ctx->Stencil.TestTwoSide) {
         SETUP_STENCIL(facing);
         QUAD( (v[0]), (v[1]), (v[2]), (v[3]) );
         UNSET_STENCIL(facing);
      } else {
         QUAD( (v[0]), (v[1]), (v[2]), (v[3]) );
      }
   }

   if (DO_OFFSET)
   {
      VERT_SET_Z(v[0], z[0]);
      VERT_SET_Z(v[1], z[1]);
      VERT_SET_Z(v[2], z[2]);
      VERT_SET_Z(v[3], z[3]);
   }

   if (DO_TWOSIDE && facing == 1)
   {
      if (HAVE_RGBA) {
	 if (!DO_FLAT) {
	    VERT_RESTORE_RGBA( 0 );
	    VERT_RESTORE_RGBA( 1 );
	    VERT_RESTORE_RGBA( 2 );
	 }
	 VERT_RESTORE_RGBA( 3 );
	 if (HAVE_SPEC) {
	    if (!DO_FLAT) {
	       VERT_RESTORE_SPEC( 0 );
	       VERT_RESTORE_SPEC( 1 );
	       VERT_RESTORE_SPEC( 2 );
	    }
	    VERT_RESTORE_SPEC( 3 );
	 }
      }
      else {
	 if (!DO_FLAT) {
	    VERT_RESTORE_IND( 0 );
	    VERT_RESTORE_IND( 1 );
	    VERT_RESTORE_IND( 2 );
	 }
	 VERT_RESTORE_IND( 3 );
      }
   }


   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_RGBA( 0 );
	 VERT_RESTORE_RGBA( 1 );
	 VERT_RESTORE_RGBA( 2 );
	 if (HAVE_SPEC && VB->SecondaryColorPtr[0]) {
	    VERT_RESTORE_SPEC( 0 );
	    VERT_RESTORE_SPEC( 1 );
	    VERT_RESTORE_SPEC( 2 );
	 }
      }
      else {
	 VERT_RESTORE_IND( 0 );
	 VERT_RESTORE_IND( 1 );
	 VERT_RESTORE_IND( 2 );
      }
   }
}
#else
static void TAG(quadr)( GLcontext *ctx, GLuint e0,
		       GLuint e1, GLuint e2, GLuint e3 )
{
   if (DO_UNFILLED) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
      GLubyte ef1 = VB->EdgeFlag[e1];
      GLubyte ef3 = VB->EdgeFlag[e3];
      VB->EdgeFlag[e1] = 0;
      TAG(triangle)( ctx, e0, e1, e3 );
      VB->EdgeFlag[e1] = ef1;
      VB->EdgeFlag[e3] = 0;
      TAG(triangle)( ctx, e1, e2, e3 );
      VB->EdgeFlag[e3] = ef3;
   } else {
      TAG(triangle)( ctx, e0, e1, e3 );
      TAG(triangle)( ctx, e1, e2, e3 );
   }
}
#endif
#endif

#if DO_LINE
static void TAG(line)( GLcontext *ctx, GLuint e0, GLuint e1 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   VERTEX *v[2];
   LOCAL_VARS(2);

   v[0] = (VERTEX *)GET_VERTEX(e0);
   v[1] = (VERTEX *)GET_VERTEX(e1);

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_SAVE_RGBA( 0 );
	 VERT_COPY_RGBA( v[0], v[1] );
	 if (HAVE_SPEC && VB->SecondaryColorPtr[0]) {
	    VERT_SAVE_SPEC( 0 );
	    VERT_COPY_SPEC( v[0], v[1] );
	 }
      }
      else {
	 VERT_SAVE_IND( 0 );
	 VERT_COPY_IND( v[0], v[1] );
      }
   }

   LINE( v[0], v[1] );

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_RGBA( 0 );

	 if (HAVE_SPEC && VB->SecondaryColorPtr[0]) {
	    VERT_RESTORE_SPEC( 0 );
	 }
      }
      else {
	 VERT_RESTORE_IND( 0 );
      }
   }
}
#endif

#if DO_POINTS
static void TAG(points)( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   GLuint i;
   LOCAL_VARS(1);

   if (VB->Elts == 0) {
      for ( i = first ; i < last ; i++ ) {
	 if ( VB->ClipMask[i] == 0 ) {
	    VERTEX *v = (VERTEX *)GET_VERTEX(i);
	    POINT( v );
	 }
      }
   } else {
      for ( i = first ; i < last ; i++ ) {
	 GLuint e = VB->Elts[i];
	 if ( VB->ClipMask[e] == 0 ) {
	    VERTEX *v = (VERTEX *)GET_VERTEX(e);
	    POINT( v );
	 }
      }
   }
}
#endif

static void TAG(init)( void )
{
#if DO_QUAD
   TAB[IND].quad = TAG(quadr);
#endif
#if DO_TRI
   TAB[IND].triangle = TAG(triangle);
#endif
#if DO_LINE
   TAB[IND].line = TAG(line);
#endif
#if DO_POINTS
   TAB[IND].points = TAG(points);
#endif
}

#undef IND
#undef TAG

#if HAVE_RGBA
#undef VERT_SET_IND
#undef VERT_COPY_IND
#undef VERT_SAVE_IND
#undef VERT_RESTORE_IND
#if HAVE_BACK_COLORS
#undef VERT_SET_RGBA
#endif
#else
#undef VERT_SET_RGBA
#undef VERT_COPY_RGBA
#undef VERT_SAVE_RGBA
#undef VERT_RESTORE_RGBA
#if HAVE_BACK_COLORS
#undef VERT_SET_IND
#endif
#endif

#if !HAVE_SPEC
#undef VERT_SET_SPEC
#undef VERT_COPY_SPEC
#undef VERT_SAVE_SPEC
#undef VERT_RESTORE_SPEC
#if HAVE_BACK_COLORS
#undef VERT_COPY_SPEC1
#endif
#else
#if HAVE_BACK_COLORS
#undef VERT_SET_SPEC
#endif
#endif

#if !HAVE_BACK_COLORS
#undef VERT_COPY_SPEC1
#undef VERT_COPY_IND1
#undef VERT_COPY_RGBA1
#endif

#ifndef INSANE_VERTICES
#undef VERT_SET_Z
#undef VERT_Z_ADD
#endif
