/*
 * jidctint.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a slow-but-accurate integer implementation of the
 * inverse DCT (Discrete Cosine Transform).  In the IJG code, this routine
 * must also perform dequantization of the input coefficients.
 *
 * A 2-D IDCT can be done by 1-D IDCT on each column followed by 1-D IDCT
 * on each row (or vice versa, but it's more convenient to emit a row at
 * a time).  Direct algorithms are also available, but they are much more
 * complex and seem not to be any faster when reduced to code.
 *
 * This implementation is based on an algorithm described in
 *   C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
 *   Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 *   Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
 * The primary algorithm described there uses 11 multiplies and 29 adds.
 * We use their alternate method with 12 multiplies and 32 adds.
 * The advantage of this method is that no data path contains more than one
 * multiplication; this allows a very simple and accurate implementation in
 * scaled fixed-point arithmetic, with a minimal number of shifts.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */

#ifdef DCT_ISLOW_SUPPORTED


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/*
 * The poop on this scaling stuff is as follows:
 *
 * Each 1-D IDCT step produces outputs which are a factor of sqrt(N)
 * larger than the true IDCT outputs.  The final outputs are therefore
 * a factor of N larger than desired; since N=8 this can be cured by
 * a simple right shift at the end of the algorithm.  The advantage of
 * this arrangement is that we save two multiplications per 1-D IDCT,
 * because the y0 and y4 inputs need not be divided by sqrt(N).
 *
 * We have to do addition and subtraction of the integer inputs, which
 * is no problem, and multiplication by fractional constants, which is
 * a problem to do in integer arithmetic.  We multiply all the constants
 * by CONST_SCALE and convert them to integer constants (thus retaining
 * CONST_BITS bits of precision in the constants).  After doing a
 * multiplication we have to divide the product by CONST_SCALE, with proper
 * rounding, to produce the correct output.  This division can be done
 * cheaply as a right shift of CONST_BITS bits.  We postpone shifting
 * as long as possible so that partial sums can be added together with
 * full fractional precision.
 *
 * The outputs of the first pass are scaled up by PASS1_BITS bits so that
 * they are represented to better-than-integral precision.  These outputs
 * require BITS_IN_JSAMPLE + PASS1_BITS + 3 bits; this fits in a 16-bit word
 * with the recommended scaling.  (To scale up 12-bit sample data further, an
 * intermediate INT32 array would be needed.)
 *
 * To avoid overflow of the 32-bit intermediate results in pass 2, we must
 * have BITS_IN_JSAMPLE + CONST_BITS + PASS1_BITS <= 26.  Error analysis
 * shows that the values given below are the most effective.
 */

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  13
#define PASS1_BITS  2
#else
#define CONST_BITS  13
#define PASS1_BITS  1		/* lose a little precision to avoid overflow */
#endif

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

#if CONST_BITS == 13
#define FIX_0_298631336  ((INT32)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((INT32)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((INT32)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((INT32)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((INT32)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((INT32)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((INT32)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((INT32)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((INT32)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((INT32)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((INT32)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((INT32)  25172)	/* FIX(3.072711026) */
#else
#define FIX_0_298631336  FIX(0.298631336)
#define FIX_0_390180644  FIX(0.390180644)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_175875602  FIX(1.175875602)
#define FIX_1_501321110  FIX(1.501321110)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_1_961570560  FIX(1.961570560)
#define FIX_2_053119869  FIX(2.053119869)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_072711026  FIX(3.072711026)
#endif


/* Multiply an INT32 variable by an INT32 constant to yield an INT32 result.
 * For 8-bit samples with the recommended scaling, all the variable
 * and constant values involved are no more than 16 bits wide, so a
 * 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
 * For 12-bit samples, a full 32-bit multiplication will be needed.
 */

#if BITS_IN_JSAMPLE == 8
#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY(var,const)  ((var) * (const))
#endif


/* Dequantize a coefficient by multiplying it by the multiplier-table
 * entry; produce an int result.  In this module, both inputs and result
 * are 16 bits or less, so either int or short multiply will work.
 */

#define DEQUANTIZE(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))


/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */

GLOBAL(void)
jpeg_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1, z2, z3, z4, z5;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE2];	/* buffers data between passes */
  SHIFT_TEMPS

  /* Pass 1: process columns from input, store into work array. */
  /* Note results are scaled up by sqrt(8) compared to a true IDCT; */
  /* furthermore, we scale the results by 2**PASS1_BITS. */

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; ctr--) {
    /* Due to quantization, we will usually find that many of the input
     * coefficients are zero, especially the AC terms.  We can exploit this
     * by short-circuiting the IDCT calculation for any column in which all
     * the AC terms are zero.  In that case each output is equal to the
     * DC coefficient (with scale factor as needed).
     * With typical images and quantization tables, half or more of the
     * column DCT calculations can be simplified this way.
     */

    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
	inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] == 0 &&
	inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] == 0 &&
	inptr[DCTSIZE*7] == 0) {
      /* AC terms all zero */
      int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      wsptr[DCTSIZE*4] = dcval;
      wsptr[DCTSIZE*5] = dcval;
      wsptr[DCTSIZE*6] = dcval;
      wsptr[DCTSIZE*7] = dcval;

      inptr++;			/* advance pointers to next column */
      quantptr++;
      wsptr++;
      continue;
    }

    /* Even part: reverse the even part of the forward DCT. */
    /* The rotator is sqrt(2)*c(-6). */

    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

    z2 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);

    tmp0 = (z2 + z3) << CONST_BITS;
    tmp1 = (z2 - z3) << CONST_BITS;

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    /* Odd part per figure 8; the matrix is unitary and hence its
     * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
     */

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    tmp1 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp3 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);

    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

    tmp0 = MULTIPLY(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
    tmp1 = MULTIPLY(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
    tmp2 = MULTIPLY(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
    tmp3 = MULTIPLY(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
    z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
    z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
    z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
    z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

    z3 += z5;
    z4 += z5;

    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;

    /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*7] = (int) DESCALE(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*6] = (int) DESCALE(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*5] = (int) DESCALE(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*3] = (int) DESCALE(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
    wsptr[DCTSIZE*4] = (int) DESCALE(tmp13 - tmp0, CONST_BITS-PASS1_BITS);

    inptr++;			/* advance pointers to next column */
    quantptr++;
    wsptr++;
  }

  /* Pass 2: process rows from work array, store into output array. */
  /* Note that we must descale the results by a factor of 8 == 2**3, */
  /* and also undo the PASS1_BITS scaling. */

  wsptr = workspace;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    outptr = output_buf[ctr] + output_col;
    /* Rows of zeroes can be exploited in the same way as we did with columns.
     * However, the column calculation has created many nonzero AC terms, so
     * the simplification applies less often (typically 5% to 10% of the time).
     * On machines with very fast multiplication, it's possible that the
     * test takes more time than it's worth.  In that case this section
     * may be commented out.
     */

#ifndef NO_ZERO_ROW_TEST
    if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 && wsptr[4] == 0 &&
	wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0) {
      /* AC terms all zero */
      JSAMPLE dcval = range_limit[(int) DESCALE((INT32) wsptr[0], PASS1_BITS+3)
				  & RANGE_MASK];

      outptr[0] = dcval;
      outptr[1] = dcval;
      outptr[2] = dcval;
      outptr[3] = dcval;
      outptr[4] = dcval;
      outptr[5] = dcval;
      outptr[6] = dcval;
      outptr[7] = dcval;

      wsptr += DCTSIZE;		/* advance pointer to next row */
      continue;
    }
#endif

    /* Even part: reverse the even part of the forward DCT. */
    /* The rotator is sqrt(2)*c(-6). */

    z2 = (INT32) wsptr[2];
    z3 = (INT32) wsptr[6];

    z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
    tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
    tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

    tmp0 = ((INT32) wsptr[0] + (INT32) wsptr[4]) << CONST_BITS;
    tmp1 = ((INT32) wsptr[0] - (INT32) wsptr[4]) << CONST_BITS;

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    /* Odd part per figure 8; the matrix is unitary and hence its
     * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
     */

    tmp0 = (INT32) wsptr[7];
    tmp1 = (INT32) wsptr[5];
    tmp2 = (INT32) wsptr[3];
    tmp3 = (INT32) wsptr[1];

    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

    tmp0 = MULTIPLY(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
    tmp1 = MULTIPLY(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
    tmp2 = MULTIPLY(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
    tmp3 = MULTIPLY(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
    z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
    z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
    z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
    z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

    z3 += z5;
    z4 += z5;

    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;

    /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp3,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[7] = range_limit[(int) DESCALE(tmp10 - tmp3,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp11 + tmp2,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[6] = range_limit[(int) DESCALE(tmp11 - tmp2,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE(tmp12 + tmp1,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) DESCALE(tmp12 - tmp1,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE(tmp13 + tmp0,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) DESCALE(tmp13 - tmp0,
					  CONST_BITS+PASS1_BITS+3)
			    & RANGE_MASK];

    wsptr += DCTSIZE;		/* advance pointer to next row */
  }
}


#ifdef HAVE_SSE2_INTEL_MNEMONICS

/*
* Intel SSE2 optimized Inverse Discrete Cosine Transform
*
*
* Copyright (c) 2001-2002 Intel Corporation
* All Rights Reserved
*
*
*  Authors:
*      Danilov G.
*
*
*-----------------------------------------------------------------------------
*
* References:
*    K.R. Rao and P. Yip
*       Discrete Cosine Transform.
*       Algorithms, Advantages, Applications.
*       Academic Press, Inc, London, 1990.
*    JPEG Group's software.
*       This implementation is based on Appendix A.2 of the book (R&Y) ...
*
*-----------------------------------------------------------------------------
*/

typedef unsigned char   Ipp8u;
typedef unsigned short  Ipp16u;
typedef unsigned int    Ipp32u;

typedef signed char    Ipp8s;
typedef signed short   Ipp16s;
typedef signed int     Ipp32s;

#define BITS_INV_ACC  4
#define SHIFT_INV_ROW  16 - BITS_INV_ACC
#define SHIFT_INV_COL 1 + BITS_INV_ACC

#define RND_INV_ROW  1024 * (6 - BITS_INV_ACC)	/* 1 << (SHIFT_INV_ROW-1)		*/
#define RND_INV_COL = 16 * (BITS_INV_ACC - 3)   /* 1 << (SHIFT_INV_COL-1)		*/
#define RND_INV_CORR = RND_INV_COL - 1          /* correction -1.0 and round	*/

#define c_inv_corr_0 -1024 * (6 - BITS_INV_ACC) + 65536		/* -0.5 + (16.0 or 32.0)	*/
#define c_inv_corr_1 1877 * (6 - BITS_INV_ACC)				/* 0.9167	*/
#define c_inv_corr_2 1236 * (6 - BITS_INV_ACC)				/* 0.6035	*/
#define c_inv_corr_3 680  * (6 - BITS_INV_ACC)				/* 0.3322	*/
#define c_inv_corr_4 0    * (6 - BITS_INV_ACC)				/* 0.0		*/
#define c_inv_corr_5 -569  * (6 - BITS_INV_ACC)				/* -0.278	*/
#define c_inv_corr_6 -512  * (6 - BITS_INV_ACC)				/* -0.25	*/
#define c_inv_corr_7 -651  * (6 - BITS_INV_ACC)				/* -0.3176	*/

#define RND_INV_ROW_0 RND_INV_ROW + c_inv_corr_0
#define RND_INV_ROW_1 RND_INV_ROW + c_inv_corr_1
#define RND_INV_ROW_2 RND_INV_ROW + c_inv_corr_2
#define RND_INV_ROW_3 RND_INV_ROW + c_inv_corr_3
#define RND_INV_ROW_4 RND_INV_ROW + c_inv_corr_4
#define RND_INV_ROW_5 RND_INV_ROW + c_inv_corr_5
#define RND_INV_ROW_6 RND_INV_ROW + c_inv_corr_6
#define RND_INV_ROW_7 RND_INV_ROW + c_inv_corr_7

/* Table for rows 0,4 - constants are multiplied on cos_4_16 */

__declspec(align(16)) short tab_i_04[] = {
	16384, 21407, 16384, 8867,
	-16384, 21407, 16384, -8867,
	16384,  -8867,  16384, -21407,
    16384,   8867, -16384, -21407,
    22725,  19266,  19266,  -4520,
    4520,  19266,  19266, -22725,
    12873, -22725,   4520, -12873,
    12873,   4520, -22725, -12873};

/* Table for rows 1,7 - constants are multiplied on cos_1_16 */

__declspec(align(16)) short tab_i_17[] = {
	22725,  29692,  22725,  12299,
    -22725,  29692,  22725, -12299,
    22725, -12299,  22725, -29692,
    22725,  12299, -22725, -29692,
    31521,  26722,  26722,  -6270,
    6270,  26722,  26722, -31521,
    17855, -31521,   6270, -17855,
    17855,   6270, -31521, -17855};

/* Table for rows 2,6 - constants are multiplied on cos_2_16 */

__declspec(align(16)) short tab_i_26[] = {
	21407,  27969,  21407,  11585,
    -21407,  27969,  21407, -11585,
    21407, -11585,  21407, -27969,
    21407,  11585, -21407, -27969,
    29692,  25172,  25172,  -5906,
    5906,  25172,  25172, -29692,
    16819, -29692,   5906, -16819,
    16819,   5906, -29692, -16819};

/* Table for rows 3,5 - constants are multiplied on cos_3_16 */

__declspec(align(16)) short tab_i_35[] = {
	19266,  25172,  19266,  10426,
    -19266,  25172,  19266, -10426,
    19266, -10426,  19266, -25172,
    19266,  10426, -19266, -25172,
    26722,  22654,  22654,  -5315,
    5315,  22654,  22654, -26722,
    15137, -26722,   5315, -15137,
    15137,   5315, -26722, -15137};

__declspec(align(16)) long round_i_0[] = {RND_INV_ROW_0,RND_INV_ROW_0,
	RND_INV_ROW_0,RND_INV_ROW_0};
__declspec(align(16)) long round_i_1[] = {RND_INV_ROW_1,RND_INV_ROW_1,
	RND_INV_ROW_1,RND_INV_ROW_1};
__declspec(align(16)) long round_i_2[] = {RND_INV_ROW_2,RND_INV_ROW_2,
	RND_INV_ROW_2,RND_INV_ROW_2};
__declspec(align(16)) long round_i_3[] = {RND_INV_ROW_3,RND_INV_ROW_3,
	RND_INV_ROW_3,RND_INV_ROW_3};
__declspec(align(16)) long round_i_4[] = {RND_INV_ROW_4,RND_INV_ROW_4,
	RND_INV_ROW_4,RND_INV_ROW_4};
__declspec(align(16)) long round_i_5[] = {RND_INV_ROW_5,RND_INV_ROW_5,
	RND_INV_ROW_5,RND_INV_ROW_5};
__declspec(align(16)) long round_i_6[] = {RND_INV_ROW_6,RND_INV_ROW_6,
	RND_INV_ROW_6,RND_INV_ROW_6};
__declspec(align(16)) long round_i_7[] = {RND_INV_ROW_7,RND_INV_ROW_7,
	RND_INV_ROW_7,RND_INV_ROW_7};

__declspec(align(16)) short tg_1_16[] = {
	13036,  13036,  13036,  13036,	/* tg * (2<<16) + 0.5 */
	13036,  13036,  13036,  13036};
__declspec(align(16)) short tg_2_16[] = {
	27146,  27146,  27146,  27146,	/* tg * (2<<16) + 0.5 */
	27146,  27146,  27146,  27146};
__declspec(align(16)) short tg_3_16[] = {
	-21746, -21746, -21746, -21746,	/* tg * (2<<16) + 0.5 */
	-21746, -21746, -21746, -21746};
__declspec(align(16)) short cos_4_16[] = {
	-19195, -19195, -19195, -19195,	/* cos * (2<<16) + 0.5 */
	-19195, -19195, -19195, -19195};

/*
* In this implementation the outputs of the iDCT-1D are multiplied
*    for rows 0,4 - on cos_4_16,
*    for rows 1,7 - on cos_1_16,
*    for rows 2,6 - on cos_2_16,
*    for rows 3,5 - on cos_3_16
* and are shifted to the left for rise of accuracy
*
* For used constants
*    FIX(float_const) = (short) (float_const * (1<<15) + 0.5)
*
*-----------------------------------------------------------------------------
*
* On the first stage the calculation is executed at once for two rows.
* The permutation for each output row is done on second stage
*    t7 t6 t5 t4 t3 t2 t1 t0 -> t4 t5 t6 t7 t3 t2 t1 t0
*
*-----------------------------------------------------------------------------
*/

#define DCT_8_INV_ROW_2R(TABLE, ROUND1, ROUND2) __asm {	\
	__asm pshuflw  xmm1, xmm0, 10001000b				\
    __asm pshuflw  xmm0, xmm0, 11011101b    			\
    __asm pshufhw  xmm1, xmm1, 10001000b    			\
	__asm pshufhw  xmm0, xmm0, 11011101b				\
	__asm movdqa   xmm2, XMMWORD PTR [TABLE]			\
	__asm pmaddwd  xmm2, xmm1							\
	__asm movdqa   xmm3, XMMWORD PTR [TABLE + 32]		\
	__asm pmaddwd  xmm3, xmm0               			\
	__asm pmaddwd  xmm1, XMMWORD PTR [TABLE + 16]		\
	__asm pmaddwd  xmm0, XMMWORD PTR [TABLE + 48]		\
	__asm pshuflw  xmm5, xmm4, 10001000b				\
	__asm pshuflw  xmm4, xmm4, 11011101b    			\
	__asm pshufhw  xmm5, xmm5, 10001000b    			\
	__asm pshufhw  xmm4, xmm4, 11011101b    			\
	__asm movdqa   xmm6, XMMWORD PTR [TABLE]			\
	__asm pmaddwd  xmm6, xmm5               			\
	__asm movdqa   xmm7, XMMWORD PTR [TABLE + 32]		\
	__asm pmaddwd  xmm7, xmm4               			\
	__asm pmaddwd  xmm5, XMMWORD PTR [TABLE + 16]		\
	__asm pmaddwd  xmm4, XMMWORD PTR [TABLE + 48]		\
	__asm pshufd   xmm1, xmm1, 01001110b    			\
	__asm pshufd   xmm0, xmm0, 01001110b    			\
	__asm paddd    xmm2, XMMWORD PTR [ROUND1]			\
	__asm paddd    xmm3, xmm0							\
	__asm paddd    xmm1, xmm2							\
	__asm pshufd   xmm5, xmm5, 01001110b    			\
	__asm pshufd   xmm4, xmm4, 01001110b    			\
	__asm movdqa   xmm2, xmm1             				\
	__asm psubd    xmm2, xmm3             				\
	__asm psrad    xmm2, SHIFT_INV_ROW    				\
	__asm paddd    xmm1, xmm3							\
	__asm psrad    xmm1, SHIFT_INV_ROW      			\
	__asm packssdw xmm1, xmm2							\
	__asm paddd    xmm6, XMMWORD PTR [ROUND2]			\
	__asm paddd    xmm7, xmm4							\
	__asm paddd    xmm5, xmm6							\
	__asm movdqa   xmm6, xmm5	            			\
	__asm psubd    xmm6, xmm7               			\
	__asm psrad    xmm6, SHIFT_INV_ROW      			\
	__asm paddd    xmm5, xmm7							\
	__asm psrad    xmm5, SHIFT_INV_ROW      			\
	__asm packssdw xmm5, xmm6							\
	}

/*
*
* The second stage - inverse DCTs of columns
*
* The inputs are multiplied
*    for rows 0,4 - on cos_4_16,
*    for rows 1,7 - on cos_1_16,
*    for rows 2,6 - on cos_2_16,
*    for rows 3,5 - on cos_3_16
* and are shifted to the left for rise of accuracy
*/

#define DCT_8_INV_COL_8R(INP, OUTP) __asm {		\
	__asm movdqa   xmm0, [INP + 5*16]			\
    __asm movdqa   xmm1, XMMWORD PTR tg_3_16	\
    __asm movdqa   xmm2, xmm0            		\
    __asm movdqa   xmm3, [INP + 3*16]   		\
    __asm pmulhw   xmm0, xmm1           		\
    __asm movdqa   xmm4, [INP + 7*16]   		\
    __asm pmulhw   xmm1, xmm3           		\
    __asm movdqa   xmm5, XMMWORD PTR tg_1_16   	\
    __asm movdqa   xmm6, xmm4            		\
    __asm pmulhw   xmm4, xmm5           		\
    __asm paddsw   xmm0, xmm2           		\
    __asm pmulhw   xmm5, [INP + 1*16]   		\
    __asm paddsw   xmm1, xmm3           		\
    __asm movdqa   xmm7, [INP + 6*16]    		\
    __asm paddsw   xmm0, xmm3					\
    __asm movdqa   xmm3, XMMWORD PTR tg_2_16	\
    __asm psubsw   xmm2, xmm1					\
    __asm pmulhw   xmm7, xmm3            		\
    __asm movdqa   xmm1, xmm0            		\
    __asm pmulhw   xmm3, [INP + 2*16]   		\
    __asm psubsw   xmm5, xmm6					\
    __asm paddsw   xmm4, [INP + 1*16]    		\
    __asm paddsw   xmm0, xmm4            		\
    __asm psubsw   xmm4, xmm1					\
    __asm pshufhw  xmm0, xmm0, 00011011b		\
    __asm paddsw   xmm7, [INP + 2*16]    		\
    __asm movdqa   xmm6, xmm5					\
    __asm psubsw   xmm3, [INP + 6*16]    		\
    __asm psubsw   xmm5, xmm2            		\
    __asm paddsw   xmm6, xmm2					\
	__asm movdqa   [OUTP + 7*16], xmm0    		\
    __asm movdqa   xmm1, xmm4            		\
    __asm movdqa   xmm2, XMMWORD PTR cos_4_16  	\
    __asm paddsw   xmm4, xmm5            		\
    __asm movdqa   xmm0, XMMWORD PTR cos_4_16  	\
    __asm pmulhw   xmm2, xmm4					\
    __asm pshufhw  xmm6, xmm6, 00011011b		\
    __asm movdqa   [OUTP + 3*16], xmm6    		\
    __asm psubsw   xmm1, xmm5            		\
    __asm movdqa   xmm6, [INP + 0*16]   		\
    __asm pmulhw   xmm0, xmm1					\
    __asm movdqa   xmm5, [INP + 4*16]    		\
    __asm paddsw   xmm4, xmm2					\
    __asm paddsw   xmm5, xmm6       			\
    __asm psubsw   xmm6, [INP + 4*16]   		\
    __asm paddsw   xmm0, xmm1					\
    __asm pshufhw  xmm4, xmm4, 00011011b		\
    __asm movdqa   xmm2, xmm5            		\
    __asm paddsw   xmm5, xmm7            		\
    __asm movdqa   xmm1, xmm6					\
    __asm psubsw   xmm2, xmm7					\
    __asm movdqa   xmm7, [OUTP + 7*16]    		\
    __asm paddsw   xmm6, xmm3            		\
    __asm pshufhw  xmm5, xmm5, 00011011b		\
	__asm paddsw   xmm7, xmm5					\
    __asm psubsw   xmm1, xmm3					\
    __asm pshufhw  xmm6, xmm6, 00011011b		\
	__asm movdqa   xmm3, xmm6					\
    __asm paddsw   xmm6, xmm4            		\
    __asm pshufhw  xmm2, xmm2, 00011011b		\
    __asm psraw    xmm7, SHIFT_INV_COL   		\
    __asm movdqa   [OUTP + 0*16], xmm7    		\
    __asm movdqa   xmm7, xmm1            		\
    __asm paddsw   xmm1, xmm0					\
    __asm psraw    xmm6, SHIFT_INV_COL			\
    __asm movdqa   [OUTP + 1*16], xmm6    		\
    __asm pshufhw  xmm1, xmm1, 00011011b		\
	__asm movdqa   xmm6, [OUTP + 3*16]			\
    __asm psubsw   xmm7, xmm0            		\
    __asm psraw    xmm1, SHIFT_INV_COL   		\
    __asm movdqa   [OUTP + 2*16], xmm1    		\
    __asm psubsw   xmm5, [OUTP + 7*16]			\
    __asm paddsw   xmm6, xmm2            		\
    __asm psubsw   xmm2, [OUTP + 3*16]			\
    __asm psubsw   xmm3, xmm4            		\
    __asm psraw    xmm7, SHIFT_INV_COL  		\
    __asm pshufhw  xmm7, xmm7, 00011011b		\
    __asm movdqa   [OUTP + 5*16], xmm7    		\
    __asm psraw    xmm5, SHIFT_INV_COL			\
    __asm movdqa   [OUTP + 7*16], xmm5    		\
    __asm psraw    xmm6, SHIFT_INV_COL			\
    __asm movdqa   [OUTP + 3*16], xmm6    		\
    __asm psraw    xmm2, SHIFT_INV_COL			\
    __asm movdqa   [OUTP + 4*16], xmm2    		\
    __asm psraw    xmm3, SHIFT_INV_COL			\
    __asm movdqa   [OUTP + 6*16], xmm3    		\
	}

/*
*
*  Name:      dct_8x8_inv_16s
*  Purpose:   Inverse Discrete Cosine Transform 8x8 with
*             2D buffer of short int data
*  Context:
*      void dct_8x8_inv_16s ( short *src, short *dst )
*  Parameters:
*      src  - Pointer to the source buffer
*      dst  - Pointer to the destination buffer
*
*/

GLOBAL(void)
dct_8x8_inv_16s ( short *src, short *dst ) {

	__asm {

		mov     ecx,  src
		mov     edx,  dst

		movdqa  xmm0, [ecx+0*16]
		movdqa  xmm4, [ecx+4*16]
		DCT_8_INV_ROW_2R(tab_i_04, round_i_0, round_i_4)
		movdqa     [edx+0*16], xmm1
		movdqa     [edx+4*16], xmm5

		movdqa  xmm0, [ecx+1*16]
		movdqa  xmm4, [ecx+7*16]
		DCT_8_INV_ROW_2R(tab_i_17, round_i_1, round_i_7)
		movdqa     [edx+1*16], xmm1
		movdqa     [edx+7*16], xmm5

		movdqa  xmm0, [ecx+3*16]
		movdqa  xmm4, [ecx+5*16]
		DCT_8_INV_ROW_2R(tab_i_35, round_i_3, round_i_5);
		movdqa     [edx+3*16], xmm1
		movdqa     [edx+5*16], xmm5

		movdqa  xmm0, [ecx+2*16]
		movdqa  xmm4, [ecx+6*16]
		DCT_8_INV_ROW_2R(tab_i_26, round_i_2, round_i_6);
		movdqa     [edx+2*16], xmm1
		movdqa     [edx+6*16], xmm5

		DCT_8_INV_COL_8R(edx+0, edx+0);
	}
}


/*
*  Name:
*    ownpj_QuantInv_8x8_16s
*
*  Purpose:
*    Dequantize 8x8 block of DCT coefficients
*
*  Context:
*    void ownpj_QuantInv_8x8_16s
*            Ipp16s*  pSrc,
*            Ipp16s*  pDst,
*      const Ipp16u*  pQTbl)*
*
*/

GLOBAL(void)
ownpj_QuantInv_8x8_16s(short * pSrc, short * pDst, const unsigned short * pQTbl)
{
	__asm {

		push        ebx
		push        ecx
		push        edx
		push        esi
		push        edi

		mov         esi, pSrc
		mov         edi, pDst
		mov         edx, pQTbl
		mov         ecx, 4
		mov         ebx, 32

	again:

		movq        mm0, QWORD PTR [esi+0]
		movq        mm1, QWORD PTR [esi+8]
		movq        mm2, QWORD PTR [esi+16]
		movq        mm3, QWORD PTR [esi+24]

		prefetcht0  [esi+ebx] ; fetch next cache line

		pmullw      mm0, QWORD PTR [edx+0]
		pmullw      mm1, QWORD PTR [edx+8]
		pmullw      mm2, QWORD PTR [edx+16]
		pmullw      mm3, QWORD PTR [edx+24]

		movq        QWORD PTR [edi+0], mm0
		movq        QWORD PTR [edi+8], mm1
		movq        QWORD PTR [edi+16], mm2
		movq        QWORD PTR [edi+24], mm3

		add         esi, ebx
		add         edi, ebx
		add         edx, ebx
		dec         ecx
		jnz         again

		emms

		pop         edi
		pop         esi
		pop         edx
		pop         ecx
		pop         ebx
	}
}


/*
*  Name:
*    ownpj_Add128_8x8_16s8u
*
*  Purpose:
*    signed to unsigned conversion (level shift)
*    for 8x8 block of DCT coefficients
*
*  Context:
*    void ownpj_Add128_8x8_16s8u
*      const Ipp16s* pSrc,
*            Ipp8u*  pDst,
*            int     DstStep);
*
*/

__declspec(align(16)) long const_128[]= {0x00800080, 0x00800080, 0x00800080, 0x00800080};

GLOBAL(void)
ownpj_Add128_8x8_16s8u(const short * pSrc, unsigned char * pDst, int DstStep)
{
	__asm {
		push        eax
		push        ebx
		push        ecx
		push        edx
		push        esi
		push        edi

		mov         esi, pSrc
		mov         edi, pDst
		mov         edx, DstStep
		mov         ecx, 2
		mov         ebx, edx
		mov         eax, edx
		sal         ebx, 1
		add         eax, ebx
		movdqa      xmm7, XMMWORD PTR const_128

	again:

		movdqa      xmm0, XMMWORD PTR [esi+0]  ; line 0
		movdqa      xmm1, XMMWORD PTR [esi+16] ; line 1
		movdqa      xmm2, XMMWORD PTR [esi+32] ; line 2
		movdqa      xmm3, XMMWORD PTR [esi+48] ; line 3

		paddw     xmm0, xmm7
		paddw     xmm1, xmm7
		paddw     xmm2, xmm7
		paddw     xmm3, xmm7

		packuswb  xmm0, xmm1
		packuswb  xmm2, xmm3

		movq      QWORD PTR [edi], xmm0      ;0*DstStep
		movq      QWORD PTR [edi+ebx], xmm2  ;2*DstStep

		psrldq      xmm0, 8
		psrldq      xmm2, 8

		movq      QWORD PTR [edi+edx], xmm0  ;1*DstStep
		movq      QWORD PTR [edi+eax], xmm2  ;3*DstStep

		add         edi, ebx
		add         esi, 64
		add         edi, ebx
		dec         ecx
		jnz         again

		pop         edi
		pop         esi
		pop         edx
		pop         ecx
		pop         ebx
		pop         eax
	}
}


/*
*  Name:
*    ippiDCTQuantInv8x8LS_JPEG_16s8u_C1R
*
*  Purpose:
*    Inverse DCT transform, de-quantization and level shift
*
*  Parameters:
*    pSrc               - pointer to source
*    pDst               - pointer to output array
*    DstStep            - line offset for output data
*    pEncoderQuantTable - pointer to Quantization table
*
*/

GLOBAL(void)
ippiDCTQuantInv8x8LS_JPEG_16s8u_C1R(
  short * pSrc,
  unsigned char *  pDst,
  int     DstStep,
  const unsigned short * pQuantInvTable)
{

	__declspec(align(16)) Ipp8u buf[DCTSIZE2*sizeof(Ipp16s)];
	Ipp16s * workbuf = (Ipp16s *)buf;

	ownpj_QuantInv_8x8_16s(pSrc,workbuf,pQuantInvTable);
	dct_8x8_inv_16s(workbuf,workbuf);
	ownpj_Add128_8x8_16s8u(workbuf,pDst,DstStep);

}

GLOBAL(void)
jpeg_idct_islow_sse2 (
	j_decompress_ptr cinfo,
	jpeg_component_info * compptr,
	JCOEFPTR coef_block,
	JSAMPARRAY output_buf,
	JDIMENSION output_col)
{
	int			ctr;
	JCOEFPTR	inptr;
	Ipp16u*		quantptr;
	Ipp8u*		wsptr;
	__declspec(align(16)) Ipp8u workspace[DCTSIZE2];
	JSAMPROW	outptr;

	inptr = coef_block;
	quantptr = (Ipp16u*)compptr->dct_table;
	wsptr = workspace;

	ippiDCTQuantInv8x8LS_JPEG_16s8u_C1R(inptr, workspace, 8, quantptr);

	for(ctr = 0; ctr < DCTSIZE; ctr++)
	{
		outptr = output_buf[ctr] + output_col;

		outptr[0] = wsptr[0];
		outptr[1] = wsptr[1];
		outptr[2] = wsptr[2];
		outptr[3] = wsptr[3];
		outptr[4] = wsptr[4];
		outptr[5] = wsptr[5];
		outptr[6] = wsptr[6];
		outptr[7] = wsptr[7];

		wsptr += DCTSIZE;
	}
}
#endif /* HAVE_SSE2_INTEL_MNEMONICS */

#endif /* DCT_ISLOW_SUPPORTED */
