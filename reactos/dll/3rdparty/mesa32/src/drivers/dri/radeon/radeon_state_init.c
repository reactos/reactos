/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_state_init.c,v 1.3 2003/02/22 06:21:11 dawes Exp $ */
/*
 * Copyright 2000, 2001 VA Linux Systems Inc., Fremont, California.
 *
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "api_arrayelt.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_tcl.h"
#include "radeon_tex.h"
#include "radeon_swtcl.h"

#include "xmlpool.h"

/* =============================================================
 * State initialization
 */

void radeonPrintDirty( radeonContextPtr rmesa, const char *msg )
{
   struct radeon_state_atom *l;

   fprintf(stderr, msg);
   fprintf(stderr, ": ");

   foreach(l, &rmesa->hw.atomlist) {
      if (l->dirty || rmesa->hw.all_dirty)
	 fprintf(stderr, "%s, ", l->name);
   }

   fprintf(stderr, "\n");
}

static int cmdpkt( int id ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.packet.cmd_type = RADEON_CMD_PACKET;
   h.packet.packet_id = id;
   return h.i;
}

static int cmdvec( int offset, int stride, int count ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.vectors.cmd_type = RADEON_CMD_VECTORS;
   h.vectors.offset = offset;
   h.vectors.stride = stride;
   h.vectors.count = count;
   return h.i;
}

static int cmdscl( int offset, int stride, int count ) 
{
   drm_radeon_cmd_header_t h;
   h.i = 0;
   h.scalars.cmd_type = RADEON_CMD_SCALARS;
   h.scalars.offset = offset;
   h.scalars.stride = stride;
   h.scalars.count = count;
   return h.i;
}

#define CHECK( NM, FLAG )			\
static GLboolean check_##NM( GLcontext *ctx )	\
{						\
   return FLAG;					\
}

#define TCL_CHECK( NM, FLAG )				\
static GLboolean check_##NM( GLcontext *ctx )		\
{							\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);	\
   return !rmesa->TclFallback && (FLAG);		\
}


CHECK( always, GL_TRUE )
CHECK( never, GL_FALSE )
CHECK( tex0, ctx->Texture.Unit[0]._ReallyEnabled )
CHECK( tex1, ctx->Texture.Unit[1]._ReallyEnabled )
/* need this for the cubic_map on disabled unit 2 bug, maybe r100 only? */
CHECK( tex2, ctx->Texture._EnabledUnits )
CHECK( cube0, (ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_CUBE_BIT))
CHECK( cube1, (ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_CUBE_BIT))
CHECK( cube2, (ctx->Texture.Unit[2]._ReallyEnabled & TEXTURE_CUBE_BIT))
CHECK( fog, ctx->Fog.Enabled )
TCL_CHECK( tcl, GL_TRUE )
TCL_CHECK( tcl_tex0, ctx->Texture.Unit[0]._ReallyEnabled )
TCL_CHECK( tcl_tex1, ctx->Texture.Unit[1]._ReallyEnabled )
TCL_CHECK( tcl_tex2, ctx->Texture.Unit[2]._ReallyEnabled )
TCL_CHECK( tcl_lighting, ctx->Light.Enabled )
TCL_CHECK( tcl_eyespace_or_lighting, ctx->_NeedEyeCoords || ctx->Light.Enabled )
TCL_CHECK( tcl_lit0, ctx->Light.Enabled && ctx->Light.Light[0].Enabled )
TCL_CHECK( tcl_lit1, ctx->Light.Enabled && ctx->Light.Light[1].Enabled )
TCL_CHECK( tcl_lit2, ctx->Light.Enabled && ctx->Light.Light[2].Enabled )
TCL_CHECK( tcl_lit3, ctx->Light.Enabled && ctx->Light.Light[3].Enabled )
TCL_CHECK( tcl_lit4, ctx->Light.Enabled && ctx->Light.Light[4].Enabled )
TCL_CHECK( tcl_lit5, ctx->Light.Enabled && ctx->Light.Light[5].Enabled )
TCL_CHECK( tcl_lit6, ctx->Light.Enabled && ctx->Light.Light[6].Enabled )
TCL_CHECK( tcl_lit7, ctx->Light.Enabled && ctx->Light.Light[7].Enabled )
TCL_CHECK( tcl_ucp0, (ctx->Transform.ClipPlanesEnabled & 0x1) )
TCL_CHECK( tcl_ucp1, (ctx->Transform.ClipPlanesEnabled & 0x2) )
TCL_CHECK( tcl_ucp2, (ctx->Transform.ClipPlanesEnabled & 0x4) )
TCL_CHECK( tcl_ucp3, (ctx->Transform.ClipPlanesEnabled & 0x8) )
TCL_CHECK( tcl_ucp4, (ctx->Transform.ClipPlanesEnabled & 0x10) )
TCL_CHECK( tcl_ucp5, (ctx->Transform.ClipPlanesEnabled & 0x20) )
TCL_CHECK( tcl_eyespace_or_fog, ctx->_NeedEyeCoords || ctx->Fog.Enabled ) 

