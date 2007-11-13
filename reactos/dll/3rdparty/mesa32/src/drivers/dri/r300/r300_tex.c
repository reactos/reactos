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
*/

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "image.h"
#include "simple_list.h"
#include "texformat.h"
#include "texstore.h"
#include "texmem.h"
#include "teximage.h"
#include "texobj.h"

#include "r300_context.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "r300_tex.h"

#include "xmlpool.h"

/**
 * Set the texture wrap modes.
 *
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */

static void r300SetTexWrap(r300TexObjPtr t, GLenum swrap, GLenum twrap,
			   GLenum rwrap)
{
	unsigned long hw_swrap = 0, hw_twrap = 0, hw_qwrap = 0;

	t->filter &=
	    ~(R300_TX_WRAP_S_MASK | R300_TX_WRAP_T_MASK | R300_TX_WRAP_Q_MASK);

	switch (swrap) {
	case GL_REPEAT:
		hw_swrap |= R300_TX_REPEAT;
		break;
	case GL_CLAMP:
		hw_swrap |= R300_TX_CLAMP;
		break;
	case GL_CLAMP_TO_EDGE:
		hw_swrap |= R300_TX_CLAMP_TO_EDGE;
		break;
	case GL_CLAMP_TO_BORDER:
		hw_swrap |= R300_TX_CLAMP_TO_BORDER;
		break;
	case GL_MIRRORED_REPEAT:
		hw_swrap |= R300_TX_REPEAT | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_EXT:
		hw_swrap |= R300_TX_CLAMP | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_TO_EDGE_EXT:
		hw_swrap |= R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_TO_BORDER_EXT:
		hw_swrap |= R300_TX_CLAMP_TO_BORDER | R300_TX_MIRRORED;
		break;
	default:
		_mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
	}

	switch (twrap) {
	case GL_REPEAT:
		hw_twrap |= R300_TX_REPEAT;
		break;
	case GL_CLAMP:
		hw_twrap |= R300_TX_CLAMP;
		break;
	case GL_CLAMP_TO_EDGE:
		hw_twrap |= R300_TX_CLAMP_TO_EDGE;
		break;
	case GL_CLAMP_TO_BORDER:
		hw_twrap |= R300_TX_CLAMP_TO_BORDER;
		break;
	case GL_MIRRORED_REPEAT:
		hw_twrap |= R300_TX_REPEAT | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_EXT:
		hw_twrap |= R300_TX_CLAMP | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_TO_EDGE_EXT:
		hw_twrap |= R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_TO_BORDER_EXT:
		hw_twrap |= R300_TX_CLAMP_TO_BORDER | R300_TX_MIRRORED;
		break;
	default:
		_mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
	}

	switch (rwrap) {
	case GL_REPEAT:
		hw_qwrap |= R300_TX_REPEAT;
		break;
	case GL_CLAMP:
		hw_qwrap |= R300_TX_CLAMP;
		break;
	case GL_CLAMP_TO_EDGE:
		hw_qwrap |= R300_TX_CLAMP_TO_EDGE;
		break;
	case GL_CLAMP_TO_BORDER:
		hw_qwrap |= R300_TX_CLAMP_TO_BORDER;
		break;
	case GL_MIRRORED_REPEAT:
		hw_qwrap |= R300_TX_REPEAT | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_EXT:
		hw_qwrap |= R300_TX_CLAMP | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_TO_EDGE_EXT:
		hw_qwrap |= R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
		break;
	case GL_MIRROR_CLAMP_TO_BORDER_EXT:
		hw_qwrap |= R300_TX_CLAMP_TO_BORDER | R300_TX_MIRRORED;
		break;
	default:
		_mesa_problem(NULL, "bad R wrap mode in %s", __FUNCTION__);
	}

	t->filter |= hw_swrap << R300_TX_WRAP_S_SHIFT;
	t->filter |= hw_twrap << R300_TX_WRAP_T_SHIFT;
	t->filter |= hw_qwrap << R300_TX_WRAP_Q_SHIFT;
}

