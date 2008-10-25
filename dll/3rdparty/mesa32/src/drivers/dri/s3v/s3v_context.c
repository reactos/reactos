/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "context.h"
#include "simple_list.h"
#include "matrix.h"
#include "extensions.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif
#include "simple_list.h"
#include "mm.h"

#include "drivers/common/driverfuncs.h"
#include "s3v_vb.h"
#include "s3v_tris.h"

#if 0
extern const struct tnl_pipeline_stage _s3v_render_stage;

static const struct tnl_pipeline_stage *s3v_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
				/* REMOVE: point attenuation stage */
#if 1
   &_s3v_render_stage,	/* ADD: unclipped rastersetup-to-dma */
#endif
   &_tnl_render_stage,
   0,
};
#endif

GLboolean s3vCreateContext(const __GLcontextModes *glVisual,
			   __DRIcontextPrivate *driContextPriv,
                     	   void *sharedContextPrivate)
{
	GLcontext *ctx, *shareCtx;
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
	s3vContextPtr vmesa;
	s3vScreenPtr s3vScrn;
	S3VSAREAPtr saPriv=(S3VSAREAPtr)(((char*)sPriv->pSAREA) + 
                            sizeof(drm_sarea_t));
        struct dd_function_table functions;

	DEBUG_WHERE(("*** s3vCreateContext ***\n"));

	vmesa = (s3vContextPtr) CALLOC( sizeof(*vmesa) );
	if ( !vmesa ) return GL_FALSE;

	/* Allocate the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((s3vContextPtr) sharedContextPrivate)->glCtx;
	else
		shareCtx = NULL;

        _mesa_init_driver_functions(&functions);

	vmesa->glCtx = _mesa_create_context(glVisual, shareCtx, &functions,
                                            (void *)vmesa);
	if (!vmesa->glCtx) {
		FREE(vmesa);
		return GL_FALSE;
	}

	vmesa->driContext = driContextPriv;
	vmesa->driScreen = sPriv;
	vmesa->driDrawable = NULL; /* Set by XMesaMakeCurrent */

	vmesa->hHWContext = driContextPriv->hHWContext;
	vmesa->driHwLock = (drmLock *)&sPriv->pSAREA->lock;
	vmesa->driFd = sPriv->fd;
	vmesa->sarea = saPriv;

	s3vScrn = vmesa->s3vScreen = (s3vScreenPtr)(sPriv->private);

	ctx = vmesa->glCtx;

	ctx->Const.MaxTextureLevels = 11;  /* it is (11-1) -> 1024 * 1024 FIXME */

	ctx->Const.MaxTextureUnits = 1; /* FIXME: or 2 ? */

	/* No wide points.
	 */
	ctx->Const.MinPointSize = 1.0;
	ctx->Const.MinPointSizeAA = 1.0;
	ctx->Const.MaxPointSize = 1.0;
	ctx->Const.MaxPointSizeAA = 1.0;

	/* No wide lines.
	 */
	ctx->Const.MinLineWidth = 1.0;
	ctx->Const.MinLineWidthAA = 1.0;
	ctx->Const.MaxLineWidth = 1.0;
	ctx->Const.MaxLineWidthAA = 1.0;
	ctx->Const.LineWidthGranularity = 1.0;

	vmesa->texHeap = mmInit( 0, vmesa->s3vScreen->textureSize );
	DEBUG(("vmesa->s3vScreen->textureSize = 0x%x\n",
		vmesa->s3vScreen->textureSize));
	
	/* NOTE */
	/* mmInit(offset, size); */

	/* allocates a structure like this:

	struct mem_block_t {
		struct mem_block_t *next;
		struct mem_block_t *heap;
		int ofs,size;
		int align;
		int free:1;
		int reserved:1;
	};

	*/

	make_empty_list(&vmesa->TexObjList);
	make_empty_list(&vmesa->SwappedOut);

	vmesa->CurrentTexObj[0] = 0;
	vmesa->CurrentTexObj[1] = 0; /* FIXME */

	vmesa->RenderIndex = ~0;

	/* Initialize the software rasterizer and helper modules.
	 */
	_swrast_CreateContext( ctx );
	_vbo_CreateContext( ctx );
	_tnl_CreateContext( ctx );
	_swsetup_CreateContext( ctx );

	/* Install the customized pipeline:
	 */
#if 0
	_tnl_destroy_pipeline( ctx );
	_tnl_install_pipeline( ctx, s3v_pipeline );
#endif
	/* Configure swrast to match hardware characteristics:
	 */
#if 0
	_swrast_allow_pixel_fog( ctx, GL_FALSE );
	_swrast_allow_vertex_fog( ctx, GL_TRUE );
#endif
	vmesa->_3d_mode = 0;

	/* 3D lines / gouraud tris */
	vmesa->CMD = ( AUTO_EXEC_ON | HW_CLIP_ON | DEST_COL_1555
			| FOG_OFF | ALPHA_OFF | Z_OFF | Z_UPDATE_OFF
			| Z_LESS | TEX_WRAP_ON | TEX_MODULATE | LINEAR
			| TEX_COL_ARGB1555 | CMD_3D );

	vmesa->_alpha[0] = vmesa->_alpha[1] = ALPHA_OFF;
	vmesa->alpha_cmd = vmesa->_alpha[0];
	vmesa->_tri[0] = DO_GOURAUD_TRI;
	vmesa->_tri[1] = DO_TEX_LIT_TRI;
	vmesa->prim_cmd = vmesa->_tri[0];

	/* printf("first vmesa->CMD = 0x%x\n", vmesa->CMD); */

	vmesa->TexOffset = vmesa->s3vScreen->texOffset;

	s3vInitVB( ctx );
	s3vInitExtensions( ctx );
	s3vInitDriverFuncs( ctx );
	s3vInitStateFuncs( ctx );
	s3vInitSpanFuncs( ctx );
	s3vInitTextureFuncs( ctx );
	s3vInitTriFuncs( ctx );
	s3vInitState( vmesa );

	driContextPriv->driverPrivate = (void *)vmesa;

	/* HACK */
	vmesa->bufSize = S3V_DMA_BUF_SZ;

	DEBUG(("vmesa->bufSize = %i\n", vmesa->bufSize));
	DEBUG(("vmesa->bufCount = %i\n", vmesa->bufCount));


	/* dma init */
	DEBUG_BUFS(("GET_FIRST_DMA\n"));
	
	vmesa->_bufNum = 0;

	GET_FIRST_DMA(vmesa->driFd, vmesa->hHWContext,
	1, &(vmesa->bufIndex[0]), &(vmesa->bufSize),
	&vmesa->_buf[0], &vmesa->bufCount, s3vScrn);

	GET_FIRST_DMA(vmesa->driFd, vmesa->hHWContext,
    1, &(vmesa->bufIndex[1]), &(vmesa->bufSize),
    &vmesa->_buf[1], &vmesa->bufCount, s3vScrn);

	vmesa->buf = vmesa->_buf[vmesa->_bufNum];
	
/*
	vmesa->CMD = (AUTO_EXEC_ON | HW_CLIP_ON | DEST_COL_1555
	| FOG_OFF | ALPHA_OFF | Z_OFF | Z_UPDATE_OFF
	| DO_GOURAUD_TRI | CMD_3D);

	vmesa->TexOffset = vmesa->s3vScreen->texOffset;
*/

/* ... but we should support only 15 bit in virge (out of 8/15/24)... */

	DEBUG(("glVisual->depthBits = %i\n", glVisual->depthBits));

	switch (glVisual->depthBits) {
	case 8:
		break;
	
	case 15:
	case 16:
		vmesa->depth_scale = 1.0f / 0xffff; 
		break;
	case 24:
		vmesa->depth_scale = 1.0f / 0xffffff;
		break;
	default:
		break;
	}

	vmesa->cull_zero = 0.0f;

	vmesa->DepthSize = glVisual->depthBits;
	vmesa->Flags  = S3V_FRONT_BUFFER;
	vmesa->Flags |= (glVisual->doubleBufferMode ? S3V_BACK_BUFFER : 0);
	vmesa->Flags |= (vmesa->DepthSize > 0 ? S3V_DEPTH_BUFFER : 0);

	vmesa->EnabledFlags = S3V_FRONT_BUFFER;
	vmesa->EnabledFlags |= (glVisual->doubleBufferMode ? S3V_BACK_BUFFER : 0);


	if (vmesa->Flags & S3V_BACK_BUFFER) {
       	vmesa->readOffset = vmesa->drawOffset = vmesa->s3vScreen->backOffset;
	} else {
	   	vmesa->readOffset = vmesa->drawOffset = 0;
	}

	s3vInitHW( vmesa );

	driContextPriv->driverPrivate = (void *)vmesa;

	return GL_TRUE;
}
