/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_tex.c,v 1.4 2002/11/05 17:46:07 tsi Exp $ */

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "texstore.h"
#include "teximage.h"
#include "texformat.h"
#include "texobj.h"
#include "swrast/swrast.h"

#include "mm.h"
#include "gamma_context.h"
#include "colormac.h"


/*
 * Compute the 'S2.4' lod bias factor from the floating point OpenGL bias.
 */
#if 0
static GLuint gammaComputeLodBias(GLfloat bias)
{
   return bias;
}
#endif

static void gammaSetTexWrapping(gammaTextureObjectPtr t, 
			       GLenum wraps, GLenum wrapt)
{
   u_int32_t t1 = t->TextureAddressMode;
   u_int32_t t2 = t->TextureReadMode;

   t1 &= ~(TAM_SWrap_Mask | TAM_TWrap_Mask);
   t2 &= ~(TRM_UWrap_Mask | TRM_VWrap_Mask);
   
   if (wraps != GL_CLAMP) {
      t1 |= TAM_SWrap_Repeat;
      t2 |= TRM_UWrap_Repeat;
   }

   if (wrapt != GL_CLAMP) {
      t1 |= TAM_TWrap_Repeat;
      t2 |= TRM_VWrap_Repeat;
   }

   t->TextureAddressMode = t1;
   t->TextureReadMode = t2;
}


static void gammaSetTexFilter(gammaContextPtr gmesa, 
			     gammaTextureObjectPtr t, 
			     GLenum minf, GLenum magf,
                             GLfloat bias)
{
   u_int32_t t1 = t->TextureAddressMode;
   u_int32_t t2 = t->TextureReadMode;

   t2 &= ~(TRM_Mag_Mask | TRM_Min_Mask);

   switch (minf) {
   case GL_NEAREST:
      t1 &= ~TAM_LODEnable;
      t2 &= ~TRM_MipMapEnable;
      t2 |= TRM_Min_Nearest;
      break;
   case GL_LINEAR:
      t1 &= ~TAM_LODEnable;
      t2 &= ~TRM_MipMapEnable;
      t2 |= TRM_Min_Linear;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      t2 |= TRM_Min_NearestMMNearest;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      t2 |= TRM_Min_LinearMMNearest;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      t2 |= TRM_Min_NearestMMLinear;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      t2 |= TRM_Min_LinearMMLinear;
      break;
   default:
      break;
   }

   switch (magf) {
   case GL_NEAREST:
      t2 |= TRM_Mag_Nearest;
      break;
   case GL_LINEAR:
      t2 |= TRM_Mag_Linear;
      break;
   default:
      break;
   }  

   t->TextureAddressMode = t1;
   t->TextureReadMode = t2;
}


static void gammaSetTexBorderColor(gammaContextPtr gmesa,
				  gammaTextureObjectPtr t, 
				  GLubyte color[4])
{
    t->TextureBorderColor = PACK_COLOR_8888(color[0], color[1], color[2], color[3]);
}


static void gammaTexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   gammaTextureObjectPtr t = (gammaTextureObjectPtr) tObj->DriverData;
   if (!t)
      return;

   /* Can't do the update now as we don't know whether to flush
    * vertices or not.  Setting gmesa->new_state means that
    * gammaUpdateTextureState() will be called before any triangles are
    * rendered.  If a statechange has occurred, it will be detected at
    * that point, and buffered vertices flushed.  
    */
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      {
         GLfloat bias = ctx->Texture.Unit[ctx->Texture.CurrentUnit].LodBias;
         gammaSetTexFilter( gmesa, t, tObj->MinFilter, tObj->MagFilter, bias );
      }
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      gammaSetTexWrapping( t, tObj->WrapS, tObj->WrapT );
      break;
  
   case GL_TEXTURE_BORDER_COLOR:
      gammaSetTexBorderColor( gmesa, t, tObj->_BorderChan );
      break;

   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      /* This isn't the most efficient solution but there doesn't appear to
       * be a nice alternative for Radeon.  Since there's no LOD clamping,
       * we just have to rely on loading the right subset of mipmap levels
       * to simulate a clamped LOD.
       */
      gammaSwapOutTexObj( gmesa, t );
      break;

   default:
      return;
   }

   if (t == gmesa->CurrentTexObj[0])
      gmesa->dirty |= GAMMA_UPLOAD_TEX0;

#if 0
   if (t == gmesa->CurrentTexObj[1]) {
      gmesa->dirty |= GAMMA_UPLOAD_TEX1;
   }
#endif
}


static void gammaTexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT( ctx );
   GLuint unit = ctx->Texture.CurrentUnit;

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

      col = ((a << 24) | 
	     (r << 16) | 
	     (g <<  8) | 
	     (b <<  0));

      break;
   }
   case GL_TEXTURE_ENV_MODE:
      gmesa->TexEnvImageFmt[unit] = 0; /* force recalc of env state */
      break;

   case GL_TEXTURE_LOD_BIAS_EXT:
#if 0  /* ?!?!?! */
      {
         struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
         gammaTextureObjectPtr t = (gammaTextureObjectPtr) tObj->DriverData;
         (void) t;
	 /* XXX Looks like there's something missing here */
      }
#endif
      break;

   default:
      break;
   }
} 

#if 0
static void gammaTexImage1D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint border,
			    GLenum format, GLenum type, 
			    const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *pack,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   gammaTextureObjectPtr t = (gammaTextureObjectPtr) texObj->DriverData;
   if (t) {
      gammaSwapOutTexObj( GAMMA_CONTEXT(ctx), t );
   }
   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, pack, texObj, texImage );
}
#endif

