/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include <X11/Xlibint.h>
#include "s3v_context.h"
#include "s3v_macros.h"
#include "macros.h"
#include "s3v_dri.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"

/* #define DEBUG(str) printf str */
#define ENABLELIGHTING 0


/* =============================================================
 * Alpha blending
 */

static void s3vUpdateAlphaMode( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	CARD32 cmd = vmesa->CMD;
	cmd &= ~ALPHA_BLEND_MASK;

	if ( ctx->Color.BlendEnabled ) {
		DEBUG(("ctx->Color.AlphaEnabled = 1"));
		vmesa->_alpha[0] = ALPHA_SRC;
		vmesa->_alpha[1] = vmesa->_alpha_tex; /* FIXME: not all tex modes
							 support alpha */
	} else {
		DEBUG(("ctx->Color.AlphaEnabled = 0"));
		vmesa->_alpha[0] = vmesa->_alpha[1] = ALPHA_OFF;
	}
#if 1
	if ((cmd & DO_MASK) & DO_3D_LINE) { 	/* we are drawing 3d lines */
						/* which don't support tex */
		cmd |= vmesa->_alpha[0];
	} else {
		cmd |= vmesa->_alpha[vmesa->_3d_mode];
	}

	vmesa->CMD = cmd; /* FIXME: enough? */
#else
	vmesa->restore_primitive = -1;
#endif
	
}

static void s3vDDAlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);

   DEBUG(("s3vDDAlphaFunc\n"));

   vmesa->new_state |= S3V_NEW_ALPHA;
}

static void s3vDDBlendFunc( GLcontext *ctx, GLenum sfactor, GLenum dfactor )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);

   DEBUG(("s3vDDBlendFunc\n"));

   vmesa->new_state |= S3V_NEW_ALPHA;
}

/* ================================================================
 * Buffer clear
 */

static void s3vDDClear( GLcontext *ctx, GLbitfield mask )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	unsigned int _stride;
        GLint cx = ctx->DrawBuffer->_Xmin;
        GLint cy = ctx->DrawBuffer->_Ymin;
        GLint cw = ctx->DrawBuffer->_Xmax - cx;
        GLint ch = ctx->DrawBuffer->_Ymax - cy;

        /* XXX FIX ME: the cx,cy,cw,ch vars are currently ignored! */

	vmesa->restore_primitive = -1;

	/* Update and emit any new state.  We need to do this here to catch
	 * changes to the masks.
	 * FIXME: Just update the masks?
	 */

	if ( vmesa->new_state )
		s3vDDUpdateHWState( ctx );

/*	s3vUpdateMasks( ctx ); */
/* 	s3vUpdateClipping( ctx ); */
/*	s3vEmitHwState( vmesa ); */
	

#if 1 /* soft (0)/hw (1)*/

	DEBUG(("*** s3vDDClear ***\n"));

	DMAOUT_CHECK(BITBLT_SRC_BASE, 15);
		DMAOUT(vmesa->SrcBase);
		DMAOUT(vmesa->DestBlit);
		DMAOUT( vmesa->ScissorLR );
		DMAOUT( vmesa->ScissorTB );
		DMAOUT( (vmesa->SrcStride << 16) | vmesa->SrcStride );	/* FIXME: unify */
		DMAOUT( (~(0)) ); /* masks */
		DMAOUT( (~(0)) );
		DMAOUT(0);
		DMAOUT(vmesa->ClearColor);
		DMAOUT(0);
		DMAOUT(0);
		/* FIXME */
		DMAOUT(0x16000122 | 0x5 | (0xF0 << 17));    /* black magic to me */
		DMAOUT(vmesa->ScissorWH);
		DMAOUT(vmesa->SrcXY);
		DMAOUT(vmesa->DestXY);
	DMAFINISH();

	if (mask & BUFFER_BIT_DEPTH) { /* depth */
		DEBUG(("BUFFER_BIT_DEPTH\n"));
		
		_stride = ((cw+31)&~31) * 2; /* XXX cw or Buffer->Width??? */

		DMAOUT_CHECK(BITBLT_SRC_BASE, 15);
	        	DMAOUT(0);
	        	DMAOUT(vmesa->s3vScreen->depthOffset);
			DMAOUT( (0 << 16) | cw );
                	DMAOUT( (0 << 16) | ch );
			DMAOUT( (vmesa->SrcStride << 16) | vmesa->DestStride );
	        	DMAOUT( (~(0)) ); /* masks */
		        DMAOUT( (~(0)) );
		        DMAOUT(0);
		        DMAOUT(vmesa->ClearDepth); /* 0x7FFF */
		        /* FIXME */
		        DMAOUT(0);
		        DMAOUT(0);
		        DMAOUT(0x16000122 | 0x5 | (0xF0 << 17));
		        DMAOUT( ((cw-1) << 16) | (ch-1) );
	        	DMAOUT(0);
		        DMAOUT( (0 << 16) | 0 );
		DMAFINISH();		

		DEBUG(("vmesa->ClearDepth = 0x%x\n", vmesa->ClearDepth));
		mask &= ~BUFFER_BIT_DEPTH;
	}

	if (!vmesa->NotClipped) {
		DEBUG(("vmesa->NotClipped\n")); /* yes */
	}

	if (!(vmesa->EnabledFlags & S3V_BACK_BUFFER)) {
		DEBUG(("!S3V_BACK_BUFFER -> flush\n"));
		DMAFLUSH();
	}
