/*
 * TGA file handling
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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tga.h"


int
tga_read_32 (const char *filename, int *w, int *h, void **p)
{
    FILE *f;
    void *img;
    unsigned char header[18];
    unsigned int skip_cmap_size;
    unsigned int type, width, height, tga_bpp;
    unsigned int raw_image_size;
    unsigned char *raw_image;
    unsigned int i, j, k;

    if ((f = fopen(filename, "rb")) == NULL) {
	return TGA_ERR_OPEN;
    }

    if (!fread(header, 18, 1, f)) {
	fclose(f);
	return TGA_ERR_READ;
    }

    type = header[2];
    if (type != 2) {
	fclose(f);
	return TGA_ERR_FORMAT;
    }
    width = ((short *)header)[6];
    height = ((short *)header)[7];
    tga_bpp = header[16];

    if (header[1]) {
	skip_cmap_size = *(short *)&header[5] * header[7] >> 3;
    } else {
	skip_cmap_size = 0;
    }
    fseek(f, skip_cmap_size + header[0], SEEK_CUR);

    raw_image_size = width * height;
    switch (tga_bpp) {
	case 16:
	    raw_image_size *= 2;
	    break;
	case 24:
	    raw_image_size *= 3;
	    break;
	case 32:
	    raw_image_size *= 4;
	    break;
	default:
	    fclose(f);
	    return TGA_ERR_FORMAT;
    }

    if ((img = malloc(width * height * 4)) == NULL) {
	fclose(f);
	return TGA_ERR_MEM;
    }

    if ((raw_image = malloc(raw_image_size)) == NULL) {
	free(img);
	fclose(f);
	return TGA_ERR_MEM;
    }

    if (!fread(raw_image, raw_image_size, 1, f)) {
	free(raw_image);
	free(img);
	fclose(f);
	return TGA_ERR_READ;
    }

    k = 0;
    for (i = 0; i < height; i++) {
	unsigned long decoded;
	int l = (header[17] & 0x20) ? i : (height - i - 1);
	unsigned char *bmp_line = (unsigned char *)img + l * width * 4;
	for (j = 0; j < width; j++) {
	    switch (tga_bpp) {
		case 16:
		    assert(0);
		    k += 2;
		    break;
		case 24:
		    decoded = (*(unsigned long *)&raw_image[k]) | 0xff000000UL;
		    k += 3;
		    break;
		case 32:
		    decoded = *(unsigned long *)&raw_image[k];
		    k += 4;
		    break;
		default:
		    decoded = 0;
	    }
	    ((unsigned long *)bmp_line)[j] = decoded;
	}
    }

    free(raw_image);
    fclose(f);

    *p = img;
    *w = width;
    *h = height;
    return TGA_OK;
}


int
tga_write (const char *filename, int width, int height, void *data, int bpp)
{
    int len;
    char header[18];
    FILE *f = fopen(filename, "wb");

    if (f == NULL) {
	return TGA_ERR_CREATE;
    }

    memset(header, 0, sizeof(header));
    header[2] = 2;
    ((unsigned short *)header)[6] = width;
    ((unsigned short *)header)[7] = height;
    header[16] = bpp;
    header[17] |= 0x20;

    len = width * height * ((header[16] + 7) / 8);

    if (!fwrite(header, sizeof(header), 1, f) || !fwrite(data, len, 1, f)) {
	return TGA_ERR_WRITE;
    }

    fclose(f);

    return TGA_OK;
}
