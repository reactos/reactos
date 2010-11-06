/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#define CLIP_DOTPROD(K, A, B, C, D) X(K)*A + Y(K)*B + Z(K)*C + W(K)*D

#define POLY_CLIP( PLANE_BIT, A, B, C, D )				\
do {									\
   if (mask & PLANE_BIT) {						\
      GLuint idxPrev = inlist[0];					\
      GLfloat dpPrev = CLIP_DOTPROD(idxPrev, A, B, C, D );		\
      GLuint outcount = 0;						\
      GLuint i;								\
									\
      inlist[n] = inlist[0]; /* prevent rotation of vertices */		\
      for (i = 1; i <= n; i++) {					\
	 GLuint idx = inlist[i];					\
	 GLfloat dp = CLIP_DOTPROD(idx, A, B, C, D );			\
									\
	 if (!IS_NEGATIVE(dpPrev)) {					\
	    outlist[outcount++] = idxPrev;				\
	 }								\
									\
	 if (DIFFERENT_SIGNS(dp, dpPrev)) {				\
	    if (IS_NEGATIVE(dp)) {					\
	       /* Going out of bounds.  Avoid division by zero as we	\
		* know dp != dpPrev from DIFFERENT_SIGNS, above.	\
		*/							\
	       GLfloat t = dp / (dp - dpPrev);				\
               INTERP_4F( t, coord[newvert], coord[idx], coord[idxPrev]); \
      	       interp( ctx, t, newvert, idx, idxPrev, GL_TRUE );	\
	    } else {							\
	       /* Coming back in.					\
		*/							\
	       GLfloat t = dpPrev / (dpPrev - dp);			\
               INTERP_4F( t, coord[newvert], coord[idxPrev], coord[idx]); \
	       interp( ctx, t, newvert, idxPrev, idx, GL_FALSE );	\
	    }								\
            outlist[outcount++] = newvert++;				\
	 }								\
									\
	 idxPrev = idx;							\
	 dpPrev = dp;							\
      }									\
									\
      if (outcount < 3)							\
	 return;							\
									\
      {									\
	 GLuint *tmp = inlist;						\
	 inlist = outlist;						\
	 outlist = tmp;							\
	 n = outcount;							\
      }									\
   }									\
} while (0)


#define LINE_CLIP(PLANE_BIT, A, B, C, D )				\
do {									\
   if (mask & PLANE_BIT) {						\
      const GLfloat dp0 = CLIP_DOTPROD( v0, A, B, C, D );		\
      const GLfloat dp1 = CLIP_DOTPROD( v1, A, B, C, D );		\
      const GLboolean neg_dp0 = IS_NEGATIVE(dp0);			\
      const GLboolean neg_dp1 = IS_NEGATIVE(dp1);			\
      									\
      /* For regular clipping, we know from the clipmask that one	\
       * (or both) of these must be negative (otherwise we wouldn't	\
       * be here).							\
       * For userclip, there is only a single bit for all active	\
       * planes, so we can end up here when there is nothing to do,	\
       * hence the second IS_NEGATIVE() test:				\
       */								\
      if (neg_dp0 && neg_dp1)						\
         return; /* both vertices outside clip plane: discard */	\
									\
      if (neg_dp1) {							\
	 GLfloat t = dp1 / (dp1 - dp0);					\
	 if (t > t1) t1 = t;						\
      } else if (neg_dp0) {						\
	 GLfloat t = dp0 / (dp0 - dp1);					\
	 if (t > t0) t0 = t;						\
      }									\
      if (t0 + t1 >= 1.0)						\
	 return; /* discard */						\
   }									\
} while (0)



/* Clip a line against the viewport and user clip planes.
 */
