/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "mm.h"
#include "savagecontext.h"
#include "savagetex.h"
#include "savagetris.h"
#include "savageioctl.h"
#include "simple_list.h"
#include "enums.h"
#include "savage_bci.h"

#include "macros.h"
#include "texformat.h"
#include "texstore.h"
#include "texobj.h"

#include "convolve.h"
#include "colormac.h"

#include "swrast/swrast.h"

#include "xmlpool.h"

#define TILE_INDEX_DXT1 0
#define TILE_INDEX_8    1
#define TILE_INDEX_16   2
#define TILE_INDEX_DXTn 3
#define TILE_INDEX_32   4

/* On Savage4 the texure LOD-bias needs an offset of ~ 0.3 to get
 * somewhere close to software rendering.
 */
#define SAVAGE4_LOD_OFFSET 10

/* Tile info for S3TC formats counts in 4x4 blocks instead of texels.
 * In DXT1 each block is encoded in 64 bits. In DXT3 and 5 each block is
 * encoded in 128 bits. */

/* Size 1, 2 and 4 images are packed into the last subtile. Each image
 * is repeated to fill a 4x4 pixel area. The figure below shows the
 * layout of those 4x4 pixel areas in the 8x8 subtile.
 *
 *    4 2
 *    x 1
 *
 * Yuck! 8-bit texture formats use 4x8 subtiles. See below.
 */
static const savageTileInfo tileInfo_pro[5] = {
    {16, 16, 16, 8, 1, 2, {0x18, 0x10}}, /* DXT1 */
    {64, 32, 16, 4, 4, 8, {0x30, 0x20}}, /* 8-bit */
    {64, 16,  8, 2, 8, 8, {0x48, 0x08}}, /* 16-bit */
    {16,  8, 16, 4, 1, 2, {0x30, 0x20}}, /* DXT3, DXT5 */
    {32, 16,  4, 2, 8, 8, {0x90, 0x10}}, /* 32-bit */
};

/* Size 1, 2 and 4 images are packed into the last two subtiles. Each
 * image is repeated to fill a 4x4 pixel area. The figures below show
 * the layout of those 4x4 pixel areas in the two 4x8 subtiles.
 *
 * second last subtile: 4   last subtile: 2
 *                      x                 1
 */
static const savageTileInfo tileInfo_s3d_s4[5] = {
    {16, 16, 16, 8, 1, 2, {0x18, 0x10}}, /* DXT1 */
    {64, 32, 16, 4, 4, 8, {0x30, 0x20}}, /* 8-bit */
    {64, 16, 16, 2, 4, 8, {0x60, 0x40}}, /* 16-bit */
    {16,  8, 16, 4, 1, 2, {0x30, 0x20}}, /* DXT3, DXT5 */
    {32, 16,  8, 2, 4, 8, {0xc0, 0x80}}, /* 32-bit */
};

/** \brief Template for subtile uploads.
 * \param h   height in pixels
 * \param w   width in bytes
 */
#define SUBTILE_FUNC(w,h)					\
static __inline GLubyte *savageUploadSubtile_##w##x##h		\
(GLubyte *dest, GLubyte *src, GLuint srcStride)			\
{								\
    GLuint y;							\
    for (y = 0; y < h; ++y) {					\
	memcpy (dest, src, w);					\
	src += srcStride;					\
	dest += w;						\
    }								\
    return dest;						\
}

SUBTILE_FUNC(2, 8) /* 4 bits per pixel, 4 pixels wide */
SUBTILE_FUNC(4, 8)
SUBTILE_FUNC(8, 8)
SUBTILE_FUNC(16, 8)
SUBTILE_FUNC(32, 8) /* 4 bytes per pixel, 8 pixels wide */

SUBTILE_FUNC(8, 2) /* DXT1 */
SUBTILE_FUNC(16, 2) /* DXT3 and DXT5 */

/** \brief Upload a complete tile from src (srcStride) to dest
 *
 * \param tileInfo     Pointer to tiling information
 * \param wInSub       Width of source/dest image in subtiles
 * \param hInSub       Height of source/dest image in subtiles
 * \param bpp          Bytes per pixel
 * \param src          Pointer to source data
 * \param srcStride    Byte stride of rows in the source data
 * \param dest         Pointer to destination
 *
 * Writes linearly to the destination memory in order to exploit write
 * combining.
 *
 * For a complete tile wInSub and hInSub are set to the same values as
 * in tileInfo. If the source image is smaller than a whole tile in
 * one or both dimensions then they are set to the values of the
 * source image. This only works as long as the source image is bigger
 * than 8x8 pixels.
 */
static void savageUploadTile (const savageTileInfo *tileInfo,
			      GLuint wInSub, GLuint hInSub, GLuint bpp,
			      GLubyte *src, GLuint srcStride, GLubyte *dest) {
    GLuint subStride = tileInfo->subWidth * bpp;
    GLubyte *srcSRow = src, *srcSTile = src;
    GLubyte *(*subtileFunc) (GLubyte *, GLubyte *, GLuint);
    GLuint sx, sy;
    switch (subStride) {
    case  2: subtileFunc = savageUploadSubtile_2x8; break;
    case  4: subtileFunc = savageUploadSubtile_4x8; break;
    case  8: subtileFunc = tileInfo->subHeight == 8 ?
		 savageUploadSubtile_8x8 : savageUploadSubtile_8x2; break;
    case 16: subtileFunc = tileInfo->subHeight == 8 ?
		 savageUploadSubtile_16x8 : savageUploadSubtile_16x2; break;
    case 32: subtileFunc = savageUploadSubtile_32x8; break;
    default: assert(0);
    }
    for (sy = 0; sy < hInSub; ++sy) {
	srcSTile = srcSRow;
	for (sx = 0; sx < wInSub; ++sx) {
	    src = srcSTile;
	    dest = subtileFunc (dest, src, srcStride);
	    srcSTile += subStride;
	}
	srcSRow += srcStride * tileInfo->subHeight;
    }
}

/** \brief Upload a image that is smaller than 8 pixels in either dimension.
 *
 * \param tileInfo    Pointer to tiling information
 * \param width       Width of the image
 * \param height      Height of the image
 * \param bpp         Bytes per pixel
 * \param src         Pointer to source data
 * \param dest        Pointer to destination
 *
 * This function handles all the special cases that need to be taken
 * care off. The caller may need to call this function multiple times
 * with the destination offset in different ways since small texture
 * images must be repeated in order to fill a whole tile (or 4x4 for
 * the last 3 levels).
 *
 * FIXME: Repeating inside this function would be more efficient.
 */
static void savageUploadTiny (const savageTileInfo *tileInfo,
			      GLuint pixWidth, GLuint pixHeight,
			      GLuint width, GLuint height, GLuint bpp,
			      GLubyte *src, GLubyte *dest) {
    GLuint size = MAX2(pixWidth, pixHeight);

    if (width > tileInfo->subWidth) { /* assert: height <= subtile height */
	GLuint wInSub = width / tileInfo->subWidth;
	GLuint srcStride = width * bpp;
	GLuint subStride = tileInfo->subWidth * bpp;
	GLuint subSkip = (tileInfo->subHeight - height) * subStride;
	GLubyte *srcSTile = src;
	GLuint sx, y;
	for (sx = 0; sx < wInSub; ++sx) {
	    src = srcSTile;
	    for (y = 0; y < height; ++y) {
		memcpy (dest, src, subStride);
		src += srcStride;
		dest += subStride;
	    }
	    dest += subSkip;
	    srcSTile += subStride;
	}
    } else if (size > 4) { /* a tile or less wide, except the last 3 levels */
	GLuint srcStride = width * bpp;
	GLuint subStride = tileInfo->subWidth * bpp;
	/* if the subtile width is 4 we have to skip every other subtile */
	GLuint subSkip = tileInfo->subWidth <= 4 ?
	    subStride * tileInfo->subHeight : 0;
	GLuint skipRemainder = tileInfo->subHeight - 1;
	GLuint y;
	for (y = 0; y < height; ++y) {
	    memcpy (dest, src, srcStride);
	    src += srcStride;
	    dest += subStride;
	    if ((y & skipRemainder) == skipRemainder)
		dest += subSkip;
	}
    } else { /* the last 3 mipmap levels */
	GLuint offset = (size <= 2 ? tileInfo->tinyOffset[size-1] : 0);
	GLuint subStride = tileInfo->subWidth * bpp;
	GLuint y;
	dest += offset;
	for (y = 0; y < height; ++y) {
	    memcpy (dest, src, bpp*width);
	    src += width * bpp;
	    dest += subStride;
	}
    }
}

/** \brief Upload an image from mesa's internal copy.
 */
