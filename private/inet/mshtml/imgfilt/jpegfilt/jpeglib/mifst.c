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
 * jidctfst.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a fast, not so accurate integer implementation of the
 * inverse DCT (Discrete Cosine Transform).  In the IJG code, this routine
 * must also perform dequantization of the input coefficients.
 *
 * A 2-D IDCT can be done by 1-D IDCT on each column followed by 1-D IDCT
 * on each row (or vice versa, but it's more convenient to emit a row at
 * a time).  Direct algorithms are also available, but they are much more
 * complex and seem not to be any faster when reduced to code.
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

#ifndef _X86_

GLOBAL(void)
midct8x8aan (JCOEFPTR coef_block, short * wsptr, short * quantptr,
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


/* Scaling decisions are generally the same as in the LL&M algorithm;
 * see jidctint.c for more details.  However, we choose to descale
 * (right shift) multiplication products as soon as they are formed,
 * rather than carrying additional fractional bits into subsequent additions.
 * This compromises accuracy slightly, but it lets us save a few shifts.
 * More importantly, 16-bit arithmetic is then adequate (for 8-bit samples)
 * everywhere except in the multiplications proper; this saves a good deal
 * of work on 16-bit-int machines.
 *
 * The dequantized coefficients are not integers because the AA&N scaling
 * factors have been incorporated.  We represent them scaled up by PASS1_BITS,
 * so that the first and second IDCT rounds have the same input scaling.
 * For 8-bit JSAMPLEs, we choose IFAST_SCALE_BITS = PASS1_BITS so as to
 * avoid a descaling shift; this compromises accuracy rather drastically
 * for small quantization table entries, but it saves a lot of shifts.
 * For 12-bit JSAMPLEs, there's no hope of using 16x16 multiplies anyway,
 * so we use a much larger scaling factor to preserve accuracy.
 *
 * A final compromise is to represent the multiplicative constants to only
 * 8 fractional bits, rather than 13.  This saves some shifting work on some
 * machines, and may also reduce the cost of multiplication (since there
 * are fewer one-bits in the constants).
 */

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  8
#define PASS1_BITS  2
#else
#define CONST_BITS  8
#define PASS1_BITS  1		/* lose a little precision to avoid overflow */
#endif

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */ 

#if CONST_BITS == 8
#define FIX_1_082392200  ((INT32)  277)		/* FIX(1.082392200) */
#define FIX_1_414213562  ((INT32)  362)		/* FIX(1.414213562) */
#define FIX_1_847759065  ((INT32)  473)		/* FIX(1.847759065) */
#define FIX_2_613125930  ((INT32)  669)		/* FIX(2.613125930) */
#else
#define FIX_1_082392200  FIX(1.082392200)
#define FIX_1_414213562  FIX(1.414213562)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_2_613125930  FIX(2.613125930)
#endif


/* We can gain a little more speed, with a further compromise in accuracy,
 * by omitting the addition in a descaling shift.  This yields an incorrectly
 * rounded result half the time...
 */

#ifndef USE_ACCURATE_ROUNDING
#undef DESCALE
#define DESCALE(x,n)  RIGHT_SHIFT(x, n)
#endif

//#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)
/* Multiply a DCTELEM variable by an INT32 constant, and immediately
 * descale to yield a DCTELEM result.
 */

//#define MULTIPLY(var,const)  ((DCTELEM) DESCALE((var) * (const), CONST_BITS))
#define MULTIPLY(var,const)  ((DCTELEM) ((var) * (const)))


/* Dequantize a coefficient by multiplying it by the multiplier-table
 * entry; produce a DCTELEM result.  For 8-bit data a 16x16->16
 * multiplication will do.  For 12-bit data, the multiplier table is
 * declared INT32, so a 32-bit multiply will be used.
 */

#if BITS_IN_JSAMPLE == 8
//#define DEQUANTIZE(coef,quantval)  (((IFAST_MULT_TYPE) (coef)) * (quantval))
#define DEQUANTIZE(coef,quantval)  (((coef)) * (quantval))
#else
#define DEQUANTIZE(coef,quantval)  \
	DESCALE((coef)*(quantval), IFAST_SCALE_BITS-PASS1_BITS)
#endif

 
/* Like DESCALE, but applies to a DCTELEM and produces an int.
 * We assume that int right shift is unsigned if INT32 right shift is.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define ISHIFT_TEMPS	DCTELEM ishift_temp;
#if BITS_IN_JSAMPLE == 8
#define DCTELEMBITS  16		/* DCTELEM may be 16 or 32 bits */
#else
#define DCTELEMBITS  32		/* DCTELEM must be 32 bits */
#endif
#define IRIGHT_SHIFT(x,shft)  \
    ((ishift_temp = (x)) < 0 ? \
     (ishift_temp >> (shft)) | ((~((DCTELEM) 0)) << (DCTELEMBITS-(shft))) : \
     (ishift_temp >> (shft)))
#else
#define ISHIFT_TEMPS
#define IRIGHT_SHIFT(x,shft)	((x) >> (shft))
#endif

#ifdef USE_ACCURATE_ROUNDING
#define IDESCALE(x,n)  ((int) IRIGHT_SHIFT((x) + (1 << ((n)-1)), n))
#else
#define IDESCALE(x,n)  ((int) IRIGHT_SHIFT(x, n))
#endif

static const __int64  x5a825a825a825a82 = 0x0000016a0000016a ;
static const __int64  x539f539f539f539f = 0x0000fd630000fd63 ; 
static const __int64  x4546454645464546 = 0x0000011500000115 ; 
static const __int64  x61f861f861f861f8 = 0x000001d9000001d9 ; 
static const __int64  const_mask  = 0x03ff03ff03ff03ff ;
static const __int64  const_zero  = 0x0000000000000000 ;


 
/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */

GLOBAL(void)
midct8x8aan (JCOEFPTR coef_block, short * wsptr, short * quantptr,
		 JSAMPARRAY output_buf, JDIMENSION output_col, JSAMPLE *range_limit )
{
  __int64 scratch3, scratch5, scratch7 ;

  // do the 2-Dal idct and store the corresponding results
  // from the range_limit array



__asm {

mov ebx, coef_block   ; source coeff
mov	esi, wsptr	  ; temp results
mov edi, quantptr	  ; quant factors

movq mm0,  [ebx+8*12]	; V12
pmullw mm0,  [edi+8*12]
movq mm1,  [ebx+8*4]	; V4
pmullw mm1,  [edi+8*4]
movq mm3,  [ebx+8*0]	; V0
pmullw mm3,  [edi+8*0]

movq mm5,  [ebx+8*8]	; V8
movq mm2, mm1						; duplicate V4

pmullw mm5,  [edi+8*8]
psubw mm1, mm0						; V16 (s1)

movq 	mm7,  x5a825a825a825a82	; 23170 ->V18 (s3)
;***************************************************PackMulW
movq		mm6, mm1

punpcklwd	mm1,  const_zero
paddw mm2, mm0						; V17

pmaddwd		mm1, mm7
movq mm0, mm2						; duplicate V17

punpckhwd	mm6,  const_zero
movq mm4, mm3						; duplicate V0

pmaddwd		mm6, mm7
paddw mm3, mm5						; V19

psrad		mm1, 8
psubw mm4, mm5						; V20 ;mm5 free

psrad		mm6, 8				; mm6 = (s1)

packssdw	mm1, mm6
;**********************************************************
movq mm6, mm3						; duplicate t74=t81

psubw mm1, mm0						; V21 ; mm0 free
paddw mm3, mm2						; V22

movq mm5, mm1						; duplicate V21
paddw mm1, mm4						; V23

movq  [esi+8*4], mm3		; V22
psubw mm4, mm5						; V24; mm5 free

movq  [esi+8*12], mm1		; V23
psubw mm6, mm2						; V25; mm2 free

movq  [esi+8*0], mm4		; V24

; keep mm6 alive all along the next block
movq mm7,  [ebx+8*10]	; V10

pmullw mm7,  [edi+8*10]

movq mm0,  [ebx+8*6]	; V6

pmullw mm0,  [edi+8*6]
movq mm3, mm7						; duplicate V10

movq mm5,  [ebx+8*2]	; V2

pmullw mm5,  [edi+8*2]
psubw mm7, mm0						; V26 (s1/7)

movq mm4,  [ebx+8*14]	; V14

pmullw mm4,  [edi+8*14]
paddw mm3, mm0						; V29 ; free mm0

movq mm1,  x539f539f539f539f	;23170 ->V18 (scratch3)
 ;mm0 = s5, 
;***************************************************PackMulW
movq		 scratch7, mm7
movq		mm2, mm7

punpcklwd	mm7,  const_zero
movq		mm0, mm5				; duplicate V2

pmaddwd		mm7, mm1
paddw		mm5, mm4				; V27

punpckhwd	mm2,  const_zero
psubw 		mm0, mm4		;(s1) for next	; V28 ; free mm4

pmaddwd		mm2, mm1
movq		mm4, mm0

punpcklwd	mm0,  const_zero
psrad		mm7, 8

psrad		mm2, 8			; mm2 = scratch1
movq		mm1, mm4			; duplicate V28

punpckhwd	mm4,  const_zero
packssdw	mm7, mm2

movq		mm2,  x4546454645464546	; 23170 ->V18
;**********************************************************

;***************************************************PackMulW
pmaddwd		mm0, mm2

pmaddwd		mm4, mm2
psrad		mm0, 8

movq	mm2,  x61f861f861f861f8	; 23170 ->V18
psrad		mm4, 8

packssdw	mm0, mm4
movq		mm4, mm1

movq mm1,  scratch7
;**********************************************************

movq	 scratch5, mm0
paddw mm1, mm4						; V32 ; free mm4

;***************************************************PackMulW
movq		mm0, mm1

punpcklwd	mm1,  const_zero
movq		mm4, mm5						; duplicate t90=t93

pmaddwd		mm1, mm2
paddw		mm5, mm3						; V31

punpckhwd	mm0,  const_zero
psubw		mm4, mm3						; V30 ; free mm3

movq 	mm3,  x5a825a825a825a82	; 23170 ->V18
pmaddwd		mm0, mm2

psrad		mm1, 8
movq		mm2, mm4		; make a copy of mm4

punpcklwd	mm4,  const_zero
psrad		mm0, 8

pmaddwd		mm4, mm3
packssdw	mm1, mm0
;**********************************************************

;***************************************************PackMulW
punpckhwd	mm2,  const_zero

movq		mm0,  scratch5
pmaddwd		mm2, mm3

psubw		mm0, mm1						; V38
paddw		mm1, mm7						; V37 ; free mm7

movq		mm7,  [esi+8*4]		; V22
psrad		mm4, 8

psrad		mm2, 8
movq mm3, mm6                       ; duplicate V25

packssdw	mm4, mm2
psubw mm1, mm5						; V39 (mm5 still needed for next block)
;**********************************************************

movq mm2,  [esi+8*12]     ; V23
psubw mm4, mm1						; V40

paddw mm0, mm4						; V41; free mm0

psubw mm6, mm0						; tm6
paddw mm3, mm0						; tm8; free mm1

movq mm0, mm1		; line added by Kumar
movq mm1, mm7						; duplicate V22

movq  [esi+8*8], mm3     ; tm8; free mm3
paddw mm7, mm5						; tm0

movq  [esi+8*6], mm6		; tm6; free mm6
psubw mm1, mm5						; tm14; free mm5

movq mm6,  [esi+8*0]		; V24
movq mm3, mm2						; duplicate t117=t125

movq  [esi+8*0], mm7      ; tm0; free mm7
paddw mm2, mm0						; tm2

movq  [esi+8*14], mm1		; tm14; free mm1
psubw mm3, mm0						; tm12; free mm0

movq  [esi+8*2], mm2		; tm2; free mm2
movq mm0, mm6						; duplicate t119=t123

movq  [esi+8*12], mm3      ; tm12; free mm3
paddw mm6, mm4						; tm4

movq mm1,  [ebx+8*5]	; V5
psubw mm0, mm4						; tm10; free mm4

pmullw mm1,  [edi+8*5]
movq  [esi+8*4], mm6		; tm4; free mm6
movq  [esi+8*10], mm0     ; tm10; free mm0

; column 1: even part
; use V5, V13, V1, V9 to produce V56..V59

movq mm7,  [ebx+8*13]	; V13
movq mm2, mm1						; duplicate t128=t130

pmullw mm7,  [edi+8*13]
movq mm3,  [ebx+8*1]	; V1
pmullw mm3,  [edi+8*1]

movq mm5,  [ebx+8*9]	; V9
psubw mm1, mm7						; V50

pmullw mm5,  [edi+8*9]
paddw mm2, mm7						; V51

movq mm7,  x5a825a825a825a82	; 23170 ->V18
;***************************************************PackMulW
movq		mm4, mm1

punpcklwd	mm1,  const_zero
movq		mm6, mm2						; duplicate V51

pmaddwd		mm1, mm7

punpckhwd	mm4,  const_zero

movq mm0,  [ebx+8*11]	; V11
pmaddwd		mm4, mm7

pmullw mm0,  [edi+8*11]
psrad		mm1, 8

psrad		mm4, 8

packssdw	mm1, mm4
movq		mm4, mm3						; duplicate V1
;**********************************************************

paddw		mm3, mm5						; V53
psubw mm4, mm5						; V54 ;mm5 free

movq mm7, mm3						; duplicate V53
psubw mm1, mm6						; V55 ; mm6 free

movq mm6,  [ebx+8*7]	; V7
paddw mm3, mm2						; V56

movq mm5, mm4						; duplicate t140=t142
paddw mm4, mm1						; V57

movq  [esi+8*5], mm3		; V56
psubw mm5, mm1						; V58; mm1 free

pmullw mm6,  [edi+8*7]
psubw mm7, mm2						; V59; mm2 free

movq  [esi+8*13], mm4		; V57
movq mm3, mm0						; duplicate V11

; keep mm7 alive all along the next block
movq  [esi+8*9], mm5		; V58
paddw mm0, mm6						; V63

movq mm4,  [ebx+8*15]	; V15
psubw mm3, mm6						; V60 ; free mm6

pmullw mm4,  [edi+8*15]
; note that V15 computation has a correction step:
; this is a 'magic' constant that rebiases the results to be closer to the expected result
; this magic constant can be refined to reduce the error even more
; by doing the correction step in a later stage when the number is actually multiplied by 16
movq mm1, mm3						; duplicate V60

movq mm5,  [ebx+8*3]	; V3
movq		mm2, mm1

pmullw mm5,  [edi+8*3]

movq  scratch7, mm7
movq mm6, mm5						; duplicate V3

movq mm7,  x539f539f539f539f	; 23170 ->V18
paddw mm5, mm4						; V61

;***************************************************PackMulW
punpcklwd	mm1,  const_zero
psubw mm6, mm4						; V62 ; free mm4

pmaddwd		mm1, mm7
movq mm4, mm5						; duplicate V61

punpckhwd	mm2,  const_zero
paddw mm5, mm0						; V65 -> result

pmaddwd		mm2, mm7
psubw mm4, mm0						; V64 ; free mm0

movq  scratch3, mm3
psrad		mm1, 8

movq mm3,  x5a825a825a825a82	; 23170 ->V18
psrad		mm2, 8

packssdw	mm1, mm2
movq		mm2, mm4
;**********************************************************

;***************************************************PackMulW
punpcklwd	mm4,  const_zero

pmaddwd		mm4, mm3

punpckhwd	mm2,  const_zero

pmaddwd		mm2, mm3
psrad		mm4, 8

movq mm3,  scratch3

movq mm0,  x61f861f861f861f8	; 23170 ->V18
paddw		mm3, mm6						; V66

psrad		mm2, 8
movq		mm7, mm3

packssdw	mm4, mm2
movq mm2, mm5					; duplicate V65
;**********************************************************

;***************************************************PackMulW
punpcklwd	mm3,  const_zero

pmaddwd		mm3, mm0

punpckhwd	mm7,  const_zero

pmaddwd		mm7, mm0
movq		mm0, mm6

psrad		mm3, 8

punpcklwd	mm6,  const_zero

psrad		mm7, 8

packssdw	mm3, mm7
;**********************************************************

movq mm7,  x4546454645464546	; 23170 ->V18

;***************************************************PackMulW
punpckhwd	mm0,  const_zero
pmaddwd		mm6, mm7

pmaddwd		mm0, mm7
psrad		mm6, 8

psrad		mm0, 8

packssdw	mm6, mm0
;**********************************************************

movq mm0,  [esi+8*5]		; V56
psubw mm6, mm3						; V72

paddw mm3, mm1						; V71 ; free mm1

psubw mm3, mm2						; V73 ; free mm2
movq mm1, mm0						; duplicate t177=t188

psubw mm4, mm3						; V74
paddw mm0, mm5						; tm1

movq mm2,  [esi+8*13]     ; V57
paddw mm6, mm4						; V75

;location 
;  5 - V56
; 13 - V57
;  9 - V58
;  X - V59, mm7
;  X - V65, mm5
;  X - V73, mm6
;  X - V74, mm4
;  X - V75, mm3                              
; free mm0, mm1 & mm2                        

movq mm7,  scratch7      ; tm1; free mm0
psubw mm1, mm5						; tm15; free mm5

;save the store as used directly in the transpose
movq  [esi+8*1], mm0      ; tm1; free mm0
movq mm5, mm7                       ; duplicate t182=t184

movq mm0,  [esi+8*9]		; V58
psubw mm7, mm6						; tm7

paddw mm5, mm6						; tm9; free mm6
movq mm6, mm3

movq  [esi+8*7], mm7		; tm7; free mm7
movq mm3, mm2						; duplicate V57

psubw mm3, mm6						; tm13
paddw mm2, mm6						; tm3 ; free mm6

movq  [esi+8*3], mm2		; tm3; free mm2
movq mm6, mm0						; duplicate V58

paddw mm0, mm4						; tm5
psubw mm6, mm4						; tm11; free mm4

movq  [esi+8*5], mm0		; tm5; free mm0
movq		mm0, mm5			; copy w4---0,1,3,5,6


; transpose the bottom right quadrant(4X4) of the matrix
;  ---------       ---------
; | M1 | M2 |     | M1'| M3'|
;  ---------  -->  ---------
; | M3 | M4 |     | M2'| M4'|
;  ---------       ---------

punpcklwd	mm5, mm6			;

punpckhwd	mm0, mm6			;---0,1,3,5,6 
movq	mm6,  [esi+8*0]  ;get w0 of top left quadrant

movq		mm2, mm3			;---0,1,2,3,5,6
punpcklwd	mm3, mm1			;

movq	mm7,  [esi+8*2]  ;get w1 of top left quadrant
punpckhwd	mm2, mm1			;---0,2,3,5,6,7

movq		mm4, mm5			;---0,2,3,4,5,6,7
punpckldq	mm5, mm3			; transposed w4

movq	 [esi+8*9], mm5  ; store w4
punpckhdq	mm4, mm3			; transposed w5---0,2,4,6,7

movq		mm3, mm0			;---0,2,3,4,6,7
punpckldq	mm0, mm2			; transposed w6

movq	 [esi+8*11], mm4  ; store w5
punpckhdq	mm3, mm2			; transposed w7---0,3,6,7

movq	 [esi+8*13], mm0  ; store w6---3,5,6,7	
movq	mm5, mm6				; copy w0

movq	 [esi+8*15], mm3  ; store w7---5,6,7
punpcklwd	mm6, mm7

; transpose the top left quadrant(4X4) of the matrix

punpckhwd	mm5, mm7			;---5,6,7
movq	mm7,  [esi+8*4]  ; get w2 of TL quadrant

movq	mm4,  [esi+8*6]  ; get w3 of TL quadrant
movq	mm3, mm7				; copy w2---3,4,5,6,7

movq		mm2, mm6
punpcklwd	mm7, mm4			;---2,3,4,5,6,7

punpckhwd	mm3, mm4			;---2,3,4,5,6,7
movq		mm4, mm5			;	

movq		mm1, mm5
punpckldq	mm6, mm7			;---1,2,3,4,5,6,7

movq	 [esi+8*0], mm6	; store w0 of TL quadrant
punpckhdq	mm2, mm7			;---1,2,3,4,5,6,7

movq	 [esi+8*2], mm2	; store w1 of TL quadrant
punpckldq	mm5, mm3			;---1,2,3,4,5,6,7

movq	 [esi+8*4], mm5	; store w2 of TL quadrant
punpckhdq	mm1, mm3			;---1,2,3,4,5,6,7

movq	 [esi+8*6], mm1	; store w3 of TL quadrant


; transpose the top right quadrant(4X4) of the matrix

movq	mm0,  [esi+8*1]	;---0

movq	mm1,  [esi+8*3]	;---0,1,2
movq	mm2, mm0

movq	mm3,  [esi+8*5]
punpcklwd	mm0, mm1				;---0,1,2,3

punpckhwd	mm2, mm1
movq	mm1,  [esi+8*7]	;---0,1,2,3

movq	mm4, mm3
punpcklwd	mm3, mm1				;---0,1,2,3,4

punpckhwd	mm4, mm1				;---0,1,2,3,4
movq	mm1, mm0

movq	mm5, mm2
punpckldq	mm0, mm3				;---0,1,2,3,4,5

punpckhdq	mm1, mm3				;---0,1,2,3,4,5
movq		mm3,  [esi+8*8]

movq		 [esi+8*8], mm0
punpckldq	mm2, mm4				;---1,2,3,4,5

punpckhdq	mm5, mm4				;---1,2,3,4,5
movq		mm4,  [esi+8*10]

; transpose the bottom left quadrant(4X4) of the matrix
; Also store w1,w2,w3 of top right quadrant into
; w5,w6,w7 of bottom left quadrant. Storing w0 of TR in w4
; of BL is already done.

movq	 [esi+8*10], mm1
movq	mm1, mm3					;---1,2,3,4,5

movq	mm0,  [esi+8*12]
punpcklwd	mm3, mm4				;---0,1,2,3,4,5

punpckhwd	mm1, mm4				;---0,1,2,3,4,5
movq	mm4,  [esi+8*14]

movq	 [esi+8*12], mm2
movq	mm2, mm0

movq	 [esi+8*14], mm5
punpcklwd	mm0, mm4				;---0,1,2,3,4

punpckhwd	mm2, mm4				;---0,1,2,3,4
movq	mm4, mm3

movq	mm5, mm1
punpckldq	mm3, mm0				;---0,1,2,3,4,5

movq	 [esi+8*1], mm3
punpckhdq	mm4, mm0				;---1,2,4,5

movq	 [esi+8*3], mm4
punpckldq	mm1, mm2				;---1,2,5

movq	 [esi+8*5], mm1
punpckhdq	mm5, mm2				;---5

movq	 [esi+8*7], mm5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;   1D DCT of the rows    ;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


mov	esi, wsptr  ; source

; column 0: even part
; use V4, V12, V0, V8 to produce V22..V25
movq mm0,  [esi+8*12]	; V12

movq mm1,  [esi+8*4]	; V4

movq mm3,  [esi+8*0]	; V0
movq mm2, mm1						; duplicate V4

movq mm5,  [esi+8*8]	; V8
psubw mm1, mm0						; V16

movq mm6,  x5a825a825a825a82	; 23170 ->V18
;***************************************************PackMulW
movq		mm4, mm1

punpcklwd	mm1,  const_zero
paddw mm2, mm0						; V17

pmaddwd		mm1, mm6
movq mm0, mm2						; duplicate V17

punpckhwd	mm4,  const_zero

pmaddwd		mm4, mm6
psrad		mm1, 8

psrad		mm4, 8

packssdw	mm1, mm4
movq		mm4, mm3						; duplicate V0
;**********************************************************

paddw mm3, mm5						; V19
psubw mm4, mm5						; V20 ;mm5 free

movq mm6, mm3						; duplicate t74=t81
psubw mm1, mm0						; V21 ; mm0 free

paddw mm3, mm2						; V22
movq mm5, mm1						; duplicate V21

paddw mm1, mm4						; V23

movq  [esi+8*4], mm3		; V22
psubw mm4, mm5						; V24; mm5 free

movq  [esi+8*12], mm1		; V23
psubw mm6, mm2						; V25; mm2 free

movq  [esi+8*0], mm4		; V24
; keep mm6 alive all along the next block
; column 0: odd part
; use V2, V6, V10, V14 to produce V31, V39, V40, V41
movq mm7,  [esi+8*10]	; V10

movq mm0,  [esi+8*6]	; V6
movq mm3, mm7						; duplicate V10

movq mm5,  [esi+8*2]	; V2
psubw mm7, mm0						; V26

movq mm4,  [esi+8*14]	; V14
paddw mm3, mm0						; V29 ; free mm0

movq mm2,  x539f539f539f539f	; 23170 ->V18
movq mm1, mm7						; duplicate V26

;***************************************************PackMulW
movq		 scratch5, mm6	; store mm6
movq		mm0, mm7

punpcklwd	mm7,  const_zero

pmaddwd		mm7, mm2

punpckhwd	mm0,  const_zero

pmaddwd		mm0, mm2
psrad		mm7, 8

movq mm6,  x4546454645464546	; 23170 ->V18
psrad		mm0, 8

packssdw	mm7, mm0
movq		mm0, mm5				; duplicate V2
;**********************************************************

paddw mm5, mm4						; V27
psubw mm0, mm4						; V28 ; free mm4

movq mm2, mm0						; duplicate V28
;***************************************************PackMulW
movq		mm4, mm0

punpcklwd	mm0,  const_zero

pmaddwd		mm0, mm6

punpckhwd	mm4,  const_zero

pmaddwd		mm4, mm6
paddw mm1, mm2						; V32 ; free mm2

movq mm2,  x61f861f861f861f8	; 23170 ->V18
psrad		mm0, 8

psrad		mm4, 8
movq		mm6, mm1

packssdw	mm0, mm4
movq		mm4, mm5			; duplicate t90=t93
;**********************************************************

;***************************************************PackMulW
punpcklwd	mm1,  const_zero
paddw		mm5, mm3				; V31

pmaddwd		mm1, mm2
psubw		mm4, mm3				; V30 ; free mm3

punpckhwd	mm6,  const_zero

pmaddwd		mm6, mm2
psrad		mm1, 8

psrad		mm6, 8

packssdw	mm1, mm6
;**********************************************************

psubw mm0, mm1						; V38
paddw mm1, mm7						; V37 ; free mm7

movq		mm7,  x5a825a825a825a82	; 23170 ->V18
;***************************************************PackMulW
movq		mm3, mm4

punpcklwd	mm4,  const_zero
psubw		mm1, mm5				; V39 (mm5 still needed for next block)

pmaddwd		mm4, mm7

punpckhwd	mm3,  const_zero

movq		mm6,  scratch5
pmaddwd		mm3, mm7

movq mm2,  [esi+8*12]     ; V23
psrad		mm4, 8

movq mm7,  [esi+8*4]		; V22
psrad		mm3, 8

packssdw	mm4, mm3
movq		mm3, mm6                       ; duplicate V25
;**********************************************************						  

psubw mm4, mm1						; V40

paddw mm0, mm4						; V41; free mm0

; column 0: output butterfly

psubw mm6, mm0						; tm6
paddw mm3, mm0						; tm8; free mm1

movq mm0, mm1		; line added by Kumar
movq mm1, mm7						; duplicate V22

movq  [esi+8*8], mm3     ; tm8; free mm3
paddw mm7, mm5						; tm0

movq  [esi+8*6], mm6		; tm6; free mm6
psubw mm1, mm5						; tm14; free mm5

movq mm6,  [esi+8*0]		; V24
movq mm3, mm2						; duplicate t117=t125

movq  [esi+8*0], mm7      ; tm0; free mm7
paddw mm2, mm0						; tm2

movq  [esi+8*14], mm1		; tm14; free mm1
psubw mm3, mm0						; tm12; free mm0

movq  [esi+8*2], mm2		; tm2; free mm2
movq mm0, mm6						; duplicate t119=t123

movq  [esi+8*12], mm3      ; tm12; free mm3
paddw mm6, mm4						; tm4

movq mm1,  [esi+8*5]	; V5
psubw mm0, mm4						; tm10; free mm4

movq  [esi+8*4], mm6		; tm4; free mm6

movq  [esi+8*10], mm0     ; tm10; free mm0

; column 1: even part
; use V5, V13, V1, V9 to produce V56..V59

movq mm7,  [esi+8*13]	; V13
movq mm2, mm1						; duplicate t128=t130

movq mm3,  [esi+8*1]	; V1
psubw mm1, mm7						; V50

movq mm5,  [esi+8*9]	; V9
paddw mm2, mm7						; V51

movq mm4,  x5a825a825a825a82	; 23170 ->V18
;***************************************************PackMulW
movq		mm6, mm1

punpcklwd	mm1,  const_zero

pmaddwd		mm1, mm4

punpckhwd	mm6,  const_zero

pmaddwd		mm6, mm4
movq		mm4, mm3				; duplicate V1

paddw mm3, mm5						; V53
psrad		mm1, 8

psubw mm4, mm5						; V54 ;mm5 free
movq mm7, mm3						; duplicate V53

psrad		mm6, 8

packssdw	mm1, mm6
movq		mm6, mm2				; duplicate V51

;**********************************************************
psubw mm1, mm6						; V55 ; mm6 free
paddw mm3, mm2						; V56

movq mm5, mm4						; duplicate t140=t142
paddw mm4, mm1						; V57

movq  [esi+8*5], mm3		; V56
psubw mm5, mm1						; V58; mm1 free

movq  [esi+8*13], mm4		; V57
psubw mm7, mm2						; V59; mm2 free

movq  [esi+8*9], mm5		; V58

; keep mm7 alive all along the next block

movq mm0,  [esi+8*11]	; V11

movq mm6,  [esi+8*7]	; V7

movq mm4,  [esi+8*15]	; V15
movq mm3, mm0						; duplicate V11

movq mm5,  [esi+8*3]	; V3
paddw mm0, mm6						; V63

; note that V15 computation has a correction step:
; this is a 'magic' constant that rebiases the results to be closer to the expected result
; this magic constant can be refined to reduce the error even more
; by doing the correction step in a later stage when the number is actually multiplied by 16
movq	 scratch7, mm7
psubw mm3, mm6						; V60 ; free mm6

movq mm6,  x539f539f539f539f	; 23170 ->V18
movq mm1, mm3						; duplicate V60

;***************************************************PackMulW
movq		mm7, mm1

punpcklwd	mm1,  const_zero

pmaddwd		mm1, mm6

punpckhwd	mm7,  const_zero

pmaddwd		mm7, mm6
movq mm6, mm5						; duplicate V3

paddw mm5, mm4						; V61
psrad		mm1, 8

psubw mm6, mm4						; V62 ; free mm4
movq mm4, mm5						; duplicate V61

psrad		mm7, 8
paddw mm5, mm0						; V65 -> result

packssdw	mm1, mm7
psubw mm4, mm0						; V64 ; free mm0
;**********************************************************

movq mm7,  x5a825a825a825a82	; 23170 ->V18
;***************************************************PackMulW
movq		mm2, mm4

punpcklwd	mm4,  const_zero
paddw		mm3, mm6			; V66

pmaddwd		mm4, mm7

punpckhwd	mm2,  const_zero

pmaddwd		mm2, mm7

movq mm7,  x61f861f861f861f8	; 23170 ->V18
psrad		mm4, 8

psrad		mm2, 8

packssdw	mm4, mm2
;**********************************************************
;***************************************************PackMulW
movq		mm2, mm3

punpcklwd	mm3,  const_zero

pmaddwd		mm3, mm7

punpckhwd	mm2,  const_zero

pmaddwd		mm2, mm7

movq mm7,  x4546454645464546	; 23170 ->V18
psrad		mm3, 8

psrad		mm2, 8

packssdw	mm3, mm2
;**********************************************************
;***************************************************PackMulW
movq		mm2, mm6

punpcklwd	mm6,  const_zero

pmaddwd		mm6, mm7

punpckhwd	mm2,  const_zero

pmaddwd		mm2, mm7

movq mm0,  [esi+8*5]		; V56
psrad		mm6, 8

movq	mm7,  scratch7
psrad		mm2, 8

packssdw	mm6, mm2
movq		mm2, mm5			; duplicate V65
;**********************************************************

psubw mm6, mm3						; V72
paddw mm3, mm1						; V71 ; free mm1

psubw mm3, mm2						; V73 ; free mm2
movq mm1, mm0						; duplicate t177=t188

psubw mm4, mm3						; V74
paddw mm0, mm5						; tm1

movq mm2,  [esi+8*13]     ; V57
paddw mm6, mm4						; V75


;location 
;  5 - V56
; 13 - V57
;  9 - V58
;  X - V59, mm7
;  X - V65, mm5
;  X - V73, mm6
;  X - V74, mm4
;  X - V75, mm3                              
; free mm0, mm1 & mm2                        

movq  [esi+8*1], mm0      ; tm1; free mm0
psubw mm1, mm5						; tm15; free mm5

;save the store as used directly in the transpose
movq mm5, mm7                       ; duplicate t182=t184
psubw mm7, mm6						; tm7

paddw mm5, mm6						; tm9; free mm3
movq mm6, mm3

movq mm0,  [esi+8*9]		; V58
movq mm3, mm2						; duplicate V57

movq  [esi+8*7], mm7		; tm7; free mm7
psubw mm3, mm6						; tm13

paddw mm2, mm6						; tm3 ; free mm6
movq mm6, mm0						; duplicate V58

movq  [esi+8*3], mm2		; tm3; free mm2
paddw mm0, mm4						; tm5

psubw mm6, mm4						; tm11; free mm4

movq  [esi+8*5], mm0		; tm5; free mm0


; Final results to be stored after the transpose
; transpose the bottom right quadrant(4X4) of the matrix
;  ---------       ---------
; | M1 | M2 |     | M1'| M3'|
;  ---------  -->  ---------
; | M3 | M4 |     | M2'| M4'|
;  ---------       ---------
;
; get the pointer to array "range"
mov		edi, range_limit

; calculate the destination address
mov		edx,  output_buf		; get output_buf[4]

mov		ebx, [edx+16]
add		ebx,  output_col			; add to output_col	

movq		mm0, mm5			; copy w4---0,1,3,5,6
punpcklwd	mm5, mm6			;

punpckhwd	mm0, mm6			;---0,1,3,5,6
movq		mm2, mm3			;---0,1,2,3,5,6
 
movq	mm6,  [esi+8*0]  ;get w0 of top left quadrant
punpcklwd	mm3, mm1			;

movq	mm7,  [esi+8*2]  ;get w1 of top left quadrant
punpckhwd	mm2, mm1			;---0,2,3,5,6,7

movq		mm4, mm5			;---0,2,3,4,5,6,7
punpckldq	mm5, mm3			; transposed w4

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al

mov		ebx, [edx+20]
add		ebx,  output_col			; add to output_col	

punpckhdq	mm4, mm3			; transposed w5---0,2,4,6,7

movq		mm3, mm0			;---0,2,3,4,6,7
punpckldq	mm0, mm2			; transposed w6

psrlw	mm4, 5
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al

mov		ecx, [edx+24]			
add		ecx,  output_col			; add to output_col	

punpckhdq	mm3, mm2			; transposed w7---0,3,6,7

psrlw	mm0, 5

movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+4], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+5], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+6], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+7], al