static void r300SetTexMaxAnisotropy(r300TexObjPtr t, GLfloat max)
{

	t->filter &= ~R300_TX_MAX_ANISO_MASK;

	if (max <= 1.0) {
		t->filter |= R300_TX_MAX_ANISO_1_TO_1;
	} else if (max <= 2.0) {
		t->filter |= R300_TX_MAX_ANISO_2_TO_1;
	} else if (max <= 4.0) {
		t->filter |= R300_TX_MAX_ANISO_4_TO_1;
	} else if (max <= 8.0) {
		t->filter |= R300_TX_MAX_ANISO_8_TO_1;
	} else {
		t->filter |= R300_TX_MAX_ANISO_16_TO_1;
	}
}

/**
 * Set the texture magnification and minification modes.
 *
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 */

static void r300SetTexFilter(r300TexObjPtr t, GLenum minf, GLenum magf)
{
	GLuint anisotropy = (t->filter & R300_TX_MAX_ANISO_MASK);

	t->filter &= ~(R300_TX_MIN_FILTER_MASK | R300_TX_MAG_FILTER_MASK);

	if (anisotropy == R300_TX_MAX_ANISO_1_TO_1) {
		switch (minf) {
		case GL_NEAREST:
			t->filter |= R300_TX_MIN_FILTER_NEAREST;
			break;
		case GL_LINEAR:
			t->filter |= R300_TX_MIN_FILTER_LINEAR;
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			t->filter |= R300_TX_MIN_FILTER_NEAREST_MIP_NEAREST;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			t->filter |= R300_TX_MIN_FILTER_NEAREST_MIP_LINEAR;
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			t->filter |= R300_TX_MIN_FILTER_LINEAR_MIP_NEAREST;
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			t->filter |= R300_TX_MIN_FILTER_LINEAR_MIP_LINEAR;
			break;
		}
	} else {
		switch (minf) {
		case GL_NEAREST:
			t->filter |= R300_TX_MIN_FILTER_ANISO_NEAREST;
			break;
		case GL_LINEAR:
			t->filter |= R300_TX_MIN_FILTER_ANISO_LINEAR;
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
			t->filter |=
			    R300_TX_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			t->filter |=
			    R300_TX_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR;
			break;
		}
	}

	/* Note we don't have 3D mipmaps so only use the mag filter setting
	 * to set the 3D texture filter mode.
	 */
	switch (magf) {
	case GL_NEAREST:
		t->filter |= R300_TX_MAG_FILTER_NEAREST;
		break;
	case GL_LINEAR:
		t->filter |= R300_TX_MAG_FILTER_LINEAR;
		break;
	}
}

static void r300SetTexBorderColor(r300TexObjPtr t, GLubyte c[4])
{
	t->pp_border_color = PACK_COLOR_8888(c[0], c[1], c[2], c[3]);
}

/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

static r300TexObjPtr r300AllocTexObj(struct gl_texture_object *texObj)
{
	r300TexObjPtr t;

	t = CALLOC_STRUCT(r300_tex_obj);
	texObj->DriverData = t;
	if (t != NULL) {
		if (RADEON_DEBUG & DEBUG_TEXTURE) {
			fprintf(stderr, "%s( %p, %p )\n", __FUNCTION__,
				(void *)texObj, (void *)t);
		}

		/* Initialize non-image-dependent parts of the state:
		 */
		t->base.tObj = texObj;
		t->border_fallback = GL_FALSE;

		make_empty_list(&t->base);

		r300SetTexWrap(t, texObj->WrapS, texObj->WrapT, texObj->WrapR);
		r300SetTexMaxAnisotropy(t, texObj->MaxAnisotropy);
		r300SetTexFilter(t, texObj->MinFilter, texObj->MagFilter);
		r300SetTexBorderColor(t, texObj->_BorderChan);
	}

	return t;
}