static void savageUploadTexLevel( savageTexObjPtr t, int level )
{
    const struct gl_texture_image *image = t->base.tObj->Image[0][level];
    const savageTileInfo *tileInfo = t->tileInfo;
    GLuint pixWidth = image->Width2, pixHeight = image->Height2;
    GLuint bpp = t->texelBytes;
    GLuint width, height;

    /* FIXME: Need triangle (rather than pixel) fallbacks to simulate
     * this using normal textured triangles.
     *
     * DO THIS IN DRIVER STATE MANAGMENT, not hardware state.
     */
    if(image->Border != 0) 
	fprintf (stderr, "Not supported texture border %d.\n",
		 (int) image->Border);

    if (t->hwFormat == TFT_S3TC4A4Bit || t->hwFormat == TFT_S3TC4CA4Bit ||
	t->hwFormat == TFT_S3TC4Bit) {
	width = (pixWidth+3) / 4;
	height = (pixHeight+3) / 4;
    } else {
	width = pixWidth;
	height = pixHeight;
    }

    if (pixWidth >= 8 && pixHeight >= 8) {
	GLuint *dirtyPtr = t->image[level].dirtyTiles;
	GLuint dirtyMask = 1;

	if (width >= tileInfo->width && height >= tileInfo->height) {
	    GLuint wInTiles = width / tileInfo->width;
	    GLuint hInTiles = height / tileInfo->height;
	    GLubyte *srcTRow = image->Data, *src;
	    GLubyte *dest = (GLubyte *)(t->bufAddr + t->image[level].offset);
	    GLuint x, y;
	    for (y = 0; y < hInTiles; ++y) {
		src = srcTRow;
		for (x = 0; x < wInTiles; ++x) {
		    if (*dirtyPtr & dirtyMask) {
			savageUploadTile (tileInfo,
					  tileInfo->wInSub, tileInfo->hInSub,
					  bpp, src, width * bpp, dest);
		    }
		    src += tileInfo->width * bpp;
		    dest += 2048; /* tile size is always 2k */
		    if (dirtyMask == 1<<31) {
			dirtyMask = 1;
			dirtyPtr++;
		    } else
			dirtyMask <<= 1;
		}
		srcTRow += width * tileInfo->height * bpp;
	    }
	} else if (width >= tileInfo->width) {
	    GLuint wInTiles = width / tileInfo->width;
	    GLubyte *src = image->Data;
	    GLubyte *dest = (GLubyte *)(t->bufAddr + t->image[level].offset);
	    GLuint tileStride = tileInfo->width * bpp * height;
	    savageContextPtr imesa = (savageContextPtr)t->base.heap->driverContext;
	    GLuint x;
	    /* Savage3D-based chips seem so use a constant tile stride
	     * of 2048 for vertically incomplete tiles, but only if
	     * the color depth is 32bpp. Nobody said this was supposed
	     * to be logical!
	     */
	    if (bpp == 4 && imesa->savageScreen->chipset < S3_SAVAGE4)
		tileStride = 2048;
	    for (x = 0; x < wInTiles; ++x) {
		if (*dirtyPtr & dirtyMask) {
		    savageUploadTile (tileInfo,
				      tileInfo->wInSub,
				      height / tileInfo->subHeight,
				      bpp, src, width * bpp, dest);
		}
		src += tileInfo->width * bpp;
		dest += tileStride;
		if (dirtyMask == 1<<31) {
		    dirtyMask = 1;
		    dirtyPtr++;
		} else
		    dirtyMask <<= 1;
	    }
	} else {
	    savageUploadTile (tileInfo, width / tileInfo->subWidth,
			      height / tileInfo->subHeight, bpp,
			      image->Data, width * bpp,
			      (GLubyte *)(t->bufAddr+t->image[level].offset));
	}
    } else {
	GLuint minHeight, minWidth, hRepeat, vRepeat, x, y;
	if (t->hwFormat == TFT_S3TC4A4Bit || t->hwFormat == TFT_S3TC4CA4Bit ||
	    t->hwFormat == TFT_S3TC4Bit)
	    minWidth = minHeight = 1;
	else
	    minWidth = minHeight = 4;
	if (width > minWidth || height > minHeight) {
	    minWidth = tileInfo->subWidth;
	    minHeight = tileInfo->subHeight;
	}
	hRepeat = width  >= minWidth  ? 1 : minWidth  / width;
	vRepeat = height >= minHeight ? 1 : minHeight / height;
	for (y = 0; y < vRepeat; ++y) {
	    GLuint offset = y * tileInfo->subWidth*height * bpp;
	    for (x = 0; x < hRepeat; ++x) {
		savageUploadTiny (tileInfo, pixWidth, pixHeight,
				  width, height, bpp, image->Data,
				  (GLubyte *)(t->bufAddr +
					      t->image[level].offset+offset));
		offset += width * bpp;
	    }
	}
    }
}

/** \brief Compute the destination size of a texture image
 */
static GLuint savageTexImageSize (GLuint width, GLuint height, GLuint bpp) {
    /* full subtiles */
    if (width >= 8 && height >= 8)
	return width * height * bpp;
    /* special case for the last three mipmap levels: the hardware computes
     * the offset internally */
    else if (width <= 4 && height <= 4)
	return 0;
    /* partially filled sub tiles waste memory
     * on Savage3D and Savage4 with subtile width 4 every other subtile is
     * skipped if width < 8 so we can assume a uniform subtile width of 8 */
    else if (width >= 8)
	return width * 8 * bpp;
    else if (height >= 8)
	return 8 * height * bpp;
    else
	return 64 * bpp;
}

/** \brief Compute the destination size of a compressed texture image
 */
static GLuint savageCompressedTexImageSize (GLuint width, GLuint height,
					    GLuint bpp) {
    width = (width+3) / 4;
    height = (height+3) / 4;
    /* full subtiles */
    if (width >= 2 && height >= 2)
	return width * height * bpp;
    /* special case for the last three mipmap levels: the hardware computes
     * the offset internally */
    else if (width <= 1 && height <= 1)
	return 0;
    /* partially filled sub tiles waste memory
     * on Savage3D and Savage4 with subtile width 4 every other subtile is
     * skipped if width < 8 so we can assume a uniform subtile width of 8 */
    else if (width >= 2)
	return width * 2 * bpp;
    else if (height >= 2)
	return 2 * height * bpp;
    else
	return 4 * bpp;
}

/** \brief Compute the number of (partial) tiles of a texture image
 */
static GLuint savageTexImageTiles (GLuint width, GLuint height,
				   const savageTileInfo *tileInfo)
{
   return (width + tileInfo->width - 1) / tileInfo->width *
      (height + tileInfo->height - 1) / tileInfo->height;
}

/** \brief Mark dirty tiles
 *
 * Some care must be taken because tileInfo may not be set or not
 * up-to-date. So we check if tileInfo is initialized and if the number
 * of tiles in the bit vector matches the number of tiles computed from
 * the current tileInfo.
 */
static void savageMarkDirtyTiles (savageTexObjPtr t, GLuint level,
				  GLuint totalWidth, GLuint totalHeight,
				  GLint xoffset, GLint yoffset,
				  GLsizei width, GLsizei height)
{
   GLuint wInTiles, hInTiles;
   GLuint x0, y0, x1, y1;
   GLuint x, y;
   if (!t->tileInfo)
      return;
   wInTiles = (totalWidth + t->tileInfo->width - 1) / t->tileInfo->width;
   hInTiles = (totalHeight + t->tileInfo->height - 1) / t->tileInfo->height;
   if (wInTiles * hInTiles != t->image[level].nTiles)
      return;

   x0 = xoffset / t->tileInfo->width;
   y0 = yoffset / t->tileInfo->height;
   x1 = (xoffset + width - 1) / t->tileInfo->width;
   y1 = (yoffset + height - 1) / t->tileInfo->height;

   for (y = y0; y <= y1; ++y) {
      GLuint *ptr = t->image[level].dirtyTiles + (y * wInTiles + x0) / 32;
      GLuint mask = 1 << (y * wInTiles + x0) % 32;
      for (x = x0; x <= x1; ++x) {
	 *ptr |= mask;
	 if (mask == (1<<31)) {
	    ptr++;
	    mask = 1;
	 } else {
	    mask <<= 1;
	 }
      }
   }
}

/** \brief Mark all tiles as dirty
 */
static void savageMarkAllTiles (savageTexObjPtr t, GLuint level)
{
   GLuint words = (t->image[level].nTiles + 31) / 32;
   if (words)
      memset(t->image[level].dirtyTiles, ~0, words*sizeof(GLuint));
}


static void savageSetTexWrapping(savageTexObjPtr tex, GLenum s, GLenum t)
{
    tex->setup.sWrapMode = s;
    tex->setup.tWrapMode = t;
}

