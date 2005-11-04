/* $XFree86$ */
/**************************************************************************

Copyright 2002 ATI Technologies Inc., Ontario, Canada, and
               Tungsten Graphics Inc., Austin, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "glheader.h"
#include "imports.h"

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"


static struct { 
	int start; 
	int len; 
	const char *name;
} packet[RADEON_MAX_STATE_PACKETS] = {
	{ RADEON_PP_MISC,7,"RADEON_PP_MISC" },
	{ RADEON_PP_CNTL,3,"RADEON_PP_CNTL" },
	{ RADEON_RB3D_COLORPITCH,1,"RADEON_RB3D_COLORPITCH" },
	{ RADEON_RE_LINE_PATTERN,2,"RADEON_RE_LINE_PATTERN" },
	{ RADEON_SE_LINE_WIDTH,1,"RADEON_SE_LINE_WIDTH" },
	{ RADEON_PP_LUM_MATRIX,1,"RADEON_PP_LUM_MATRIX" },
	{ RADEON_PP_ROT_MATRIX_0,2,"RADEON_PP_ROT_MATRIX_0" },
	{ RADEON_RB3D_STENCILREFMASK,3,"RADEON_RB3D_STENCILREFMASK" },
	{ RADEON_SE_VPORT_XSCALE,6,"RADEON_SE_VPORT_XSCALE" },
	{ RADEON_SE_CNTL,2,"RADEON_SE_CNTL" },
	{ RADEON_SE_CNTL_STATUS,1,"RADEON_SE_CNTL_STATUS" },
	{ RADEON_RE_MISC,1,"RADEON_RE_MISC" },
	{ RADEON_PP_TXFILTER_0,6,"RADEON_PP_TXFILTER_0" },
	{ RADEON_PP_BORDER_COLOR_0,1,"RADEON_PP_BORDER_COLOR_0" },
	{ RADEON_PP_TXFILTER_1,6,"RADEON_PP_TXFILTER_1" },
	{ RADEON_PP_BORDER_COLOR_1,1,"RADEON_PP_BORDER_COLOR_1" },
	{ RADEON_PP_TXFILTER_2,6,"RADEON_PP_TXFILTER_2" },
	{ RADEON_PP_BORDER_COLOR_2,1,"RADEON_PP_BORDER_COLOR_2" },
	{ RADEON_SE_ZBIAS_FACTOR,2,"RADEON_SE_ZBIAS_FACTOR" },
	{ RADEON_SE_TCL_OUTPUT_VTX_FMT,11,"RADEON_SE_TCL_OUTPUT_VTX_FMT" },
	{ RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED,17,"RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED" },
};


static void radeonCompatEmitPacket( radeonContextPtr rmesa, 
				    struct radeon_state_atom *state )
{
   drm_radeon_sarea_t *sarea = rmesa->sarea;
   drm_radeon_context_regs_t *ctx = &sarea->context_state;
   drm_radeon_texture_regs_t *tex0 = &sarea->tex_state[0];
   drm_radeon_texture_regs_t *tex1 = &sarea->tex_state[1];
   int i;
   int *buf = state->cmd;

   for ( i = 0 ; i < state->cmd_size ; ) {
      drm_radeon_cmd_header_t *header = (drm_radeon_cmd_header_t *)&buf[i++];

      if (RADEON_DEBUG & DEBUG_STATE)
	 fprintf(stderr, "%s %d: %s\n", __FUNCTION__, header->packet.packet_id,
		 packet[(int)header->packet.packet_id].name);

      switch (header->packet.packet_id) {
      case RADEON_EMIT_PP_MISC:
	 ctx->pp_misc = buf[i++]; 
	 ctx->pp_fog_color = buf[i++];
	 ctx->re_solid_color = buf[i++];
	 ctx->rb3d_blendcntl = buf[i++];
	 ctx->rb3d_depthoffset = buf[i++];
	 ctx->rb3d_depthpitch = buf[i++];
	 ctx->rb3d_zstencilcntl = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_CONTEXT;
	 break;
      case RADEON_EMIT_PP_CNTL:
	 ctx->pp_cntl = buf[i++];
	 ctx->rb3d_cntl = buf[i++];
	 ctx->rb3d_coloroffset = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_CONTEXT;
	 break;
      case RADEON_EMIT_RB3D_COLORPITCH:
	 ctx->rb3d_colorpitch = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_CONTEXT;
	 break;
      case RADEON_EMIT_RE_LINE_PATTERN:
	 ctx->re_line_pattern = buf[i++];
	 ctx->re_line_state = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_LINE;
	 break;
      case RADEON_EMIT_SE_LINE_WIDTH:
	 ctx->se_line_width = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_LINE;
	 break;
      case RADEON_EMIT_PP_LUM_MATRIX:
	 ctx->pp_lum_matrix = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_BUMPMAP;
	 break;
      case RADEON_EMIT_PP_ROT_MATRIX_0:
	 ctx->pp_rot_matrix_0 = buf[i++];
	 ctx->pp_rot_matrix_1 = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_BUMPMAP;
	 break;
      case RADEON_EMIT_RB3D_STENCILREFMASK:
	 ctx->rb3d_stencilrefmask = buf[i++];
	 ctx->rb3d_ropcntl = buf[i++];
	 ctx->rb3d_planemask = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_MASKS;
	 break;
      case RADEON_EMIT_SE_VPORT_XSCALE:
	 ctx->se_vport_xscale = buf[i++];
	 ctx->se_vport_xoffset = buf[i++];
	 ctx->se_vport_yscale = buf[i++];
	 ctx->se_vport_yoffset = buf[i++];
	 ctx->se_vport_zscale = buf[i++];
	 ctx->se_vport_zoffset = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_VIEWPORT;
	 break;
      case RADEON_EMIT_SE_CNTL:
	 ctx->se_cntl = buf[i++];
	 ctx->se_coord_fmt = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_CONTEXT | RADEON_UPLOAD_VERTFMT;
	 break;
      case RADEON_EMIT_SE_CNTL_STATUS:
	 ctx->se_cntl_status = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_SETUP;
	 break;
      case RADEON_EMIT_RE_MISC:
	 ctx->re_misc = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_MISC;
	 break;
      case RADEON_EMIT_PP_TXFILTER_0:
	 tex0->pp_txfilter = buf[i++];
	 tex0->pp_txformat = buf[i++];
	 tex0->pp_txoffset = buf[i++];
	 tex0->pp_txcblend = buf[i++];
	 tex0->pp_txablend = buf[i++];
	 tex0->pp_tfactor = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_TEX0;
	 break;
      case RADEON_EMIT_PP_BORDER_COLOR_0:
	 tex0->pp_border_color = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_TEX0;
	 break;
      case RADEON_EMIT_PP_TXFILTER_1:
	 tex1->pp_txfilter = buf[i++];
	 tex1->pp_txformat = buf[i++];
	 tex1->pp_txoffset = buf[i++];
	 tex1->pp_txcblend = buf[i++];
	 tex1->pp_txablend = buf[i++];
	 tex1->pp_tfactor = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_TEX1;
	 break;
      case RADEON_EMIT_PP_BORDER_COLOR_1:
	 tex1->pp_border_color = buf[i++];
	 sarea->dirty |= RADEON_UPLOAD_TEX1;
	 break;

      case RADEON_EMIT_SE_ZBIAS_FACTOR:
	 i++;
	 i++;
	 break;

      case RADEON_EMIT_PP_TXFILTER_2:
      case RADEON_EMIT_PP_BORDER_COLOR_2:
      case RADEON_EMIT_SE_TCL_OUTPUT_VTX_FMT:
      case RADEON_EMIT_SE_TCL_MATERIAL_EMMISSIVE_RED:
      default:
	 /* These states aren't understood by radeon drm 1.1 */
	 fprintf(stderr, "Tried to emit unsupported state\n");
	 return;
      }
   }
}