/*
	if ( mask )
		DEBUG(("still masked ;3(\n")); */ /* yes */
#else
      _swrast_Clear( ctx, mask );
#endif
}

/* =============================================================
 * Depth testing
 */

static void s3vUpdateZMode( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	CARD32 cmd = vmesa->CMD;

	DEBUG(("Depth.Test = %i\n", ctx->Depth.Test));
	DEBUG(("CMD was = 0x%x ", cmd));

/*	printf("depth --- CMD was = 0x%x \n", cmd); */

	cmd &= ~Z_MASK; /*  0xfc0fffff; */
	/* Z_BUFFER */ /* 000 mode */ /* Z_UPDATE_OFF */

	if (!ctx->Depth.Test)
		cmd |= Z_OFF;

	if ( ctx->Depth.Mask )
		cmd |= Z_UPDATE_ON;
			
	switch ( ctx->Depth.Func ) {
		case GL_NEVER:
			cmd |= Z_NEVER;
			break;
		case GL_ALWAYS:
			cmd |= Z_ALWAYS;
			break;
		case GL_LESS:
			cmd |= Z_LESS;
			break;
		case GL_LEQUAL:
			cmd |= Z_LEQUAL;
			break;
		case GL_EQUAL:
			cmd |= Z_EQUAL;
			break;
		case GL_GEQUAL:
			cmd |= Z_GEQUAL;
			break;
		case GL_GREATER:
			cmd |= Z_GREATER;
			break;
		case GL_NOTEQUAL:
			cmd |= Z_NOTEQUAL;
			break;
	}

	DEBUG(("CMD is 0x%x\n", cmd));

	vmesa->dirty |= S3V_UPLOAD_DEPTH;
	vmesa->CMD = cmd;
}

static void s3vDDDepthFunc( GLcontext *ctx, GLenum func )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

/*	FLUSH_BATCH( vmesa ); */
	DEBUG(("s3vDDDepthFunc\n"));
	vmesa->new_state |= S3V_NEW_DEPTH;
}

static void s3vDDDepthMask( GLcontext *ctx, GLboolean flag )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	/* FLUSH_BATCH( vmesa ); */
	DEBUG(("s3vDDDepthMask\n"));
	vmesa->new_state |= S3V_NEW_DEPTH;
}

static void s3vDDClearDepth( GLcontext *ctx, GLclampd d )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	switch ( vmesa->DepthSize ) {
	case 15:
	case 16:
		vmesa->ClearDepth = d * 0x0000ffff;	/* 65536 */
		DEBUG(("GLclampd d = %f\n", d));
		DEBUG(("ctx->Depth.Clear = %f\n", ctx->Depth.Clear));
		DEBUG(("(They should be the same)\n"));
		break;
	case 24:
		vmesa->ClearDepth = d * 0x00ffffff;
		break;
	case 32:
		vmesa->ClearDepth = d * 0xffffffff;
		break;
   }
}

