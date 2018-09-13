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


static const long  x5a825a825a825a82	= 0x0000016a ;				
static const long  x539f539f539f539f 	= 0xfffffd63 ;
static const long  x4546454645464546	= 0x00000115 ;	
static const long  x61f861f861f861f8	= 0x000001d9 ;	


/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */

GLOBAL(void)
pidct8x8aan (JCOEFPTR coef_block, short * wsptr, short * quantptr,
		 JSAMPARRAY output_buf, JDIMENSION output_col, JSAMPLE *range_limit )
{

  INT32	locdwinptr, locdwqptr, locdwwsptr, locwctr ;
  short locwcounter, locwtmp0, locwtmp1	;
  short locwtmp3, scratch1, scratch2, scratch3 ;


  
  // do the 2-Dal idct and store the corresponding results
  // from the range_limit array
//  pidct(coef_block, quantptr, wsptr, output_buf, output_col, range_limit) ;

__asm {


mov esi, coef_block   ; source coeff
mov edi, quantptr	  ; quant pointer

mov locdwinptr, esi
mov eax, wsptr	  ; temp storage pointer

mov locdwqptr, edi
mov locdwwsptr, eax

mov locwcounter, 8

;; perform the 1D-idct on each of the eight columns

idct_column:

mov esi, locdwinptr
mov edi, locdwqptr

mov ax, word ptr [esi+16*0]

mov bx, word ptr [esi+16*4]
imul ax, word ptr [edi+16*0]

mov cx, word ptr [esi+16*2]

imul bx, word ptr [edi+16*4]

mov dx, word ptr [esi+16*6]
imul cx, word ptr [edi+16*2]

imul dx, word ptr [edi+16*6]

;;;; at this point C0, C2, C4 and C6 have been dequantized

mov scratch1, ax
add ax, bx		; tmp10 in ax

sub scratch1, bx		; tmp11 
mov bx, cx

add cx, dx		; tmp13 in cx
sub bx, dx		; tmp1 - tmp3 in bx

mov dx, ax
movsx ebx, bx	; sign extend bx: get ready to do imul

add ax, cx		; tmp0 in ax
imul ebx, dword ptr x5a825a825a825a82

sub dx, cx		; tmp3 in dx
mov locwtmp0, ax 

mov locwtmp3, dx
sar ebx, 8		; bx now has (tmp1-tmp3)*1.414

mov ax, scratch1	; copy of tmp11
sub bx, cx		; tmp12 in bx

add ax, bx		; tmp1 in ax
sub scratch1, bx		; tmp2 

mov locwtmp1, ax

;;;;;completed computing/storing the even part;;;;;;;;;; 

mov ax, [esi+16*1]		; get C1

imul ax, [edi+16*1]
mov bx, [esi+16*7]		; get C7

mov cx, [esi+16*3]

imul bx, [edi+16*7]	

mov dx, [esi+16*5]

imul cx, [edi+16*3]

imul dx, [edi+16*5]

mov scratch2, ax
add ax, bx		; z11 in ax

sub scratch2, bx		; z12 
mov bx, dx		; copy of deQ C5

add dx, cx		; z13 in dx
sub bx, cx		; z10 in bx

mov cx, ax		; copy of z11
add ax, dx		; tmp7 in ax

sub cx, dx		; partial tmp11

movsx ecx, cx
mov dx, bx		; copy of z10

add bx, scratch2		; partial z5 
imul ecx, dword ptr x5a825a825a825a82

movsx edx, dx	; sign extend z10: get ready for imul
movsx ebx, bx	; sign extend partial z5 for imul

imul edx, dword ptr x539f539f539f539f	; partial tmp12
imul ebx, dword ptr x61f861f861f861f8	; partial z5 product

mov	di, scratch2
movsx edi, di	; sign extend z12: get ready for imul
sar ecx, 8		; tmp11 in cx

sar ebx, 8		; z5 in bx
imul edi, dword ptr x4546454645464546

sar edx, 8
sar edi, 8

sub di, bx		; tmp10 
add dx, bx		; tmp12 in dx

sub dx, ax		; tmp6 in dx

sub cx, dx		; tmp5 in cx

add di, cx		; tmp4 
mov	scratch3, di

;;; completed calculating the odd part ;;;;;;;;;;;

mov edi, dword ptr locdwwsptr	; get address of temp. destn

mov si, ax		; copy of tmp7
mov bx, locwtmp0	; get tmp0

add ax, locwtmp0	; wsptr[0]
sub bx, si		; wsptr[7]

mov word ptr [edi+16*0], ax
mov word ptr [edi+16*7], bx

mov ax, dx		; copy of tmp6
mov bx, locwtmp1

add dx, bx		; wsptr[1]
sub bx, ax		; wsptr[6]

mov word ptr [edi+16*1], dx
mov word ptr [edi+16*6], bx

mov dx, cx		; copy of tmp5
mov bx, scratch1


add cx, bx		; wsptr[2]
sub bx, dx		; wsptr[5]

mov word ptr [edi+16*2], cx
mov word ptr [edi+16*5], bx

mov cx, scratch3		; copy of tmp4
mov ax, locwtmp3

add scratch3, ax		; wsptr[4]
sub ax, cx		; wsptr[3]

mov	bx, scratch3
mov word ptr [edi+16*4], bx
mov word ptr [edi+16*3], ax

;;;;; completed storing 1D idct of one column ;;;;;;;;

;; update inptr, qptr and wsptr for next column

add locdwinptr, 2
add locdwqptr, 2

add locdwwsptr, 2
mov ax, locwcounter	; get loop count

dec ax		; another loop done

mov locwcounter, ax
jnz idct_column

;;;;;;; end of 1D idct on all columns  ;;;;;;;
;;;;;;; temp result is stored in wsptr  ;;;;;;;

;;;;;;; perform 1D-idct on each row and store final result

mov esi, wsptr	; initialize source ptr to original wsptr
mov locwctr, 0

mov locwcounter, 8
mov locdwwsptr, esi

idct_row:

mov edi, output_buf
mov esi, locdwwsptr

add edi, locwctr

mov	edi, [edi]		; get output_buf[ctr]

add edi, output_col	; now edi is pointing to the resp. row
add locwctr, 4

;; get even coeffs. and do the even part

mov ax, word ptr [esi+2*0]

mov bx, word ptr [esi+2*4]

mov cx, word ptr [esi+2*2]

mov dx, word ptr [esi+2*6]

mov scratch1, ax
add ax, bx		; tmp10 in ax

sub scratch1, bx		; tmp11 
mov bx, cx

add cx, dx		; tmp13 in cx
sub bx, dx		; tmp1 - tmp3 in bx

mov dx, ax
movsx ebx, bx	; sign extend bx: get ready to do imul

add ax, cx		; tmp0 in ax
imul ebx, dword ptr x5a825a825a825a82

sub dx, cx		; tmp3 in dx
mov locwtmp0, ax 

mov locwtmp3, dx
sar ebx, 8		; bx now has (tmp1-tmp3)*1.414

mov ax, scratch1	; copy of tmp11
sub bx, cx		; tmp12 in bx

add ax, bx		; tmp1 in ax
sub scratch1, bx		; tmp2 

mov locwtmp1, ax

;;;;;completed computing/storing the even part;;;;;;;;;; 

mov ax, [esi+2*1]		; get C1
mov bx, [esi+2*7]		; get C7

mov cx, [esi+2*3]
mov dx, [esi+2*5]

mov scratch2, ax
add ax, bx		; z11 in ax

sub scratch2, bx		; z12 
mov bx, dx		; copy of deQ C5

add dx, cx		; z13 in dx
sub bx, cx		; z10 in bx

mov cx, ax		; copy of z11
add ax, dx		; tmp7 in ax

sub cx, dx		; partial tmp11

movsx ecx, cx
mov dx, bx		; copy of z10

add bx, scratch2	; partial z5 
imul ecx, dword ptr x5a825a825a825a82

movsx edx, dx	; sign extend z10: get ready for imul
movsx ebx, bx	; sign extend partial z5 for imul

imul edx, dword ptr x539f539f539f539f	; partial tmp12
imul ebx, dword ptr x61f861f861f861f8	; partial z5 product

mov	si, scratch2
movsx esi, si	; sign extend z12: get ready for imul
sar ecx, 8		; tmp11 in cx

sar ebx, 8		; z5 in bx
imul esi, dword ptr x4546454645464546

sar edx, 8
sar esi, 8

sub si, bx		; tmp10 
add dx, bx		; tmp12 in dx

sub dx, ax		; tmp6 in dx

sub cx, dx		; tmp5 in cx

add si, cx		; tmp4 
mov	scratch3, si

;;; completed calculating the odd part ;;;;;;;;;;;

mov si, ax		; copy of tmp7
mov bx, locwtmp0	; get tmp0

add ax, locwtmp0	; wsptr[0]
sub bx, si		; wsptr[7]

mov esi, range_limit	; initialize esi to range_limit pointer

sar ax, 5
sar bx, 5

and eax, 3ffh
and ebx, 3ffh

mov al, byte ptr [esi][eax]
mov bl, byte ptr [esi][ebx]

mov byte ptr [edi+0], al
mov byte ptr [edi+7], bl

mov ax, dx		; copy of tmp6
mov bx, locwtmp1

add dx, bx		; wsptr[1]
sub bx, ax		; wsptr[6]

sar dx, 5
sar bx, 5

and edx, 3ffh
and ebx, 3ffh

mov dl, byte ptr [esi][edx]
mov bl, byte ptr [esi][ebx]

mov byte ptr [edi+1], dl
mov byte ptr [edi+6], bl

mov dx, cx		; copy of tmp5
mov bx, scratch1

add cx, bx		; wsptr[2]
sub bx, dx		; wsptr[5]

sar cx, 5
sar bx, 5

and ecx, 3ffh
and ebx, 3ffh

mov cl, byte ptr [esi][ecx]
mov bl, byte ptr [esi][ebx]

mov byte ptr [edi+2], cl
mov byte ptr [edi+5], bl

mov cx, scratch3		; copy of tmp4
mov ax, locwtmp3

add scratch3, ax		; wsptr[4]
sub ax, cx		; wsptr[3]

sar scratch3, 5
sar ax, 5

mov	cx, scratch3

and ecx, 3ffh
and eax, 3ffh


mov bl, byte ptr [esi][ecx]
mov al, byte ptr [esi][eax]

mov byte ptr [edi+4], bl
mov byte ptr [edi+3], al

;;;;; completed storing 1D idct of one row ;;;;;;;;

;; update the source pointer (wsptr) for next row

add locdwwsptr, 16

mov ax, locwcounter	; get loop count

dec ax		; another loop done

mov locwcounter, ax
jnz idct_row


;; end of 1D idct on all rows
;; final result is stored in outptr

}	/* end of __asm */
}

#endif /* DCT_IFAST_SUPPORTED */
