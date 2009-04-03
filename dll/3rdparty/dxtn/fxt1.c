/*
 * FXT1 codec
 * Version:  1.1
 *
 * Copyright (C) 2004  Daniel Borca   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DANIEL BORCA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "internal.h"
#include "fxt1.h"


/***************************************************************************\
 * FXT1 encoder
 *
 * The encoder was built by reversing the decoder,
 * and is vaguely based on Texus2 by 3dfx. Note that this code
 * is merely a proof of concept, since it is highly UNoptimized;
 * moreover, it is sub-optimal due to initial conditions passed
 * to Lloyd's algorithm (the interpolation modes are even worse).
\***************************************************************************/


#define MAX_COMP 4 /* ever needed maximum number of components in texel */
#define MAX_VECT 4 /* ever needed maximum number of base vectors to find */
#define N_TEXELS 32 /* number of texels in a block (always 32) */
#define LL_N_REP 50 /* number of iterations in lloyd's vq */
#define LL_RMS_D 10 /* fault tolerance (maximum delta) */
#define LL_RMS_E 255 /* fault tolerance (maximum error) */
#define ALPHA_TS 2 /* alpha threshold: (255 - ALPHA_TS) deemed opaque */
#define ISTBLACK(v) (*((dword *)(v)) == 0)
#define COPY_4UBV(DST, SRC) *((dword *)(DST)) = *((dword *)(SRC))


static int
fxt1_bestcol (float vec[][MAX_COMP], int nv,
	      byte input[MAX_COMP], int nc)
{
    int i, j, best = -1;
    float err = 1e9; /* big enough */

    for (j = 0; j < nv; j++) {
	float e = 0.0F;
	for (i = 0; i < nc; i++) {
	    e += (vec[j][i] - input[i]) * (vec[j][i] - input[i]);
	}
	if (e < err) {
	    err = e;
	    best = j;
	}
    }

    return best;
}


static int
fxt1_worst (float vec[MAX_COMP],
	    byte input[N_TEXELS][MAX_COMP], int nc, int n)
{
    int i, k, worst = -1;
    float err = -1.0F; /* small enough */

    for (k = 0; k < n; k++) {
	float e = 0.0F;
	for (i = 0; i < nc; i++) {
	    e += (vec[i] - input[k][i]) * (vec[i] - input[k][i]);
	}
	if (e > err) {
	    err = e;
	    worst = k;
	}
    }

    return worst;
}


static int
fxt1_variance (double variance[MAX_COMP],
	       byte input[N_TEXELS][MAX_COMP], int nc, int n)
{
    int i, k, best = 0;
    dword sx, sx2;
    double var, maxvar = -1; /* small enough */
    double teenth = 1.0 / n;

    for (i = 0; i < nc; i++) {
	sx = sx2 = 0;
	for (k = 0; k < n; k++) {
	    int t = input[k][i];
	    sx += t;
	    sx2 += t * t;
	}
	var = sx2 * teenth - sx * sx * teenth * teenth;
	if (maxvar < var) {
	    maxvar = var;
	    best = i;
	}
	if (variance) {
	    variance[i] = var;
	}
    }

    return best;
}


