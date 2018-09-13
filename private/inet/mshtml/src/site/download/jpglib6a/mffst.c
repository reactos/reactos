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

#ifndef USEINLINEASM

GLOBAL(void)
mfdct8x8aan (DCTELEM * data)
{
}

#else

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

//The following constant is shifted left 8 for the pmulhw instruction
const __int64 Const_FIX_0_382683433	=	0x6200620062006200;

  //The following constants are shifted left 7 for the pmulhw instruction
const __int64 Const_FIX_0_541196100	=	0x4580458045804580;
const __int64 Const_FIX_0_707106781	=	0x5a805a805a805a80;

//The following constant is shifted left 6 for the pmulhw instruction
const __int64 Const_FIX_1_306562965	=	0x5380538053805380;

/* We can gain a little more speed, with a further compromise in accuracy,
 * by omitting the addition in a descaling shift.  This yields an incorrectly
 * rounded result half the time...
 */

// The assembly version makes this compromise.
 
//#ifndef USE_ACCURATE_ROUNDING
//#undef DESCALE
//#define DESCALE(x,n)  RIGHT_SHIFT(x, n)
//#endif

#define	DATASIZE	32


/* Multiply a DCTELEM variable by an INT32 constant, and immediately
 * descale to yield a DCTELEM result.
 */

#define MULTIPLY(var,const)  ((DCTELEM) DESCALE((var) * (const), CONST_BITS))


/*
 * Perform the forward DCT on one block of samples.
 */

