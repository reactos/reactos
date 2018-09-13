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


/*
 * This module is specialized to the case DATASIZE = 8.
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

const __int64 Const_1					=	0x0000000100000001;
const __int64 Const_2					=	0x0002000200020002;
const __int64 Const_1024				=	0x0000040000000400;
const __int64 Const_16384				=	0x0000400000004000;
const __int64 Const_FFFF				=	0xFFFFFFFFFFFFFFFF;
										 
const __int64 Const_0xFIX_0_298631336	=	0x0000098e0000098e;
const __int64 Const_FIX_0_298631336x0	=	0x098e0000098e0000;
const __int64 Const_0xFIX_0_390180644	=	0x00000c7c00000c7c;
const __int64 Const_FIX_0_390180644x0	=	0x0c7c00000c7c0000;
const __int64 Const_0xFIX_0_541196100	=	0x0000115100001151;
const __int64 Const_FIX_0_541196100x0	=	0x1151000011510000;
const __int64 Const_0xFIX_0_765366865	=	0x0000187e0000187e;
const __int64 Const_FIX_0_765366865x0	=	0x187e0000187e0000;
const __int64 Const_0xFIX_0_899976223	=	0x00001ccd00001ccd;
const __int64 Const_FIX_0_899976223x0	=	0x1ccd00001ccd0000;
const __int64 Const_0xFIX_1_175875602	=	0x000025a1000025a1;		
const __int64 Const_FIX_1_175875602x0	=	0x25a1000025a10000;
const __int64 Const_0xFIX_1_501321110	=	0x0000300b0000300b;
const __int64 Const_FIX_1_501321110x0	=	0x300b0000300b0000;
const __int64 Const_0xFIX_1_847759065	=	0x00003b2100003b21;
const __int64 Const_FIX_1_847759065x0	=	0x3b2100003b210000;
const __int64 Const_0xFIX_1_961570560	=	0x00003ec500003ec5;
const __int64 Const_FIX_1_961570560x0	=	0x3ec500003ec50000;
const __int64 Const_0xFIX_2_053119869	=	0x000041b3000041b3;
const __int64 Const_FIX_2_053119869x0	=	0x41b3000041b30000;
const __int64 Const_0xFIX_2_562915447	=	0x0000520300005203;
const __int64 Const_FIX_2_562915447x0	=	0x5203000052030000;
const __int64 Const_0xFIX_3_072711026	=	0x0000625400006254;
const __int64 Const_FIX_3_072711026x0	=	0x6254000062540000;

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

#define	DATASIZE	32 
 /*
 * Perform the forward DCT on one block of samples.
 */

