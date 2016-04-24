/*
 * DXTn codec
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


#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "internal.h"
#include "dxtn.h"


/***************************************************************************\
 * DXTn encoder
 *
 * The encoder was built by reversing the decoder,
 * and is vaguely based on FXT1 codec. Note that this code
 * is merely a proof of concept, since it is highly UNoptimized!
\***************************************************************************/


#define MAX_COMP 4 /* ever needed maximum number of components in texel */
#define MAX_VECT 4 /* ever needed maximum number of base vectors to find */
#define N_TEXELS 16 /* number of texels in a block (always 16) */
#define COLOR565(v) (word)((((v)[RCOMP] & 0xf8) << 8) | (((v)[GCOMP] & 0xfc) << 3) | ((v)[BCOMP] >> 3))


static const int dxtn_color_tlat[2][4] = {
    { 0, 2, 3, 1 },
    { 0, 2, 1, 3 }
};

static const int dxtn_alpha_tlat[2][8] = {
    { 0, 2, 3, 4, 5, 6, 7, 1 },
    { 0, 2, 3, 4, 5, 1, 6, 7 }
};


static void
dxt1_rgb_quantize (dword *cc, const byte *lines[], int comps)
{
    float b, iv[MAX_COMP];   /* interpolation vector */

    dword hi; /* high doubleword */
    int color0, color1;
    int n_vect;
    const int n_comp = 3;
    int black = 0;

    int minSum = 2000; /* big enough */
    int maxSum = -1; /* small enough */
    int minCol = 0; /* phoudoin: silent compiler! */
    int maxCol = 0; /* phoudoin: silent compiler! */

    byte input[N_TEXELS][MAX_COMP];
    int i, k, l;

    /* make the whole block opaque */
    /* we will NEVER reference ACOMP of any pixel */

    /* 4 texels each line */
    for (l = 0; l < 4; l++) {
	for (k = 0; k < 4; k++) {
	    for (i = 0; i < comps; i++) {
		input[k + l * 4][i] = *lines[l]++;
	    }
	}
    }

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    for (k = 0; k < N_TEXELS; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minCol = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxCol = k;
	}
	if (sum == 0) {
	    black = 1;
	}
    }

    color0 = COLOR565(input[minCol]);
    color1 = COLOR565(input[maxCol]);

    if (color0 == color1) {
	/* we'll use 3-vector */
	cc[0] = color0 | (color1 << 16);
	hi = black ? -1 : 0;
    } else {
	if (black && ((color0 == 0) || (color1 == 0))) {
	    /* we still can use 4-vector */
	    black = 0;
	}

	if (black ^ (color0 <= color1)) {
	    int aux;
	    aux = color0;
	    color0 = color1;
	    color1 = aux;
	    aux = minCol;
	    minCol = maxCol;
	    maxCol = aux;
	}
	n_vect = (color0 <= color1) ? 2 : 3;

	MAKEIVEC(n_vect, n_comp, iv, b, input[minCol], input[maxCol]);

	/* add in texels */
	cc[0] = color0 | (color1 << 16);
	hi = 0;
	for (k = N_TEXELS - 1; k >= 0; k--) {
	    int texel = 3;
	    int sum = 0;
	    if (black) {
		for (i = 0; i < n_comp; i++) {
		    sum += input[k][i];
		}
	    }
	    if (!black || sum) {
		/* interpolate color */
		CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
		texel = dxtn_color_tlat[black][texel];
	    }
	    /* add in texel */
	    hi <<= 2;
	    hi |= texel;
	}
    }
    cc[1] = hi;
}


