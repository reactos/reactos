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
 * jfdctfst.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a fast, not so accurate integer implementation of the
 * forward DCT (Discrete Cosine Transform).
 *
 * A 2-D DCT can be done by 1-D DCT on each row followed by 1-D DCT
 * on each column.  Direct algorithms are also available, but they are
 * much more complex and seem not to be any faster when reduced to code.
 *
 * This implementation is based on Arai, Agui, and Nakajima's algorithm for
 * scaled DCT.  Their original paper (Trans. IEICE E-71(11):1095) is in
 * Japanese, but the algorithm is described in the Pennebaker & Mitchell
 * JPEG textbook (see REFERENCES section in file README).  The following code
 * is based directly on figure 4-8 in P&M.
 * While an 8-point DCT cannot be done in less than 11 multiplies, it is
 * possible to arrange the computation so that many of the multiplies are
 * simple scalings of the final outputs.  These multiplies can then be
 * folded into the multiplications or divisions by the JPEG quantization
 * table entries.  The AA&N method leaves only 5 multiplies and 29 adds
 * to be done in the DCT itself.
 * The primary disadvantage of this method is that with fixed-point math,
 * accuracy is lost due to imprecise representation of the scaled
 * quantization values.  The smaller the quantization table entry, the less
 * precise the scaled value, so this implementation does worse with high-
 * quality-setting files than with low-quality ones.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */

#ifdef DCT_IFAST_SUPPORTED


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/* Scaling decisions are generally the same as in the LL&M algorithm;
 * see jfdctint.c for more details.  However, we choose to descale
 * (right shift) multiplication products as soon as they are formed,
 * rather than carrying additional fractional bits into subsequent additions.
 * This compromises accuracy slightly, but it lets us save a few shifts.
 * More importantly, 16-bit arithmetic is then adequate (for 8-bit samples)
 * everywhere except in the multiplications proper; this saves a good deal
 * of work on 16-bit-int machines.
 *
 * Again to save a few shifts, the intermediate results between pass 1 and
 * pass 2 are not upscaled, but are represented only to integral precision.
 *
 * A final compromise is to represent the multiplicative constants to only
 * 8 fractional bits, rather than 13.  This saves some shifting work on some
 * machines, and may also reduce the cost of multiplication (since there
 * are fewer one-bits in the constants).
 */

#define CONST_BITS  8


/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

#if CONST_BITS == 8
#define FIX_0_382683433				 98		/* FIX(0.382683433) */
#define FIX_0_541196100				139		/* FIX(0.541196100) */
#define FIX_0_707106781				181		/* FIX(0.707106781) */
#define FIX_1_306562965				334		/* FIX(1.306562965) */
#else
#define FIX_0_382683433  FIX(0.382683433)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_707106781  FIX(0.707106781)
#define FIX_1_306562965  FIX(1.306562965)
#endif


/* We can gain a little more speed, with a further compromise in accuracy,
 * by omitting the addition in a descaling shift.  This yields an incorrectly
 * rounded result half the time...
 */

// The assembly version makes this compromise.
 
//#ifndef USE_ACCURATE_ROUNDING
//#undef DESCALE
//#define DESCALE(x,n)  RIGHT_SHIFT(x, n)
//#endif

#define	DCTWIDTH	32
#define	DATASIZE	4


/* Multiply a DCTELEM variable by an INT32 constant, and immediately
 * descale to yield a DCTELEM result.
 */

#define MULTIPLY(var,const)  ((DCTELEM) DESCALE((var) * (const), CONST_BITS))


/*
 * Perform the forward DCT on one block of samples.
 */

