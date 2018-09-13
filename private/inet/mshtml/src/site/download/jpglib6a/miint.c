/***************************************************************************
*
*                INTEL Corporation Proprietary Information  
*
*      
*                  Copyright (c) 1996 Intel Corporation.
*                         All rights reserved.
*
***************************************************************************
			AUTHOR:  Kumar Balasubramanian 
***************************************************************************

** MMX version of the "integer LLM mode" within IJG decompressor code.
** The following is an MMX implementation of the integer slow mode
** IDCT within the IJG code.
*/

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */

#ifdef DCT_ISLOW_SUPPORTED

#ifndef USEINLINEASM

GLOBAL(void)
midct8x8llm (JCOEFPTR inptr, short *quantptr, short *wsptr,
		 JSAMPARRAY output_buf, JDIMENSION output_col, JSAMPLE *range_limit )
{
}

#else


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif



#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  13
#define PASS1_BITS  2
#else
#define CONST_BITS  13
#define PASS1_BITS  1		/* lose a little precision to avoid overflow */
#endif

/* Define the constants for the case BITS_IN_JSAMPLE = 8 */

static const __int64 const_0_2986	=	0x0000098E0000098E ;
static const __int64 const_0_3901	=	0x00000c7c00000c7c;
static const __int64 const_0_54119	=	0x0000115100001151;
static const __int64 const_0_7653	=	0x0000187E0000187E;
static const __int64 const_0_899	=	0x00001ccd00001ccd;
static const __int64 const_1_175	=	0x000025a1000025a1;
static const __int64 const_1_501	=	0x0000300b0000300b;
static const __int64 const_1_8477	=	0x00003b2100003b21;
static const __int64 const_1_961	=	0x00003ec500003ec5 ;
static const __int64 const_2_053	=	0x000041b3000041b3 ;
static const __int64 const_2_562	=	0x0000520300005203 ;
static const __int64 const_3_072	=	0x0000625400006254 ;

static const __int64 const_all_ones	=	0x0ffffffffffffffff;	
static const __int64 const_0_1_0_1		=	0x0000000100000001	 ;
static const __int64 const_zero		=	0x0000000000000000;	
static const __int64 const_1_0			=	0x0000000100000001	;
static const __int64 const_round		=	0x0000040000000400;
static const __int64 const_round_two	=	0x0002000000020000;
static const __int64 const_mask		=  0x000003ff000003ff;

static const __int64 const_00_1_84_00_0_765	=	0x00003b210000187E;
static const __int64 const_00_0_5411_00_00		=	0x0000115100000000;
static const __int64 const_3_072_00_1_501_00	=	0x62540000300b0000;
static const __int64 const_0_2986_00_2_053_00	=	0x098E000041b30000;
static const __int64 const_0_899_00_2_562_00	=   0x1ccd000052030000;
static const __int64 const_1_96_00_0_3901_00	=   0x3ec500000c7c0000;
static const __int64 const_1_175_00_00_00		=	0x25a1000000000000;







/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */

GLOBAL(void)
midct8x8llm (JCOEFPTR inptr, short *quantptr, short *wsptr,
		 JSAMPARRAY output_buf, JDIMENSION output_col, JSAMPLE *range_limit )
{

	INT32 locdwinptr,	locdwqptr, locdwwsptr, locdwcounter, locdwrowctr ;
__int64 locqwtmp0e,locqwtmp0o, locqwtmp1e, locqwtmp1o, locqwtmp2e ;

__int64 locqwtmp10e	, locqwtmp10o	,locqwtmp11e	,
		 locqwtmp11o	, locqwtmp12e	, locqwtmp12o	,
		 locqwtmp13e	, locqwtmp13o	,locqwtmp0	,
		locqwtmp1	,locqwtmp2	,locqwtmp3	,
		locqwz5e ,locqwz5o	,locqwz1e ,locqwz1o	,
		locqwz13e	,locqwz13o	,locqwz14e	,
		locqwz14o	,locqwz23e	,locqwz23o	,
		locqwz24e	,locqwz24o ;




// Inline assembly to do the IDCT and store the result */

__asm {

mov	esi, inptr	; load the input pointer
mov edi, quantptr		; load the quant table pointer

mov locdwinptr, esi	; to be used in the idct_column loop
mov locdwqptr, edi	; to be used in the idct_column loop

mov esi, wsptr
mov locdwcounter, 2	; idct_column loop counter

mov locdwwsptr, esi



;; do the idct on all the columns. Do four columns per
;; iteration of the loop.

idct_column:

mov		esi, locdwinptr	; get the source pointer
mov		edi, locdwqptr		; get the quantzn. pointer

;; fetch C2 and Q2
movq	mm0,  [esi+16*2]	; get C2

movq	mm1,  [edi+16*2]	; get Q2

movq	mm2,  [esi+16*6]	; get C6
pmullw	mm0, mm1		; dequantized C2 = z2

movq	mm3, [edi+16*6]	; get Q6

movq	mm6,  const_0_7653	
pmullw	mm2, mm3		; dequant. C6 = z3

movq	mm7,  const_1_8477	
movq	mm4, mm0		; copy z2

pmaddwd	mm4, mm6		; tmp3 - z1 for columns 0 & 2
movq	mm5, mm0		; copy z2

movq	mm3, mm2		; z3 copy
psrlq	mm5, 16			; move z2 columns 1 & 3 to 0 & 2

movq	mm1,  const_0_54119
pmaddwd	mm5, mm6		; tmp3 - z1 for columns 1 & 3

psrlq	mm3, 16			; move z3 columns 1 & 3 to 0 & 2
paddw	mm0, mm2		; z2 + z3

pmaddwd	mm2, mm7		; tmp2 - z1 for columns 0 & 2
movq	mm6, mm0		; z2 + z3 copy

psrlq	mm6, 16			; z2 + z3 columns 1 & 3 in 0 & 2
pmaddwd	mm3, mm7		; tmp2 - z1 for columns 1 & 3

movq	mm7,  const_all_ones
pmaddwd	mm0, mm1		; z1 columns 0 & 2

pmaddwd	mm6, mm1		; z1 columns 1 & 3
pxor	mm2, mm7		; 1s complement of tmp2 - z1

movq	mm1,  const_0_1_0_1
pxor	mm3, mm7		; 1s complement of tmp2 - z1 

paddd	mm2, mm1		; 2s complement of tmp2 - z1(col 0 &2)
paddd	mm3, mm1		; 2s complement of tmp2 - z1(col 1 & 3)

paddd	mm2, mm0		; tmp2 (columns 0 & 2)
paddd	mm4, mm0		; tmp2 (cols. 1 & 3)

;; get C0 and Q0
movq	mm0,  [esi+16*0]	; get C0
paddd	mm3, mm6		; tmp3

movq	mm1,  [edi+16*0]	; getQ0
paddd	mm5, mm6		; tmp3

movq	mm6,  [esi+16*4]	; get C4
pmullw	mm0, mm1		; dequant C0 = z2

movq	mm7,  [edi+16*4]	; get Q4
nop

movq	locqwtmp2e, mm2	; store tmp2 even part
pmullw	mm6, mm7		; dequant C4 = z3

movq	mm7,  const_1_0
movq	mm1, mm0		; copy of z2

paddw	mm0, mm6		; z2+z3
nop

psubw	mm1, mm6		; z2-z3
movq	mm6, mm0		; z2+z3 copy

pmaddwd	mm0, mm7		; get 0 & 2 cols
psrlq	mm6, 16			; get the other two cols.

pmaddwd	mm6, mm7		; 
movq	mm2, mm1		; copy of z2-z3

pmaddwd	mm1, mm7
psrlq	mm2, 16

pmaddwd	mm2, mm7
pslld	mm0, 13			; tmp0 cols 0&2

movq	mm7, mm4
pslld	mm6, 13			; tmp0 cols 1 & 3

paddd	mm4, mm0		; 
psubd	mm0, mm7		; 

movq	mm7, mm5
pslld	mm2, 13

movq	locqwtmp13e, mm0	; store tmp13 cols 0&2
paddd	mm5, mm6

movq	mm0, locqwtmp2e
psubd	mm6, mm7


movq	locqwtmp10o, mm5	; store tmp10 cols 1&3
movq	mm7, mm3

movq	locqwtmp13o, mm6	; store tmp13 cols 1&3
paddd	mm3, mm2

movq	locqwtmp10e, mm4	; store tmp10 cols 0&2
pslld	mm1, 13

movq	locqwtmp11o, mm3	; store tmp11 cols 1,3
psubd	mm2, mm7

movq	mm6,  [esi+16*1]
movq	mm3, mm0

movq	locqwtmp12o, mm2	; store tmp12 cols. 1,3
paddd	mm0, mm1

movq	mm7,  [edi+16*1]

movq	locqwtmp11e, mm0	; store tmp11 cols. 0,2
psubd	mm1, mm3

movq	mm0,  [esi+16*7]
pmullw	mm6, mm7	; dequant. C1 = tmp3

movq	locqwtmp12e, mm1

;; completed the even part.
;; Now start the odd part

movq	mm1,  [edi+16*7]	; get C7

movq	mm2,  [esi+16*5]	; get C5
pmullw	mm0, mm1	; dequant. C7 = tmp0

movq	mm3,  [edi+16*5]

movq	mm4,  [esi+16*3]
pmullw	mm2, mm3	; dequant. C5 = tmp1

movq	mm5,  [edi+16*3]
movq	mm1, mm0

movq	locqwtmp3, mm6
pmullw	mm4, mm5	; dequant. C3 = tmp2

movq	locqwtmp0, mm0
paddw	mm0, mm6	; z1 

movq	locqwtmp1, mm2
movq	mm3, mm2

movq	locqwtmp2, mm4
paddw	mm2, mm4	; z2

paddw	mm1, mm4	; z3

movq	mm4,  const_1_175
paddw	mm3, mm6	; z4	

movq	mm5, mm1
movq	mm7, mm0

psrlq	mm7, 16		; other two cols. of z1
paddw	mm5, mm3	; z3 + z4

movq	mm6, mm5
pmaddwd	mm5, mm4	; z5 cols 0 & 2

pmaddwd	mm0,  const_0_899	; z1 even part
psrlq	mm6, 16

pmaddwd	mm6, mm4	; z5 cols 1 & 3
movq	mm4, mm2	; z2 copy

movq	locqwz5e, mm5
psrlq	mm4, 16		; get z2 cols 1 & 3

pxor	mm0,  const_all_ones
movq	mm5, mm1

movq	locqwz5o, mm6
psrlq	mm5, 16

movq	mm6,  const_2_562
nop

paddd	mm0,  const_0_1_0_1
pmaddwd	mm2, mm6	; z2 cols 0 & 2

movq	locqwz1e, mm0
pmaddwd	mm4, mm6	; z2 cols 1 & 3

pmaddwd	mm7,  const_0_899	; z1
movq	mm0, mm3

movq	mm6,  const_1_961
psrlq	mm0, 16

pxor	mm2,  const_all_ones
pmaddwd	mm1, mm6	; z3 cols 0 & 2

paddd	mm2,  const_0_1_0_1
pmaddwd	mm5, mm6	; z3 cols 1 & 3

movq	mm6,  const_0_3901
nop

pxor	mm4,  const_all_ones
pmaddwd	mm3, mm6	; z4 cols 0 & 2

paddd	mm4,  const_0_1_0_1
pmaddwd	mm0, mm6	; z4 cols 1 & 3

movq	mm6,  const_all_ones
nop

pxor	mm1, mm6
pxor	mm7, mm6

;; twos complement of z1, z2, z3, z4

paddd	mm1,  const_0_1_0_1	
pxor	mm5, mm6

paddd	mm7,  const_0_1_0_1
pxor	mm3, mm6

paddd	mm5,  const_0_1_0_1
nop

movq	locqwz1o, mm7
pxor	mm0, mm6

paddd	mm1, locqwz5e	; z3+z5 cols 0 & 2
nop

movq	mm6, locqwz1e
nop

paddd	mm5, locqwz5o	; z3+z5 cols 1 & 3
paddd	mm6, mm1

paddd	mm3,  const_0_1_0_1
paddd	mm1, mm2

paddd	mm0,  const_0_1_0_1
paddd	mm7, mm5

paddd	mm3, locqwz5e	; z4+z5 cols 0 & 2
paddd	mm5, mm4

paddd	mm0, locqwz5o	; z4+z5 cols 0 & 2
paddd	mm2, mm3

paddd	mm3, locqwz1e
paddd	mm4, mm0

paddd	mm0, locqwz1o

movq	locqwz23e, mm1
nop

movq	locqwz14o, mm0
nop

movq	mm0, locqwtmp0
nop

movq	locqwz24e, mm2
movq	mm1, mm0

movq	mm2,  const_0_2986
psrlq	mm1, 16

movq	locqwz14e, mm3
pmaddwd	mm0, mm2	; tmp0 even

movq	mm3, locqwtmp1
pmaddwd	mm1, mm2	; tmp0 odd

movq	locqwz24o, mm4
movq	mm2, mm3

movq	mm4,  const_2_053
psrlq	mm2, 16

movq	locqwz23o, mm5
pmaddwd	mm3, mm4	; tmp1 even

movq	mm5, locqwtmp2
pmaddwd	mm2, mm4	; tmp1 odd

movq	locqwz13e, mm6
movq	mm4, mm5

movq	mm6,  const_3_072
psrlq	mm4, 16

movq	locqwz13o, mm7
pmaddwd	mm5, mm6	; tmp2 even
	
;;;;;;; now calculate tmp0..tmp3
;; then calculate the pre-descaled values
;; this includes the right shift with rounding

movq	mm7, locqwtmp3
pmaddwd	mm4, mm6	; tmp2 odd

paddd	mm0, locqwz13e
movq	mm6, mm7

paddd	mm1, locqwz13o
psrlq	mm6, 16

movq	locqwtmp0e, mm0		; tmp0 even
nop

movq	mm0,  const_1_501
nop

movq	locqwtmp0o, mm1
pmaddwd	mm7, mm0

paddd	mm3, locqwz24e
pmaddwd	mm6, mm0

movq	mm0, locqwtmp10e
nop

paddd	mm7, locqwz14e
nop

paddd	mm6, locqwz14o
psubd	mm0, mm7

movq	mm1, locqwtmp10o
nop

movq	locqwtmp1e, mm3
psubd	mm1, mm6

movq	mm3,  const_round
nop

paddd	mm2, locqwz24o
paddd	mm0, mm3

paddd	mm7, locqwtmp10e
psrad	mm0, 11

movq	locqwtmp1o, mm2
paddd	mm1, mm3

paddd	mm6, locqwtmp10o
psrad	mm1, 11

paddd	mm5, locqwz23e
movq	mm2, mm0

paddd	mm4, locqwz23o
punpcklwd	mm0, mm1

paddd	mm6, mm3
punpckhwd	mm2, mm1

paddd	mm7, mm3
punpckldq	mm0, mm2

;; now do all the stores of the 1D-iDCT of the four columns

mov		edi, locdwwsptr	; get pointer to scratch pad array

movq	 [edi+16*7], mm0	; store wsptr[7]
psrad	mm6, 11

movq	mm2, locqwtmp11e
psrad	mm7, 11

psubd	mm2, mm5
movq	mm0, mm7

movq	mm1, locqwtmp11o
punpcklwd	mm7, mm6

psubd	mm1, mm4
punpckhwd	mm0, mm6

paddd	mm5, locqwtmp11e
punpckldq	mm7, mm0

paddd	mm4, locqwtmp11o
paddd	mm2, mm3

paddd	mm1, mm3
paddd	mm5, mm3

paddd	mm4, mm3
psrad	mm2, 11

movq	 [edi+16*0], mm7	; store wsptr[0]
psrad	mm1, 11

movq	mm0, mm2
psrad	mm5, 11

movq	mm6, locqwtmp12e
punpcklwd	mm2, mm1

punpckhwd	mm0, mm1
movq	mm1, mm5

movq	mm7, locqwtmp12o
punpckldq	mm2, mm0

movq	 [edi+16*6], mm2	; store wsptr[6]
psrad	mm4, 11

movq	mm2, mm6
punpcklwd	mm5, mm4

paddd	mm6, locqwtmp1e
punpckhwd	mm1, mm4

psubd	mm2, locqwtmp1e
punpckldq	mm5, mm1

movq	 [edi+16*1], mm5	; store wsptr[1]
movq	mm0, mm7

paddd	mm7, locqwtmp1o
paddd	mm6, mm3

psubd	mm0, locqwtmp1o
paddd	mm7, mm3

paddd	mm2, mm3
psrad	mm7, 11

paddd	mm0, mm3
psrad	mm6, 11

movq	mm1, mm6
psrad	mm2, 11

movq	mm4, locqwtmp13e
punpcklwd	mm6, mm7

movq	mm5, mm4
punpckhwd	mm1, mm7

paddd	mm4, locqwtmp0e
punpckldq	mm6, mm1

psubd	mm5, locqwtmp0e
psrad	mm0, 11

movq	 [edi+16*2], mm6	; store wsptr[2]
movq	mm6, mm2

paddd	mm4, mm3
punpcklwd	mm2, mm0

paddd	mm5, mm3
punpckhwd	mm6, mm0

movq	mm0, locqwtmp13o
punpckldq	mm2, mm6

movq	mm1, mm0
psrad	mm4, 11

paddd	mm0, locqwtmp0o
psrad	mm5, 11

paddd	mm0, mm3
movq	mm6, mm4

psubd	mm1, locqwtmp0o
psrad	mm0, 11

paddd	mm1, mm3
punpcklwd	mm4, mm0

movq	mm3, mm5
punpckhwd	mm6, mm0

movq	 [edi+16*5], mm2	; store wsptr[5]
punpckldq	mm4, mm6

psrad	mm1, 11

movq	 [edi+16*3], mm4	; store wsptr[3]
punpcklwd	mm5, mm1

punpckhwd	mm3, mm1

punpckldq	mm5, mm3

add locdwinptr, 8	; skip first four columns
add	locdwqptr,  8

movq	 [edi+16*4], mm5	; store wsptr[4]


;;;;;;; done with 1D-idct of four columns ;;;;;;;

;; now update pointers for next four columns

add locdwwsptr, 8
mov	eax, locdwcounter

dec eax

mov locdwcounter, eax
jnz idct_column

;;;;;;;end of 1D-idct on the columns ;;;;;;;

mov	esi, wsptr	; get start addr of temp array
mov locdwcounter, 8

mov	locdwwsptr, esi
mov	locdwrowctr, 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;; start of 1D-idct on the rows ;;;;;;;


idct_row:

mov	esi, locdwwsptr	; get next row start addr of temp array
mov	edi, output_buf	

movq	mm0,  [esi+0]	; get first 4 elements of row

movq	mm1,  [esi+2*4] ; get next 4 elem. of row
movq	mm2, mm0

movq	mm3, mm0	; copy of e3|e2|e1|e0
paddw	mm2, mm1	; (e3+e7)|(e2+e6)|(e1+e5)|(e0+e4)

movq	mm4, mm2	; copy of (e3+e7)|(e2+e6)|(e1+e5)|(e0+e4)
punpckhdq	mm3, mm1	; e7|e6|e3|e2

pmaddwd	mm3,  const_00_1_84_00_0_765	; (tmp2 - z1)||(tmp3-z1)
movq	mm6, mm0	; copy of e3|e2|e1|e0

pmaddwd	mm2,  const_00_0_5411_00_00	; z1||xxx
psubw	mm6, mm1	; (e3-e7)|(e2-e6)|(e1-e5)|(e0-e4)

punpckldq	mm4, mm6	; (e1-e5)|(e0-e4)|(e1+e5)|(e0+e4)
movq	mm6, mm0	; 

movq	mm5, mm3
pslld	mm4, 16	; (e0-e4)|(e1+e5)||(e0+e4)|x0000

pxor	mm3,  const_all_ones
punpckhdq	mm2, mm2	; z1||z1

paddd	mm3,  const_0_1_0_1
psrad	mm4, 3	; (e0-e4)<<13||(e0+e4)<<13

psrlq	mm3, 32
movq	mm7, mm4	; copy of tmp1||tmp0

punpckldq	mm5, mm3
movq	mm3, mm0	; e3|e2|e1|e0

paddd	mm5, mm2		; tmp2 || tmp3
paddw	mm3, mm1	; (e7+e3)|(e2+e6)|(e1+e5)|(e0+e4)

paddd	mm4, mm5
psubd	mm7, mm5


;; end of even part calculation ;;
;; mm0 => e3|e2|e1|e0
;; mm1 => e7|e6|e5|e4
;; mm4 => tmp11||tmp10
;; mm7 => tmp12||tmp13

movq	mm5, mm3
movq	mm2, mm0

pmaddwd	mm0,  const_3_072_00_1_501_00	; tmp2|tmp3
punpckldq	mm5, mm5

paddw	mm5, mm3
punpckldq	mm2, mm2

pmaddwd	mm5,  const_1_175_00_00_00		; z5|0
punpckhdq	mm6, mm2

pmaddwd		mm3,  const_1_96_00_0_3901_00	; z3|z4
paddw	mm6, mm1

pmaddwd		mm6,  const_0_899_00_2_562_00	; z1|z2
nop

pmaddwd		mm1,  const_0_2986_00_2_053_00	; tmp0|tmp1
punpckhdq	mm5, mm5

movq	mm2,  const_0_1_0_1
nop

pxor	mm3,  const_all_ones
nop

pxor	mm6,  const_all_ones
paddd	mm3, mm2

paddd	mm6, mm2
paddd	mm3, mm5

movq	mm5, mm6
paddd	mm6, mm3

movq	mm2, mm5
punpckldq	mm5, mm5

punpckhdq	mm2, mm5
paddd	mm1, mm6

paddd	mm2, mm3
movq	mm5, mm1

movq	mm3, mm4
paddd	mm0, mm2

movq	mm2, mm7
punpckldq	mm5, mm5

punpckhdq	mm1, mm5
psubd	mm3, mm0

movq	mm5,  const_round_two
paddd	mm0, mm4

movq	mm6,  const_mask
psubd	mm2, mm1

paddd	mm0, mm5
paddd	mm1, mm7



;; descale the resulting coeff values
paddd	mm1, mm5
psrad	mm0, 18

paddd	mm3, mm5
psrad	mm1, 18

paddd	mm2, mm5
psrad	mm3, 18


;; mask the result with RANGE_MASK (least 10 bits)
pand	mm1, mm6	; w2|w3
psrad	mm2, 18

movd	ebx, mm1	; w3
psrlq	mm1, 32		; 0|w2

;; using the results as index, get the corresponding
;; value from array range_limit and store the final result

mov		ecx, range_limit	; get start addr of range_limit array
add	edi, locdwrowctr

movd	edx, mm1	; w2
pand	mm0, mm6	; w1|w0

mov		ah, [ecx][ebx]	; w3
mov		edi, [edi]

movd	ebx, mm0	; w0
psrlq	mm0, 32		; 0|w1

mov		al, [ecx][edx]	; w2
add	locdwrowctr, 4

movd	edx, mm0	; w1
pand	mm3, mm6	; w6|w7

add	edi, output_col	; this is the dest start addr for this row
shl		eax, 16		; w3|w2|0|0

mov		al, [ecx][ebx]	; w0

mov		ah, [ecx][edx]	; w1

movd	mm4, eax	; w3|w2|w1|w0
pand	mm2, mm6	; w5|w4

movd	ebx, mm3	; w7
psrlq	mm3, 32		; 0|w6

movd	edx, mm3	; w6

mov		ah, [ecx][ebx]	; w7

mov		al, [ecx][edx]	; w6

movd	ebx, mm2	; w4
psrlq	mm2, 32		; 0|w5

shl		eax, 16		; w7|w6|0|0

movd	edx, mm2	; w5

mov		al, [ecx][ebx]	; w4

mov		ah, [ecx][edx]	; w5

movd	mm5, eax	; w7|w6|w5|w4

punpckldq	mm4, mm5	; w7|w6|w5|w4|w3|w2|w1|w0

add	locdwwsptr, 16
mov	eax, locdwcounter

movq	 [edi], mm4

;; update address pointer and loop counter

dec eax

mov	locdwcounter, eax
jnz	idct_row

;;;;;;; end of 1D-idct on all the rows ;;;;;;;
 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

emms


} //end of __asm

}


#endif /* X86 */

#endif /* DCT_ISLOW_SUPPORTED */
