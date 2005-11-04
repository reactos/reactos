/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_ioctl.h"
#include "mach64_state.h"
#include "mach64_vb.h"
#include "mach64_tris.h"
#include "mach64_tex.h"

#include "context.h"
#include "macros.h"
#include "simple_list.h"
#include "enums.h"
#include "texstore.h"
#include "texformat.h"
#include "teximage.h"
#include "texobj.h"
#include "imports.h"


static void mach64SetTexWrap( mach64TexObjPtr t,
			      GLenum swrap, GLenum twrap )
{
   switch ( swrap ) {
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
   case GL_CLAMP_TO_BORDER:
      t->ClampS = GL_TRUE;
      break;
   case GL_REPEAT:
      t->ClampS = GL_FALSE;
      break;
   }

   switch ( twrap ) {
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
   case GL_CLAMP_TO_BORDER:
      t->ClampT = GL_TRUE;
      break;
   case GL_REPEAT:
      t->ClampT = GL_FALSE;
      break;
   }
}

static void mach64SetTexFilter( mach64TexObjPtr t,
				GLenum minf, GLenum magf )
{
   switch ( minf ) {
   case GL_NEAREST:
   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_NEAREST_MIPMAP_LINEAR:
      t->BilinearMin = GL_FALSE;
      break;
   case GL_LINEAR:
   case GL_LINEAR_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_LINEAR:
      t->BilinearMin = GL_TRUE;
      break;
   }

   switch ( magf ) {
   case GL_NEAREST:
      t->BilinearMag = GL_FALSE;
      break;
   case GL_LINEAR:
      t->BilinearMag = GL_TRUE;
      break;
   }
}

static void mach64SetTexBorderColor( mach64TexObjPtr t, GLubyte c[4] )
{
#if 0
   GLuint border = mach64PackColor( 4, c[0], c[1], c[2], c[3] );
#endif
}


static mach64TexObjPtr
mach64AllocTexObj( struct gl_texture_object *texObj )
{
   mach64TexObjPtr t;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API )
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, texObj );

   t = (mach64TexObjPtr) CALLOC_STRUCT( mach64_texture_object );
   if ( !t )
      return NULL;

   /* Initialize non-image-dependent parts of the state:
    */
   t->tObj = texObj;

   t->offset = 0;

   t->dirty = 1;

   make_empty_list( t );

   mach64SetTexWrap( t, texObj->WrapS, texObj->WrapT );
   /*mach64SetTexMaxAnisotropy( t, texObj->MaxAnisotropy );*/
   mach64SetTexFilter( t, texObj->MinFilter, texObj->MagFilter );
   mach64SetTexBorderColor( t, texObj->_BorderChan );

   return t;
}


/* Called by the _mesa_store_teximage[123]d() functions. */
static const struct gl_texture_format *
mach64ChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			   GLenum format, GLenum type )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   (void) format;
   (void) type;

   switch ( internalFormat ) {
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case 4:
   case GL_RGBA:
   case GL_RGBA2:
      if (mmesa->mach64Screen->cpp == 4)
         return &_mesa_texformat_argb8888;
      else
         return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      if (mmesa->mach64Screen->cpp == 4)
         return &_mesa_texformat_argb8888;
      else
         return &_mesa_texformat_argb1555;

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
   case GL_RGBA4:
      if (mmesa->mach64Screen->cpp == 4)
         return &_mesa_texformat_argb8888;
      else
         return &_mesa_texformat_argb4444;

   case 3:
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      if (mmesa->mach64Screen->cpp == 4)
         return &_mesa_texformat_argb8888;
      else
         return &_mesa_texformat_rgb565;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      if (mmesa->mach64Screen->cpp == 4)
         return &_mesa_texformat_argb8888; /* inefficient but accurate */
      else
         return &_mesa_texformat_argb1555;

   case GL_INTENSITY4:
   case GL_INTENSITY:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      if (mmesa->mach64Screen->cpp == 4)
         return &_mesa_texformat_argb8888; /* inefficient but accurate */
      else
         return &_mesa_texformat_argb4444;

   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
          type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   default:
      _mesa_problem( ctx, "unexpected format in %s", __FUNCTION__ );
      return NULL;
   }
}

static void mach64TexImage1D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64TexObjPtr t = (mach64TexObjPtr) texObj->DriverData;

   if ( t ) {
      mach64SwapOutTexObj( mmesa, t );
   }
   else {
      t = mach64AllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
         return;
      }
      texObj->DriverData = t;
   }

   /* Note, this will call mach64ChooseTextureFormat */
   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, packing, texObj, texImage );

   mmesa->new_state |= MACH64_NEW_TEXTURE;
}

