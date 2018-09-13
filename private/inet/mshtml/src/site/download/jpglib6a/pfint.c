/***************************************************************************
*
*                INTEL Corporation Proprietary Information  
*
*      
*                  Copyright (c) 1996 Intel Corporation.
*                         All rights reserved.
*
***************************************************************************
*/
/*
 * jfdctint.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a slow-but-accurate integer implementation of the
 * forward DCT (Discrete Cosine Transform).
 *
 * A 2-D DCT can be done by 1-D DCT on each row followed by 1-D DCT
 * on each column.  Direct algorithms are also available, but they are
 * much more complex and seem not to be any faster when reduced to code.
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

#ifndef USEINLINEASM

GLOBAL(void)
pfdct8x8llm (DCTELEM * data)
{
}

#else


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/*
 * The poop on this scaling stuff is as follows:
 *
 * Each 1-D DCT step produces outputs which are a factor of sqrt(N)
 * larger than the true DCT outputs.  The final outputs are therefore
 * a factor of N larger than desired; since N=8 this can be cured by
 * a simple right shift at the end of the algorithm.  The advantage of
 * this arrangement is that we save two multiplications per 1-D DCT,
 * because the y0 and y4 outputs need not be divided by sqrt(N).
 * In the IJG code, this factor of 8 is removed by the quantization step
 * (in jcdctmgr.c), NOT in this module.
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
 * with the recommended scaling.  (For 12-bit sample data, the intermediate
 * array is INT32 anyway.)
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
#define FIX_0_298631336				2446		/* FIX(0.298631336) */
#define FIX_0_390180644				3196		/* FIX(0.390180644) */
#define FIX_0_541196100				4433		/* FIX(0.541196100) */
#define FIX_0_765366865				6270		/* FIX(0.765366865) */
#define FIX_0_899976223				7373		/* FIX(0.899976223) */
#define FIX_1_175875602				9633		/* FIX(1.175875602) */
#define FIX_1_501321110				12299		/* FIX(1.501321110) */
#define FIX_1_847759065				15137		/* FIX(1.847759065) */
#define FIX_1_961570560				16069		/* FIX(1.961570560) */
#define FIX_2_053119869				16819		/* FIX(2.053119869) */
#define FIX_2_562915447				20995		/* FIX(2.562915447) */
#define FIX_3_072711026				25172		/* FIX(3.072711026) */
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

#define	DATASIZE	4
#define	DCTWIDTH	32

/*
 * Perform the forward DCT on one block of samples.
 */