GLOBAL(void)
mfdct8x8llm (DCTELEM * data)
{
	__int64 qwTemp0, qwTemp2, qwTemp4, qwTemp6;
	__int64 qwZ1, qwZ2, qwZ4_even, qwZ4_odd;
	__int64 qwTmp4_Z3_Even, qwTmp4_Z3_Odd;
	__int64 qwTmp6_Z3_Even, qwTmp6_Z3_Odd;
	__int64 qwTmp5_Z4_Even, qwTmp5_Z4_Odd;
	__int64 qwScratch7, qwScratch6, qwScratch5;

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
		movq		mm2, mm6			// copy w0---0,1,3,5,6

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
		punpckhwd	mm2, mm4			//---0,1,3,5,6 

		packssdw mm1, mm0
		movq		mm5, mm3			//---0,1,2,3,5,6 w2

		punpcklwd	mm3, mm1			//mm1 = w3
		movq		mm0, mm6			//---0,2,3,4,5,6,7

		movq		mm4, [edi][DATASIZE*7]
		punpckhwd	mm5, mm1			//---0,2,3,5,6,7

		movq		mm1, [edi][DATASIZE*4]
		punpckhdq	mm6, mm3			// transposed w4

		punpckldq	mm0, mm3			// transposed w5---0,2,4,6,7
		movq		mm3, mm2			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*0], mm0  // store w4
		punpckldq	mm2, mm5			// transposed w6

		movq	[edi][DATASIZE*1], mm6  // store w5
		punpckhdq	mm3, mm5			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*2], mm2  // store w6---3,5,6,7	
		paddw	mm0, mm4

		movq	[edi][DATASIZE*3], mm3  // store w7---5,6,7
		paddw	mm3, mm1


	//******************************************************************************
	// End of transpose.  Begin row dct.
	//******************************************************************************

	//		tmp0 = dataptr[DATASIZE*0] + dataptr[DATASIZE*7];

		movq	mm7, mm0
		paddw	mm0, mm3	//tmp10

		paddw	mm6, [edi][DATASIZE*6]
		psubw	mm7, mm3	//tmp13

		paddw	mm2, [edi][DATASIZE*5]
		movq	mm1, mm6

	//		tmp10 = tmp0 + tmp3;

		paddw	mm1, mm2	//tmp11
		psubw	mm6, mm2	//tmp12

	//    dataptr[0] = (DCTELEM) ((tmp10 + tmp11) << PASS1_BITS);
	//    dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << PASS1_BITS);

		movq	mm3, mm0	
		paddw	mm0, mm1	//tmp10 + tmp11	

		psubw	mm3, mm1	//tmp10 - tmp11
		psllw	mm0, 2			// descale it

 		movq	mm1, mm6	//copy tmp12
		psllw	mm3, 2			// descale it

	//		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);

		movq	qwTemp0, mm0	//store 
		paddw	mm1, mm7	//tmp12 + tmp13

		movq	mm2, mm1	//copy

	//		dataptr[2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
	//					   CONST_BITS-PASS1_BITS);
	//		dataptr[6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
	//					   CONST_BITS-PASS1_BITS);

		pmaddwd	mm1, Const_0xFIX_0_541196100	//| z12 | z10 |
		movq	mm4, mm7

		pmaddwd	mm7, Const_0xFIX_0_765366865	//| r2 | r0 |
		movq	mm0, mm6

		pmaddwd	mm2, Const_FIX_0_541196100x0	//| z13 | z11 |

		pmaddwd	mm4, Const_FIX_0_765366865x0	//| r3 | r1 |

		pmaddwd	mm6, Const_0xFIX_1_847759065	//| r2 | r0 |
		paddd	mm7, mm1						// add z1

		pmaddwd	mm0, Const_FIX_1_847759065x0	//| r3 | r1 |

		paddd	mm7, Const_1024
		paddd	mm4, mm2

		paddd	mm4, Const_1024
		psrad	mm7, 11				// descale it |  |R2|  |R0|
		
	//!!!!!! Negate the results in mm6 and mm0
		pxor	mm6, Const_FFFF			//invert result
		psrad	mm4, 11				// descale it |  |R3|  |R1|

		paddd	mm6, Const_1			// 2's complement
		movq	mm5, mm7

		pxor	mm0, Const_FFFF			//invert result
		punpckldq mm7, mm4			//|  |R1|  |R0|

		paddd	mm0, Const_1			// 2's complement
		punpckhdq mm5, mm4			//|  |R3|  |R2|

		movq	qwTemp4, mm3	//store
		packssdw mm7, mm5

		movq	mm5, Const_1024
		paddd	mm6, mm1						// add z1

		movq	qwTemp2, mm7	//store
		paddd	mm6, mm5

		paddd	mm0, mm2
		psrad	mm6, 11				// descale it |  |R2|  |R0|

		paddd	mm0, mm5
		movq	mm5, mm6
		
		movq	mm4, [edi][DATASIZE*3]
		psrad	mm0, 11				// descale it |  |R3|  |R1|

		psubw	mm4, [edi][DATASIZE*4]
		punpckldq mm6, mm0			//|  |R1|  |R0|

		movq	mm7, [edi][DATASIZE*0]
		punpckhdq mm5, mm0			//|  |R3|  |R2|

		psubw	mm7, [edi][DATASIZE*7]
		packssdw mm6, mm5

	//		tmp4 = dataptr[3] - dataptr[4];

		movq	mm5, [edi][DATASIZE*2]
		movq	mm0, mm4

		psubw	mm5, [edi][DATASIZE*5]
		movq	mm2, mm4

		movq	qwTemp6, mm6	//store
		paddw	mm0, mm7	//z1

		movq	mm6, [edi][DATASIZE*1]
		movq	mm1, mm5

		psubw	mm6, [edi][DATASIZE*6]
		movq	mm3, mm5

	//		z1 = tmp4 + tmp7;

		movq	qwScratch5, mm5
		paddw	mm3, mm7	//z4

		movq	qwScratch7, mm7
		paddw	mm2, mm6	//z3

		movq	qwZ1, mm0	//store
		paddw	mm1, mm6	//z2

	//	    z3 = MULTIPLY(z3, - FIX_1_961570560);
	//	    z4 = MULTIPLY(z4, - FIX_0_390180644);
	//	    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);

		movq	mm0, Const_FFFF
		movq	mm5, mm2

		movq	qwZ2, mm1
		movq	mm7, mm2

		pmaddwd	mm5, Const_0xFIX_1_961570560	//z32, z30
		paddw	mm2, mm3		//z3 + z4

		pmaddwd	mm7, Const_FIX_1_961570560x0	//z33, z31
		movq	mm1, mm3

		movq	qwScratch6, mm6
		movq	mm6, mm2

	//	    z3 += z5;

	//!!!!!! Negate the results
		pmaddwd	mm2, Const_0xFIX_1_175875602	//z52, z50
		pxor	mm5, mm0			//invert result
		
		paddd	mm5, Const_1			// 2's complement
		pxor	mm7, mm0			//invert result

		pmaddwd	mm3, Const_0xFIX_0_390180644	//z42, z40

		pmaddwd	mm1, Const_FIX_0_390180644x0	//z43, z41
		paddd	mm5, mm2	//z3_even

		paddd	mm7, Const_1			// 2's complement

		pmaddwd	mm6, Const_FIX_1_175875602x0	//z53, z51
		pxor	mm3, mm0			//invert result

	//	    z4 += z5;

	//!!!!!! Negate the results
		paddd	mm3, Const_1			// 2's complement
		pxor	mm1, mm0			//invert result

		paddd	mm1, Const_1			// 2's complement
		paddd	mm3, mm2

		movq	mm0, qwScratch6
		movq	mm2, mm4

	//	    tmp4 = MULTIPLY(tmp4, FIX_0_298631336);

		pmaddwd	mm4, Const_0xFIX_0_298631336	//T42, T40
		paddd	mm7, mm6	//z3_odd

		pmaddwd	mm2, Const_FIX_0_298631336x0	//T43, T41
		paddd	mm1, mm6
		
		movq	mm6, mm0
		paddd	mm4, mm5

	//	    tmp6 = MULTIPLY(tmp6, FIX_3_072711026);

		pmaddwd	mm6, Const_0xFIX_3_072711026	//T62, T60
		paddd	mm2, mm7

		pmaddwd	mm0, Const_FIX_3_072711026x0	//T63, T61

		movq	qwTmp4_Z3_Odd, mm2	

		movq	qwTmp4_Z3_Even, mm4	
		paddd	mm6, mm5

		movq	mm5, qwScratch5
		paddd	mm0, mm7
		
		movq	mm7, qwScratch7
		movq	mm2, mm5

		movq	qwTmp6_Z3_Even, mm6
		movq	mm6, mm7
			
	//	    tmp5 = MULTIPLY(tmp5, FIX_2_053119869);		
	//	    tmp7 = MULTIPLY(tmp7, FIX_1_501321110);

		pmaddwd	mm5, Const_0xFIX_2_053119869	//T52, T50

		pmaddwd	mm2, Const_FIX_2_053119869x0	//T53, T51

		pmaddwd	mm7, Const_0xFIX_1_501321110	//T72, T70

		pmaddwd	mm6, Const_FIX_1_501321110x0	//T73, T71
		paddd	mm5, mm3

		movq	qwTmp6_Z3_Odd, mm0
		paddd	mm2, mm1
		
		movq	qwTmp5_Z4_Even, mm5
		paddd	mm7, mm3
			
		movq	mm0, qwZ1
		paddd	mm6, mm1
		
	//	    z1 = MULTIPLY(z1, - FIX_0_899976223);

		movq	mm1, Const_FFFF
		movq	mm4, mm0

	//!!!!!! Negate the results
		pmaddwd	mm0, Const_0xFIX_0_899976223	//z12, z10

		pmaddwd	mm4, Const_FIX_0_899976223x0	//z13, z11

		movq	mm3, qwTmp4_Z3_Even

		movq	qwTmp5_Z4_Odd, mm2
		pxor	mm0, mm1			//invert result

		movq	mm2, qwTmp4_Z3_Odd
		pxor	mm4, mm1			//invert result

		paddd	mm4, Const_1			// 2's complement
		paddd	mm7, mm0	//tmp7 + z1 + z4 EVEN

		paddd	mm0, Const_1			// 2's complement
		paddd	mm6, mm4	//tmp7 + z1 + z4 ODD

	//	    dataptr[1] = (DCTELEM) DESCALE(tmp7 + z1 + z4, CONST_BITS-PASS1_BITS);

		paddd	mm7, Const_1024		//rounding adj
		paddd	mm3, mm0	//tmp4 + z1 + z3 EVEN

		paddd	mm6, Const_1024		//rounding adj
		psrad	mm7, 11				// descale it |  |R2|  |R0|
		
		psrad	mm6, 11				// descale it |  |R3|  |R1|

		movq	mm5, mm7
		punpckldq mm7, mm6			//|  |R1|  |R0|

	//	    dataptr[7] = (DCTELEM) DESCALE(tmp4 + z1 + z3, CONST_BITS-PASS1_BITS);

		punpckhdq mm5, mm6			//|  |R3|  |R2|
		paddd	mm2, mm4	//tmp4 + z1 + z3 ODD

		paddd	mm3, Const_1024	//rounding adj
		packssdw mm7, mm5

		paddd	mm2, Const_1024	//rounding adj
		psrad	mm3, 11				// descale it |  |R2|  |R0|
		
		movq	mm0, qwZ2
		psrad	mm2, 11				// descale it |  |R3|  |R1|

		movq	mm5, mm3
		movq	mm4, mm0

	//	    z2 = MULTIPLY(z2, - FIX_2_562915447);

		pmaddwd	mm0, Const_0xFIX_2_562915447	//z22, z20
		punpckldq mm3, mm2			//|  |R1|  |R0|

		pmaddwd	mm4, Const_FIX_2_562915447x0	//z23, z21
		punpckhdq mm5, mm2			//|  |R3|  |R2|

		movq	mm2, Const_FFFF
		packssdw mm3, mm5

		movq	[edi][DATASIZE*1], mm7	//store
	//!!!!!! Negate the results
		pxor	mm0, mm2			//invert result

		movq	mm5, Const_1
		pxor	mm4, mm2			//invert result

		movq	[edi][DATASIZE*7], mm3	//store
		paddd	mm0, mm5			// 2's complement

		movq	mm7, qwTmp6_Z3_Even
		paddd	mm4, mm5			// 2's complement

	//	    dataptr[3] = (DCTELEM) DESCALE(tmp6 + z2 + z3, CONST_BITS-PASS1_BITS);

		movq	mm2, qwTmp6_Z3_Odd
		paddd	mm7, mm0	//tmp6 + z2 + z3 EVEN

		paddd	mm7, Const_1024		//rounding adj
		paddd	mm2, mm4	//tmp6 + z2 + z3 ODD

		paddd	mm2, Const_1024		//rounding adj
		psrad	mm7, 11				// descale it |  |R2|  |R0|
		
		movq	mm6, qwTemp0	//restore 
		psrad	mm2, 11				// descale it |  |R3|  |R1|

		movq	mm3, qwTmp5_Z4_Even
		movq	mm5, mm7

		movq	[edi][DATASIZE*0], mm6	//store 
		punpckldq mm7, mm2			//|  |R1|  |R0|

		movq	mm1, qwTmp5_Z4_Odd
		punpckhdq mm5, mm2			//|  |R3|  |R2|

		movq	mm6, qwTemp2	//restore 
		packssdw mm7, mm5

		movq	mm5, Const_1024
		paddd	mm3, mm0	//tmp5 + z2 + z4 EVEN

	//	    dataptr[5] = (DCTELEM) DESCALE(tmp5 + z2 + z4, CONST_BITS-PASS1_BITS);

		movq	[edi][DATASIZE*3], mm7	//store
		paddd	mm1, mm4	//tmp5 + z2 + z4 ODD

		movq	mm7, qwTemp4	//restore 
		paddd	mm3, mm5		//rounding adj

		movq	[edi][DATASIZE*2], mm6	//store 
		paddd	mm1, mm5		//rounding adj

		movq	[edi][DATASIZE*4], mm7	//store 
		psrad	mm3, 11				// descale it |  |R2|  |R0|
		
		movq	mm6, qwTemp6	//restore 
		psrad	mm1, 11				// descale it |  |R3|  |R1|

		movq	mm0, [edi][DATASIZE*0+16]
		movq	mm5, mm3

		movq	[edi][DATASIZE*6], mm6	//store 
		punpckldq mm3, mm1			//|  |R1|  |R0|

		paddw	mm0, [edi][DATASIZE*7+16]
		punpckhdq mm5, mm1			//|  |R3|  |R2|

		movq	mm1, [edi][DATASIZE*1+16]
		packssdw mm3, mm5

		paddw	mm1, [edi][DATASIZE*6+16]
		movq	mm7, mm0

		movq	[edi][DATASIZE*5], mm3	//store
		movq	mm6, mm1

	//******************************************************************************
	// This completes 4x8 dct locations.  Copy to do other 4x8.
	//******************************************************************************

	//		tmp0 = dataptr[DATASIZE*0] + dataptr[DATASIZE*7];

		movq	mm3, [edi][DATASIZE*3+16]

		paddw	mm3, [edi][DATASIZE*4+16]

		movq	mm2, [edi][DATASIZE*2+16]
		paddw	mm0, mm3	//tmp10

		paddw	mm2, [edi][DATASIZE*5+16]
		psubw	mm7, mm3	//tmp13

	//		tmp10 = tmp0 + tmp3;

		paddw	mm1, mm2	//tmp11
		psubw	mm6, mm2	//tmp12

	//    dataptr[0] = (DCTELEM) ((tmp10 + tmp11) << PASS1_BITS);
	//    dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << PASS1_BITS);

		movq	mm3, mm0	
		paddw	mm0, mm1	//tmp10 + tmp11	

		psubw	mm3, mm1	//tmp10 - tmp11
		psllw	mm0, 2			// descale it

 		movq	mm1, mm6	//copy tmp12
		psllw	mm3, 2			// descale it

	//		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);

		movq	qwTemp0, mm0	//store 
		paddw	mm1, mm7	//tmp12 + tmp13

	//;;; 	movq	[edi][DATASIZE*6+16], mm4  ; store w6---3,5,6,7	
		movq	mm2, mm1	//copy

	//		dataptr[2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
	//					   CONST_BITS-PASS1_BITS);
	//		dataptr[6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
	//					   CONST_BITS-PASS1_BITS);

		pmaddwd	mm1, Const_0xFIX_0_541196100	//| z12 | z10 |
		movq	mm4, mm7

		pmaddwd	mm7, Const_0xFIX_0_765366865	//| r2 | r0 |
		movq	mm0, mm6

		pmaddwd	mm2, Const_FIX_0_541196100x0	//| z13 | z11 |

		pmaddwd	mm4, Const_FIX_0_765366865x0	//| r3 | r1 |

		pmaddwd	mm6, Const_0xFIX_1_847759065	//| r2 | r0 |
		paddd	mm7, mm1						// add z1

		pmaddwd	mm0, Const_FIX_1_847759065x0	//| r3 | r1 |

		paddd	mm7, Const_1024
		paddd	mm4, mm2

		paddd	mm4, Const_1024
		psrad	mm7, 11				// descale it |  |R2|  |R0|
		
	//!!!!!! Negate the results in mm6 and mm0
		pxor	mm6, Const_FFFF			//invert result
		psrad	mm4, 11				// descale it |  |R3|  |R1|

		paddd	mm6, Const_1			// 2's complement
		movq	mm5, mm7

		pxor	mm0, Const_FFFF			//invert result
		punpckldq mm7, mm4			//|  |R1|  |R0|

		paddd	mm0, Const_1			// 2's complement
		punpckhdq mm5, mm4			//|  |R3|  |R2|

		movq	qwTemp4, mm3	//store
		packssdw mm7, mm5

		movq	mm5, Const_1024
		paddd	mm6, mm1						// add z1
		
		movq	qwTemp2, mm7	//store
		paddd	mm0, mm2

		movq	mm4, [edi][DATASIZE*3+16]
		paddd	mm6, mm5

		psubw	mm4, [edi][DATASIZE*4+16]
		psrad	mm6, 11				// descale it |  |R2|  |R0|
		
		paddd	mm0, mm5
		movq	mm5, mm6

		movq	mm7, [edi][DATASIZE*0+16]
		psrad	mm0, 11				// descale it |  |R3|  |R1|

		psubw	mm7, [edi][DATASIZE*7+16]
		punpckldq mm6, mm0			//|  |R1|  |R0|

		punpckhdq mm5, mm0			//|  |R3|  |R2|
		movq	mm0, mm4

		packssdw mm6, mm5
		movq	mm2, mm4

	//		tmp4 = dataptr[3] - dataptr[4];

		movq	mm5, [edi][DATASIZE*2+16]
		paddw	mm0, mm7	//z1

		psubw	mm5, [edi][DATASIZE*5+16]

		movq	qwTemp6, mm6	//store
		movq	mm1, mm5

		movq	mm6, [edi][DATASIZE*1+16]
		movq	mm3, mm5

	//		z1 = tmp4 + tmp7;

		psubw	mm6, [edi][DATASIZE*6+16]
		paddw	mm3, mm7	//z4

		movq	qwScratch7, mm7
		paddw	mm2, mm6	//z3

		movq	qwScratch5, mm5
		paddw	mm1, mm6	//z2

	//	    z3 = MULTIPLY(z3, - FIX_1_961570560);
	//	    z4 = MULTIPLY(z4, - FIX_0_390180644);
	//	    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);

		movq	qwZ1, mm0	//store
		movq	mm5, mm2

		movq	qwZ2, mm1
		movq	mm7, mm2

		movq	mm0, Const_FFFF
		paddw	mm2, mm3		//z3 + z4

		pmaddwd	mm5, Const_0xFIX_1_961570560	//z32, z30
		movq	mm1, mm3

		pmaddwd	mm7, Const_FIX_1_961570560x0	//z33, z31

		movq	qwScratch6, mm6
		movq	mm6, mm2

	//	    z3 += z5//

	//!!!!!! Negate the results
		pmaddwd	mm2, Const_0xFIX_1_175875602	//z52, z50
		pxor	mm5, mm0			//invert result
		
		paddd	mm5, Const_1			// 2's complement
		pxor	mm7, mm0			//invert result

		pmaddwd	mm3, Const_0xFIX_0_390180644	//z42, z40

		pmaddwd	mm1, Const_FIX_0_390180644x0	//z43, z41
		paddd	mm5, mm2	//z3_even

		paddd	mm7, Const_1			// 2's complement

		pmaddwd	mm6, Const_FIX_1_175875602x0	//z53, z51
		pxor	mm3, mm0			//invert result

	//	    z4 += z5;

	//!!!!!! Negate the results
		paddd	mm3, Const_1			// 2's complement
		pxor	mm1, mm0			//invert result

		paddd	mm1, Const_1			// 2's complement
		paddd	mm3, mm2

		movq	mm0, qwScratch6
		movq	mm2, mm4

	//	    tmp4 = MULTIPLY(tmp4, FIX_0_298631336);

		pmaddwd	mm4, Const_0xFIX_0_298631336	//T42, T40
		paddd	mm7, mm6	//z3_odd

		pmaddwd	mm2, Const_FIX_0_298631336x0	//T43, T41
		paddd	mm1, mm6
		
		movq	mm6, mm0
		paddd	mm4, mm5

	//	    tmp6 = MULTIPLY(tmp6, FIX_3_072711026);

		pmaddwd	mm6, Const_0xFIX_3_072711026	//T62, T60
		paddd	mm2, mm7

		pmaddwd	mm0, Const_FIX_3_072711026x0	//T63, T61

		movq	qwTmp4_Z3_Odd, mm2	

		movq	qwTmp4_Z3_Even, mm4	
		paddd	mm6, mm5

		movq	mm5, qwScratch5
		paddd	mm0, mm7
		
		movq	mm7, qwScratch7
		movq	mm2, mm5

		movq	qwTmp6_Z3_Even, mm6
		movq	mm6, mm7
			
	//	    tmp5 = MULTIPLY(tmp5, FIX_2_053119869);		
	//	    tmp7 = MULTIPLY(tmp7, FIX_1_501321110);

		pmaddwd	mm5, Const_0xFIX_2_053119869	//T52, T50

		pmaddwd	mm2, Const_FIX_2_053119869x0	//T53, T51

		pmaddwd	mm7, Const_0xFIX_1_501321110	//T72, T70

		pmaddwd	mm6, Const_FIX_1_501321110x0	//T73, T71
		paddd	mm5, mm3

		movq	qwTmp6_Z3_Odd, mm0
		paddd	mm2, mm1
		
		movq	qwTmp5_Z4_Even, mm5
		paddd	mm7, mm3
			
		movq	mm0, qwZ1
		paddd	mm6, mm1
		
	//	    z1 = MULTIPLY(z1, - FIX_0_899976223);

		movq	mm1, Const_FFFF
		movq	mm4, mm0

	//!!!!!! Negate the results
		pmaddwd	mm0, Const_0xFIX_0_899976223	//z12, z10

		pmaddwd	mm4, Const_FIX_0_899976223x0	//z13, z11

		movq	mm3, qwTmp4_Z3_Even

		movq	qwTmp5_Z4_Odd, mm2
		pxor	mm0, mm1			//invert result

		movq	mm2, qwTmp4_Z3_Odd
		pxor	mm4, mm1			//invert result

		paddd	mm4, Const_1			// 2's complement
		paddd	mm7, mm0	//tmp7 + z1 + z4 EVEN

		paddd	mm0, Const_1			// 2's complement
		paddd	mm6, mm4	//tmp7 + z1 + z4 ODD

	//	    dataptr[1] = (DCTELEM) DESCALE(tmp7 + z1 + z4, CONST_BITS-PASS1_BITS);

		paddd	mm7, Const_1024		//rounding adj
		paddd	mm3, mm0	//tmp4 + z1 + z3 EVEN

		paddd	mm6, Const_1024		//rounding adj
		psrad	mm7, 11				// descale it |  |R2|  |R0|
		
		psrad	mm6, 11				// descale it |  |R3|  |R1|

		movq	mm5, mm7
		punpckldq mm7, mm6			//|  |R1|  |R0|

	//	    dataptr[7] = (DCTELEM) DESCALE(tmp4 + z1 + z3, CONST_BITS-PASS1_BITS);

		punpckhdq mm5, mm6			//|  |R3|  |R2|
		paddd	mm2, mm4	//tmp4 + z1 + z3 ODD

		paddd	mm3, Const_1024	//rounding adj
		packssdw mm7, mm5

		paddd	mm2, Const_1024	//rounding adj
		psrad	mm3, 11				// descale it |  |R2|  |R0|
		
		movq	mm0, qwZ2
		psrad	mm2, 11				// descale it |  |R3|  |R1|

		movq	mm5, mm3
		movq	mm4, mm0

	//	    z2 = MULTIPLY(z2, - FIX_2_562915447);

		pmaddwd	mm0, Const_0xFIX_2_562915447	//z22, z20
		punpckldq mm3, mm2			//|  |R1|  |R0|

		pmaddwd	mm4, Const_FIX_2_562915447x0	//z23, z21
		punpckhdq mm5, mm2			//|  |R3|  |R2|

		movq	mm2, Const_FFFF
		packssdw mm3, mm5

		movq	[edi][DATASIZE*1+16], mm7	//store
	//!!!!!! Negate the results
		pxor	mm0, mm2			//invert result

		movq	mm5, Const_1
		pxor	mm4, mm2			//invert result

		movq	[edi][DATASIZE*7+16], mm3	//store
		paddd	mm0, mm5			// 2's complement

		movq	mm7, qwTmp6_Z3_Even
		paddd	mm4, mm5			// 2's complement

	//	    dataptr[3] = (DCTELEM) DESCALE(tmp6 + z2 + z3, CONST_BITS-PASS1_BITS);

		movq	mm2, qwTmp6_Z3_Odd
		paddd	mm7, mm0	//tmp6 + z2 + z3 EVEN

		paddd	mm7, Const_1024		//rounding adj
		paddd	mm2, mm4	//tmp6 + z2 + z3 ODD

		paddd	mm2, Const_1024		//rounding adj
		psrad	mm7, 11				// descale it |  |R2|  |R0|
		
		movq	mm6, qwTemp0	//restore 
		psrad	mm2, 11				// descale it |  |R3|  |R1|

		movq	mm5, mm7

		movq	[edi][DATASIZE*0+16], mm6	//store 
		punpckldq mm7, mm2			//|  |R1|  |R0|

		movq	mm3, qwTmp5_Z4_Even
		punpckhdq mm5, mm2			//|  |R3|  |R2|

		movq	mm1, qwTmp5_Z4_Odd
		packssdw mm7, mm5

		movq	mm6, qwTemp2	//restore 
		paddd	mm3, mm0	//tmp5 + z2 + z4 EVEN

	//	    dataptr[5] = (DCTELEM) DESCALE(tmp5 + z2 + z4, CONST_BITS-PASS1_BITS);

		movq	mm0, Const_1024
		paddd	mm1, mm4	//tmp5 + z2 + z4 ODD

		movq	[edi][DATASIZE*3+16], mm7	//store
		paddd	mm3, mm0		//rounding adj

		movq	mm7, qwTemp4	//restore 
		paddd	mm1, mm0		//rounding adj

		movq	[edi][DATASIZE*2+16], mm6	//store 
		psrad	mm3, 11				// descale it |  |R2|  |R0|
		
		movq	mm6, qwTemp6	//restore 
		psrad	mm1, 11				// descale it |  |R3|  |R1|

		movq	[edi][DATASIZE*4+16], mm7	//store 
		movq	mm5, mm3

		movq	[edi][DATASIZE*6+16], mm6	//store 
		punpckldq mm3, mm1			//|  |R1|  |R0|

		punpckhdq mm5, mm1			//|  |R3|  |R2|
		movq		mm0, mm7			// copy w4---0,1,3,5,6

		movq	mm1, [edi][DATASIZE*7+16]
		packssdw mm3, mm5

		movq	[edi][DATASIZE*5+16], mm3	//store
		punpcklwd	mm7, mm3			//mm6 = w5

	//******************************************************************************

	//******************************************************************************
	// This completes all 8x8 dct locations for the row case.
	// Now transpose the data for the columns.
	//******************************************************************************

	// transpose the bottom right quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq		mm4, mm7			//---0,2,3,4,5,6,7
		punpckhwd	mm0, mm3			//---0,1,3,5,6 

		movq		mm2, mm6			//---0,1,2,3,5,6 w6
		punpcklwd	mm6, mm1			//mm1 = w7

	//		tmp0 = dataptr[DATASIZE*0] + dataptr[DATASIZE*7]//

		movq	mm5, [edi][DATASIZE*5]
		punpckldq	mm7, mm6			// transposed w4

		punpckhdq	mm4, mm6			// transposed w5---0,2,4,6,7
		movq		mm6, mm0			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*4+16], mm7  // store w4
		punpckhwd	mm2, mm1			//---0,2,3,5,6,7

		movq	[edi][DATASIZE*5+16], mm4  // store w5
		punpckldq	mm0, mm2			// transposed w6

		movq	mm7, [edi][DATASIZE*4]
		punpckhdq	mm6, mm2			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*6+16], mm0  // store w6---3,5,6,7	
		movq		mm0, mm7			// copy w0---0,1,3,5,6

		movq	[edi][DATASIZE*7+16], mm6  // store w7---5,6,7
		punpcklwd	mm7, mm5			//mm6 = w1

	// transpose the bottom left quadrant(4X4) of the matrix and place
	// in the top right quadrant while doing the same for the top
	// right quadrant
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq	mm3, [edi][DATASIZE*6]
		punpckhwd	mm0, mm5			//---0,1,3,5,6 

		movq	mm1, [edi][DATASIZE*7]
		movq		mm2, mm3			//---0,1,2,3,5,6 w2

		movq		mm6, [edi][DATASIZE*0+16]
		punpcklwd	mm3, mm1			//mm1 = w3

		movq		mm5, [edi][DATASIZE*1+16]
		punpckhwd	mm2, mm1			//---0,2,3,5,6,7

		movq		mm4, mm7			//---0,2,3,4,5,6,7
		punpckldq	mm7, mm3			// transposed w4

		punpckhdq	mm4, mm3			// transposed w5---0,2,4,6,7
		movq		mm3, mm0			//---0,2,3,4,6,7

		movq	[edi][DATASIZE*0+16], mm7  // store w4
		punpckldq	mm0, mm2			// transposed w6

		movq		mm1, [edi][DATASIZE*2+16]
		punpckhdq	mm3, mm2			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*2+16], mm0  // store w6---3,5,6,7	
		movq		mm0, mm6			// copy w4---0,1,3,5,6

		movq		mm7, [edi][DATASIZE*3+16]
		punpcklwd	mm6, mm5			//mm6 = w5

		movq	[edi][DATASIZE*1+16], mm4  // store w5
		punpckhwd	mm0, mm5			//---0,1,3,5,6 

	// transpose the top right quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq		mm2, mm1			//---0,1,2,3,5,6 w6
		punpcklwd	mm1, mm7			//mm1 = w7

		movq		mm4, mm6			//---0,2,3,4,5,6,7
		punpckldq	mm6, mm1			// transposed w4

		movq	[edi][DATASIZE*3+16], mm3  // store w7---5,6,7
		punpckhdq	mm4, mm1			// transposed w5---0,2,4,6,7

		movq	[edi][DATASIZE*4], mm6  // store w4
		punpckhwd	mm2, mm7			//---0,2,3,5,6,7

		movq	mm7, [edi][DATASIZE*0]
		movq		mm1, mm0			//---0,2,3,4,6,7

		movq	mm3, [edi][DATASIZE*1]
		punpckldq	mm0, mm2			// transposed w6

		movq	[edi][DATASIZE*5], mm4  // store w5
		punpckhdq	mm1, mm2			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*6], mm0  // store w6---3,5,6,7	
		movq		mm2, mm7			// copy w0---0,1,3,5,6

		movq	mm4, [edi][DATASIZE*3]
		punpcklwd	mm7, mm3			//mm6 = w1

	// transpose the top left quadrant(4X4) of the matrix
	//  ---------       ---------
	// | M1 | M2 |     | M1'| M3'|
	//  ---------  -->  ---------
	// | M3 | M4 |     | M2'| M4'|
	//  ---------       ---------

		movq	mm6, [edi][DATASIZE*2]
		punpckhwd	mm2, mm3			//---0,1,3,5,6 

		movq		mm0, mm6			//---0,1,2,3,5,6 w2
		punpcklwd	mm6, mm4			//mm1 = w3

		movq	[edi][DATASIZE*7], mm1  // store w7---5,6,7
		punpckhwd	mm0, mm4			//---0,2,3,5,6,7

		movq		mm1, mm7			//---0,2,3,4,5,6,7
		punpckldq	mm7, mm6			// transposed w4

		punpckhdq	mm1, mm6			// transposed w5---0,2,4,6,7
		movq		mm6, mm2			//---0,2,3,4,6,7

 		movq	[edi][DATASIZE*0], mm7  // store w4
		punpckldq	mm2, mm0			// transposed w6

		paddw	mm7, [edi][DATASIZE*7]
		punpckhdq	mm6, mm0			// transposed w7---0,3,6,7

		movq	[edi][DATASIZE*3], mm6  // store w7---5,6,7
		movq	mm4, mm7

		paddw	mm6, [edi][DATASIZE*4]

		movq	[edi][DATASIZE*1], mm1  // store w5
		paddw	mm7, mm6	//tmp10


	//******************************************************************************
	// This begins the column dct
	//******************************************************************************

		paddw	mm1, [edi][DATASIZE*6]
		psubw	mm4, mm6	//tmp13

		movq	[edi][DATASIZE*2], mm2  // store w6---3,5,6,7	
		movq	mm6, mm1

		paddw	mm2, [edi][DATASIZE*5]
		movq	mm3, mm7	

		paddw	mm1, mm2	//tmp11
		psubw	mm6, mm2	//tmp12

	//    dataptr[DATASIZE*0] = (DCTELEM) DESCALE(tmp10 + tmp11, PASS1_BITS);
	//    dataptr[DATASIZE*4] = (DCTELEM) DESCALE(tmp10 - tmp11, PASS1_BITS);

		paddw	mm7, mm1	//tmp10 + tmp11	

		paddw	mm7, Const_2	// round  add 2 to each element
		psubw	mm3, mm1	//tmp10 - tmp11

		paddw	mm3, Const_2	// round  add 2 to each element
		psraw	mm7, 2			// descale it

	//		unpack word to dword sign extended
		movq	mm5, mm7
		punpcklwd mm7, mm7

		psrad	mm7, 16			// even results store in Temp0			
		punpckhwd mm5, mm5

		psrad	mm5, 16			// odd results store in array
		movq	mm1, mm6	//copy tmp12

		movq	qwTemp0, mm7	//store 
		psraw	mm3, 2			// descale it

		movq	[edi][DATASIZE*0+8], mm5
		movq	mm5, mm3

		punpcklwd mm3, mm3
		paddw	mm1, mm4	//tmp12 + tmp13

		psrad	mm3, 16			// even results store in Temp4
		movq	mm2, mm1	//copy
					
	//		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);

		pmaddwd	mm1, Const_0xFIX_0_541196100	//| z12 | z10 |
		punpckhwd mm5, mm5

		pmaddwd	mm2, Const_FIX_0_541196100x0	//| z13 | z11 |
		movq	mm7, mm4

	//		dataptr[DATASIZE*2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
	//					   CONST_BITS+PASS1_BITS);

		pmaddwd	mm4, Const_FIX_0_765366865x0	//| r3 | r1 |
		psrad	mm5, 16			// odd results store in array

		pmaddwd	mm7, Const_0xFIX_0_765366865	//| r2 | r0 |
		movq	mm0, mm6

	//		dataptr[DATASIZE*6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
	//					   CONST_BITS+PASS1_BITS);

		pmaddwd	mm6, Const_0xFIX_1_847759065	//| r2 | r0 |

		movq	qwTemp4, mm3	//store
		paddd	mm4, mm2

		paddd	mm4, Const_16384
		paddd	mm7, mm1						// add z1

		paddd	mm7, Const_16384
		psrad	mm4, 15				// descale it |  |R3|  |R1|

		movq	[edi][DATASIZE*4+8], mm5
		psrad	mm7, 15				// descale it |  |R2|  |R0|
		
		pmaddwd	mm0, Const_FIX_1_847759065x0	//| r3 | r1 |
		movq	mm5, mm7

	//!!!!!! Negate result
		movq	mm3, Const_1
		punpckldq mm7, mm4			//|  |R1|  |R0|

		pxor	mm6, Const_FFFF			//invert result
		punpckhdq mm5, mm4			//|  |R3|  |R2|

		movq	qwTemp2, mm7	//store
		paddd	mm6, mm3			// 2's complement

		pxor	mm0, Const_FFFF			//invert result
		paddd	mm6, mm1						// add z1

		movq	[edi][DATASIZE*2+8], mm5	//write out 2nd half in unused memory
		paddd	mm0, mm3			// 2's complement

		movq	mm3, Const_16384
		paddd	mm0, mm2

		movq	mm7, [edi][DATASIZE*0]
		paddd	mm6, mm3

		movq	mm4, [edi][DATASIZE*3]
		paddd	mm0, mm3

		psubw	mm7, [edi][DATASIZE*7]
		psrad	mm6, 15				// descale it |  |R2|  |R0|
		
		psubw	mm4, [edi][DATASIZE*4]
		psrad	mm0, 15				// descale it |  |R3|  |R1|

		movq	mm3, [edi][DATASIZE*2]
		movq	mm5, mm6

		psubw	mm3, [edi][DATASIZE*5]
		punpckldq mm6, mm0			//|  |R1|  |R0|

		punpckhdq mm5, mm0			//|  |R3|  |R2|
		movq	mm0, mm4

		movq	qwTemp6, mm6	//store
		movq	mm2, mm4

	//		tmp4 = dataptr[3] - dataptr[4];
	//		z1 = tmp4 + tmp7;

		movq	mm6, [edi][DATASIZE*1]
		paddw	mm0, mm7	//z1

		movq	[edi][DATASIZE*6+8], mm5	//write out 2nd half in unused memory
		movq	mm1, mm3
		
		psubw	mm6, [edi][DATASIZE*6]
		movq	mm5, mm3

		movq	qwZ1, mm0	//store
		paddw	mm5, mm7	//z4

		movq	qwScratch7, mm7
		paddw	mm1, mm6	//z2

		movq	qwScratch5, mm3
		paddw	mm2, mm6	//z3

		movq	qwZ2, mm1
		movq	mm3, mm2

	//	    z3 = MULTIPLY(z3, - FIX_1_961570560);
	//	    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);
	//	    z4 = MULTIPLY(z4, - FIX_0_390180644);

		movq	qwScratch6, mm6
		movq	mm1, mm2

		pmaddwd	mm3, Const_0xFIX_1_961570560	//z32, z30
		movq	mm7, mm5

		movq	mm6, Const_FFFF
		paddw	mm2, mm5		//z3 + z4

		pmaddwd	mm1, Const_FIX_1_961570560x0	//z33, z31
		movq	mm0, mm2
		
		pmaddwd	mm7, Const_FIX_0_390180644x0	//z43, z41
	//!!!!!! Negate the results
		pxor	mm3, mm6			//invert result

		pmaddwd	mm5, Const_0xFIX_0_390180644	//z42, z40

		pmaddwd	mm2, Const_0xFIX_1_175875602	//z52, z50
 		pxor	mm1, mm6			//invert result

		pmaddwd	mm0, Const_FIX_1_175875602x0	//z53, z51
	//!!!!!! Negate the results
		pxor	mm7, mm6			//invert result

		paddd	mm3, Const_1			// 2's complement
		pxor	mm5, mm6			//invert result

	//	    z3 += z5//

		paddd	mm1, Const_1			// 2's complement
		paddd	mm3, mm2	//z3_even

		paddd	mm5, Const_1			// 2's complement
		paddd	mm1, mm0	//z3_odd

	//	    z4 += z5;

		paddd	mm7, Const_1			// 2's complement
		paddd	mm5, mm2

		paddd	mm7, mm0
		movq	mm2, mm4
		
	//	    tmp4 = MULTIPLY(tmp4, FIX_0_298631336);

		pmaddwd	mm4, Const_0xFIX_0_298631336	//T42, T40

		pmaddwd	mm2, Const_FIX_0_298631336x0	//T43, T41

		movq	qwZ4_even, mm5

		movq	qwZ4_odd, mm7
		paddd	mm4, mm3

		movq	mm6, qwScratch6
		paddd	mm2, mm1

		movq	qwTmp4_Z3_Even, mm4
		movq	mm5, mm6
			
	//	    tmp6 = MULTIPLY(tmp6, FIX_3_072711026);

		pmaddwd	mm6, Const_0xFIX_3_072711026	//T62, T60

		pmaddwd	mm5, Const_FIX_3_072711026x0	//T63, T61

		movq	qwTmp4_Z3_Odd, mm2	
			
		movq	mm4, qwZ4_even	
		paddd	mm6, mm3

		movq	mm3, qwScratch5
		paddd	mm5, mm1
		
		movq	qwTmp6_Z3_Even, mm6	
		movq	mm2, mm3

	//	    tmp5 = MULTIPLY(tmp5, FIX_2_053119869);		

		pmaddwd	mm3, Const_0xFIX_2_053119869	//T52, T50

		pmaddwd	mm2, Const_FIX_2_053119869x0	//T53, T51

		movq	qwTmp6_Z3_Odd, mm5
		
		movq	mm0, qwZ4_odd
		paddd	mm3, mm4

		movq	mm7, qwScratch7	
		paddd	mm2, mm0
		
		movq	qwTmp5_Z4_Even, mm3	
		movq	mm6, mm7

	//	    tmp7 = MULTIPLY(tmp7, FIX_1_501321110);

		pmaddwd	mm7, Const_0xFIX_1_501321110	//T72, T70

		pmaddwd	mm6, Const_FIX_1_501321110x0	//T73, T71

		movq	mm3, qwZ1

		movq	qwTmp5_Z4_Odd, mm2
		paddd	mm7, mm4

		movq	mm5, Const_FFFF
		movq	mm4, mm3

	//	    z1 = MULTIPLY(z1, - FIX_0_899976223);

		pmaddwd	mm3, Const_0xFIX_0_899976223	//z12, z10
 		paddd	mm6, mm0

		pmaddwd	mm4, Const_FIX_0_899976223x0	//z13, z11

		movq	mm2, qwTmp4_Z3_Odd
	//!!!!!! Negate the results
		pxor	mm3, mm5			//invert result

		paddd	mm3, Const_1			// 2's complement
		pxor	mm4, mm5			//invert result

		paddd	mm4, Const_1			// 2's complement
		paddd	mm7, mm3	//tmp7 + z1 + z4 EVEN

	//	    dataptr[DATASIZE*1] = (DCTELEM) DESCALE(tmp7 + z1 + z4,
	//					   CONST_BITS+PASS1_BITS);

		paddd	mm7, Const_16384	//rounding adj
		paddd	mm6, mm4	//tmp7 + z1 + z4 ODD

		paddd	mm6, Const_16384	//rounding adj
		psrad	mm7, 15				// descale it |  |R2|  |R0|
		
		movq	mm0, qwTmp4_Z3_Even
		psrad	mm6, 15				// descale it |  |R3|  |R1|

		paddd	mm0, mm3	//tmp4 + z1 + z3 EVEN
		movq	mm5, mm7

		movq	mm3, qwTemp0			//restore 
		punpckldq mm7, mm6			//|  |R1|  |R0|

		paddd	mm0, Const_16384	//rounding adj
		paddd	mm2, mm4	//tmp4 + z1 + z3 ODD

		movq	[edi][DATASIZE*0], mm3	//store 
		punpckhdq mm5, mm6			//|  |R3|  |R2|

	//	    dataptr[DATASIZE*7] = (DCTELEM) DESCALE(tmp4 + z1 + z3,
	//					   CONST_BITS+PASS1_BITS);

		paddd	mm2, Const_16384	//rounding adj
		psrad	mm0, 15				// descale it |  |R2|  |R0|
		
		movq	mm6, qwZ2
		psrad	mm2, 15				// descale it |  |R3|  |R1|

		movq	[edi][DATASIZE*1+8], mm5	//store
		movq	mm4, mm6

	//	    z2 = MULTIPLY(z2, - FIX_2_562915447);

		pmaddwd	mm6, Const_0xFIX_2_562915447	//z22, z20
		movq	mm5, mm0

		pmaddwd	mm4, Const_FIX_2_562915447x0	//z23, z21
		punpckldq mm0, mm2			//|  |R1|  |R0|

		movq	mm3, Const_FFFF
		punpckhdq mm5, mm2			//|  |R3|  |R2|

		movq	[edi][DATASIZE*1], mm7	//store
	//!!!!!! Negate the results
		pxor	mm6, mm3			//invert result

		movq	mm1, Const_1
		pxor	mm4, mm3			//invert result

		movq	mm7, qwTmp6_Z3_Even
		paddd	mm6, mm1			// 2's complement

		movq	mm2, qwTmp6_Z3_Odd
		paddd	mm4, mm1			// 2's complement

	//	    dataptr[DATASIZE*3] = (DCTELEM) DESCALE(tmp6 + z2 + z3,
	//					   CONST_BITS+PASS1_BITS);

		movq	[edi][DATASIZE*7], mm0	//store
		paddd	mm7, mm6	//tmp6 + z2 + z3 EVEN

		movq	mm1, Const_16384
		paddd	mm2, mm4	//tmp6 + z2 + z3 ODD

		movq	mm3, qwTemp2			//restore 
		paddd	mm7, mm1	//rounding adj

		movq	[edi][DATASIZE*7+8], mm5	//store
		paddd	mm2, mm1	//rounding adj

		movq	[edi][DATASIZE*2], mm3	//store 
		psrad	mm7, 15				// descale it |  |R2|  |R0|
		
 		movq	mm0, qwTemp4			//restore 
		psrad	mm2, 15				// descale it |  |R3|  |R1|

		movq	mm3, qwTmp5_Z4_Even
		movq	mm5, mm7

		movq	[edi][DATASIZE*4], mm0	//store 
		paddd	mm3, mm6	//tmp5 + z2 + z4 EVEN

		movq	mm6, qwTmp5_Z4_Odd
		punpckldq mm7, mm2			//|  |R1|  |R0|

		punpckhdq mm5, mm2			//|  |R3|  |R2|
		paddd	mm6, mm4	//tmp5 + z2 + z4 ODD

		movq	[edi][DATASIZE*3], mm7	//store
		paddd	mm3, mm1	//rounding adj

	//	    dataptr[DATASIZE*5] = (DCTELEM) DESCALE(tmp5 + z2 + z4,
	//					   CONST_BITS+PASS1_BITS);

		movq	mm0, qwTemp6			//restore 
		paddd	mm6, mm1	//rounding adj

		movq	[edi][DATASIZE*3+8], mm5	//store
		psrad	mm3, 15				// descale it |  |R2|  |R0|
		
		movq	[edi][DATASIZE*6], mm0	//store 
		psrad	mm6, 15				// descale it |  |R3|  |R1|

		movq	mm7, [edi][DATASIZE*0+16]
		movq	mm5, mm3

		paddw	mm7, [edi][DATASIZE*7+16]
		punpckldq mm3, mm6			//|  |R1|  |R0|

		movq	mm1, [edi][DATASIZE*1+16]
		punpckhdq mm5, mm6			//|  |R3|  |R2|

		paddw	mm1, [edi][DATASIZE*6+16]
		movq	mm4, mm7

	//******************************************************************************
	// This completes 4x8 dct locations.  Copy to do other 4x8.
	//******************************************************************************

		movq	mm6, [edi][DATASIZE*3+16]

		paddw	mm6, [edi][DATASIZE*4+16]

		movq	mm2, [edi][DATASIZE*2+16]
		psubw	mm4, mm6	//tmp13

		paddw	mm2, [edi][DATASIZE*5+16]
		paddw	mm7, mm6	//tmp10

		movq	[edi][DATASIZE*5], mm3	//store
		movq	mm6, mm1

 		movq	[edi][DATASIZE*5+8], mm5	//store
		paddw	mm1, mm2	//tmp11

		psubw	mm6, mm2	//tmp12
		movq	mm3, mm7	

	//    dataptr[DATASIZE*0] = (DCTELEM) DESCALE(tmp10 + tmp11, PASS1_BITS);
	//    dataptr[DATASIZE*4] = (DCTELEM) DESCALE(tmp10 - tmp11, PASS1_BITS);

		paddw	mm7, mm1	//tmp10 + tmp11	

		paddw	mm7, Const_2	// round  add 2 to each element
		psubw	mm3, mm1	//tmp10 - tmp11

		paddw	mm3, Const_2	// round  add 2 to each element
		psraw	mm7, 2			// descale it

	//		unpack word to dword sign extended
		movq	mm5, mm7
		punpcklwd mm7, mm7

		psrad	mm7, 16			// even results store in Temp0			
		punpckhwd mm5, mm5

		psrad	mm5, 16			// odd results store in array
		movq	mm1, mm6	//copy tmp12

		movq	qwTemp0, mm7	//store 
		psraw	mm3, 2			// descale it

		movq	[edi][DATASIZE*0+24], mm5
		movq	mm5, mm3

		punpcklwd mm3, mm3
		paddw	mm1, mm4	//tmp12 + tmp13

		psrad	mm3, 16			// even results store in Temp4
		movq	mm2, mm1	//copy
					
	//		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);

		pmaddwd	mm1, Const_0xFIX_0_541196100	//| z12 | z10 |
		punpckhwd mm5, mm5

		pmaddwd	mm2, Const_FIX_0_541196100x0	//| z13 | z11 |
		movq	mm7, mm4

	//		dataptr[DATASIZE*2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
	//					   CONST_BITS+PASS1_BITS);

		pmaddwd	mm4, Const_FIX_0_765366865x0	//| r3 | r1 |
		psrad	mm5, 16			// odd results store in array

		pmaddwd	mm7, Const_0xFIX_0_765366865	//| r2 | r0 |
		movq	mm0, mm6

	//		dataptr[DATASIZE*6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
	//					   CONST_BITS+PASS1_BITS);

		pmaddwd	mm6, Const_0xFIX_1_847759065	//| r2 | r0 |

		movq	qwTemp4, mm3	//store
		paddd	mm4, mm2

		paddd	mm4, Const_16384
		paddd	mm7, mm1						// add z1

		paddd	mm7, Const_16384
		psrad	mm4, 15				// descale it |  |R3|  |R1|

		movq	[edi][DATASIZE*4+24], mm5
		psrad	mm7, 15				// descale it |  |R2|  |R0|
		
		pmaddwd	mm0, Const_FIX_1_847759065x0	//| r3 | r1 |
		movq	mm5, mm7

	//!!!!!! Negate result
		movq	mm3, Const_1
		punpckldq mm7, mm4			//|  |R1|  |R0|

		pxor	mm6, Const_FFFF			//invert result
		punpckhdq mm5, mm4			//|  |R3|  |R2|

		movq	qwTemp2, mm7	//store
		paddd	mm6, mm3			// 2's complement

		pxor	mm0, Const_FFFF			//invert result
		paddd	mm6, mm1						// add z1

		movq	[edi][DATASIZE*2+24], mm5	//write out 2nd half in unused memory
		paddd	mm0, mm3			// 2's complement

		movq	mm3, Const_16384
		paddd	mm0, mm2

		movq	mm7, [edi][DATASIZE*0+16]
		paddd	mm6, mm3

		movq	mm4, [edi][DATASIZE*3+16]
		paddd	mm0, mm3

		psubw	mm7, [edi][DATASIZE*7+16]
		psrad	mm6, 15				// descale it |  |R2|  |R0|
		
		psubw	mm4, [edi][DATASIZE*4+16]
		psrad	mm0, 15				// descale it |  |R3|  |R1|

		movq	mm3, [edi][DATASIZE*2+16]
		movq	mm5, mm6

		psubw	mm3, [edi][DATASIZE*5+16]
		punpckldq mm6, mm0			//|  |R1|  |R0|

		punpckhdq mm5, mm0			//|  |R3|  |R2|
		movq	mm0, mm4

		movq	qwTemp6, mm6	//store
		movq	mm2, mm4

	//		tmp4 = dataptr[3] - dataptr[4];
	//		z1 = tmp4 + tmp7;

		movq	mm6, [edi][DATASIZE*1+16]
		paddw	mm0, mm7	//z1

		movq	[edi][DATASIZE*6+24], mm5	//write out 2nd half in unused memory
		movq	mm1, mm3
		
		psubw	mm6, [edi][DATASIZE*6+16]
		movq	mm5, mm3

		movq	qwZ1, mm0	//store
		paddw	mm5, mm7	//z4

		movq	qwScratch7, mm7
		paddw	mm1, mm6	//z2

		movq	qwScratch5, mm3
		paddw	mm2, mm6	//z3

		movq	qwZ2, mm1
		movq	mm3, mm2

	//	    z3 = MULTIPLY(z3, - FIX_1_961570560);
	//	    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);
	//	    z4 = MULTIPLY(z4, - FIX_0_390180644);

		movq	qwScratch6, mm6
		movq	mm1, mm2

		pmaddwd	mm3, Const_0xFIX_1_961570560	//z32, z30
		movq	mm7, mm5

		movq	mm6, Const_FFFF
		paddw	mm2, mm5		//z3 + z4

		pmaddwd	mm1, Const_FIX_1_961570560x0	//z33, z31
		movq	mm0, mm2
		
		pmaddwd	mm7, Const_FIX_0_390180644x0	//z43, z41
	//!!!!!! Negate the results
		pxor	mm3, mm6			//invert result

		pmaddwd	mm5, Const_0xFIX_0_390180644	//z42, z40

		pmaddwd	mm2, Const_0xFIX_1_175875602	//z52, z50
 		pxor	mm1, mm6			//invert result

		pmaddwd	mm0, Const_FIX_1_175875602x0	//z53, z51
	//!!!!!! Negate the results
		pxor	mm7, mm6			//invert result

		paddd	mm3, Const_1			// 2's complement
		pxor	mm5, mm6			//invert result

	//	    z3 += z5;

		paddd	mm1, Const_1			// 2's complement
		paddd	mm3, mm2	//z3_even

		paddd	mm5, Const_1			// 2's complement
		paddd	mm1, mm0	//z3_odd

	//	    z4 += z5;

		paddd	mm7, Const_1			// 2's complement
		paddd	mm5, mm2

		paddd	mm7, mm0
		movq	mm2, mm4
		
	//	    tmp4 = MULTIPLY(tmp4, FIX_0_298631336);

		pmaddwd	mm4, Const_0xFIX_0_298631336	//T42, T40

		pmaddwd	mm2, Const_FIX_0_298631336x0	//T43, T41

		movq	qwZ4_even, mm5

		movq	qwZ4_odd, mm7
		paddd	mm4, mm3

		movq	mm6, qwScratch6
		paddd	mm2, mm1

		movq	qwTmp4_Z3_Even, mm4
		movq	mm5, mm6
			
	//	    tmp6 = MULTIPLY(tmp6, FIX_3_072711026);

		pmaddwd	mm6, Const_0xFIX_3_072711026	//T62, T60

		pmaddwd	mm5, Const_FIX_3_072711026x0	//T63, T61

		movq	qwTmp4_Z3_Odd, mm2	
			
		movq	mm4, qwZ4_even	
		paddd	mm6, mm3

		movq	mm3, qwScratch5
		paddd	mm5, mm1
		
		movq	qwTmp6_Z3_Even, mm6	
		movq	mm2, mm3

	//	    tmp5 = MULTIPLY(tmp5, FIX_2_053119869);		

		pmaddwd	mm3, Const_0xFIX_2_053119869	//T52, T50

		pmaddwd	mm2, Const_FIX_2_053119869x0	//T53, T51

		movq	qwTmp6_Z3_Odd, mm5
		
		movq	mm0, qwZ4_odd
		paddd	mm3, mm4

		movq	mm7, qwScratch7	
		paddd	mm2, mm0
		
		movq	qwTmp5_Z4_Even, mm3	
		movq	mm6, mm7

	//	    tmp7 = MULTIPLY(tmp7, FIX_1_501321110);

		pmaddwd	mm7, Const_0xFIX_1_501321110	//T72, T70

		pmaddwd	mm6, Const_FIX_1_501321110x0	//T73, T71

		movq	mm3, qwZ1

		movq	qwTmp5_Z4_Odd, mm2
		paddd	mm7, mm4

		movq	mm5, Const_FFFF
		movq	mm4, mm3

	//	    z1 = MULTIPLY(z1, - FIX_0_899976223);

		pmaddwd	mm3, Const_0xFIX_0_899976223	//z12, z10
 		paddd	mm6, mm0

		pmaddwd	mm4, Const_FIX_0_899976223x0	//z13, z11

		movq	mm2, qwTmp4_Z3_Odd
	//!!!!!! Negate the results
		pxor	mm3, mm5			//invert result

		paddd	mm3, Const_1			// 2's complement
		pxor	mm4, mm5			//invert result

		paddd	mm4, Const_1			// 2's complement
		paddd	mm7, mm3	//tmp7 + z1 + z4 EVEN

	//	    dataptr[DATASIZE*1] = (DCTELEM) DESCALE(tmp7 + z1 + z4,
	//					   CONST_BITS+PASS1_BITS);

		paddd	mm7, Const_16384	//rounding adj
		paddd	mm6, mm4	//tmp7 + z1 + z4 ODD

		paddd	mm6, Const_16384	//rounding adj
		psrad	mm7, 15				// descale it |  |R2|  |R0|
		
		movq	mm0, qwTmp4_Z3_Even
		psrad	mm6, 15				// descale it |  |R3|  |R1|

		paddd	mm0, mm3	//tmp4 + z1 + z3 EVEN
		movq	mm5, mm7

		movq	mm3, qwTemp0			//restore 
		punpckldq mm7, mm6			//|  |R1|  |R0|

		paddd	mm0, Const_16384	//rounding adj
		paddd	mm2, mm4	//tmp4 + z1 + z3 ODD

		movq	[edi][DATASIZE*0+16], mm3	//store 
		punpckhdq mm5, mm6			//|  |R3|  |R2|

	//	    dataptr[DATASIZE*7] = (DCTELEM) DESCALE(tmp4 + z1 + z3,
	//					   CONST_BITS+PASS1_BITS);

		paddd	mm2, Const_16384	//rounding adj
		psrad	mm0, 15				// descale it |  |R2|  |R0|
		
		movq	mm6, qwZ2
		psrad	mm2, 15				// descale it |  |R3|  |R1|

		movq	[edi][DATASIZE*1+24], mm5	//store
		movq	mm4, mm6

	//	    z2 = MULTIPLY(z2, - FIX_2_562915447);

		pmaddwd	mm6, Const_0xFIX_2_562915447	//z22, z20
		movq	mm5, mm0

		pmaddwd	mm4, Const_FIX_2_562915447x0	//z23, z21
		punpckldq mm0, mm2			//|  |R1|  |R0|

		movq	mm3, Const_FFFF
		punpckhdq mm5, mm2			//|  |R3|  |R2|

		movq	[edi][DATASIZE*1+16], mm7	//store
	//!!!!!! Negate the results
		pxor	mm6, mm3			//invert result

		movq	mm1, Const_1
		pxor	mm4, mm3			//invert result

		movq	mm7, qwTmp6_Z3_Even
		paddd	mm6, mm1			// 2's complement

		movq	mm2, qwTmp6_Z3_Odd
		paddd	mm4, mm1			// 2's complement

	//	    dataptr[DATASIZE*3] = (DCTELEM) DESCALE(tmp6 + z2 + z3,
	//					   CONST_BITS+PASS1_BITS);

		movq	[edi][DATASIZE*7+16], mm0	//store
		paddd	mm7, mm6	//tmp6 + z2 + z3 EVEN

		movq	mm1, Const_16384
		paddd	mm2, mm4	//tmp6 + z2 + z3 ODD

		movq	mm3, qwTemp2			//restore 
		paddd	mm7, mm1	//rounding adj

		movq	[edi][DATASIZE*7+24], mm5	//store
		paddd	mm2, mm1	//rounding adj

		movq	[edi][DATASIZE*2+16], mm3	//store 
		psrad	mm7, 15				// descale it |  |R2|  |R0|
		
		movq	mm3, qwTmp5_Z4_Even
		psrad	mm2, 15				// descale it |  |R3|  |R1|

		movq	mm5, mm7
		paddd	mm3, mm6	//tmp5 + z2 + z4 EVEN

		movq	mm6, qwTmp5_Z4_Odd
		punpckldq mm7, mm2			//|  |R1|  |R0|

		punpckhdq mm5, mm2			//|  |R3|  |R2|
		paddd	mm6, mm4	//tmp5 + z2 + z4 ODD

		movq	[edi][DATASIZE*3+16], mm7	//store
		paddd	mm3, mm1	//rounding adj

	//	    dataptr[DATASIZE*5] = (DCTELEM) DESCALE(tmp5 + z2 + z4,
	//					   CONST_BITS+PASS1_BITS);

 		movq	mm7, qwTemp4			//restore 
		paddd	mm6, mm1	//rounding adj

		movq	[edi][DATASIZE*3+24], mm5	//store
		psrad	mm3, 15				// descale it |  |R2|  |R0|
		
		movq	[edi][DATASIZE*4+16], mm7	//store 
		psrad	mm6, 15				// descale it |  |R3|  |R1|

		movq	mm7, qwTemp6			//restore 
		movq	mm5, mm3

		punpckldq mm3, mm6			//|  |R1|  |R0|

		movq	[edi][DATASIZE*6+16], mm7	//store 
		punpckhdq mm5, mm6			//|  |R3|  |R2|

		movq	[edi][DATASIZE*5+16], mm3	//store

 		movq	[edi][DATASIZE*5+24], mm5	//store

	//******************************************************************************
	// This completes all 8x8 dct locations for the column case.
	//******************************************************************************

		emms
	}
}

#endif /* DCT_ISLOW_SUPPORTED */