static void savageSetTexFilter(savageTexObjPtr t, GLenum minf, GLenum magf)
{
   t->setup.minFilter = minf;
   t->setup.magFilter = magf;
}


/* Need a fallback ?
 */
static void savageSetTexBorderColor(savageTexObjPtr t, GLubyte color[4])
{
/*    t->Setup[SAVAGE_TEXREG_TEXBORDERCOL] =  */
    /*t->setup.borderColor = SAVAGEPACKCOLOR8888(color[0],color[1],color[2],color[3]); */
}



static savageTexObjPtr
savageAllocTexObj( struct gl_texture_object *texObj ) 
{
   savageTexObjPtr t;

   t = (savageTexObjPtr) calloc(1,sizeof(*t));
   texObj->DriverData = t;
   if ( t != NULL ) {
      GLuint i;

      /* Initialize non-image-dependent parts of the state:
       */
      t->base.tObj = texObj;
      t->base.dirty_images[0] = 0;
      t->dirtySubImages = 0;
      t->tileInfo = NULL;

      /* Initialize dirty tiles bit vectors
       */
      for (i = 0; i < SAVAGE_TEX_MAXLEVELS; ++i)
	 t->image[i].nTiles = 0;

      /* FIXME Something here to set initial values for other parts of
       * FIXME t->setup?
       */
  
      make_empty_list( &t->base );

      savageSetTexWrapping(t,texObj->WrapS,texObj->WrapT);
      savageSetTexFilter(t,texObj->MinFilter,texObj->MagFilter);
      savageSetTexBorderColor(t,texObj->_BorderChan);
   }

   return t;
}

/* Mesa texture formats for alpha-images on Savage3D/IX/MX
 *
 * Promoting texture images to ARGB888 or ARGB4444 doesn't work
 * because we can't tell the hardware to ignore the color components
 * and only use the alpha component. So we define our own texture
 * formats that promote to ARGB8888 or ARGB4444 and set the color
 * components to white. This way we get the correct result.
 */

static GLboolean
_savage_texstore_a1114444(TEXSTORE_PARAMS);

static GLboolean
_savage_texstore_a1118888(TEXSTORE_PARAMS);

static struct gl_texture_format _savage_texformat_a1114444 = {
    MESA_FORMAT_ARGB4444,		/* MesaFormat */
    GL_RGBA,				/* BaseFormat */
    GL_UNSIGNED_NORMALIZED_ARB,		/* DataType */
    4,					/* RedBits */
    4,					/* GreenBits */
    4,					/* BlueBits */
    4,					/* AlphaBits */
    0,					/* LuminanceBits */
    0,					/* IntensityBits */
    0,					/* IndexBits */
    0,					/* DepthBits */
    0,					/* StencilBits */
    2,					/* TexelBytes */
    _savage_texstore_a1114444,		/* StoreTexImageFunc */
    NULL, NULL, NULL, NULL, NULL, NULL  /* FetchTexel* filled in by 
					 * savageDDInitTextureFuncs */
};
static struct gl_texture_format _savage_texformat_a1118888 = {
    MESA_FORMAT_ARGB8888,		/* MesaFormat */
    GL_RGBA,				/* BaseFormat */
    GL_UNSIGNED_NORMALIZED_ARB,		/* DataType */
    8,					/* RedBits */
    8,					/* GreenBits */
    8,					/* BlueBits */
    8,					/* AlphaBits */
    0,					/* LuminanceBits */
    0,					/* IntensityBits */
    0,					/* IndexBits */
    0,					/* DepthBits */
    0,					/* StencilBits */
    4,					/* TexelBytes */
    _savage_texstore_a1118888,		/* StoreTexImageFunc */
    NULL, NULL, NULL, NULL, NULL, NULL  /* FetchTexel* filled in by 
					 * savageDDInitTextureFuncs */
};


static GLboolean
_savage_texstore_a1114444(TEXSTORE_PARAMS)
{
    const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseInternalFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
    const GLchan *src = tempImage;
    GLint img, row, col;

    ASSERT(dstFormat == &_savage_texformat_a1114444);
    ASSERT(baseInternalFormat == GL_ALPHA);

    if (!tempImage)
	return GL_FALSE;
    _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
    for (img = 0; img < srcDepth; img++) {
        GLubyte *dstRow = (GLubyte *) dstAddr
           + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
           + dstYoffset * dstRowStride
           + dstXoffset * dstFormat->TexelBytes;
	for (row = 0; row < srcHeight; row++) {
            GLushort *dstUI = (GLushort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
		dstUI[col] = PACK_COLOR_4444( CHAN_TO_UBYTE(src[0]),
					      255, 255, 255 );
		src += 1;
            }
            dstRow += dstRowStride;
	}
    }
    _mesa_free((void *) tempImage);

    return GL_TRUE;
}


static GLboolean
_savage_texstore_a1118888(TEXSTORE_PARAMS)
{
    const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseInternalFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
    const GLchan *src = tempImage;
    GLint img, row, col;

    ASSERT(dstFormat == &_savage_texformat_a1118888);
    ASSERT(baseInternalFormat == GL_ALPHA);

    if (!tempImage)
	return GL_FALSE;
    _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
    for (img = 0; img < srcDepth; img++) {
        GLubyte *dstRow = (GLubyte *) dstAddr
           + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
           + dstYoffset * dstRowStride
           + dstXoffset * dstFormat->TexelBytes;
	for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
		dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[0]),
					      255, 255, 255 );
		src += 1;
            }
            dstRow += dstRowStride;
	}
    }
    _mesa_free((void *) tempImage);

    return GL_TRUE;
}


/* Called by the _mesa_store_teximage[123]d() functions. */
static const struct gl_texture_format *
savageChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			   GLenum format, GLenum type )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   const GLboolean do32bpt =
       ( imesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32 );
   const GLboolean force16bpt =
       ( imesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16 );
   const GLboolean isSavage4 = (imesa->savageScreen->chipset >= S3_SAVAGE4);
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
   case GL_RGBA12:
   case GL_RGBA16:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGB10_A2:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555;

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
   case GL_COMPRESSED_ALPHA:
      return isSavage4 ? &_mesa_texformat_a8 : (
	 do32bpt ? &_savage_texformat_a1118888 : &_savage_texformat_a1114444);
   case GL_ALPHA4:
      return isSavage4 ? &_mesa_texformat_a8 : &_savage_texformat_a1114444;
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return isSavage4 ? &_mesa_texformat_a8 : (
	 !force16bpt ? &_savage_texformat_a1118888 : &_savage_texformat_a1114444);

   case 1:
   case GL_LUMINANCE:
   case GL_COMPRESSED_LUMINANCE:
      /* no alpha, but use argb1555 in 16bit case to get pure grey values */
      return isSavage4 ? &_mesa_texformat_l8 : (
	 do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555);
   case GL_LUMINANCE4:
      return isSavage4 ? &_mesa_texformat_l8 : &_mesa_texformat_argb1555;
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      return isSavage4 ? &_mesa_texformat_l8 : (
	 !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555);

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      /* Savage4 has a al44 texture format. But it's not supported by Mesa. */
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
      return &_mesa_texformat_argb4444;
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      return !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
#if 0
   /* TFT_I8 produces garbage on ProSavageDDR and subsequent texture
    * disable keeps rendering garbage. Disabled for now. */
   case GL_INTENSITY:
   case GL_COMPRESSED_INTENSITY:
      return isSavage4 ? &_mesa_texformat_i8 : (
	 do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444);
   case GL_INTENSITY4:
      return isSavage4 ? &_mesa_texformat_i8 : &_mesa_texformat_argb4444;
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return isSavage4 ? &_mesa_texformat_i8 : (
	 !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444);
#else
   case GL_INTENSITY:
   case GL_COMPRESSED_INTENSITY:
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
   case GL_INTENSITY4:
      return &_mesa_texformat_argb4444;
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return !force16bpt ? &_mesa_texformat_argb8888 :
	  &_mesa_texformat_argb4444;
#endif

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgb_dxt1;
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;

   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return &_mesa_texformat_rgba_dxt3;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      if (!isSavage4)
	 /* Not the best choice but Savage3D/MX/IX don't support DXT3 or DXT5. */
	 return &_mesa_texformat_rgba_dxt1;
      /* fall through */
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;

/*
   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;
*/
   default:
      _mesa_problem(ctx, "unexpected texture format in %s", __FUNCTION__);
      return NULL;
   }
}