/* try to find a format which will only need a memcopy */
static const struct gl_texture_format *r300Choose8888TexFormat(GLenum srcFormat,
							       GLenum srcType)
{
	const GLuint ui = 1;
	const GLubyte littleEndian = *((const GLubyte *)&ui);

	if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
	    (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE
	     && !littleEndian) || (srcFormat == GL_ABGR_EXT
				   && srcType == GL_UNSIGNED_INT_8_8_8_8_REV)
	    || (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE
		&& littleEndian)) {
		return &_mesa_texformat_rgba8888;
	} else
	    if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV)
		|| (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE
		    && littleEndian) || (srcFormat == GL_ABGR_EXT
					 && srcType == GL_UNSIGNED_INT_8_8_8_8)
		|| (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE
		    && !littleEndian)) {
		return &_mesa_texformat_rgba8888_rev;
	} else if (srcFormat == GL_BGRA &&
		   ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
		    srcType == GL_UNSIGNED_INT_8_8_8_8)) {
		return &_mesa_texformat_argb8888_rev;
	} else if (srcFormat == GL_BGRA &&
		   ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
		    srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
		return &_mesa_texformat_argb8888;
	} else
		return _dri_texformat_argb8888;
}

static const struct gl_texture_format *r300ChooseTextureFormat(GLcontext * ctx,
							       GLint
							       internalFormat,
							       GLenum format,
							       GLenum type)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const GLboolean do32bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32);
	const GLboolean force16bpt =
	    (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16);
	(void)format;

#if 0
	fprintf(stderr, "InternalFormat=%s(%d) type=%s format=%s\n",
		_mesa_lookup_enum_by_nr(internalFormat), internalFormat,
		_mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));
	fprintf(stderr, "do32bpt=%d force16bpt=%d\n", do32bpt, force16bpt);
#endif

	switch (internalFormat) {
	case 4:
	case GL_RGBA:
	case GL_COMPRESSED_RGBA:
		switch (type) {
		case GL_UNSIGNED_INT_10_10_10_2:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
			return do32bpt ? _dri_texformat_argb8888 :
			    _dri_texformat_argb1555;
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _dri_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _dri_texformat_argb1555;
		default:
			return do32bpt ? r300Choose8888TexFormat(format, type) :
			    _dri_texformat_argb4444;
		}

	case 3:
	case GL_RGB:
	case GL_COMPRESSED_RGB:
		switch (type) {
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return _dri_texformat_argb4444;
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return _dri_texformat_argb1555;
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			return _dri_texformat_rgb565;
		default:
			return do32bpt ? _dri_texformat_argb8888 :
			    _dri_texformat_rgb565;
		}

	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
		return !force16bpt ?
		    r300Choose8888TexFormat(format,
					    type) : _dri_texformat_argb4444;

	case GL_RGBA4:
	case GL_RGBA2:
		return _dri_texformat_argb4444;

	case GL_RGB5_A1:
		return _dri_texformat_argb1555;

	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		return !force16bpt ? _dri_texformat_argb8888 :
		    _dri_texformat_rgb565;

	case GL_RGB5:
	case GL_RGB4:
	case GL_R3_G3_B2:
		return _dri_texformat_rgb565;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_COMPRESSED_ALPHA:
		return _dri_texformat_a8;

	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_COMPRESSED_LUMINANCE:
		return _dri_texformat_l8;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_COMPRESSED_LUMINANCE_ALPHA:
		return _dri_texformat_al88;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_COMPRESSED_INTENSITY:
		return _dri_texformat_i8;

	case GL_YCBCR_MESA:
		if (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
		    type == GL_UNSIGNED_BYTE)
			return &_mesa_texformat_ycbcr;
		else
			return &_mesa_texformat_ycbcr_rev;

	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return &_mesa_texformat_rgb_dxt1;

	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return &_mesa_texformat_rgba_dxt1;

	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return &_mesa_texformat_rgba_dxt3;

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return &_mesa_texformat_rgba_dxt5;

	case GL_ALPHA16F_ARB:
		return &_mesa_texformat_alpha_float16;
	case GL_ALPHA32F_ARB:
		return &_mesa_texformat_alpha_float32;
	case GL_LUMINANCE16F_ARB:
		return &_mesa_texformat_luminance_float16;
	case GL_LUMINANCE32F_ARB:
		return &_mesa_texformat_luminance_float32;
	case GL_LUMINANCE_ALPHA16F_ARB:
		return &_mesa_texformat_luminance_alpha_float16;
	case GL_LUMINANCE_ALPHA32F_ARB:
		return &_mesa_texformat_luminance_alpha_float32;
	case GL_INTENSITY16F_ARB:
		return &_mesa_texformat_intensity_float16;
	case GL_INTENSITY32F_ARB:
		return &_mesa_texformat_intensity_float32;
	case GL_RGB16F_ARB:
		return &_mesa_texformat_rgba_float16;
	case GL_RGB32F_ARB:
		return &_mesa_texformat_rgba_float32;
	case GL_RGBA16F_ARB:
		return &_mesa_texformat_rgba_float16;
	case GL_RGBA32F_ARB:
		return &_mesa_texformat_rgba_float32;

	default:
		_mesa_problem(ctx,
			      "unexpected internalFormat 0x%x in r300ChooseTextureFormat",
			      (int)internalFormat);
		return NULL;
	}

	return NULL;		/* never get here */
}

