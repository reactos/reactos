/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "texstore.h"
#include "texformat.h"
#include "teximage.h"
#include "swrast/swrast.h"

#include "mm.h"
#include "s3v_context.h"
#include "s3v_tex.h"


extern void s3vSwapOutTexObj(s3vContextPtr vmesa, s3vTextureObjectPtr t);
extern void s3vDestroyTexObj(s3vContextPtr vmesa, s3vTextureObjectPtr t);

/*
static GLuint s3vComputeLodBias(GLfloat bias)
{
#if TEX_DEBUG_ON
	DEBUG_TEX(("*** s3vComputeLodBias ***\n"));
#endif
	return bias;
}
*/

static void s3vSetTexWrapping(s3vContextPtr vmesa,
                               s3vTextureObjectPtr t, 
			       GLenum wraps, GLenum wrapt)
{
	GLuint t0 = t->TextureCMD;
	GLuint cmd = vmesa->CMD;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vSetTexWrapping: #%i ***\n", ++times));
#endif


	t0 &= ~TEX_WRAP_MASK;
	cmd &= ~TEX_WRAP_MASK;

	if ((wraps != GL_CLAMP) || (wrapt != GL_CLAMP)) {
		DEBUG(("TEX_WRAP_ON\n"));
		t0 |= TEX_WRAP_ON;
		cmd |= TEX_WRAP_ON; 
	}

	cmd |= TEX_WRAP_ON; /* FIXME: broken if off */
	t->TextureCMD = t0;
	vmesa->CMD = cmd;
}


static void s3vSetTexFilter(s3vContextPtr vmesa, 
			     s3vTextureObjectPtr t, 
			     GLenum minf, GLenum magf)
{
	GLuint t0 = t->TextureCMD;
	GLuint cmd = vmesa->CMD;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vSetTexFilter: #%i ***\n", ++times));
#endif

	t0 &= ~TEX_FILTER_MASK;
	cmd &= ~TEX_FILTER_MASK;

	switch (minf) {
		case GL_NEAREST:
			DEBUG(("GL_NEAREST\n"));
			t0 |= NEAREST;
			cmd |= NEAREST;
			break;
		case GL_LINEAR:
			DEBUG(("GL_LINEAR\n"));
			t0 |= LINEAR;
			cmd |= LINEAR;
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			DEBUG(("GL_MIPMAP_NEAREST\n"));
			t0 |= MIP_NEAREST;
			cmd |= MIP_NEAREST;
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			DEBUG(("GL_LINEAR_MIPMAP_NEAREST\n"));
			t0 |= LINEAR_MIP_NEAREST;
			cmd |= LINEAR_MIP_NEAREST;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			DEBUG(("GL_NEAREST_MIPMAP_LINEAR\n"));
			t0 |= MIP_LINEAR;
			cmd |= MIP_LINEAR;
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			DEBUG(("GL_LINEAR_MIPMAP_LINEAR\n"));
			t0 |= LINEAR_MIP_LINEAR;
			cmd |= LINEAR_MIP_LINEAR;
			break;
		default:
			break;
	}
	/* FIXME: bilinear? */

#if 0
	switch (magf) {
		case GL_NEAREST:
			break;
		case GL_LINEAR:
			break;
		default:
			break;
	}  
#endif

	t->TextureCMD = t0;

	DEBUG(("CMD was = 0x%x\n", vmesa->CMD));
	DEBUG(("CMD is = 0x%x\n", cmd));

	vmesa->CMD = cmd; 
	/* CMDCHANGE(); */
}


static void s3vSetTexBorderColor(s3vContextPtr vmesa,
				  s3vTextureObjectPtr t, 
				  GLubyte color[4])
{
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vSetTexBorderColor: #%i ***\n", ++times));
#endif

	/*FIXME: it should depend on tex col format */
	/* switch(t0 ... t->TextureColorMode) */

	/* case TEX_COL_ARGB1555: */
	t->TextureBorderColor =
		S3VIRGEPACKCOLOR555(color[0], color[1], color[2], color[3]);

	DEBUG(("TextureBorderColor = 0x%x\n", t->TextureBorderColor));

	vmesa->TextureBorderColor = t->TextureBorderColor;
}

static void s3vTexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	s3vTextureObjectPtr t = (s3vTextureObjectPtr) tObj->DriverData;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vTexParameter: #%i ***\n", ++times));
#endif
   
	if (!t) return;

	/* Can't do the update now as we don't know whether to flush
	 * vertices or not.  Setting vmesa->new_state means that
	 * s3vUpdateTextureState() will be called before any triangles are
	 * rendered.  If a statechange has occurred, it will be detected at
	 * that point, and buffered vertices flushed.  
	*/
	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
		s3vSetTexFilter( vmesa, t, tObj->MinFilter, tObj->MagFilter );
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
		s3vSetTexWrapping( vmesa, t, tObj->WrapS, tObj->WrapT );
		break;
  
	case GL_TEXTURE_BORDER_COLOR:
		s3vSetTexBorderColor( vmesa, t, tObj->_BorderChan );
		break;

	case GL_TEXTURE_BASE_LEVEL:
	case GL_TEXTURE_MAX_LEVEL:
	case GL_TEXTURE_MIN_LOD:
	case GL_TEXTURE_MAX_LOD:
	/* This isn't the most efficient solution but there doesn't appear to
	 * be a nice alternative for Virge.  Since there's no LOD clamping,
	 * we just have to rely on loading the right subset of mipmap levels
	 * to simulate a clamped LOD.
	 */
		s3vSwapOutTexObj( vmesa, t ); 
		break;

	default:
		return;
	}

	if (t == vmesa->CurrentTexObj[0])
		vmesa->dirty |= S3V_UPLOAD_TEX0;