GLOBAL(void)
pfdct8x8aan (DCTELEM * data)
{
  DCTELEM tmp4, tmp6, tmp7;
  int counter;

  __asm{
 
  /* Pass 1: process rows. */

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
 		
    /* Even part */
    
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
  		
//    dataptr[0] = tmp10 + tmp11; /* phase 3 */
//    dataptr[4] = tmp10 - tmp11;
    
		mov		edi, eax
		add		eax, edx	; eax = tmp10 + tmp11
		
		sub		edi, edx	; edi = tmp10 - tmp11
		add		ebp, ecx	; ebp = tmp12 + tmp13
		
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */

		imul	ebp, FIX_0_707106781	; ebp = z1
		
		sar		ebp, 8
		mov		[esi][DATASIZE*0], eax
		
//    dataptr[2] = tmp13 + z1; /* phase 5 */
//    dataptr[6] = tmp13 - z1;

		mov		eax, ecx
		add		ecx, ebp

		sub		eax, ebp
		pop		ebp

		mov		[esi][DATASIZE*4], edi
		mov		[esi][DATASIZE*2], ecx

		mov		[esi][DATASIZE*6], eax
		mov		edi, tmp4

    /* Odd part */
    
//    tmp10 = tmp4 + tmp5;	/* phase 2 */
//    tmp11 = tmp5 + tmp6;
//    tmp12 = tmp6 + tmp7;

		mov		ecx, tmp6
		mov		edx, tmp7

		add		edi, ebx	; edi = tmp10
		add		ebx, ecx	; ebx = tmp11

//    z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */
//    z11 = tmp7 + z3;		/* phase 5 */
//    z13 = tmp7 - z3;
		
		imul	ebx, FIX_0_707106781	; ebx = z3

		sar		ebx, 8
		add		ecx, edx	; ecx = tmp12

		mov		eax, edx
		add		edx, ebx	; edx = z11

		sub		eax, ebx	; eax = z13
		mov		ebx, edi

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
//    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
//    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
//    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */
		
		imul	ebx, FIX_0_541196100
		
		sar		ebx, 8
		sub		edi, ecx	; edi = tmp10 - tmp12	

		imul	edi, FIX_0_382683433	; edi = z5

		sar		edi, 8
		add		esi, 32

		imul	ecx, FIX_1_306562965

		sar		ecx, 8
		add		ebx, edi	; ebx = z2

		add		ecx, edi	; ecx = z4
		mov		edi, eax
		
//    dataptr[5] = z13 + z2;	/* phase 6 */
//    dataptr[3] = z13 - z2;
//    dataptr[1] = z11 + z4;
//    dataptr[7] = z11 - z4;

		add		eax, ebx	; eax = z13 + z2
		sub		edi, ebx	; edi = z13 - z2

		mov		[esi][DATASIZE*5-32], eax
		mov		ebx, edx

		mov		[esi][DATASIZE*3-32], edi
		add		edx, ecx	; edx = z11 + z4

		mov		[esi][DATASIZE*1-32], edx
		sub		ebx, ecx	; ebx = z11 - z4

		mov		ecx, counter
		mov		[esi][DATASIZE*7-32], ebx

		dec		ecx

		mov		counter, ecx
		jnz		StartRow
    
//    dataptr += DCTSIZE;		/* advance pointer to next row */
//  }

  
  
  
  /* Pass 2: process columns.*/


//  dataptr = data;
		mov 	esi, [data]
		mov		counter, 8
		
//  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
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
 		
    /* Even part */
    
//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;
    
		mov		ecx, eax
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
  		mov		ebp, edx
  		
  		add		edx, edi	; edx = tmp11
  		sub		ebp, edi	; ebp = tmp12
  		
//    dataptr[DCTSIZE*0] = tmp10 + tmp11; /* phase 3 */
//    dataptr[DCTSIZE*4] = tmp10 - tmp11;
    
		mov		edi, eax
		add		eax, edx	; eax = tmp10 + tmp11
		
		sub		edi, edx	; edi = tmp10 - tmp11
		add		ebp, ecx	; ebp = tmp12 + tmp13
		
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */

		imul	ebp, FIX_0_707106781	; ebp = z1
		
		sar		ebp, 8
		mov		[esi][DCTWIDTH*0], eax
		
//    dataptr[DCTSIZE*2] = tmp13 + z1; /* phase 5 */
//    dataptr[DCTSIZE*6] = tmp13 - z1;

		mov		eax, ecx
		add		ecx, ebp

		sub		eax, ebp
		pop		ebp

		mov		[esi][DCTWIDTH*4], edi
		mov		[esi][DCTWIDTH*2], ecx

		mov		[esi][DCTWIDTH*6], eax
		mov		edi, tmp4

    /* Odd part */
    
//    tmp10 = tmp4 + tmp5;	/* phase 2 */
//    tmp11 = tmp5 + tmp6;
//    tmp12 = tmp6 + tmp7;

		mov		ecx, tmp6
		mov		edx, tmp7

		add		edi, ebx	; edi = tmp10
		add		ebx, ecx	; ebx = tmp11

//    z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */
//    z11 = tmp7 + z3;		/* phase 5 */
//    z13 = tmp7 - z3;
		
		imul	ebx, FIX_0_707106781	; ebx = z3

		sar		ebx, 8
		add		ecx, edx	; ecx = tmp12

		mov		eax, edx
		add		edx, ebx	; edx = z11

		sub		eax, ebx	; eax = z13
		mov		ebx, edi

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
//    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
//    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
//    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */
		
		imul	ebx, FIX_0_541196100
		
		sar		ebx, 8
		sub		edi, ecx	; edi = tmp10 - tmp12	

		imul	edi, FIX_0_382683433	; edi = z5

		sar		edi, 8
		add		esi, 4

		imul	ecx, FIX_1_306562965

		sar		ecx, 8
		add		ebx, edi	; ebx = z2

		add		ecx, edi	; ecx = z4
		mov		edi, eax
		
//    dataptr[DCTSIZE*5] = z13 + z2; /* phase 6 */
//    dataptr[DCTSIZE*3] = z13 - z2;
//    dataptr[DCTSIZE*1] = z11 + z4;
//    dataptr[DCTSIZE*7] = z11 - z4;

		add		eax, ebx	; eax = z13 + z2
		sub		edi, ebx	; edi = z13 - z2

		mov		[esi][DCTWIDTH*5-4], eax
		mov		ebx, edx

		mov		[esi][DCTWIDTH*3-4], edi
		add		edx, ecx	; edx = z11 + z4

		mov		[esi][DCTWIDTH*1-4], edx
		sub		ebx, ecx	; ebx = z11 - z4

		mov		ecx, counter
		mov		[esi][DCTWIDTH*7-4], ebx

		dec		ecx

		mov		counter, ecx
		jnz		StartCol
  } //end asm

//    dataptr++;			/* advance pointer to next column */
//  }
}

#endif /* DCT_ISLOW_SUPPORTED */
