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
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
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



#define CVT_32_TO_16(s, tmp)			\
    /* SRC = bbbbbbbbggggggggrrrrrrrr******** */\
		movl	%e##s##x, %tmp		;\
    /* TMP = bbbbbbbbggggggggrrrrrrrr******** */\
		shrb	$2, %s##h		;\
    /* SRC = bbbbbbbbgggggg00rrrrrrrr******** */\
		andl	$0xF80000, %tmp		;\
    /* TMP = 0000000000000000000rrrrr00000000 */\
		shrw	$3, %s##x		;\
    /* SRC = bbbbbgggggg00000rrrrrrrr******** */\
		shrl	$8, %tmp		;\
    /* TMP = 00000000000rrrrr0000000000000000 */\
		orl	%tmp, %e##s##x		;\
    /* SRC = bbbbbggggggrrrrrrrrrrrrr******** */

#define CVT_32_TO_15(s, tmp)			\
    /* SRC = bbbbbbbbggggggggrrrrrrrr******** */\
		movl	%e##s##x, %tmp		;\
    /* TMP = bbbbbbbbggggggggrrrrrrrr******** */\
		shrb	$3, %s##h		;\
    /* SRC = bbbbbbbbgggggg00rrrrrrrr******** */\
		andl	$0xF80000, %tmp		;\
    /* TMP = 0000000000000000000rrrrr00000000 */\
		shrw	$3, %s##x		;\
    /* SRC = bbbbbgggggg00000rrrrrrrr******** */\
		shrl	$9, %tmp		;\
    /* TMP = 00000000000rrrrr0000000000000000 */\
		orl	%tmp, %e##s##x		;\
    /* SRC = bbbbbggggggrrrrrrrrrrrrr******** */

#define CVT_16_TO_15(src, tmp)			\
    /* SRC = bbbbbggggggrrrrrBBBBBGGGGGGRRRRR */\
		movl	%src, %tmp		;\
    /* TMP = bbbbbggggggrrrrrBBBBBGGGGGGRRRRR */\
		andl	$0x1F001F, %src		;\
    /* SRC = bbbbb00000000000BBBBB00000000000 */\
		andl	$0xFFC0FFC0, %tmp	;\
    /* TMP = 000000gggggrrrrr000000GGGGGRRRRR */\
		shrl	%tmp			;\
    /* TMP = 00000gggggrrrrr000000GGGGGRRRRR0 */\
		orl	%tmp, %src		;\
    /* SRC = bbbbbgggggrrrrr0BBBBBGGGGGRRRRR0 */\



/* transform BGRA to BGR */
		.p2align 5,,31
		.global	_vesa_l_dump_32_to_24
_vesa_l_dump_32_to_24:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%ecx
	1:
		movl	(%esi), %eax
		addl	$4, %esi
		movw	%ax, %fs:(%edi)
		shrl	$16, %eax
		movb	%al, %fs:2(%edi)
		addl	$3, %edi
		subl	$3, %ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

/* transform BGRA to B5G6R5 */
		.p2align 5,,31
		.global	_vesa_l_dump_32_to_16
_vesa_l_dump_32_to_16:
		pushl	%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%ecx
	1:
		movl	(%esi), %eax
		addl	$4, %esi
		CVT_32_TO_16(a, ebp)
		movw	%ax, %fs:(%edi)
		addl	$2, %edi
		subl	$2, %ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret

/* transform BGRA to B5G5R5 */
		.p2align 5,,31
		.global	_vesa_l_dump_32_to_15
_vesa_l_dump_32_to_15:
		pushl	%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%ecx
	1:
		movl	(%esi), %eax
		addl	$4, %esi
		CVT_32_TO_15(a, ebp)
		movw	%ax, %fs:(%edi)
		addl	$2, %edi
		subl	$2, %ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret

/* transform BGRA to fake8 */
		.p2align 5,,31
		.global	_vesa_l_dump_32_to_8
_vesa_l_dump_32_to_8:
		pushl	%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%edx
		pushl	%ecx
		pushl	%ebx
	1:
		movl	(%esi), %eax
		addl	$4, %esi