static int
fxt1_choose (float vec[][MAX_COMP], int nv,
	     byte input[N_TEXELS][MAX_COMP], int nc, int n)
{
#if 0
    /* Choose colors from a grid.
     */
    int i, j;

    for (j = 0; j < nv; j++) {
	int m = j * (n - 1) / (nv - 1);
	for (i = 0; i < nc; i++) {
	    vec[j][i] = input[m][i];
	}
    }
#else
    /* Our solution here is to find the darkest and brightest colors in
     * the 8x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    int i, j, k;
    int minSum = 2000; /* big enough */
    int maxSum = -1; /* small enough */
    int minCol = 0; /* phoudoin: silent compiler! */
    int maxCol = 0; /* phoudoin: silent compiler! */

    struct {
	int flag;
	dword key;
	int freq;
	int idx;
    } hist[N_TEXELS];
    int lenh = 0;

    memset(hist, 0, sizeof(hist));

    for (k = 0; k < n; k++) {
	int l;
	dword key = 0;
	int sum = 0;
	for (i = 0; i < nc; i++) {
	    key <<= 8;
	    key |= input[k][i];
	    sum += input[k][i];
	}
	for (l = 0; l < n; l++) {
	    if (!hist[l].flag) {
		/* alloc new slot */
		hist[l].flag = !0;
		hist[l].key = key;
		hist[l].freq = 1;
		hist[l].idx = k;
		lenh = l + 1;
		break;
	    } else if (hist[l].key == key) {
		hist[l].freq++;
		break;
	    }
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

    if (lenh <= nv) {
	for (j = 0; j < lenh; j++) {
	    for (i = 0; i < nc; i++) {
		vec[j][i] = (float)input[hist[j].idx][i];
	    }
	}
	for (; j < nv; j++) {
	    for (i = 0; i < nc; i++) {
		vec[j][i] = vec[0][i];
	    }
	}
	return 0;
    }

    for (j = 0; j < nv; j++) {
	for (i = 0; i < nc; i++) {
	    vec[j][i] = ((nv - 1 - j) * input[minCol][i] + j * input[maxCol][i] + (nv - 1) / 2) / (float)(nv - 1);
	}
    }
#endif

    return !0;
}


static int
fxt1_lloyd (float vec[][MAX_COMP], int nv,
	    byte input[N_TEXELS][MAX_COMP], int nc, int n)
{
    /* Use the generalized lloyd's algorithm for VQ:
     *     find 4 color vectors.
     *
     *     for each sample color
     *         sort to nearest vector.
     *
     *     replace each vector with the centroid of it's matching colors.
     *
     *     repeat until RMS doesn't improve.
     *
     *     if a color vector has no samples, or becomes the same as another
     *     vector, replace it with the color which is farthest from a sample.
     *
     * vec[][MAX_COMP]           initial vectors and resulting colors
     * nv                        number of resulting colors required
     * input[N_TEXELS][MAX_COMP] input texels
     * nc                        number of components in input / vec
     * n                         number of input samples
     */

    int sum[MAX_VECT][MAX_COMP]; /* used to accumulate closest texels */
    int cnt[MAX_VECT]; /* how many times a certain vector was chosen */
    float error, lasterror = 1e9;

    int i, j, k, rep;

    /* the quantizer */
    for (rep = 0; rep < LL_N_REP; rep++) {
	/* reset sums & counters */
	for (j = 0; j < nv; j++) {
	    for (i = 0; i < nc; i++) {
		sum[j][i] = 0;
	    }
	    cnt[j] = 0;
	}
	error = 0;

	/* scan whole block */
	for (k = 0; k < n; k++) {
#if 1
	    int best = -1;
	    float err = 1e9; /* big enough */
	    /* determine best vector */
	    for (j = 0; j < nv; j++) {
		float e = (vec[j][0] - input[k][0]) * (vec[j][0] - input[k][0]) +
			  (vec[j][1] - input[k][1]) * (vec[j][1] - input[k][1]) +
			  (vec[j][2] - input[k][2]) * (vec[j][2] - input[k][2]);
		if (nc == 4) {
		    e += (vec[j][3] - input[k][3]) * (vec[j][3] - input[k][3]);
		}
		if (e < err) {
		    err = e;
		    best = j;
		}
	    }
#else
	    int best = fxt1_bestcol(vec, nv, input[k], nc, &err);
#endif
	    /* add in closest color */
	    for (i = 0; i < nc; i++) {
		sum[best][i] += input[k][i];
	    }
	    /* mark this vector as used */
	    cnt[best]++;
	    /* accumulate error */
	    error += err;
	}

	/* check RMS */
	if ((error < LL_RMS_E) ||
	    ((error < lasterror) && ((lasterror - error) < LL_RMS_D))) {
	    return !0; /* good match */
	}
	lasterror = error;

	/* move each vector to the barycenter of its closest colors */
	for (j = 0; j < nv; j++) {
	    if (cnt[j]) {
		float div = 1.0F / cnt[j];
		for (i = 0; i < nc; i++) {
		    vec[j][i] = div * sum[j][i];
		}
	    } else {
		/* this vec has no samples or is identical with a previous vec */
		int worst = fxt1_worst(vec[j], input, nc, n);
		for (i = 0; i < nc; i++) {
		    vec[j][i] = input[worst][i];
		}
	    }
	}
    }

    return 0; /* could not converge fast enough */
}


static void
fxt1_quantize_CHROMA (dword *cc,
		      byte input[N_TEXELS][MAX_COMP])
{
    const int n_vect = 4; /* 4 base vectors to find */
    const int n_comp = 3; /* 3 components: R, G, B */
    float vec[MAX_VECT][MAX_COMP];
    int i, j, k;
    qword hi; /* high quadword */
    dword lohi, lolo; /* low quadword: hi dword, lo dword */

    if (fxt1_choose(vec, n_vect, input, n_comp, N_TEXELS) != 0) {
	fxt1_lloyd(vec, n_vect, input, n_comp, N_TEXELS);
    }

    Q_MOV32(hi, 4); /* cc-chroma = "010" + unused bit */
    for (j = n_vect - 1; j >= 0; j--) {
	for (i = 0; i < n_comp; i++) {
	    /* add in colors */
	    Q_SHL(hi, 5);
	    Q_OR32(hi, (dword)(vec[j][i] / 8.0F));
	}
    }
    ((qword *)cc)[1] = hi;

    lohi = lolo = 0;
    /* right microtile */
    for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
	lohi <<= 2;
	lohi |= fxt1_bestcol(vec, n_vect, input[k], n_comp);
    }
    /* left microtile */
    for (; k >= 0; k--) {
	lolo <<= 2;
	lolo |= fxt1_bestcol(vec, n_vect, input[k], n_comp);
    }
    cc[1] = lohi;
    cc[0] = lolo;
}


static void
fxt1_quantize_ALPHA0 (dword *cc,
		      byte input[N_TEXELS][MAX_COMP],
		      byte reord[N_TEXELS][MAX_COMP], int n)
{
    const int n_vect = 3; /* 3 base vectors to find */
    const int n_comp = 4; /* 4 components: R, G, B, A */
    float vec[MAX_VECT][MAX_COMP];
    int i, j, k;
    qword hi; /* high quadword */
    dword lohi, lolo; /* low quadword: hi dword, lo dword */

    /* the last vector indicates zero */
    for (i = 0; i < n_comp; i++) {
	vec[n_vect][i] = 0;
    }

    /* the first n texels in reord are guaranteed to be non-zero */
    if (fxt1_choose(vec, n_vect, reord, n_comp, n) != 0) {
	fxt1_lloyd(vec, n_vect, reord, n_comp, n);
    }

    Q_MOV32(hi, 6); /* alpha = "011" + lerp = 0 */
    for (j = n_vect - 1; j >= 0; j--) {
	/* add in alphas */
	Q_SHL(hi, 5);
	Q_OR32(hi, (dword)(vec[j][ACOMP] / 8.0F));
    }
    for (j = n_vect - 1; j >= 0; j--) {
	for (i = 0; i < n_comp - 1; i++) {
	    /* add in colors */
	    Q_SHL(hi, 5);
	    Q_OR32(hi, (dword)(vec[j][i] / 8.0F));
	}
    }
    ((qword *)cc)[1] = hi;

    lohi = lolo = 0;
    /* right microtile */
    for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
	lohi <<= 2;
	lohi |= fxt1_bestcol(vec, n_vect + 1, input[k], n_comp);
    }
    /* left microtile */
    for (; k >= 0; k--) {
	lolo <<= 2;
	lolo |= fxt1_bestcol(vec, n_vect + 1, input[k], n_comp);
    }
    cc[1] = lohi;
    cc[0] = lolo;
}


