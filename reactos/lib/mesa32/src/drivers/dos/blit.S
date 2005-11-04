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


		.file	"blit.S"

/*
 * extern unsigned int vesa_gran_mask, vesa_gran_shift;
 * extern int vl_video_selector;

 * extern void *vl_current_draw_buffer;
 * extern int vl_current_stride, vl_current_height;
 * extern int vl_current_offset, vl_current_delta;
 */

		.text

/* Desc: VESA bank switching routine (BIOS)
 *
 * In  : EBX=0, EDX = bank number
 * Out : -
 *
 * Note: thrashes EAX
 */
		.p2align 5,,31
_vesa_swbankBIOS:
		movw	$0x4f05, %ax
		int	$0x10
		ret

		.p2align 2,,3
		.global	_vesa_swbank
_vesa_swbank:	.long	_vesa_swbankBIOS

/* Desc: void vesa_b_dump_virtual (void);
 *
 * In  : -
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_vesa_b_dump_virtual
_vesa_b_dump_virtual:
		cld
		pushl	%es
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %es
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vesa_gran_shift, %ecx
		movl	_vesa_gran_mask, %ebp
		movl	%edi, %edx
		xorl	%ebx, %ebx
		andl	%ebp, %edi
		shrl	%cl, %edx
		incl	%ebp
		call	*_vesa_swbank
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %eax
		movl	_vl_current_delta, %ebx
		shrl	$2, %ecx
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		pushl	%eax
		pushl	%ebx
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		popl	%ebx
		popl	%eax
		subl	%ebp, %edi
		.balign	4
	2:
		movsl
		decl	%ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%es
		ret

/* Desc: void vesa_l_dump_virtual (void);
 *
 * In  : -
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_vesa_l_dump_virtual
_vesa_l_dump_virtual:
		cld
		pushl	%es
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %es
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %eax
		shrl	$2, %ecx
		.balign	4
	0:
		pushl	%ecx
		rep;	movsl
		popl	%ecx
		addl	%eax, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%es
		ret

/* Desc: void vesa_l_dump_virtual_mmx (void);
 *
 * In  : -
 * Out : -
 *
 * Note: uses current draw buffer
 */
		.p2align 5,,31
		.global	_vesa_l_dump_virtual_mmx
_vesa_l_dump_virtual_mmx:
#ifdef USE_MMX_ASM
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %eax
		shrl	$3, %ecx
		.balign	4
	0:
		pushl	%ecx
		.balign	4
	1:
		movq	(%esi), %mm0
		addl	$8, %esi
		movq	%mm0, %fs:(%edi)
		addl	$8, %edi
		decl	%ecx
		jnz	1b
		popl	%ecx
		addl	%eax, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		emms
#endif
		ret