CHECK( txr0, (ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_RECT_BIT))
CHECK( txr1, (ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_RECT_BIT))
CHECK( txr2, (ctx->Texture.Unit[2]._ReallyEnabled & TEXTURE_RECT_BIT))



/* Initialize the context's hardware state.
 */
void radeonInitState( radeonContextPtr rmesa )
{
   GLcontext *ctx = rmesa->glCtx;
   GLuint color_fmt, depth_fmt, i;
   GLint drawPitch, drawOffset;

   switch ( rmesa->radeonScreen->cpp ) {
   case 2:
      color_fmt = RADEON_COLOR_FORMAT_RGB565;
      break;
   case 4:
      color_fmt = RADEON_COLOR_FORMAT_ARGB8888;
      break;
   default:
      fprintf( stderr, "Error: Unsupported pixel depth... exiting\n" );
      exit( -1 );
   }

   rmesa->state.color.clear = 0x00000000;

   switch ( ctx->Visual.depthBits ) {
   case 16:
      rmesa->state.depth.clear = 0x0000ffff;
      rmesa->state.depth.scale = 1.0 / (GLfloat)0xffff;
      depth_fmt = RADEON_DEPTH_FORMAT_16BIT_INT_Z;
      rmesa->state.stencil.clear = 0x00000000;
      break;
   case 24:
      rmesa->state.depth.clear = 0x00ffffff;
      rmesa->state.depth.scale = 1.0 / (GLfloat)0xffffff;
      depth_fmt = RADEON_DEPTH_FORMAT_24BIT_INT_Z;
      rmesa->state.stencil.clear = 0xffff0000;
      break;
   default:
      fprintf( stderr, "Error: Unsupported depth %d... exiting\n",
	       ctx->Visual.depthBits );
      exit( -1 );
   }

   /* Only have hw stencil when depth buffer is 24 bits deep */
   rmesa->state.stencil.hwBuffer = ( ctx->Visual.stencilBits > 0 &&
				     ctx->Visual.depthBits == 24 );

   rmesa->Fallback = 0;

   if ( ctx->Visual.doubleBufferMode && rmesa->sarea->pfCurrentPage == 0 ) {
      drawOffset = rmesa->radeonScreen->backOffset;
      drawPitch  = rmesa->radeonScreen->backPitch;
   } else {
      drawOffset = rmesa->radeonScreen->frontOffset;
      drawPitch  = rmesa->radeonScreen->frontPitch;
   }

   rmesa->hw.max_state_size = 0;

#define ALLOC_STATE( ATOM, CHK, SZ, NM, FLAG )				\
   do {								\
      rmesa->hw.ATOM.cmd_size = SZ;				\
      rmesa->hw.ATOM.cmd = (int *)CALLOC(SZ * sizeof(int));	\
      rmesa->hw.ATOM.lastcmd = (int *)CALLOC(SZ * sizeof(int));	\
      rmesa->hw.ATOM.name = NM;					\
      rmesa->hw.ATOM.is_tcl = FLAG;					\
      rmesa->hw.ATOM.check = check_##CHK;				\
      rmesa->hw.ATOM.dirty = GL_TRUE;				\
      rmesa->hw.max_state_size += SZ * sizeof(int);		\
   } while (0)
      
      
   /* Allocate state buffers:
    */
   ALLOC_STATE( ctx, always, CTX_STATE_SIZE, "CTX/context", 0 );
   ALLOC_STATE( lin, always, LIN_STATE_SIZE, "LIN/line", 0 );
   ALLOC_STATE( msk, always, MSK_STATE_SIZE, "MSK/mask", 0 );
   ALLOC_STATE( vpt, always, VPT_STATE_SIZE, "VPT/viewport", 0 );
   ALLOC_STATE( set, always, SET_STATE_SIZE, "SET/setup", 0 );
   ALLOC_STATE( msc, always, MSC_STATE_SIZE, "MSC/misc", 0 );
   ALLOC_STATE( zbs, always, ZBS_STATE_SIZE, "ZBS/zbias", 0 );
   ALLOC_STATE( tcl, always, TCL_STATE_SIZE, "TCL/tcl", 1 );
   ALLOC_STATE( mtl, tcl_lighting, MTL_STATE_SIZE, "MTL/material", 1 );
   ALLOC_STATE( grd, always, GRD_STATE_SIZE, "GRD/guard-band", 1 );
   ALLOC_STATE( fog, fog, FOG_STATE_SIZE, "FOG/fog", 1 );
   ALLOC_STATE( glt, tcl_lighting, GLT_STATE_SIZE, "GLT/light-global", 1 );
   ALLOC_STATE( eye, tcl_lighting, EYE_STATE_SIZE, "EYE/eye-vector", 1 );
   ALLOC_STATE( tex[0], tex0, TEX_STATE_SIZE, "TEX/tex-0", 0 );
   ALLOC_STATE( tex[1], tex1, TEX_STATE_SIZE, "TEX/tex-1", 0 );
   ALLOC_STATE( tex[2], tex2, TEX_STATE_SIZE, "TEX/tex-2", 0 );
   if (rmesa->radeonScreen->drmSupportsCubeMapsR100)
   {
      ALLOC_STATE( cube[0], cube0, CUBE_STATE_SIZE, "CUBE/cube-0", 0 );
      ALLOC_STATE( cube[1], cube1, CUBE_STATE_SIZE, "CUBE/cube-1", 0 );
      ALLOC_STATE( cube[2], cube2, CUBE_STATE_SIZE, "CUBE/cube-2", 0 );
   }
   else
   {
      ALLOC_STATE( cube[0], never, CUBE_STATE_SIZE, "CUBE/cube-0", 0 );
      ALLOC_STATE( cube[1], never, CUBE_STATE_SIZE, "CUBE/cube-1", 0 );
      ALLOC_STATE( cube[2], never, CUBE_STATE_SIZE, "CUBE/cube-2", 0 );
   }
   ALLOC_STATE( mat[0], tcl, MAT_STATE_SIZE, "MAT/modelproject", 1 );
   ALLOC_STATE( mat[1], tcl_eyespace_or_fog, MAT_STATE_SIZE, "MAT/modelview", 1 );
   ALLOC_STATE( mat[2], tcl_eyespace_or_lighting, MAT_STATE_SIZE, "MAT/it-modelview", 1 );
   ALLOC_STATE( mat[3], tcl_tex0, MAT_STATE_SIZE, "MAT/texmat0", 1 );
   ALLOC_STATE( mat[4], tcl_tex1, MAT_STATE_SIZE, "MAT/texmat1", 1 );
   ALLOC_STATE( mat[5], tcl_tex2, MAT_STATE_SIZE, "MAT/texmat2", 1 );
   ALLOC_STATE( ucp[0], tcl_ucp0, UCP_STATE_SIZE, "UCP/userclip-0", 1 );
   ALLOC_STATE( ucp[1], tcl_ucp1, UCP_STATE_SIZE, "UCP/userclip-1", 1 );
   ALLOC_STATE( ucp[2], tcl_ucp2, UCP_STATE_SIZE, "UCP/userclip-2", 1 );
   ALLOC_STATE( ucp[3], tcl_ucp3, UCP_STATE_SIZE, "UCP/userclip-3", 1 );
   ALLOC_STATE( ucp[4], tcl_ucp4, UCP_STATE_SIZE, "UCP/userclip-4", 1 );
   ALLOC_STATE( ucp[5], tcl_ucp5, UCP_STATE_SIZE, "UCP/userclip-5", 1 );
   ALLOC_STATE( lit[0], tcl_lit0, LIT_STATE_SIZE, "LIT/light-0", 1 );
   ALLOC_STATE( lit[1], tcl_lit1, LIT_STATE_SIZE, "LIT/light-1", 1 );
   ALLOC_STATE( lit[2], tcl_lit2, LIT_STATE_SIZE, "LIT/light-2", 1 );
   ALLOC_STATE( lit[3], tcl_lit3, LIT_STATE_SIZE, "LIT/light-3", 1 );
   ALLOC_STATE( lit[4], tcl_lit4, LIT_STATE_SIZE, "LIT/light-4", 1 );
   ALLOC_STATE( lit[5], tcl_lit5, LIT_STATE_SIZE, "LIT/light-5", 1 );
   ALLOC_STATE( lit[6], tcl_lit6, LIT_STATE_SIZE, "LIT/light-6", 1 );
   ALLOC_STATE( lit[7], tcl_lit7, LIT_STATE_SIZE, "LIT/light-7", 1 );
   ALLOC_STATE( txr[0], txr0, TXR_STATE_SIZE, "TXR/txr-0", 0 );
   ALLOC_STATE( txr[1], txr1, TXR_STATE_SIZE, "TXR/txr-1", 0 );
   ALLOC_STATE( txr[2], txr2, TXR_STATE_SIZE, "TXR/txr-2", 0 );

   radeonSetUpAtomList( rmesa );

   /* Fill in the packet headers:
    */
   rmesa->hw.ctx.cmd[CTX_CMD_0] = cmdpkt(RADEON_EMIT_PP_MISC);
   rmesa->hw.ctx.cmd[CTX_CMD_1] = cmdpkt(RADEON_EMIT_PP_CNTL);
   rmesa->hw.ctx.cmd[CTX_CMD_2] = cmdpkt(RADEON_EMIT_RB3D_COLORPITCH);
   rmesa->hw.lin.cmd[LIN_CMD_0] = cmdpkt(RADEON_EMIT_RE_LINE_PATTERN);
   rmesa->hw.lin.cmd[LIN_CMD_1] = cmdpkt(RADEON_EMIT_SE_LINE_WIDTH);
   rmesa->hw.msk.cmd[MSK_CMD_0] = cmdpkt(RADEON_EMIT_RB3D_STENCILREFMASK);
   rmesa->hw.vpt.cmd[VPT_CMD_0] = cmdpkt(RADEON_EMIT_SE_VPORT_XSCALE);
   rmesa->hw.set.cmd[SET_CMD_0] = cmdpkt(RADEON_EMIT_SE_CNTL);
   rmesa->hw.set.cmd[SET_CMD_1] = cmdpkt(RADEON_EMIT_SE_CNTL_STATUS);
   rmesa->hw.msc.cmd[MSC_CMD_0] = cmdpkt(RADEON_EMIT_RE_MISC);
   rmesa->hw.tex[0].cmd[TEX_CMD_0] = cmdpkt(RADEON_EMIT_PP_TXFILTER_0);
   rmesa->hw.tex[0].cmd[TEX_CMD_1] = cmdpkt(RADEON_EMIT_PP_BORDER_COLOR_0);
   rmesa->hw.tex[1].cmd[TEX_CMD_0] = cmdpkt(RADEON_EMIT_PP_TXFILTER_1);
   rmesa->hw.tex[1].cmd[TEX_CMD_1] = cmdpkt(RADEON_EMIT_PP_BORDER_COLOR_1);
   rmesa->hw.tex[2].cmd[TEX_CMD_0] = cmdpkt(RADEON_EMIT_PP_TXFILTER_2);
   rmesa->hw.tex[2].cmd[TEX_CMD_1] = cmdpkt(RADEON_EMIT_PP_BORDER_COLOR_2);
   rmesa->hw.cube[0].cmd[CUBE_CMD_0] = cmdpkt(RADEON_EMIT_PP_CUBIC_FACES_0);
   rmesa->hw.cube[0].cmd[CUBE_CMD_1] = cmdpkt(RADEON_EMIT_PP_CUBIC_OFFSETS_T0);
   rmesa->hw.cube[1].cmd[CUBE_CMD_0] = cmdpkt(RADEON_EMIT_PP_CUBIC_FACES_1);
   rmesa->hw.cube[1].cmd[CUBE_CMD_1] = cmdpkt(RADEON_EMIT_PP_CUBIC_OFFSETS_T1);
   rmesa->hw.cube[2].cmd[CUBE_CMD_0] = cmdpkt(RADEON_EMIT_PP_CUBIC_FACES_2);
   rmesa->hw.cube[2].cmd[CUBE_CMD_1] = cmdpkt(RADEON_EMIT_PP_CUBIC_OFFSETS_T2);
   rmesa->hw.zbs.cmd[ZBS_CMD_0] = cmdpkt(RADEON_EMIT_SE_ZBIAS_FACTOR);
   rmesa->hw.tcl.cmd[TCL_CMD_0] = cmdpkt(RADEON_EMIT_SE_TCL_OUTPUT_VTX_FMT);
   rmesa->hw.mtl.cmd[MTL_CMD_0] = 
      cmdpkt(RADEON_EMIT_SE_TCL_MATERIAL_EMMISSIVE_RED);
   rmesa->hw.txr[0].cmd[TXR_CMD_0] = cmdpkt(RADEON_EMIT_PP_TEX_SIZE_0);
   rmesa->hw.txr[1].cmd[TXR_CMD_0] = cmdpkt(RADEON_EMIT_PP_TEX_SIZE_1);
   rmesa->hw.txr[2].cmd[TXR_CMD_0] = cmdpkt(RADEON_EMIT_PP_TEX_SIZE_2);
   rmesa->hw.grd.cmd[GRD_CMD_0] = 
      cmdscl( RADEON_SS_VERT_GUARD_CLIP_ADJ_ADDR, 1, 4 );
   rmesa->hw.fog.cmd[FOG_CMD_0] = 
      cmdvec( RADEON_VS_FOG_PARAM_ADDR, 1, 4 );
   rmesa->hw.glt.cmd[GLT_CMD_0] = 
      cmdvec( RADEON_VS_GLOBAL_AMBIENT_ADDR, 1, 4 );
   rmesa->hw.eye.cmd[EYE_CMD_0] = 
      cmdvec( RADEON_VS_EYE_VECTOR_ADDR, 1, 4 );

   for (i = 0 ; i < 6; i++) {
      rmesa->hw.mat[i].cmd[MAT_CMD_0] = 
	 cmdvec( RADEON_VS_MATRIX_0_ADDR + i*4, 1, 16);
   }

   for (i = 0 ; i < 8; i++) {
      rmesa->hw.lit[i].cmd[LIT_CMD_0] = 
	 cmdvec( RADEON_VS_LIGHT_AMBIENT_ADDR + i, 8, 24 );
      rmesa->hw.lit[i].cmd[LIT_CMD_1] = 
	 cmdscl( RADEON_SS_LIGHT_DCD_ADDR + i, 8, 6 );
   }

   for (i = 0 ; i < 6; i++) {
      rmesa->hw.ucp[i].cmd[UCP_CMD_0] = 
	 cmdvec( RADEON_VS_UCP_ADDR + i, 1, 4 );
   }

   rmesa->last_ReallyEnabled = -1;

   /* Initial Harware state:
    */
   rmesa->hw.ctx.cmd[CTX_PP_MISC] = (RADEON_ALPHA_TEST_PASS |
				     RADEON_CHROMA_FUNC_FAIL |
				     RADEON_CHROMA_KEY_NEAREST |
				     RADEON_SHADOW_FUNC_EQUAL |
				     RADEON_SHADOW_PASS_1 /*|
				     RADEON_RIGHT_HAND_CUBE_OGL */);

   rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] = (RADEON_FOG_VERTEX |
					  /* this bit unused for vertex fog */
					  RADEON_FOG_USE_DEPTH);

   rmesa->hw.ctx.cmd[CTX_RE_SOLID_COLOR] = 0x00000000;

   rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = (RADEON_COMB_FCN_ADD_CLAMP |
					    RADEON_SRC_BLEND_GL_ONE |
					    RADEON_DST_BLEND_GL_ZERO );

   rmesa->hw.ctx.cmd[CTX_RB3D_DEPTHOFFSET] =
      rmesa->radeonScreen->depthOffset + rmesa->radeonScreen->fbLocation;

   rmesa->hw.ctx.cmd[CTX_RB3D_DEPTHPITCH] = 
      ((rmesa->radeonScreen->depthPitch &
	RADEON_DEPTHPITCH_MASK) |
       RADEON_DEPTH_ENDIAN_NO_SWAP);
       
   if (rmesa->using_hyperz)
       rmesa->hw.ctx.cmd[CTX_RB3D_DEPTHPITCH] |= RADEON_DEPTH_HYPERZ;

   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] = (depth_fmt |
					       RADEON_Z_TEST_LESS |
					       RADEON_STENCIL_TEST_ALWAYS |
					       RADEON_STENCIL_FAIL_KEEP |
					       RADEON_STENCIL_ZPASS_KEEP |
					       RADEON_STENCIL_ZFAIL_KEEP |
					       RADEON_Z_WRITE_ENABLE);

   if (rmesa->using_hyperz) {
       rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_COMPRESSION_ENABLE |
						   RADEON_Z_DECOMPRESSION_ENABLE;
      if (rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	 /* works for q3, but slight rendering errors with glxgears ? */
/*	 rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_Z_HIERARCHY_ENABLE;*/
	 /* need this otherwise get lots of lockups with q3 ??? */
	 rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= RADEON_FORCE_Z_DIRTY;
      } 
   }

   rmesa->hw.ctx.cmd[CTX_PP_CNTL] = (RADEON_SCISSOR_ENABLE |
				     RADEON_ANTI_ALIAS_NONE);

   rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = (RADEON_PLANE_MASK_ENABLE |
				       color_fmt |
				       RADEON_ZBLOCK16);

   switch ( driQueryOptioni( &rmesa->optionCache, "dither_mode" ) ) {
   case DRI_CONF_DITHER_XERRORDIFFRESET:
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= RADEON_DITHER_INIT;
      break;
   case DRI_CONF_DITHER_ORDERED:
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= RADEON_SCALE_DITHER_ENABLE;
      break;
   }
   if ( driQueryOptioni( &rmesa->optionCache, "round_mode" ) ==
	DRI_CONF_ROUND_ROUND )
      rmesa->state.color.roundEnable = RADEON_ROUND_ENABLE;
   else
      rmesa->state.color.roundEnable = 0;
   if ( driQueryOptioni (&rmesa->optionCache, "color_reduction" ) ==
	DRI_CONF_COLOR_REDUCTION_DITHER )
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= RADEON_DITHER_ENABLE;
   else
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |= rmesa->state.color.roundEnable;

   rmesa->hw.ctx.cmd[CTX_RB3D_COLOROFFSET] = ((drawOffset +
					       rmesa->radeonScreen->fbLocation)
					      & RADEON_COLOROFFSET_MASK);

   rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] = ((drawPitch &
					      RADEON_COLORPITCH_MASK) |
					     RADEON_COLOR_ENDIAN_NO_SWAP);


   /* (fixed size) sarea is initialized to zero afaics so can omit version check. Phew! */
   if (rmesa->sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |= RADEON_COLOR_TILE_ENABLE;
   }

   rmesa->hw.set.cmd[SET_SE_CNTL] = (RADEON_FFACE_CULL_CCW |
				     RADEON_BFACE_SOLID |
				     RADEON_FFACE_SOLID |
/*  			     RADEON_BADVTX_CULL_DISABLE | */
				     RADEON_FLAT_SHADE_VTX_LAST |
				     RADEON_DIFFUSE_SHADE_GOURAUD |
				     RADEON_ALPHA_SHADE_GOURAUD |
				     RADEON_SPECULAR_SHADE_GOURAUD |
				     RADEON_FOG_SHADE_GOURAUD |
				     RADEON_VPORT_XY_XFORM_ENABLE |
				     RADEON_VPORT_Z_XFORM_ENABLE |
				     RADEON_VTX_PIX_CENTER_OGL |
				     RADEON_ROUND_MODE_TRUNC |
				     RADEON_ROUND_PREC_8TH_PIX);

   rmesa->hw.set.cmd[SET_SE_CNTL_STATUS] =