static void
dxt1_rgba_quantize (dword *cc, const byte *lines[], int comps)
{
    float b, iv[MAX_COMP];	/* interpolation vector */

    dword hi;		/* high doubleword */
    int color0, color1;
    int n_vect;
    const int n_comp = 3;
    int transparent = 0;

    int minSum = 2000;		/* big enough */
    int maxSum = -1;		/* small enough */
    int minCol = 0;		/* phoudoin: silent compiler! */
    int maxCol = 0;		/* phoudoin: silent compiler! */

    byte input[N_TEXELS][MAX_COMP];
    int i, k, l;

    if (comps == 3) {
	/* make the whole block opaque */
	memset(input, -1, sizeof(input));
    }

    /* 4 texels each line */
    for (l = 0; l < 4; l++) {
	for (k = 0; k < 4; k++) {
	    for (i = 0; i < comps; i++) {
		input[k + l * 4][i] = *lines[l]++;
	    }
	}
    }

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    for (k = 0; k < N_TEXELS; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minCol = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxCol = k;
	}
	if (input[k][ACOMP] < 128) {
	    transparent = 1;
	}
    }

    color0 = COLOR565(input[minCol]);
    color1 = COLOR565(input[maxCol]);

    if (color0 == color1) {
	/* we'll use 3-vector */
	cc[0] = color0 | (color1 << 16);
	hi = transparent ? -1 : 0;
    } else {
	if (transparent ^ (color0 <= color1)) {
	    int aux;
	    aux = color0;
	    color0 = color1;
	    color1 = aux;
	    aux = minCol;
	    minCol = maxCol;
	    maxCol = aux;
	}
	n_vect = (color0 <= color1) ? 2 : 3;

	MAKEIVEC(n_vect, n_comp, iv, b, input[minCol], input[maxCol]);

	/* add in texels */
	cc[0] = color0 | (color1 << 16);
	hi = 0;
	for (k = N_TEXELS - 1; k >= 0; k--) {
	    int texel = 3;
	    if (input[k][ACOMP] >= 128) {
		/* interpolate color */
		CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
		texel = dxtn_color_tlat[transparent][texel];
	    }
	    /* add in texel */
	    hi <<= 2;
	    hi |= texel;
	}
    }
    cc[1] = hi;
}


static void
dxt3_rgba_quantize (dword *cc, const byte *lines[], int comps)
{
    float b, iv[MAX_COMP];	/* interpolation vector */

    dword lolo, lohi;	/* low quadword: lo dword, hi dword */
    dword hihi;		/* high quadword: high dword */
    int color0, color1;
    const int n_vect = 3;
    const int n_comp = 3;

    int minSum = 2000;		/* big enough */
    int maxSum = -1;		/* small enough */
    int minCol = 0;		/* phoudoin: silent compiler! */
    int maxCol = 0;		/* phoudoin: silent compiler! */

    byte input[N_TEXELS][MAX_COMP];
    int i, k, l;

    if (comps == 3) {
	/* make the whole block opaque */
	memset(input, -1, sizeof(input));
    }

    /* 4 texels each line */
    for (l = 0; l < 4; l++) {
	for (k = 0; k < 4; k++) {
	    for (i = 0; i < comps; i++) {
		input[k + l * 4][i] = *lines[l]++;
	    }
	}
    }

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    for (k = 0; k < N_TEXELS; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minCol = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxCol = k;
	}
    }

    /* add in alphas */
    lolo = lohi = 0;
    for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
	/* add in alpha */
	lohi <<= 4;
	lohi |= input[k][ACOMP] >> 4;
    }
    cc[1] = lohi;
    for (; k >= 0; k--) {
	/* add in alpha */
	lolo <<= 4;
	lolo |= input[k][ACOMP] >> 4;
    }
    cc[0] = lolo;

    color0 = COLOR565(input[minCol]);
    color1 = COLOR565(input[maxCol]);
    cc[2] = color0 | (color1 << 16);

    hihi = 0;
    if (color0 != color1) {
	MAKEIVEC(n_vect, n_comp, iv, b, input[minCol], input[maxCol]);

	/* add in texels */
	for (k = N_TEXELS - 1; k >= 0; k--) {
	    int texel;
	    /* interpolate color */
	    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
	    texel = dxtn_color_tlat[0][texel];
	    /* add in texel */
	    hihi <<= 2;
	    hihi |= texel;
	}
    }
    cc[3] = hihi;
}