GLOBAL(void)
pfdct8x8llm (DCTELEM * data)
{
  INT32 tmp4, tmp5, tmp6, tmp7;
  int counter;

  __asm{
 
  /* Pass 1: process rows. */
  /* Note results are scaled up by sqrt(8) compared to a true DCT; */
  /* furthermore, we scale the results by 2**PASS1_BITS. */

//  dataptr = data;
		mov 	esi, [data]
		mov		counter, 8
		
//  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
//   tmp0 = dataptr[0] + dataptr[7];
//   tmp7 = dataptr[0] - dataptr[7];
//    tmp1 = dataptr[1] + dataptr[6];
//    tmp6 = dataptr[1] - dataptr[6];
//    tmp2 = dataptr[2] + dataptr[5];
//    tmp5 = dataptr[2] - dataptr[5];
//    tmp3 = dataptr[3] + dataptr[4];
//    tmp4 = dataptr[3] - dataptr[4];
    
 StartRow:
 		mov		eax, [esi][DATASIZE*0]
 		mov		ebx, [esi][DATASIZE*7]
 		
 		mov		edx, eax
 		add		eax, ebx	; eax = tmp0
 		
 		sub		edx, ebx	; edx = tmp7
  		mov		ebx, [esi][DATASIZE*3]
 
 		mov		ecx, [esi][DATASIZE*4]
 		mov		edi, ebx
 		
 		add		ebx, ecx	; ebx = tmp3
 		sub		edi, ecx	; edi = tmp4
 		
 		mov		tmp4, edi
 		mov		tmp7, edx
 		
    /* Even part per LL&M figure 1 --- note that published figure is faulty;
     * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
     */
    
//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;
    
		mov		ecx, eax
		add		eax, ebx	; eax = tmp10
		
		sub		ecx, ebx	; ecx = tmp13
  		mov		edx, [esi][DATASIZE*1] 
  		
  		mov		edi, [esi][DATASIZE*6]
  		mov		ebx, edx
  		
  		add		edx, edi	; edx = tmp1
  		sub		ebx, edi	; ebx = tmp6
  		
  		mov		tmp6, ebx
  		push	ebp
  		
  		mov		edi, [esi][DATASIZE*2]
  		mov		ebp, [esi][DATASIZE*5]

  		mov		ebx, edi
  		add		edi, ebp	; edi = tmp2
  		
  		sub		ebx, ebp	; ebx = tmp5
  		mov		ebp, edx
  		
  		add		edx, edi	; edx = tmp11
  		sub		ebp, edi	; ebp = tmp12
  		
//    dataptr[0] = (DCTELEM) ((tmp10 + tmp11) << PASS1_BITS);
//    dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << PASS1_BITS);
    
		mov		edi, eax
		add		eax, edx	; eax = tmp10 + tmp11
		
		shl		eax, 2
		sub		edi, edx	; edi = tmp10 - tmp11

		shl		edi, 2
		mov		[esi][DATASIZE*0], eax
		
		mov		[esi][DATASIZE*4], edi
		mov		eax, ebp	; eax = tmp12
		
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);

		add		ebp, ecx	; eax = tmp12 + tmp13
		add		esi, 32
		
		imul	ebp, FIX_0_541196100	; ebp = z1
		
//    dataptr[2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
//				   CONST_BITS-PASS1_BITS);

		imul	ecx, FIX_0_765366865
		
//    dataptr[6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
//				   CONST_BITS-PASS1_BITS);
    
		imul	eax, FIX_1_847759065
		
		add		ecx, ebp		; add z1
		xor		eax, 0xFFFFFFFF
		
		add		ecx, 1024		; rounding adj
		inc		eax				; negate the result
		
		add		eax, ebp		; add z1
		pop		ebp
		
		sar		ecx, 11
		add		eax, 1024

		mov		[esi][DATASIZE*2-32], ecx
		mov		edi, tmp4
		
		sar		eax, 11
		mov		ecx, tmp6

		mov		[esi][DATASIZE*6-32], eax
		push	esi
		
    /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
     * cK represents cos(K*pi/16).
     * i0..i3 in the paper are tmp4..tmp7 here.
     */
    
//    z1 = tmp4 + tmp7;
//    z2 = tmp5 + tmp6;
//    z3 = tmp4 + tmp6;
//    z4 = tmp5 + tmp7;

		mov		edx, tmp7
		mov		eax, edi	; edi = eax = tmp4
		
		mov		esi, edi	; esi = tmp4
		add		edi, edx	; edi = z1

		add		eax, ecx	; eax = z3
		add		ecx, ebx	; ecx = z2
		
//    z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
//    z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */

		imul	edi, FIX_0_899976223

		imul	ecx, FIX_2_562915447

		xor		ecx, 0xFFFFFFFF
		add		edx, ebx	; edx = z4

//    tmp4 = MULTIPLY(tmp4, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
//    tmp5 = MULTIPLY(tmp5, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */

		imul	esi, FIX_0_298631336

		imul	ebx, FIX_2_053119869

		xor		edi, 0xFFFFFFFF
		inc		ecx			; ecx = z2

		inc		edi			; edi = z1
		add		ebx, ecx	; ebx = z2 + tmp5

		add		esi, edi	; esi = z1 + tmp4
		mov		tmp5, ebx

//    z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

		mov		ebx, eax	; ebx = z3
		add		eax, edx	; eax = z3 + z4

		imul	eax, FIX_1_175875602

//    z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
//    z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */
    
		imul	ebx, FIX_1_961570560

		imul	edx, FIX_0_390180644

		xor		ebx, 0xFFFFFFFF
		xor		edx, 0xFFFFFFFF

		inc		ebx		; ebx = z3
		inc		edx		; edx = z4

//    z3 += z5;
//    z4 += z5;

		add		ebx, eax	; ebx = z3
		add		edx, eax	; edx = z4
    
//    tmp6 = MULTIPLY(tmp6, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
//    tmp7 = MULTIPLY(tmp7, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */

		mov		eax, tmp6
		add		ecx, ebx	; ecx = z2 + z3

		imul	eax, FIX_3_072711026

		add		ecx, eax	; ecx = tmp6 + z2 + z3
		mov		eax, tmp7

		imul	eax, FIX_1_501321110

//    dataptr[7] = (DCTELEM) DESCALE(tmp4 + z1 + z3, CONST_BITS-PASS1_BITS);
//    dataptr[5] = (DCTELEM) DESCALE(tmp5 + z2 + z4, CONST_BITS-PASS1_BITS);
//    dataptr[3] = (DCTELEM) DESCALE(tmp6 + z2 + z3, CONST_BITS-PASS1_BITS);
//    dataptr[1] = (DCTELEM) DESCALE(tmp7 + z1 + z4, CONST_BITS-PASS1_BITS);

		add		edi, edx	; edi = z1 + z4
		add		ecx, 1024
		
		add		edi, eax	; edi = tmp7 + z1 + z4
		mov		eax, tmp5	; eax = tmp5 + z2

		add		ebx, esi	; ebx = tmp4 + z1 + z3
		add		edx, eax	; edx = tmp5 + z2 + z4

		sar		ecx, 11
		add		ebx, 1024

		sar		ebx, 11
		pop		esi

		add		edx, 1024
		add		edi, 1024

		sar		edx, 11
		mov		[esi][DATASIZE*7-32], ebx

		sar		edi, 11
		mov		[esi][DATASIZE*3-32], ecx

		mov		[esi][DATASIZE*5-32], edx
		mov		ecx, counter

		mov		[esi][DATASIZE*1-32], edi
		dec		ecx

		mov		counter, ecx
		jnz		StartRow
    
//    dataptr += DCTSIZE;		/* advance pointer to next row */
//  }

  
  
  
  /* Pass 2: process columns.
   * We remove the PASS1_BITS scaling, but leave the results scaled up
   * by an overall factor of 8.
   */


//  dataptr = data;
		mov 	esi, [data]

		mov		counter, 8
    
//for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
//    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
//    tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
//    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
//    tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
//    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
//    tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
//    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
//    tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

 StartCol:
 		mov		eax, [esi][DCTWIDTH*0]
 		mov		ebx, [esi][DCTWIDTH*7]
 		
 		mov		edx, eax
 		add		eax, ebx	; eax = tmp0
 		
 		sub		edx, ebx	; edx = tmp7
  		mov		ebx, [esi][DCTWIDTH*3]
 
 		mov		ecx, [esi][DCTWIDTH*4]
 		mov		edi, ebx
 		
 		add		ebx, ecx	; ebx = tmp3
 		sub		edi, ecx	; edi = tmp4
 		
 		mov		tmp4, edi
 		mov		tmp7, edx

    /* Even part per LL&M figure 1 --- note that published figure is faulty;
     * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
     */
    
//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;

		mov		ecx, eax	; ecx = tmp0
		add		eax, ebx	; eax = tmp10
		
		sub		ecx, ebx	; ecx = tmp13
  		mov		edx, [esi][DCTWIDTH*1] 
  		
  		mov		edi, [esi][DCTWIDTH*6]
  		mov		ebx, edx
  		
  		add		edx, edi	; edx = tmp1
  		sub		ebx, edi	; ebx = tmp6
  		
  		mov		tmp6, ebx
  		push	ebp
  		
  		mov		edi, [esi][DCTWIDTH*2]
  		mov		ebp, [esi][DCTWIDTH*5]

  		mov		ebx, edi
  		add		edi, ebp	; edi = tmp2
  		
  		sub		ebx, ebp	; ebx = tmp5
		mov		ebp, edx	; ebp = tmp1
		
  		add		edx, edi	; edx = tmp11
 		sub		ebp, edi	; ebx = tmp12

//    dataptr[DCTSIZE*0] = (DCTELEM) DESCALE(tmp10 + tmp11, PASS1_BITS);
//    dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(tmp10 - tmp11, PASS1_BITS);

 		add		eax, 2			; adj for rounding

		mov		edi, eax
		add		eax, edx	; eax = tmp10 + tmp11
		
		sar		eax, 2
		sub		edi, edx	; edi = tmp10 - tmp11

		sar		edi, 2
		mov		[esi][DCTWIDTH*0], eax
		
		mov		[esi][DCTWIDTH*4], edi
		mov		eax, ebp	; eax = tmp12
		
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);

		add		ebp, ecx	; eax = tmp12 + tmp13
		add		esi, 4
		
		imul	ebp, FIX_0_541196100	; ebp = z1
    