static GLboolean
r300ValidateClientStorage(GLcontext * ctx, GLenum target,
			  GLint internalFormat,
			  GLint srcWidth, GLint srcHeight,
			  GLenum format, GLenum type, const void *pixels,
			  const struct gl_pixelstore_attrib *packing,
			  struct gl_texture_object *texObj,
			  struct gl_texture_image *texImage)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_TEXTURE)
		fprintf(stderr, "intformat %s format %s type %s\n",
			_mesa_lookup_enum_by_nr(internalFormat),
			_mesa_lookup_enum_by_nr(format),
			_mesa_lookup_enum_by_nr(type));

	if (!ctx->Unpack.ClientStorage)
		return 0;

	if (ctx->_ImageTransferState ||
	    texImage->IsCompressed || texObj->GenerateMipmap)
		return 0;

	/* This list is incomplete, may be different on ppc???
	 */
	switch (internalFormat) {
	case GL_RGBA:
		if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV) {
			texImage->TexFormat = _dri_texformat_argb8888;
		} else
			return 0;
		break;

	case GL_RGB:
		if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) {
			texImage->TexFormat = _dri_texformat_rgb565;
		} else
			return 0;
		break;

	case GL_YCBCR_MESA:
		if (format == GL_YCBCR_MESA &&
		    type == GL_UNSIGNED_SHORT_8_8_REV_APPLE) {
			texImage->TexFormat = &_mesa_texformat_ycbcr_rev;
		} else if (format == GL_YCBCR_MESA &&
			   (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
			    type == GL_UNSIGNED_BYTE)) {
			texImage->TexFormat = &_mesa_texformat_ycbcr;
		} else
			return 0;
		break;

	default:
		return 0;
	}

	/* Could deal with these packing issues, but currently don't:
	 */
	if (packing->SkipPixels ||
	    packing->SkipRows || packing->SwapBytes || packing->LsbFirst) {
		return 0;
	}

	{
		GLint srcRowStride = _mesa_image_row_stride(packing, srcWidth,
							    format, type);

		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: srcRowStride %d/%x\n",
				__FUNCTION__, srcRowStride, srcRowStride);

		/* Could check this later in upload, pitch restrictions could be
		 * relaxed, but would need to store the image pitch somewhere,
		 * as packing details might change before image is uploaded:
		 */
		if (!r300IsGartMemory(rmesa, pixels, srcHeight * srcRowStride)
		    || (srcRowStride & 63))
			return 0;

		/* Have validated that _mesa_transfer_teximage would be a straight
		 * memcpy at this point.  NOTE: future calls to TexSubImage will
		 * overwrite the client data.  This is explicitly mentioned in the
		 * extension spec.
		 */
		texImage->Data = (void *)pixels;
		texImage->IsClientData = GL_TRUE;
		texImage->RowStride =
		    srcRowStride / texImage->TexFormat->TexelBytes;

		return 1;
	}
}