#if 1
		xorl	%ebx, %ebx
		movl	%eax, %edx
		movb	%ah, %bl
		shrl	$16, %edx
		andl	$0xFF, %edx
		andl	$0xFF, %eax

		movb	_array_b(%eax), %al
		movb	_array_r(%edx), %dl
		movb	_array_g(%ebx), %bl

		imull	$36, %eax
		imull	$6, %ebx
		addl	%edx, %eax
		addl	%ebx, %eax
#endif
		movb	%al, %fs:(%edi)
		incl	%edi
		decl	%ecx
		jnz	1b
		popl	%ebx
		popl	%ecx
		popl	%edx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret

/* transform BGR to BGRx */
		.p2align 5,,31
		.global	_vesa_l_dump_24_to_32
_vesa_l_dump_24_to_32:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%ecx
	1:
		movl	(%esi), %eax
		addl	$3, %esi
		movl	%eax, %fs:(%edi)
		addl	$4, %edi
		subl	$4, %ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

/* transform BGR to fake8 */
		.p2align 5,,31
		.global	_vesa_l_dump_24_to_8
_vesa_l_dump_24_to_8:
		pushl	%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%edx
		pushl	%ecx
		pushl	%ebx
	1:
		movl	(%esi), %eax
		addl	$3, %esi
#if 1
		xorl	%ebx, %ebx
		movl	%eax, %edx
		movb	%ah, %bl
		shrl	$16, %edx
		andl	$0xFF, %edx
		andl	$0xFF, %eax

		movb	_array_b(%eax), %al
		movb	_array_r(%edx), %dl
		movb	_array_g(%ebx), %bl

		imull	$36, %eax
		imull	$6, %ebx
		addl	%edx, %eax
		addl	%ebx, %eax
#endif
		movb	%al, %fs:(%edi)
		incl	%edi
		decl	%ecx
		jnz	1b
		popl	%ebx
		popl	%ecx
		popl	%edx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret

/* transform B5G6R5 to B5G5R5 */
		.p2align 5,,31
		.global	_vesa_l_dump_16_to_15
_vesa_l_dump_16_to_15:
		pushl	%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%ecx
	1:
		movl	(%esi), %eax
		addl	$4, %esi
		CVT_16_TO_15(eax, ebp)
		movl	%eax, %fs:(%edi)
		addl	$4, %edi
		subl	$4, %ecx
		jnz	1b
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret

/* transform B5G6R5 to fake8 */
		.p2align 5,,31
		.global	_vesa_l_dump_16_to_8
_vesa_l_dump_16_to_8:
		pushl	%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_vl_video_selector, %fs
		movl	_vl_current_draw_buffer, %esi
		movl	_vl_current_offset, %edi
		movl	_vl_current_stride, %ecx
		movl	_vl_current_height, %edx
		movl	_vl_current_delta, %ebx
		.balign	4
	0:
		pushl	%ecx
		pushl	%ebx
	1:
		movl	(%esi), %eax
		addl	$4, %esi
#if 1
		movl	%eax, %ebx
		andl	$0xFFFF, %eax
		shrl	$16, %ebx
		movb	_tab_16_8(%eax), %al
		movb	_tab_16_8(%ebx), %ah
#endif
		movw	%ax, %fs:(%edi)
		addl	$2, %edi
		subl	$2, %ecx
		jnz	1b
		popl	%ebx
		popl	%ecx
		addl	%ebx, %edi
		decl	%edx
		jnz	0b
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret



		.p2align 5,,31
		.global	_vesa_b_dump_32_to_24
_vesa_b_dump_32_to_24:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		movl	$0x00FFFFFF, %ebx
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		pushl	%ebx
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		popl	%ebx
		subl	%ebp, %edi
		.balign	4
	2:
		movb	(%esi), %al	/* XXX too many accesses */
		incl	%esi
		rorl	$8, %ebx
		jnc	2b
		movb	%al, %fs:(%edi)
		incl	%edi
		decl	%ecx
		jnz	1b
		popl	%ecx
		popl	%eax
		addl	_vl_current_delta, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_32_to_16
_vesa_b_dump_32_to_16:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		subl	%ebp, %edi
		.balign	4
	2:
		movl	(%esi), %eax
		addl	$4, %esi
		CVT_32_TO_16(a, ebx)
		movw	%ax, %fs:(%edi)
		addl	$2, %edi
		subl	$2, %ecx
		jnz	1b
		popl	%ecx
		popl	%eax
		addl	_vl_current_delta, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_32_to_15
