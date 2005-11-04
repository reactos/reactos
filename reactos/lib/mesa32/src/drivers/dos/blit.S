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
 * DOS/DJGPP device driver v1.4 for Mesa
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "x86/assyntax.h"


		FILE("blit.S")

/*
 * extern unsigned int vesa_gran_mask, vesa_gran_shift;
 * extern int vl_video_selector;

 * extern void *vl_current_draw_buffer;
 * extern int vl_current_stride, vl_current_height;
 * extern int vl_current_offset, vl_current_delta;
 */
EXTERN _vesa_gran_mask, _vesa_gran_shift
EXTERN _vl_video_selector

EXTERN _vl_current_draw_buffer
EXTERN _vl_current_stride, _vl_current_height
EXTERN _vl_current_offset, _vl_current_delta

		SEG_TEXT
		USE32

/* Desc: VESA bank switching routine (BIOS)
 *
 * In  : EBX=0, EDX = bank number
 * Out : -
 *
 * Note: thrashes EAX
 */
		ALIGNTEXT32
_vesa_swbankBIOS:
		MOV_W	(CONST(0x4f05), AX)
		INT	(CONST(0x10))
		RET

		ALIGNTEXT8
		GLOBL	_vesa_swbank
_vesa_swbank:	D_LONG	_vesa_swbankBIOS

/* Desc: void vesa_b_dump_virtual (void);
 *
 * In  : -
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_vesa_b_dump_virtual
_vesa_b_dump_virtual:
		CLD
		PUSH_SR	(ES)
		PUSH_L	(EBX)
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		PUSH_L	(EBP)
		MOV_SR	(CONTENT(_vl_video_selector), ES)
		MOV_L	(CONTENT(_vl_current_draw_buffer), ESI)
		MOV_L	(CONTENT(_vl_current_offset), EDI)
		MOV_L	(CONTENT(_vesa_gran_shift), ECX)
		MOV_L	(CONTENT(_vesa_gran_mask), EBP)
		MOV_L	(EDI, EDX)
		XOR_L	(EBX, EBX)
		AND_L	(EBP, EDI)
		SHR_L	(CL, EDX)
		INC_L	(EBP)
		CALL	(VARINDIRECT(_vesa_swbank))
		MOV_L	(CONTENT(_vl_current_stride), ECX)
		MOV_L	(CONTENT(_vl_current_height), EAX)
		MOV_L	(CONTENT(_vl_current_delta), EBX)
		SHR_L	(CONST(2), ECX)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		ALIGNTEXT4
	TLBL(1):
		CMP_L	(EBP, EDI)
		jb	TLBL(2)
		PUSH_L	(EAX)
		PUSH_L	(EBX)
		INC_L	(EDX)
		XOR_L	(EBX, EBX)
		CALL	(VARINDIRECT(_vesa_swbank))
		POP_L	(EBX)
		POP_L	(EAX)
		SUB_L	(EBP, EDI)
		ALIGNTEXT4
	TLBL(2):
		MOVS_L
		DEC_L	(ECX)
		jnz	TLBL(1)
		POP_L	(ECX)
		ADD_L	(EBX, EDI)
		DEC_L	(EAX)
		jnz	TLBL(0)
		POP_L	(EBP)
		POP_L	(EDI)
		POP_L	(ESI)
		POP_L	(EBX)
		POP_SR	(ES)
		RET

/* Desc: void vesa_l_dump_virtual (void);
 *
 * In  : -
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_vesa_l_dump_virtual
_vesa_l_dump_virtual:
		CLD
		PUSH_SR	(ES)
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		MOV_SR	(CONTENT(_vl_video_selector), ES)
		MOV_L	(CONTENT(_vl_current_draw_buffer), ESI)
		MOV_L	(CONTENT(_vl_current_offset), EDI)
		MOV_L	(CONTENT(_vl_current_stride), ECX)
		MOV_L	(CONTENT(_vl_current_height), EDX)
		MOV_L	(CONTENT(_vl_current_delta), EAX)
		SHR_L	(CONST(2), ECX)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		REP	MOVS_L
		POP_L	(ECX)
		ADD_L	(EAX, EDI)
		DEC_L	(EDX)
		jnz	TLBL(0)
		POP_L	(EDI)
		POP_L	(ESI)
		POP_SR	(ES)
		RET

/* Desc: void vesa_l_dump_virtual_mmx (void);
 *
 * In  : -
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_vesa_l_dump_virtual_mmx
_vesa_l_dump_virtual_mmx:
#ifdef USE_MMX_ASM
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		MOV_SR	(CONTENT(_vl_video_selector), FS)
		MOV_L	(CONTENT(_vl_current_draw_buffer), ESI)
		MOV_L	(CONTENT(_vl_current_offset), EDI)
		MOV_L	(CONTENT(_vl_current_stride), ECX)
		MOV_L	(CONTENT(_vl_current_height), EDX)
		MOV_L	(CONTENT(_vl_current_delta), EAX)
		SHR_L	(CONST(3), ECX)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		ALIGNTEXT4
	TLBL(1):
		MOVQ	(REGIND(ESI), MM0)
		ADD_L	(CONST(8), ESI)
		SEGFS
		MOVQ	(MM0, REGIND(EDI))
		ADD_L	(CONST(8), EDI)
		DEC_L	(ECX)
		jnz	TLBL(1)
		POP_L	(ECX)
		ADD_L	(EAX, EDI)
		DEC_L	(EDX)
		jnz	TLBL(0)
		POP_L	(EDI)
		POP_L	(ESI)
		EMMS
#endif
		RET