static void r300TexImage1D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;

	if (t) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
			return;
		}
	}

	/* Note, this will call ChooseTextureFormat */
	_mesa_store_teximage1d(ctx, target, level, internalFormat,
			       width, border, format, type, pixels,
			       &ctx->Unpack, texObj, texImage);

	t->dirty_images[0] |= (1 << level);
}

static void r300TexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset,
			      GLsizei width,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;

	assert(t);		/* this _should_ be true */
	if (t) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
			return;
		}
	}

	_mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
				  format, type, pixels, packing, texObj,
				  texImage);

	t->dirty_images[0] |= (1 << level);
}

static void r300TexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;
	GLuint face;

	/* which cube face or ordinary 2D image */
	switch (target) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		face =
		    (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		ASSERT(face < 6);
		break;
	default:
		face = 0;
	}

	if (t != NULL) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
			return;
		}
	}

	texImage->IsClientData = GL_FALSE;

	if (r300ValidateClientStorage(ctx, target,
				      internalFormat,
				      width, height,
				      format, type, pixels,
				      packing, texObj, texImage)) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: Using client storage\n",
				__FUNCTION__);
	} else {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: Using normal storage\n",
				__FUNCTION__);

		/* Normal path: copy (to cached memory) and eventually upload
		 * via another copy to GART memory and then a blit...  Could
		 * eliminate one copy by going straight to (permanent) GART.
		 *
		 * Note, this will call r300ChooseTextureFormat.
		 */
		_mesa_store_teximage2d(ctx, target, level, internalFormat,
				       width, height, border, format, type,
				       pixels, &ctx->Unpack, texObj, texImage);

		t->dirty_images[face] |= (1 << level);
	}
}

static void r300TexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset, GLint yoffset,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;
	GLuint face;

	/* which cube face or ordinary 2D image */
	switch (target) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		face =
		    (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		ASSERT(face < 6);
		break;
	default:
		face = 0;
	}

	assert(t);		/* this _should_ be true */
	if (t) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
			return;
		}
	}

	_mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
				  height, format, type, pixels, packing, texObj,
				  texImage);

	t->dirty_images[face] |= (1 << level);
}

static void r300CompressedTexImage2D(GLcontext * ctx, GLenum target,
				     GLint level, GLint internalFormat,
				     GLint width, GLint height, GLint border,
				     GLsizei imageSize, const GLvoid * data,
				     struct gl_texture_object *texObj,
				     struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;
	GLuint face;

	/* which cube face or ordinary 2D image */
	switch (target) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		face =
		    (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		ASSERT(face < 6);
		break;
	default:
		face = 0;
	}

	if (t != NULL) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY,
				    "glCompressedTexImage2D");
			return;
		}
	}

	texImage->IsClientData = GL_FALSE;

	/* can't call this, different parameters. Would never evaluate to true anyway currently */
#if 0
	if (r300ValidateClientStorage(ctx, target,
				      internalFormat,
				      width, height,
				      format, type, pixels,
				      packing, texObj, texImage)) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: Using client storage\n",
				__FUNCTION__);
	} else
#endif
	{
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: Using normal storage\n",
				__FUNCTION__);

		/* Normal path: copy (to cached memory) and eventually upload
		 * via another copy to GART memory and then a blit...  Could
		 * eliminate one copy by going straight to (permanent) GART.
		 *
		 * Note, this will call r300ChooseTextureFormat.
		 */
		_mesa_store_compressed_teximage2d(ctx, target, level,
						  internalFormat, width, height,
						  border, imageSize, data,
						  texObj, texImage);

		t->dirty_images[face] |= (1 << level);
	}
}

static void r300CompressedTexSubImage2D(GLcontext * ctx, GLenum target,
					GLint level, GLint xoffset,
					GLint yoffset, GLsizei width,
					GLsizei height, GLenum format,
					GLsizei imageSize, const GLvoid * data,
					struct gl_texture_object *texObj,
					struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;
	GLuint face;

	/* which cube face or ordinary 2D image */
	switch (target) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		face =
		    (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		ASSERT(face < 6);
		break;
	default:
		face = 0;
	}

	assert(t);		/* this _should_ be true */
	if (t) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY,
				    "glCompressedTexSubImage3D");
			return;
		}
	}

	_mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset,
					     yoffset, width, height, format,
					     imageSize, data, texObj, texImage);

	t->dirty_images[face] |= (1 << level);
}