static INLINE void
TAG(clip_line)( GLcontext *ctx, GLuint v0, GLuint v1, GLubyte mask )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   tnl_interp_func interp = tnl->Driver.Render.Interp;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint newvert = VB->Count;
   GLfloat t0 = 0;
   GLfloat t1 = 0;
   GLuint p;
   const GLuint v0_orig = v0;

   if (mask & 0x3f) {
      LINE_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
      LINE_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
      LINE_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
      LINE_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
      LINE_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
      LINE_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );
   }

   if (mask & CLIP_USER_BIT) {
      for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
	 if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
            const GLfloat a = ctx->Transform._ClipUserPlane[p][0];
            const GLfloat b = ctx->Transform._ClipUserPlane[p][1];
            const GLfloat c = ctx->Transform._ClipUserPlane[p][2];
            const GLfloat d = ctx->Transform._ClipUserPlane[p][3];
	    LINE_CLIP( CLIP_USER_BIT, a, b, c, d );
	 }
      }
   }

   if (VB->ClipMask[v0]) {
      INTERP_4F( t0, coord[newvert], coord[v0], coord[v1] );
      interp( ctx, t0, newvert, v0, v1, GL_FALSE );
      v0 = newvert;
      newvert++;
   }
   else {
      ASSERT(t0 == 0.0);
   }

   /* Note: we need to use vertex v0_orig when computing the new
    * interpolated/clipped vertex position, not the current v0 which
    * may have got set when we clipped the other end of the line!
    */
   if (VB->ClipMask[v1]) {
      INTERP_4F( t1, coord[newvert], coord[v1], coord[v0_orig] );
      interp( ctx, t1, newvert, v1, v0_orig, GL_FALSE );

      if (ctx->Light.ShadeModel == GL_FLAT)
	 tnl->Driver.Render.CopyPV( ctx, newvert, v1 );

      v1 = newvert;

      newvert++;
   }
   else {
      ASSERT(t1 == 0.0);
   }

   tnl->Driver.Render.ClippedLine( ctx, v0, v1 );
}


/* Clip a triangle against the viewport and user clip planes.
 */
static INLINE void
TAG(clip_tri)( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLubyte mask )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   tnl_interp_func interp = tnl->Driver.Render.Interp;
   GLuint newvert = VB->Count;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint pv = v2;
   GLuint vlist[2][MAX_CLIPPED_VERTICES];
   GLuint *inlist = vlist[0], *outlist = vlist[1];
   GLuint p;
   GLuint n = 3;

   ASSIGN_3V(inlist, v2, v0, v1 ); /* pv rotated to slot zero */

   if (mask & 0x3f) {
      POLY_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
      POLY_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
      POLY_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
      POLY_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
      POLY_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
      POLY_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );
   }

   if (mask & CLIP_USER_BIT) {
      for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
         if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
            const GLfloat a = ctx->Transform._ClipUserPlane[p][0];
            const GLfloat b = ctx->Transform._ClipUserPlane[p][1];
            const GLfloat c = ctx->Transform._ClipUserPlane[p][2];
            const GLfloat d = ctx->Transform._ClipUserPlane[p][3];
            POLY_CLIP( CLIP_USER_BIT, a, b, c, d );
         }
      }
   }

   if (ctx->Light.ShadeModel == GL_FLAT) {
      if (pv != inlist[0]) {
	 ASSERT( inlist[0] >= VB->Count );
	 tnl->Driver.Render.CopyPV( ctx, inlist[0], pv );
      }
   }

   tnl->Driver.Render.ClippedPolygon( ctx, inlist, n );
}


/* Clip a quad against the viewport and user clip planes.
 */
static INLINE void
TAG(clip_quad)( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3,
                GLubyte mask )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   tnl_interp_func interp = tnl->Driver.Render.Interp;
   GLuint newvert = VB->Count;
   GLfloat (*coord)[4] = VB->ClipPtr->data;
   GLuint pv = v3;
   GLuint vlist[2][MAX_CLIPPED_VERTICES];
   GLuint *inlist = vlist[0], *outlist = vlist[1];
   GLuint p;
   GLuint n = 4;

   ASSIGN_4V(inlist, v3, v0, v1, v2 ); /* pv rotated to slot zero */

   if (mask & 0x3f) {
      POLY_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
      POLY_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
      POLY_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
      POLY_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
      POLY_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
      POLY_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );
   }

   if (mask & CLIP_USER_BIT) {
      for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
	 if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
            const GLfloat a = ctx->Transform._ClipUserPlane[p][0];
            const GLfloat b = ctx->Transform._ClipUserPlane[p][1];
            const GLfloat c = ctx->Transform._ClipUserPlane[p][2];
            const GLfloat d = ctx->Transform._ClipUserPlane[p][3];
	    POLY_CLIP( CLIP_USER_BIT, a, b, c, d );
	 }
      }
   }

   if (ctx->Light.ShadeModel == GL_FLAT) {
      if (pv != inlist[0]) {
	 ASSERT( inlist[0] >= VB->Count );
	 tnl->Driver.Render.CopyPV( ctx, inlist[0], pv );
      }
   }

   tnl->Driver.Render.ClippedPolygon( ctx, inlist, n );
}

#undef W
#undef Z
#undef Y
#undef X
#undef SIZE
#undef TAG
#undef POLY_CLIP
#undef LINE_CLIP
