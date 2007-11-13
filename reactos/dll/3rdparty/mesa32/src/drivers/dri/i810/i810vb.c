/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810vb.c,v 1.13 2003/03/26 20:43:48 tsi Exp $ */
 

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810vb.h"
#include "i810ioctl.h"
#include "i810tris.h"
#include "i810state.h"


#define I810_TEX1_BIT       0x1
#define I810_TEX0_BIT       0x2
#define I810_RGBA_BIT       0x4
#define I810_SPEC_BIT       0x8
#define I810_FOG_BIT	    0x10
#define I810_XYZW_BIT       0x20
#define I810_PTEX_BIT       0x40
#define I810_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[I810_MAX_SETUP];

#define TINY_VERTEX_FORMAT (GFX_OP_VERTEX_FMT |		\
		            VF_TEXCOORD_COUNT_0 |	\
		            VF_RGBA_ENABLE |		\
		            VF_XYZ)

#define NOTEX_VERTEX_FORMAT (GFX_OP_VERTEX_FMT |	\
		             VF_TEXCOORD_COUNT_0 |	\
		             VF_SPEC_FOG_ENABLE |	\
		             VF_RGBA_ENABLE |		\
		             VF_XYZW)

#define TEX0_VERTEX_FORMAT (GFX_OP_VERTEX_FMT |		\
		            VF_TEXCOORD_COUNT_1 |	\
		            VF_SPEC_FOG_ENABLE |	\
		            VF_RGBA_ENABLE |		\
		            VF_XYZW)

#define TEX1_VERTEX_FORMAT (GFX_OP_VERTEX_FMT |		\
		            VF_TEXCOORD_COUNT_2 |	\
		            VF_SPEC_FOG_ENABLE |	\
		            VF_RGBA_ENABLE |		\
		            VF_XYZW)

#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & I810_XYZW_BIT)
#define DO_RGBA (IND & I810_RGBA_BIT)
#define DO_SPEC (IND & I810_SPEC_BIT)
#define DO_FOG  (IND & I810_FOG_BIT)
#define DO_TEX0 (IND & I810_TEX0_BIT)
#define DO_TEX1 (IND & I810_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & I810_PTEX_BIT)

#define VERTEX i810Vertex
#define VERTEX_COLOR i810_color_t
#define GET_VIEWPORT_MAT() I810_CONTEXT(ctx)->ViewportMatrix.m
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() I810_CONTEXT(ctx)->Setup[I810_CTXREG_VF]
#define GET_VERTEX_STORE() I810_CONTEXT(ctx)->verts
#define GET_VERTEX_SIZE() I810_CONTEXT(ctx)->vertex_size * sizeof(GLuint)
#define INVALIDATE_STORED_VERTICES()

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

#define UNVIEWPORT_VARS  GLfloat h = I810_CONTEXT(ctx)->driDrawable->h
#define UNVIEWPORT_X(x)  x - SUBPIXEL_X
#define UNVIEWPORT_Y(y)  - y + h + SUBPIXEL_Y
#define UNVIEWPORT_Z(z)  z * (float)0xffff

#define PTEX_FALLBACK() FALLBACK(I810_CONTEXT(ctx), I810_FALLBACK_TEXTURE, 1)

#define INTERP_VERTEX setup_tab[I810_CONTEXT(ctx)->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[I810_CONTEXT(ctx)->SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) i810_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#define IND (I810_XYZW_BIT|I810_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_TEX0_BIT|I810_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_SPEC_BIT|I810_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_SPEC_BIT|I810_TEX0_BIT|\
             I810_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_SPEC_BIT|I810_TEX0_BIT|\
             I810_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_TEX0_BIT|\
             I810_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_TEX0_BIT|\
             I810_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT|\
             I810_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT|\
             I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_XYZW_BIT|I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT|\
             I810_TEX0_BIT|I810_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_FOG_BIT|I810_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_FOG_BIT|I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_SPEC_BIT|I810_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_SPEC_BIT|I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_FOG_BIT|I810_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_FOG_BIT|I810_TEX0_BIT|I810_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT|I810_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I810_RGBA_BIT|I810_FOG_BIT|I810_SPEC_BIT|I810_TEX0_BIT|\
             I810_TEX1_BIT)
#define TAG(x) x##_gfst0t1
#include "tnl_dd/t_dd_vbtmp.h"


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



static void i810PrintSetupFlags(const char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & I810_XYZW_BIT)      ? " xyzw," : "",
	   (flags & I810_RGBA_BIT)     ? " rgba," : "",
	   (flags & I810_SPEC_BIT)     ? " spec," : "",
	   (flags & I810_FOG_BIT)      ? " fog," : "",
	   (flags & I810_TEX0_BIT)     ? " tex-0," : "",
	   (flags & I810_TEX1_BIT)     ? " tex-1," : "");
}



void i810CheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   i810ContextPtr imesa = I810_CONTEXT( ctx );

   if (!setup_tab[imesa->SetupIndex].check_tex_sizes(ctx)) {
      /* Invalidate stored verts
       */
      imesa->SetupNewInputs = ~0;
      imesa->SetupIndex |= I810_PTEX_BIT;

      if (!imesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[imesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[imesa->SetupIndex].copy_pv;
      }
      if (imesa->Fallback) {
         tnl->Driver.Render.Start(ctx);
      }
   }
}

void i810BuildVertices( GLcontext *ctx,
			GLuint start,
			GLuint count,
			GLuint newinputs )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint stride = imesa->vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)imesa->verts + (start * stride));

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   newinputs |= imesa->SetupNewInputs;
   imesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[imesa->SetupIndex].emit( ctx, start, count, v, stride );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= I810_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= I810_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= I810_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= I810_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= I810_FOG_BIT;

      if (imesa->SetupIndex & I810_PTEX_BIT)
	 ind = ~0;

      ind &= imesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );
      }
   }
}

void i810ChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint ind = I810_XYZW_BIT|I810_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= I810_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= I810_FOG_BIT;

   if (ctx->Texture._EnabledUnits & 0x2)
      /* unit 1 enabled */
      ind |= I810_TEX1_BIT|I810_TEX0_BIT;
   else if (ctx->Texture._EnabledUnits & 0x1)
      /* unit 0 enabled */
      ind |= I810_TEX0_BIT;

   imesa->SetupIndex = ind;

   if (I810_DEBUG & (DEBUG_VERTS|DEBUG_STATE))
      i810PrintSetupFlags( __FUNCTION__, ind );

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = i810_interp_extras;
      tnl->Driver.Render.CopyPV = i810_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != imesa->Setup[I810_CTXREG_VF]) {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->Setup[I810_CTXREG_VF] = setup_tab[ind].vertex_format;
      imesa->vertex_size = setup_tab[ind].vertex_size;
   }
}



void *i810_emit_contiguous_verts( GLcontext *ctx,
				  GLuint start,
				  GLuint count,
				  void *dest )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint stride = imesa->vertex_size * 4;
   setup_tab[imesa->SetupIndex].emit( ctx, start, count, dest, stride );
   return (void *)((char *)dest + stride * (count - start));
}



void i810InitVB( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   imesa->verts = (GLubyte *)ALIGN_MALLOC(size * 4 * 16, 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }
}


void i810FreeVB( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   if (imesa->verts) {
      ALIGN_FREE(imesa->verts);
      imesa->verts = 0;
   }
}
