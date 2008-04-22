/*
 * Mesa 3-D graphics library
 * Version:  7.0.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef USE_X86_64_ASM

#include "matypes.h"

.text

.align 16

.globl _mesa_x86_64_transform_points4_general
_mesa_x86_64_transform_points4_general:
/*
 *	rdi = dest
 *	rsi = matrix
 *	rdx = source
 */
	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	.byte 0x66, 0x66, 0x66, 0x90		/* manual align += 3 */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	testl %ecx, %ecx		/* verify non-zero count */
	prefetchnta 64(%rsi)
	jz p4_general_done

	movq V4F_START(%rdx), %rdx	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */

	prefetch 16(%rdx)

	movaps 0(%rsi), %xmm4		/* m3  | m2  | m1  | m0  */
	movaps 16(%rsi), %xmm5		/* m7  | m6  | m5  | m4  */
	.byte 0x66, 0x66, 0x90		/* manual align += 3 */
	movaps 32(%rsi), %xmm6		/* m11 | m10 | m9  | m8  */
        movaps 48(%rsi), %xmm7		/* m15 | m14 | m13 | m12 */

p4_general_loop:

	movups (%rdx), %xmm8		/* ox | oy | oz | ow */
	prefetchw 16(%rdi)

	pshufd $0x00, %xmm8, %xmm0	/* ox | ox | ox | ox */
	addq %rax, %rdx
	pshufd $0x55, %xmm8, %xmm1	/* oy | oy | oy | oy */
	mulps %xmm4, %xmm0		/* ox*m3 | ox*m2 | ox*m1 | ox*m0 */
	pshufd $0xAA, %xmm8, %xmm2	/* oz | oz | oz | ox */
	mulps %xmm5, %xmm1		/* oy*m7 | oy*m6 | oy*m5 | oy*m4 */
	pshufd $0xFF, %xmm8, %xmm3	/* ow | ow | ow | ow */
	mulps %xmm6, %xmm2		/* oz*m11 | oz*m10 | oz*m9 | oz*m8 */
	addps %xmm1, %xmm0		/* ox*m3+oy*m7 | ... */
	mulps %xmm7, %xmm3		/* ow*m15 | ow*m14 | ow*m13 | ow*m12 */
	addps %xmm2, %xmm0		/* ox*m3+oy*m7+oz*m11 | ... */
	prefetch 16(%rdx)
	addps %xmm3, %xmm0		/* ox*m3+oy*m7+oz*m11+ow*m15 | ... */

	movaps %xmm0, (%rdi)		/* ->D(3) | ->D(2) | ->D(1) | ->D(0) */
	addq $16, %rdi

	decl %ecx
	jnz p4_general_loop

p4_general_done:
	.byte 0xf3
	ret
	
.section .rodata

.align 16
p4_constants:
.byte  0xff, 0xff, 0xff, 0xff
.byte  0xff, 0xff, 0xff, 0xff
.byte  0xff, 0xff, 0xff, 0xff
.byte  0x00, 0x00, 0x00, 0x00

.byte  0x00, 0x00, 0x00, 0x00
.byte  0x00, 0x00, 0x00, 0x00
.byte  0x00, 0x00, 0x00, 0x00
.float 0f+1.0

.text
.align 16
.globl _mesa_x86_64_transform_points4_3d
/*
 * this is slower than _mesa_x86_64_transform_points4_general
 * because it ensures that the last matrix row (or is it column?) is 0,0,0,1
 */
_mesa_x86_64_transform_points4_3d:

	leaq p4_constants(%rip), %rax

	prefetchnta 64(%rsi)
	
	movaps (%rax), %xmm9
	movaps 16(%rax), %xmm10

	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	testl %ecx, %ecx		/* verify non-zero count */
	jz p4_3d_done

	movq V4F_START(%rdx), %rdx	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */

	prefetch 16(%rdx)

	movaps 0(%rsi), %xmm4		/* m3  | m2  | m1  | m0  */
	movaps 16(%rsi), %xmm5		/* m7  | m6  | m5  | m4  */
	andps  %xmm9, %xmm4             /* 0.0 | m2  | m1  | m0  */
	movaps 32(%rsi), %xmm6		/* m11 | m10 | m9  | m8  */
	andps  %xmm9, %xmm5		/* 0.0 | m6  | m5  | m4  */
        movaps 48(%rsi), %xmm7		/* m15 | m14 | m13 | m12 */
	andps  %xmm9, %xmm6		/* 0.0 | m10 | m9  | m8  */
	andps  %xmm9, %xmm7		/* 0.0 | m14 | m13 | m12  */
	.byte 0x66, 0x66, 0x90		/* manual align += 3 */
	orps   %xmm10, %xmm7		/* 1.0 | m14 | m13 | m12  */