static void s3vDDFinish( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	DMAFLUSH(); 
}

static void s3vDDFlush( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	DMAFLUSH();
}

/* =============================================================
 * Fog
 */

static void s3vUpdateFogAttrib( GLcontext *ctx )
{
/*	s3vContextPtr vmesa = S3V_CONTEXT(ctx); */

	if (ctx->Fog.Enabled) {
	} else {
	}

	switch (ctx->Fog.Mode) {
		case GL_LINEAR:
			break;
		case GL_EXP:
			break;
		case GL_EXP2:
			break;
	}
}

static void s3vDDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   vmesa->new_state |= S3V_NEW_FOG;
}

/* =============================================================
 * Lines
 */
static void s3vDDLineWidth( GLcontext *ctx, GLfloat width )
{
	/* FIXME: on virge you only have one size of 3d lines	 *
	 * if we wanted more, we should start using tris instead *
	 * but virge has problem with some tris when all of the  *
	 * vertices stay on a line */
}

/* =============================================================
 * Points
 */
static void s3vDDPointSize( GLcontext *ctx, GLfloat size )
{
	/* FIXME: we use 3d line to fake points. So same limitations
	 * as above apply */
}

/* =============================================================
 * Polygon 
 */

static void s3vUpdatePolygon( GLcontext *ctx )
{
	/* FIXME: I don't think we could do much here */

	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	vmesa->dirty |= S3V_UPLOAD_POLYGON;
}

/* =============================================================
 * Clipping
 */

static void s3vUpdateClipping( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;

	int x0,y0,x1,y1;

	DEBUG((">>> s3vUpdateClipping <<<\n"));
/*
	if ( vmesa->driDrawable ) {
		DEBUG(("s3vUpdateClipping\n"));
*/
	if (vmesa->EnabledFlags & S3V_BACK_BUFFER) {
		DEBUG(("S3V_BACK_BUFFER\n"));

		x0 = 0;
		y0 = 0;
		x1 = dPriv->w - 1;
		y1 = dPriv->h - 1;

		vmesa->SrcBase = 0;
		vmesa->DestBase =  vmesa->s3vScreen->backOffset;
		vmesa->DestBlit = vmesa->DestBase;
	        vmesa->ScissorLR = ( (0 << 16) | (dPriv->w-1) );
	        vmesa->ScissorTB = ( (0 << 16) | (dPriv->h-1) );
/*
	        vmesa->ScissorLR = ( (x0 << 16) | x1 );
	        vmesa->ScissorTB = ( (y0 << 16) | y1 );
*/
	        vmesa->SrcStride = ( ((dPriv->w+31)&~31) * vmesa->s3vScreen->cpp );
		vmesa->DestStride = vmesa->driScreen->fbWidth*vmesa->s3vScreen->cpp;
		vmesa->ScissorWH = ( (dPriv->w << 16) | dPriv->h );
		vmesa->SrcXY = 0;
/*		vmesa->DestXY = ( (dPriv->x << 16) | dPriv->y );  */
		vmesa->DestXY = ( (0 << 16) | 0 );
	} else {
		DEBUG(("S3V_FRONT_BUFFER\n"));

		x0 = dPriv->x;
	        y0 = dPriv->y;
	        x1 = x0 + dPriv->w - 1;
	        y1 = y0 + dPriv->h - 1;

		vmesa->SrcBase = 0;
		vmesa->DestBase = 0;
		vmesa->ScissorLR = ( (x0 << 16) | x1 );
		vmesa->ScissorTB = ( (y0 << 16) | y1 );
		vmesa->DestStride = vmesa->driScreen->fbWidth*vmesa->s3vScreen->cpp;
		vmesa->SrcStride = vmesa->DestStride;
		vmesa->DestBase = (y0 * vmesa->DestStride)
				+ x0*vmesa->s3vScreen->cpp;
		vmesa->DestBlit = 0;
		vmesa->ScissorWH = ( (x1 << 16) | y1 );
		vmesa->SrcXY = 0;
		vmesa->DestXY = ( (0 << 16) | 0 );
/*		vmesa->DestXY = ( (dPriv->x << 16) | dPriv->y ); */
    	}

	DEBUG(("x0=%i y0=%i x1=%i y1=%i\n", x0, y0, x1, y1));
	DEBUG(("stride=%i rectWH=0x%x\n\n", vmesa->DestStride, vmesa->ScissorWH));

	/* FIXME: how could we use the following info? */
	/* if (ctx->Scissor.Enabled) {} */

	vmesa->dirty |= S3V_UPLOAD_CLIP; 
/*	}  */
}

