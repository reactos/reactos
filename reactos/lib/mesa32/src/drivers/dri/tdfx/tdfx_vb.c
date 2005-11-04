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
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_vb.c,v 1.3 2002/10/30 12:52:01 alanh Exp $ */
 
#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "math/m_translate.h"
#include "swrast_setup/swrast_setup.h"

#include "tdfx_context.h"
#include "tdfx_vb.h"
#include "tdfx_tris.h"
#include "tdfx_state.h"
#include "tdfx_render.h"

static void copy_pv( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   tdfxVertex *dst = fxMesa->verts + edst;
   tdfxVertex *src = fxMesa->verts + esrc;
   *(GLuint *)&dst->color = *(GLuint *)&src->color;
}

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void * );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_format;
} setup_tab[TDFX_MAX_SETUP];




#define GET_COLOR(ptr, idx) ((ptr)->data[idx])


static void interp_extras( GLcontext *ctx,
			   GLfloat t,
			   GLuint dst, GLuint out, GLuint in,
			   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /*fprintf(stderr, "%s\n", __FUNCTION__);*/

   if (VB->ColorPtr[1]) {
      INTERP_4F( t,
		    GET_COLOR(VB->ColorPtr[1], dst),
		    GET_COLOR(VB->ColorPtr[1], out),
		    GET_COLOR(VB->ColorPtr[1], in) );
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   setup_tab[TDFX_CONTEXT(ctx)->SetupIndex].interp(ctx, t, dst, out, in,
						   force_boundary);
}

static void copy_pv_extras( GLcontext *ctx, GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
	 COPY_4FV( GET_COLOR(VB->ColorPtr[1], dst), 
		     GET_COLOR(VB->ColorPtr[1], src) );
   }

   setup_tab[TDFX_CONTEXT(ctx)->SetupIndex].copy_pv(ctx, dst, src);
}



#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT)
#define TAG(x) x##_wg
#include "tdfx_vbtmp.h"

/* Special for tdfx: fog requires w
 */
#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT)
#define TAG(x) x##_wg_fog
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT|\
             TDFX_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tdfx_vbtmp.h"

#define IND (TDFX_RGBA_BIT)
#define TAG(x) x##_g
#include "tdfx_vbtmp.h"

#define IND (TDFX_TEX0_BIT)
#define TAG(x) x##_t0
#include "tdfx_vbtmp.h"

#define IND (TDFX_TEX0_BIT|TDFX_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tdfx_vbtmp.h"

#define IND (TDFX_RGBA_BIT|TDFX_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tdfx_vbtmp.h"

#define IND (TDFX_RGBA_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tdfx_vbtmp.h"


/* fogc { */
#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_FOGC_BIT)
#define TAG(x) x##_wgf
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_FOGC_BIT)
#define TAG(x) x##_wgt0f
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT|TDFX_FOGC_BIT)
#define TAG(x) x##_wgt0t1f
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_PTEX_BIT|TDFX_FOGC_BIT)
#define TAG(x) x##_wgpt0f
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT|\
             TDFX_PTEX_BIT|TDFX_FOGC_BIT)
#define TAG(x) x##_wgpt0t1f
#include "tdfx_vbtmp.h"
/* fogc } */


static void init_setup_tab( void )
{
   init_wg();
   init_wg_fog();
   init_wgt0();
   init_wgt0t1();
   init_wgpt0();
   init_wgpt0t1();

   init_g();
   init_t0();
   init_t0t1();
   init_gt0();
   init_gt0t1();

   /* fogcoord */
   init_wgf();
   init_wgt0f();
   init_wgt0t1f();
   init_wgpt0f();
   init_wgpt0t1f();
}


void tdfxPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & TDFX_XYZ_BIT)     ? " xyz," : "", 
	   (flags & TDFX_W_BIT)     ? " w," : "", 
	   (flags & TDFX_RGBA_BIT)     ? " rgba," : "",
	   (flags & TDFX_TEX0_BIT)     ? " tex-0," : "",
	   (flags & TDFX_TEX1_BIT)     ? " tex-1," : "",
	   (flags & TDFX_FOGC_BIT)     ? " fogc," : "");
}



void tdfxCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   if (!setup_tab[fxMesa->SetupIndex].check_tex_sizes(ctx)) {
      GLuint ind = fxMesa->SetupIndex |= (TDFX_PTEX_BIT|TDFX_RGBA_BIT);

      /* Tdfx handles projective textures nicely; just have to change
       * up to the new vertex format.
       */
      if (setup_tab[ind].vertex_format != fxMesa->vertexFormat) {
	 FLUSH_BATCH(fxMesa);
	 fxMesa->dirty |= TDFX_UPLOAD_VERTEX_LAYOUT;      
	 fxMesa->vertexFormat = setup_tab[ind].vertex_format;

	 /* This is required as we have just changed the vertex
	  * format, so the interp and copy routines must also change.
	  * In the unfilled and twosided cases we are using the
	  * swrast_setup ones anyway, so leave them in place.
	  */
	 if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	    tnl->Driver.Render.Interp = setup_tab[fxMesa->SetupIndex].interp;
	    tnl->Driver.Render.CopyPV = setup_tab[fxMesa->SetupIndex].copy_pv;
	 }
      }
   }
}


void tdfxBuildVertices( GLcontext *ctx, GLuint start, GLuint end,
			GLuint newinputs )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   tdfxVertex *v = fxMesa->verts + start;

   newinputs |= fxMesa->SetupNewInputs;
   fxMesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[fxMesa->SetupIndex].emit( ctx, start, end, v );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= TDFX_RGBA_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= TDFX_FOGC_BIT;
      
      if (newinputs & VERT_BIT_TEX0)
	 ind |= TDFX_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= TDFX_TEX0_BIT|TDFX_TEX1_BIT;

      if (fxMesa->SetupIndex & TDFX_PTEX_BIT)
	 ind = ~0;

      ind &= fxMesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, end, v );
      }
   }
}


void tdfxChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GLuint ind = TDFX_XYZ_BIT|TDFX_RGBA_BIT;

   fxMesa->tmu_source[0] = 0;
   fxMesa->tmu_source[1] = 1;

   if (ctx->Texture._EnabledUnits & 0x2) {
      if (ctx->Texture._EnabledUnits & 0x1) {
         ind |= TDFX_TEX1_BIT;
      }
      ind |= TDFX_W_BIT|TDFX_TEX0_BIT;
      fxMesa->tmu_source[0] = 1;
      fxMesa->tmu_source[1] = 0;
   } else if (ctx->Texture._EnabledUnits & 0x1) {
      /* unit 0 enabled */
      ind |= TDFX_W_BIT|TDFX_TEX0_BIT;
   } else if (fxMesa->Fog.Mode != GR_FOG_DISABLE) {
      ind |= TDFX_W_BIT;
   }

   if (fxMesa->Fog.Mode == GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT) {
      ind |= TDFX_FOGC_BIT;
   }

   fxMesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = interp_extras;
      tnl->Driver.Render.CopyPV = copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != fxMesa->vertexFormat) {
      FLUSH_BATCH(fxMesa);
      fxMesa->dirty |= TDFX_UPLOAD_VERTEX_LAYOUT;      
      fxMesa->vertexFormat = setup_tab[ind].vertex_format;
   }
}



void tdfxInitVB( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   static int firsttime = 1;
   if (firsttime) {
      init_setup_tab();
      firsttime = 0;
   }

   fxMesa->verts = ALIGN_MALLOC(size * sizeof(tdfxVertex), 32);
   fxMesa->vertexFormat = TDFX_LAYOUT_TINY;
   fxMesa->SetupIndex = TDFX_XYZ_BIT|TDFX_RGBA_BIT;
}


void tdfxFreeVB( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   if (fxMesa->verts) {
      ALIGN_FREE(fxMesa->verts);
      fxMesa->verts = 0;
   }
}