static void savageSetTexImages( savageContextPtr imesa,
				const struct gl_texture_object *tObj )
{
   savageTexObjPtr t = (savageTexObjPtr) tObj->DriverData;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   GLuint offset, i, textureFormat, tileIndex, size;
   GLint firstLevel, lastLevel;

   assert(t);
   assert(image);

   switch (image->TexFormat->MesaFormat) {
   case MESA_FORMAT_ARGB8888:
      textureFormat = TFT_ARGB8888;
      t->texelBytes = tileIndex = 4;
      break;
   case MESA_FORMAT_ARGB1555:
      textureFormat = TFT_ARGB1555;
      t->texelBytes = tileIndex = 2;
      break;
   case MESA_FORMAT_ARGB4444:
      textureFormat = TFT_ARGB4444;
      t->texelBytes = tileIndex = 2;
      break;
   case MESA_FORMAT_RGB565:
      textureFormat = TFT_RGB565;
      t->texelBytes = tileIndex = 2;
      break;
   case MESA_FORMAT_L8:
      textureFormat = TFT_L8;
      t->texelBytes = tileIndex = 1;
      break;
   case MESA_FORMAT_I8:
      textureFormat = TFT_I8;
      t->texelBytes = tileIndex = 1;
      break;
   case MESA_FORMAT_A8:
      textureFormat = TFT_A8;
      t->texelBytes = tileIndex = 1;
      break;
   case MESA_FORMAT_RGB_DXT1:
      textureFormat = TFT_S3TC4Bit;
      tileIndex = TILE_INDEX_DXT1;
      t->texelBytes = 8;
      break;
   case MESA_FORMAT_RGBA_DXT1:
      textureFormat = TFT_S3TC4Bit;
      tileIndex = TILE_INDEX_DXT1;
      t->texelBytes = 8;
      break;
   case MESA_FORMAT_RGBA_DXT3:
      textureFormat =  TFT_S3TC4A4Bit;
      tileIndex = TILE_INDEX_DXTn;
      t->texelBytes = 16;
      break;
   case MESA_FORMAT_RGBA_DXT5:
      textureFormat = TFT_S3TC4CA4Bit;
      tileIndex = TILE_INDEX_DXTn;
      t->texelBytes = 16;
      break;
   default:
      _mesa_problem(imesa->glCtx, "Bad texture format in %s", __FUNCTION__);
      return;
   }
   t->hwFormat = textureFormat;

   /* Select tiling format depending on the chipset and texture format */
   if (imesa->savageScreen->chipset <= S3_SAVAGE4)
       t->tileInfo = &tileInfo_s3d_s4[tileIndex];
   else
       t->tileInfo = &tileInfo_pro[tileIndex];

   /* Compute which mipmap levels we really want to send to the hardware.
    */
   driCalculateTextureFirstLastLevel( &t->base );
   firstLevel = t->base.firstLevel;
   lastLevel  = t->base.lastLevel;

   /* Figure out the size now (and count the levels).  Upload won't be
    * done until later. If the number of tiles changes, it means that
    * this function is called for the first time on this tex object or
    * the image or the destination color format changed. So all tiles
    * are marked as dirty.
    */ 
   offset = 0;
   size = 1;
   for ( i = firstLevel ; i <= lastLevel && tObj->Image[0][i] ; i++ ) {
      GLuint nTiles;
      nTiles = savageTexImageTiles (image->Width2, image->Height2, t->tileInfo);
      if (t->image[i].nTiles != nTiles) {
	 GLuint words = (nTiles + 31) / 32;
	 if (t->image[i].nTiles != 0) {
	    free(t->image[i].dirtyTiles);
	 }
	 t->image[i].dirtyTiles = malloc(words*sizeof(GLuint));
	 memset(t->image[i].dirtyTiles, ~0, words*sizeof(GLuint));
      }
      t->image[i].nTiles = nTiles;

      t->image[i].offset = offset;

      image = tObj->Image[0][i];
      if (t->texelBytes >= 8)
	 size = savageCompressedTexImageSize (image->Width2, image->Height2,
					      t->texelBytes);
      else
	 size = savageTexImageSize (image->Width2, image->Height2,
				    t->texelBytes);
      offset += size;
   }

   t->base.lastLevel = i-1;
   t->base.totalSize = offset;
   /* the last three mipmap levels don't add to the offset. They are packed
    * into 64 pixels. */
   if (size == 0)
       t->base.totalSize += (t->texelBytes >= 8 ? 4 : 64) * t->texelBytes;
   /* 2k-aligned (really needed?) */
   t->base.totalSize = (t->base.totalSize + 2047UL) & ~2047UL;
}

void savageDestroyTexObj(savageContextPtr imesa, savageTexObjPtr t)
{
    GLuint i;

    /* Free dirty tiles bit vectors */
    for (i = 0; i < SAVAGE_TEX_MAXLEVELS; ++i) {
	if (t->image[i].nTiles)
	    free (t->image[i].dirtyTiles);
    }

    /* See if it was the driver's current object.
     */
    if ( imesa != NULL )
    { 
	for ( i = 0 ; i < imesa->glCtx->Const.MaxTextureUnits ; i++ )
	{
	    if ( &t->base == imesa->CurrentTexObj[ i ] ) {
		assert( t->base.bound & (1 << i) );
		imesa->CurrentTexObj[ i ] = NULL;
	    }
	}
    }
}

/* Upload a texture's images to one of the texture heaps. May have to
 * eject our own and/or other client's texture objects to make room
 * for the upload.
 */
static void savageUploadTexImages( savageContextPtr imesa, savageTexObjPtr t )
{
   const GLint numLevels = t->base.lastLevel - t->base.firstLevel + 1;
   GLuint i;

   assert(t);

   LOCK_HARDWARE(imesa);
   
   /* Do we need to eject LRU texture objects?
    */
   if (!t->base.memBlock) {
      GLint heap;
      GLuint ofs;

      heap = driAllocateTexture(imesa->textureHeaps, imesa->lastTexHeap,
				(driTextureObject *)t);
      if (heap == -1) {
	  UNLOCK_HARDWARE(imesa);
	  return;
      }

      ofs = t->base.memBlock->ofs;
      t->setup.physAddr = imesa->savageScreen->textureOffset[heap] + ofs;
      t->bufAddr = (GLubyte *)imesa->savageScreen->texVirtual[heap] + ofs;
      imesa->dirty |= SAVAGE_UPLOAD_GLOBAL; /* FIXME: really needed? */
   }

   /* Let the world know we've used this memory recently.
    */
   driUpdateTextureLRU( &t->base );
   UNLOCK_HARDWARE(imesa);

   if (t->base.dirty_images[0] || t->dirtySubImages) {
      if (SAVAGE_DEBUG & DEBUG_VERBOSE_TEX)
	 fprintf(stderr, "Texture upload: |");

      /* Heap timestamps are only reliable with Savage DRM 2.3.x or
       * later. Earlier versions had only 16 bit time stamps which
       * would wrap too frequently. */
      if (imesa->savageScreen->driScrnPriv->drmMinor >= 3) {
	  unsigned int heap = t->base.heap->heapId;
	  LOCK_HARDWARE(imesa);
	  savageWaitEvent (imesa, imesa->textureHeaps[heap]->timestamp);
      } else {
	  savageFlushVertices (imesa);
	  LOCK_HARDWARE(imesa);
	  savageFlushCmdBufLocked (imesa, GL_FALSE);
	  WAIT_IDLE_EMPTY_LOCKED(imesa);
      }

      for (i = 0 ; i < numLevels ; i++) {
         const GLint j = t->base.firstLevel + i;  /* the texObj's level */
	 if (t->base.dirty_images[0] & (1 << j)) {
	    savageMarkAllTiles(t, j);
	    if (SAVAGE_DEBUG & DEBUG_VERBOSE_TEX)
		fprintf (stderr, "*");
	 } else if (SAVAGE_DEBUG & DEBUG_VERBOSE_TEX) {
	    if (t->dirtySubImages & (1 << j))
	       fprintf (stderr, ".");
	    else
	       fprintf (stderr, " ");
	 }
	 if ((t->base.dirty_images[0] | t->dirtySubImages) & (1 << j))
	    savageUploadTexLevel( t, j );
      }

      UNLOCK_HARDWARE(imesa);
      t->base.dirty_images[0] = 0;
      t->dirtySubImages = 0;

      if (SAVAGE_DEBUG & DEBUG_VERBOSE_TEX)
	 fprintf(stderr, "|\n");
   }
}


static void
savage4_set_wrap_mode( savageContextPtr imesa, unsigned unit,
		      GLenum s_mode, GLenum t_mode )
{
    switch( s_mode ) {
    case GL_REPEAT:
	imesa->regs.s4.texCtrl[ unit ].ni.uMode = TAM_Wrap;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	imesa->regs.s4.texCtrl[ unit ].ni.uMode = TAM_Clamp;
	break;
    case GL_MIRRORED_REPEAT:
	imesa->regs.s4.texCtrl[ unit ].ni.uMode = TAM_Mirror;
	break;
    }

    switch( t_mode ) {
    case GL_REPEAT:
	imesa->regs.s4.texCtrl[ unit ].ni.vMode = TAM_Wrap;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	imesa->regs.s4.texCtrl[ unit ].ni.vMode = TAM_Clamp;
	break;
    case GL_MIRRORED_REPEAT:
	imesa->regs.s4.texCtrl[ unit ].ni.vMode = TAM_Mirror;
	break;
    }
}