GLOBAL(void)
mfdct8x8aan (DCTELEM * data)
{

__asm{
	
		mov		edi, [data]
		
	// transpose the bottom right quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------
	// Get the 32-bit quantities and pack into 16 bits

		movq	mm5, [edi][DATASIZE*4+16]		//| w41 | w40 |
		
		movq	mm3, [edi][DATASIZE*4+24]		//| w43 | w42 |
		
		movq	mm6, [edi][DATASIZE*5+16]
		packssdw mm5, mm3				//|w43|w42|w41|w40|

		movq	mm7, [edi][DATASIZE*5+24]
		movq		mm4, mm5			// copy w4---0,1,3,5,6

		movq	mm3, [edi][DATASIZE*6+16]
		packssdw mm6, mm7

		movq	mm2, [edi][DATASIZE*6+24]
		punpcklwd	mm5, mm6			//mm6 = w5

		movq	mm1, [edi][DATASIZE*7+16]
		packssdw mm3, mm2

		movq	mm0, [edi][DATASIZE*7+24]
		punpckhwd	mm4, mm6			//---0,1,3,5,6 

		packssdw mm1, mm0
		movq		mm7, mm3			//---0,1,2,3,5,6 w6

		punpcklwd	mm3, mm1			//mm1 = w7
		movq		mm0, mm5			//---0,2,3,4,5,6,7

		movq	mm2, [edi][DATASIZE*4]	//| w01 | w00 |
		punpckhdq	mm0, mm3			// transposed w5---0,2,4,6,7

		punpckhwd	mm7, mm1			//---0,2,3,5,6,7

		movq	mm1, [edi][DATASIZE*5+8]
		movq		mm6, mm4			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*5+16], mm0  // store w5
		punpckldq	mm5, mm3			// transposed w4

		movq	mm3, [edi][DATASIZE*5]
		punpckldq	mm4, mm7			// transposed w6

		movq	mm0, [edi][DATASIZE*4+8]  //| w03 | w02 |
		punpckhdq	mm6, mm7			// transposed w7---0,3,6,7


	// transpose the bottom left quadrant(4X4) of the matrix and place
	// in the top right quadrant while doing the same for the top
	// right quadrant
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq	[edi][DATASIZE*4+16], mm5  // store w4
		packssdw mm2, mm0				//|w03|w02|w01|w00|

		movq	mm5, [edi][DATASIZE*7]
		packssdw mm3, mm1

		movq	mm0, [edi][DATASIZE*7+8]

		movq	[edi][DATASIZE*7+16], mm6  // store w7---5,6,7
		packssdw mm5, mm0

		movq	mm6, [edi][DATASIZE*6]
		movq		mm0, mm2			// copy w0---0,1,3,5,6

		movq	mm7, [edi][DATASIZE*6+8]
		punpcklwd	mm2, mm3			//mm6 = w1

		movq	[edi][DATASIZE*6+16], mm4  // store w6---3,5,6,7	
		packssdw mm6, mm7

		movq		mm1, [edi][DATASIZE*0+24]
		punpckhwd	mm0, mm3			//---0,1,3,5,6 

		movq		mm7, mm6			//---0,1,2,3,5,6 w2
		punpcklwd	mm6, mm5			//mm1 = w3

		movq		mm3, [edi][DATASIZE*0+16]
		punpckhwd	mm7, mm5			//---0,2,3,5,6,7

		movq		mm4, [edi][DATASIZE*2+24]
		packssdw	mm3, mm1

		movq		mm1, mm2			//---0,2,3,4,5,6,7
		punpckldq	mm2, mm6			// transposed w4

		movq		mm5, [edi][DATASIZE*2+16]
		punpckhdq	mm1, mm6			// transposed w5---0,2,4,6,7

		movq	[edi][DATASIZE*0+16], mm2  // store w4
 		packssdw	mm5, mm4

		movq		mm4, [edi][DATASIZE*1+16]
		movq		mm6, mm0			//---0,2,3,4,6,7

		movq		mm2, [edi][DATASIZE*1+24]
		punpckldq	mm0, mm7			// transposed w6

		movq	[edi][DATASIZE*1+16], mm1  // store w5
		punpckhdq	mm6, mm7			// transposed w7---0,3,6,7

		movq		mm7, [edi][DATASIZE*3+24]
		packssdw	mm4, mm2

		movq	[edi][DATASIZE*2+16], mm0  // store w6---3,5,6,7	
		movq		mm1, mm3			// copy w4---0,1,3,5,6

		movq		mm2, [edi][DATASIZE*3+16]
		punpcklwd	mm3, mm4			//mm6 = w5

		movq	[edi][DATASIZE*3+16], mm6  // store w7---5,6,7
		packssdw	mm2, mm7


	// transpose the bottom left quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

   		movq	mm6, [edi][DATASIZE*0]	//| w01 | w00 |
		punpckhwd	mm1, mm4			//---0,1,3,5,6
		
		movq		mm7, mm5			//---0,1,2,3,5,6 w6
		punpcklwd	mm5, mm2			//mm1 = w7

		movq	mm4, [edi][DATASIZE*0+8]		//| w03 | w02 |
		punpckhwd	mm7, mm2			//---0,2,3,5,6,7

		movq		mm0, mm3			//---0,2,3,4,5,6,7
		packssdw mm6, mm4				//|w03|w02|w01|w00|

		movq	mm2, [edi][DATASIZE*2+8]
		punpckldq	mm3, mm5			// transposed w4

		movq	mm4, [edi][DATASIZE*1]
		punpckhdq	mm0, mm5			// transposed w5---0,2,4,6,7
		
		movq	[edi][DATASIZE*4], mm3  // store w4
		movq		mm5, mm1			//---0,2,3,4,6,7

		movq	mm3, [edi][DATASIZE*2]
		punpckldq	mm1, mm7			// transposed w6

		movq	[edi][DATASIZE*5], mm0  // store w5
		punpckhdq	mm5, mm7			// transposed w7---0,3,6,7

		movq	mm7, [edi][DATASIZE*1+8]
		packssdw mm3, mm2

		movq	[edi][DATASIZE*7], mm5  // store w7---5,6,7
		movq		mm5, mm6			// copy w0---0,1,3,5,6

		movq	[edi][DATASIZE*6], mm1  // store w6---3,5,6,7	
		packssdw mm4, mm7

	// transpose the top left quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

	// Get the 32-bit quantities and pack into 16 bits
		movq	mm1, [edi][DATASIZE*3]
		punpcklwd	mm6, mm4			//mm6 = w1

		movq	mm0, [edi][DATASIZE*3+8]
		punpckhwd	mm5, mm4			//---0,1,3,5,6 

		packssdw mm1, mm0
		movq		mm2, mm3			//---0,1,2,3,5,6 w2

		punpcklwd	mm3, mm1			//mm1 = w3
		movq		mm0, mm6			//---0,2,3,4,5,6,7

		movq		mm4, [edi][DATASIZE*7]
		punpckhwd	mm2, mm1			//---0,2,3,5,6,7

		movq		mm1, [edi][DATASIZE*4]
		punpckldq	mm6, mm3			// transposed w4

		punpckhdq	mm0, mm3			// transposed w5---0,2,4,6,7
		movq		mm3, mm5			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*0], mm6  // store w4
		punpckldq	mm5, mm2			// transposed w6

		movq	[edi][DATASIZE*1], mm0  // store w5
		punpckhdq	mm3, mm2			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*2], mm5  // store w6---3,5,6,7	
		paddw	mm6, mm4				// tmp0

		movq	[edi][DATASIZE*3], mm3  // store w7---5,6,7
		movq	mm7, mm6


	//******************************************************************************
	// End of transpose.  Begin row dct.
	//******************************************************************************