static void s3vDDScissor( GLcontext *ctx,
			   GLint x, GLint y, GLsizei w, GLsizei h )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	DEBUG((">>> s3vDDScissor <<<"));
	/* FLUSH_BATCH( vmesa ); */
	vmesa->new_state |= S3V_NEW_CLIP;
}

/* =============================================================
 * Culling
 */

static void s3vUpdateCull( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	GLfloat backface_sign = 1;

	DEBUG(("s3vUpdateCull\n"));
	/* FIXME: GL_FRONT_AND_BACK */

	switch ( ctx->Polygon.CullFaceMode ) {
	case GL_BACK:
		if (ctx->Polygon.FrontFace == GL_CCW)
			backface_sign = -1;
		break;

	case GL_FRONT:
		if (ctx->Polygon.FrontFace != GL_CCW)
			backface_sign = -1;
		break;

	default:
		break;
	}

	vmesa->backface_sign = backface_sign;
	vmesa->dirty |= S3V_UPLOAD_GEOMETRY;
}


static void s3vDDCullFace( GLcontext *ctx, GLenum mode )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	/* FLUSH_BATCH( vmesa ); */
	vmesa->new_state |= S3V_NEW_CULL;
}

static void s3vDDFrontFace( GLcontext *ctx, GLenum mode )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	/* FLUSH_BATCH( vmesa ); */
	vmesa->new_state |= S3V_NEW_CULL;
}

/* =============================================================
 * Masks
 */

static void s3vUpdateMasks( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	GLuint mask = s3vPackColor( vmesa->s3vScreen->cpp,
				ctx->Color.ColorMask[RCOMP],
				ctx->Color.ColorMask[GCOMP],
				ctx->Color.ColorMask[BCOMP],
				ctx->Color.ColorMask[ACOMP] );

	if (vmesa->s3vScreen->cpp == 2) mask |= mask << 16;

	/* FIXME: can we do something in virge? */
}
/*
static void s3vDDColorMask( GLcontext *ctx, GLboolean r, GLboolean g,
			      GLboolean b, GLboolean a)
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);

   FLUSH_BATCH( vmesa );
   vmesa->new_state |= S3V_NEW_MASKS;
}
*/
/* =============================================================
 * Rendering attributes
 */

/* =============================================================
 * Miscellaneous
 */

static void s3vDDClearColor( GLcontext *ctx, const GLfloat color[4])
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);

   DEBUG(("*** s3vDDClearColor\n"));

   vmesa->ClearColor = s3vPackColor( 2, /* vmesa->s3vScreen->cpp, */
				      color[0], color[1], color[2], color[3] );

#if 0
   if (vmesa->s3vScreen->cpp == 2) vmesa->ClearColor |= vmesa->ClearColor<<16;
#endif
}

static void s3vDDSetDrawBuffer( GLcontext *ctx, GLenum mode )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	int found = GL_TRUE;

	DEBUG(("*** s3vDDSetDrawBuffer ***\n"));

	/* FLUSH_BATCH( vmesa ); */

	switch ( mode ) {
		case GL_FRONT_LEFT:
			vmesa->drawOffset = vmesa->s3vScreen->frontOffset;
			break;
		case GL_BACK_LEFT:
			vmesa->drawOffset = vmesa->s3vScreen->backOffset;
			/* vmesa->driScreen->fbHeight *
			 * vmesa->driScreen->fbWidth *
			 * vmesa->s3vScreen->cpp; */
			break;
		default:
			found = GL_FALSE;
			break;
	}

	DEBUG(("vmesa->drawOffset = 0x%x\n", vmesa->drawOffset));