#if 0
	if (t == vmesa->CurrentTexObj[1]) {
		vmesa->dirty |= S3V_UPLOAD_TEX1;
	}
#endif
}


static void s3vTexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	GLuint unit = ctx->Texture.CurrentUnit;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vTexEnv: #%i ***\n", ++times));
#endif

	/* Only one env color.  Need a fallback if env colors are different
	 * and texture setup references env color in both units.  
	 */
	switch (pname) {
	case GL_TEXTURE_ENV_COLOR: {
		struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
		GLfloat *fc = texUnit->EnvColor;
		GLuint r, g, b, a, col;
		CLAMPED_FLOAT_TO_UBYTE(r, fc[0]);
		CLAMPED_FLOAT_TO_UBYTE(g, fc[1]);
		CLAMPED_FLOAT_TO_UBYTE(b, fc[2]);
		CLAMPED_FLOAT_TO_UBYTE(a, fc[3]);

		col =  ((a << 24) | 
			(r << 16) | 
			(g <<  8) | 
			(b <<  0));

		break;
	}
	case GL_TEXTURE_ENV_MODE:
		vmesa->TexEnvImageFmt[unit] = 0; /* force recalc of env state */
		break;
	case GL_TEXTURE_LOD_BIAS_EXT: {
/*
		struct gl_texture_object *tObj =
			ctx->Texture.Unit[unit]._Current;

		s3vTextureObjectPtr t = (s3vTextureObjectPtr) tObj->DriverData;
*/
		break;
	}
	default:
		break;
	}
} 

static void s3vTexImage1D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint border,
			    GLenum format, GLenum type, 
			    const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *pack,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	s3vTextureObjectPtr t = (s3vTextureObjectPtr) texObj->DriverData;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vTexImage1D: #%i ***\n", ++times));
#endif

#if 1 
	if (t) {
#if _TEXFLUSH
		DMAFLUSH();
#endif
		s3vSwapOutTexObj( vmesa, t );
/*
		s3vDestroyTexObj( vmesa, t );
		texObj->DriverData = 0;
*/
	}
#endif
	_mesa_store_teximage1d( ctx, target, level, internalFormat,
				width, border, format, type,
				pixels, pack, texObj, texImage );
}

static void s3vTexSubImage1D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset,
			       GLsizei width,
			       GLenum format, GLenum type,
			       const GLvoid *pixels,
			       const struct gl_pixelstore_attrib *pack,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	s3vTextureObjectPtr t = (s3vTextureObjectPtr) texObj->DriverData;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vTexSubImage1D: #%i ***\n", ++times));
#endif

#if 1
	if (t) {
#if _TEXFLUSH
		DMAFLUSH();
#endif
		s3vSwapOutTexObj( vmesa, t );
/*
		s3vDestroyTexObj( vmesa, t );
		texObj->DriverData = 0;
*/
	}
#endif
	_mesa_store_texsubimage1d(ctx, target, level, xoffset, width, 
				format, type, pixels, pack, texObj,
				texImage);
}

static void s3vTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	s3vTextureObjectPtr t = (s3vTextureObjectPtr) texObj->DriverData;

#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vTexImage2D: #%i ***\n", ++times));
#endif

#if 1 
	if (t) {
#if _TEXFLUSH
		DMAFLUSH();
#endif
		s3vSwapOutTexObj( vmesa, t ); 
/*
		s3vDestroyTexObj( vmesa, t );
		texObj->DriverData = 0;
*/
	}
#endif
	_mesa_store_teximage2d( ctx, target, level, internalFormat,
				width, height, border, format, type,
				pixels, packing, texObj, texImage );
}

static void s3vTexSubImage2D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset, GLint yoffset,
			       GLsizei width, GLsizei height,
			       GLenum format, GLenum type,
			       const GLvoid *pixels,
			       const struct gl_pixelstore_attrib *packing,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	s3vTextureObjectPtr t = (s3vTextureObjectPtr) texObj->DriverData;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vTexSubImage2D: #%i ***\n", ++times));
#endif

#if 1
	if (t) {
#if _TEXFLUSH
		DMAFLUSH();
#endif
		s3vSwapOutTexObj( vmesa, t );
/* 
		s3vDestroyTexObj( vmesa, t );
		texObj->DriverData = 0;
*/
	}
#endif
	_mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
				height, format, type, pixels, packing, texObj,
				texImage);
}