//    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
//					   CONST_BITS+PASS1_BITS);

		imul	ecx, FIX_0_765366865
		
//    dataptr[DCTSIZE*6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
//					   CONST_BITS+PASS1_BITS);
    
		imul	eax, FIX_1_847759065
		
		add		ecx, ebp		; add z1
		xor		eax, 0xFFFFFFFF
		
		add		ecx, 16384		; rounding adj
		inc		eax				; negate the result
		
		add		eax, ebp		; add z1
		pop		ebp
		
		sar		ecx, 15
		add		eax, 16384

		mov		[esi][DCTWIDTH*2-4], ecx
		mov		edi, tmp4
		
		sar		eax, 15
		mov		ecx, tmp6

		mov		[esi][DCTWIDTH*6-4], eax
		push	esi

    /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
     * cK represents cos(K*pi/16).
     * i0..i3 in the paper are tmp4..tmp7 here.
     */
    
//    z1 = tmp4 + tmp7;
//    z2 = tmp5 + tmp6;
//    z3 = tmp4 + tmp6;
//    z4 = tmp5 + tmp7;

		mov		edx, tmp7
		mov		eax, edi	; edi = eax = tmp4
		
		mov		esi, edi	; esi = tmp4
		add		edi, edx	; edi = z1

		add		eax, ecx	; eax = z3
		add		ecx, ebx	; ecx = z2

