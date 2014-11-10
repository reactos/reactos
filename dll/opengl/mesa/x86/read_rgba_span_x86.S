/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
/**
 * \file read_rgba_span_x86.S
 * Optimized routines to transfer pixel data from the framebuffer to a
 * buffer in main memory.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

	.file	"read_rgba_span_x86.S"
#if !defined(__DJGPP__) && !defined(__MINGW32__) && !defined(__APPLE__) /* this one cries for assyntax.h */
/* Kevin F. Quinn 2nd July 2006
 * Replaced data segment constants with text-segment instructions.
 */
#define	LOAD_MASK(mvins,m1,m2) \
   	pushl	$0xff00ff00 ;\
   	pushl	$0xff00ff00 ;\
   	pushl	$0xff00ff00 ;\
   	pushl	$0xff00ff00 ;\
	mvins	(%esp), m1	;\
   	pushl	$0x00ff0000 ;\
   	pushl	$0x00ff0000 ;\
   	pushl	$0x00ff0000 ;\
   	pushl	$0x00ff0000 ;\
	mvins	(%esp), m2	;\
	addl	$32, %esp

/* I implemented these as macros because they appear in several places,
 * and I've tweaked them a number of times.  I got tired of changing every
 * place they appear. :)
 */

#define DO_ONE_PIXEL() \
	movl	(%ebx), %eax ; \
	addl	$4, %ebx ; \
	bswap	%eax          /* ARGB -> BGRA */ ; \
	rorl	$8, %eax      /* BGRA -> ABGR */ ; \
	movl	%eax, (%ecx)  /* ABGR -> R, G, B, A */ ; \
	addl	$4, %ecx

#define DO_ONE_LAST_PIXEL() \
	movl	(%ebx), %eax ; \
	bswap	%eax          /* ARGB -> BGRA */ ; \
	rorl	$8, %eax      /* BGRA -> ABGR */ ; \
	movl	%eax, (%ecx)  /* ABGR -> R, G, B, A */ ; \


/**
 * MMX optimized version of the BGRA8888_REV to RGBA copy routine.
 * 
 * \warning
 * This function assumes that the caller will issue the EMMS instruction
 * at the correct places.
 */

.globl _generic_read_RGBA_span_BGRA8888_REV_MMX
#ifndef USE_DRICORE
.hidden _generic_read_RGBA_span_BGRA8888_REV_MMX
#endif
	.type	_generic_read_RGBA_span_BGRA8888_REV_MMX, @function
_generic_read_RGBA_span_BGRA8888_REV_MMX:
	pushl	%ebx

#ifdef USE_INNER_EMMS
	emms
#endif
	LOAD_MASK(movq,%mm1,%mm2)

	movl	8(%esp), %ebx	/* source pointer */
	movl	16(%esp), %edx	/* number of pixels to copy */
	movl	12(%esp), %ecx	/* destination pointer */

	testl	%edx, %edx
	jle	.L20		/* Bail if there's nothing to do. */

	movl	%ebx, %eax

	negl	%eax
	sarl	$2, %eax
	andl	$1, %eax
	je	.L17

	subl	%eax, %edx
	DO_ONE_PIXEL()
.L17:

	/* Would it be faster to unroll this loop once and process 4 pixels
	 * per pass, instead of just two?
	 */

	movl	%edx, %eax
	shrl	%eax
	jmp	.L18
.L19:
	movq	(%ebx), %mm0
	addl	$8, %ebx

	/* These 9 instructions do what PSHUFB (if there were such an
	 * instruction) could do in 1. :(
	 */

	movq	%mm0, %mm3
	movq	%mm0, %mm4

	pand	%mm2, %mm3
	psllq	$16, %mm4
	psrlq	$16, %mm3
	pand	%mm2, %mm4

	pand	%mm1, %mm0
	por	%mm4, %mm3
	por	%mm3, %mm0

	movq	%mm0, (%ecx)
	addl	$8, %ecx
	subl	$1, %eax
.L18:
	jne	.L19

#ifdef USE_INNER_EMMS
	emms
#endif

	/* At this point there are either 1 or 0 pixels remaining to be
	 * converted.  Convert the last pixel, if needed.
	 */

	testl	$1, %edx
	je	.L20

	DO_ONE_LAST_PIXEL()

.L20:
	popl	%ebx
	ret
	.size	_generic_read_RGBA_span_BGRA8888_REV_MMX, .-_generic_read_RGBA_span_BGRA8888_REV_MMX


/**
 * SSE optimized version of the BGRA8888_REV to RGBA copy routine.  SSE
 * instructions are only actually used to read data from the framebuffer.
 * In practice, the speed-up is pretty small.
 *
 * \todo
 * Do some more testing and determine if there's any reason to have this
 * function in addition to the MMX version.
 *
 * \warning
 * This function assumes that the caller will issue the EMMS instruction
 * at the correct places.
 */