static void
fxt1_quantize_ALPHA1 (dword *cc,
		      byte input[N_TEXELS][MAX_COMP])
{
    const int n_vect = 3; /* highest vector number in each microtile */
    const int n_comp = 4; /* 4 components: R, G, B, A */
    float vec[1 + 1 + 1][MAX_COMP]; /* 1.5 extrema for each sub-block */
    float b, iv[MAX_COMP]; /* interpolation vector */
    int i, j, k;
    qword hi; /* high quadword */
    dword lohi, lolo; /* low quadword: hi dword, lo dword */

    int minSum;
    int maxSum;
    int minColL = 0, maxColL = 0;
    int minColR = 0, maxColR = 0;
    int sumL = 0, sumR = 0;

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    minSum = 2000; /* big enough */
    maxSum = -1; /* small enough */
    for (k = 0; k < N_TEXELS / 2; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minColL = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxColL = k;
	}
	sumL += sum;
    }
    minSum = 2000; /* big enough */
    maxSum = -1; /* small enough */
    for (; k < N_TEXELS; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minColR = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxColR = k;
	}
	sumR += sum;
    }

    /* choose the common vector (yuck!) */
    {
	int j1, j2;
	int v1 = 0, v2 = 0;
	float err = 1e9; /* big enough */
	float tv[2 * 2][MAX_COMP]; /* 2 extrema for each sub-block */
	for (i = 0; i < n_comp; i++) {
	    tv[0][i] = input[minColL][i];
	    tv[1][i] = input[maxColL][i];
	    tv[2][i] = input[minColR][i];
	    tv[3][i] = input[maxColR][i];
	}
	for (j1 = 0; j1 < 2; j1++) {
	    for (j2 = 2; j2 < 4; j2++) {
		float e = 0.0F;
		for (i = 0; i < n_comp; i++) {
		    e += (tv[j1][i] - tv[j2][i]) * (tv[j1][i] - tv[j2][i]);
		}
		if (e < err) {
		    err = e;
		    v1 = j1;
		    v2 = j2;
		}
	    }
	}
	for (i = 0; i < n_comp; i++) {
	    vec[0][i] = tv[1 - v1][i];
	    vec[1][i] = (tv[v1][i] * sumL + tv[v2][i] * sumR) / (sumL + sumR);
	    vec[2][i] = tv[5 - v2][i];
	}
    }

    /* left microtile */
    cc[0] = 0;
    if (minColL != maxColL) {
	/* compute interpolation vector */
	MAKEIVEC(n_vect, n_comp, iv, b, vec[0], vec[1]);

	/* add in texels */
	lolo = 0;
	for (k = N_TEXELS / 2 - 1; k >= 0; k--) {
	    int texel;
	    /* interpolate color */
	    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
	    /* add in texel */
	    lolo <<= 2;
	    lolo |= texel;
	}

	cc[0] = lolo;
    }

    /* right microtile */
    cc[1] = 0;
    if (minColR != maxColR) {
	/* compute interpolation vector */
	MAKEIVEC(n_vect, n_comp, iv, b, vec[2], vec[1]);

	/* add in texels */
	lohi = 0;
	for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
	    int texel;
	    /* interpolate color */
	    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
	    /* add in texel */
	    lohi <<= 2;
	    lohi |= texel;
	}

	cc[1] = lohi;
    }

    Q_MOV32(hi, 7); /* alpha = "011" + lerp = 1 */
    for (j = n_vect - 1; j >= 0; j--) {
	/* add in alphas */
	Q_SHL(hi, 5);
	Q_OR32(hi, (dword)(vec[j][ACOMP] / 8.0F));
    }
    for (j = n_vect - 1; j >= 0; j--) {
	for (i = 0; i < n_comp - 1; i++) {
	    /* add in colors */
	    Q_SHL(hi, 5);
	    Q_OR32(hi, (dword)(vec[j][i] / 8.0F));
	}
    }
    ((qword *)cc)[1] = hi;
}