static void r300TexImage3D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint depth,
			   GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;

	if (t) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
			return;
		}
	}

	texImage->IsClientData = GL_FALSE;

#if 0
	if (r300ValidateClientStorage(ctx, target,
				      internalFormat,
				      width, height,
				      format, type, pixels,
				      packing, texObj, texImage)) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: Using client storage\n",
				__FUNCTION__);
	} else
#endif
	{
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: Using normal storage\n",
				__FUNCTION__);

		/* Normal path: copy (to cached memory) and eventually upload
		 * via another copy to GART memory and then a blit...  Could
		 * eliminate one copy by going straight to (permanent) GART.
		 *
		 * Note, this will call r300ChooseTextureFormat.
		 */
		_mesa_store_teximage3d(ctx, target, level, internalFormat,
				       width, height, depth, border,
				       format, type, pixels,
				       &ctx->Unpack, texObj, texImage);

		t->dirty_images[0] |= (1 << level);
	}
}

static void
r300TexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
		  GLint xoffset, GLint yoffset, GLint zoffset,
		  GLsizei width, GLsizei height, GLsizei depth,
		  GLenum format, GLenum type,
		  const GLvoid * pixels,
		  const struct gl_pixelstore_attrib *packing,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage)
{
	driTextureObject *t = (driTextureObject *) texObj->DriverData;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

	assert(t);		/* this _should_ be true */
	if (t) {
		driSwapOutTextureObject(t);
	} else {
		t = (driTextureObject *) r300AllocTexObj(texObj);
		if (!t) {
			_mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage3D");
			return;
		}
		texObj->DriverData = t;
	}

	_mesa_store_texsubimage3d(ctx, target, level, xoffset, yoffset, zoffset,
				  width, height, depth,
				  format, type, pixels, packing, texObj,
				  texImage);

	t->dirty_images[0] |= (1 << level);
}

static void r300TexEnv(GLcontext * ctx, GLenum target,
		       GLenum pname, const GLfloat * param)
{
	if (RADEON_DEBUG & DEBUG_STATE) {
		fprintf(stderr, "%s( %s )\n",
			__FUNCTION__, _mesa_lookup_enum_by_nr(pname));
	}

	/* This is incorrect: Need to maintain this data for each of
	 * GL_TEXTURE_{123}D, GL_TEXTURE_RECTANGLE_NV, etc, and switch
	 * between them according to _ReallyEnabled.
	 */
	switch (pname) {
	case GL_TEXTURE_LOD_BIAS_EXT:{
#if 0				/* Needs to be relocated in order to make sure we got the right tmu */
			GLfloat bias, min;
			GLuint b;

			/* The R300's LOD bias is a signed 2's complement value with a
			 * range of -16.0 <= bias < 16.0.
			 *
			 * NOTE: Add a small bias to the bias for conform mipsel.c test.
			 */
			bias = *param + .01;
			min =
			    driQueryOptionb(&rmesa->radeon.optionCache,
					    "no_neg_lod_bias") ? 0.0 : -16.0;
			bias = CLAMP(bias, min, 16.0);

			/* 0.0 - 16.0 == 0x0 - 0x1000 */
			/* 0.0 - -16.0 == 0x1001 - 0x1fff */
			b = 0x1000 / 16.0 * bias;
			b &= R300_LOD_BIAS_MASK;

			if (b !=
			    (rmesa->hw.tex.unknown1.
			     cmd[R300_TEX_VALUE_0 +
				 unit] & R300_LOD_BIAS_MASK)) {
				R300_STATECHANGE(rmesa, tex.unknown1);
				rmesa->hw.tex.unknown1.cmd[R300_TEX_VALUE_0 +
							   unit] &=
				    ~R300_LOD_BIAS_MASK;
				rmesa->hw.tex.unknown1.cmd[R300_TEX_VALUE_0 +
							   unit] |= b;
			}
#endif
			break;
		}

	default:
		return;
	}
}

