/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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

/*
 * DOS/DJGPP device driver v1.3 for Mesa
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


		.file	"virtual.S"

/*
 * extern void *vl_current_draw_buffer;
 * extern int vl_current_width, vl_current_bytes;
 */

		.text

/* Desc: void v_clear8 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear8
_v_clear8:
		movl	4(%esp), %eax
		movb	%al, %ah
		pushw	%ax
		pushw	%ax
		popl	%eax
		jmp	_v_clear_common

/* Desc: void v_clear16 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear16
_v_clear16:
		movl	4(%esp), %eax
		pushw	%ax
		pushw	%ax
		popl	%eax
		jmp	_v_clear_common

/* Desc: void v_clear32 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear32
_v_clear32:
		movl	4(%esp), %eax
		.balign	4
_v_clear_common:
		movl	_vl_current_bytes, %ecx
		movl	_vl_current_draw_buffer, %edx
		shrl	$2, %ecx
		.balign	4
	0:
		movl	%eax, (%edx)
		addl	$4, %edx
		decl	%ecx
		jnz	0b
		ret

/* Desc: void v_clear8_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear8_mmx
_v_clear8_mmx:
#ifdef USE_MMX_ASM
		movd	4(%esp), %mm0
		punpcklbw %mm0, %mm0
		punpcklwd %mm0, %mm0
		jmp	_v_clear_common_mmx
#endif

/* Desc: void v_clear16_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear16_mmx
_v_clear16_mmx:
#ifdef USE_MMX_ASM
		movd	4(%esp), %mm0
		punpcklwd %mm0, %mm0
		jmp	_v_clear_common_mmx
#endif

/* Desc: void v_clear32_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear32_mmx
_v_clear32_mmx:
#ifdef USE_MMX_ASM
		movd	4(%esp), %mm0
		.balign	4
_v_clear_common_mmx:
		punpckldq %mm0, %mm0
		movl	_vl_current_bytes, %ecx
		movl	_vl_current_draw_buffer, %edx
		shrl	$3, %ecx
		.balign	4
	0:
		movq	%mm0, (%edx)
		addl	$8, %edx
		decl	%ecx
		jnz	0b
		emms
#endif
		ret

/* Desc: void v_clear24 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear24
_v_clear24:
		movl	$0xaaaaaaab, %eax
		mull	_vl_current_bytes
		movl	4(%esp), %eax
		movl	%edx, %ecx
		pushl	%ebx
		movl	_vl_current_draw_buffer, %edx
		shrl	%ecx
		movb	10(%esp), %bl
		.balign	4
	0:
		movw	%ax, (%edx)
		movb	%bl, 2(%edx)
		addl	$3, %edx
		decl	%ecx
		jnz	0b
		popl	%ebx
		ret

/* Desc: void v_clear24_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_clear24_mmx
_v_clear24_mmx:
#ifdef USE_MMX_ASM
		movl	4(%esp), %eax
		movl	%eax, %edx
		movl	%eax, %ecx
		shll	$16, %edx
		rorl	$8, %ecx
		movw	%cx, %dx
		rorl	$16, %ecx
		movb	%dh, %cl
		shll	$8, %eax
		movb	%ch, %al
		rorl	$8, %eax

		pushl	%edx
		pushl	%eax
		movq	(%esp), %mm0
		pushl	%ecx
		movq	(%esp), %mm1
		pushl	%edx
		movq	(%esp), %mm2

		movl	$0xaaaaaaab, %eax
		mull	_vl_current_bytes
		movl	%edx, %ecx
		movl	_vl_current_draw_buffer, %edx
		shrl	$4, %ecx
		.balign	4
	0:
		movq	%mm0, (%edx)
		movq	%mm1, 8(%edx)
		movq	%mm2, 16(%edx)
		addl	$24, %edx
		decl	%ecx
		jnz	0b
		emms
		addl	$16, %esp
#endif
		ret

/* Desc: void v_rect8 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_rect8
_v_rect8:
		cld
		pushl	%esi
		pushl	%edi
		movl	28(%esp), %eax
		movl	_vl_current_width, %esi
		movl	16(%esp), %edi
		movb	%al, %ah
		movl	20(%esp), %ecx
		imull	%esi, %edi
		movl	24(%esp), %edx
		subl	%ecx, %esi
		addl	12(%esp), %edi
		pushw	%ax
		pushw	%ax
		pushl	%ds
		popl	%es
		addl	_vl_current_draw_buffer, %edi
		popl	%eax
		pushl	%ebx
		movl	%ecx, %ebx
		andl	$3, %ebx
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		shrl	$2, %ecx
		rep;	stosl
		testl	%ebx, %ebx
		jz	2f
		movl	%ebx, %ecx
		rep;	stosb
		.balign	4
	2:
		popl	%ecx
		addl	%esi, %edi
		decl	%edx
		jnz	0b
		popl	%ebx
		popl	%edi
		popl	%esi
		ret

/* Desc: void v_rect16 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_rect16
_v_rect16:
		cld
		pushl	%esi
		pushl	%edi
		movl	28(%esp), %eax
		movl	_vl_current_width, %esi
		movl	16(%esp), %edi
		movl	20(%esp), %ecx
		imull	%esi, %edi
		movl	24(%esp), %edx
		subl	%ecx, %esi
		addl	12(%esp), %edi
		pushw	%ax
		shll	%esi
		pushw	%ax
		shll	%edi
		pushl	%ds
		popl	%es
		addl	_vl_current_draw_buffer, %edi
		popl	%eax
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		shrl	%ecx
		rep;	stosl
		jnc	2f
		stosw
		.balign	4
	2:
		popl	%ecx
		addl	%esi, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		ret

/* Desc: void v_rect24 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_rect24
_v_rect24:
		pushl	%esi
		pushl	%edi
		movl	28(%esp), %eax
		movl	_vl_current_width, %esi
		movl	16(%esp), %edi
		movl	20(%esp), %ecx
		imull	%esi, %edi
		movl	24(%esp), %edx
		subl	%ecx, %esi
		addl	12(%esp), %edi
		leal	(%esi, %esi, 2), %esi
		pushl	%ebx
		leal	(%edi, %edi, 2), %edi
		movl	%eax, %ebx
		addl	_vl_current_draw_buffer, %edi
		shrl	$16, %ebx
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		movw	%ax, (%edi)
		movb	%bl, 2(%edi)
		addl	$3, %edi
		decl	%ecx
		jnz	1b
		popl	%ecx
		addl	%esi, %edi
		decl	%edx
		jnz	0b
		popl	%ebx
		popl	%edi
		popl	%esi
		ret

/* Desc: void v_rect32 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_rect32
_v_rect32:
		pushl	%esi
		pushl	%edi
		movl	_vl_current_width, %esi
		movl	16(%esp), %edi
		movl	20(%esp), %ecx
		imull	%esi, %edi
		movl	24(%esp), %edx
		subl	%ecx, %esi
		addl	12(%esp), %edi
		shll	$2, %esi
		shll	$2, %edi
		movl	28(%esp), %eax
		addl	_vl_current_draw_buffer, %edi
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		movl	%eax, (%edi)
		addl	$4, %edi
		decl	%ecx
		jnz	1b
		popl	%ecx
		addl	%esi, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		ret

/* Desc: void v_putpixel8 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_putpixel8
_v_putpixel8:
		movl	8(%esp), %ecx
		movl	4(%esp), %edx
		movl	_vl_current_draw_buffer, %eax
		movb	%cl, (%eax,%edx)
		ret

/* Desc: void v_putpixel16 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_putpixel16
_v_putpixel16:
		movl	8(%esp), %ecx
		movl	4(%esp), %edx
		movl	_vl_current_draw_buffer, %eax
		movw	%cx, (%eax,%edx,2)
		ret

/* Desc: void v_putpixel24 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_putpixel24
_v_putpixel24:
		movl	4(%esp), %eax
		movl	8(%esp), %edx
		movl	_vl_current_draw_buffer, %ecx
		leal	(%eax,%eax,2), %eax
		movw	%dx, (%ecx,%eax)
		shrl	$16, %edx
		movb	%dl, 2(%ecx,%eax)
		ret

/* Desc: void v_putpixel32 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_v_putpixel32
_v_putpixel32:
		movl	8(%esp), %ecx
		movl	4(%esp), %edx
		movl	_vl_current_draw_buffer, %eax
		movl	%ecx, (%eax,%edx,4)
		ret