.globl _generic_read_RGBA_span_BGRA8888_REV_SSE
#ifndef USE_DRICORE
.hidden _generic_read_RGBA_span_BGRA8888_REV_SSE
#endif
	.type	_generic_read_RGBA_span_BGRA8888_REV_SSE, @function
_generic_read_RGBA_span_BGRA8888_REV_SSE:
	pushl	%esi
	pushl	%ebx
	pushl	%ebp

#ifdef USE_INNER_EMMS
	emms
#endif

	LOAD_MASK(movq,%mm1,%mm2)

	movl	16(%esp), %ebx	/* source pointer */
	movl	24(%esp), %edx	/* number of pixels to copy */
	movl	20(%esp), %ecx	/* destination pointer */

	testl	%edx, %edx
	jle	.L35		/* Bail if there's nothing to do. */

	movl	%esp, %ebp
	subl	$16, %esp
	andl	$0xfffffff0, %esp

	movl	%ebx, %eax
	movl	%edx, %esi

	negl	%eax
	andl	$15, %eax
	sarl	$2, %eax
	cmpl	%edx, %eax
	cmovle	%eax, %esi

	subl	%esi, %edx

	testl	$1, %esi
	je	.L32

	DO_ONE_PIXEL()
.L32:

	testl	$2, %esi
	je	.L31

	movq	(%ebx), %mm0
	addl	$8, %ebx

	movq	%mm0, %mm3
	movq	%mm0, %mm4
	
	pand	%mm2, %mm3
	psllq	$16, %mm4
	psrlq	$16, %mm3
	pand	%mm2, %mm4

	pand	%mm1, %mm0
	por	%mm4, %mm3
	por	%mm3, %mm0

	movq	%mm0, (%ecx)
	addl	$8, %ecx
.L31:

	movl	%edx, %eax
	shrl	$2, %eax
	jmp	.L33
.L34:
	movaps	(%ebx), %xmm0
	addl	$16, %ebx

	/* This would be so much better if we could just move directly from
	 * an SSE register to an MMX register.  Unfortunately, that
	 * functionality wasn't introduced until SSE2 with the MOVDQ2Q
	 * instruction.
	 */

	movaps	%xmm0, (%esp)
	movq	(%esp), %mm0
	movq	8(%esp), %mm5

	movq	%mm0, %mm3
	movq	%mm0, %mm4
	movq	%mm5, %mm6
	movq	%mm5, %mm7

	pand	%mm2, %mm3
	pand	%mm2, %mm6

	psllq	$16, %mm4
	psllq	$16, %mm7

	psrlq	$16, %mm3
	psrlq	$16, %mm6

	pand	%mm2, %mm4
	pand	%mm2, %mm7

	pand	%mm1, %mm0
	pand	%mm1, %mm5

	por	%mm4, %mm3
	por	%mm7, %mm6

	por	%mm3, %mm0
	por	%mm6, %mm5

	movq	%mm0, (%ecx)
	movq	%mm5, 8(%ecx)
	addl	$16, %ecx

	subl	$1, %eax
.L33:
	jne	.L34

#ifdef USE_INNER_EMMS
	emms
#endif
	movl	%ebp, %esp

	/* At this point there are either [0, 3] pixels remaining to be
	 * converted.
	 */

	testl	$2, %edx
	je	.L36

	movq	(%ebx), %mm0
	addl	$8, %ebx

	movq	%mm0, %mm3
	movq	%mm0, %mm4
	
	pand	%mm2, %mm3
	psllq	$16, %mm4
	psrlq	$16, %mm3
	pand	%mm2, %mm4

	pand	%mm1, %mm0
	por	%mm4, %mm3
	por	%mm3, %mm0

	movq	%mm0, (%ecx)
	addl	$8, %ecx
.L36:

	testl	$1, %edx
	je	.L35

	DO_ONE_LAST_PIXEL()
.L35:
	popl	%ebp
	popl	%ebx
	popl	%esi
	ret
	.size	_generic_read_RGBA_span_BGRA8888_REV_SSE, .-_generic_read_RGBA_span_BGRA8888_REV_SSE


/**
 * SSE2 optimized version of the BGRA8888_REV to RGBA copy routine.
 */

	.text
.globl _generic_read_RGBA_span_BGRA8888_REV_SSE2
#ifndef USE_DRICORE
.hidden _generic_read_RGBA_span_BGRA8888_REV_SSE2
#endif
	.type	_generic_read_RGBA_span_BGRA8888_REV_SSE2, @function
