/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgavb.c,v 1.15 2003/03/26 20:43:49 tsi Exp $ */

#include <stdlib.h>
#include "mgacontext.h"
#include "mgavb.h"
#include "mgatris.h"
#include "mgaioctl.h"
#include "mga_xmesa.h"

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "tnl/t_context.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/swrast.h"


#define MGA_TEX1_BIT       0x1
#define MGA_TEX0_BIT       0x2	
#define MGA_RGBA_BIT       0x4
#define MGA_SPEC_BIT       0x8
#define MGA_FOG_BIT	   0x10
#define MGA_XYZW_BIT       0x20
#define MGA_PTEX_BIT       0x40
#define MGA_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[MGA_MAX_SETUP];


#define TINY_VERTEX_FORMAT      0
#define NOTEX_VERTEX_FORMAT     0
#define TEX0_VERTEX_FORMAT      (MGA_A|MGA_S|MGA_F)
#define TEX1_VERTEX_FORMAT      (MGA_A|MGA_S|MGA_F|MGA_T2)
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & MGA_XYZW_BIT)
#define DO_RGBA (IND & MGA_RGBA_BIT)
#define DO_SPEC (IND & MGA_SPEC_BIT)
#define DO_FOG  (IND & MGA_FOG_BIT)
#define DO_TEX0 (IND & MGA_TEX0_BIT)
#define DO_TEX1 (IND & MGA_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & MGA_PTEX_BIT)

			       
#define VERTEX mgaVertex
#define VERTEX_COLOR mga_color_t
#define LOCALVARS mgaContextPtr mmesa = MGA_CONTEXT(ctx);
#define GET_VIEWPORT_MAT() mmesa->hw_viewport
#define GET_TEXSOURCE(n)  mmesa->tmu_source[n]
#define GET_VERTEX_FORMAT() mmesa->vertex_format
#define GET_VERTEX_STORE() mmesa->verts
#define GET_VERTEX_SIZE() mmesa->vertex_size * sizeof(GLuint)

#define HAVE_HW_VIEWPORT    0
#define HAVE_HW_DIVIDE      0
#define HAVE_RGBA_COLOR     0
#define HAVE_TINY_VERTICES  0
#define HAVE_NOTEX_VERTICES 0
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  0

#define UNVIEWPORT_VARS					\
   const GLfloat dx = - mmesa->drawX - SUBPIXEL_X;	\
   const GLfloat dy = (mmesa->driDrawable->h + 		\
		       mmesa->drawY + SUBPIXEL_Y);	\
   const GLfloat sz = 1.0 / mmesa->depth_scale

#define UNVIEWPORT_X(x)    x      + dx;
#define UNVIEWPORT_Y(y)  - y      + dy;
#define UNVIEWPORT_Z(z)    z * sz;

#define PTEX_FALLBACK() FALLBACK(ctx, MGA_FALLBACK_TEXTURE, 1)

#define INTERP_VERTEX setup_tab[mmesa->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[mmesa->SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) mga_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_TEX0_BIT|MGA_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT|MGA_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_TEX0_BIT|MGA_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_XYZW_BIT|MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT|MGA_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_FOG_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_FOG_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_FOG_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_FOG_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (MGA_RGBA_BIT|MGA_FOG_BIT|MGA_SPEC_BIT|MGA_TEX0_BIT|MGA_TEX1_BIT)
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




void mgaPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s: %d %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & MGA_XYZW_BIT)      ? " xyzw," : "", 
	   (flags & MGA_RGBA_BIT)     ? " rgba," : "",
	   (flags & MGA_SPEC_BIT)     ? " spec," : "",
	   (flags & MGA_FOG_BIT)      ? " fog," : "",
	   (flags & MGA_TEX0_BIT)     ? " tex-0," : "",
	   (flags & MGA_TEX1_BIT)     ? " tex-1," : "");
}


void mgaCheckTexSizes( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /*fprintf(stderr, "%s\n", __FUNCTION__);*/

   if (!setup_tab[mmesa->SetupIndex].check_tex_sizes(ctx)) {
      mmesa->SetupIndex |= MGA_PTEX_BIT;
      mmesa->SetupNewInputs = ~0;

      if (!mmesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[mmesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[mmesa->SetupIndex].copy_pv;
      }
      if (mmesa->Fallback) {
         tnl->Driver.Render.Start(ctx);
      }
   }
}


void mgaBuildVertices( GLcontext *ctx, 
		       GLuint start, 
		       GLuint count,
		       GLuint newinputs )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
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
	 ind |= MGA_RGBA_BIT;
      
      if (newinputs & VERT_BIT_COLOR1)
	 ind |= MGA_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0) 
	 ind |= MGA_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= MGA_TEX0_BIT|MGA_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= MGA_FOG_BIT;

      if (mmesa->SetupIndex & MGA_PTEX_BIT)
	 ind = ~0;

      ind &= mmesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );   
      }
   }
}


void mgaChooseVertexState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = MGA_XYZW_BIT|MGA_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) 
      ind |= MGA_SPEC_BIT;

   if (ctx->Fog.Enabled) 
      ind |= MGA_FOG_BIT;
   
   if (ctx->Texture._EnabledUnits & 0x2) {
      /* unit 1 enabled */
      if (ctx->Texture._EnabledUnits & 0x1) {
         /* unit 0 enabled */
	 ind |= MGA_TEX1_BIT|MGA_TEX0_BIT;
      }
      else {
	 ind |= MGA_TEX0_BIT;
      }
   }
   else if (ctx->Texture._EnabledUnits & 0x1) {
      /* unit 0 enabled */
      ind |= MGA_TEX0_BIT;
   }
   
   mmesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = mga_interp_extras;
      tnl->Driver.Render.CopyPV = mga_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != mmesa->vertex_format) {
      FLUSH_BATCH(mmesa);      
      mmesa->dirty |= MGA_UPLOAD_PIPE;
      mmesa->vertex_format = setup_tab[ind].vertex_format;
      mmesa->vertex_size = setup_tab[ind].vertex_size;
   }
}



void *mga_emit_contiguous_verts( GLcontext *ctx,
				 GLuint start,
				 GLuint count,
				 void *dest)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint stride = mmesa->vertex_size * 4;
   setup_tab[mmesa->SetupIndex].emit( ctx, start, count, dest, stride );
   return (void *)((char *)dest + stride * (count - start));
}
				   


void mgaInitVB( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   mmesa->verts = (GLubyte *)ALIGN_MALLOC(size * sizeof(mgaVertex), 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }

   mmesa->dirty |= MGA_UPLOAD_PIPE;
   mmesa->vertex_format = setup_tab[0].vertex_format;
   mmesa->vertex_size = setup_tab[0].vertex_size;
}


void mgaFreeVB( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   if (mmesa->verts) {
      ALIGN_FREE(mmesa->verts);
      mmesa->verts = 0;
   }
}

