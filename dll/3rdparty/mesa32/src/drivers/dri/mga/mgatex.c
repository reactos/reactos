/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatex.c,v 1.14 2002/10/30 12:51:36 alanh Exp $ */

#include "glheader.h"
#include "mm.h"
#include "mgacontext.h"
#include "mgatex.h"
#include "mgaregs.h"
#include "mgatris.h"
#include "mgaioctl.h"

#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "simple_list.h"
#include "imports.h"
#include "macros.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"
#include "texobj.h"

#include "swrast/swrast.h"

#include "xmlpool.h"

/**
 * Set the texture wrap modes.
 * Currently \c GL_REPEAT, \c GL_CLAMP and \c GL_CLAMP_TO_EDGE are supported.
 * 
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */

static void 
mgaSetTexWrapping( mgaTextureObjectPtr t, GLenum swrap, GLenum twrap )
{
   GLboolean  is_clamp = GL_FALSE;
   GLboolean  is_clamp_to_edge = GL_FALSE;

   t->setup.texctl &= (TMC_clampu_MASK & TMC_clampv_MASK);
   t->setup.texctl2 &= (TMC_borderen_MASK);

   switch( swrap ) {
   case GL_REPEAT:
      break;
   case GL_CLAMP:
      t->setup.texctl |= TMC_clampu_enable;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->setup.texctl |= TMC_clampu_enable;
      is_clamp_to_edge = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
   }

   switch( twrap ) {
   case GL_REPEAT:
      break;
   case GL_CLAMP:
      t->setup.texctl |= TMC_clampv_enable;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->setup.texctl |= TMC_clampv_enable;
      is_clamp_to_edge = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
   }

   if ( is_clamp ) {
      t->setup.texctl2 |= TMC_borderen_enable;
   }

   t->border_fallback = (is_clamp && is_clamp_to_edge);
}


/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 */

static void
mgaSetTexFilter( mgaTextureObjectPtr t, GLenum minf, GLenum magf )
{
   GLuint val = 0;

   switch (minf) {
   case GL_NEAREST: val = TF_minfilter_nrst; break;
   case GL_LINEAR: val = TF_minfilter_bilin; break;
   case GL_NEAREST_MIPMAP_NEAREST: val = TF_minfilter_mm1s; break;
   case GL_LINEAR_MIPMAP_NEAREST: val = TF_minfilter_mm4s; break;
   case GL_NEAREST_MIPMAP_LINEAR: val = TF_minfilter_mm2s; break;
   case GL_LINEAR_MIPMAP_LINEAR: val = TF_minfilter_mm8s; break;
   default: val = TF_minfilter_nrst; break;
   }

   switch (magf) {
   case GL_NEAREST: val |= TF_magfilter_nrst; break;
   case GL_LINEAR: val |= TF_magfilter_bilin; break;
   default: val |= TF_magfilter_nrst; break;
   }

   /* See OpenGL 1.2 specification */
   if (magf == GL_LINEAR && (minf == GL_NEAREST_MIPMAP_NEAREST ||
			     minf == GL_NEAREST_MIPMAP_LINEAR)) {
      val |= MGA_FIELD( TF_fthres, 0x20 ); /* c = 0.5 */
   } else {
      val |= MGA_FIELD( TF_fthres, 0x10 ); /* c = 0 */
   }


   /* Mask off the bits for the fields we are setting.  Remember, the MGA mask
    * defines have 0s for the bits in the named fields.  This is the opposite
    * of most of the other drivers.
    */

   t->setup.texfilter &= (TF_minfilter_MASK &
			  TF_magfilter_MASK &
			  TF_fthres_MASK);
   t->setup.texfilter |= val;
}

static void mgaSetTexBorderColor(mgaTextureObjectPtr t, GLubyte color[4])
{
   t->setup.texbordercol = PACK_COLOR_8888(color[3], color[0], 
					   color[1], color[2] );
}


static const struct gl_texture_format *
mgaChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
		        GLenum format, GLenum type )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const GLboolean do32bpt =
       ( mmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32 );
   const GLboolean force16bpt =
       ( mmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16 );
   (void) format;

   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      switch ( type ) {
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
	 return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return &_mesa_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return &_mesa_texformat_argb1555;
      default:
         return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
      }

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      switch ( type ) {
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return &_mesa_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return &_mesa_texformat_argb1555;
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
	 return &_mesa_texformat_rgb565;
      default:
         return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;
      }

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      /* FIXME: This will report incorrect component sizes... */
      return MGA_IS_G400(mmesa) ? &_mesa_texformat_al88 : &_mesa_texformat_argb4444;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      /* FIXME: This will report incorrect component sizes... */
      return MGA_IS_G400(mmesa) ? &_mesa_texformat_al88 : &_mesa_texformat_rgb565;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      /* FIXME: This will report incorrect component sizes... */
      return MGA_IS_G400(mmesa) ? &_mesa_texformat_al88 : &_mesa_texformat_argb4444;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      /* FIXME: This will report incorrect component sizes... */
      return MGA_IS_G400(mmesa) ? &_mesa_texformat_i8 : &_mesa_texformat_argb4444;

   case GL_YCBCR_MESA:
      if (MGA_IS_G400(mmesa) &&
          (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
           type == GL_UNSIGNED_BYTE))
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;

   default:
      _mesa_problem( ctx, "unexpected texture format in %s", __FUNCTION__ );
      return NULL;
   }

   return NULL; /* never get here */
}




