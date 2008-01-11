/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 */

/* Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 *    Daniel Borca <dborca@users.sourceforge.net>
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef FX

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "math/m_translate.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "fxdrv.h"


static void copy_pv( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GrVertex *dst = fxMesa->verts + edst;
   GrVertex *src = fxMesa->verts + esrc;

#if FX_PACKEDCOLOR
   *(GLuint *)&dst->pargb = *(GLuint *)&src->pargb;
#else  /* !FX_PACKEDCOLOR */
   COPY_FLOAT(dst->r, src->r);
   COPY_FLOAT(dst->g, src->g);
   COPY_FLOAT(dst->b, src->b);
   COPY_FLOAT(dst->a, src->a);
#endif /* !FX_PACKEDCOLOR */
}

static void copy_pv2( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GrVertex *dst = fxMesa->verts + edst;
   GrVertex *src = fxMesa->verts + esrc;

#if FX_PACKEDCOLOR
   *(GLuint *)&dst->pargb = *(GLuint *)&src->pargb;
   *(GLuint *)&dst->pspec = *(GLuint *)&src->pspec;
#else  /* !FX_PACKEDCOLOR */
   COPY_FLOAT(dst->r, src->r);
   COPY_FLOAT(dst->g, src->g);
   COPY_FLOAT(dst->b, src->b);
   COPY_FLOAT(dst->a, src->a);
   COPY_FLOAT(dst->r1, src->r1);
   COPY_FLOAT(dst->g1, src->g1);
   COPY_FLOAT(dst->b1, src->b1);
#endif /* !FX_PACKEDCOLOR */
}

static struct {
   void		      (*emit) (GLcontext *ctx, GLuint start, GLuint end, void *dest);
   tnl_copy_pv_func	copy_pv;
   tnl_interp_func	interp;
   GLboolean	      (*check_tex_sizes) (GLcontext *ctx);
   GLuint		vertex_format;
} setup_tab[MAX_SETUP];


#define GET_COLOR(ptr, idx) ((ptr)->data[idx])


static void interp_extras( GLcontext *ctx,
			   GLfloat t,
			   GLuint dst, GLuint out, GLuint in,
			   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      /* If stride is zero, ColorPtr[1] is constant across the VB, so
       * there is no point interpolating between two values as they will
       * be identical.  This case is handled in t_dd_tritmp.h
       */
      if (VB->ColorPtr[1]->stride) {
	 assert(VB->ColorPtr[1]->stride == 4 * sizeof(GLfloat));
	 INTERP_4F( t,
		    GET_COLOR(VB->ColorPtr[1], dst),
		    GET_COLOR(VB->ColorPtr[1], out),
		    GET_COLOR(VB->ColorPtr[1], in) );
      }

      if (VB->SecondaryColorPtr[1]) {
	 INTERP_3F( t,
		    GET_COLOR(VB->SecondaryColorPtr[1], dst),
		    GET_COLOR(VB->SecondaryColorPtr[1], out),
		    GET_COLOR(VB->SecondaryColorPtr[1], in) );
      }
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   setup_tab[FX_CONTEXT(ctx)->SetupIndex].interp(ctx, t, dst, out, in,
						   force_boundary);
}

static void copy_pv_extras( GLcontext *ctx, GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
	 COPY_4FV( GET_COLOR(VB->ColorPtr[1], dst),
		   GET_COLOR(VB->ColorPtr[1], src) );

	 if (VB->SecondaryColorPtr[1]) {
	    COPY_3FV( GET_COLOR(VB->SecondaryColorPtr[1], dst),
		      GET_COLOR(VB->SecondaryColorPtr[1], src) );
	 }
   }

   setup_tab[FX_CONTEXT(ctx)->SetupIndex].copy_pv(ctx, dst, src);
}