#ifdef MESA_BIG_ENDIAN
					    RADEON_VC_32BIT_SWAP;
#else
  					    RADEON_VC_NO_SWAP;
#endif

   if (!(rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
     rmesa->hw.set.cmd[SET_SE_CNTL_STATUS] |= RADEON_TCL_BYPASS;
   }

   rmesa->hw.set.cmd[SET_SE_COORDFMT] = (
      RADEON_VTX_W0_IS_NOT_1_OVER_W0 |
      RADEON_TEX1_W_ROUTING_USE_Q1);


   rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] = ((1 << 16) | 0xffff);

   rmesa->hw.lin.cmd[LIN_RE_LINE_STATE] = 
      ((0 << RADEON_LINE_CURRENT_PTR_SHIFT) |
       (1 << RADEON_LINE_CURRENT_COUNT_SHIFT));

   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] = (1 << 4);

   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] = 
      ((0x00 << RADEON_STENCIL_REF_SHIFT) |
       (0xff << RADEON_STENCIL_MASK_SHIFT) |
       (0xff << RADEON_STENCIL_WRITEMASK_SHIFT));

   rmesa->hw.msk.cmd[MSK_RB3D_ROPCNTL] = RADEON_ROP_COPY;
   rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] = 0xffffffff;

   rmesa->hw.msc.cmd[MSC_RE_MISC] = 
      ((0 << RADEON_STIPPLE_X_OFFSET_SHIFT) |
       (0 << RADEON_STIPPLE_Y_OFFSET_SHIFT) |
       RADEON_STIPPLE_BIG_BIT_ORDER);

   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZSCALE]  = 0x00000000;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZOFFSET] = 0x00000000;

   for ( i = 0 ; i < ctx->Const.MaxTextureUnits ; i++ ) {
      rmesa->hw.tex[i].cmd[TEX_PP_TXFILTER] = RADEON_BORDER_MODE_OGL;
      rmesa->hw.tex[i].cmd[TEX_PP_TXFORMAT] = 
	  (RADEON_TXFORMAT_ENDIAN_NO_SWAP |
	   RADEON_TXFORMAT_PERSPECTIVE_ENABLE |
	   (i << 24) | /* This is one of RADEON_TXFORMAT_ST_ROUTE_STQ[012] */
	   (2 << RADEON_TXFORMAT_WIDTH_SHIFT) |
	   (2 << RADEON_TXFORMAT_HEIGHT_SHIFT));

      /* Initialize the texture offset to the start of the card texture heap */
      rmesa->hw.tex[i].cmd[TEX_PP_TXOFFSET] =
	  rmesa->radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];

      rmesa->hw.tex[i].cmd[TEX_PP_BORDER_COLOR] = 0;
      rmesa->hw.tex[i].cmd[TEX_PP_TXCBLEND] =  
	  (RADEON_COLOR_ARG_A_ZERO |
	   RADEON_COLOR_ARG_B_ZERO |
	   RADEON_COLOR_ARG_C_CURRENT_COLOR |
	   RADEON_BLEND_CTL_ADD |
	   RADEON_SCALE_1X |
	   RADEON_CLAMP_TX);
      rmesa->hw.tex[i].cmd[TEX_PP_TXABLEND] = 
	  (RADEON_ALPHA_ARG_A_ZERO |
	   RADEON_ALPHA_ARG_B_ZERO |
	   RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
	   RADEON_BLEND_CTL_ADD |
	   RADEON_SCALE_1X |
	   RADEON_CLAMP_TX);
      rmesa->hw.tex[i].cmd[TEX_PP_TFACTOR] = 0;

      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_FACES] = 0;
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_0] =
	  rmesa->radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_1] =
	  rmesa->radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_2] =
	  rmesa->radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_3] =
	  rmesa->radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
      rmesa->hw.cube[i].cmd[CUBE_PP_CUBIC_OFFSET_4] =
	  rmesa->radeonScreen->texOffset[RADEON_LOCAL_TEX_HEAP];
   }

   /* Can only add ST1 at the time of doing some multitex but can keep
    * it after that.  Errors if DIFFUSE is missing.
    */
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] = 
      (RADEON_TCL_VTX_Z0 |
       RADEON_TCL_VTX_W0 |
       RADEON_TCL_VTX_PK_DIFFUSE
	 );	/* need to keep this uptodate */
						   
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXSEL] =
      ( RADEON_TCL_COMPUTE_XYZW 	|
	(RADEON_TCL_TEX_INPUT_TEX_0 << RADEON_TCL_TEX_0_OUTPUT_SHIFT) |
	(RADEON_TCL_TEX_INPUT_TEX_1 << RADEON_TCL_TEX_1_OUTPUT_SHIFT) |
	(RADEON_TCL_TEX_INPUT_TEX_2 << RADEON_TCL_TEX_2_OUTPUT_SHIFT));


   /* XXX */
   rmesa->hw.tcl.cmd[TCL_MATRIX_SELECT_0] = 
      ((MODEL << RADEON_MODELVIEW_0_SHIFT) |
       (MODEL_IT << RADEON_IT_MODELVIEW_0_SHIFT));

   rmesa->hw.tcl.cmd[TCL_MATRIX_SELECT_1] = 
      ((MODEL_PROJ << RADEON_MODELPROJECT_0_SHIFT) |
       (TEXMAT_0 << RADEON_TEXMAT_0_SHIFT) |
       (TEXMAT_1 << RADEON_TEXMAT_1_SHIFT) |
       (TEXMAT_2 << RADEON_TEXMAT_2_SHIFT));

   rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] = 
      (RADEON_UCP_IN_CLIP_SPACE |
       RADEON_CULL_FRONT_IS_CCW);

   rmesa->hw.tcl.cmd[TCL_TEXTURE_PROC_CTL] = 0; 

   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL] = 
      (RADEON_SPECULAR_LIGHTS |
       RADEON_DIFFUSE_SPECULAR_COMBINE |
       RADEON_LOCAL_LIGHT_VEC_GL |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_EMISSIVE_SOURCE_SHIFT) |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_AMBIENT_SOURCE_SHIFT) |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_DIFFUSE_SOURCE_SHIFT) |
       (RADEON_LM_SOURCE_STATE_MULT << RADEON_SPECULAR_SOURCE_SHIFT));

   for (i = 0 ; i < 8; i++) {
      struct gl_light *l = &ctx->Light.Light[i];
      GLenum p = GL_LIGHT0 + i;
      *(float *)&(rmesa->hw.lit[i].cmd[LIT_RANGE_CUTOFF]) = FLT_MAX;

      ctx->Driver.Lightfv( ctx, p, GL_AMBIENT, l->Ambient );
      ctx->Driver.Lightfv( ctx, p, GL_DIFFUSE, l->Diffuse );
      ctx->Driver.Lightfv( ctx, p, GL_SPECULAR, l->Specular );
      ctx->Driver.Lightfv( ctx, p, GL_POSITION, NULL );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_DIRECTION, NULL );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_EXPONENT, &l->SpotExponent );
      ctx->Driver.Lightfv( ctx, p, GL_SPOT_CUTOFF, &l->SpotCutoff );
      ctx->Driver.Lightfv( ctx, p, GL_CONSTANT_ATTENUATION,
			   &l->ConstantAttenuation );
      ctx->Driver.Lightfv( ctx, p, GL_LINEAR_ATTENUATION, 
			   &l->LinearAttenuation );
      ctx->Driver.Lightfv( ctx, p, GL_QUADRATIC_ATTENUATION, 
		     &l->QuadraticAttenuation );
      *(float *)&(rmesa->hw.lit[i].cmd[LIT_ATTEN_XXX]) = 0.0;
   }

   ctx->Driver.LightModelfv( ctx, GL_LIGHT_MODEL_AMBIENT, 
			     ctx->Light.Model.Ambient );

   TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );

   for (i = 0 ; i < 6; i++) {
      ctx->Driver.ClipPlane( ctx, GL_CLIP_PLANE0 + i, NULL );
   }

   ctx->Driver.Fogfv( ctx, GL_FOG_MODE, NULL );
   ctx->Driver.Fogfv( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
   ctx->Driver.Fogfv( ctx, GL_FOG_START, &ctx->Fog.Start );
   ctx->Driver.Fogfv( ctx, GL_FOG_END, &ctx->Fog.End );
   ctx->Driver.Fogfv( ctx, GL_FOG_COLOR, ctx->Fog.Color );
   ctx->Driver.Fogfv( ctx, GL_FOG_COORDINATE_SOURCE_EXT, NULL );
   
   rmesa->hw.grd.cmd[GRD_VERT_GUARD_CLIP_ADJ] = IEEE_ONE;
   rmesa->hw.grd.cmd[GRD_VERT_GUARD_DISCARD_ADJ] = IEEE_ONE;
   rmesa->hw.grd.cmd[GRD_HORZ_GUARD_CLIP_ADJ] = IEEE_ONE;
   rmesa->hw.grd.cmd[GRD_HORZ_GUARD_DISCARD_ADJ] = IEEE_ONE;

   rmesa->hw.eye.cmd[EYE_X] = 0;
   rmesa->hw.eye.cmd[EYE_Y] = 0;
   rmesa->hw.eye.cmd[EYE_Z] = IEEE_ONE;
   rmesa->hw.eye.cmd[EYE_RESCALE_FACTOR] = IEEE_ONE;
   
   rmesa->hw.all_dirty = GL_TRUE;
}