static void
dxt5_rgba_quantize (dword *cc, const byte *lines[], int comps)
{
    float b, iv[MAX_COMP];	/* interpolation vector */

    qword lo;			/* low quadword */
    dword hihi;		/* high quadword: high dword */
    int color0, color1;
    const int n_vect = 3;
    const int n_comp = 3;

    int minSum = 2000;		/* big enough */
    int maxSum = -1;		/* small enough */
    int minCol = 0;		/* phoudoin: silent compiler! */
    int maxCol = 0;		/* phoudoin: silent compiler! */
    int alpha0 = 2000;		/* big enough */
    int alpha1 = -1;		/* small enough */
    int anyZero = 0, anyOne = 0;
    int a_vect;

    byte input[N_TEXELS][MAX_COMP];
    int i, k, l;

    if (comps == 3) {
	/* make the whole block opaque */
	memset(input, -1, sizeof(input));
    }

    /* 4 texels each line */
    for (l = 0; l < 4; l++) {
	for (k = 0; k < 4; k++) {
	    for (i = 0; i < comps; i++) {
		input[k + l * 4][i] = *lines[l]++;
	    }
	}
    }

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    for (k = 0; k < N_TEXELS; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minCol = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxCol = k;
	}
	if (alpha0 > input[k][ACOMP]) {
	    alpha0 = input[k][ACOMP];
	}
	if (alpha1 < input[k][ACOMP]) {
	    alpha1 = input[k][ACOMP];
	}
	if (input[k][ACOMP] == 0) {
	    anyZero = 1;
	}
	if (input[k][ACOMP] == 255) {
	    anyOne = 1;
	}
    }

    /* add in alphas */
    if (alpha0 == alpha1) {
	/* we'll use 6-vector */
	cc[0] = alpha0 | (alpha1 << 8);
	cc[1] = 0;
    } else {
	if (anyZero && ((alpha0 == 0) || (alpha1 == 0))) {
	    /* we still might use 8-vector */
	    anyZero = 0;
	}
	if (anyOne && ((alpha0 == 255) || (alpha1 == 255))) {
	    /* we still might use 8-vector */
	    anyOne = 0;
	}
	if ((anyZero | anyOne) ^ (alpha0 <= alpha1)) {
	    int aux;
	    aux = alpha0;
	    alpha0 = alpha1;
	    alpha1 = aux;
	}
	a_vect = (alpha0 <= alpha1) ? 5 : 7;

	/* compute interpolation vector */
	iv[ACOMP] = (float)a_vect / (alpha1 - alpha0);
	b = -iv[ACOMP] * alpha0 + 0.5F;

	/* add in alphas */
	Q_MOV32(lo, 0);
	for (k = N_TEXELS - 1; k >= 0; k--) {
	    int texel = -1;
	    if (anyZero | anyOne) {
		if (input[k][ACOMP] == 0) {
		    texel = 6;
		} else if (input[k][ACOMP] == 255) {
		    texel = 7;
		}
	    }
	    /* interpolate alpha */
	    if (texel == -1) {
		float dot = input[k][ACOMP] * iv[ACOMP];
		texel = (int)(dot + b);
#if SAFECDOT
		if (texel < 0) {
		    texel = 0;
		} else if (texel > a_vect) {
		    texel = a_vect;
		}
#endif
		texel = dxtn_alpha_tlat[anyZero | anyOne][texel];
	    }
	    /* add in texel */
	    Q_SHL(lo, 3);
	    Q_OR32(lo, texel);
	}
	Q_SHL(lo, 16);
	Q_OR32(lo, alpha0 | (alpha1 << 8));
	((qword *)cc)[0] = lo;
    }

    color0 = COLOR565(input[minCol]);
    color1 = COLOR565(input[maxCol]);
    cc[2] = color0 | (color1 << 16);

    hihi = 0;
    if (color0 != color1) {
	MAKEIVEC(n_vect, n_comp, iv, b, input[minCol], input[maxCol]);

	/* add in texels */
	for (k = N_TEXELS - 1; k >= 0; k--) {
	    int texel;
	    /* interpolate color */
	    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
	    texel = dxtn_color_tlat[0][texel];
	    /* add in texel */
	    hihi <<= 2;
	    hihi |= texel;
	}
    }
    cc[3] = hihi;
}