/*	return GL_TRUE; */
}

/* =============================================================
 * Window position and viewport transformation
 */

void s3vUpdateWindow( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;
	GLfloat xoffset = (GLfloat)dPriv->x;
	GLfloat yoffset =
		vmesa->driScreen->fbHeight - (GLfloat)dPriv->y - dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat sx = v[MAT_SX];
	GLfloat tx = v[MAT_TX] + xoffset;
	GLfloat sy = v[MAT_SY];
	GLfloat ty = v[MAT_TY] + yoffset;
	GLfloat sz = v[MAT_SZ] * vmesa->depth_scale;
	GLfloat tz = v[MAT_TZ] * vmesa->depth_scale;

	vmesa->dirty |= S3V_UPLOAD_VIEWPORT;

	vmesa->ViewportScaleX = sx;
	vmesa->ViewportScaleY = sy;
	vmesa->ViewportScaleZ = sz;
	vmesa->ViewportOffsetX = tx;
	vmesa->ViewportOffsetY = ty;
	vmesa->ViewportOffsetZ = tz;
}


/*
static void s3vDDViewport( GLcontext *ctx, GLint x, GLint y,
			    GLsizei width, GLsizei height )
{
	s3vUpdateWindow( ctx );
}

static void s3vDDDepthRange( GLcontext *ctx, GLclampd nearval,
			      GLclampd farval )
{
	s3vUpdateWindow( ctx );
}
*/
void s3vUpdateViewportOffset( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;
	GLfloat xoffset = (GLfloat)dPriv->x;
	GLfloat yoffset =
		vmesa->driScreen->fbHeight - (GLfloat)dPriv->y - dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat tx = v[MAT_TX] + xoffset;
	GLfloat ty = v[MAT_TY] + yoffset;

	DEBUG(("*** s3vUpdateViewportOffset ***\n"));

	if ( vmesa->ViewportOffsetX != tx ||
		vmesa->ViewportOffsetY != ty )
	{
		vmesa->ViewportOffsetX = tx;
		vmesa->ViewportOffsetY = ty;

		vmesa->new_state |= S3V_NEW_WINDOW;
	}

/*	vmesa->new_state |= S3V_NEW_CLIP; */
}

/* =============================================================
 * State enable/disable
 */

static void s3vDDEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	switch ( cap ) {
	case GL_ALPHA_TEST:
	case GL_BLEND:
		vmesa->new_state |= S3V_NEW_ALPHA;
		DEBUG(("s3vDDEnable: GL_BLEND\n"));
	break;

	case GL_CULL_FACE:
		vmesa->new_state |= S3V_NEW_CULL;
		DEBUG(("s3vDDEnable: GL_CULL_FACE\n"));
	break;

	case GL_DEPTH_TEST:
		vmesa->new_state |= S3V_NEW_DEPTH;
		DEBUG(("s3vDDEnable: GL_DEPTH\n"));
	break;
#if 0
	case GL_FOG:
		vmesa->new_state |= S3V_NEW_FOG;
	break;
#endif

	case GL_SCISSOR_TEST:
		vmesa->new_state |= S3V_NEW_CLIP;
	break;

	case GL_TEXTURE_2D:
	   	DEBUG(("*** GL_TEXTURE_2D: %i\n", state));
		vmesa->_3d_mode = state;
		vmesa->restore_primitive = -1;
	break;
	
	default:
		return;
	}
}

/* =============================================================
 * State initialization, management
 */


/*
 * Load the current context's state into the hardware.
 *
 * NOTE: Be VERY careful about ensuring the context state is marked for
 * upload, the only place it shouldn't be uploaded is when the setup
 * state has changed in ReducedPrimitiveChange as this comes right after
 * a state update.
 *
 * Blits of any type should always upload the context and masks after
 * they are done.
 */
