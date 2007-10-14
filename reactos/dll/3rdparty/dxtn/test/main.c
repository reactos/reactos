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

#include "tga.h"
#include "util.h"
#include "../types.h"
#include "../internal.h"
#include "../fxt1.h"
#include "../dxtn.h"


#if VERBOSE
int cc_chroma = 0;
int cc_alpha = 0;
int cc_high = 0;
int cc_mixed = 0;
#endif


typedef int (*encoder) (int width, int height, int comps,
			const void *source, int srcRowStride,
			void *dest, int destRowStride);
typedef void (*decoder) (const void *texture, int stride,
			 int i, int j, unsigned char *rgba);


static struct {
    const char *name;
    int type;
    encoder enc;
    decoder dec;
    int wround, hround;
} *q = NULL, tc[] = {
    { "fxt1rgba", GL_COMPRESSED_RGBA_FXT1_3DFX,     fxt1_encode,      fxt1_decode_1,      7, 3 },
    { "fxt1rgb",  GL_COMPRESSED_RGB_FXT1_3DFX,      fxt1_encode,      fxt1_decode_1,      7, 3 },
    { "dxt1rgb",  GL_COMPRESSED_RGB_S3TC_DXT1_EXT,  dxt1_rgb_encode,  dxt1_rgb_decode_1,  3, 3 },
    { "dxt1rgba", GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, dxt1_rgba_encode, dxt1_rgba_decode_1, 3, 3 },
    { "dxt3",     GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, dxt3_rgba_encode, dxt3_rgba_decode_1, 3, 3 },
    { "dxt5",     GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, dxt5_rgba_encode, dxt5_rgba_decode_1, 3, 3 },
    { NULL,       -1,                               NULL,             NULL,               0, 0 }
};


int
main (int argc, char **argv)
{
    const char *myself = argv[0];
    const char *inf = NULL, *outf = NULL, *cmpf = NULL, *a0 = NULL, *a1 = NULL;

    int i, j;
    int width, height;
    void *input, *output;
    unsigned char *rgba;
#if VERBOSE
    t_type t0;
#endif

    /* user options */
    while (--argc) {
	char *p = *++argv;
	if (!strcmp(p, "-h") || !strcmp(p, "--help")) {
	    fprintf(stderr, "usage: %s [type] infile [-o outfile] [-k cfile] [-a0 inalpha] [-a1 outalpha]\n", myself);
	    fprintf(stderr, "    infile must be a 24/32bpp TGA\n");
	    fprintf(stderr, "    outfile will be a 32bpp TGA\n");
	    fprintf(stderr, "    cfile will be the compressed file\n");
	    fprintf(stderr, "    inalpha will be original alpha 32bpp TGA\n");
	    fprintf(stderr, "    outalpha will be output alpha 32bpp TGA\n");
	    fprintf(stderr, "    type can be one of the following:\n");
	    for (i = 0; tc[i].name != NULL; i++) {
		fprintf(stderr, "        -%s%s\n", tc[i].name, (i == 0) ? " (default)" : "");
	    }
	    return 0;
	} /*else*/ if (!strcmp(p, "-o")) {
	    if (argc > 1) {
		argc--;
		outf = *++argv;
		continue;
	    } else {
		fprintf(stderr, "%s: argument to `%s' is missing\n", myself, p);
		return -1;
	    }
	} else if (!strcmp(p, "-k")) {
	    if (argc > 1) {
		argc--;
		cmpf = *++argv;
		continue;
	    } else {
		fprintf(stderr, "%s: argument to `%s' is missing\n", myself, p);
		return -1;
	    }
	} else if (!strcmp(p, "-a0")) {
	    if (argc > 1) {
		argc--;
		a0 = *++argv;
		continue;
	    } else {
		fprintf(stderr, "%s: argument to `%s' is missing\n", myself, p);
		return -1;
	    }
	} else if (!strcmp(p, "-a1")) {
	    if (argc > 1) {
		argc--;
		a1 = *++argv;
		continue;
	    } else {
		fprintf(stderr, "%s: argument to `%s' is missing\n", myself, p);
		return -1;
	    }
	} else if (*p == '-') {
	    for (q = tc; q->name != NULL; q++) {
		if (!strcmp(p + 1, q->name)) {
		    break;
		}
	    }
	    if (q->name != NULL) {
		continue;
	    }
	    fprintf(stderr, "%s: bad option `%s'\n", myself, p);
	    return -1;
	} else if (inf == NULL) {
	    inf = p;
	    continue;
	}
	fprintf(stderr, "%s: too many input files\n", myself);
	return -1;
    }
    if (inf == NULL) {
	fprintf(stderr, "%s: no input files\n", myself);
	return -1;
    }
    if (outf == NULL) {
	outf = "aout.tga";
    }
    if ((q == NULL) || (q->name == NULL)) {
	q = tc;
    }

    /* get input data */
#if 1
    if (tga_read_32(inf, &width, &height, &input)) {
	fprintf(stderr, "%s: cannot read `%s'\n", myself, inf);
	return -1;
    }
#elif 0
    {
	static char pattern[8 * 32 + 1] = { "\
                                \
    MMM    EEEE   SSS    AAA    \
   M M M  E      S   S  A   A   \
   M M M  EEEE    SS    A   A   \
   M M M  E         SS  AAAAA   \
   M   M  E      S   S  A   A   \
   M   M   EEEE   SSS   A   A   \
                                " };

	unsigned char (*texture)[8 * 32][4];
	width = 32;
	height = 8;
	input = malloc(width * height * 4);
	texture = (unsigned char (*)[8 * 32][4])input;
	for (i = 0; i < sizeof(pattern) - 1; i++) {
	    switch (pattern[i]) {
		default:
		case ' ':
		    (*texture)[i][0] = 255;
		    (*texture)[i][1] = 255;
		    (*texture)[i][2] = 255;
		    (*texture)[i][3] = 64;
		    break;
		case 'M':
		    (*texture)[i][0] = 255;
		    (*texture)[i][1] = 0;
		    (*texture)[i][2] = 0;
		    (*texture)[i][3] = 255;
		    break;
		case 'E':
		    (*texture)[i][0] = 0;
		    (*texture)[i][1] = 255;
		    (*texture)[i][2] = 0;
		    (*texture)[i][3] = 255;
		    break;
		case 'S':
		    (*texture)[i][0] = 0;
		    (*texture)[i][1] = 0;
		    (*texture)[i][2] = 255;
		    (*texture)[i][3] = 255;
		    break;
		case 'A':
		    (*texture)[i][0] = 255;
		    (*texture)[i][1] = 255;
		    (*texture)[i][2] = 0;
		    (*texture)[i][3] = 255;
		    break;
	    }
	}
    }
#else
    {
	unsigned char (*texture)[4 * 8][4];
	width = 8;
	height = 4;
	input = malloc(width * height * 4);
	texture = (unsigned char (*)[4 * 8][4])input;
	for (i = 0; i < 4 * 8; i++) {
	    (*texture)[i][0] =
	    (*texture)[i][1] =
	    (*texture)[i][2] = 255 * i / 31;
	    (*texture)[i][3] = 255;
	}
    }
#endif

    /* make alpha tga (input values) */
    if (a0 != NULL) {
	unsigned long *alpha0 = malloc(width * height * 4);
	if (alpha0) {
	    unsigned long *ap = alpha0;
	    for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
		    unsigned char alp = ((unsigned char *)input)[(j * width + i) * 4 + 3];
		    *ap++ = alp | (alp << 8) | (alp << 16) | (alp << 24);
		}
	    }
	    if (tga_write(a0, width, height, alpha0, 32) != 0) {
		fprintf(stderr, "%s: cannot write `%s'\n", myself, a0);
	    }
	    free(alpha0);
	}
    }

    /* allocate compressed output storage */
    output = malloc(tc_size(width, height, q->type));
    if (output == NULL) {
	free(input);
	fprintf(stderr, "%s: out of memory\n", myself);
	return -1;
    }

    /* encode */