//	tmp0 = dataptr[0] + dataptr[7];
//	tmp7 = dataptr[0] - dataptr[7];
//	tmp1 = dataptr[1] + dataptr[6];
//	tmp6 = dataptr[1] - dataptr[6];
//	tmp2 = dataptr[2] + dataptr[5];
//	tmp5 = dataptr[2] - dataptr[5];
//	tmp3 = dataptr[3] + dataptr[4];
//	tmp4 = dataptr[3] - dataptr[4];

		paddw	mm0, [edi][DATASIZE*6]	// tmp1
		paddw	mm3, mm1				// tmp3

		paddw	mm5, [edi][DATASIZE*5]	// tmp2
		movq	mm1, mm0


//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;

		psubw	mm7, mm3				//tmp13
		psubw	mm0, mm5				//tmp12

		paddw	mm0, mm7	//tmp12 + tmp13
		paddw	mm6, mm3				//tmp10

//    dataptr[0] = tmp10 + tmp11; /* phase 3 */
//    dataptr[4] = tmp10 - tmp11;
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!


		paddw	mm1, mm5				//tmp11
 		psllw	mm0, 1

		pmulhw	mm0, Const_FIX_0_707106781	// z1
		movq	mm3, mm6	

//    dataptr[2] = tmp13 + z1; /* phase 5 */
//    dataptr[6] = tmp13 - z1;
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		movq	mm5, [edi][DATASIZE*3]
		paddw	mm6, mm1	//tmp10 + tmp11
		
	//		tmp4 = dataptr[3] - dataptr[4]//
		psubw	mm5, [edi][DATASIZE*4]	//tmp4
		movq	mm4, mm7

		movq	mm2, [edi][DATASIZE*2]
		psubw	mm3, mm1	//tmp10 - tmp11

		psubw	mm2, [edi][DATASIZE*5]	//tmp5
		paddw	mm7, mm0	//tmp13 + z1

		movq	mm1, [edi][DATASIZE*1]
		psubw	mm4, mm0	//tmp13 - z1

//    tmp10 = tmp4 + tmp5;	/* phase 2 */
//    tmp11 = tmp5 + tmp6;
//    tmp12 = tmp6 + tmp7;

		psubw	mm1, [edi][DATASIZE*6]	//tmp6
		paddw	mm5, mm2	//tmp10
		
		movq	mm0, [edi][DATASIZE*0]
		paddw	mm2, mm1	//tmp11

//    z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */
//    z11 = tmp7 + z3;		/* phase 5 */
//    z13 = tmp7 - z3;
		
		psubw	mm0, [edi][DATASIZE*7]	//tmp7
 		psllw	mm2, 1

		movq	[edi][DATASIZE*0], mm6
		movq	mm6, mm0

		movq	[edi][DATASIZE*2], mm7
		movq	mm7, mm5

		pmulhw	mm2, Const_FIX_0_707106781	//z3
		paddw	mm1, mm0	//tmp12

		movq	[edi][DATASIZE*4], mm3
		psubw	mm5, mm1	//tmp10 - tmp12

		pmulhw	mm5, Const_FIX_0_382683433	//z5
		psllw	mm7, 1

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
//    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
//    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
//    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */
		
		pmulhw	mm7, Const_FIX_0_541196100
		psllw	mm1, 2

		pmulhw	mm1, Const_FIX_1_306562965
		psubw	mm6, mm2	//z13

		movq	[edi][DATASIZE*6], mm4
		paddw	mm0, mm2	//z11

		movq	mm2, [edi][DATASIZE*3+16]
		paddw	mm7, mm5	//z2

		paddw	mm2, [edi][DATASIZE*4+16]	// tmp3
		paddw	mm1, mm5	//z4

//    dataptr[5] = z13 + z2;	/* phase 6 */
//    dataptr[3] = z13 - z2;
//    dataptr[1] = z11 + z4;
//    dataptr[7] = z11 - z4;

		movq	mm5, [edi][DATASIZE*0+16]
		movq	mm3, mm6

		paddw	mm5, [edi][DATASIZE*7+16]	//tmp0
		paddw	mm6, mm7	//z13 + z2

		psubw	mm3, mm7	//z13 - z2
		movq	mm7, mm5

		movq	[edi][DATASIZE*5], mm6	//store 
		movq	mm4, mm0
		
		movq	[edi][DATASIZE*3], mm3	//store 
		paddw	mm0, mm1	//z11 + z4

		movq	mm3, [edi][DATASIZE*1+16]
		psubw	mm4, mm1	//z11 - z4

	//******************************************************************************
	// This completes 4x8 dct locations.  Copy to do other 4x8.
	//******************************************************************************
