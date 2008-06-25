/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"

#include "mach64_context.h"
#include "mach64_vb.h"
#include "mach64_ioctl.h"
#include "mach64_tris.h"
#include "mach64_state.h"


#define MACH64_TEX1_BIT       0x1
#define MACH64_TEX0_BIT       0x2
#define MACH64_RGBA_BIT       0x4
#define MACH64_SPEC_BIT       0x8
#define MACH64_FOG_BIT        0x10
#define MACH64_XYZW_BIT       0x20
#define MACH64_PTEX_BIT       0x40
#define MACH64_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[MACH64_MAX_SETUP];

#define TINY_VERTEX_FORMAT      1
#define NOTEX_VERTEX_FORMAT     2
#define TEX0_VERTEX_FORMAT      3
#define TEX1_VERTEX_FORMAT      4
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & MACH64_XYZW_BIT)
#define DO_RGBA (IND & MACH64_RGBA_BIT)
#define DO_SPEC (IND & MACH64_SPEC_BIT)
#define DO_FOG  (IND & MACH64_FOG_BIT)
#define DO_TEX0 (IND & MACH64_TEX0_BIT)
#define DO_TEX1 (IND & MACH64_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & MACH64_PTEX_BIT)

#define VERTEX mach64Vertex
#define LOCALVARS mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
#define GET_VIEWPORT_MAT() mmesa->hw_viewport
#define GET_TEXSOURCE(n)  mmesa->tmu_source[n]
#define GET_VERTEX_FORMAT() mmesa->vertex_format
#define GET_VERTEX_STORE() mmesa->verts
#define GET_VERTEX_SIZE() mmesa->vertex_size * sizeof(GLuint)

#define HAVE_HW_VIEWPORT    0
#define HAVE_HW_DIVIDE      0
#define HAVE_RGBA_COLOR     0
#define HAVE_TINY_VERTICES  1
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  0

#define UNVIEWPORT_VARS						\
   const GLfloat dx = - (GLfloat)mmesa->drawX - SUBPIXEL_X;	\
   const GLfloat dy = (mmesa->driDrawable->h +			\
		       (GLfloat)mmesa->drawY  + SUBPIXEL_Y);	\
   const GLfloat sz = 1.0 / mmesa->depth_scale

#if MACH64_NATIVE_VTXFMT
   
#define UNVIEWPORT_X(x)    ((GLfloat)(x) / 4.0)  +  dx
#define UNVIEWPORT_Y(y)  - ((GLfloat)(y) / 4.0)  +  dy
#define UNVIEWPORT_Z(z)    (GLfloat)((z) >> 15)  *  sz

#else

#define UNVIEWPORT_X(x)    x  +  dx;
#define UNVIEWPORT_Y(y)  - y  +  dy;
#define UNVIEWPORT_Z(z)    z  *  sz;

#endif

#define PTEX_FALLBACK() FALLBACK(MACH64_CONTEXT(ctx), MACH64_FALLBACK_TEXTURE, 1)

#define IMPORT_FLOAT_COLORS mach64_import_float_colors
#define IMPORT_FLOAT_SPEC_COLORS mach64_import_float_spec_colors

#define INTERP_VERTEX setup_tab[mmesa->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[mmesa->SetupIndex].copy_pv

/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#if MACH64_NATIVE_VTXFMT

#define TAG(x) mach64_##x
#include "mach64_native_vb.c"

#else

#define TAG(x) mach64_##x
#include "tnl_dd/t_dd_vb.c"

#endif

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#if MACH64_NATIVE_VTXFMT

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT)
#define TAG(x) x##_wg
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_wgs
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_TEX0_BIT|MACH64_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|\
             MACH64_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|\
             MACH64_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT)
#define TAG(x) x##_wgf
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT|\
             MACH64_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT|\
             MACH64_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|\
             MACH64_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|\
             MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|\
             MACH64_TEX0_BIT|MACH64_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_TEX0_BIT)
#define TAG(x) x##_t0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_FOG_BIT)
#define TAG(x) x##_f
#include "mach64_native_vbtmp.h"

#define IND (MACH64_FOG_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_ft0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_FOG_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT)
#define TAG(x) x##_g
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_gs
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gt0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gst0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT)
#define TAG(x) x##_gf
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_gfs
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gft0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "mach64_native_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|\
             MACH64_TEX1_BIT)
#define TAG(x) x##_gfst0t1
#include "mach64_native_vbtmp.h"

#else

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT)
#define TAG(x) x##_wg
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_wgs
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_TEX0_BIT|MACH64_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|\
             MACH64_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|\
             MACH64_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT)
#define TAG(x) x##_wgf
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT|\
             MACH64_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT|\
             MACH64_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|\
             MACH64_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|\
             MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_XYZW_BIT|MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|\
             MACH64_TEX0_BIT|MACH64_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "mach64_vbtmp.h"

#define IND (MACH64_TEX0_BIT)
#define TAG(x) x##_t0
#include "mach64_vbtmp.h"