/**
 * Sets the hardware bits for the specified GL texture filter modes.
 * 
 * \todo
 * Does the Savage4 have the ability to select the magnification filter?
 */
static void
savage4_set_filter_mode( savageContextPtr imesa, unsigned unit,
			 GLenum minFilter, GLenum magFilter )
{
    (void) magFilter;

    switch (minFilter) {
    case GL_NEAREST:
	imesa->regs.s4.texCtrl[ unit ].ni.filterMode   = TFM_Point;
	imesa->regs.s4.texCtrl[ unit ].ni.mipmapEnable = GL_FALSE;
	break;

    case GL_LINEAR:
	imesa->regs.s4.texCtrl[ unit ].ni.filterMode   = TFM_Bilin;
	imesa->regs.s4.texCtrl[ unit ].ni.mipmapEnable = GL_FALSE;
	break;

    case GL_NEAREST_MIPMAP_NEAREST:
	imesa->regs.s4.texCtrl[ unit ].ni.filterMode   = TFM_Point;
	imesa->regs.s4.texCtrl[ unit ].ni.mipmapEnable = GL_TRUE;
	break;

    case GL_LINEAR_MIPMAP_NEAREST:
	imesa->regs.s4.texCtrl[ unit ].ni.filterMode   = TFM_Bilin;
	imesa->regs.s4.texCtrl[ unit ].ni.mipmapEnable = GL_TRUE;
	break;

    case GL_NEAREST_MIPMAP_LINEAR:
    case GL_LINEAR_MIPMAP_LINEAR:
	imesa->regs.s4.texCtrl[ unit ].ni.filterMode   = TFM_Trilin;
	imesa->regs.s4.texCtrl[ unit ].ni.mipmapEnable = GL_TRUE;
	break;
    }
}