#if VERBOSE
    T_START(t0);
#endif
    q->enc(width, height, 4, input, width * 4, output, tc_stride(q->type, width));
#if VERBOSE
    T_STOP(t0);
    fprintf(stderr, "ENC(%s): %lu ticks\n", q->name, T_DELTA(t0));
#endif

    /* free raw input data, make encoded data as input */
    free(input);
    input = output;

    /* allocate uncompressed output storage */
    width = (width + q->wround) & ~q->wround;
    height = (height + q->hround) & ~q->hround;
    output = malloc(width * height * 4);
    if (output == NULL) {
	free(input);
	fprintf(stderr, "%s: out of memory\n", myself);
	return -1;
    }

    /* decode */
    rgba = output;
#if VERBOSE
    T_START(t0);
#endif
    for (j = 0; j < height; j++) {
	for (i = 0; i < width; i++) {
	    q->dec(input, width, i, j, rgba);
	    rgba += 4;
	}
    }
#if VERBOSE
    T_STOP(t0);
#endif

    /* write encoded block */
    if (cmpf != NULL) {
	FILE *eff = fopen(cmpf, "wb");
	if (eff) {
	    if (!fwrite(input, tc_size(width, height, q->type), 1, eff)) {
		fprintf(stderr, "%s: cannot write compressed data\n", myself);
	    }
	    fclose(eff);
	} else {
	    fprintf(stderr, "%s: cannot create `%s'\n", cmpf, myself);
	}
    }

    /* free encoded block */
    free(input);

    /* write decoded block */
    if (tga_write(outf, width, height, output, 32) != 0) {
	fprintf(stderr, "%s: cannot write `%s'\n", myself, outf);
	return -1;
    }

    /* make alpha tga (output values) */
    if (a1 != NULL) {
	unsigned long *alpha1 = malloc(width * height * 4);
	if (alpha1) {
	    unsigned long *ap = alpha1;
	    for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
		    unsigned char alp = ((unsigned char *)output)[(j * width + i) * 4 + 3];
		    *ap++ = alp | (alp << 8) | (alp << 16) | (alp << 24);
		}
	    }
	    if (tga_write(a1, width, height, alpha1, 32) != 0) {
		fprintf(stderr, "%s: cannot write `%s'\n", myself, a1);
	    }
	    free(alpha1);
	}
    }

    /* free decoded block */
    free(output);

    /* final stats */
#if VERBOSE
    fprintf(stderr, "CHROMA = %d\n", cc_chroma);
    fprintf(stderr, "ALPHA  = %d\n", cc_alpha);
    fprintf(stderr, "HIGH   = %d\n", cc_high);
    fprintf(stderr, "MIXED  = %d\n", cc_mixed);
    fprintf(stderr, "DEC(%s): %lu ticks\n", q->name, T_DELTA(t0));
#endif

    return 0;
}