_generic_read_RGBA_span_BGRA8888_REV_SSE2:
	pushl	%esi
	pushl	%ebx

	LOAD_MASK(movdqu,%xmm1,%xmm2)

	movl	12(%esp), %ebx	/* source pointer */
	movl	20(%esp), %edx	/* number of pixels to copy */
	movl	16(%esp), %ecx	/* destination pointer */

	movl	%ebx, %eax
	movl	%edx, %esi

	testl	%edx, %edx
	jle	.L46		/* Bail if there's nothing to do. */

	/* If the source pointer isn't a multiple of 16 we have to process
	 * a few pixels the "slow" way to get the address aligned for
	 * the SSE fetch intsructions.
	 */

	negl	%eax
	andl	$15, %eax
	sarl	$2, %eax

	cmpl	%edx, %eax
	cmovbe	%eax, %esi
	subl	%esi, %edx

	testl	$1, %esi
	je	.L41

	DO_ONE_PIXEL()  
.L41:
	testl	$2, %esi
	je	.L40

	movq	(%ebx), %xmm0
	addl	$8, %ebx

	movdqa	%xmm0, %xmm3
	movdqa	%xmm0, %xmm4
	andps	%xmm1, %xmm0

	andps	%xmm2, %xmm3
	pslldq	$2, %xmm4
	psrldq	$2, %xmm3
	andps	%xmm2, %xmm4

	orps	%xmm4, %xmm3
	orps	%xmm3, %xmm0

	movq	%xmm0, (%ecx)
	addl	$8, %ecx
.L40:

	/* Would it be worth having a specialized version of this loop for
	 * the case where the destination is 16-byte aligned?  That version
	 * would be identical except that it could use movedqa instead of
	 * movdqu.
	 */

	movl	%edx, %eax
	shrl	$2, %eax
	jmp	.L42
.L43:
	movdqa	(%ebx), %xmm0
	addl	$16, %ebx

	movdqa	%xmm0, %xmm3
	movdqa	%xmm0, %xmm4
	andps	%xmm1, %xmm0

	andps	%xmm2, %xmm3
	pslldq	$2, %xmm4
	psrldq	$2, %xmm3
	andps	%xmm2, %xmm4

	orps	%xmm4, %xmm3
	orps	%xmm3, %xmm0

	movdqu	%xmm0, (%ecx)
	addl	$16, %ecx
	subl	$1, %eax
.L42:
	jne	.L43


	/* There may be upto 3 pixels remaining to be copied.  Take care
	 * of them now.  We do the 2 pixel case first because the data
	 * will be aligned.
	 */

	testl	$2, %edx
	je	.L47

	movq	(%ebx), %xmm0
	addl	$8, %ebx
        
	movdqa	%xmm0, %xmm3
	movdqa	%xmm0, %xmm4
	andps	%xmm1, %xmm0

	andps	%xmm2, %xmm3
	pslldq	$2, %xmm4
	psrldq	$2, %xmm3
	andps	%xmm2, %xmm4

	orps	%xmm4, %xmm3
	orps	%xmm3, %xmm0

	movq	%xmm0, (%ecx)
	addl	$8, %ecx        
.L47:

	testl	$1, %edx
	je	.L46

	DO_ONE_LAST_PIXEL()  
.L46:

	popl	%ebx
	popl	%esi
	ret
	.size	_generic_read_RGBA_span_BGRA8888_REV_SSE2, .-_generic_read_RGBA_span_BGRA8888_REV_SSE2



#define MASK_565_L	0x07e0f800
#define MASK_565_H	0x0000001f
/* Setting SCALE_ADJUST to 5 gives a perfect match with the
 * classic C implementation in Mesa.  Setting SCALE_ADJUST
 * to 0 is slightly faster but at a small cost to accuracy.
 */
#define SCALE_ADJUST	5
#if SCALE_ADJUST == 5
#define PRESCALE_L 0x00100001
#define PRESCALE_H 0x00000200
#define SCALE_L 0x40C620E8
#define SCALE_H 0x0000839d
#elif SCALE_ADJUST == 0
#define PRESCALE_L 0x00200001
#define PRESCALE_H 0x00000800
#define SCALE_L 0x01040108
#define SCALE_H 0x00000108
#else
#error SCALE_ADJUST must either be 5 or 0.
#endif
#define ALPHA_L 0x00000000
#define ALPHA_H 0x00ff0000

/**
 * MMX optimized version of the RGB565 to RGBA copy routine.
 */

	.text
	.globl	_generic_read_RGBA_span_RGB565_MMX
#ifndef USE_DRICORE
        .hidden _generic_read_RGBA_span_RGB565_MMX
#endif
	.type	_generic_read_RGBA_span_RGB565_MMX, @function

_generic_read_RGBA_span_RGB565_MMX:

#ifdef USE_INNER_EMMS
	emms