/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

static mgaTextureObjectPtr
mgaAllocTexObj( struct gl_texture_object *tObj )
{
   mgaTextureObjectPtr t;


   t = CALLOC( sizeof( *t ) );
   tObj->DriverData = t;
   if ( t != NULL ) {
      /* Initialize non-image-dependent parts of the state:
       */
      t->base.tObj = tObj;

      t->setup.texctl = TMC_takey_1 | TMC_tamask_0;
      t->setup.texctl2 = TMC_ckstransdis_enable;
      t->setup.texfilter = TF_filteralpha_enable | TF_uvoffset_OGL;

      t->border_fallback = GL_FALSE;
      t->texenv_fallback = GL_FALSE;

      make_empty_list( & t->base );

      mgaSetTexWrapping( t, tObj->WrapS, tObj->WrapT );
      mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
      mgaSetTexBorderColor( t, tObj->_BorderChan );
   }

   return( t );
}


static void mgaTexEnv( GLcontext *ctx, GLenum target,
			 GLenum pname, const GLfloat *param )
{
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   switch( pname ) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];

      UNCLAMPED_FLOAT_TO_RGBA_CHAN( c, texUnit->EnvColor );
      mmesa->envcolor[unit] = PACK_COLOR_8888( c[3], c[0], c[1], c[2] );
      break;
   }
   }
}


static void mgaTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   if ( t != NULL ) {
      driSwapOutTextureObject( t );
   } 
   else {
      t = (driTextureObject *) mgaAllocTexObj( texObj );
      if ( t == NULL ) {
	 _mesa_error( ctx, GL_OUT_OF_MEMORY, "glTexImage2D" );
	 return;
      }
   }

   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
   level -= t->firstLevel;
   if (level >= 0)
      t->dirty_images[0] |= (1UL << level);
}

static void mgaTexSubImage2D( GLcontext *ctx, 
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
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   assert( t ); /* this _should_ be true */
   if ( t != NULL ) {
      driSwapOutTextureObject( t );
   } 
   else {
      t = (driTextureObject *) mgaAllocTexObj( texObj );
      if ( t == NULL ) {
	 _mesa_error( ctx, GL_OUT_OF_MEMORY, "glTexImage2D" );
	 return;
      }
   }

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);
   level -= t->firstLevel;
   if (level >= 0)
      t->dirty_images[0] |= (1UL << level);
}


/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void
mgaTexParameter( GLcontext *ctx, GLenum target,
		   struct gl_texture_object *tObj,
		   GLenum pname, const GLfloat *params )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaTextureObjectPtr t = (mgaTextureObjectPtr) tObj->DriverData;

   /* If we don't have a hardware texture, it will be automatically
    * created with current state before it is used, so we don't have
    * to do anything now 
    */
   if ( (t == NULL) ||
        (target != GL_TEXTURE_2D &&
         target != GL_TEXTURE_RECTANGLE_NV) ) {
      return;
   }

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
      driSwapOutTextureObject( (driTextureObject *) t );
      /* FALLTHROUGH */
   case GL_TEXTURE_MAG_FILTER:
      FLUSH_BATCH(mmesa);
      mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      FLUSH_BATCH(mmesa);
      mgaSetTexWrapping(t,tObj->WrapS,tObj->WrapT);
      break;

   case GL_TEXTURE_BORDER_COLOR:
      FLUSH_BATCH(mmesa);
      mgaSetTexBorderColor(t, tObj->_BorderChan);
      break;

   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      /* This isn't the most efficient solution but there doesn't appear to
       * be a nice alternative.  Since there's no LOD clamping,
       * we just have to rely on loading the right subset of mipmap levels
       * to simulate a clamped LOD.
       */
      driSwapOutTextureObject( (driTextureObject *) t );
      break;

   default:
      return;
   }
}


static void
mgaBindTexture( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *tObj )
{
   assert( (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_NV) ||
           (tObj->DriverData != NULL) );
}


static void
mgaDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   driTextureObject * t = (driTextureObject *) tObj->DriverData;

   if ( t ) {
      if ( mmesa ) {
	 FLUSH_BATCH( mmesa );
      }

      driDestroyTextureObject( t );
   }

   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}


/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *
mgaNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   mgaAllocTexObj( obj );
   return obj;
}


void
mgaInitTextureFuncs( struct dd_function_table *functions )
{
   functions->ChooseTextureFormat	= mgaChooseTextureFormat;
   functions->TexImage2D		= mgaTexImage2D;
   functions->TexSubImage2D		= mgaTexSubImage2D;
   functions->BindTexture		= mgaBindTexture;
   functions->NewTextureObject		= mgaNewTextureObject;
   functions->DeleteTexture		= mgaDeleteTexture;
   functions->IsTextureResident		= driIsTextureResident;
   functions->TexEnv			= mgaTexEnv;
   functions->TexParameter		= mgaTexParameter;
}