void s3vEmitHwState( s3vContextPtr vmesa )
{
    if (!vmesa->driDrawable) return;
    if (!vmesa->dirty) return; 

	DEBUG(("**********************\n"));
	DEBUG(("*** s3vEmitHwState ***\n"));
	DEBUG(("**********************\n"));

    if (vmesa->dirty & S3V_UPLOAD_VIEWPORT) {
	vmesa->dirty &= ~S3V_UPLOAD_VIEWPORT;
	DEBUG(("S3V_UPLOAD_VIEWPORT\n"));
    }
   
    if ( (vmesa->dirty & S3V_UPLOAD_POINTMODE) ||
	 (vmesa->dirty & S3V_UPLOAD_LINEMODE) ||
	 (vmesa->dirty & S3V_UPLOAD_TRIMODE) ) {

    }
    
    if (vmesa->dirty & S3V_UPLOAD_POINTMODE) {
	vmesa->dirty &= ~S3V_UPLOAD_POINTMODE;
    }

    if (vmesa->dirty & S3V_UPLOAD_LINEMODE) {
	vmesa->dirty &= ~S3V_UPLOAD_LINEMODE;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_TRIMODE) {
	vmesa->dirty &= ~S3V_UPLOAD_TRIMODE;
    }

    if (vmesa->dirty & S3V_UPLOAD_FOG) {
	GLchan c[3], col;
   	UNCLAMPED_FLOAT_TO_RGB_CHAN( c, vmesa->glCtx->Fog.Color );
	DEBUG(("uploading ** FOG **\n"));
	col = s3vPackColor(2, c[0], c[1], c[2], 0);
	vmesa->dirty &= ~S3V_UPLOAD_FOG;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_DITHER) {
	vmesa->dirty &= ~S3V_UPLOAD_DITHER;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_LOGICOP) {
	vmesa->dirty &= ~S3V_UPLOAD_LOGICOP;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_CLIP) {
	vmesa->dirty &= ~S3V_UPLOAD_CLIP;
	DEBUG(("S3V_UPLOAD_CLIP\n"));
	DEBUG(("vmesa->ScissorLR: %i\n",  vmesa->ScissorLR));
	DEBUG(("vmesa->ScissorTB: %i\n",  vmesa->ScissorTB));
    }

    if (vmesa->dirty & S3V_UPLOAD_MASKS) {
	vmesa->dirty &= ~S3V_UPLOAD_MASKS;
	DEBUG(("S3V_UPLOAD_BLEND\n"));
    }
    
    if (vmesa->dirty & S3V_UPLOAD_ALPHA) {
	vmesa->dirty &= ~S3V_UPLOAD_ALPHA;
	DEBUG(("S3V_UPLOAD_ALPHA\n"));
    }
    
    if (vmesa->dirty & S3V_UPLOAD_SHADE) {
	vmesa->dirty &= ~S3V_UPLOAD_SHADE;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_POLYGON) {
	vmesa->dirty &= ~S3V_UPLOAD_POLYGON;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_DEPTH) {
	vmesa->dirty &= ~S3V_UPLOAD_DEPTH;
	DEBUG(("S3V_UPLOAD_DEPTH: DepthMode = 0x%x08\n", vmesa->DepthMode));
    }
    
    if (vmesa->dirty & S3V_UPLOAD_GEOMETRY) {
	vmesa->dirty &= ~S3V_UPLOAD_GEOMETRY;
    }

    if (vmesa->dirty & S3V_UPLOAD_TRANSFORM) {
	vmesa->dirty &= ~S3V_UPLOAD_TRANSFORM;
    }
    
    if (vmesa->dirty & S3V_UPLOAD_TEX0) {
	s3vTextureObjectPtr curTex = vmesa->CurrentTexObj[0];
	vmesa->dirty &= ~S3V_UPLOAD_TEX0;
	DEBUG(("S3V_UPLOAD_TEX0\n"));
	if (curTex) {
		DEBUG(("S3V_UPLOAD_TEX0: curTex\n"));
	} else {
		DEBUG(("S3V_UPLOAD_TEX0: !curTex\n"));
	}
    }
}