//    z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
//    z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */

		imul	edi, FIX_0_899976223

		imul	ecx, FIX_2_562915447

		xor		ecx, 0xFFFFFFFF
		add		edx, ebx	; edx = z4

//    tmp4 = MULTIPLY(tmp4, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
//    tmp5 = MULTIPLY(tmp5, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */

		imul	esi, FIX_0_298631336

		imul	ebx, FIX_2_053119869

		xor		edi, 0xFFFFFFFF
		inc		ecx			; ecx = z2

		inc		edi			; edi = z1
		add		ebx, ecx	; ebx = z2 + tmp5

		add		esi, edi	; esi = z1 + tmp4
		mov		tmp5, ebx

//    z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */
    
		mov		ebx, eax	; ebx = z3
		add		eax, edx	; eax = z3 + z4

		imul	eax, FIX_1_175875602

//    z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
//    z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */
    
		imul	ebx, FIX_1_961570560

		imul	edx, FIX_0_390180644

		xor		ebx, 0xFFFFFFFF
		xor		edx, 0xFFFFFFFF

		inc		ebx		; ebx = z3
		inc		edx		; edx = z4

//    z3 += z5;
//    z4 += z5;

		add		ebx, eax	; ebx = z3
		add		edx, eax	; edx = z4
    
//    tmp6 = MULTIPLY(tmp6, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
//    tmp7 = MULTIPLY(tmp7, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */

		mov		eax, tmp6
		add		ecx, ebx	; ecx = z2 + z3

		imul	eax, FIX_3_072711026

		add		ecx, eax	; ecx = tmp6 + z2 + z3
		mov		eax, tmp7

		imul	eax, FIX_1_501321110

//    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp4 + z1 + z3,
//					   CONST_BITS+PASS1_BITS);
//    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp5 + z2 + z4,
//					   CONST_BITS+PASS1_BITS);
//    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp6 + z2 + z3,
//					   CONST_BITS+PASS1_BITS);
//    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp7 + z1 + z4,
//					   CONST_BITS+PASS1_BITS);

		add		edi, edx	; edi = z1 + z4
		add		ecx, 16384
		
		add		edi, eax	; edi = tmp7 + z1 + z4
		mov		eax, tmp5	; eax = tmp5 + z2

		add		ebx, esi	; ebx = tmp4 + z1 + z3
		add		edx, eax	; edx = tmp5 + z2 + z4

		sar		ecx, 15
		add		ebx, 16384

		sar		ebx, 15
		pop		esi

		add		edx, 16384
		add		edi, 16384

		sar		edx, 15
		mov		[esi][DCTWIDTH*7-4], ebx

		sar		edi, 15
		mov		[esi][DCTWIDTH*3-4], ecx

		mov		[esi][DCTWIDTH*5-4], edx
		mov		ecx, counter

		mov		[esi][DCTWIDTH*1-4], edi
		dec		ecx

		mov		counter, ecx
		jnz		StartCol
  } //end asm

//    dataptr++;			/* advance pointer to next column */
//  }
}

#endif /* X86 */
#endif /* DCT_ISLOW_SUPPORTED */