p4_3d_loop:

	movups (%rdx), %xmm8		/* ox | oy | oz | ow */
	prefetchw 16(%rdi)

	pshufd $0x00, %xmm8, %xmm0	/* ox | ox | ox | ox */
	addq %rax, %rdx
	pshufd $0x55, %xmm8, %xmm1	/* oy | oy | oy | oy */
	mulps %xmm4, %xmm0		/* ox*m3 | ox*m2 | ox*m1 | ox*m0 */
	pshufd $0xAA, %xmm8, %xmm2	/* oz | oz | oz | ox */
	mulps %xmm5, %xmm1		/* oy*m7 | oy*m6 | oy*m5 | oy*m4 */
	pshufd $0xFF, %xmm8, %xmm3	/* ow | ow | ow | ow */
	mulps %xmm6, %xmm2		/* oz*m11 | oz*m10 | oz*m9 | oz*m8 */
	addps %xmm1, %xmm0		/* ox*m3+oy*m7 | ... */
	mulps %xmm7, %xmm3		/* ow*m15 | ow*m14 | ow*m13 | ow*m12 */
	addps %xmm2, %xmm0		/* ox*m3+oy*m7+oz*m11 | ... */
	prefetch 16(%rdx)
	addps %xmm3, %xmm0		/* ox*m3+oy*m7+oz*m11+ow*m15 | ... */

	movaps %xmm0, (%rdi)		/* ->D(3) | ->D(2) | ->D(1) | ->D(0) */
	addq $16, %rdi

	dec %ecx
	jnz p4_3d_loop

p4_3d_done:
	.byte 0xf3
	ret


.align 16
.globl _mesa_x86_64_transform_points4_identity
_mesa_x86_64_transform_points4_identity:

	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	test %ecx, %ecx
	jz p4_identity_done

	movq V4F_START(%rdx), %rsi	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */
	prefetch 64(%rsi)
	prefetchw 64(%rdi)

	add %ecx, %ecx

	rep movsq

p4_identity_done:
	.byte 0xf3
	ret

	
.align 16
.globl _mesa_x86_64_transform_points4_3d_no_rot
_mesa_x86_64_transform_points4_3d_no_rot:

	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	.byte 0x66, 0x66, 0x90	        /* manual align += 3 */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	test %ecx, %ecx
	.byte 0x66, 0x66, 0x90		/* manual align += 3 */
	jz p4_3d_no_rot_done

	movq V4F_START(%rdx), %rdx	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */

	prefetch (%rdx)
	
	movd (%rsi), %mm0		/*                 | m00             */
	.byte 0x66, 0x66, 0x90	        /* manual align += 3 */
	punpckldq 20(%rsi), %mm0	/* m11             | m00             */

	movd 40(%rsi), %mm2		/*                 | m22             */
	movq 48(%rsi), %mm1		/* m31             | m30             */

	punpckldq 56(%rsi), %mm2	/* m11             | m00             */

p4_3d_no_rot_loop:

	prefetchw 32(%rdi)
	
	movq  (%rdx), %mm4		/* x1              | x0              */
	movq  8(%rdx), %mm5		/* x3              | x2              */
	movd  12(%rdx), %mm7		/*                 | x3              */

	movq  %mm5, %mm6		/* x3              | x2              */
	pfmul %mm0, %mm4		/* x1*m11          | x0*m00          */

	punpckhdq %mm6, %mm6		/* x3              | x3              */
	pfmul %mm2, %mm5		/* x3*m32          | x2*m22          */

	pfmul %mm1, %mm6		/* x3*m31          | x3*m30          */
	pfacc %mm7, %mm5		/* x3              | x2*m22+x3*m32   */

        pfadd %mm6, %mm4		/* x1*m11+x3*m31   | x0*m00+x3*m30   */

	addq %rax, %rdx
	movq %mm4, (%rdi)		/* write r0, r1                      */
	movq %mm5, 8(%rdi)		/* write r2, r3                      */

	addq $16, %rdi
	
	decl %ecx
	prefetch 32(%rdx)
	jnz p4_3d_no_rot_loop

p4_3d_no_rot_done:
	femms
	ret

	
.align 16
.globl _mesa_x86_64_transform_points4_perspective
_mesa_x86_64_transform_points4_perspective:

	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	test %ecx, %ecx
	.byte 0x66, 0x66, 0x90		/* manual align += 3 */
	jz p4_perspective_done

	movq V4F_START(%rdx), %rdx	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */

	movd (%rsi), %mm0		/*                 | m00             */
        pxor %mm7, %mm7			/* 0               | 0               */
	punpckldq 20(%rsi), %mm0	/* m11             | m00             */
	
	movq 32(%rsi), %mm2		/* m21             | m20             */
	prefetch (%rdx)
	
	movd 40(%rsi), %mm1		/*                 | m22             */

	.byte 0x66, 0x66, 0x90	        /* manual align += 3 */
	punpckldq 56(%rsi), %mm1	/* m32             | m22             */


