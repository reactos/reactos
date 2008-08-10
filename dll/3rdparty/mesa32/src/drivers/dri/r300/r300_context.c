/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "extensions.h"
#include "state.h"
#include "bufferobj.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vp_build.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_ioctl.h"
#include "radeon_span.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "r300_tex.h"
#include "r300_emit.h"

#ifdef USER_BUFFERS
#include "r300_mem.h"
#endif

#include "vblank.h"
#include "utils.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */

/* hw_tcl_on derives from future_hw_tcl_on when its safe to change it. */
int future_hw_tcl_on = 1;
int hw_tcl_on = 1;

#define need_GL_EXT_stencil_two_side
#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_EXT_blend_minmax
//#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_gpu_program_parameters
#define need_GL_NV_vertex_program
#include "extension_helper.h"

const struct dri_extension card_extensions[] = {
  /* *INDENT-OFF* */
  {"GL_ARB_multisample",		GL_ARB_multisample_functions},
  {"GL_ARB_multitexture",		NULL},
  {"GL_ARB_texture_border_clamp",	NULL},
  {"GL_ARB_texture_compression",	GL_ARB_texture_compression_functions},
  {"GL_ARB_texture_cube_map",		NULL},
  {"GL_ARB_texture_env_add",		NULL},
  {"GL_ARB_texture_env_combine",	NULL},
  {"GL_ARB_texture_env_crossbar",	NULL},
  {"GL_ARB_texture_env_dot3",		NULL},
  {"GL_ARB_texture_mirrored_repeat",	NULL},
  {"GL_ARB_vertex_buffer_object",	GL_ARB_vertex_buffer_object_functions},
  {"GL_ARB_vertex_program",		GL_ARB_vertex_program_functions},
  {"GL_ARB_fragment_program",		NULL},
  {"GL_EXT_blend_equation_separate",	GL_EXT_blend_equation_separate_functions},
  {"GL_EXT_blend_func_separate",	GL_EXT_blend_func_separate_functions},
  {"GL_EXT_blend_minmax",		GL_EXT_blend_minmax_functions},
  {"GL_EXT_blend_subtract",		NULL},
//  {"GL_EXT_fog_coord",			GL_EXT_fog_coord_functions },
  {"GL_EXT_gpu_program_parameters",     GL_EXT_gpu_program_parameters_functions},
  {"GL_EXT_secondary_color", 		GL_EXT_secondary_color_functions},
  {"GL_EXT_stencil_two_side",		GL_EXT_stencil_two_side_functions},
  {"GL_EXT_stencil_wrap",		NULL},
  {"GL_EXT_texture_edge_clamp",		NULL},
  {"GL_EXT_texture_env_combine", 	NULL},
  {"GL_EXT_texture_env_dot3", 		NULL},
  {"GL_EXT_texture_filter_anisotropic",	NULL},
  {"GL_EXT_texture_lod_bias",		NULL},
  {"GL_EXT_texture_mirror_clamp",	NULL},
  {"GL_EXT_texture_rectangle",		NULL},
  {"GL_ATI_texture_env_combine3",	NULL},
  {"GL_ATI_texture_mirror_once",	NULL},
  {"GL_MESA_pack_invert",		NULL},
  {"GL_MESA_ycbcr_texture",		NULL},
  {"GL_MESAX_texture_float",		NULL},
  {"GL_NV_blend_square",		NULL},
  {"GL_NV_vertex_program",		GL_NV_vertex_program_functions},
  {"GL_SGIS_generate_mipmap",		NULL},
  {NULL,				NULL}
  /* *INDENT-ON* */
};

extern struct tnl_pipeline_stage _r300_render_stage;
extern const struct tnl_pipeline_stage _r300_tcl_stage;

static const struct tnl_pipeline_stage *r300_pipeline[] = {

	/* Try and go straight to t&l
	 */
	&_r300_tcl_stage,

	/* Catch any t&l fallbacks
	 */
	&_tnl_vertex_transform_stage,
	&_tnl_normal_transform_stage,
	&_tnl_lighting_stage,
	&_tnl_fog_coordinate_stage,
	&_tnl_texgen_stage,
	&_tnl_texture_transform_stage,
	&_tnl_vertex_program_stage,

	/* Try again to go to tcl?
	 *     - no good for asymmetric-twoside (do with multipass)
	 *     - no good for asymmetric-unfilled (do with multipass)
	 *     - good for material
	 *     - good for texgen
	 *     - need to manipulate a bit of state
	 *
	 * - worth it/not worth it?
	 */

	/* Else do them here.
	 */
	&_r300_render_stage,
	&_tnl_render_stage,	/* FALLBACK  */
	0,
};