mov		ebx, [edx+28]		
add		ebx,  output_col			; add to output_col	

movq	mm5, mm6				; copy w0

psrlw	mm3, 5
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al


punpcklwd	mm6, mm7

; transpose the top left quadrant(4X4) of the matrix

; calculate the destination address
mov		edx, output_buf		; get output_buf[0]

mov		ebx, [edx+0]
add		ebx, output_col			; add to output_col	


movq	mm4,  [esi+8*6]  ; get w3 of TL quadrant
punpckhwd	mm5, mm7			;---5,6,7

movq	mm7,  [esi+8*4]  ; get w2 of TL quadrant
movq		mm2, mm6

movq	mm3, mm7				; copy w2---3,4,5,6,7
punpcklwd	mm7, mm4			;---2,3,4,5,6,7

punpckhwd	mm3, mm4			;---2,3,4,5,6,7
movq		mm4, mm5			;	

movq		mm1, mm5
punpckldq	mm6, mm7			;---1,2,3,4,5,6,7

psrlw	mm6, 5
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm6, 16
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm6, 16
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm6, 16
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al


mov		ebx, [edx+4]
add		ebx, output_col			; add to output_col	

punpckhdq	mm2, mm7			;---1,2,3,4,5,6,7

psrlw	mm2, 5
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al