_vesa_b_dump_32_to_15:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		subl	%ebp, %edi
		.balign	4
	2:
		movl	(%esi), %eax
		addl	$4, %esi
		CVT_32_TO_15(a, ebx)
		movw	%ax, %fs:(%edi)
		addl	$2, %edi
		subl	$2, %ecx
		jnz	1b
		popl	%ecx
		popl	%eax
		addl	_vl_current_delta, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_32_to_8
_vesa_b_dump_32_to_8:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		pushl	%edx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		popl	%edx
		incl	%edx
		pushl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		subl	%ebp, %edi
		.balign	4
	2:
		movl	(%esi), %eax
		addl	$4, %esi
#if 1
		xorl	%ebx, %ebx
		movl	%eax, %edx
		movb	%ah, %bl
		shrl	$16, %edx
		andl	$0xFF, %edx
		andl	$0xFF, %eax

		movb	_array_b(%eax), %al
		movb	_array_r(%edx), %dl
		movb	_array_g(%ebx), %bl

		imull	$36, %eax
		imull	$6, %ebx
		addl	%edx, %eax
		addl	%ebx, %eax
#endif
		movb	%al, %fs:(%edi)
		incl	%edi
		decl	%ecx
		jnz	1b
		popl	%edx
		popl	%ecx
		popl	%eax
		addl	_vl_current_delta, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_24_to_32
_vesa_b_dump_24_to_32:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		pushl	%ebx
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		popl	%ebx
		subl	%ebp, %edi
		.balign	4
	2:
		movl	(%esi), %eax
		addl	$3, %esi
		movl	%eax, %fs:(%edi)
		addl	$4, %edi
		subl	$4, %ecx
		jnz	1b
		popl	%ecx
		popl	%eax
		addl	%ebx, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_24_to_8
_vesa_b_dump_24_to_8:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		pushl	%edx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		popl	%edx
		incl	%edx
		pushl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		subl	%ebp, %edi
		.balign	4
	2:
		movl	(%esi), %eax
		addl	$3, %esi
#if 1
		xorl	%ebx, %ebx
		movl	%eax, %edx
		movb	%ah, %bl
		shrl	$16, %edx
		andl	$0xFF, %edx
		andl	$0xFF, %eax

		movb	_array_b(%eax), %al
		movb	_array_r(%edx), %dl
		movb	_array_g(%ebx), %bl

		imull	$36, %eax
		imull	$6, %ebx
		addl	%edx, %eax
		addl	%ebx, %eax
#endif
		movb	%al, %fs:(%edi)
		incl	%edi
		decl	%ecx
		jnz	1b
		popl	%edx
		popl	%ecx
		popl	%eax
		addl	_vl_current_delta, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_16_to_15
_vesa_b_dump_16_to_15:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		subl	%ebp, %edi
		.balign	4
	2:
		movw	(%esi), %ax
		addl	$2, %esi
		CVT_16_TO_15(eax, ebx)
		movw	%ax, %fs:(%edi)
		addl	$2, %edi
		subl	$2, %ecx
		jnz	1b
		popl	%ecx
		popl	%eax
		addl	_vl_current_delta, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.p2align 5,,31
		.global	_vesa_b_dump_16_to_8
_vesa_b_dump_16_to_8:
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		pushl	%ebp
		movl	_vl_video_selector, %fs
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
		.balign	4
	0:
		pushl	%eax
		pushl	%ecx
		.balign	4
	1:
		cmpl	%ebp, %edi
		jb	2f
		pushl	%ebx
		incl	%edx
		xorl	%ebx, %ebx
		call	*_vesa_swbank
		popl	%ebx
		subl	%ebp, %edi
		.balign	4
	2:
		movw	(%esi), %ax
		addl	$2, %esi
#if 1
		andl	$0xFFFF, %eax
		movb	_tab_16_8(%eax), %al
#endif
		movb	%al, %fs:(%edi)
		addl	$1, %edi
		subl	$1, %ecx
		jnz	1b
		popl	%ecx
		popl	%eax
		addl	%ebx, %edi
		decl	%eax
		jnz	0b
		popl	%ebp
		popl	%edi
		popl	%esi
		popl	%ebx
		ret