static void s3vBindTexture( GLcontext *ctx, GLenum target,
			     struct gl_texture_object *tObj )
{
	s3vContextPtr vmesa = S3V_CONTEXT( ctx );
	s3vTextureObjectPtr t = (s3vTextureObjectPtr) tObj->DriverData;
	GLuint cmd = vmesa->CMD;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vBindTexture: #%i ***\n", ++times));
#endif

	if (!t) {
/*
		GLfloat bias = ctx->Texture.Unit[ctx->Texture.CurrentUnit].LodBias;
*/
		t = CALLOC_STRUCT(s3v_texture_object_t);

		/* Initialize non-image-dependent parts of the state:
		 */
		t->globj = tObj;
#if 0
		if (target == GL_TEXTURE_2D) {
		} else
		if (target == GL_TEXTURE_1D) {
		}

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
		t->TextureFormat = (TF_LittleEndian |
#else
		t->TextureFormat = (TF_BigEndian |
#endif
#endif
		t->dirty_images = ~0;

		tObj->DriverData = t;
		make_empty_list( t );
#if 0
		s3vSetTexWrapping( vmesa, t, tObj->WrapS, tObj->WrapT );
		s3vSetTexFilter( vmesa, t, tObj->MinFilter, tObj->MagFilter );
		s3vSetTexBorderColor( vmesa, t, tObj->BorderColor );
#endif
	}

	cmd = vmesa->CMD & ~MIP_MASK;
    vmesa->dirty |= S3V_UPLOAD_TEX0;
    vmesa->TexOffset = t->TextureBaseAddr[tObj->BaseLevel];
    vmesa->TexStride = t->Pitch;
    cmd |= MIPMAP_LEVEL(t->WidthLog2);
	vmesa->CMD = cmd;
	vmesa->restore_primitive = -1;
#if 0
	printf("t->TextureBaseAddr[0] = 0x%x\n", t->TextureBaseAddr[0]);
	printf("t->TextureBaseAddr[1] = 0x%x\n", t->TextureBaseAddr[1]);
	printf("t->TextureBaseAddr[2] = 0x%x\n", t->TextureBaseAddr[2]);
#endif
}


static void s3vDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
	s3vTextureObjectPtr t = (s3vTextureObjectPtr)tObj->DriverData;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vDeleteTexture: #%i ***\n", ++times));
#endif

	if (t) {
		s3vContextPtr vmesa = S3V_CONTEXT( ctx );

#if _TEXFLUSH
		if (vmesa) {
			DMAFLUSH();
		}
#endif

		s3vDestroyTexObj( vmesa, t );
		tObj->DriverData = 0;

	}
}

static GLboolean s3vIsTextureResident( GLcontext *ctx, 
					struct gl_texture_object *tObj )
{
	s3vTextureObjectPtr t = (s3vTextureObjectPtr)tObj->DriverData;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vIsTextureResident: #%i ***\n", ++times));
#endif
   
	return (t && t->MemBlock);
}

static void s3vInitTextureObjects( GLcontext *ctx )
{
	/* s3vContextPtr vmesa = S3V_CONTEXT(ctx); */
	struct gl_texture_object *texObj;
	GLuint tmp = ctx->Texture.CurrentUnit;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vInitTextureObjects: #%i ***\n", ++times));
#endif

#if 1
	ctx->Texture.CurrentUnit = 0;

	texObj = ctx->Texture.Unit[0].Current1D;
	s3vBindTexture( ctx, GL_TEXTURE_1D, texObj );

	texObj = ctx->Texture.Unit[0].Current2D;
	s3vBindTexture( ctx, GL_TEXTURE_2D, texObj );
#endif

#if 0
	ctx->Texture.CurrentUnit = 1;

	texObj = ctx->Texture.Unit[1].Current1D;
	s3vBindTexture( ctx, GL_TEXTURE_1D, texObj );

	texObj = ctx->Texture.Unit[1].Current2D;
	s3vBindTexture( ctx, GL_TEXTURE_2D, texObj );
#endif

	ctx->Texture.CurrentUnit = tmp;
}


void s3vInitTextureFuncs( GLcontext *ctx )
{
#if TEX_DEBUG_ON
	static unsigned int times=0;
	DEBUG_TEX(("*** s3vInitTextureFuncs: #%i ***\n", ++times));
#endif

	ctx->Driver.TexEnv = s3vTexEnv;
	ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
	ctx->Driver.TexImage1D = _mesa_store_teximage1d;
	ctx->Driver.TexImage2D = s3vTexImage2D;
	ctx->Driver.TexImage3D = _mesa_store_teximage3d;
	ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
	ctx->Driver.TexSubImage2D = s3vTexSubImage2D;
	ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
	ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
	ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
	ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
	ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
	ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
	ctx->Driver.BindTexture = s3vBindTexture;
	ctx->Driver.DeleteTexture = s3vDeleteTexture;
	ctx->Driver.TexParameter = s3vTexParameter;
	ctx->Driver.UpdateTexturePalette = 0;
	ctx->Driver.IsTextureResident = s3vIsTextureResident;
	ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

	s3vInitTextureObjects( ctx );
}
