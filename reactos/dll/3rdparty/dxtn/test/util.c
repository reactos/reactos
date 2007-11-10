/*
 * Texture compression
 * Version:  1.1
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>

#include "util.h"


#define GL_RGB_S3TC                       0x83A0
#define GL_RGB4_S3TC                      0x83A1
#define GL_RGBA_S3TC                      0x83A2
#define GL_RGBA4_S3TC                     0x83A3


int
tc_stride (int format, unsigned int width)
{
    int stride;

    switch (format) {
	case GL_COMPRESSED_RGB_FXT1_3DFX:
	case GL_COMPRESSED_RGBA_FXT1_3DFX:
	    stride = ((width + 7) / 8) * 16;	/* 16 bytes per 8x4 tile */
	    break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	    stride = ((width + 3) / 4) * 8;	/* 8 bytes per 4x4 tile */
	    break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	    stride = ((width + 3) / 4) * 16;	/* 16 bytes per 4x4 tile */
	    break;
	default:
	    return 0;
    }

    return stride;
}


unsigned int
tc_size (unsigned int width, unsigned int height, int format)
{
    unsigned int size;

    switch (format) {
	case GL_COMPRESSED_RGB_FXT1_3DFX:
	case GL_COMPRESSED_RGBA_FXT1_3DFX:
	    /* round up width to next multiple of 8, height to next multiple of 4 */
	    width = (width + 7) & ~7;
	    height = (height + 3) & ~3;
	    /* 16 bytes per 8x4 tile of RGB[A] texels */
	    size = width * height / 2;
	    /* Textures smaller than 8x4 will effectively be made into 8x4 and
	     * take 16 bytes.
	     */
	    if (size < 16)
		size = 16;
	    return size;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	    /* round up width, height to next multiple of 4 */
	    width = (width + 3) & ~3;
	    height = (height + 3) & ~3;
	    /* 8 bytes per 4x4 tile of RGB[A] texels */
	    size = width * height / 2;
	    /* Textures smaller than 4x4 will effectively be made into 4x4 and
	     * take 8 bytes.
	     */
	    if (size < 8)
		size = 8;
	    return size;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	    /* round up width, height to next multiple of 4 */
	    width = (width + 3) & ~3;
	    height = (height + 3) & ~3;
	    /* 16 bytes per 4x4 tile of RGBA texels */
	    size = width * height;	/* simple! */
	    /* Textures smaller than 4x4 will effectively be made into 4x4 and
	     * take 16 bytes.
	     */
	    if (size < 16)
		size = 16;
	    return size;
	default:
	    return 0;
    }
}


void *
txs_read_fxt1 (const char *filename, int *width, int *height)
{
    FILE *f;
    void *data;

    char cookie[5];
    float version;
    int format;
    int levels;
    unsigned int offset;

    int rv;

    f = fopen(filename, "rb");
    if (f == NULL) {
	fprintf(stderr, "txs_read_fxt1: cannot open `%s'\n", filename);
	return NULL;
    }

    if ((fscanf(f, "%4s %f %d %d %d %d %8x", cookie, &version,
					     &format, width, height, &levels,
					     &offset) != 7) ||
	strcmp(cookie, "TXSF") ||
	(version != 1.0) ||
	(format != 17) ||
	(*width & 7) ||
	(*height & 4) ||
	(levels != 1)) {
	fclose(f);
	fprintf(stderr, "txs_read_fxt1: bad TXS file %s\n", filename);
	return NULL;
    }

    rv = tc_size(*width, *height, GL_COMPRESSED_RGBA_FXT1_3DFX);
    data = malloc(rv);
    if (data == NULL) {
	fclose(f);
	fprintf(stderr, "txs_read_fxt1: out of memory\n");
	return NULL;
    }

    fseek(f, offset, SEEK_SET);
    rv = fread(data, rv, 1, f);
    fclose(f);

    if (!rv) {
	free(data);
	data = NULL;
    }

    return data;
}