static void savageUpdateTex0State_s4( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   struct gl_texture_object	*tObj;
   struct gl_texture_image *image;
   savageTexObjPtr t;
   GLuint format;

   /* disable */
   imesa->regs.s4.texDescr.ni.tex0En = GL_FALSE;
   imesa->regs.s4.texBlendCtrl[0].ui = TBC_NoTexMap;
   imesa->regs.s4.texCtrl[0].ui = 0x20f040;
   if (ctx->Texture.Unit[0]._ReallyEnabled == 0)
      return;

   tObj = ctx->Texture.Unit[0]._Current;
   if ((ctx->Texture.Unit[0]._ReallyEnabled & ~(TEXTURE_1D_BIT|TEXTURE_2D_BIT))
       || tObj->Image[0][tObj->BaseLevel]->Border > 0) {
      /* 3D texturing enabled, or texture border - fallback */
      FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
      return;
   }

   /* Do 2D texture setup */

   t = tObj->DriverData;
   if (!t) {
      t = savageAllocTexObj( tObj );
      if (!t)
         return;
   }

   imesa->CurrentTexObj[0] = &t->base;
   t->base.bound |= 1;

   if (t->base.dirty_images[0] || t->dirtySubImages) {
       savageSetTexImages(imesa, tObj);
       savageUploadTexImages(imesa, t); 
   }
   
   driUpdateTextureLRU( &t->base );

   format = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;

   switch (ctx->Texture.Unit[0].EnvMode) {
   case GL_REPLACE:
      imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_FALSE;
      switch(format)
      {
          case GL_LUMINANCE:
          case GL_RGB:
               imesa->regs.s4.texBlendCtrl[0].ui = TBC_Decal;
               break;

          case GL_LUMINANCE_ALPHA:
          case GL_RGBA:
          case GL_INTENSITY:
               imesa->regs.s4.texBlendCtrl[0].ui = TBC_Copy;
               break;

          case GL_ALPHA:
               imesa->regs.s4.texBlendCtrl[0].ui = TBC_CopyAlpha;
               break;
      }
       __HWEnvCombineSingleUnitScale(imesa, 0, 0,
				     &imesa->regs.s4.texBlendCtrl[0]);
      break;

    case GL_DECAL:
        imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_FALSE;
        switch (format)
        {
            case GL_RGB:
            case GL_LUMINANCE:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_Decal;
                break;

            case GL_RGBA:
            case GL_INTENSITY:
            case GL_LUMINANCE_ALPHA:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_DecalAlpha;
                break;

            /*
             GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_INTENSITY
             are undefined with GL_DECAL
            */

            case GL_ALPHA:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_CopyAlpha;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 0,
				      &imesa->regs.s4.texBlendCtrl[0]);
        break;

    case GL_MODULATE:
        imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_FALSE;
        imesa->regs.s4.texBlendCtrl[0].ui = TBC_ModulAlpha;
        __HWEnvCombineSingleUnitScale(imesa, 0, 0,
				      &imesa->regs.s4.texBlendCtrl[0]);
        break;

    case GL_BLEND:
	imesa->regs.s4.texBlendColor.ui = imesa->texEnvColor;

        switch (format)
        {
            case GL_ALPHA:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_ModulAlpha;
                imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_FALSE;
                break;

            case GL_LUMINANCE:
            case GL_RGB:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_Blend0;
                imesa->regs.s4.texDescr.ni.tex1En = GL_TRUE;
                imesa->regs.s4.texDescr.ni.texBLoopEn = GL_TRUE;
                imesa->regs.s4.texDescr.ni.tex1Width  =
		    imesa->regs.s4.texDescr.ni.tex0Width;
                imesa->regs.s4.texDescr.ni.tex1Height =
		    imesa->regs.s4.texDescr.ni.tex0Height;
                imesa->regs.s4.texDescr.ni.tex1Fmt =
		    imesa->regs.s4.texDescr.ni.tex0Fmt;

		imesa->regs.s4.texAddr[1].ui = imesa->regs.s4.texAddr[0].ui;
		imesa->regs.s4.texBlendCtrl[1].ui = TBC_Blend1;

                imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_TRUE;
                imesa->bTexEn1 = GL_TRUE;
                break;

            case GL_LUMINANCE_ALPHA:
            case GL_RGBA:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_BlendAlpha0;
                imesa->regs.s4.texDescr.ni.tex1En = GL_TRUE;
                imesa->regs.s4.texDescr.ni.texBLoopEn = GL_TRUE;
                imesa->regs.s4.texDescr.ni.tex1Width  =
		    imesa->regs.s4.texDescr.ni.tex0Width;
                imesa->regs.s4.texDescr.ni.tex1Height =
		    imesa->regs.s4.texDescr.ni.tex0Height;
                imesa->regs.s4.texDescr.ni.tex1Fmt =
		    imesa->regs.s4.texDescr.ni.tex0Fmt;

		imesa->regs.s4.texAddr[1].ui = imesa->regs.s4.texAddr[0].ui;
		imesa->regs.s4.texBlendCtrl[1].ui = TBC_BlendAlpha1;

                imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_TRUE;
                imesa->bTexEn1 = GL_TRUE;
                break;

            case GL_INTENSITY:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_BlendInt0;
                imesa->regs.s4.texDescr.ni.tex1En = GL_TRUE;
                imesa->regs.s4.texDescr.ni.texBLoopEn = GL_TRUE;
                imesa->regs.s4.texDescr.ni.tex1Width  =
		    imesa->regs.s4.texDescr.ni.tex0Width;
                imesa->regs.s4.texDescr.ni.tex1Height =
		    imesa->regs.s4.texDescr.ni.tex0Height;
                imesa->regs.s4.texDescr.ni.tex1Fmt =
		    imesa->regs.s4.texDescr.ni.tex0Fmt;

		imesa->regs.s4.texAddr[1].ui = imesa->regs.s4.texAddr[0].ui;
		imesa->regs.s4.texBlendCtrl[1].ui = TBC_BlendInt1;

                imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_TRUE;
                imesa->regs.s4.texCtrl[0].ni.alphaArg1Invert = GL_TRUE;
                imesa->bTexEn1 = GL_TRUE;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 0,
				      &imesa->regs.s4.texBlendCtrl[0]);
        break;

    case GL_ADD:
        imesa->regs.s4.texCtrl[0].ni.clrArg1Invert = GL_FALSE;
        switch (format)
        {
            case GL_ALPHA:
                imesa->regs.s4.texBlendCtrl[0].ui = TBC_ModulAlpha;
		break;

            case GL_LUMINANCE:
            case GL_RGB:
		imesa->regs.s4.texBlendCtrl[0].ui = TBC_Add;
		break;

            case GL_LUMINANCE_ALPHA:
            case GL_RGBA:
		imesa->regs.s4.texBlendCtrl[0].ui = TBC_Add;
		break;

            case GL_INTENSITY:
		imesa->regs.s4.texBlendCtrl[0].ui = TBC_AddAlpha;
		break;
	}
        __HWEnvCombineSingleUnitScale(imesa, 0, 0,
				      &imesa->regs.s4.texBlendCtrl[0]);
        break;

#if GL_ARB_texture_env_combine
    case GL_COMBINE_ARB:
        __HWParseTexEnvCombine(imesa, 0, &imesa->regs.s4.texCtrl[0],
			       &imesa->regs.s4.texBlendCtrl[0]);
        break;
#endif

   default:
      fprintf(stderr, "unknown tex env mode");
      exit(1);
      break;			
   }

    savage4_set_wrap_mode( imesa, 0, t->setup.sWrapMode, t->setup.tWrapMode );
    savage4_set_filter_mode( imesa, 0, t->setup.minFilter, t->setup.magFilter );

    if((ctx->Texture.Unit[0].LodBias !=0.0F) ||
       (imesa->regs.s4.texCtrl[0].ni.dBias != 0))
    {
	int bias = (int)(ctx->Texture.Unit[0].LodBias * 32.0) +
	    SAVAGE4_LOD_OFFSET;
	if (bias < -256)
	    bias = -256;
	else if (bias > 255)
	    bias = 255;
	imesa->regs.s4.texCtrl[0].ni.dBias = bias & 0x1ff;
    }

    image = tObj->Image[0][tObj->BaseLevel];
    imesa->regs.s4.texDescr.ni.tex0En = GL_TRUE;
    imesa->regs.s4.texDescr.ni.tex0Width  = image->WidthLog2;
    imesa->regs.s4.texDescr.ni.tex0Height = image->HeightLog2;
    imesa->regs.s4.texDescr.ni.tex0Fmt = t->hwFormat;
    imesa->regs.s4.texCtrl[0].ni.dMax = t->base.lastLevel - t->base.firstLevel;

    if (imesa->regs.s4.texDescr.ni.tex1En)
        imesa->regs.s4.texDescr.ni.texBLoopEn = GL_TRUE;

    imesa->regs.s4.texAddr[0].ui = (u_int32_t) t->setup.physAddr | 0x2;
    if(t->base.heap->heapId == SAVAGE_AGP_HEAP)
	imesa->regs.s4.texAddr[0].ui |= 0x1;
    
    return;
}
static void savageUpdateTex1State_s4( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   struct gl_texture_object	*tObj;
   struct gl_texture_image *image;
   savageTexObjPtr t;
   GLuint format;

   /* disable */
   if(imesa->bTexEn1)
   {
       imesa->bTexEn1 = GL_FALSE;
       return;
   }

   imesa->regs.s4.texDescr.ni.tex1En = GL_FALSE;
   imesa->regs.s4.texBlendCtrl[1].ui = TBC_NoTexMap1;
   imesa->regs.s4.texCtrl[1].ui = 0x20f040;
   imesa->regs.s4.texDescr.ni.texBLoopEn = GL_FALSE;
   if (ctx->Texture.Unit[1]._ReallyEnabled == 0)
      return;

   tObj = ctx->Texture.Unit[1]._Current;

   if ((ctx->Texture.Unit[1]._ReallyEnabled & ~(TEXTURE_1D_BIT|TEXTURE_2D_BIT))
       || tObj->Image[0][tObj->BaseLevel]->Border > 0) {
      /* 3D texturing enabled, or texture border - fallback */
      FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
      return;
   }

   /* Do 2D texture setup */

   t = tObj->DriverData;
   if (!t) {
      t = savageAllocTexObj( tObj );
      if (!t)
         return;
   }
    
   imesa->CurrentTexObj[1] = &t->base;

   t->base.bound |= 2;

   if (t->base.dirty_images[0] || t->dirtySubImages) {
       savageSetTexImages(imesa, tObj);
       savageUploadTexImages(imesa, t);
   }
   
   driUpdateTextureLRU( &t->base );

   format = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;

   switch (ctx->Texture.Unit[1].EnvMode) {
   case GL_REPLACE:
        imesa->regs.s4.texCtrl[1].ni.clrArg1Invert = GL_FALSE;
        switch (format)
        {
            case GL_LUMINANCE:
            case GL_RGB:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_Decal;
                break;

            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
            case GL_RGBA:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_Copy;
                break;

            case GL_ALPHA:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_CopyAlpha1;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &imesa->regs.s4.texBlendCtrl);
      break;
   case GL_MODULATE:
       imesa->regs.s4.texCtrl[1].ni.clrArg1Invert = GL_FALSE;
       imesa->regs.s4.texBlendCtrl[1].ui = TBC_ModulAlpha1;
       __HWEnvCombineSingleUnitScale(imesa, 0, 1, &imesa->regs.s4.texBlendCtrl);
       break;

    case GL_ADD:
        imesa->regs.s4.texCtrl[1].ni.clrArg1Invert = GL_FALSE;
        switch (format)
        {
            case GL_ALPHA:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_ModulAlpha1;
		break;

            case GL_LUMINANCE:
            case GL_RGB:
		imesa->regs.s4.texBlendCtrl[1].ui = TBC_Add1;
		break;

            case GL_LUMINANCE_ALPHA:
            case GL_RGBA:
		imesa->regs.s4.texBlendCtrl[1].ui = TBC_Add1;
		break;

            case GL_INTENSITY:
		imesa->regs.s4.texBlendCtrl[1].ui = TBC_AddAlpha1;
		break;
	}
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &imesa->regs.s4.texBlendCtrl);
        break;

#if GL_ARB_texture_env_combine
    case GL_COMBINE_ARB:
        __HWParseTexEnvCombine(imesa, 1, &texCtrl, &imesa->regs.s4.texBlendCtrl);
        break;
#endif

   case GL_DECAL:
        imesa->regs.s4.texCtrl[1].ni.clrArg1Invert = GL_FALSE;

        switch (format)
        {
            case GL_LUMINANCE:
            case GL_RGB:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_Decal1;
                break;
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
            case GL_RGBA:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_DecalAlpha1;
                break;

                /*
                // GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_INTENSITY
                // are undefined with GL_DECAL
                */
            case GL_ALPHA:
                imesa->regs.s4.texBlendCtrl[1].ui = TBC_CopyAlpha1;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &imesa->regs.s4.texBlendCtrl);
        break;

   case GL_BLEND:
        if (format == GL_LUMINANCE)
        {
            /*
            // This is a hack for GLQuake, invert.
            */
            imesa->regs.s4.texCtrl[1].ni.clrArg1Invert = GL_TRUE;
            imesa->regs.s4.texBlendCtrl[1].ui = 0;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &imesa->regs.s4.texBlendCtrl);
      break;

   default:
      fprintf(stderr, "unknown tex 1 env mode\n");
      exit(1);
      break;			
   }

    savage4_set_wrap_mode( imesa, 1, t->setup.sWrapMode, t->setup.tWrapMode );
    savage4_set_filter_mode( imesa, 1, t->setup.minFilter, t->setup.magFilter );

    if((ctx->Texture.Unit[1].LodBias !=0.0F) ||
       (imesa->regs.s4.texCtrl[1].ni.dBias != 0))
    {
	int bias = (int)(ctx->Texture.Unit[1].LodBias * 32.0) +
	    SAVAGE4_LOD_OFFSET;
	if (bias < -256)
	    bias = -256;
	else if (bias > 255)
	    bias = 255;
	imesa->regs.s4.texCtrl[1].ni.dBias = bias & 0x1ff;
    }

    image = tObj->Image[0][tObj->BaseLevel];
    imesa->regs.s4.texDescr.ni.tex1En = GL_TRUE;
    imesa->regs.s4.texDescr.ni.tex1Width  = image->WidthLog2;
    imesa->regs.s4.texDescr.ni.tex1Height = image->HeightLog2;
    imesa->regs.s4.texDescr.ni.tex1Fmt = t->hwFormat;
    imesa->regs.s4.texCtrl[1].ni.dMax = t->base.lastLevel - t->base.firstLevel;
    imesa->regs.s4.texDescr.ni.texBLoopEn = GL_TRUE;

    imesa->regs.s4.texAddr[1].ui = (u_int32_t) t->setup.physAddr | 2;
    if(t->base.heap->heapId == SAVAGE_AGP_HEAP)
	imesa->regs.s4.texAddr[1].ui |= 0x1;
}
static void savageUpdateTexState_s3d( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    struct gl_texture_object *tObj;
    struct gl_texture_image *image;
    savageTexObjPtr t;
    GLuint format;

    /* disable */
    imesa->regs.s3d.texCtrl.ui = 0;
    imesa->regs.s3d.texCtrl.ni.texEn = GL_FALSE;
    imesa->regs.s3d.texCtrl.ni.dBias = 0x08;
    imesa->regs.s3d.texCtrl.ni.texXprEn = GL_TRUE;
    if (ctx->Texture.Unit[0]._ReallyEnabled == 0)
	return;

    tObj = ctx->Texture.Unit[0]._Current;
    if ((ctx->Texture.Unit[0]._ReallyEnabled & ~(TEXTURE_1D_BIT|TEXTURE_2D_BIT))
	|| tObj->Image[0][tObj->BaseLevel]->Border > 0) {
	/* 3D texturing enabled, or texture border - fallback */
	FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
	return;
    }

    /* Do 2D texture setup */
    t = tObj->DriverData;
    if (!t) {
	t = savageAllocTexObj( tObj );
	if (!t)
	    return;
    }

    imesa->CurrentTexObj[0] = &t->base;
    t->base.bound |= 1;

    if (t->base.dirty_images[0] || t->dirtySubImages) {
	savageSetTexImages(imesa, tObj);
	savageUploadTexImages(imesa, t);
    }

    driUpdateTextureLRU( &t->base );

    format = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;

    /* FIXME: copied from utah-glx, probably needs some tuning */
    switch (ctx->Texture.Unit[0].EnvMode) {
    case GL_DECAL:
	imesa->regs.s3d.drawCtrl.ni.texBlendCtrl = SAVAGETBC_DECALALPHA_S3D;
	break;
    case GL_REPLACE:
	switch (format) {
	case GL_ALPHA: /* FIXME */
	    imesa->regs.s3d.drawCtrl.ni.texBlendCtrl = 1;
	    break;
	case GL_LUMINANCE_ALPHA:
	case GL_RGBA:
	    imesa->regs.s3d.drawCtrl.ni.texBlendCtrl = 4;
	    break;
	case GL_RGB:
	case GL_LUMINANCE:
	    imesa->regs.s3d.drawCtrl.ni.texBlendCtrl = SAVAGETBC_DECAL_S3D;
	    break;
	case GL_INTENSITY:
	    imesa->regs.s3d.drawCtrl.ni.texBlendCtrl = SAVAGETBC_COPY_S3D;
	}
	break;
    case GL_BLEND: /* hardware can't do GL_BLEND */
	FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
	return;
    case GL_MODULATE:
	imesa->regs.s3d.drawCtrl.ni.texBlendCtrl = SAVAGETBC_MODULATEALPHA_S3D;
	break;
    default:
	fprintf(stderr, "unknown tex env mode\n");
	/*exit(1);*/
	break;			
    }

    /* The Savage3D can't handle different wrapping modes in s and t.
     * If they are not the same, fall back to software. */
    if (t->setup.sWrapMode != t->setup.tWrapMode) {
	FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
	return;
    }
    imesa->regs.s3d.texCtrl.ni.uWrapEn = 0;
    imesa->regs.s3d.texCtrl.ni.vWrapEn = 0;
    imesa->regs.s3d.texCtrl.ni.wrapMode =
	(t->setup.sWrapMode == GL_REPEAT) ? TAM_Wrap : TAM_Clamp;

    switch (t->setup.minFilter) {
    case GL_NEAREST:
	imesa->regs.s3d.texCtrl.ni.filterMode    = TFM_Point;
	imesa->regs.s3d.texCtrl.ni.mipmapDisable = GL_TRUE;
	break;

    case GL_LINEAR:
	imesa->regs.s3d.texCtrl.ni.filterMode    = TFM_Bilin;
	imesa->regs.s3d.texCtrl.ni.mipmapDisable = GL_TRUE;
	break;

    case GL_NEAREST_MIPMAP_NEAREST:
	imesa->regs.s3d.texCtrl.ni.filterMode    = TFM_Point;
	imesa->regs.s3d.texCtrl.ni.mipmapDisable = GL_FALSE;
	break;

    case GL_LINEAR_MIPMAP_NEAREST:
	imesa->regs.s3d.texCtrl.ni.filterMode    = TFM_Bilin;
	imesa->regs.s3d.texCtrl.ni.mipmapDisable = GL_FALSE;
	break;

    case GL_NEAREST_MIPMAP_LINEAR:
    case GL_LINEAR_MIPMAP_LINEAR:
	imesa->regs.s3d.texCtrl.ni.filterMode    = TFM_Trilin;
	imesa->regs.s3d.texCtrl.ni.mipmapDisable = GL_FALSE;
	break;
    }

    /* There is no way to specify a maximum mipmap level. We may have to
       disable mipmapping completely. */
    /*
    if (t->max_level < t->image[0].image->WidthLog2 ||
	t->max_level < t->image[0].image->HeightLog2) {
	texCtrl.ni.mipmapEnable = GL_TRUE;
	if (texCtrl.ni.filterMode == TFM_Trilin)
	    texCtrl.ni.filterMode = TFM_Bilin;
	texCtrl.ni.filterMode = TFM_Point;
    }
    */

    if((ctx->Texture.Unit[0].LodBias !=0.0F) ||
       (imesa->regs.s3d.texCtrl.ni.dBias != 0))
    {
	int bias = (int)(ctx->Texture.Unit[0].LodBias * 16.0);
	if (bias < -256)
	    bias = -256;
	else if (bias > 255)
	    bias = 255;
	imesa->regs.s3d.texCtrl.ni.dBias = bias & 0x1ff;
    }

    image = tObj->Image[0][tObj->BaseLevel];
    imesa->regs.s3d.texCtrl.ni.texEn = GL_TRUE;
    imesa->regs.s3d.texDescr.ni.texWidth  = image->WidthLog2;
    imesa->regs.s3d.texDescr.ni.texHeight = image->HeightLog2;
    assert (t->hwFormat <= 7);
    imesa->regs.s3d.texDescr.ni.texFmt = t->hwFormat;

    imesa->regs.s3d.texAddr.ui = (u_int32_t) t->setup.physAddr | 2;
    if(t->base.heap->heapId == SAVAGE_AGP_HEAP)
	imesa->regs.s3d.texAddr.ui |= 0x1;
}