mov		ecx, [edx+8]			
add		ecx, output_col			; add to output_col	

punpckldq	mm5, mm3			;---1,2,3,4,5,6,7

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+1], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+2], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+3], al


mov		ebx, [edx+12]		
add		ebx, output_col			; add to output_col	

punpckhdq	mm1, mm3			;---1,2,3,4,5,6,7

psrlw	mm1, 5
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al


; transpose the top right quadrant(4X4) of the matrix

; calculate the destination address for **bottom left quadrant
mov		edx, output_buf		; get output_buf[4]

mov		ebx, [edx+16]
add		ebx, output_col			; add to output_col	

movq	mm0,  [esi+8*1]	;---0

movq	mm1,  [esi+8*3]	;---0,1,2
movq	mm2, mm0

movq	mm3,  [esi+8*5]
punpcklwd	mm0, mm1				;---0,1,2,3

punpckhwd	mm2, mm1
movq	mm4, mm3

movq	mm1,  [esi+8*7]	;---0,1,2,3
movq	mm5, mm2

punpcklwd	mm3, mm1				;---0,1,2,3,4

punpckhwd	mm4, mm1				;---0,1,2,3,4
movq	mm1, mm0

punpckldq	mm0, mm3				;---0,1,2,3,4,5

punpckhdq	mm1, mm3				;---0,1,2,3,4,5