static void
fxt1_quantize_HI (dword *cc,
		  byte input[N_TEXELS][MAX_COMP],
		  byte reord[N_TEXELS][MAX_COMP], int n)
{
    const int n_vect = 6; /* highest vector number */
    const int n_comp = 3; /* 3 components: R, G, B */
    float b = 0.0F;       /* phoudoin: silent compiler! */
    float iv[MAX_COMP];   /* interpolation vector */
    int i, k;
    dword hihi; /* high quadword: hi dword */

    int minSum = 2000; /* big enough */
    int maxSum = -1; /* small enough */
    int minCol = 0; /* phoudoin: silent compiler! */
    int maxCol = 0; /* phoudoin: silent compiler! */

    /* Our solution here is to find the darkest and brightest colors in
     * the 8x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    for (k = 0; k < n; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += reord[k][i];
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

    hihi = 0; /* cc-hi = "00" */
    for (i = 0; i < n_comp; i++) {
	/* add in colors */
	hihi <<= 5;
	hihi |= reord[maxCol][i] >> 3;
    }
    for (i = 0; i < n_comp; i++) {
	/* add in colors */
	hihi <<= 5;
	hihi |= reord[minCol][i] >> 3;
    }
    cc[3] = hihi;
    cc[0] = cc[1] = cc[2] = 0;

    /* compute interpolation vector */
    if (minCol != maxCol) {
	MAKEIVEC(n_vect, n_comp, iv, b, reord[minCol], reord[maxCol]);
    }

    /* add in texels */
    for (k = N_TEXELS - 1; k >= 0; k--) {
	int t = k * 3;
	dword *kk = (dword *)((byte *)cc + t / 8);
	int texel = n_vect + 1; /* transparent black */

	if (!ISTBLACK(input[k])) {
	    if (minCol != maxCol) {
		/* interpolate color */
		CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
		/* add in texel */
		kk[0] |= texel << (t & 7);
	    }
	} else {
	    /* add in texel */
	    kk[0] |= texel << (t & 7);
	}
    }
}


static void
fxt1_quantize_MIXED1 (dword *cc,
		      byte input[N_TEXELS][MAX_COMP])
{
    const int n_vect = 2; /* highest vector number in each microtile */
    const int n_comp = 3; /* 3 components: R, G, B */
    byte vec[2 * 2][MAX_COMP]; /* 2 extrema for each sub-block */
    float b, iv[MAX_COMP]; /* interpolation vector */
    int i, j, k;
    qword hi; /* high quadword */
    dword lohi, lolo; /* low quadword: hi dword, lo dword */

    int minSum;
    int maxSum;
    int minColL = 0, maxColL = -1;
    int minColR = 0, maxColR = -1;

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    minSum = 2000; /* big enough */
    maxSum = -1; /* small enough */
    for (k = 0; k < N_TEXELS / 2; k++) {
	if (!ISTBLACK(input[k])) {
	    int sum = 0;
	    for (i = 0; i < n_comp; i++) {
		sum += input[k][i];
	    }
	    if (minSum > sum) {
		minSum = sum;
		minColL = k;
	    }
	    if (maxSum < sum) {
		maxSum = sum;
		maxColL = k;
	    }
	}
    }
    minSum = 2000; /* big enough */
    maxSum = -1; /* small enough */
    for (; k < N_TEXELS; k++) {
	if (!ISTBLACK(input[k])) {
	    int sum = 0;
	    for (i = 0; i < n_comp; i++) {
		sum += input[k][i];
	    }
	    if (minSum > sum) {
		minSum = sum;
		minColR = k;
	    }
	    if (maxSum < sum) {
		maxSum = sum;
		maxColR = k;
	    }
	}
    }

    /* left microtile */
    if (maxColL == -1) {
	/* all transparent black */
	cc[0] = ~0UL;
	for (i = 0; i < n_comp; i++) {
	    vec[0][i] = 0;
	    vec[1][i] = 0;
	}
    } else {
	cc[0] = 0;
	for (i = 0; i < n_comp; i++) {
	    vec[0][i] = input[minColL][i];
	    vec[1][i] = input[maxColL][i];
	}
	if (minColL != maxColL) {
	    /* compute interpolation vector */
	    MAKEIVEC(n_vect, n_comp, iv, b, vec[0], vec[1]);

	    /* add in texels */
	    lolo = 0;
	    for (k = N_TEXELS / 2 - 1; k >= 0; k--) {
		int texel = n_vect + 1;	/* transparent black */
		if (!ISTBLACK(input[k])) {
		    /* interpolate color */
		    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
		}
		/* add in texel */
		lolo <<= 2;
		lolo |= texel;
	    }
	    cc[0] = lolo;
	}
    }

    /* right microtile */
    if (maxColR == -1) {
	/* all transparent black */
	cc[1] = ~0UL;
	for (i = 0; i < n_comp; i++) {
	    vec[2][i] = 0;
	    vec[3][i] = 0;
	}
    } else {
	cc[1] = 0;
	for (i = 0; i < n_comp; i++) {
	    vec[2][i] = input[minColR][i];
	    vec[3][i] = input[maxColR][i];
	}
	if (minColR != maxColR) {
	    /* compute interpolation vector */
	    MAKEIVEC(n_vect, n_comp, iv, b, vec[2], vec[3]);

	    /* add in texels */
	    lohi = 0;
	    for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
		int texel = n_vect + 1;	/* transparent black */
		if (!ISTBLACK(input[k])) {
		    /* interpolate color */
		    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
		}
		/* add in texel */
		lohi <<= 2;
		lohi |= texel;
	    }
	    cc[1] = lohi;
	}
    }

    Q_MOV32(hi, 9 | (vec[3][GCOMP] & 4) | ((vec[1][GCOMP] >> 1) & 2)); /* chroma = "1" */
    for (j = 2 * 2 - 1; j >= 0; j--) {
	for (i = 0; i < n_comp; i++) {
	    /* add in colors */
	    Q_SHL(hi, 5);
	    Q_OR32(hi, vec[j][i] >> 3);
	}
    }
    ((qword *)cc)[1] = hi;
}