#define IND (SETUP_XYZW|SETUP_RGBA)
#define TAG(x) x##_wg
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_wgt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_wgt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PTEX)
#define TAG(x) x##_wgpt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX)
#define TAG(x) x##_wgpt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_PSIZ)
#define TAG(x) x##_wga
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PSIZ)
#define TAG(x) x##_wgt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|SETUP_PSIZ)
#define TAG(x) x##_wgt0t1a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_wgpt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_wgpt0t1a
#include "fxvbtmp.h"


#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC)
#define TAG(x) x##_2wg
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0)
#define TAG(x) x##_2wgt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_2wgt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PTEX)
#define TAG(x) x##_2wgpt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX)
#define TAG(x) x##_2wgpt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_PSIZ)
#define TAG(x) x##_2wga
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PSIZ)
#define TAG(x) x##_2wgt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|SETUP_PSIZ)
#define TAG(x) x##_2wgt0t1a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_2wgpt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_2wgpt0t1a
#include "fxvbtmp.h"

/* fog { */
#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_FOGC)
#define TAG(x) x##_wgf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_FOGC)
#define TAG(x) x##_wgt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|SETUP_FOGC)
#define TAG(x) x##_wgt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_wgpt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_wgpt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wgaf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wgt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wgt0t1af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wgpt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wgpt0t1af
#include "fxvbtmp.h"


#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_FOGC)
#define TAG(x) x##_2wgf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_FOGC)
#define TAG(x) x##_2wgt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|SETUP_FOGC)
#define TAG(x) x##_2wgt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_2wgpt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_2wgpt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wgaf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wgt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wgt0t1af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wgpt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wgpt0t1af
#include "fxvbtmp.h"
/* fog } */


/* Snapping for voodoo-1
 */
#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA)
#define TAG(x) x##_wsg
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_wsgt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_TMU1)
#define TAG(x) x##_wsgt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_PTEX)
#define TAG(x) x##_wsgpt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX)
#define TAG(x) x##_wsgpt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_PSIZ)
#define TAG(x) x##_wsga
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|SETUP_PSIZ)
#define TAG(x) x##_wsgt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_TMU1|SETUP_PSIZ)
#define TAG(x) x##_wsgt0t1a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_wsgpt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_wsgpt0t1a
#include "fxvbtmp.h"


#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC)
#define TAG(x) x##_2wsg
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0)
#define TAG(x) x##_2wsgt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_TMU1)
#define TAG(x) x##_2wsgt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_PTEX)
#define TAG(x) x##_2wsgpt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX)
#define TAG(x) x##_2wsgpt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_PSIZ)
#define TAG(x) x##_2wsga
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PSIZ)
#define TAG(x) x##_2wsgt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_TMU1|SETUP_PSIZ)
#define TAG(x) x##_2wsgt0t1a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_2wsgpt0a
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX|SETUP_PSIZ)
#define TAG(x) x##_2wsgpt0t1a
#include "fxvbtmp.h"

/* fog { */
#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_FOGC)
#define TAG(x) x##_wsgf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|SETUP_FOGC)
#define TAG(x) x##_wsgt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_TMU1|SETUP_FOGC)
#define TAG(x) x##_wsgt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_wsgpt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_wsgpt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wsgaf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wsgt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_TMU1|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wsgt0t1af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wsgpt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_wsgpt0t1af
#include "fxvbtmp.h"


#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_FOGC)
#define TAG(x) x##_2wsgf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_FOGC)
#define TAG(x) x##_2wsgt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_TMU1|SETUP_FOGC)
#define TAG(x) x##_2wsgt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_2wsgpt0f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX|SETUP_FOGC)
#define TAG(x) x##_2wsgpt0t1f
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wsgaf
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wsgt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_TMU1|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wsgt0t1af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
             SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wsgpt0af
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX|SETUP_PSIZ|SETUP_FOGC)
#define TAG(x) x##_2wsgpt0t1af
#include "fxvbtmp.h"
/* fog } */


/* Vertex repair (multipass rendering)
 */