//	tmp0 = dataptr[0] + dataptr[7];
//	tmp7 = dataptr[0] - dataptr[7];
//	tmp1 = dataptr[1] + dataptr[6];
//	tmp6 = dataptr[1] - dataptr[6];
//	tmp2 = dataptr[2] + dataptr[5];
//	tmp5 = dataptr[2] - dataptr[5];
//	tmp3 = dataptr[3] + dataptr[4];
//	tmp4 = dataptr[3] - dataptr[4];

		paddw	mm3, [edi][DATASIZE*6+16]	// tmp1
		paddw	mm5, mm2				//tmp10

		movq	mm1, [edi][DATASIZE*2+16]
		psubw	mm7, mm2				//tmp13

		paddw	mm1, [edi][DATASIZE*5+16]	// tmp2
		movq	mm6, mm3

//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;

		paddw	mm3, mm1				//tmp11
		psubw	mm6, mm1				//tmp12

//    dataptr[0] = tmp10 + tmp11; /* phase 3 */
//    dataptr[4] = tmp10 - tmp11;
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		movq	[edi][DATASIZE*1], mm0	//store 
		paddw	mm6, mm7	//tmp12 + tmp13

		movq	[edi][DATASIZE*7], mm4	//store 
 		psllw	mm6, 1

 		pmulhw	mm6, Const_FIX_0_707106781	// z1
		movq	mm1, mm5	

//    dataptr[2] = tmp13 + z1; /* phase 5 */
//    dataptr[6] = tmp13 - z1;
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		movq	mm2, [edi][DATASIZE*3+16]
		paddw	mm5, mm3	//tmp10 + tmp11
		
	//		tmp4 = dataptr[3] - dataptr[4]//
		psubw	mm2, [edi][DATASIZE*4+16]	//tmp4
		movq	mm4, mm7

		movq	mm0, [edi][DATASIZE*2+16]
		psubw	mm1, mm3	//tmp10 - tmp11

		psubw	mm0, [edi][DATASIZE*5+16]	//tmp5
		paddw	mm7, mm6	//tmp13 + z1

		movq	mm3, [edi][DATASIZE*1+16]
		psubw	mm4, mm6	//tmp13 - z1

//    tmp10 = tmp4 + tmp5;	/* phase 2 */
//    tmp11 = tmp5 + tmp6;
//    tmp12 = tmp6 + tmp7;

		psubw	mm3, [edi][DATASIZE*6+16]	//tmp6
		paddw	mm2, mm0	//tmp10
		
		movq	mm6, [edi][DATASIZE*0+16]
		paddw	mm0, mm3	//tmp11

//    z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */
//    z11 = tmp7 + z3;		/* phase 5 */
//    z13 = tmp7 - z3;
		
		psubw	mm6, [edi][DATASIZE*7+16]	//tmp7
 		psllw	mm0, 1

		movq	[edi][DATASIZE*0+16], mm5
		movq	mm5, mm6

		movq	[edi][DATASIZE*2+16], mm7
		movq	mm7, mm2

		pmulhw	mm0, Const_FIX_0_707106781	//z3
		paddw	mm3, mm6	//tmp12

		movq	[edi][DATASIZE*4+16], mm1
		psubw	mm2, mm3	//tmp10 - tmp12

		pmulhw	mm2, Const_FIX_0_382683433	//z5
		psllw	mm7, 1

		pmulhw	mm7, Const_FIX_0_541196100
		paddw	mm6, mm0	//z11

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
//    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
//    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
//    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */

		movq	[edi][DATASIZE*6+16], mm4
		psllw	mm3, 2

		pmulhw	mm3, Const_FIX_1_306562965
		psubw	mm5, mm0	//z13

		paddw	mm7, mm2	//z2
		movq	mm1, mm5

		paddw	mm5, mm7	//z13 + z2
		psubw	mm1, mm7	//z13 - z2
		
		movq	mm7, [edi][DATASIZE*4]
		paddw	mm3, mm2	//z4