/* Create the device specific rendering context.
 */
GLboolean r300CreateContext(const __GLcontextModes * glVisual,
			    __DRIcontextPrivate * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	struct dd_function_table functions;
	r300ContextPtr r300;
	GLcontext *ctx;
	int tcl_mode, i;

	assert(glVisual);
	assert(driContextPriv);
	assert(screen);

	/* Allocate the R300 context */
	r300 = (r300ContextPtr) CALLOC(sizeof(*r300));
	if (!r300)
		return GL_FALSE;

	if (!(screen->chip_flags & RADEON_CHIPSET_TCL))
		hw_tcl_on = future_hw_tcl_on = 0;

	/* Parse configuration files.
	 * Do this here so that initialMaxAnisotropy is set before we create
	 * the default textures.
	 */
	driParseConfigFiles(&r300->radeon.optionCache, &screen->optionCache,
			    screen->driScreen->myNum, "r300");
	r300->initialMaxAnisotropy = driQueryOptionf(&r300->radeon.optionCache,
						     "def_max_anisotropy");

	/* Init default driver functions then plug in our R300-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions(&functions);
	r300InitIoctlFuncs(&functions);
	r300InitStateFuncs(&functions);
	r300InitTextureFuncs(&functions);
	r300InitShaderFuncs(&functions);

#ifdef USER_BUFFERS
	r300_mem_init(r300);
#endif

	if (!radeonInitContext(&r300->radeon, &functions,
			       glVisual, driContextPriv,
			       sharedContextPrivate)) {
		FREE(r300);
		return GL_FALSE;
	}

	/* Init r300 context data */
	r300->dma.buf0_address =
	    r300->radeon.radeonScreen->buffers->list[0].address;

	(void)memset(r300->texture_heaps, 0, sizeof(r300->texture_heaps));
	make_empty_list(&r300->swapped);

	r300->nr_heaps = 1 /* screen->numTexHeaps */ ;
	assert(r300->nr_heaps < RADEON_NR_TEX_HEAPS);
	for (i = 0; i < r300->nr_heaps; i++) {
		/* *INDENT-OFF* */
		r300->texture_heaps[i] = driCreateTextureHeap(i, r300,
							       screen->
							       texSize[i], 12,
							       RADEON_NR_TEX_REGIONS,
							       (drmTextureRegionPtr)
							       r300->radeon.sarea->
							       tex_list[i],
							       &r300->radeon.sarea->
							       tex_age[i],
							       &r300->swapped,
							       sizeof
							       (r300TexObj),
							       (destroy_texture_object_t
								*)
							       r300DestroyTexObj);
		/* *INDENT-ON* */
	}
	r300->texture_depth = driQueryOptioni(&r300->radeon.optionCache,
					      "texture_depth");
	if (r300->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
		r300->texture_depth = (screen->cpp == 4) ?
		    DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

	/* Set the maximum texture size small enough that we can guarentee that
	 * all texture units can bind a maximal texture and have them both in
	 * texturable memory at once.
	 */

	ctx = r300->radeon.glCtx;

	ctx->Const.MaxTextureImageUnits =
	    driQueryOptioni(&r300->radeon.optionCache, "texture_image_units");
	ctx->Const.MaxTextureCoordUnits =
	    driQueryOptioni(&r300->radeon.optionCache, "texture_coord_units");
	ctx->Const.MaxTextureUnits =
	    MIN2(ctx->Const.MaxTextureImageUnits,
		 ctx->Const.MaxTextureCoordUnits);
	ctx->Const.MaxTextureMaxAnisotropy = 16.0;

	ctx->Const.MinPointSize = 1.0;
	ctx->Const.MinPointSizeAA = 1.0;
	ctx->Const.MaxPointSize = R300_POINTSIZE_MAX;
	ctx->Const.MaxPointSizeAA = R300_POINTSIZE_MAX;

	ctx->Const.MinLineWidth = 1.0;
	ctx->Const.MinLineWidthAA = 1.0;
	ctx->Const.MaxLineWidth = R300_LINESIZE_MAX;
	ctx->Const.MaxLineWidthAA = R300_LINESIZE_MAX;

#ifdef USER_BUFFERS
	/* Needs further modifications */
#if 0
	ctx->Const.MaxArrayLockSize =
	    ( /*512 */ RADEON_BUFFER_SIZE * 16 * 1024) / (4 * 4);
#endif
#endif

	/* Initialize the software rasterizer and helper modules.
	 */
	_swrast_CreateContext(ctx);
	_vbo_CreateContext(ctx);
	_tnl_CreateContext(ctx);
	_swsetup_CreateContext(ctx);
	_swsetup_Wakeup(ctx);
	_ae_create_context(ctx);

	/* Install the customized pipeline:
	 */
	_tnl_destroy_pipeline(ctx);
	_tnl_install_pipeline(ctx, r300_pipeline);

	/* Try and keep materials and vertices separate:
	 */
/* 	_tnl_isolate_materials(ctx, GL_TRUE); */

	/* Configure swrast and TNL to match hardware characteristics:
	 */
	_swrast_allow_pixel_fog(ctx, GL_FALSE);
	_swrast_allow_vertex_fog(ctx, GL_TRUE);
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_TRUE);

	/* currently bogus data */
	ctx->Const.VertexProgram.MaxInstructions = VSF_MAX_FRAGMENT_LENGTH / 4;
	ctx->Const.VertexProgram.MaxNativeInstructions =
	    VSF_MAX_FRAGMENT_LENGTH / 4;
	ctx->Const.VertexProgram.MaxNativeAttribs = 16;	/* r420 */
	ctx->Const.VertexProgram.MaxTemps = 32;
	ctx->Const.VertexProgram.MaxNativeTemps =
	    /*VSF_MAX_FRAGMENT_TEMPS */ 32;
	ctx->Const.VertexProgram.MaxNativeParameters = 256;	/* r420 */
	ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;

	ctx->Const.FragmentProgram.MaxNativeTemps = PFS_NUM_TEMP_REGS;
	ctx->Const.FragmentProgram.MaxNativeAttribs = 11;	/* copy i915... */
	ctx->Const.FragmentProgram.MaxNativeParameters = PFS_NUM_CONST_REGS;
	ctx->Const.FragmentProgram.MaxNativeAluInstructions = PFS_MAX_ALU_INST;
	ctx->Const.FragmentProgram.MaxNativeTexInstructions = PFS_MAX_TEX_INST;
	ctx->Const.FragmentProgram.MaxNativeInstructions =
	    PFS_MAX_ALU_INST + PFS_MAX_TEX_INST;
	ctx->Const.FragmentProgram.MaxNativeTexIndirections =
	    PFS_MAX_TEX_INDIRECT;
	ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;	/* and these are?? */
	_tnl_ProgramCacheInit(ctx);
	ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

	driInitExtensions(ctx, card_extensions, GL_TRUE);

	if (driQueryOptionb
	    (&r300->radeon.optionCache, "disable_stencil_two_side"))
		_mesa_disable_extension(ctx, "GL_EXT_stencil_two_side");

	if (r300->radeon.glCtx->Mesa_DXTn
	    && !driQueryOptionb(&r300->radeon.optionCache, "disable_s3tc")) {
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
		_mesa_enable_extension(ctx, "GL_S3_s3tc");
	} else
	    if (driQueryOptionb(&r300->radeon.optionCache, "force_s3tc_enable"))
	{
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
	}

	r300->disable_lowimpact_fallback =
	    driQueryOptionb(&r300->radeon.optionCache,
			    "disable_lowimpact_fallback");

	radeonInitSpanFuncs(ctx);
	r300InitCmdBuf(r300);
	r300InitState(r300);

	TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

	tcl_mode = driQueryOptioni(&r300->radeon.optionCache, "tcl_mode");
	if (driQueryOptionb(&r300->radeon.optionCache, "no_rast")) {
		fprintf(stderr, "disabling 3D acceleration\n");
#if R200_MERGED
		FALLBACK(&r300->radeon, RADEON_FALLBACK_DISABLE, 1);
#endif
	}
	if (tcl_mode == DRI_CONF_TCL_SW ||
	    !(r300->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
		if (r300->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
			r300->radeon.radeonScreen->chip_flags &=
			    ~RADEON_CHIPSET_TCL;
			fprintf(stderr, "Disabling HW TCL support\n");
		}
		TCL_FALLBACK(r300->radeon.glCtx,
			     RADEON_TCL_FALLBACK_TCL_DISABLE, 1);
	}

	return GL_TRUE;
}

static void r300FreeGartAllocations(r300ContextPtr r300)
{
	int i, ret, tries = 0, done_age, in_use = 0;
	drm_radeon_mem_free_t memfree;

	memfree.region = RADEON_MEM_REGION_GART;

#ifdef USER_BUFFERS
	for (i = r300->rmm->u_last; i > 0; i--) {
		if (r300->rmm->u_list[i].ptr == NULL) {
			continue;
		}

		/* check whether this buffer is still in use */
		if (r300->rmm->u_list[i].pending) {
			in_use++;
		}
	}
	/* Cannot flush/lock if no context exists. */
	if (in_use)
		r300FlushCmdBuf(r300, __FUNCTION__);

	done_age = radeonGetAge((radeonContextPtr) r300);

	for (i = r300->rmm->u_last; i > 0; i--) {
		if (r300->rmm->u_list[i].ptr == NULL) {
			continue;
		}

		/* check whether this buffer is still in use */
		if (!r300->rmm->u_list[i].pending) {
			continue;
		}

		assert(r300->rmm->u_list[i].h_pending == 0);

		tries = 0;
		while (r300->rmm->u_list[i].age > done_age && tries++ < 1000) {
			usleep(10);
			done_age = radeonGetAge((radeonContextPtr) r300);
		}
		if (tries >= 1000) {
			WARN_ONCE("Failed to idle region!");
		}

		memfree.region_offset = (char *)r300->rmm->u_list[i].ptr -
		    (char *)r300->radeon.radeonScreen->gartTextures.map;

		ret = drmCommandWrite(r300->radeon.radeonScreen->driScreen->fd,
				      DRM_RADEON_FREE, &memfree,
				      sizeof(memfree));
		if (ret) {
			fprintf(stderr, "Failed to free at %p\nret = %s\n",
				r300->rmm->u_list[i].ptr, strerror(-ret));
		} else {
			if (i == r300->rmm->u_last)
				r300->rmm->u_last--;

			r300->rmm->u_list[i].pending = 0;
			r300->rmm->u_list[i].ptr = NULL;
		}
	}
	r300->rmm->u_head = i;
#endif				/* USER_BUFFERS */
}

/* Destroy the device specific context.
 */
void r300DestroyContext(__DRIcontextPrivate * driContextPriv)
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr r300 = (r300ContextPtr) driContextPriv->driverPrivate;
	radeonContextPtr radeon = (radeonContextPtr) r300;
	radeonContextPtr current = ctx ? RADEON_CONTEXT(ctx) : NULL;

	if (RADEON_DEBUG & DEBUG_DRI) {
		fprintf(stderr, "Destroying context !\n");
	}

	/* check if we're deleting the currently bound context */
	if (&r300->radeon == current) {
		radeonFlush(r300->radeon.glCtx);
		_mesa_make_current(NULL, NULL, NULL);
	}

	/* Free r300 context resources */
	assert(r300);		/* should never be null */

	if (r300) {
		GLboolean release_texture_heaps;

		release_texture_heaps =
		    (r300->radeon.glCtx->Shared->RefCount == 1);
		_swsetup_DestroyContext(r300->radeon.glCtx);
		_tnl_ProgramCacheDestroy(r300->radeon.glCtx);
		_tnl_DestroyContext(r300->radeon.glCtx);
		_vbo_DestroyContext(r300->radeon.glCtx);
		_swrast_DestroyContext(r300->radeon.glCtx);

		if (r300->dma.current.buf) {
			r300ReleaseDmaRegion(r300, &r300->dma.current,
					     __FUNCTION__);
#ifndef USER_BUFFERS
			r300FlushCmdBuf(r300, __FUNCTION__);
#endif
		}
		r300FreeGartAllocations(r300);
		r300DestroyCmdBuf(r300);

		if (radeon->state.scissor.pClipRects) {
			FREE(radeon->state.scissor.pClipRects);
			radeon->state.scissor.pClipRects = NULL;
		}

		if (release_texture_heaps) {
			/* This share group is about to go away, free our private
			 * texture object data.
			 */
			int i;

			for (i = 0; i < r300->nr_heaps; i++) {
				driDestroyTextureHeap(r300->texture_heaps[i]);
				r300->texture_heaps[i] = NULL;
			}

			assert(is_empty_list(&r300->swapped));
		}

		radeonCleanupContext(&r300->radeon);

#ifdef USER_BUFFERS
		/* the memory manager might be accessed when Mesa frees the shared
		 * state, so don't destroy it earlier
		 */
		r300_mem_destroy(r300);
#endif

		/* free the option cache */
		driDestroyOptionCache(&r300->radeon.optionCache);

		FREE(r300);
	}
}