/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void r300TexParameter(GLcontext * ctx, GLenum target,
			     struct gl_texture_object *texObj,
			     GLenum pname, const GLfloat * params)
{
	r300TexObjPtr t = (r300TexObjPtr) texObj->DriverData;

	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_TEXTURE)) {
		fprintf(stderr, "%s( %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(pname));
	}

	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		r300SetTexMaxAnisotropy(t, texObj->MaxAnisotropy);
		r300SetTexFilter(t, texObj->MinFilter, texObj->MagFilter);
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
		r300SetTexWrap(t, texObj->WrapS, texObj->WrapT, texObj->WrapR);
		break;

	case GL_TEXTURE_BORDER_COLOR:
		r300SetTexBorderColor(t, texObj->_BorderChan);
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
		driSwapOutTextureObject((driTextureObject *) t);
		break;

	default:
		return;
	}

	/* Mark this texobj as dirty (one bit per tex unit)
	 */
	t->dirty_state = TEX_ALL;
}

static void r300BindTexture(GLcontext * ctx, GLenum target,
			    struct gl_texture_object *texObj)
{
	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_TEXTURE)) {
		fprintf(stderr, "%s( %p ) unit=%d\n", __FUNCTION__,
			(void *)texObj, ctx->Texture.CurrentUnit);
	}

	if ((target == GL_TEXTURE_1D)
	    || (target == GL_TEXTURE_2D)
	    || (target == GL_TEXTURE_3D)
	    || (target == GL_TEXTURE_CUBE_MAP)
	    || (target == GL_TEXTURE_RECTANGLE_NV)) {
		assert(texObj->DriverData != NULL);
	}
}

static void r300DeleteTexture(GLcontext * ctx, struct gl_texture_object *texObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	driTextureObject *t = (driTextureObject *) texObj->DriverData;

	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_TEXTURE)) {
		fprintf(stderr, "%s( %p (target = %s) )\n", __FUNCTION__,
			(void *)texObj,
			_mesa_lookup_enum_by_nr(texObj->Target));
	}

	if (t != NULL) {
		if (rmesa) {
			R300_FIREVERTICES(rmesa);
		}

		driDestroyTextureObject(t);
	}
	/* Free mipmap images and the texture object itself */
	_mesa_delete_texture_object(ctx, texObj);
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 * Fixup MaxAnisotropy according to user preference.
 */
static struct gl_texture_object *r300NewTextureObject(GLcontext * ctx,
						      GLuint name,
						      GLenum target)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_object *obj;
	obj = _mesa_new_texture_object(ctx, name, target);
	if (!obj)
		return NULL;
	obj->MaxAnisotropy = rmesa->initialMaxAnisotropy;

	r300AllocTexObj(obj);
	return obj;
}

void r300InitTextureFuncs(struct dd_function_table *functions)
{
	/* Note: we only plug in the functions we implement in the driver
	 * since _mesa_init_driver_functions() was already called.
	 */
	functions->ChooseTextureFormat = r300ChooseTextureFormat;
	functions->TexImage1D = r300TexImage1D;
	functions->TexImage2D = r300TexImage2D;
	functions->TexImage3D = r300TexImage3D;
	functions->TexSubImage1D = r300TexSubImage1D;
	functions->TexSubImage2D = r300TexSubImage2D;
	functions->TexSubImage3D = r300TexSubImage3D;
	functions->NewTextureObject = r300NewTextureObject;
	functions->BindTexture = r300BindTexture;
	functions->DeleteTexture = r300DeleteTexture;
	functions->IsTextureResident = driIsTextureResident;

	functions->TexEnv = r300TexEnv;
	functions->TexParameter = r300TexParameter;

	functions->CompressedTexImage2D = r300CompressedTexImage2D;
	functions->CompressedTexSubImage2D = r300CompressedTexSubImage2D;

	driInitTextureFormats();
}