//    dataptr[5] = z13 + z2;	/* phase 6 */
//    dataptr[3] = z13 - z2;
//    dataptr[1] = z11 + z4;
//    dataptr[7] = z11 - z4;

		movq	[edi][DATASIZE*5+16], mm5	//store 
		movq	mm4, mm6

		movq	mm2, [edi][DATASIZE*7]
		paddw	mm6, mm3	//z11 + z4

		movq	mm5, [edi][DATASIZE*5]
		psubw	mm4, mm3	//z11 - z4

	//******************************************************************************

	//******************************************************************************
	// This completes all 8x8 dct locations for the row case.
	// Now transpose the data for the columns.
	//******************************************************************************
	// transpose the bottom left quadrant(4X4) of the matrix and place
	// in the top right quadrant while doing the same for the top
	// right quadrant
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq		mm0, mm7			// copy w0---0,1,3,5,6
		punpcklwd	mm7, mm5			//mm6 = w1

		movq	mm3, [edi][DATASIZE*6]
		punpckhwd	mm0, mm5			//---0,1,3,5,6 

		movq		mm5, mm3			//---0,1,2,3,5,6 w2
		punpcklwd	mm3, mm2			//mm1 = w3

		movq	[edi][DATASIZE*7+16], mm4	//store
		punpckhwd	mm5, mm2			//---0,2,3,5,6,7

		movq		mm4, mm7			//---0,2,3,4,5,6,7
		punpckldq	mm7, mm3			// transposed w4
		
		movq		mm2, [edi][DATASIZE*0+16]
		punpckhdq	mm4, mm3			// transposed w5---0,2,4,6,7

		movq	[edi][DATASIZE*0+16], mm7  // store w4
		movq		mm3, mm0			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*1+16], mm4  // store w5
		punpckldq	mm0, mm5			// transposed w6

		movq		mm7, [edi][DATASIZE*2+16]
		punpckhdq	mm3, mm5			// transposed w7---0,3,6,7

		movq		mm5, mm2			// copy w4---0,1,3,5,6
		punpcklwd	mm2, mm6			//mm6 = w5

	// transpose the top right quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq	[edi][DATASIZE*2+16], mm0  // store w6---3,5,6,7	
		punpckhwd	mm5, mm6			//---0,1,3,5,6 

		movq		mm4, mm7			//---0,1,2,3,5,6 w6
		punpckhwd	mm7, mm1			//---0,2,3,5,6,7

		movq	[edi][DATASIZE*3+16], mm3  // store w7---5,6,7
		movq		mm0, mm2			//---0,2,3,4,5,6,7

		movq	mm6, [edi][DATASIZE*5+16]
		punpcklwd	mm4, mm1			//mm1 = w7

		movq	mm1, [edi][DATASIZE*4+16]
		punpckldq	mm0, mm4			// transposed w4

		movq	mm3, [edi][DATASIZE*6+16]
		punpckhdq	mm2, mm4			// transposed w5---0,2,4,6,7

	// transpose the bottom right quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq	[edi][DATASIZE*4], mm0  // store w4
		movq		mm4, mm5			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*5], mm2  // store w5
		punpckldq	mm5, mm7			// transposed w6

		movq	mm2, [edi][DATASIZE*7+16]
		punpckhdq	mm4, mm7			// transposed w7---0,3,6,7

		movq		mm7, mm1			// copy w4---0,1,3,5,6
		punpcklwd	mm1, mm6			//mm6 = w5

		movq	[edi][DATASIZE*6], mm5  // store w6---3,5,6,7	
		punpckhwd	mm7, mm6			//---0,1,3,5,6 

		movq		mm5, mm3			//---0,1,2,3,5,6 w6
		punpcklwd	mm3, mm2			//mm1 = w7

		movq	[edi][DATASIZE*7], mm4  // store w7---5,6,7
		punpckhwd	mm5, mm2			//---0,2,3,5,6,7

		movq	mm0, [edi][DATASIZE*0]
		movq		mm4, mm1			//---0,2,3,4,5,6,7

		movq	mm6, [edi][DATASIZE*1]
		punpckldq	mm1, mm3			// transposed w4

		punpckhdq	mm4, mm3			// transposed w5---0,2,4,6,7
		movq		mm3, mm7			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*4+16], mm1  // store w4
		punpckldq	mm7, mm5			// transposed w6

		movq	[edi][DATASIZE*5+16], mm4  // store w5
		punpckhdq	mm3, mm5			// transposed w7---0,3,6,7


	// transpose the top left quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq	mm1, [edi][DATASIZE*3]
		movq		mm2, mm0			// copy w0---0,1,3,5,6

		movq	[edi][DATASIZE*7+16], mm3  // store w7---5,6,7
		punpcklwd	mm0, mm6			//mm6 = w1

		movq	mm3, [edi][DATASIZE*2]
		punpckhwd	mm2, mm6			//---0,1,3,5,6 

		movq		mm5, mm3			//---0,1,2,3,5,6 w2
		punpcklwd	mm3, mm1			//mm1 = w3

		movq	[edi][DATASIZE*6+16], mm7  // store w6---3,5,6,7	
		punpckhwd	mm5, mm1			//---0,2,3,5,6,7

		movq		mm1, mm0			//---0,2,3,4,5,6,7
		punpckldq	mm0, mm3			// transposed w4

		movq		mm6, [edi][DATASIZE*4]
		punpckhdq	mm1, mm3			// transposed w5---0,2,4,6,7

 		movq	[edi][DATASIZE*0], mm0  // store w4
		movq		mm3, mm2			//---0,2,3,4,6,7

		paddw	mm0, [edi][DATASIZE*7]	// tmp0
		punpckhdq	mm3, mm5			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*1], mm1  // store w5
		punpckldq	mm2, mm5			// transposed w6


	//******************************************************************************
	// This begins the column dct
	//******************************************************************************