static void savageTimestampTextures( savageContextPtr imesa )
{
   /* Timestamp current texture objects for texture heap aging.
    * Only useful with long-lived 32-bit event tags available
    * with Savage DRM 2.3.x or later. */
   if ((imesa->CurrentTexObj[0] || imesa->CurrentTexObj[1]) &&
       imesa->savageScreen->driScrnPriv->drmMinor >= 3) {
       unsigned int e;
       FLUSH_BATCH(imesa);
       e = savageEmitEvent(imesa, SAVAGE_WAIT_3D);
       if (imesa->CurrentTexObj[0])
	   imesa->CurrentTexObj[0]->timestamp = e;
       if (imesa->CurrentTexObj[1])
	   imesa->CurrentTexObj[1]->timestamp = e;
   }
}


static void savageUpdateTextureState_s4( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);

   /* When a texture is about to change or be disabled, timestamp the
    * old texture(s). We'll have to wait for this time stamp before
    * uploading anything to the same texture heap.
    */
   if ((imesa->CurrentTexObj[0] && ctx->Texture.Unit[0]._ReallyEnabled &&
	ctx->Texture.Unit[0]._Current->DriverData != imesa->CurrentTexObj[0]) ||
       (imesa->CurrentTexObj[1] && ctx->Texture.Unit[1]._ReallyEnabled &&
	ctx->Texture.Unit[1]._Current->DriverData != imesa->CurrentTexObj[1]) ||
       (imesa->CurrentTexObj[0] && !ctx->Texture.Unit[0]._ReallyEnabled) ||
       (imesa->CurrentTexObj[1] && !ctx->Texture.Unit[1]._ReallyEnabled))
       savageTimestampTextures(imesa);

   if (imesa->CurrentTexObj[0]) imesa->CurrentTexObj[0]->bound &= ~1;
   if (imesa->CurrentTexObj[1]) imesa->CurrentTexObj[1]->bound &= ~2;
   imesa->CurrentTexObj[0] = 0;
   imesa->CurrentTexObj[1] = 0;   
   savageUpdateTex0State_s4( ctx );
   savageUpdateTex1State_s4( ctx );
   imesa->dirty |= (SAVAGE_UPLOAD_TEX0 | 
		    SAVAGE_UPLOAD_TEX1);
}
static void savageUpdateTextureState_s3d( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);

   /* When a texture is about to change or be disabled, timestamp the
    * old texture(s). We'll have to wait for this time stamp before
    * uploading anything to the same texture heap.
    */
    if ((imesa->CurrentTexObj[0] && ctx->Texture.Unit[0]._ReallyEnabled &&
	 ctx->Texture.Unit[0]._Current->DriverData != imesa->CurrentTexObj[0]) ||
	(imesa->CurrentTexObj[0] && !ctx->Texture.Unit[0]._ReallyEnabled))
	savageTimestampTextures(imesa);

    if (imesa->CurrentTexObj[0]) imesa->CurrentTexObj[0]->bound &= ~1;
    imesa->CurrentTexObj[0] = 0;
    savageUpdateTexState_s3d( ctx );
    imesa->dirty |= (SAVAGE_UPLOAD_TEX0);
}
void savageUpdateTextureState( GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_FALSE);
    FALLBACK(ctx, SAVAGE_FALLBACK_PROJ_TEXTURE, GL_FALSE);
    if (imesa->savageScreen->chipset >= S3_SAVAGE4)
	savageUpdateTextureState_s4 (ctx);
    else
	savageUpdateTextureState_s3d (ctx);
}