#endif

	movl	4(%esp), %eax	/* source pointer */
	movl	8(%esp), %edx	/* destination pointer */
	movl	12(%esp), %ecx	/* number of pixels to copy */

	pushl	$MASK_565_H
	pushl	$MASK_565_L
	movq	(%esp), %mm5
	pushl	$PRESCALE_H
	pushl	$PRESCALE_L
	movq	(%esp), %mm6
	pushl	$SCALE_H
	pushl	$SCALE_L
	movq	(%esp), %mm7
	pushl	$ALPHA_H
	pushl	$ALPHA_L
	movq	(%esp), %mm3
	addl	$32,%esp

	sarl	$2, %ecx
	jl	.L01		/* Bail early if the count is negative. */
	jmp	.L02

.L03:
	/* Fetch 4 RGB565 pixels into %mm4.  Distribute the first and
	 * second pixels into the four words of %mm0 and %mm2.
      	 */

	movq	(%eax), %mm4
	addl	$8, %eax

	pshufw	$0x00, %mm4, %mm0
	pshufw	$0x55, %mm4, %mm2


	/* Mask the pixels so that each word of each register contains only
	 * one color component.
	 */

	pand	%mm5, %mm0
	pand	%mm5, %mm2


	/* Adjust the component values so that they are as small as possible,
	 * but large enough so that we can multiply them by an unsigned 16-bit
	 * number and get a value as large as 0x00ff0000.
 	 */

	pmullw	%mm6, %mm0
	pmullw	%mm6, %mm2
#if SCALE_ADJUST > 0
	psrlw	$SCALE_ADJUST, %mm0
	psrlw	$SCALE_ADJUST, %mm2
#endif

	/* Scale the input component values to be on the range
	 * [0, 0x00ff0000].  This it the real magic of the whole routine.
	 */

	pmulhuw	%mm7, %mm0
	pmulhuw	%mm7, %mm2


	/* Always set the alpha value to 0xff.
	 */

 	por %mm3, %mm0
 	por %mm3, %mm2


	/* Pack the 16-bit values to 8-bit values and store the converted
	 * pixel data.
	 */

	packuswb	%mm2, %mm0
	movq	%mm0, (%edx)
	addl	$8, %edx

	pshufw	$0xaa, %mm4, %mm0
	pshufw	$0xff, %mm4, %mm2

	pand	%mm5, %mm0
	pand	%mm5, %mm2
	pmullw	%mm6, %mm0
	pmullw	%mm6, %mm2
#if SCALE_ADJUST > 0
	psrlw	$SCALE_ADJUST, %mm0
	psrlw	$SCALE_ADJUST, %mm2
#endif
	pmulhuw	%mm7, %mm0
	pmulhuw	%mm7, %mm2

 	por %mm3, %mm0
 	por %mm3, %mm2

	packuswb	%mm2, %mm0

	movq	%mm0, (%edx)
	addl	$8, %edx

	subl	$1, %ecx
.L02:
	jne	.L03


	/* At this point there can be at most 3 pixels left to process.  If
	 * there is either 2 or 3 left, process 2.
         */

	movl	12(%esp), %ecx
	testl	$0x02, %ecx
	je	.L04

	movd	(%eax), %mm4
	addl	$4, %eax

	pshufw	$0x00, %mm4, %mm0
	pshufw	$0x55, %mm4, %mm2

	pand	%mm5, %mm0
	pand	%mm5, %mm2
	pmullw	%mm6, %mm0
	pmullw	%mm6, %mm2
#if SCALE_ADJUST > 0
	psrlw	$SCALE_ADJUST, %mm0
	psrlw	$SCALE_ADJUST, %mm2
#endif
	pmulhuw	%mm7, %mm0
	pmulhuw	%mm7, %mm2

 	por %mm3, %mm0
 	por %mm3, %mm2

	packuswb	%mm2, %mm0

	movq	%mm0, (%edx)
	addl	$8, %edx

.L04:
	/* At this point there can be at most 1 pixel left to process.
	 * Process it if needed.
         */

	testl	$0x01, %ecx
	je	.L01

	movzwl	(%eax), %ecx
	movd	%ecx, %mm4

	pshufw	$0x00, %mm4, %mm0

	pand	%mm5, %mm0
	pmullw	%mm6, %mm0
#if SCALE_ADJUST > 0
	psrlw	$SCALE_ADJUST, %mm0
#endif
	pmulhuw	%mm7, %mm0

 	por %mm3, %mm0

	packuswb	%mm0, %mm0

	movd	%mm0, (%edx)

.L01:
#ifdef USE_INNER_EMMS
	emms
#endif
	ret
#endif /* !defined(__DJGPP__) && !defined(__MINGW32__) && !defined(__APPLE__) */
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