void s3vDDUpdateHWState( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	int new_state = vmesa->new_state;

	/* s3vUpdateClipping( ctx ); */

	if ( new_state )
	{

		vmesa->new_state = 0;

		/* Update the various parts of the context's state.
		 */
		if ( new_state & S3V_NEW_ALPHA )
			s3vUpdateAlphaMode( ctx );

		if ( new_state & S3V_NEW_DEPTH )
			s3vUpdateZMode( ctx );

		if ( new_state & S3V_NEW_FOG )
			s3vUpdateFogAttrib( ctx );

		if ( new_state & S3V_NEW_CLIP )
		{
			DEBUG(("---> going to s3vUpdateClipping\n"));
			s3vUpdateClipping( ctx );
		}

		if ( new_state & S3V_NEW_POLYGON )
			s3vUpdatePolygon( ctx );

		if ( new_state & S3V_NEW_CULL )
			s3vUpdateCull( ctx );

		if ( new_state & S3V_NEW_MASKS )
			s3vUpdateMasks( ctx );

		if ( new_state & S3V_NEW_WINDOW )
			s3vUpdateWindow( ctx );
/*
		if ( new_state & S3_NEW_TEXTURE )
			s3vUpdateTextureState( ctx );		
*/
		CMDCHANGE();
	}

	/* HACK ! */
	s3vEmitHwState( vmesa );
}


static void s3vDDUpdateState( GLcontext *ctx, GLuint new_state )
{
	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_vbo_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );
	S3V_CONTEXT(ctx)->new_gl_state |= new_state;
}


/* Initialize the context's hardware state.
 */
void s3vInitState( s3vContextPtr vmesa )
{
	vmesa->new_state = 0;
}

/* Initialize the driver's state functions.
 */
void s3vInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= s3vDDUpdateState;

   ctx->Driver.Clear			= s3vDDClear;
   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearColor		= s3vDDClearColor;
   ctx->Driver.DrawBuffer		= s3vDDSetDrawBuffer; 
   ctx->Driver.ReadBuffer               = NULL; /* XXX */

   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.ColorMask		= NULL; /* s3vDDColorMask; */ /* FIXME */

   ctx->Driver.AlphaFunc		= s3vDDAlphaFunc; /* FIXME */
#if 0
   ctx->Driver.BlendEquation		= NULL; /* s3vDDBlendEquation; */
   ctx->Driver.BlendFunc		= s3vDDBlendFunc; /* FIXME */
#endif
   ctx->Driver.BlendFuncSeparate	= NULL; /* s3vDDBlendFuncSeparate; */
   ctx->Driver.ClearDepth		= s3vDDClearDepth;
   ctx->Driver.CullFace			= s3vDDCullFace; 
   ctx->Driver.FrontFace		= s3vDDFrontFace;
   ctx->Driver.DepthFunc		= s3vDDDepthFunc;	/* FIXME */
   ctx->Driver.DepthMask		= s3vDDDepthMask;	/* FIXME */
   ctx->Driver.DepthRange		= NULL; /* s3vDDDepthRange; */
   ctx->Driver.Enable			= s3vDDEnable;		/* FIXME */
   ctx->Driver.Finish			= s3vDDFinish;
   ctx->Driver.Flush			= s3vDDFlush;
#if 1
   ctx->Driver.Fogfv			= NULL; /* s3vDDFogfv; */
#endif
   ctx->Driver.Hint			= NULL;
   ctx->Driver.LineWidth		= NULL; /* s3vDDLineWidth; */
   ctx->Driver.LineStipple		= NULL; /* s3vDDLineStipple; */
#if ENABLELIGHTING
   ctx->Driver.Lightfv			= NULL; /* s3vDDLightfv; */

   ctx->Driver.LightModelfv		= NULL; /* s3vDDLightModelfv; */
#endif
   ctx->Driver.LogicOpcode		= NULL; /* s3vDDLogicalOpcode; */
   ctx->Driver.PointSize		= NULL; /* s3vDDPointSize; */
   ctx->Driver.PolygonMode		= NULL; /* s3vDDPolygonMode; */
   ctx->Driver.PolygonStipple		= NULL; /* s3vDDPolygonStipple; */
   ctx->Driver.Scissor			= s3vDDScissor; /* ScissorLR / ScissorTB */
   ctx->Driver.ShadeModel		= NULL; /* s3vDDShadeModel; */
   ctx->Driver.Viewport			= NULL; /* s3vDDViewport; */
}