static void
fxt1_quantize_MIXED0 (dword *cc,
		      byte input[N_TEXELS][MAX_COMP])
{
    const int n_vect = 3; /* highest vector number in each microtile */
    const int n_comp = 3; /* 3 components: R, G, B */
    byte vec[2 * 2][MAX_COMP]; /* 2 extrema for each sub-block */
    float b, iv[MAX_COMP]; /* interpolation vector */
    int i, j, k;
    qword hi; /* high quadword */
    dword lohi, lolo; /* low quadword: hi dword, lo dword */

    int minColL = 0, maxColL = 0;
    int minColR = 0, maxColR = 0;
#if 0
    int minSum;
    int maxSum;

    /* Our solution here is to find the darkest and brightest colors in
     * the 4x4 tile and use those as the two representative colors.
     * There are probably better algorithms to use (histogram-based).
     */
    minSum = 2000; /* big enough */
    maxSum = -1; /* small enough */
    for (k = 0; k < N_TEXELS / 2; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minColL = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxColL = k;
	}
    }
    minSum = 2000; /* big enough */
    maxSum = -1; /* small enough */
    for (; k < N_TEXELS; k++) {
	int sum = 0;
	for (i = 0; i < n_comp; i++) {
	    sum += input[k][i];
	}
	if (minSum > sum) {
	    minSum = sum;
	    minColR = k;
	}
	if (maxSum < sum) {
	    maxSum = sum;
	    maxColR = k;
	}
    }
#else
    int minVal;
    int maxVal;
    int maxVarL = fxt1_variance(NULL, input, n_comp, N_TEXELS / 2);
    int maxVarR = fxt1_variance(NULL, &input[N_TEXELS / 2], n_comp, N_TEXELS / 2);

    /* Scan the channel with max variance for lo & hi
     * and use those as the two representative colors.
     */
    minVal = 2000; /* big enough */
    maxVal = -1; /* small enough */
    for (k = 0; k < N_TEXELS / 2; k++) {
	int t = input[k][maxVarL];
	if (minVal > t) {
	    minVal = t;
	    minColL = k;
	}
	if (maxVal < t) {
	    maxVal = t;
	    maxColL = k;
	}
    }
    minVal = 2000; /* big enough */
    maxVal = -1; /* small enough */
    for (; k < N_TEXELS; k++) {
	int t = input[k][maxVarR];
	if (minVal > t) {
	    minVal = t;
	    minColR = k;
	}
	if (maxVal < t) {
	    maxVal = t;
	    maxColR = k;
	}
    }
#endif

    /* left microtile */
    cc[0] = 0;
    for (i = 0; i < n_comp; i++) {
	vec[0][i] = input[minColL][i];
	vec[1][i] = input[maxColL][i];
    }
    if (minColL != maxColL) {
	/* compute interpolation vector */
	MAKEIVEC(n_vect, n_comp, iv, b, vec[0], vec[1]);

	/* add in texels */
	lolo = 0;
	for (k = N_TEXELS / 2 - 1; k >= 0; k--) {
	    int texel;
	    /* interpolate color */
	    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
	    /* add in texel */
	    lolo <<= 2;
	    lolo |= texel;
	}

	/* funky encoding for LSB of green */
	if ((int)((lolo >> 1) & 1) != (((vec[1][GCOMP] ^ vec[0][GCOMP]) >> 2) & 1)) {
	    for (i = 0; i < n_comp; i++) {
		vec[1][i] = input[minColL][i];
		vec[0][i] = input[maxColL][i];
	    }
	    lolo = ~lolo;
	}

	cc[0] = lolo;
    }

    /* right microtile */
    cc[1] = 0;
    for (i = 0; i < n_comp; i++) {
	vec[2][i] = input[minColR][i];
	vec[3][i] = input[maxColR][i];
    }
    if (minColR != maxColR) {
	/* compute interpolation vector */
	MAKEIVEC(n_vect, n_comp, iv, b, vec[2], vec[3]);

	/* add in texels */
	lohi = 0;
	for (k = N_TEXELS - 1; k >= N_TEXELS / 2; k--) {
	    int texel;
	    /* interpolate color */
	    CALCCDOT(texel, n_vect, n_comp, iv, b, input[k]);
	    /* add in texel */
	    lohi <<= 2;
	    lohi |= texel;
	}

	/* funky encoding for LSB of green */
	if ((int)((lohi >> 1) & 1) != (((vec[3][GCOMP] ^ vec[2][GCOMP]) >> 2) & 1)) {
	    for (i = 0; i < n_comp; i++) {
		vec[3][i] = input[minColR][i];
		vec[2][i] = input[maxColR][i];
	    }
	    lohi = ~lohi;
	}

	cc[1] = lohi;
    }

    Q_MOV32(hi, 8 | (vec[3][GCOMP] & 4) | ((vec[1][GCOMP] >> 1) & 2)); /* chroma = "1" */
    for (j = 2 * 2 - 1; j >= 0; j--) {
	for (i = 0; i < n_comp; i++) {
	    /* add in colors */
	    Q_SHL(hi, 5);
	    Q_OR32(hi, vec[j][i] >> 3);
	}
    }
    ((qword *)cc)[1] = hi;
}


