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

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
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
#include "array_cache/acache.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_ioctl.h"
#include "radeon_span.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "r300_tex.h"

#include "vblank.h"
#include "utils.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */

/* hw_tcl_on derives from future_hw_tcl_on when its safe to change it. */
int future_hw_tcl_on=0;
int hw_tcl_on=0;

#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_secondary_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_NV_vertex_program
#include "extension_helper.h"

const struct dri_extension card_extensions[] = {
  {"GL_ARB_multisample",		GL_ARB_multisample_functions},
  {"GL_ARB_multitexture",		NULL},
  {"GL_ARB_texture_border_clamp",	NULL},
  {"GL_ARB_texture_compression",	GL_ARB_texture_compression_functions},
/* disable until we support it, fixes a few things in ut2004 */
/*	{"GL_ARB_texture_cube_map",	NULL}, */
  {"GL_ARB_texture_env_add",		NULL},
  {"GL_ARB_texture_env_combine",	NULL},
  {"GL_ARB_texture_env_crossbar",	NULL},
  {"GL_ARB_texture_env_dot3",		NULL},
  {"GL_ARB_texture_mirrored_repeat",	NULL},
  {"GL_ARB_vertex_buffer_object",	GL_ARB_vertex_buffer_object_functions},
  {"GL_ARB_vertex_program",		GL_ARB_vertex_program_functions},
#if USE_ARB_F_P == 1
  {"GL_ARB_fragment_program",		NULL},
#endif
  {"GL_EXT_blend_equation_separate",	GL_EXT_blend_equation_separate_functions},
  {"GL_EXT_blend_func_separate",	GL_EXT_blend_func_separate_functions},
  {"GL_EXT_blend_minmax",		GL_EXT_blend_minmax_functions},
  {"GL_EXT_blend_subtract",		NULL},
  {"GL_EXT_secondary_color", 		GL_EXT_secondary_color_functions},
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
  {"GL_NV_blend_square",		NULL},
  {"GL_NV_vertex_program",		GL_NV_vertex_program_functions},
  {"GL_SGIS_generate_mipmap",		NULL},
  {NULL,				NULL}
};

extern struct tnl_pipeline_stage _r300_render_stage;
extern struct tnl_pipeline_stage _r300_tcl_stage;

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
	&_tnl_arb_vertex_program_stage,
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

static void r300BufferData(GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		const GLvoid *data, GLenum usage, struct gl_buffer_object *obj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	drm_radeon_mem_alloc_t alloc;
	int offset, ret;

	/* Free previous buffer */
	if (obj->OnCard) {
		drm_radeon_mem_free_t memfree;
		
		memfree.region = RADEON_MEM_REGION_GART;
		memfree.region_offset = (char *)obj->Data - (char *)rmesa->radeon.radeonScreen->gartTextures.map;

		ret = drmCommandWrite(rmesa->radeon.radeonScreen->driScreen->fd,
				      DRM_RADEON_FREE, &memfree, sizeof(memfree));

		if (ret) {
			WARN_ONCE("Failed to free GART memroy!\n");
		}
		obj->OnCard = GL_FALSE;
	}
	
	alloc.region = RADEON_MEM_REGION_GART;
	alloc.alignment = 4;
	alloc.size = size;
	alloc.region_offset = &offset;

	ret = drmCommandWriteRead( rmesa->radeon.dri.fd, DRM_RADEON_ALLOC, &alloc, sizeof(alloc));
   	if (ret) {
		WARN_ONCE("Ran out of GART memory!\n");
		obj->Data = NULL;
		_mesa_buffer_data(ctx, target, size, data, usage, obj);
		return ;
	}
	obj->Data = ((char *)rmesa->radeon.radeonScreen->gartTextures.map) + offset;
	
	if (data)
		memcpy(obj->Data, data, size);
	
	obj->Size = size;
	obj->Usage = usage;
	obj->OnCard = GL_TRUE;
#if 0
	fprintf(stderr, "allocated %d bytes at %p, offset=%d\n", size, obj->Data, offset);
#endif
}

static void r300DeleteBuffer(GLcontext *ctx, struct gl_buffer_object *obj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	
	if(r300IsGartMemory(rmesa, obj->Data, obj->Size)){
		drm_radeon_mem_free_t memfree;
		int ret;
		
		memfree.region = RADEON_MEM_REGION_GART;
		memfree.region_offset = (char *)obj->Data - (char *)rmesa->radeon.radeonScreen->gartTextures.map;

		ret = drmCommandWrite(rmesa->radeon.radeonScreen->driScreen->fd,
				      DRM_RADEON_FREE, &memfree, sizeof(memfree));

		if(ret){
			WARN_ONCE("Failed to free GART memroy!\n");
		}
		obj->Data = NULL;
	}
	_mesa_delete_buffer_object(ctx, obj);
}

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
	r300 = (r300ContextPtr)CALLOC(sizeof(*r300));
	if (!r300)
		return GL_FALSE;

	/* Parse configuration files.
	 * Do this here so that initialMaxAnisotropy is set before we create
	 * the default textures.
	 */
	driParseConfigFiles(&r300->radeon.optionCache, &screen->optionCache,
			    screen->driScreen->myNum, "r300");

	/* Init default driver functions then plug in our R300-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions(&functions);
	r300InitIoctlFuncs(&functions);
	r300InitStateFuncs(&functions);
	r300InitTextureFuncs(&functions);
	r300InitShaderFuncs(&functions);
	
#if 0 /* Needs various Mesa changes... */
	if (hw_tcl_on) {
		functions.BufferData = r300BufferData;
		functions.DeleteBuffer = r300DeleteBuffer;
	}