//	tmp0 = dataptr[0] + dataptr[7];
//	tmp7 = dataptr[0] - dataptr[7];
//	tmp1 = dataptr[1] + dataptr[6];
//	tmp6 = dataptr[1] - dataptr[6];
//	tmp2 = dataptr[2] + dataptr[5];
//	tmp5 = dataptr[2] - dataptr[5];
//	tmp3 = dataptr[3] + dataptr[4];
//	tmp4 = dataptr[3] - dataptr[4];

		movq	[edi][DATASIZE*3], mm3  // store w7---5,6,7
		movq	mm7, mm0

		paddw	mm1, [edi][DATASIZE*6]	// tmp1
		paddw	mm3, mm6	// tmp3

		movq	[edi][DATASIZE*2], mm2  // store w6---3,5,6,7	
		paddw	mm0, mm3				//tmp10

		paddw	mm2, [edi][DATASIZE*5]	// tmp2
		movq	mm6, mm1

//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;

		psubw	mm7, mm3				//tmp13
		movq	mm3, mm0	

		movq	mm5, [edi][DATASIZE*2]
		paddw	mm1, mm2				//tmp11

		psubw	mm3, mm1	//tmp10 - tmp11
		paddw	mm0, mm1	//tmp10 + tmp11

//    dataptr[0] = tmp10 + tmp11; /* phase 3 */
//    dataptr[4] = tmp10 - tmp11;
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		movq	mm1, mm3
		punpcklwd mm3, mm3

		psubw	mm6, mm2				//tmp12
		punpckhwd mm1, mm1

		movq	mm2, [edi][DATASIZE*3]
		psrad	mm3, 16
		
	//		tmp4 = dataptr[3] - dataptr[4]//
		psubw	mm2, [edi][DATASIZE*4]	//tmp4
		psrad	mm1, 16

		movq	[edi][DATASIZE*4], mm3
		movq	mm3, mm0

		movq	[edi][DATASIZE*4+8], mm1
		punpcklwd mm0, mm0

		paddw	mm6, mm7	//tmp12 + tmp13
		punpckhwd mm3, mm3

		movq	mm1, [edi][DATASIZE*1]
		psllw	mm6, 1

 		pmulhw	mm6, Const_FIX_0_707106781	// z1
 		psrad	mm3, 16

		psubw	mm5, [edi][DATASIZE*5]	//tmp5
		psrad	mm0, 16

//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */
//    dataptr[2] = tmp13 + z1; /* phase 5 */
//    dataptr[6] = tmp13 - z1;
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		movq	[edi][DATASIZE*0+8], mm3
		movq	mm4, mm7

		movq	mm3, [edi][DATASIZE*0]
		paddw	mm7, mm6	//tmp13 + z1

		movq	[edi][DATASIZE*0], mm0
		psubw	mm4, mm6	//tmp13 - z1

		movq	mm0, mm7
		punpcklwd mm7, mm7

		psubw	mm1, [edi][DATASIZE*6]	//tmp6
		punpckhwd mm0, mm0

//    tmp10 = tmp4 + tmp5;	/* phase 2 */
//    tmp11 = tmp5 + tmp6;
//    tmp12 = tmp6 + tmp7;

		psrad	mm7, 16
		paddw	mm2, mm5	//tmp10

		psrad	mm0, 16
		paddw	mm5, mm1	//tmp11

		movq	mm6, mm4
		punpcklwd mm4, mm4

		movq	[edi][DATASIZE*2], mm7
		punpckhwd mm6, mm6

		psubw	mm3, [edi][DATASIZE*7]	//tmp7
		movq	mm7, mm2

//    z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */
//    z11 = tmp7 + z3;		/* phase 5 */
//    z13 = tmp7 - z3;
		
		movq	[edi][DATASIZE*2+8], mm0
		movq	mm0, mm3

 		psllw	mm5, 1
		paddw	mm1, mm3	//tmp12

		pmulhw	mm5, Const_FIX_0_707106781	//z3
		psrad	mm4, 16

		psubw	mm2, mm1	//tmp10 - tmp12
		psrad	mm6, 16

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
//    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
//    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
//    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */
		
		pmulhw	mm2, Const_FIX_0_382683433	//z5
		psllw	mm7, 1

		pmulhw	mm7, Const_FIX_0_541196100
		psllw	mm1, 2

		pmulhw	mm1, Const_FIX_1_306562965
		psubw	mm0, mm5	//z13

		movq	[edi][DATASIZE*6+8], mm6
		movq	mm6, mm0

		movq	[edi][DATASIZE*6], mm4
		paddw	mm7, mm2	//z2