static void
fxt1_quantize (dword *cc, const byte *lines[], int comps)
{
    int trualpha;
    byte reord[N_TEXELS][MAX_COMP];

    byte input[N_TEXELS][MAX_COMP];
    int i, k, l;

    if (comps == 3) {
	/* make the whole block opaque */
	memset(input, -1, sizeof(input));
    }

    /* 8 texels each line */
    for (l = 0; l < 4; l++) {
	for (k = 0; k < 4; k++) {
	    for (i = 0; i < comps; i++) {
		input[k + l * 4][i] = *lines[l]++;
	    }
	}
	for (; k < 8; k++) {
	    for (i = 0; i < comps; i++) {
		input[k + l * 4 + 12][i] = *lines[l]++;
	    }
	}
    }

    /* block layout:
     * 00, 01, 02, 03, 08, 09, 0a, 0b
     * 10, 11, 12, 13, 18, 19, 1a, 1b
     * 04, 05, 06, 07, 0c, 0d, 0e, 0f
     * 14, 15, 16, 17, 1c, 1d, 1e, 1f
     */

    /* [dBorca]
     * stupidity flows forth from this
     */
    l = N_TEXELS;
    trualpha = 0;
    if (comps == 4) {
	/* skip all transparent black texels */
	l = 0;
	for (k = 0; k < N_TEXELS; k++) {
	    /* test all components against 0 */
	    if (!ISTBLACK(input[k])) {
		/* texel is not transparent black */
		COPY_4UBV(reord[l], input[k]);
		if (reord[l][ACOMP] < (255 - ALPHA_TS)) {
		    /* non-opaque texel */
		    trualpha = !0;
		}
		l++;
	    }
	}
    }

#if 0
    if (trualpha) {
	fxt1_quantize_ALPHA0(cc, input, reord, l);
    } else if (l == 0) {
	cc[0] = cc[1] = cc[2] = -1;
	cc[3] = 0;
    } else if (l < N_TEXELS) {
	fxt1_quantize_HI(cc, input, reord, l);
    } else {
	fxt1_quantize_CHROMA(cc, input);
    }
    (void)fxt1_quantize_ALPHA1;
    (void)fxt1_quantize_MIXED1;
    (void)fxt1_quantize_MIXED0;
#else
    if (trualpha) {
	fxt1_quantize_ALPHA1(cc, input);
    } else if (l == 0) {
	cc[0] = cc[1] = cc[2] = ~0UL;
	cc[3] = 0;
    } else if (l < N_TEXELS) {
	fxt1_quantize_MIXED1(cc, input);
    } else {
	fxt1_quantize_MIXED0(cc, input);
    }
    (void)fxt1_quantize_ALPHA0;
    (void)fxt1_quantize_HI;
    (void)fxt1_quantize_CHROMA;
#endif
}


TAPI int TAPIENTRY
fxt1_encode (int width, int height, int comps,
	     const void *source, int srcRowStride,
	     void *dest, int destRowStride)
{
    int x, y;
    const byte *data;
    dword *encoded = (dword *)dest;
    void *newSource = NULL;

    /* Replicate image if width is not M8 or height is not M4 */
    if ((width & 7) | (height & 3)) {
	int newWidth = (width + 7) & ~7;
	int newHeight = (height + 3) & ~3;
	newSource = malloc(comps * newWidth * newHeight * sizeof(byte *));
	_mesa_upscale_teximage2d(width, height, newWidth, newHeight,
				 comps, (const byte *)source,
				 srcRowStride, (byte *)newSource);
	source = newSource;
	width = newWidth;
	height = newHeight;
	srcRowStride = comps * newWidth;
    }

    data = (const byte *)source;
    destRowStride = (destRowStride - width * 2) / 4;
    for (y = 0; y < height; y += 4) {
	unsigned int offs = 0 + (y + 0) * srcRowStride;
	for (x = 0; x < width; x += 8) {
	    const byte *lines[4];
	    lines[0] = &data[offs];
	    lines[1] = lines[0] + srcRowStride;
	    lines[2] = lines[1] + srcRowStride;
	    lines[3] = lines[2] + srcRowStride;
	    offs += 8 * comps;
	    fxt1_quantize(encoded, lines, comps);
	    /* 128 bits per 8x4 block */
	    encoded += 4;
	}
	encoded += destRowStride;
    }

    if (newSource != NULL) {
	free(newSource);
    }

    return 0;
}


