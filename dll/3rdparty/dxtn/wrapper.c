/*
 * Texture compression
 * Version:  1.0
 *
 * Copyright (C) 2004  Daniel Borca   All Rights Reserved.
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	
 */


#include <assert.h>

#include "types.h"
#include "internal.h"
#include "dxtn.h"


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3


TAPI void TAPIENTRY
fetch_2d_texel_rgb_dxt1 (int texImage_RowStride,
			 const byte *texImage_Data,
			 int i, int j,
			 byte *texel)
{
    dxt1_rgb_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}


TAPI void TAPIENTRY
fetch_2d_texel_rgba_dxt1 (int texImage_RowStride,
			  const byte *texImage_Data,
			  int i, int j,
			  byte *texel)
{
    dxt1_rgba_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}


TAPI void TAPIENTRY
fetch_2d_texel_rgba_dxt3 (int texImage_RowStride,
			  const byte *texImage_Data,
			  int i, int j,
			  byte *texel)
{
    dxt3_rgba_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}


TAPI void TAPIENTRY
fetch_2d_texel_rgba_dxt5 (int texImage_RowStride,
			  const byte *texImage_Data,
			  int i, int j,
			  byte *texel)
{
    dxt5_rgba_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}


TAPI void TAPIENTRY
tx_compress_dxtn (int srccomps, int width, int height,
		  const byte *source, int destformat, byte *dest,
		  int destRowStride)
{
    int srcRowStride = width * srccomps;
    int rv;

    switch (destformat) {
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	    rv = dxt1_rgb_encode(width, height, srccomps,
				 source, srcRowStride,
				 dest, destRowStride);
	    break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	    rv = dxt1_rgba_encode(width, height, srccomps,
				  source, srcRowStride,
				  dest, destRowStride);
	    break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	    rv = dxt3_rgba_encode(width, height, srccomps,
				  source, srcRowStride,
				  dest, destRowStride);
	    break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	    rv = dxt5_rgba_encode(width, height, srccomps,
				  source, srcRowStride,
				  dest, destRowStride);
	    break;
	default:
	    assert(0);
    }

    /*return rv;*/
}