//    dataptr[5] = z13 + z2;	/* phase 6 */
//    dataptr[3] = z13 - z2;
//    dataptr[1] = z11 + z4;
//    dataptr[7] = z11 - z4;

		paddw	mm0, mm7	//z13 + z2
		psubw	mm6, mm7	//z13 - z2
		
		movq	mm7, mm6
		punpcklwd mm6, mm6

		punpckhwd mm7, mm7
		paddw	mm3, mm5	//z11

		movq	mm5, mm0
		punpcklwd mm0, mm0

		psrad	mm6, 16
		movq	mm4, mm3

		psrad	mm7, 16
		paddw	mm1, mm2	//z4

		punpckhwd mm5, mm5
		paddw	mm3, mm1	//z11 + z4

		psrad	mm0, 16
		psubw	mm4, mm1	//z11 - z4

		movq	[edi][DATASIZE*3], mm6	//store 
		psrad	mm5, 16

		movq	mm6, [edi][DATASIZE*1+16]
		movq	mm1, mm3

		paddw	mm6, [edi][DATASIZE*6+16]	// tmp1
		punpcklwd mm3, mm3

		movq	[edi][DATASIZE*3+8], mm7
		punpckhwd mm1, mm1

		movq	[edi][DATASIZE*5], mm0	//store 
		psrad	mm3, 16

		movq	[edi][DATASIZE*5+8], mm5
		psrad	mm1, 16

		movq	mm0, [edi][DATASIZE*0+16]
		movq	mm7, mm4

		paddw	mm0, [edi][DATASIZE*7+16]	//tmp0
		punpcklwd mm4, mm4

		movq	[edi][DATASIZE*1], mm3	//store 
		punpckhwd mm7, mm7

		movq	[edi][DATASIZE*1+8], mm1
		psrad	mm4, 16
		
		movq	mm3, [edi][DATASIZE*3+16]
		psrad	mm7, 16

	//******************************************************************************
	// This completes 4x8 dct locations.  Copy to do other 4x8.
	//******************************************************************************
//	tmp0 = dataptr[0] + dataptr[7];
//	tmp7 = dataptr[0] - dataptr[7];
//	tmp1 = dataptr[1] + dataptr[6];
//	tmp6 = dataptr[1] - dataptr[6];
//	tmp2 = dataptr[2] + dataptr[5];
//	tmp5 = dataptr[2] - dataptr[5];
//	tmp3 = dataptr[3] + dataptr[4];
//	tmp4 = dataptr[3] - dataptr[4];

		paddw	mm3, [edi][DATASIZE*4+16]	// tmp3
		movq	mm1, mm6

		movq	[edi][DATASIZE*7+8], mm7
		movq	mm7, mm0

		movq	mm2, [edi][DATASIZE*2+16]
		paddw	mm0, mm3				//tmp10

		paddw	mm2, [edi][DATASIZE*5+16]	// tmp2
		psubw	mm7, mm3				//tmp13

		movq	mm3, mm0	
		paddw	mm1, mm2				//tmp11

//    tmp10 = tmp0 + tmp3;
//    tmp13 = tmp0 - tmp3;
//    tmp11 = tmp1 + tmp2;
//    tmp12 = tmp1 - tmp2;

		paddw	mm0, mm1	//tmp10 + tmp11
		psubw	mm3, mm1	//tmp10 - tmp11

//    dataptr[0] = tmp10 + tmp11; /* phase 3 */
//    dataptr[4] = tmp10 - tmp11;
//    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		movq	mm1, mm3
		punpcklwd mm3, mm3

		punpckhwd mm1, mm1
		psubw	mm6, mm2				//tmp12

		movq	[edi][DATASIZE*7], mm4	//store 
		psrad	mm3, 16

		psrad	mm1, 16
		paddw	mm6, mm7	//tmp12 + tmp13

		movq	mm2, [edi][DATASIZE*3+16]
 		psllw	mm6, 1

		movq	mm4, mm0
		punpcklwd mm0, mm0

 		pmulhw	mm6, Const_FIX_0_707106781	// z1
		punpckhwd mm4, mm4

	//		tmp4 = dataptr[3] - dataptr[4]//
		psubw	mm2, [edi][DATASIZE*4+16]	//tmp4
		psrad	mm4, 16

		movq	mm5, [edi][DATASIZE*2+16]
		psrad	mm0, 16

		movq	[edi][DATASIZE*0+24], mm4
		movq	mm4, mm7