/***************************************************************************\
 * FXT1 decoder
 *
 * The decoder is based on GL_3DFX_texture_compression_FXT1
 * specification and serves as a concept for the encoder.
\***************************************************************************/


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
#define UP5(c) _rgb_scale_5[(c) & 31]
#define UP6(c, b) _rgb_scale_6[(((c) & 31) << 1) | ((b) & 1)]
#define LERP(n, t, c0, c1) (((n) - (t)) * (c0) + (t) * (c1) + (n) / 2) / (n)
#define ZERO_4UBV(v) *((dword *)(v)) = 0


static void
fxt1_decode_1HI (const byte *code, int t, byte *rgba)
{
    const dword *cc;

    t *= 3;
    cc = (const dword *)(code + t / 8);
    t = (cc[0] >> (t & 7)) & 7;

    if (t == 7) {
	ZERO_4UBV(rgba);
    } else {
	cc = (const dword *)(code + 12);
	if (t == 0) {
	    rgba[BCOMP] = UP5(CC_SEL(cc, 0));
	    rgba[GCOMP] = UP5(CC_SEL(cc, 5));
	    rgba[RCOMP] = UP5(CC_SEL(cc, 10));
	} else if (t == 6) {
	    rgba[BCOMP] = UP5(CC_SEL(cc, 15));
	    rgba[GCOMP] = UP5(CC_SEL(cc, 20));
	    rgba[RCOMP] = UP5(CC_SEL(cc, 25));
	} else {
	    rgba[BCOMP] = LERP(6, t, UP5(CC_SEL(cc, 0)), UP5(CC_SEL(cc, 15)));
	    rgba[GCOMP] = LERP(6, t, UP5(CC_SEL(cc, 5)), UP5(CC_SEL(cc, 20)));
	    rgba[RCOMP] = LERP(6, t, UP5(CC_SEL(cc, 10)), UP5(CC_SEL(cc, 25)));
	}
	rgba[ACOMP] = 255;
    }
}


static void
fxt1_decode_1CHROMA (const byte *code, int t, byte *rgba)
{
    const dword *cc;
    dword kk;

    cc = (const dword *)code;
    if (t & 16) {
	cc++;
	t &= 15;
    }
    t = (cc[0] >> (t * 2)) & 3;

    t *= 15;
    cc = (const dword *)(code + 8 + t / 8);
    kk = cc[0] >> (t & 7);
    rgba[BCOMP] = UP5(kk);
    rgba[GCOMP] = UP5(kk >> 5);
    rgba[RCOMP] = UP5(kk >> 10);
    rgba[ACOMP] = 255;
}


static void
fxt1_decode_1MIXED (const byte *code, int t, byte *rgba)
{
    const dword *cc;
    int col[2][3];
    int glsb, selb;

    cc = (const dword *)code;
    if (t & 16) {
	t &= 15;
	t = (cc[1] >> (t * 2)) & 3;
	/* col 2 */
	col[0][BCOMP] = (*(const dword *)(code + 11)) >> 6;
	col[0][GCOMP] = CC_SEL(cc, 99);
	col[0][RCOMP] = CC_SEL(cc, 104);
	/* col 3 */
	col[1][BCOMP] = CC_SEL(cc, 109);
	col[1][GCOMP] = CC_SEL(cc, 114);
	col[1][RCOMP] = CC_SEL(cc, 119);
	glsb = CC_SEL(cc, 126);
	selb = CC_SEL(cc, 33);
    } else {
	t = (cc[0] >> (t * 2)) & 3;
	/* col 0 */
	col[0][BCOMP] = CC_SEL(cc, 64);
	col[0][GCOMP] = CC_SEL(cc, 69);
	col[0][RCOMP] = CC_SEL(cc, 74);
	/* col 1 */
	col[1][BCOMP] = CC_SEL(cc, 79);
	col[1][GCOMP] = CC_SEL(cc, 84);
	col[1][RCOMP] = CC_SEL(cc, 89);
	glsb = CC_SEL(cc, 125);
	selb = CC_SEL(cc, 1);
    }

    if (CC_SEL(cc, 124) & 1) {
	/* alpha[0] == 1 */

	if (t == 3) {
	    ZERO_4UBV(rgba);
	} else {
	    if (t == 0) {
		rgba[BCOMP] = UP5(col[0][BCOMP]);
		rgba[GCOMP] = UP5(col[0][GCOMP]);
		rgba[RCOMP] = UP5(col[0][RCOMP]);
	    } else if (t == 2) {
		rgba[BCOMP] = UP5(col[1][BCOMP]);
		rgba[GCOMP] = UP6(col[1][GCOMP], glsb);
		rgba[RCOMP] = UP5(col[1][RCOMP]);
	    } else {
		rgba[BCOMP] = (UP5(col[0][BCOMP]) + UP5(col[1][BCOMP])) / 2;
		rgba[GCOMP] = (UP5(col[0][GCOMP]) + UP6(col[1][GCOMP], glsb)) / 2;
		rgba[RCOMP] = (UP5(col[0][RCOMP]) + UP5(col[1][RCOMP])) / 2;
	    }
	    rgba[ACOMP] = 255;
	}
    } else {
	/* alpha[0] == 0 */

	if (t == 0) {
	    rgba[BCOMP] = UP5(col[0][BCOMP]);
	    rgba[GCOMP] = UP6(col[0][GCOMP], glsb ^ selb);
	    rgba[RCOMP] = UP5(col[0][RCOMP]);
	} else if (t == 3) {
	    rgba[BCOMP] = UP5(col[1][BCOMP]);
	    rgba[GCOMP] = UP6(col[1][GCOMP], glsb);
	    rgba[RCOMP] = UP5(col[1][RCOMP]);
	} else {
	    rgba[BCOMP] = LERP(3, t, UP5(col[0][BCOMP]), UP5(col[1][BCOMP]));
	    rgba[GCOMP] = LERP(3, t, UP6(col[0][GCOMP], glsb ^ selb),
				     UP6(col[1][GCOMP], glsb));
	    rgba[RCOMP] = LERP(3, t, UP5(col[0][RCOMP]), UP5(col[1][RCOMP]));
	}
	rgba[ACOMP] = 255;
    }
}