#if 0
static void gammaTexSubImage1D( GLcontext *ctx, 
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
   gammaTextureObjectPtr t = (gammaTextureObjectPtr) texObj->DriverData;
   if (t) {
      gammaSwapOutTexObj( GAMMA_CONTEXT(ctx), t );
   }
   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width, 
			     format, type, pixels, pack, texObj,
			     texImage);
}
#endif

static void gammaTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   gammaTextureObjectPtr t = (gammaTextureObjectPtr) texObj->DriverData;
   if (t) {
      gammaSwapOutTexObj( GAMMA_CONTEXT(ctx), t );
   }
   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
}

static void gammaTexSubImage2D( GLcontext *ctx, 
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
   gammaTextureObjectPtr t = (gammaTextureObjectPtr) texObj->DriverData;
   if (t) {
      gammaSwapOutTexObj( GAMMA_CONTEXT(ctx), t );
   }
   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);
}

static void gammaBindTexture( GLcontext *ctx, GLenum target,
			     struct gl_texture_object *tObj )
{
      gammaContextPtr gmesa = GAMMA_CONTEXT( ctx );
      gammaTextureObjectPtr t = (gammaTextureObjectPtr) tObj->DriverData;

      if (!t) {
         GLfloat bias = ctx->Texture.Unit[ctx->Texture.CurrentUnit].LodBias;
	 t = CALLOC_STRUCT(gamma_texture_object_t);

	 /* Initialize non-image-dependent parts of the state:
	  */
	 t->globj = tObj;

   	 t->TextureAddressMode = TextureAddressModeEnable | TAM_Operation_3D |
			   TAM_DY_Enable | TAM_LODEnable;
         t->TextureReadMode = TextureReadModeEnable | TRM_PrimaryCacheEnable |
			TRM_MipMapEnable | TRM_BorderClamp | TRM_Border;
         t->TextureColorMode = TextureColorModeEnable;
         t->TextureFilterMode = TextureFilterModeEnable;

         if (target == GL_TEXTURE_2D) {
            t->TextureAddressMode |= TAM_TexMapType_2D;
            t->TextureReadMode |= TRM_TexMapType_2D;
         }
         else if (target == GL_TEXTURE_1D) {
            t->TextureAddressMode |= TAM_TexMapType_1D;
            t->TextureReadMode |= TRM_TexMapType_1D;
         }

         t->TextureColorMode = TextureColorModeEnable;

         t->TextureFilterMode = TextureFilterModeEnable;

#ifdef MESA_LITTLE_ENDIAN
         t->TextureFormat = (TF_LittleEndian |
#else
         t->TextureFormat = (TF_BigEndian |
#endif
			TF_ColorOrder_RGB |
			TF_OutputFmt_Texel);

	 t->dirty_images = ~0;

	 tObj->DriverData = t;
	 make_empty_list( t );

	 gammaSetTexWrapping( t, tObj->WrapS, tObj->WrapT );
	 gammaSetTexFilter( gmesa, t, tObj->MinFilter, tObj->MagFilter, bias );
	 gammaSetTexBorderColor( gmesa, t, tObj->_BorderChan );
      }
}

static void gammaDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   gammaTextureObjectPtr t = (gammaTextureObjectPtr)tObj->DriverData;

   if (t) {
      gammaContextPtr gmesa = GAMMA_CONTEXT( ctx );
#if 0
      if (gmesa)
         GAMMA_FIREVERTICES( gmesa );
#endif
      gammaDestroyTexObj( gmesa, t );
      tObj->DriverData = 0;
   }
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}

static GLboolean gammaIsTextureResident( GLcontext *ctx, 
					struct gl_texture_object *tObj )
{
   gammaTextureObjectPtr t = (gammaTextureObjectPtr)tObj->DriverData;
   return t && t->MemBlock;
}

#ifdef UNUSED
/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *
gammaNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   return obj;
}
#endif

void gammaInitTextureObjects( GLcontext *ctx )
{
   struct gl_texture_object *texObj;
   GLuint tmp = ctx->Texture.CurrentUnit;

   ctx->Texture.CurrentUnit = 0;

   texObj = ctx->Texture.Unit[0].Current1D;
   gammaBindTexture( ctx, GL_TEXTURE_1D, texObj );

   texObj = ctx->Texture.Unit[0].Current2D;
   gammaBindTexture( ctx, GL_TEXTURE_2D, texObj );

#if 0
   ctx->Texture.CurrentUnit = 1;

   texObj = ctx->Texture.Unit[1].Current1D;
   gammaBindTexture( ctx, GL_TEXTURE_1D, texObj );

   texObj = ctx->Texture.Unit[1].Current2D;
   gammaBindTexture( ctx, GL_TEXTURE_2D, texObj );
#endif

   ctx->Texture.CurrentUnit = tmp;
}


void gammaDDInitTextureFuncs( struct dd_function_table *functions )
{
   functions->TexEnv = gammaTexEnv;
   functions->TexImage2D = gammaTexImage2D;
   functions->TexSubImage2D = gammaTexSubImage2D;
   functions->BindTexture = gammaBindTexture;
   functions->DeleteTexture = gammaDeleteTexture;
   functions->TexParameter = gammaTexParameter;
   functions->IsTextureResident = gammaIsTextureResident;
}