/*****************************************
 * DRIVER functions
 *****************************************/

static void savageTexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

   if (pname == GL_TEXTURE_ENV_MODE) {

      imesa->new_state |= SAVAGE_NEW_TEXTURE;

   } else if (pname == GL_TEXTURE_ENV_COLOR) {

      struct gl_texture_unit *texUnit = 
	 &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      const GLfloat *fc = texUnit->EnvColor;
      GLuint r, g, b, a;
      CLAMPED_FLOAT_TO_UBYTE(r, fc[0]);
      CLAMPED_FLOAT_TO_UBYTE(g, fc[1]);
      CLAMPED_FLOAT_TO_UBYTE(b, fc[2]);
      CLAMPED_FLOAT_TO_UBYTE(a, fc[3]);

      imesa->texEnvColor = ((a << 24) | (r << 16) | 
			    (g <<  8) | (b <<  0));
    

   } 
}

/* Update the heap's time stamp, so the new image is not uploaded
 * while the old one is still in use. If the texture that is going to
 * be changed is currently bound, we need to timestamp the texture
 * first. */
static void savageTexImageChanged (savageTexObjPtr t) {
    if (t->base.heap) {
	if (t->base.bound)
	    savageTimestampTextures(
		(savageContextPtr)t->base.heap->driverContext);
	if (t->base.timestamp > t->base.heap->timestamp)
	    t->base.heap->timestamp = t->base.timestamp;
    }
}

static void savageTexImage1D( GLcontext *ctx, GLenum target, GLint level,
			      GLint internalFormat,
			      GLint width, GLint border,
			      GLenum format, GLenum type, const GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage )
{
   savageTexObjPtr t = (savageTexObjPtr) texObj->DriverData;
   if (t) {
      savageTexImageChanged (t);
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
         return;
      }
   }
   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, packing, texObj, texImage );
   t->base.dirty_images[0] |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageTexSubImage1D( GLcontext *ctx, 
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
   savageTexObjPtr t = (savageTexObjPtr) texObj->DriverData;
   assert( t ); /* this _should_ be true */
   if (t) {
      savageTexImageChanged (t);
      savageMarkDirtyTiles(t, level, texImage->Width2, 1,
			   xoffset, 0, width, 1);
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
         return;
      }
      t->base.dirty_images[0] |= (1 << level);
   }
   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width, 
			     format, type, pixels, packing, texObj,
			     texImage);
   t->dirtySubImages |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			      GLint internalFormat,
			      GLint width, GLint height, GLint border,
			      GLenum format, GLenum type, const GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage )
{
   savageTexObjPtr t = (savageTexObjPtr) texObj->DriverData;
   if (t) {
      savageTexImageChanged (t);
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
   t->base.dirty_images[0] |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageTexSubImage2D( GLcontext *ctx, 
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
   savageTexObjPtr t = (savageTexObjPtr) texObj->DriverData;
   assert( t ); /* this _should_ be true */
   if (t) {
      savageTexImageChanged (t);
      savageMarkDirtyTiles(t, level, texImage->Width2, texImage->Height2,
			   xoffset, yoffset, width, height);
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }
      t->base.dirty_images[0] |= (1 << level);
   }
   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);
   t->dirtySubImages |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void
savageCompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLsizei imageSize, const GLvoid *data,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   savageTexObjPtr t = (savageTexObjPtr) texObj->DriverData;
   if (t) {
      savageTexImageChanged (t);
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
         return;
      }
   }
   _mesa_store_compressed_teximage2d( ctx, target, level, internalFormat,
				      width, height, border, imageSize,
				      data, texObj, texImage );
   t->base.dirty_images[0] |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void
savageCompressedTexSubImage2D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset, GLint yoffset,
			       GLsizei width, GLsizei height,
			       GLenum format, GLsizei imageSize,
			       const GLvoid *data,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
   savageTexObjPtr t = (savageTexObjPtr) texObj->DriverData;
   assert( t ); /* this _should_ be true */
   if (t) {
      savageTexImageChanged (t);
      savageMarkDirtyTiles(t, level, texImage->Width2, texImage->Height2,
			   xoffset, yoffset, width, height);
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }
      t->base.dirty_images[0] |= (1 << level);
   }
   _mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset, yoffset,
					width, height, format, imageSize,
					data, texObj, texImage);
   t->dirtySubImages |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageTexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
   savageTexObjPtr t = (savageTexObjPtr) tObj->DriverData;
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

   if (!t || (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D))
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      savageSetTexFilter(t,tObj->MinFilter,tObj->MagFilter);
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      savageSetTexWrapping(t,tObj->WrapS,tObj->WrapT);
      break;
  
   case GL_TEXTURE_BORDER_COLOR:
      savageSetTexBorderColor(t,tObj->_BorderChan);
      break;

   default:
      return;
   }

   imesa->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageBindTexture( GLcontext *ctx, GLenum target,
			       struct gl_texture_object *tObj )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   
   assert( (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D) ||
	   (tObj->DriverData != NULL) );

   imesa->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   driTextureObject *t = (driTextureObject *)tObj->DriverData;
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

   if (t) {
      if (t->bound)
	 savageTimestampTextures(imesa);

      driDestroyTextureObject(t);
   }
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}


static struct gl_texture_object *
savageNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
    struct gl_texture_object *obj;
    obj = _mesa_new_texture_object(ctx, name, target);
    savageAllocTexObj( obj );

    return obj;
}

void savageDDInitTextureFuncs( struct dd_function_table *functions )
{
   functions->TexEnv = savageTexEnv;
   functions->ChooseTextureFormat = savageChooseTextureFormat;
   functions->TexImage1D = savageTexImage1D;
   functions->TexSubImage1D = savageTexSubImage1D;
   functions->TexImage2D = savageTexImage2D;
   functions->TexSubImage2D = savageTexSubImage2D;
   functions->CompressedTexImage2D = savageCompressedTexImage2D;
   functions->CompressedTexSubImage2D = savageCompressedTexSubImage2D;
   functions->BindTexture = savageBindTexture;
   functions->NewTextureObject = savageNewTextureObject;
   functions->DeleteTexture = savageDeleteTexture;
   functions->IsTextureResident = driIsTextureResident;
   functions->TexParameter = savageTexParameter;

   /* Texel fetching with our custom texture formats works just like
    * the standard argb formats. */
   _savage_texformat_a1114444.FetchTexel1D = _mesa_texformat_argb4444.FetchTexel1D;
   _savage_texformat_a1114444.FetchTexel2D = _mesa_texformat_argb4444.FetchTexel2D;
   _savage_texformat_a1114444.FetchTexel3D = _mesa_texformat_argb4444.FetchTexel3D;
   _savage_texformat_a1114444.FetchTexel1Df= _mesa_texformat_argb4444.FetchTexel1Df;
   _savage_texformat_a1114444.FetchTexel2Df= _mesa_texformat_argb4444.FetchTexel2Df;
   _savage_texformat_a1114444.FetchTexel3Df= _mesa_texformat_argb4444.FetchTexel3Df;

   _savage_texformat_a1118888.FetchTexel1D = _mesa_texformat_argb8888.FetchTexel1D;
   _savage_texformat_a1118888.FetchTexel2D = _mesa_texformat_argb8888.FetchTexel2D;
   _savage_texformat_a1118888.FetchTexel3D = _mesa_texformat_argb8888.FetchTexel3D;
   _savage_texformat_a1118888.FetchTexel1Df= _mesa_texformat_argb8888.FetchTexel1Df;
   _savage_texformat_a1118888.FetchTexel2Df= _mesa_texformat_argb8888.FetchTexel2Df;
   _savage_texformat_a1118888.FetchTexel3Df= _mesa_texformat_argb8888.FetchTexel3Df;
}