static void
fxt1_decode_1ALPHA (const byte *code, int t, byte *rgba)
{
    const dword *cc;

    cc = (const dword *)code;
    if (CC_SEL(cc, 124) & 1) {
	/* lerp == 1 */
	int col0[4];

	if (t & 16) {
	    t &= 15;
	    t = (cc[1] >> (t * 2)) & 3;
	    /* col 2 */
	    col0[BCOMP] = (*(const dword *)(code + 11)) >> 6;
	    col0[GCOMP] = CC_SEL(cc, 99);
	    col0[RCOMP] = CC_SEL(cc, 104);
	    col0[ACOMP] = CC_SEL(cc, 119);
	} else {
	    t = (cc[0] >> (t * 2)) & 3;
	    /* col 0 */
	    col0[BCOMP] = CC_SEL(cc, 64);
	    col0[GCOMP] = CC_SEL(cc, 69);
	    col0[RCOMP] = CC_SEL(cc, 74);
	    col0[ACOMP] = CC_SEL(cc, 109);
	}

	if (t == 0) {
	    rgba[BCOMP] = UP5(col0[BCOMP]);
	    rgba[GCOMP] = UP5(col0[GCOMP]);
	    rgba[RCOMP] = UP5(col0[RCOMP]);
	    rgba[ACOMP] = UP5(col0[ACOMP]);
	} else if (t == 3) {
	    rgba[BCOMP] = UP5(CC_SEL(cc, 79));
	    rgba[GCOMP] = UP5(CC_SEL(cc, 84));
	    rgba[RCOMP] = UP5(CC_SEL(cc, 89));
	    rgba[ACOMP] = UP5(CC_SEL(cc, 114));
	} else {
	    rgba[BCOMP] = LERP(3, t, UP5(col0[BCOMP]), UP5(CC_SEL(cc, 79)));
	    rgba[GCOMP] = LERP(3, t, UP5(col0[GCOMP]), UP5(CC_SEL(cc, 84)));
	    rgba[RCOMP] = LERP(3, t, UP5(col0[RCOMP]), UP5(CC_SEL(cc, 89)));
	    rgba[ACOMP] = LERP(3, t, UP5(col0[ACOMP]), UP5(CC_SEL(cc, 114)));
	}
    } else {
	/* lerp == 0 */

	if (t & 16) {
	    cc++;
	    t &= 15;
	}
	t = (cc[0] >> (t * 2)) & 3;

	if (t == 3) {
	    ZERO_4UBV(rgba);
	} else {
	    dword kk;
	    cc = (const dword *)code;
	    rgba[ACOMP] = UP5(cc[3] >> (t * 5 + 13));
	    t *= 15;
	    cc = (const dword *)(code + 8 + t / 8);
	    kk = cc[0] >> (t & 7);
	    rgba[BCOMP] = UP5(kk);
	    rgba[GCOMP] = UP5(kk >> 5);
	    rgba[RCOMP] = UP5(kk >> 10);
	}
    }
}


TAPI void TAPIENTRY
fxt1_decode_1 (const void *texture, int stride,
	       int i, int j, byte *rgba)
{
    static void (*decode_1[]) (const byte *, int, byte *) = {
	fxt1_decode_1HI,	/* cc-high   = "00?" */
	fxt1_decode_1HI,	/* cc-high   = "00?" */
	fxt1_decode_1CHROMA,	/* cc-chroma = "010" */
	fxt1_decode_1ALPHA,	/* alpha     = "011" */
	fxt1_decode_1MIXED,	/* mixed     = "1??" */
	fxt1_decode_1MIXED,	/* mixed     = "1??" */
	fxt1_decode_1MIXED,	/* mixed     = "1??" */
	fxt1_decode_1MIXED	/* mixed     = "1??" */
    };

    const byte *code = (const byte *)texture +
			((j / 4) * (stride / 8) + (i / 8)) * 16;
    int mode = CC_SEL(code, 125);
    int t = i & 7;

    if (t & 4) {
	t += 12;
    }
    t += (j & 3) * 4;

    decode_1[mode](code, t, rgba);

#if VERBOSE
    {
	extern int cc_chroma;
	extern int cc_alpha;
	extern int cc_high;
	extern int cc_mixed;
	static int *cctype[] = {
	    &cc_high,
	    &cc_high,
	    &cc_chroma,
	    &cc_alpha,
	    &cc_mixed,
	    &cc_mixed,
	    &cc_mixed,
	    &cc_mixed
	};
	(*cctype[mode])++;
    }
#endif
}