#endif
	
	if (!radeonInitContext(&r300->radeon, &functions,
			       glVisual, driContextPriv, sharedContextPrivate)) {
		FREE(r300);
		return GL_FALSE;
	}

	/* Init r300 context data */
	r300->dma.buf0_address = r300->radeon.radeonScreen->buffers->list[0].address;

	(void)memset(r300->texture_heaps, 0, sizeof(r300->texture_heaps));
	make_empty_list(&r300->swapped);

	r300->nr_heaps = 1 /* screen->numTexHeaps */ ;
	assert(r300->nr_heaps < R200_NR_TEX_HEAPS);
	for (i = 0; i < r300->nr_heaps; i++) {
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
	
	ctx->Const.MaxTextureImageUnits = driQueryOptioni(&r300->radeon.optionCache,
						     "texture_image_units");
	ctx->Const.MaxTextureCoordUnits = driQueryOptioni(&r300->radeon.optionCache,
						     "texture_coord_units");
	ctx->Const.MaxTextureUnits = MIN2(ctx->Const.MaxTextureImageUnits,
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

	/* Initialize the software rasterizer and helper modules.
	 */
	_swrast_CreateContext(ctx);
	_ac_CreateContext(ctx);
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
	_tnl_isolate_materials(ctx, GL_TRUE);

	/* Configure swrast and TNL to match hardware characteristics:
	 */
	_swrast_allow_pixel_fog(ctx, GL_FALSE);
	_swrast_allow_vertex_fog(ctx, GL_TRUE);
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_TRUE);

	/* currently bogus data */
	ctx->Const.MaxVertexProgramInstructions=VSF_MAX_FRAGMENT_LENGTH;
	ctx->Const.MaxVertexProgramAttribs=16; // r420
	ctx->Const.MaxVertexProgramTemps=VSF_MAX_FRAGMENT_TEMPS;
	ctx->Const.MaxVertexProgramLocalParams=256; // r420
	ctx->Const.MaxVertexProgramEnvParams=256; // r420
	ctx->Const.MaxVertexProgramAddressRegs=1;

#if USE_ARB_F_P
	ctx->Const.MaxFragmentProgramTemps = PFS_NUM_TEMP_REGS;
	ctx->Const.MaxFragmentProgramAttribs = 11; /* copy i915... */
	ctx->Const.MaxFragmentProgramLocalParams = PFS_NUM_CONST_REGS;
	ctx->Const.MaxFragmentProgramEnvParams = PFS_NUM_CONST_REGS;
	ctx->Const.MaxFragmentProgramAluInstructions = PFS_MAX_ALU_INST;
	ctx->Const.MaxFragmentProgramTexInstructions = PFS_MAX_TEX_INST;
	ctx->Const.MaxFragmentProgramInstructions = PFS_MAX_ALU_INST+PFS_MAX_TEX_INST;
	ctx->Const.MaxFragmentProgramTexIndirections = PFS_MAX_TEX_INDIRECT;
	ctx->Const.MaxFragmentProgramAddressRegs = 0; /* and these are?? */
	ctx->_MaintainTexEnvProgram = GL_TRUE;
#endif

	driInitExtensions(ctx, card_extensions, GL_TRUE);
	
	radeonInitSpanFuncs(ctx);
	r300InitCmdBuf(r300);
	r300InitState(r300);

#if 0
	/* plug in a few more device driver functions */
	/* XXX these should really go right after _mesa_init_driver_functions() */
	r300InitPixelFuncs(ctx);
	r300InitSwtcl(ctx);
#endif
	TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

	tcl_mode = driQueryOptioni(&r300->radeon.optionCache, "tcl_mode");
	if (driQueryOptionb(&r300->radeon.optionCache, "no_rast")) {
		fprintf(stderr, "disabling 3D acceleration\n");
#if R200_MERGED
		FALLBACK(&r300->radeon, RADEON_FALLBACK_DISABLE, 1);
#endif
	}
	if (tcl_mode == DRI_CONF_TCL_SW ||
	    !(r300->radeon.radeonScreen->chipset & RADEON_CHIPSET_TCL)) {
		if (r300->radeon.radeonScreen->chipset & RADEON_CHIPSET_TCL) {
			r300->radeon.radeonScreen->chipset &= ~RADEON_CHIPSET_TCL;
			fprintf(stderr, "Disabling HW TCL support\n");
		}
		TCL_FALLBACK(r300->radeon.glCtx, RADEON_TCL_FALLBACK_TCL_DISABLE, 1);
	}

	return GL_TRUE;
}

/* Destroy the device specific context.
 */
void r300DestroyContext(__DRIcontextPrivate * driContextPriv)
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr r300 = (r300ContextPtr) driContextPriv->driverPrivate;
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

		release_texture_heaps = (r300->radeon.glCtx->Shared->RefCount == 1);
		_swsetup_DestroyContext(r300->radeon.glCtx);
		_tnl_DestroyContext(r300->radeon.glCtx);
		_ac_DestroyContext(r300->radeon.glCtx);
		_swrast_DestroyContext(r300->radeon.glCtx);

		r300DestroyCmdBuf(r300);

		radeonCleanupContext(&r300->radeon);

		/* free the option cache */
		driDestroyOptionCache(&r300->radeon.optionCache);

		FREE(r300);
	}
}
