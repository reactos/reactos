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

** Pentium version of the "integer LLM mode" within IJG decompressor code.
** The following is a non-MMX Pentium implementation of the integer slow mode
** IDCT within the IJG code.
*/

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */

#ifdef DCT_ISLOW_SUPPORTED

#ifndef _X86_

GLOBAL(void)
pidct8x8llm (JCOEFPTR inptr, short *quantptr, short *wsptr,
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


static const INT32 const_0_2986	=	0x0000098E ;
static const INT32 const_0_3901	=	0x0fffff384;
static const INT32 const_0_54119	=	0x00001151;
static const INT32 const_0_7653	=	0x0000187E;
static const INT32 const_0_899	=	0x0ffffe333;
static const INT32 const_1_175	=	0x000025a1;
static const INT32 const_1_501	=	0x0000300b;
static const INT32 const_1_8477	=	0x0ffffc4df;
static const INT32 const_1_961	=	0x0ffffc13b;
static const INT32 const_2_053	=	0x000041b3;
static const INT32 const_2_562	=	0x0ffffadfd;
static const INT32 const_3_072	=	0x00006254;

static const INT32 const_round	=	0x00000400;
static const INT32 const_round_row	=	0x00020000;
static const INT32 const_mask		=	0x000003ff;


/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */

GLOBAL(void)
pidct8x8llm (JCOEFPTR inptr, short *quantptr, short *wsptr,
		 JSAMPARRAY output_buf, JDIMENSION output_col, JSAMPLE *range_limit )
{

INT32   locdwinptr, locdwqptr, locdwwsptr, locdwtmp0, locdwtmp1 ;
INT32   locdwtmp2, locdwtmp3, locdwtmp00, locdwtmp01, locdwtmp02 ;
INT32   locdwtmp03, locdwtmp10, locdwtmp11, locdwtmp12 ;
INT32   locdwtmp13, locdwcounter, locdwrowctr ;	



// Inline assembly to do the IDCT and store the result */

__asm {

mov		esi, inptr	; point to start of source
mov		edi, quantptr	;

mov		eax, wsptr
mov		locdwinptr, esi	; point to start of source

mov		locdwqptr, edi	;
mov		locdwwsptr, eax

mov	locdwcounter, 8
mov		eax, [esi]		; warm up the cache

mov		ebx, [esi+32]
mov		ecx, [esi+64]

mov		edx, [esi+96]
mov		eax, [edi]

mov		ebx, [edi+32]
mov		ecx, [edi+64]

mov		edx, [edi+96]

;; 1D-IDCT of all the eight columns
idct_column:

mov		esi, locdwinptr	; point to start of source
mov		edi, locdwqptr		;

;; do the even part

mov		ax, [esi+16*2]
mov		bx, [edi+16*2]

shl		eax, 16		; sign extend the i/p
mov		cx, [esi+16*6]

sar		eax, 16
mov		dx, [edi+16*6]

shl		ebx, 16		; sign extend the quant factor

sar		ebx, 16

imul	eax, ebx	; dequantized C2 = z2

shl		ecx, 16

sar		ecx, 16

shl		edx, 16

sar		edx, 16

imul	ecx, edx	; dequantized C6 = z3

mov		ebx, eax	; copy of z2

imul	eax, const_0_7653

add		ebx, ecx	; z2 + z3

imul	ecx, const_1_8477

imul	ebx, const_0_54119	; z1

mov		dx, [edi+16*4]	; quant factor for C4
add		ecx, ebx	; tmp2

add		eax, ebx	; tmp3
mov		locdwtmp2, ecx

mov		locdwtmp3, eax

mov		cx, [esi+16*4]	; C4
mov		ax, [esi+16*0]	; C0

mov		bx, [edi+16*0]	; quant factor for C0

movsx	edx, dx

movsx	ecx, cx

movsx	eax, ax

movsx	ebx, bx

imul	ecx, edx	; dequantize C4 = z3

imul	eax, ebx	; dequantize C0 = z2

mov		edx, ecx	; copy of z3
add		ecx, eax	; z2 + z3

shl		ecx, 13		; tmp0
sub		eax, edx	; z2 - z3

shl		eax, 13		; tmp1
mov		ebx, ecx	; copy of tmp0

add		ecx, locdwtmp3	; tmp10
mov		edx, eax	; copy of tmp1

add		eax, locdwtmp2	; tmp11
mov		locdwtmp00, ecx

sub		ebx, locdwtmp3	; tmp13
mov		locdwtmp01, eax

sub		edx, locdwtmp2	; tmp12
mov		locdwtmp03, ebx

mov		ax, [esi+16*7]	; C7 for the odd part
mov		locdwtmp02, edx

mov		bx, [edi+16*7]	; quant factor for C7

;; now do the odd part

shl		eax, 16
mov		cx, [esi+16*3]

sar		eax, 16
mov		dx, [edi+16*3]

shl		ebx, 16

sar		ebx, 16

imul	eax, ebx		; dequantized C7 = tmp0

shl		ecx, 16

sar		ecx, 16

shl		edx, 16

sar		edx, 16
mov		bx, [esi+16*1]

imul	ecx, edx		; dequantized C3 = tmp2

shl		ebx, 16
mov		dx, [edi+16*1]

sar		ebx, 16

shl		edx, 16

sar		edx, 16

imul	ebx, edx		; dequantized C1 = tmp3

mov		locdwtmp0, eax
mov		locdwtmp2, ecx

mov		ax, [esi+16*5]
mov		dx, [edi+16*5]

shl		eax, 16

sar		eax, 16

shl		edx, 16

sar		edx, 16

imul	eax, edx	; dequantized C5 = tmp1

imul	ecx, const_3_072	; tmp2

mov		locdwtmp3, ebx
mov		edx, locdwtmp0

imul	ebx, const_1_501	; tmp3

imul	edx, const_0_2986	; tmp0

mov		locdwtmp1, eax	; store tmp1
mov		locdwtmp10, edx

imul	eax, const_2_053	; tmp1

mov		locdwtmp11, eax
mov		locdwtmp12, ecx

mov		locdwtmp13, ebx
mov		eax, locdwtmp0

mov		ebx, locdwtmp1
mov		ecx, eax

mov		edx, ebx
add		eax, locdwtmp3	; z1

add		ebx, locdwtmp3	; z4
add		ecx, locdwtmp2	; z3

add		edx, locdwtmp2	; z2
mov		esi, ecx	; copy of z3

imul	eax,  const_0_899	; z1

imul	edx,  const_2_562	; z2

add		esi, ebx	; z3 + z4

imul	esi,  const_1_175	; z5

imul	ecx,  const_1_961	; z3

imul	ebx,  const_0_3901	; z4

add		ecx, esi	; z3
add		ebx, esi	; z4

mov		esi, eax	; copy of z1
add		eax, ecx	; z1 + z3

add		esi, ebx	; z1 + z4
add		ecx, edx	; z3 + z2

add		edx, ebx	; z2 + z4
add		eax, locdwtmp10		; tmp0

add		edx, locdwtmp11		; tmp1
add		ecx, locdwtmp12		; tmp2

add		esi, locdwtmp13		; tmp3
mov		ebx, locdwtmp03

sub		ebx, eax			; w4
add		eax, locdwtmp03		; w3

add		ebx,  const_round
mov		edi, locdwwsptr		; keep in mind that wsptr stores 32 bit values

sar		ebx, 11				; So store/update the pointer accordingly
add		eax,  const_round

sar		eax, 11
mov		[edi+32*4], ebx

mov		[edi+32*3], eax
mov		ebx, locdwtmp02

mov		eax, locdwtmp01
sub		ebx, edx			; w5

add		edx, locdwtmp02		; w2
sub		eax, ecx			; w6

add		ecx, locdwtmp01		; w1
add		ebx,  const_round

sar		ebx, 11
add		eax,  const_round

sar		eax, 11
add		edx,  const_round

add		ecx,  const_round
mov		[edi+32*5], ebx

sar		edx, 11
mov		[edi+32*6], eax

sar		ecx, 11
mov		[edi+32*2], edx

mov		eax, locdwtmp00
mov		[edi+32*1], ecx

mov		ebx, eax
sub		eax, esi			; w7

add		ebx, esi			; w0
add		eax, const_round

sar		eax, 11
add		ebx, const_round

sar		ebx, 11
mov		[edi+32*7], eax

mov		[edi+32*0], ebx
mov		eax, locdwcounter

add		locdwinptr, 2
add		locdwwsptr, 4		; wsptr stores 32 bit quantities

add		locdwqptr, 2
dec		eax

mov		locdwcounter, eax
jnz		idct_column

;; End of 1D-idct of all the columns

;; get ready for the 1D-idct of the rows

mov		esi, wsptr
mov		locdwcounter, 8

mov		locdwrowctr, 0
mov		locdwwsptr, esi


;; 1D-IDCT of all the eight rows
idct_row:

mov		esi, locdwwsptr	; point to start of source
mov		edi, output_buf	

add		edi, locdwrowctr
mov		edi, [edi]

add		locdwrowctr, 4
add		edi, output_col	; this is the dest start addr for this row


;; do the even part

mov		eax, [esi+4*2]
mov		ecx, [esi+4*6]

mov		ebx, eax	; copy of z2
mov		edx, [edi]	; warm up the cache for writing this output row

imul	eax, const_0_7653

add		ebx, ecx	; z2 + z3

imul	ecx,  const_1_8477

imul	ebx,  const_0_54119	; z1

add		ecx, ebx	; tmp2
add		eax, ebx	; tmp3

mov		locdwtmp2, ecx
mov		locdwtmp3, eax

mov		ecx, [esi+4*4]	; C4
mov		eax, [esi+4*0]	; C0

mov		edx, ecx	; copy of z3

add		ecx, eax	; z2 + z3
sub		eax, edx	; z2 - z3

shl		ecx, 13		; tmp0

shl		eax, 13		; tmp1
mov		ebx, ecx	; copy of tmp0

add		ecx, locdwtmp3	; tmp10
mov		edx, eax	; copy of tmp1

add		eax, locdwtmp2	; tmp11
mov		locdwtmp00, ecx

sub		ebx, locdwtmp3	; tmp13
mov		locdwtmp01, eax

sub		edx, locdwtmp2	; tmp12
mov		locdwtmp03, ebx

mov		eax, [esi+4*7]	; C7 for the odd part
mov		locdwtmp02, edx

;; now do the odd part

mov		ecx, [esi+4*3]
mov		ebx, [esi+4*1]

mov		locdwtmp0, eax
mov		locdwtmp2, ecx

mov		eax, [esi+4*5]
mov		locdwtmp3, ebx

imul	ecx,  const_3_072	; tmp2

mov		edx, locdwtmp0

imul	ebx,  const_1_501	; tmp3

imul	edx,  const_0_2986	; tmp0

mov		locdwtmp1, eax	; store tmp1

imul	eax, const_2_053	; tmp1

mov		locdwtmp10, edx
mov		locdwtmp11, eax

mov		locdwtmp12, ecx
mov		locdwtmp13, ebx

mov		eax, locdwtmp0
mov		ebx, locdwtmp1

mov		ecx, eax
mov		edx, ebx

add		eax, locdwtmp3	; z1
add		edx, locdwtmp2	; z2

add		ebx, locdwtmp3	; z4
add		ecx, locdwtmp2	; z3

mov		esi, ecx	; copy of z3

imul	eax,  const_0_899	; z1

imul	edx, const_2_562	; z2

add		esi, ebx	; z3 + z4

imul	esi, const_1_175	; z5

imul	ecx, const_1_961	; z3

imul	ebx,  const_0_3901	; z4

add		ecx, esi	; z3
add		ebx, esi	; z4

mov		esi, eax	; copy of z1
add		eax, ecx	; z1 + z3

add		esi, ebx	; z1 + z4
add		ecx, edx	; z3 + z2

add		edx, ebx	; z2 + z4
add		eax, locdwtmp10		; tmp0

add		edx, locdwtmp11		; tmp1
add		ecx, locdwtmp12		; tmp2

add		esi, locdwtmp13		; tmp3
mov		locdwtmp0, eax

mov		locdwtmp1, edx
mov		locdwtmp2, ecx

mov		locdwtmp3, esi
mov		ebx, locdwtmp03

add		ebx, locdwtmp0	; out3
mov		ecx, locdwtmp00

sub		ecx, locdwtmp3	; out7
add		ebx,  const_round_row

sar		ebx, 18
add		ecx,  const_round_row

sar		ecx, 18
mov		esi, range_limit

and		ebx,  const_mask
and		ecx, const_mask

mov		al, [esi][ebx]
mov		dl, [esi][ecx]

mov		ebx, locdwtmp02
mov		ecx, locdwtmp01

add		ebx, locdwtmp1	; out2
sub		ecx, locdwtmp2	; out6

shl		eax, 8		; get ready to receive next output byte
add		ebx,  const_round_row

shl		edx, 8		; get ready to receive next output byte
add		ecx, const_round_row

sar		ebx, 18

sar		ecx, 18
and		ebx,  const_mask

and		ecx,  const_mask
mov		al, [esi][ebx]

mov		dl, [esi][ecx]
mov		ebx, locdwtmp01

mov		ecx, locdwtmp02
add		ebx, locdwtmp2	; out1

shl		eax, 8		; get ready to receive next output byte
sub		ecx, locdwtmp1	; out5

shl		edx, 8		; get ready to receive next output byte
add		ebx,  const_round_row

sar		ebx, 18
add		ecx,  const_round_row

sar		ecx, 18
and		ebx,  const_mask

and		ecx,  const_mask
mov		al, [esi][ebx]	; out1

mov		dl, [esi][ecx]	; out5
mov		ebx, locdwtmp00

mov		ecx, locdwtmp03
add		ebx, locdwtmp3	; out0

shl		eax, 8		; get ready to receive next output byte
sub		ecx, locdwtmp0	; out4

shl		edx, 8		; get ready to receive next output byte
add		ebx,  const_round_row

sar		ebx, 18
add		ecx,  const_round_row

sar		ecx, 18
and		ebx,  const_mask

and		ecx,  const_mask
mov		al, [esi][ebx]	; out0

mov		dl, [esi][ecx]	; out4
mov		[edi], eax		; store the first four bytes

mov		[edi+4], edx	; store the next four bytes of this row
mov		eax, locdwcounter

add		locdwwsptr, 32		; wsptr stores 32 bit quantities
dec		eax

mov		locdwcounter, eax
jnz		idct_row

} //end of __asm

}


#endif /* X86 */

#endif /* DCT_ISLOW_SUPPORTED */