#define ENCODER(dxtn, n)						\
int TAPIENTRY								\
dxtn##_encode (int width, int height, int comps,			\
	       const void *source, int srcRowStride,			\
	       void *dest, int destRowStride)				\
{									\
    int x, y;								\
    const byte *data;							\
    dword *encoded = (dword *)dest;					\
    void *newSource = NULL;						\
									\
    /* Replicate image if width is not M4 or height is not M4 */	\
    if ((width & 3) | (height & 3)) {					\
	int newWidth = (width + 3) & ~3;				\
	int newHeight = (height + 3) & ~3;				\
	newSource = malloc(comps * newWidth * newHeight * sizeof(byte *));\
	_mesa_upscale_teximage2d(width, height, newWidth, newHeight,	\
                               comps, (const byte *)source,		\
			       srcRowStride, (byte *)newSource);	\
	source = newSource;						\
	width = newWidth;						\
	height = newHeight;						\
	srcRowStride = comps * newWidth;				\
    }									\
									\
    data = (const byte *)source;					\
    destRowStride = (destRowStride - width * n) / 4;			\
    for (y = 0; y < height; y += 4) {					\
	unsigned int offs = 0 + (y + 0) * srcRowStride;			\
	for (x = 0; x < width; x += 4) {				\
	    const byte *lines[4];					\
	    lines[0] = &data[offs];					\
	    lines[1] = lines[0] + srcRowStride;				\
	    lines[2] = lines[1] + srcRowStride;				\
	    lines[3] = lines[2] + srcRowStride;				\
	    offs += 4 * comps;						\
	    dxtn##_quantize(encoded, lines, comps);			\
	    /* 4x4 block */						\
	    encoded += n;						\
	}								\
	encoded += destRowStride;					\
    }									\
									\
    if (newSource != NULL) {						\
	free(newSource);						\
    }									\
									\
    return 0;								\
}

ENCODER(dxt1_rgb,  2)
ENCODER(dxt1_rgba, 2)
ENCODER(dxt3_rgba, 4)
ENCODER(dxt5_rgba, 4)


/***************************************************************************\
 * DXTn decoder
 *
 * The decoder is based on GL_EXT_texture_compression_s3tc
 * specification and serves as a concept for the encoder.
\***************************************************************************/


/* lookup table for scaling 4 bit colors up to 8 bits */
static const byte _rgb_scale_4[] = {
    0,   17,  34,  51,  68,  85,  102, 119,
    136, 153, 170, 187, 204, 221, 238, 255
};

