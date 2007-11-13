/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_vb.c,v 1.4 2003/03/26 20:43:48 tsi Exp $ */
/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *           Keith Whitwell, <keith@tungstengraphics.com>
 *
 * 3DLabs Gamma driver.
 */
 
#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/tnl.h"

#include "gamma_context.h"
#include "gamma_vb.h"
#include "gamma_tris.h"


#define GAMMA_TEX0_BIT       0x1	
#define GAMMA_RGBA_BIT       0x2
#define GAMMA_XYZW_BIT       0x4
#define GAMMA_PTEX_BIT       0x8
#define GAMMA_FOG_BIT        0x10
#define GAMMA_SPEC_BIT       0x20
#define GAMMA_MAX_SETUP      0x40

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[GAMMA_MAX_SETUP];

#define TINY_VERTEX_FORMAT      1
#define NOTEX_VERTEX_FORMAT     2
#define TEX0_VERTEX_FORMAT      3
#define TEX1_VERTEX_FORMAT      0
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & GAMMA_XYZW_BIT)
#define DO_RGBA (IND & GAMMA_RGBA_BIT)
#define DO_SPEC (IND & GAMMA_SPEC_BIT)
#define DO_FOG  (IND & GAMMA_FOG_BIT)
#define DO_TEX0 (IND & GAMMA_TEX0_BIT)
#define DO_TEX1 0
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & GAMMA_PTEX_BIT)
			       
#define VERTEX gammaVertex
#define VERTEX_COLOR gamma_color_t
#define GET_VIEWPORT_MAT() 0
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() GAMMA_CONTEXT(ctx)->vertex_format
#define GET_VERTEX_STORE() GAMMA_CONTEXT(ctx)->verts
#define GET_VERTEX_SIZE() GAMMA_CONTEXT(ctx)->vertex_size * sizeof(GLuint)
#define INVALIDATE_STORED_VERTICES()

#define HAVE_HW_VIEWPORT    1
#define HAVE_HW_DIVIDE      1
#define HAVE_RGBA_COLOR     0 	/* we're BGRA */
#define HAVE_TINY_VERTICES  1
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  0
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  1

#define PTEX_FALLBACK()		/* never needed */

#define INTERP_VERTEX setup_tab[GAMMA_CONTEXT(ctx)->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[GAMMA_CONTEXT(ctx)->SetupIndex].copy_pv



/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) gamma_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_TEX0_BIT|GAMMA_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_SPEC_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_SPEC_BIT|GAMMA_TEX0_BIT|\
             GAMMA_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_TEX0_BIT|\
             GAMMA_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_SPEC_BIT|\
             GAMMA_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_XYZW_BIT|GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_SPEC_BIT|\
             GAMMA_TEX0_BIT|GAMMA_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_FOG_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_SPEC_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (GAMMA_RGBA_BIT|GAMMA_FOG_BIT|GAMMA_SPEC_BIT|GAMMA_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

static void init_setup_tab( void )
{
   init_wg();
   init_wgs();
   init_wgt0();
   init_wgpt0();
   init_wgst0();
   init_wgspt0();
   init_wgf();
   init_wgfs();
   init_wgft0();
   init_wgfpt0();
   init_wgfst0();
   init_wgfspt0();
   init_t0();
   init_f();
   init_ft0();
   init_g();
   init_gs();
   init_gt0();
   init_gst0();
   init_gf();
   init_gfs();
   init_gft0();
   init_gfst0();
}

void gammaCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   gammaContextPtr gmesa = GAMMA_CONTEXT( ctx );

   if (!setup_tab[gmesa->SetupIndex].check_tex_sizes(ctx)) {
      /* Invalidate stored verts
       */
      gmesa->SetupNewInputs = ~0;
      gmesa->SetupIndex |= GAMMA_PTEX_BIT;

      if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[gmesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[gmesa->SetupIndex].copy_pv;
      }
   }
}

void gammaBuildVertices( GLcontext *ctx, 
			 GLuint start, 
			 GLuint count,
			 GLuint newinputs )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT( ctx );
   GLuint stride = gmesa->vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)gmesa->verts + (start * stride));

   newinputs |= gmesa->SetupNewInputs;
   gmesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[gmesa->SetupIndex].emit( ctx, start, count, v, stride );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= GAMMA_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= GAMMA_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= GAMMA_TEX0_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= GAMMA_FOG_BIT;

      if (gmesa->SetupIndex & GAMMA_PTEX_BIT)
	 ind = ~0;

      ind &= gmesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );
      }
   }
}

void gammaChooseVertexState( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = GAMMA_XYZW_BIT|GAMMA_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= GAMMA_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= GAMMA_FOG_BIT;

   if (ctx->Texture.Unit[0]._ReallyEnabled) {
      _tnl_need_projected_coords( ctx, GL_FALSE );
      ind |= GAMMA_TEX0_BIT;
   } else
      _tnl_need_projected_coords( ctx, GL_FALSE );

   gmesa->SetupIndex = ind;

   if (setup_tab[ind].vertex_format != gmesa->vertex_format) {
      gmesa->vertex_format = setup_tab[ind].vertex_format;
      gmesa->vertex_size = setup_tab[ind].vertex_size;
   }

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = gamma_interp_extras;
      tnl->Driver.Render.CopyPV = gamma_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }
}


void gammaInitVB( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   gmesa->verts = (GLubyte *)ALIGN_MALLOC(size * 4 * 16, 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
	 gmesa->vertex_size = 16; /* FIXME - only one vertex setup */
      }
   }
}


void gammaFreeVB( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   if (gmesa->verts) {
      ALIGN_FREE(gmesa->verts);
      gmesa->verts = 0;
   }
}