#define IND (SETUP_RGBA)
#define TAG(x) x##_g
#include "fxvbtmp.h"

#define IND (SETUP_TMU0)
#define TAG(x) x##_t0
#include "fxvbtmp.h"

#define IND (SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_t0t1
#include "fxvbtmp.h"

#define IND (SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_gt0
#include "fxvbtmp.h"

#define IND (SETUP_RGBA|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_gt0t1
#include "fxvbtmp.h"


#define IND (SETUP_RGBA|SETUP_SPEC)
#define TAG(x) x##_2g
#include "fxvbtmp.h"

#define IND (SETUP_TMU0|SETUP_SPEC)
#define TAG(x) x##_2t0
#include "fxvbtmp.h"

#define IND (SETUP_TMU0|SETUP_SPEC|SETUP_TMU1)
#define TAG(x) x##_2t0t1
#include "fxvbtmp.h"

#define IND (SETUP_RGBA|SETUP_SPEC|SETUP_TMU0)
#define TAG(x) x##_2gt0
#include "fxvbtmp.h"

#define IND (SETUP_RGBA|SETUP_SPEC|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_2gt0t1
#include "fxvbtmp.h"



static void init_setup_tab( void )
{
   init_wg();
   init_wgt0();
   init_wgt0t1();
   init_wgpt0();
   init_wgpt0t1();
   init_wga();
   init_wgt0a();
   init_wgt0t1a();
   init_wgpt0a();
   init_wgpt0t1a();
    init_2wg();
    init_2wgt0();
    init_2wgt0t1();
    init_2wgpt0();
    init_2wgpt0t1();
    init_2wga();
    init_2wgt0a();
    init_2wgt0t1a();
    init_2wgpt0a();
    init_2wgpt0t1a();
   init_wgf();
   init_wgt0f();
   init_wgt0t1f();
   init_wgpt0f();
   init_wgpt0t1f();
   init_wgaf();
   init_wgt0af();
   init_wgt0t1af();
   init_wgpt0af();
   init_wgpt0t1af();
    init_2wgf();
    init_2wgt0f();
    init_2wgt0t1f();
    init_2wgpt0f();
    init_2wgpt0t1f();
    init_2wgaf();
    init_2wgt0af();
    init_2wgt0t1af();
    init_2wgpt0af();
    init_2wgpt0t1af();

   init_wsg();
   init_wsgt0();
   init_wsgt0t1();
   init_wsgpt0();
   init_wsgpt0t1();
   init_wsga();
   init_wsgt0a();
   init_wsgt0t1a();
   init_wsgpt0a();
   init_wsgpt0t1a();
    init_2wsg();
    init_2wsgt0();
    init_2wsgt0t1();
    init_2wsgpt0();
    init_2wsgpt0t1();
    init_2wsga();
    init_2wsgt0a();
    init_2wsgt0t1a();
    init_2wsgpt0a();
    init_2wsgpt0t1a();
   init_wsgf();
   init_wsgt0f();
   init_wsgt0t1f();
   init_wsgpt0f();
   init_wsgpt0t1f();
   init_wsgaf();
   init_wsgt0af();
   init_wsgt0t1af();
   init_wsgpt0af();
   init_wsgpt0t1af();
    init_2wsgf();
    init_2wsgt0f();
    init_2wsgt0t1f();
    init_2wsgpt0f();
    init_2wsgpt0t1f();
    init_2wsgaf();
    init_2wsgt0af();
    init_2wsgt0t1af();
    init_2wsgpt0af();
    init_2wsgpt0t1af();

   init_g();
   init_t0();
   init_t0t1();
   init_gt0();
   init_gt0t1();
    init_2g();
    init_2t0();
    init_2t0t1();
    init_2gt0();
    init_2gt0t1();
}


void fxPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & SETUP_XYZW)     ? " xyzw," : "",
	   (flags & SETUP_SNAP)     ? " snap," : "",
	   (flags & SETUP_RGBA)     ? " rgba," : "",
	   (flags & SETUP_TMU0)     ? " tex-0," : "",
	   (flags & SETUP_TMU1)     ? " tex-1," : "",
	   (flags & SETUP_PSIZ)     ? " psiz," : "",
	   (flags & SETUP_SPEC)     ? " spec," : "",
	   (flags & SETUP_FOGC)     ? " fog," : "");
}



void fxCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   fxMesaContext fxMesa = FX_CONTEXT( ctx );

   if (!setup_tab[fxMesa->SetupIndex].check_tex_sizes(ctx)) {
      GLuint ind = fxMesa->SetupIndex |= (SETUP_PTEX|SETUP_RGBA);

      /* Tdfx handles projective textures nicely; just have to change
       * up to the new vertex format.
       */
      if (setup_tab[ind].vertex_format != fxMesa->stw_hint_state) {

	 fxMesa->stw_hint_state = setup_tab[ind].vertex_format;
	 FX_grHints(GR_HINT_STWHINT, fxMesa->stw_hint_state);

	 /* This is required as we have just changed the vertex
	  * format, so the interp routines must also change.
	  * In the unfilled and twosided cases we are using the
	  * Extras ones anyway, so leave them in place.
	  */
	 if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	    tnl->Driver.Render.Interp = setup_tab[fxMesa->SetupIndex].interp;
	 }
      }
   }
}


void fxBuildVertices( GLcontext *ctx, GLuint start, GLuint end,
			GLuint newinputs )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GrVertex *v = (fxMesa->verts + start);

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[fxMesa->SetupIndex].emit( ctx, start, end, v );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= SETUP_RGBA;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= SETUP_SPEC;

      if (newinputs & VERT_BIT_FOG)
	 ind |= SETUP_FOGC;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= SETUP_TMU0;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= SETUP_TMU0|SETUP_TMU1;

      if (fxMesa->SetupIndex & SETUP_PTEX)
	 ind = ~0;

      ind &= fxMesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, end, v );
      }
   }
}


void fxChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GLuint ind = SETUP_XYZW|SETUP_RGBA;

   if (fxMesa->snapVertices)
      ind |= SETUP_SNAP;

   fxMesa->tmu_source[0] = 0;
   fxMesa->tmu_source[1] = 1;

   if (ctx->Texture._EnabledUnits & 0x2) {
      if (ctx->Texture._EnabledUnits & 0x1) {
	 ind |= SETUP_TMU1;
      }
      ind |= SETUP_TMU0;
      fxMesa->tmu_source[0] = 1;
      fxMesa->tmu_source[1] = 0;
   }
   else if (ctx->Texture._EnabledUnits & 0x1) {
      ind |= SETUP_TMU0;
   }

   if (ctx->_TriangleCaps & DD_POINT_ATTEN) {
      ind |= SETUP_PSIZ;
   }

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
      ind |= SETUP_SPEC;
   }

   if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT) {
      ind |= SETUP_FOGC;
   }

   fxMesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = interp_extras;
      tnl->Driver.Render.CopyPV = copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != fxMesa->stw_hint_state) {
      fxMesa->stw_hint_state = setup_tab[ind].vertex_format;
      FX_grHints(GR_HINT_STWHINT, fxMesa->stw_hint_state);
   }
}



void fxAllocVB( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   static int firsttime = 1;
   if (firsttime) {
      init_setup_tab();
      firsttime = 0;
   }

   fxMesa->verts = (GrVertex *)ALIGN_MALLOC(size * sizeof(GrVertex), 32);
   fxMesa->SetupIndex = SETUP_XYZW|SETUP_RGBA;
}


void fxFreeVB( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   if (fxMesa->verts) {
      ALIGN_FREE(fxMesa->verts);
      fxMesa->verts = 0;
   }
}
#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_vb(void);
int
gl_fx_dummy_function_vb(void)
{
   return 0;
}

#endif /* FX */
