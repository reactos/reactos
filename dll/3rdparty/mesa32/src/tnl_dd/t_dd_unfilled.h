/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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

#if HAVE_RGBA
#define VERT_SET_IND( v, c )
#define VERT_COPY_IND( v0, v1 )
#define VERT_SAVE_IND( idx )
#define VERT_RESTORE_IND( idx )
#endif

#if !HAVE_SPEC
#define VERT_SET_SPEC( v, c )
#define VERT_COPY_SPEC( v0, v1 )
#define VERT_SAVE_SPEC( idx )
#define VERT_RESTORE_SPEC( idx )
#endif

static void TAG(unfilled_tri)( GLcontext *ctx,
			       GLenum mode,
			       GLuint e0, GLuint e1, GLuint e2 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ef = VB->EdgeFlag;
   VERTEX *v[3];
   LOCAL_VARS(3);

   v[0] = (VERTEX *)GET_VERTEX(e0);
   v[1] = (VERTEX *)GET_VERTEX(e1);
   v[2] = (VERTEX *)GET_VERTEX(e2);

   if (ctx->Light.ShadeModel == GL_FLAT && HAVE_HW_FLATSHADE) {
      if (HAVE_RGBA) {
	 VERT_SAVE_RGBA(0);
	 VERT_SAVE_RGBA(1);
	 VERT_COPY_RGBA(v[0], v[2]);
	 VERT_COPY_RGBA(v[1], v[2]);

	 if (HAVE_SPEC) {
	    VERT_SAVE_SPEC(0);
	    VERT_SAVE_SPEC(1);
	    VERT_COPY_SPEC(v[0], v[2]);
	    VERT_COPY_SPEC(v[1], v[2]);
	 }
      } else {
	 VERT_SAVE_IND(0);
	 VERT_SAVE_IND(1);
	 VERT_COPY_IND(v[0], v[2]);
	 VERT_COPY_IND(v[1], v[2]);
      }
   }

/*     fprintf(stderr, "%s %s %d %d %d\n", __FUNCTION__, */
/*  	   _mesa_lookup_enum_by_nr( mode ), */
/*  	   ef[e0], ef[e1], ef[e2]); */

   if (mode == GL_POINT) {
      RASTERIZE(GL_POINTS);
      if (ef[e0]) POINT( v[0] );
      if (ef[e1]) POINT( v[1] );
      if (ef[e2]) POINT( v[2] );
   }
   else {
      RASTERIZE(GL_LINES);
      if (RENDER_PRIMITIVE == GL_POLYGON) {
	 if (ef[e2]) LINE( v[2], v[0] );
	 if (ef[e0]) LINE( v[0], v[1] );
	 if (ef[e1]) LINE( v[1], v[2] );
      }
      else {
	 if (ef[e0]) LINE( v[0], v[1] );
	 if (ef[e1]) LINE( v[1], v[2] );
	 if (ef[e2]) LINE( v[2], v[0] );
      }
   }

   if (ctx->Light.ShadeModel == GL_FLAT && HAVE_HW_FLATSHADE) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_RGBA(0);
	 VERT_RESTORE_RGBA(1);

	 if (HAVE_SPEC) {
	    VERT_RESTORE_SPEC(0);
	    VERT_RESTORE_SPEC(1);
	 }
      } else {
	 VERT_RESTORE_IND(0);
	 VERT_RESTORE_IND(1);
      }
   }
}


static void TAG(unfilled_quad)( GLcontext *ctx,
				GLenum mode,
				GLuint e0, GLuint e1,
				GLuint e2, GLuint e3 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ef = VB->EdgeFlag;
   VERTEX *v[4];
   LOCAL_VARS(4);

   v[0] = (VERTEX *)GET_VERTEX(e0);
   v[1] = (VERTEX *)GET_VERTEX(e1);
   v[2] = (VERTEX *)GET_VERTEX(e2);
   v[3] = (VERTEX *)GET_VERTEX(e3);

   /* Hardware flatshading breaks down here.  If the hardware doesn't
    * support flatshading, this will already have been done:
    */
   if (ctx->Light.ShadeModel == GL_FLAT && HAVE_HW_FLATSHADE) {
      if (HAVE_RGBA) {
	 VERT_SAVE_RGBA(0);
	 VERT_SAVE_RGBA(1);
	 VERT_SAVE_RGBA(2);
	 VERT_COPY_RGBA(v[0], v[3]);
	 VERT_COPY_RGBA(v[1], v[3]);
	 VERT_COPY_RGBA(v[2], v[3]);

	 if (HAVE_SPEC) {
	    VERT_SAVE_SPEC(0);
	    VERT_SAVE_SPEC(1);
	    VERT_SAVE_SPEC(2);
	    VERT_COPY_SPEC(v[0], v[3]);
	    VERT_COPY_SPEC(v[1], v[3]);
	    VERT_COPY_SPEC(v[2], v[3]);
	 }
      } else {
	 VERT_SAVE_IND(0);
	 VERT_SAVE_IND(1);
	 VERT_SAVE_IND(2);
	 VERT_COPY_IND(v[0], v[3]);
	 VERT_COPY_IND(v[1], v[3]);
	 VERT_COPY_IND(v[2], v[3]);
      }
   }

   if (mode == GL_POINT) {
      RASTERIZE(GL_POINTS);
      if (ef[e0]) POINT( v[0] );
      if (ef[e1]) POINT( v[1] );
      if (ef[e2]) POINT( v[2] );
      if (ef[e3]) POINT( v[3] );
   }
   else {
      RASTERIZE(GL_LINES);
      if (ef[e0]) LINE( v[0], v[1] );
      if (ef[e1]) LINE( v[1], v[2] );
      if (ef[e2]) LINE( v[2], v[3] );
      if (ef[e3]) LINE( v[3], v[0] );
   }

   if (ctx->Light.ShadeModel == GL_FLAT && HAVE_HW_FLATSHADE) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_RGBA(0);
	 VERT_RESTORE_RGBA(1);
	 VERT_RESTORE_RGBA(2);

	 if (HAVE_SPEC) {
	    VERT_RESTORE_SPEC(0);
	    VERT_RESTORE_SPEC(1);
	    VERT_RESTORE_SPEC(2);
	 }
      } else {
	 VERT_RESTORE_IND(0);
	 VERT_RESTORE_IND(1);
	 VERT_RESTORE_IND(2);
      }
   }
}


#if HAVE_RGBA
#undef VERT_SET_IND
#undef VERT_COPY_IND
#undef VERT_SAVE_IND
#undef VERT_RESTORE_IND
#endif

#if !HAVE_SPEC
#undef VERT_SET_SPEC
#undef VERT_COPY_SPEC
#undef VERT_SAVE_SPEC
#undef VERT_RESTORE_SPEC
#endif

#undef TAG