static void mach64TexSubImage1D( GLcontext *ctx,
				 GLenum target,
				 GLint level,
				 GLint xoffset,
				 GLsizei width,
				 GLenum format, GLenum type,
				 const GLvoid *pixels,
				 const struct gl_pixelstore_attrib *packing,
				 struct gl_texture_object *texObj,
				 struct gl_texture_image *texImage )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64TexObjPtr t = (mach64TexObjPtr) texObj->DriverData;

   assert( t ); /* this _should_ be true */
   if ( t ) {
      mach64SwapOutTexObj( mmesa, t );
   }
   else {
      t = mach64AllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
         return;
      }
      texObj->DriverData = t;
   }

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
			     format, type, pixels, packing, texObj,
			     texImage);

   mmesa->new_state |= MACH64_NEW_TEXTURE;
}

static void mach64TexImage2D( GLcontext *ctx, GLenum target, GLint level,
			      GLint internalFormat,
			      GLint width, GLint height, GLint border,
			      GLenum format, GLenum type, const GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64TexObjPtr t = (mach64TexObjPtr) texObj->DriverData;

   if ( t ) {
      mach64SwapOutTexObj( mmesa, t );
   }
   else {
      t = mach64AllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
      texObj->DriverData = t;
   }

   /* Note, this will call mach64ChooseTextureFormat */
   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type, pixels,
			   &ctx->Unpack, texObj, texImage );

   mmesa->new_state |= MACH64_NEW_TEXTURE;
}

static void mach64TexSubImage2D( GLcontext *ctx,
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
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64TexObjPtr t = (mach64TexObjPtr) texObj->DriverData;

   assert( t ); /* this _should_ be true */
   if ( t ) {
      mach64SwapOutTexObj( mmesa, t );
   }
   else {
      t = mach64AllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }
      texObj->DriverData = t;
   }

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
			     height, format, type, pixels, packing, texObj,
			     texImage);

   mmesa->new_state |= MACH64_NEW_TEXTURE;
}

/* Due to the way we must program texture state into the Rage Pro,
 * we must leave these calculations to the absolute last minute.
 */
void mach64EmitTexStateLocked( mach64ContextPtr mmesa,
			       mach64TexObjPtr t0,
			       mach64TexObjPtr t1 )
{
   drm_mach64_sarea_t *sarea = mmesa->sarea;
   drm_mach64_context_regs_t *regs = &(mmesa->setup);

   /* for multitex, both textures must be local or AGP */
   if ( t0 && t1 )
      assert(t0->heap == t1->heap);

   if ( t0 ) {
      if (t0->heap == MACH64_CARD_HEAP) {
#if ENABLE_PERF_BOXES
	 mmesa->c_texsrc_card++;
#endif
	 mmesa->setup.tex_cntl &= ~MACH64_TEX_SRC_AGP;
      } else {
#if ENABLE_PERF_BOXES
	 mmesa->c_texsrc_agp++;
#endif
	 mmesa->setup.tex_cntl |= MACH64_TEX_SRC_AGP;
      }
      mmesa->setup.tex_offset = t0->offset;
   }

   if ( t1 ) {
      mmesa->setup.secondary_tex_off = t1->offset;
   }

   memcpy( &sarea->context_state.tex_size_pitch, &regs->tex_size_pitch,
	   MACH64_NR_TEXTURE_REGS * sizeof(GLuint) );
}


/* ================================================================
 * Device Driver API texture functions
 */

static void mach64DDTexEnv( GLcontext *ctx, GLenum target,
			    GLenum pname, const GLfloat *param )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
#if 0
   struct gl_texture_unit *texUnit;
   GLubyte c[4];
#endif

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( pname ) );
   }

   switch ( pname ) {
   case GL_TEXTURE_ENV_MODE:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_TEXTURE | MACH64_NEW_ALPHA;
      break;

#if 0
   case GL_TEXTURE_ENV_COLOR:
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      CLAMPED_FLOAT_TO_UBYTE( c[0], texUnit->EnvColor[0] );
      CLAMPED_FLOAT_TO_UBYTE( c[1], texUnit->EnvColor[1] );
      CLAMPED_FLOAT_TO_UBYTE( c[2], texUnit->EnvColor[2] );
      CLAMPED_FLOAT_TO_UBYTE( c[3], texUnit->EnvColor[3] );
      mmesa->env_color = mach64PackColor( 32, c[0], c[1], c[2], c[3] );
      if ( mmesa->setup.constant_color_c != mmesa->env_color ) {
	 FLUSH_BATCH( mmesa );
	 mmesa->setup.constant_color_c = mmesa->env_color;

	 mmesa->new_state |= MACH64_NEW_TEXTURE;

	 /* More complex multitexture/multipass fallbacks for GL_BLEND
	  * can be done later, but this allows a single pass GL_BLEND
	  * in some cases (ie. Performer town demo).
	  */
	 mmesa->blend_flags &= ~MACH64_BLEND_ENV_COLOR;
	 if ( mmesa->env_color != 0x00000000 &&
	      mmesa->env_color != 0xff000000 &&
	      mmesa->env_color != 0x00ffffff &&
	      mmesa->env_color != 0xffffffff )) {	
	    mmesa->blend_flags |= MACH64_BLEND_ENV_COLOR;
	 }
      }
      break;