static void radeonCompatEmitStateLocked( radeonContextPtr rmesa )
{
   struct radeon_state_atom *atom;

   if (RADEON_DEBUG & (DEBUG_STATE|DEBUG_PRIMS))
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!rmesa->hw.is_dirty && !rmesa->hw.all_dirty)
      return;

   foreach(atom, &rmesa->hw.atomlist) {
      if (rmesa->hw.all_dirty)
	 atom->dirty = GL_TRUE;
      if (atom->is_tcl)
	 atom->dirty = GL_FALSE;
      if (atom->dirty)
	 radeonCompatEmitPacket(rmesa, atom);
   }
 
   rmesa->hw.is_dirty = GL_FALSE;
   rmesa->hw.all_dirty = GL_FALSE;
}


static void radeonCompatEmitPrimitiveLocked( radeonContextPtr rmesa,
					     GLuint hw_primitive,
					     GLuint nverts,
					     drm_clip_rect_t *pbox,
					     GLuint nbox )
{
   int i;

   for ( i = 0 ; i < nbox ; ) {
      int nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS, nbox );
      drm_clip_rect_t *b = rmesa->sarea->boxes;
      drm_radeon_vertex_t vtx;
      
      rmesa->sarea->dirty |= RADEON_UPLOAD_CLIPRECTS;
      rmesa->sarea->nbox = nr - i;

      for ( ; i < nr ; i++) 
	 *b++ = pbox[i];
      
      if (RADEON_DEBUG & DEBUG_IOCTL)
	 fprintf(stderr, 
		 "RadeonFlushVertexBuffer: prim %x buf %d verts %d "
		 "disc %d nbox %d\n",
		 hw_primitive, 
		 rmesa->dma.current.buf->buf->idx, 
		 nverts, 
		 nr == nbox,
		 rmesa->sarea->nbox );

      vtx.prim = hw_primitive;
      vtx.idx = rmesa->dma.current.buf->buf->idx;
      vtx.count = nverts;
      vtx.discard = (nr == nbox);      

      drmCommandWrite( rmesa->dri.fd, 
		       DRM_RADEON_VERTEX,
		       &vtx, sizeof(vtx));
   }
}



/* No 'start' for 1.1 vertices ioctl: only one vertex prim/buffer!  
 */
void radeonCompatEmitPrimitive( radeonContextPtr rmesa,
				GLuint vertex_format,
				GLuint hw_primitive,
				GLuint nrverts )
{
   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   LOCK_HARDWARE( rmesa );

   radeonCompatEmitStateLocked( rmesa );
   rmesa->sarea->vc_format = vertex_format;
   
   if (rmesa->state.scissor.enabled) {
      radeonCompatEmitPrimitiveLocked( rmesa, 
				       hw_primitive,
				       nrverts,
				       rmesa->state.scissor.pClipRects,
				       rmesa->state.scissor.numClipRects );
   }
   else {
      radeonCompatEmitPrimitiveLocked( rmesa, 
				       hw_primitive,
				       nrverts,
				       rmesa->pClipRects,
				       rmesa->numClipRects );
   }


   UNLOCK_HARDWARE( rmesa );
}

