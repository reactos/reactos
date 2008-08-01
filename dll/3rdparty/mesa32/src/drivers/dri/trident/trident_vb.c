/*
 * Copyright 2002 by Alan Hourihane, Sychdyn, North Wales, UK.
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
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Trident CyberBladeXP driver.
 *
 */
#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "swrast/swrast.h"
#include "tnl/t_context.h"
#include "tnl/tnl.h"

#include "trident_context.h"

#define TRIDENT_TEX1_BIT       0x1
#define TRIDENT_TEX0_BIT       0x2
#define TRIDENT_RGBA_BIT       0x4
#define TRIDENT_SPEC_BIT       0x8
#define TRIDENT_FOG_BIT        0x10
#define TRIDENT_XYZW_BIT       0x20
#define TRIDENT_PTEX_BIT       0x40
#define TRIDENT_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func	interp;
   tnl_copy_pv_func     copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_stride_shift;
   GLuint               vertex_format;
} setup_tab[TRIDENT_MAX_SETUP];

#define TINY_VERTEX_FORMAT      1
#define NOTEX_VERTEX_FORMAT     2
#define TEX0_VERTEX_FORMAT      3
#define TEX1_VERTEX_FORMAT      4
#define PROJ_TEX1_VERTEX_FORMAT 5
#define TEX2_VERTEX_FORMAT      6
#define TEX3_VERTEX_FORMAT      7
#define PROJ_TEX3_VERTEX_FORMAT 8

#define DO_XYZW (IND & TRIDENT_XYZW_BIT)
#define DO_RGBA (IND & TRIDENT_RGBA_BIT)
#define DO_SPEC (IND & TRIDENT_SPEC_BIT)
#define DO_FOG  (IND & TRIDENT_FOG_BIT)
#define DO_TEX0 (IND & TRIDENT_TEX0_BIT)
#define DO_TEX1 (IND & TRIDENT_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & TRIDENT_PTEX_BIT)

#define VERTEX tridentVertex
#define VERTEX_COLOR trident_color_t
#define LOCALVARS tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
#define GET_VIEWPORT_MAT() 		tmesa->hw_viewport
#define GET_TEXSOURCE(n)  		tmesa->tmu_source[n]
#define GET_VERTEX_FORMAT() 		tmesa->vertex_format
#define GET_VERTEX_SIZE()               tmesa->vertex_size
#define GET_VERTEX_STORE()  		tmesa->verts
#define GET_VERTEX_STRIDE_SHIFT() 	tmesa->vertex_stride_shift
#define GET_UBYTE_COLOR_STORE() 	&tmesa->UbyteColor
#define GET_UBYTE_SPEC_COLOR_STORE() 	&tmesa->UbyteSecondaryColor
			       
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

#define UNVIEWPORT_VARS                                 \
   const GLfloat dx = - tmesa->drawX - SUBPIXEL_X;	\
   const GLfloat dy = (tmesa->driDrawable->h + 	        \
		       tmesa->drawY + SUBPIXEL_Y);	\
   const GLfloat sz = 1.0 / tmesa->depth_scale

#define UNVIEWPORT_X(x)    x  +  dx;
#define UNVIEWPORT_Y(y)  - y  +  dy;
#define UNVIEWPORT_Z(z)    z  *  sz;

#define PTEX_FALLBACK()	 tridentFallback(TRIDENT_CONTEXT(ctx), TRIDENT_FALLBACK_TEXTURE, 1)

#define IMPORT_FLOAT_COLORS trident_import_float_colors
#define IMPORT_FLOAT_SPEC_COLORS trident_import_float_spec_colors

#define INTERP_VERTEX setup_tab[tmesa->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[tmesa->SetupIndex].copy_pv

/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) trident_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_TEX0_BIT|TRIDENT_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT|TRIDENT_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT|TRIDENT_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT|TRIDENT_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (TRIDENT_RGBA_BIT|TRIDENT_FOG_BIT|TRIDENT_SPEC_BIT|TRIDENT_TEX0_BIT|TRIDENT_TEX1_BIT)
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

void tridentBuildVertices( GLcontext *ctx, 
			 GLuint start, 
			 GLuint count,
			 GLuint newinputs )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT( ctx );
   GLubyte *v = ((GLubyte *)tmesa->verts + (start<<tmesa->vertex_stride_shift));
   GLuint stride = 1<<tmesa->vertex_stride_shift;

   newinputs |= tmesa->SetupNewInputs;
   tmesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[tmesa->SetupIndex].emit( ctx, start, count, v, stride );   
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= TRIDENT_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= TRIDENT_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= TRIDENT_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= TRIDENT_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= TRIDENT_FOG_BIT;

      if (tmesa->SetupIndex & TRIDENT_PTEX_BIT)
	 ind = ~0;

      ind &= tmesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );   
      }
   }
}

void tridentCheckTexSizes( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT( ctx );

   if (!setup_tab[tmesa->SetupIndex].check_tex_sizes(ctx)) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);

      /* Invalidate stored verts
       */
      tmesa->SetupNewInputs = ~0;
      tmesa->SetupIndex |= TRIDENT_PTEX_BIT;

      if (!tmesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[tmesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[tmesa->SetupIndex].copy_pv;
      }
   }
}

void tridentChooseVertexState( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = TRIDENT_XYZW_BIT|TRIDENT_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= TRIDENT_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= TRIDENT_FOG_BIT;

   if (ctx->Texture.Unit[0]._ReallyEnabled) {
      ind |= TRIDENT_TEX0_BIT;
      if (ctx->Texture.Unit[1]._ReallyEnabled) {
	 ind |= TRIDENT_TEX1_BIT;
      }
   }

   tmesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = trident_interp_extras;
      tnl->Driver.Render.CopyPV = trident_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != tmesa->vertex_format) {
      tmesa->vertex_format = setup_tab[ind].vertex_format;
      tmesa->vertex_size = setup_tab[ind].vertex_size;
      tmesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
   }
}

void tridentInitVB( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   tmesa->verts = (char *)ALIGN_MALLOC( size * 16 * 4, 32 );

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }
}

void tridentFreeVB( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);

   if (tmesa->verts) {
      ALIGN_FREE(tmesa->verts);
      tmesa->verts = 0;
   }

   if (tmesa->UbyteSecondaryColor.Ptr) {
      ALIGN_FREE((void *)tmesa->UbyteSecondaryColor.Ptr);
      tmesa->UbyteSecondaryColor.Ptr = 0;
   }

   if (tmesa->UbyteColor.Ptr) {
      ALIGN_FREE((void *)tmesa->UbyteColor.Ptr);
      tmesa->UbyteColor.Ptr = 0;
   }
}