#endif

   default:
      return;
   }
}

static void mach64DDTexParameter( GLcontext *ctx, GLenum target,
				  struct gl_texture_object *tObj,
				  GLenum pname, const GLfloat *params )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64TexObjPtr t = (mach64TexObjPtr)tObj->DriverData;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( pname ) );
   }

   if ( ( target != GL_TEXTURE_2D ) &&
	( target != GL_TEXTURE_1D ) ) {
      return;
   }

   if (!t) {
      t = mach64AllocTexObj(tObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexParameter");
         return;
      }
      tObj->DriverData = t;
   }

   switch ( pname ) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      if ( t->bound ) FLUSH_BATCH( mmesa );
      mach64SetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      if ( t->bound ) FLUSH_BATCH( mmesa );
      mach64SetTexWrap( t, tObj->WrapS, tObj->WrapT );
      break;

   case GL_TEXTURE_BORDER_COLOR:
      if ( t->bound ) FLUSH_BATCH( mmesa );
      mach64SetTexBorderColor( t, tObj->_BorderChan );
      break;

   case GL_TEXTURE_BASE_LEVEL:
      /* From Radeon/Rage128:
       * This isn't the most efficient solution but there doesn't appear to
       * be a nice alternative.  Since there's no LOD clamping,
       * we just have to rely on loading the right subset of mipmap levels
       * to simulate a clamped LOD.  
       *
       * For mach64 we're only concerned with the base level
       * since that's the only texture we upload.
       */
      if ( t->bound ) FLUSH_BATCH( mmesa );
      mach64SwapOutTexObj( mmesa, t );
      break;

   default:
      return;
   }

   mmesa->new_state |= MACH64_NEW_TEXTURE;
}

static void mach64DDBindTexture( GLcontext *ctx, GLenum target,
				 struct gl_texture_object *tObj )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLint unit = ctx->Texture.CurrentUnit;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p ) unit=%d\n",
	       __FUNCTION__, tObj, unit );
   }

   FLUSH_BATCH( mmesa );

   if ( mmesa->CurrentTexObj[unit] ) {
      mmesa->CurrentTexObj[unit]->bound &= ~(unit+1);
      mmesa->CurrentTexObj[unit] = NULL;
   }

   mmesa->new_state |= MACH64_NEW_TEXTURE;
}

static void mach64DDDeleteTexture( GLcontext *ctx,
				   struct gl_texture_object *tObj )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64TexObjPtr t = (mach64TexObjPtr)tObj->DriverData;

   if ( t ) {
      if ( t->bound && mmesa ) {
	 FLUSH_BATCH( mmesa );

	 mmesa->CurrentTexObj[t->bound-1] = 0;
	 mmesa->new_state |= MACH64_NEW_TEXTURE;
      }

      mach64DestroyTexObj( mmesa, t );
      tObj->DriverData = NULL;
      /* Free mipmap images and the texture object itself */
      _mesa_delete_texture_object(ctx, tObj);

   }
}

static GLboolean mach64DDIsTextureResident( GLcontext *ctx,
					    struct gl_texture_object *tObj )
{
   mach64TexObjPtr t = (mach64TexObjPtr)tObj->DriverData;

   return ( t && t->memBlock );
}


void mach64InitTextureFuncs( struct dd_function_table *functions )
{
   functions->TexEnv			= mach64DDTexEnv;
   functions->ChooseTextureFormat	= mach64ChooseTextureFormat;
   functions->TexImage1D		= mach64TexImage1D;
   functions->TexSubImage1D		= mach64TexSubImage1D;
   functions->TexImage2D		= mach64TexImage2D;
   functions->TexSubImage2D		= mach64TexSubImage2D;
   functions->TexImage3D               = _mesa_store_teximage3d;
   functions->TexSubImage3D            = _mesa_store_texsubimage3d;
   functions->CopyTexImage1D           = _swrast_copy_teximage1d;
   functions->CopyTexImage2D           = _swrast_copy_teximage2d;
   functions->CopyTexSubImage1D        = _swrast_copy_texsubimage1d;
   functions->CopyTexSubImage2D        = _swrast_copy_texsubimage2d;
   functions->CopyTexSubImage3D        = _swrast_copy_texsubimage3d;
   functions->TexParameter		= mach64DDTexParameter;
   functions->BindTexture		= mach64DDBindTexture;
   functions->DeleteTexture		= mach64DDDeleteTexture;
   functions->UpdateTexturePalette	= NULL;
   functions->ActiveTexture		= NULL;
   functions->IsTextureResident	= mach64DDIsTextureResident;
   functions->PrioritizeTexture	= NULL;
}