p4_perspective_loop:

	prefetchw 32(%rdi)		/* prefetch 2 vertices ahead         */

	movq (%rdx), %mm4		/* x1              | x0              */
	movq 8(%rdx), %mm5		/* x3              | x2              */
	movd 8(%rdx), %mm3		/*                 | x2              */

	movq %mm5, %mm6			/* x3              | x2              */
	pfmul %mm0, %mm4		/* x1*m11          | x0*m00          */

	punpckldq %mm5, %mm5		/* x2              | x2              */

	pfmul %mm2, %mm5		/* x2*m21          | x2*m20          */
	pfsubr %mm7, %mm3		/*                 | -x2             */

	pfmul %mm1, %mm6		/* x3*m32          | x2*m22          */
	pfadd %mm4, %mm5		/* x1*m11+x2*m21   | x0*m00+x2*m20   */

	pfacc %mm3, %mm6		/* -x2             | x2*m22+x3*m32   */

	movq %mm5, (%rdi)		/* write r0, r1                      */
	addq %rax, %rdx	
	movq %mm6, 8(%rdi)		/* write r2, r3                      */

	addq $16, %rdi

	decl %ecx
	prefetch 32(%rdx)		/* hopefully stride is zero          */
	jnz p4_perspective_loop

p4_perspective_done:
	femms
	ret

.align 16
.globl _mesa_x86_64_transform_points4_2d_no_rot
_mesa_x86_64_transform_points4_2d_no_rot:

	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	test %ecx, %ecx
	.byte 0x90			/* manual align += 1 */
	jz p4_2d_no_rot_done

	movq V4F_START(%rdx), %rdx	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */

	movd (%rsi), %mm0		/*                 | m00             */
	prefetch (%rdx)
	punpckldq 20(%rsi), %mm0	/* m11             | m00             */
	
	movq 48(%rsi), %mm1		/* m31             | m30             */

p4_2d_no_rot_loop:

	prefetchw 32(%rdi)		/* prefetch 2 vertices ahead         */

	movq (%rdx), %mm4		/* x1              | x0              */
	movq 8(%rdx), %mm5		/* x3              | x2              */

	pfmul %mm0, %mm4		/* x1*m11          | x0*m00          */
	movq %mm5, %mm6			/* x3              | x2              */

	punpckhdq %mm6, %mm6		/* x3              | x3              */

	addq %rax, %rdx	
	pfmul %mm1, %mm6		/* x3*m31          | x3*m30          */

	prefetch 32(%rdx)		/* hopefully stride is zero          */
	pfadd %mm4, %mm6		/* x1*m11+x3*m31   | x0*m00+x3*m30   */

	movq %mm6, (%rdi)		/* write r0, r1                      */
	movq %mm5, 8(%rdi)		/* write r2, r3                      */

	addq $16, %rdi

	decl %ecx
	jnz p4_2d_no_rot_loop

p4_2d_no_rot_done:
	femms
	ret

	
.align 16
.globl _mesa_x86_64_transform_points4_2d
_mesa_x86_64_transform_points4_2d:

	movl V4F_COUNT(%rdx), %ecx	/* count */
	movzx V4F_STRIDE(%rdx), %eax	/* stride */

	movl %ecx, V4F_COUNT(%rdi)	/* set dest count */
	movl $4, V4F_SIZE(%rdi)		/* set dest size */
	.byte 0x66, 0x66, 0x90		/* manual align += 4 */
	orl $VEC_SIZE_4, V4F_FLAGS(%rdi)/* set dest flags */
	
	test %ecx, %ecx
	.byte 0x66, 0x66, 0x90		/* manual align += 4 */
	jz p4_2d_done

	movq V4F_START(%rdx), %rdx	/* ptr to first src vertex */
	movq V4F_START(%rdi), %rdi	/* ptr to first dest vertex */

	movd (%rsi), %mm0		/*                 | m00             */
	movd 4(%rsi), %mm1		/*                 | m01             */

	prefetch (%rdx)

	punpckldq 16(%rsi), %mm0	/* m10             | m00             */
	.byte 0x66, 0x66, 0x90		/* manual align += 4 */
	punpckldq 20(%rsi), %mm1	/* m11             | m01             */

	movq 48(%rsi), %mm2		/* m31             | m30             */

p4_2d_loop:

	prefetchw 32(%rdi)		/* prefetch 2 vertices ahead         */

	movq (%rdx), %mm3		/* x1              | x0              */
	movq 8(%rdx), %mm5		/* x3              | x2              */

	movq %mm3, %mm4			/* x1              | x0              */
	movq %mm5, %mm6			/* x3              | x2              */

	pfmul %mm1, %mm4		/* x1*m11          | x0*m01          */
	punpckhdq %mm6, %mm6		/* x3              | x3              */

	pfmul %mm0, %mm3		/* x1*m10          | x0*m00          */

	addq %rax, %rdx	
	pfacc %mm4, %mm3		/* x0*m01+x1*m11   | x0*m00+x1*m10   */

	pfmul %mm2, %mm6		/* x3*m31          | x3*m30          */
	prefetch 32(%rdx)		/* hopefully stride is zero          */

	pfadd %mm6, %mm3		/* r1              | r0              */

	movq %mm3, (%rdi)		/* write r0, r1                      */
	movq %mm5, 8(%rdi)		/* write r2, r3                      */

	addq $16, %rdi

	decl %ecx
	jnz p4_2d_loop

p4_2d_done:
	femms
	ret
			
#endif
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