#define IND (MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_FOG_BIT)
#define TAG(x) x##_f
#include "mach64_vbtmp.h"

#define IND (MACH64_FOG_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_ft0
#include "mach64_vbtmp.h"

#define IND (MACH64_FOG_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT)
#define TAG(x) x##_g
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_gs
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gt0
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gst0
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT)
#define TAG(x) x##_gf
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT)
#define TAG(x) x##_gfs
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gft0
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_TEX0_BIT|MACH64_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "mach64_vbtmp.h"

#define IND (MACH64_RGBA_BIT|MACH64_FOG_BIT|MACH64_SPEC_BIT|MACH64_TEX0_BIT|\
             MACH64_TEX1_BIT)
#define TAG(x) x##_gfst0t1
#include "mach64_vbtmp.h"

#endif

static void init_setup_tab( void )
{
   init_wg();
   init_wgs();
   init_wgt0();
   init_wgt0t1();
   init_wgpt0();
   init_wgst0();
   init_wgst0t1();
   init_wgspt0();
   init_wgf();
   init_wgfs();
   init_wgft0();
   init_wgft0t1();
   init_wgfpt0();
   init_wgfst0();
   init_wgfst0t1();
   init_wgfspt0();
   init_t0();
   init_t0t1();
   init_f();
   init_ft0();
   init_ft0t1();
   init_g();
   init_gs();
   init_gt0();
   init_gt0t1();
   init_gst0();
   init_gst0t1();
   init_gf();
   init_gfs();
   init_gft0();
   init_gft0t1();
   init_gfst0();
   init_gfst0t1();
}



void mach64PrintSetupFlags( char *msg, GLuint flags )
{
   fprintf( stderr, "%s: %d %s%s%s%s%s%s%s\n",
	    msg,
	    (int)flags,
	    (flags & MACH64_XYZW_BIT)	? " xyzw," : "",
	    (flags & MACH64_RGBA_BIT)	? " rgba," : "",
	    (flags & MACH64_SPEC_BIT)	? " spec," : "",
	    (flags & MACH64_FOG_BIT)	? " fog," : "",
	    (flags & MACH64_TEX0_BIT)	? " tex-0," : "",
	    (flags & MACH64_TEX1_BIT)	? " tex-1," : "",
	    (flags & MACH64_PTEX_BIT)	? " ptex," : "");
}




void mach64CheckTexSizes( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );

   if (!setup_tab[mmesa->SetupIndex].check_tex_sizes(ctx)) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);

      /* Invalidate stored verts
       */
      mmesa->SetupNewInputs = ~0;
      mmesa->SetupIndex |= MACH64_PTEX_BIT;

      if (!mmesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[mmesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[mmesa->SetupIndex].copy_pv;
      }
   }
}

void mach64BuildVertices( GLcontext *ctx,
			GLuint start,
			GLuint count,
			GLuint newinputs )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );
   GLuint stride = mmesa->vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)mmesa->verts + (start * stride));

   newinputs |= mmesa->SetupNewInputs;
   mmesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[mmesa->SetupIndex].emit( ctx, start, count, v, stride );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= MACH64_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= MACH64_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= MACH64_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= MACH64_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= MACH64_FOG_BIT;

      if (mmesa->SetupIndex & MACH64_PTEX_BIT)
	 ind = ~0;

      ind &= mmesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );
      }
   }
}

void mach64ChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );
   GLuint ind = MACH64_XYZW_BIT|MACH64_RGBA_BIT;
   
   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= MACH64_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= MACH64_FOG_BIT;

   if (ctx->Texture._EnabledUnits) {
      ind |= MACH64_TEX0_BIT;
      if (ctx->Texture.Unit[0]._ReallyEnabled &&
	  ctx->Texture.Unit[1]._ReallyEnabled) {
	 ind |= MACH64_TEX1_BIT;
      }
   }

   mmesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = mach64_interp_extras;
      tnl->Driver.Render.CopyPV = mach64_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

#if 0
   if (MACH64_DEBUG & DEBUG_VERBOSE_MSG) {
      mach64PrintSetupFlags( __FUNCTION__, ind );
  }
#endif

   if (setup_tab[ind].vertex_format != mmesa->vertex_format) {
      FLUSH_BATCH(mmesa);
      mmesa->vertex_format = setup_tab[ind].vertex_format;
      mmesa->vertex_size = setup_tab[ind].vertex_size;
   }
}


#if 0
void mach64_emit_contiguous_verts( GLcontext *ctx,
				 GLuint start,
				 GLuint count )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint vertex_size = mmesa->vertex_size * 4;
   GLuint *dest = mach64AllocDmaLow( mmesa, (count-start) * vertex_size);
   setup_tab[mmesa->SetupIndex].emit( ctx, start, count, dest, vertex_size );
}
#endif


void mach64InitVB( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   mmesa->verts = (GLubyte *)ALIGN_MALLOC(size * 4 * 16, 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }
}


void mach64FreeVB( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   if (mmesa->verts) {
      ALIGN_FREE(mmesa->verts);
      mmesa->verts = 0;
   }
}