/* lookup table for scaling 5 bit colors up to 8 bits */
static const byte _rgb_scale_5[] = {
    0,   8,   16,  25,  33,  41,  49,  58,
    66,  74,  82,  90,  99,  107, 115, 123,
    132, 140, 148, 156, 165, 173, 181, 189,
    197, 206, 214, 222, 230, 239, 247, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static const byte _rgb_scale_6[] = {
    0,   4,   8,   12,  16,  20,  24,  28,
    32,  36,  40,  45,  49,  53,  57,  61,
    65,  69,  73,  77,  81,  85,  89,  93,
    97,  101, 105, 109, 113, 117, 121, 125,
    130, 134, 138, 142, 146, 150, 154, 158,
    162, 166, 170, 174, 178, 182, 186, 190,
    194, 198, 202, 206, 210, 215, 219, 223,
    227, 231, 235, 239, 243, 247, 251, 255
};


#define CC_SEL(cc, which) (((dword *)(cc))[(which) / 32] >> ((which) & 31))
#define UP4(c) _rgb_scale_4[(c) & 15]
#define UP5(c) _rgb_scale_5[(c) & 31]
#define UP6(c) _rgb_scale_6[(c) & 63]
#define ZERO_4UBV(v) *((dword *)(v)) = 0


void TAPIENTRY
dxt1_rgb_decode_1 (const void *texture, int stride,
		   int i, int j, byte *rgba)
{
    const byte *src = (const byte *)texture
		       + ((j / 4) * ((stride + 3) / 4) + i / 4) * 8;
    const int code = (src[4 + (j & 3)] >> ((i & 3) * 2)) & 0x3;
    if (code == 0) {
	rgba[RCOMP] = UP5(CC_SEL(src, 11));
	rgba[GCOMP] = UP6(CC_SEL(src,  5));
	rgba[BCOMP] = UP5(CC_SEL(src,  0));
    } else if (code == 1) {
	rgba[RCOMP] = UP5(CC_SEL(src, 27));
	rgba[GCOMP] = UP6(CC_SEL(src, 21));
	rgba[BCOMP] = UP5(CC_SEL(src, 16));
    } else {
	const word col0 = src[0] | (src[1] << 8);
	const word col1 = src[2] | (src[3] << 8);
	if (col0 > col1) {
	    if (code == 2) {
		rgba[RCOMP] = (UP5(col0 >> 11) * 2 + UP5(col1 >> 11)) / 3;
		rgba[GCOMP] = (UP6(col0 >>  5) * 2 + UP6(col1 >>  5)) / 3;
		rgba[BCOMP] = (UP5(col0      ) * 2 + UP5(col1      )) / 3;
	    } else {
		rgba[RCOMP] = (UP5(col0 >> 11) + 2 * UP5(col1 >> 11)) / 3;
		rgba[GCOMP] = (UP6(col0 >>  5) + 2 * UP6(col1 >>  5)) / 3;
		rgba[BCOMP] = (UP5(col0      ) + 2 * UP5(col1      )) / 3;
	    }
	} else {
	    if (code == 2) {
		rgba[RCOMP] = (UP5(col0 >> 11) + UP5(col1 >> 11)) / 2;
		rgba[GCOMP] = (UP6(col0 >>  5) + UP6(col1 >>  5)) / 2;
		rgba[BCOMP] = (UP5(col0      ) + UP5(col1      )) / 2;
	    } else {
		ZERO_4UBV(rgba);
	    }
	}
    }
    rgba[ACOMP] = 255;
}


void TAPIENTRY
dxt1_rgba_decode_1 (const void *texture, int stride,
		    int i, int j, byte *rgba)
{
    /* Same as rgb_dxt1 above, except alpha=0 if col0<=col1 and code=3. */
    const byte *src = (const byte *)texture
		       + ((j / 4) * ((stride + 3) / 4) + i / 4) * 8;
    const int code = (src[4 + (j & 3)] >> ((i & 3) * 2)) & 0x3;
    if (code == 0) {
	rgba[RCOMP] = UP5(CC_SEL(src, 11));
	rgba[GCOMP] = UP6(CC_SEL(src,  5));
	rgba[BCOMP] = UP5(CC_SEL(src,  0));
	rgba[ACOMP] = 255;
    } else if (code == 1) {
	rgba[RCOMP] = UP5(CC_SEL(src, 27));
	rgba[GCOMP] = UP6(CC_SEL(src, 21));
	rgba[BCOMP] = UP5(CC_SEL(src, 16));
	rgba[ACOMP] = 255;
    } else {
	const word col0 = src[0] | (src[1] << 8);
	const word col1 = src[2] | (src[3] << 8);
	if (col0 > col1) {
	    if (code == 2) {
		rgba[RCOMP] = (UP5(col0 >> 11) * 2 + UP5(col1 >> 11)) / 3;
		rgba[GCOMP] = (UP6(col0 >>  5) * 2 + UP6(col1 >>  5)) / 3;
		rgba[BCOMP] = (UP5(col0      ) * 2 + UP5(col1      )) / 3;
	    } else {
		rgba[RCOMP] = (UP5(col0 >> 11) + 2 * UP5(col1 >> 11)) / 3;
		rgba[GCOMP] = (UP6(col0 >>  5) + 2 * UP6(col1 >>  5)) / 3;
		rgba[BCOMP] = (UP5(col0      ) + 2 * UP5(col1      )) / 3;
	    }
	    rgba[ACOMP] = 255;
	} else {
	    if (code == 2) {
		rgba[RCOMP] = (UP5(col0 >> 11) + UP5(col1 >> 11)) / 2;
		rgba[GCOMP] = (UP6(col0 >>  5) + UP6(col1 >>  5)) / 2;
		rgba[BCOMP] = (UP5(col0      ) + UP5(col1      )) / 2;
		rgba[ACOMP] = 255;
	    } else {
		ZERO_4UBV(rgba);
	    }
	}
    }
}


void TAPIENTRY
dxt3_rgba_decode_1 (const void *texture, int stride,
		    int i, int j, byte *rgba)
{
    const byte *src = (const byte *)texture
		       + ((j / 4) * ((stride + 3) / 4) + i / 4) * 16;
    const int code = (src[12 + (j & 3)] >> ((i & 3) * 2)) & 0x3;
    const dword *cc = (const dword *)(src + 8);
    if (code == 0) {
	rgba[RCOMP] = UP5(CC_SEL(cc, 11));
	rgba[GCOMP] = UP6(CC_SEL(cc,  5));
	rgba[BCOMP] = UP5(CC_SEL(cc,  0));
    } else if (code == 1) {
	rgba[RCOMP] = UP5(CC_SEL(cc, 27));
	rgba[GCOMP] = UP6(CC_SEL(cc, 21));
	rgba[BCOMP] = UP5(CC_SEL(cc, 16));
    } else if (code == 2) {
	/* (col0 * (4 - code) + col1 * (code - 1)) / 3 */
	rgba[RCOMP] = (UP5(CC_SEL(cc, 11)) * 2 + UP5(CC_SEL(cc, 27))) / 3;
	rgba[GCOMP] = (UP6(CC_SEL(cc,  5)) * 2 + UP6(CC_SEL(cc, 21))) / 3;
	rgba[BCOMP] = (UP5(CC_SEL(cc,  0)) * 2 + UP5(CC_SEL(cc, 16))) / 3;
    } else {
	rgba[RCOMP] = (UP5(CC_SEL(cc, 11)) + 2 * UP5(CC_SEL(cc, 27))) / 3;
	rgba[GCOMP] = (UP6(CC_SEL(cc,  5)) + 2 * UP6(CC_SEL(cc, 21))) / 3;
	rgba[BCOMP] = (UP5(CC_SEL(cc,  0)) + 2 * UP5(CC_SEL(cc, 16))) / 3;
    }
    rgba[ACOMP] = UP4(src[((j & 3) * 4 + (i & 3)) / 2] >> ((i & 1) * 4));
}


void TAPIENTRY
dxt5_rgba_decode_1 (const void *texture, int stride,
		    int i, int j, byte *rgba)
{
    const byte *src = (const byte *)texture
		       + ((j / 4) * ((stride + 3) / 4) + i / 4) * 16;
    const int code = (src[12 + (j & 3)] >> ((i & 3) * 2)) & 0x3;
    const dword *cc = (const dword *)(src + 8);
    const byte alpha0 = src[0];
    const byte alpha1 = src[1];
    const int alphaShift = (((j & 3) * 4) + (i & 3)) * 3 + 16;
    const int acode = ((alphaShift == 31)
			? CC_SEL(src + 2, alphaShift - 16)
			: CC_SEL(src, alphaShift)) & 0x7;
    if (code == 0) {
	rgba[RCOMP] = UP5(CC_SEL(cc, 11));
	rgba[GCOMP] = UP6(CC_SEL(cc,  5));
	rgba[BCOMP] = UP5(CC_SEL(cc,  0));
    } else if (code == 1) {
	rgba[RCOMP] = UP5(CC_SEL(cc, 27));
	rgba[GCOMP] = UP6(CC_SEL(cc, 21));
	rgba[BCOMP] = UP5(CC_SEL(cc, 16));
    } else if (code == 2) {
	/* (col0 * (4 - code) + col1 * (code - 1)) / 3 */
	rgba[RCOMP] = (UP5(CC_SEL(cc, 11)) * 2 + UP5(CC_SEL(cc, 27))) / 3;
	rgba[GCOMP] = (UP6(CC_SEL(cc,  5)) * 2 + UP6(CC_SEL(cc, 21))) / 3;
	rgba[BCOMP] = (UP5(CC_SEL(cc,  0)) * 2 + UP5(CC_SEL(cc, 16))) / 3;
    } else {
	rgba[RCOMP] = (UP5(CC_SEL(cc, 11)) + 2 * UP5(CC_SEL(cc, 27))) / 3;
	rgba[GCOMP] = (UP6(CC_SEL(cc,  5)) + 2 * UP6(CC_SEL(cc, 21))) / 3;
	rgba[BCOMP] = (UP5(CC_SEL(cc,  0)) + 2 * UP5(CC_SEL(cc, 16))) / 3;
    }
    if (acode == 0) {
	rgba[ACOMP] = alpha0;
    } else if (acode == 1) {
	rgba[ACOMP] = alpha1;
    } else if (alpha0 > alpha1) {
	rgba[ACOMP] = ((8 - acode) * alpha0 + (acode - 1) * alpha1) / 7;
    } else if (acode == 6) {
	rgba[ACOMP] = 0;
    } else if (acode == 7) {
	rgba[ACOMP] = 255;
    } else {
	rgba[ACOMP] = ((6 - acode) * alpha0 + (acode - 1) * alpha1) / 5;
    }
}