movq		mm3,  [esi+8*8]
psrlw	mm0, 5
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al

mov		ebx, [edx+20]
add		ebx, output_col			; add to output_col	

punpckldq	mm2, mm4				;---1,2,3,4,5

punpckhdq	mm5, mm4				;---1,2,3,4,5
movq		mm4,  [esi+8*10]

; transpose the bottom left quadrant(4X4) of the matrix
; Also store w1,w2,w3 of top right quadrant into
; w5,w6,w7 of bottom left quadrant. Storing w0 of TR in w4
; of BL is already done.

psrlw	mm1, 5
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al

mov		ecx, [edx+24]			
add		ecx, output_col			; add to output_col	

movq	mm0,  [esi+8*12]
movq	mm1, mm3					;---1,2,3,4,5

punpcklwd	mm3, mm4				;---0,1,2,3,4,5

punpckhwd	mm1, mm4				;---0,1,2,3,4,5

movq	mm4,  [esi+8*14]
psrlw	mm2, 5
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+1], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+2], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+3], al

mov		ebx, [edx+28]		
add		ebx, output_col			; add to output_col	

movq	mm2, mm0

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al


psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al


punpcklwd	mm0, mm4				;---0,1,2,3,4

punpckhwd	mm2, mm4				;---0,1,2,3,4
movq	mm4, mm3

movq	mm5, mm1
punpckldq	mm3, mm0				;---0,1,2,3,4,5

; calculate the destination address for **top right quadrant
mov		edx, output_buf		; get output_buf[0]

mov		ebx, [edx+0]
add		ebx, output_col			; add to output_col	

psrlw	mm3, 5
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al

mov		ebx, [edx+4]
add		ebx, output_col			; add to output_col	

punpckhdq	mm4, mm0				;---1,2,4,5

psrlw	mm4, 5
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al

mov		ecx, [edx+8]			
add		ecx, output_col			; add to output_col	

punpckldq	mm1, mm2				;---1,2,5

psrlw	mm1, 5
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+4], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+5], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+6], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+7], al

mov		ebx, [edx+12]		
add		ebx, output_col			; add to output_col	

punpckhdq	mm5, mm2				;---5

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

emms


} /* end of __asm */
}


#endif /* X86 */

#endif /* DCT_IFAST_SUPPORTED */