//    dataptr[2] = tmp13 + z1; /* phase 5 */
//    dataptr[6] = tmp13 - z1;
//NOTE: We can't write these values out immediately.  Values for tmp4 - tmp7
//haven't been calculated yet!

		psubw	mm5, [edi][DATASIZE*5+16]	//tmp5
		paddw	mm7, mm6	//tmp13 + z1

		movq	[edi][DATASIZE*4+16], mm3
		psubw	mm4, mm6	//tmp13 - z1

		movq	mm3, mm7
		punpcklwd mm7, mm7

		movq	mm6, [edi][DATASIZE*0+16]
		punpckhwd mm3, mm3

		movq	[edi][DATASIZE*4+24], mm1
		psrad	mm7, 16

		movq	[edi][DATASIZE*0+16], mm0
		psrad	mm3, 16

		movq	mm1, [edi][DATASIZE*1+16]
		movq	mm0, mm4

		psubw	mm1, [edi][DATASIZE*6+16]	//tmp6
		punpcklwd mm4, mm4

		movq	[edi][DATASIZE*2+16], mm7
		paddw	mm2, mm5	//tmp10

//    tmp10 = tmp4 + tmp5;	/* phase 2 */
//    tmp11 = tmp5 + tmp6;
//    tmp12 = tmp6 + tmp7;

		movq	mm7, mm2
		paddw	mm5, mm1	//tmp11

		psubw	mm6, [edi][DATASIZE*7+16]	//tmp7
 		punpckhwd mm0, mm0

		movq	[edi][DATASIZE*2+24], mm3
		psllw	mm5, 1

		pmulhw	mm5, Const_FIX_0_707106781	//z3
		psrad	mm0, 16

		psrad	mm4, 16
		paddw	mm1, mm6	//tmp12
		
//    z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */
//    z11 = tmp7 + z3;		/* phase 5 */
//    z13 = tmp7 - z3;
		
		movq	[edi][DATASIZE*6+24], mm0
		psubw	mm2, mm1	//tmp10 - tmp12

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
//    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
//    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
//    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */
		
		pmulhw	mm2, Const_FIX_0_382683433	//z5
		psllw	mm7, 1

		pmulhw	mm7, Const_FIX_0_541196100
		psllw	mm1, 2

		movq	[edi][DATASIZE*6+16], mm4
		movq	mm0, mm6

		pmulhw	mm1, Const_FIX_1_306562965
		psubw	mm0, mm5	//z13

		paddw	mm7, mm2	//z2
		movq	mm3, mm0

//    dataptr[5] = z13 + z2;	/* phase 6 */
//    dataptr[3] = z13 - z2;
//    dataptr[1] = z11 + z4;
//    dataptr[7] = z11 - z4;

		paddw	mm0, mm7	//z13 + z2
		psubw	mm3, mm7	//z13 - z2
		
		movq	mm7, mm3
		punpcklwd mm3, mm3

		punpckhwd mm7, mm7
		paddw	mm6, mm5	//z11

		psrad	mm3, 16
		paddw	mm1, mm2	//z4

		psrad	mm7, 16
		movq	mm4, mm6

		movq	mm5, mm0
		punpcklwd mm0, mm0

		punpckhwd mm5, mm5
		paddw	mm6, mm1	//z11 + z4

		psrad	mm0, 16
		psubw	mm4, mm1	//z11 - z4

		movq	[edi][DATASIZE*3+16], mm3	//store
		psrad	mm5, 16
		
		movq	mm1, mm6
		punpcklwd mm6, mm6

		movq	[edi][DATASIZE*3+24], mm7
		punpckhwd mm1, mm1

		movq	[edi][DATASIZE*5+16], mm0	//store 
		psrad	mm6, 16

		movq	[edi][DATASIZE*5+24], mm5
		psrad	mm1, 16

		movq	mm7, mm4
		punpcklwd mm4, mm4

		movq	[edi][DATASIZE*1+16], mm6	//store
		punpckhwd mm7, mm7
		
		movq	[edi][DATASIZE*1+24], mm1
		psrad	mm4, 16

		psrad	mm7, 16
		movq	[edi][DATASIZE*7+16], mm4	//store 
		movq	[edi][DATASIZE*7+24], mm7

	//******************************************************************************
	// This completes all 8x8 dct locations for the column case.
	//******************************************************************************

		emms
	}
}

#endif /* X86 */

#endif /* DCT_ISLOW_SUPPORTED */
