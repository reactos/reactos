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


		FILE("virtual.S")

/*
 * extern void *vl_current_draw_buffer;
 * extern int vl_current_width, vl_current_bytes;
 */
EXTERN _vl_current_draw_buffer
EXTERN _vl_current_width, _vl_current_bytes

		SEG_TEXT
		USE32

/* Desc: void v_clear8 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear8
_v_clear8:
		MOV_L	(REGOFF(4, ESP), EAX)
		MOV_B	(AL, AH)
		PUSH_W	(AX)
		PUSH_W	(AX)
		POP_L	(EAX)
		jmp	_v_clear_common

/* Desc: void v_clear16 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear16
_v_clear16:
		MOV_L	(REGOFF(4, ESP), EAX)
		PUSH_W	(AX)
		PUSH_W	(AX)
		POP_L	(EAX)
		jmp	_v_clear_common

/* Desc: void v_clear32 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear32
_v_clear32:
		MOV_L	(REGOFF(4, ESP), EAX)
		ALIGNTEXT4
_v_clear_common:
		MOV_L	(CONTENT(_vl_current_bytes), ECX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EDX)
		SHR_L	(CONST(2), ECX)
		ALIGNTEXT4
	TLBL(0):
		MOV_L	(EAX, REGIND(EDX))
		ADD_L	(CONST(4), EDX)
		DEC_L	(ECX)
		jnz	TLBL(0)
		RET

/* Desc: void v_clear8_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear8_mmx
_v_clear8_mmx:
#ifdef USE_MMX_ASM
		MOVD	(REGOFF(4, ESP), MM0)
		PUNPCKLBW (MM0, MM0)
		PUNPCKLWD (MM0, MM0)
		jmp	_v_clear_common_mmx
#endif

/* Desc: void v_clear16_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear16_mmx
_v_clear16_mmx:
#ifdef USE_MMX_ASM
		MOVD	(REGOFF(4, ESP), MM0)
		PUNPCKLWD (MM0, MM0)
		jmp	_v_clear_common_mmx
#endif

/* Desc: void v_clear32_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear32_mmx
_v_clear32_mmx:
#ifdef USE_MMX_ASM
		MOVD	(REGOFF(4, ESP), MM0)
		ALIGNTEXT4
_v_clear_common_mmx:
		PUNPCKLDQ (MM0, MM0)
		MOV_L	(CONTENT(_vl_current_bytes), ECX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EDX)
		SHR_L	(CONST(3), ECX)
		ALIGNTEXT4
	TLBL(0):
		MOVQ	(MM0, REGIND(EDX))
		ADD_L	(CONST(8), EDX)
		DEC_L	(ECX)
		jnz	TLBL(0)
		EMMS
#endif
		RET

/* Desc: void v_clear24 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear24
_v_clear24:
		MOV_L	(CONST(0xaaaaaaab), EAX)
		MUL_L	(CONTENT(_vl_current_bytes))
		MOV_L	(REGOFF(4, ESP), EAX)
		MOV_L	(EDX, ECX)
		PUSH_L	(EBX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EDX)
		SHR_L	(CONST(1), ECX)
		MOV_B	(REGOFF(10,ESP), BL)
		ALIGNTEXT4
	TLBL(0):
		MOV_W	(AX, REGIND(EDX))
		MOV_B	(BL, REGOFF(2,EDX))
		ADD_L	(CONST(3), EDX)
		DEC_L	(ECX)
		jnz	TLBL(0)
		POP_L	(EBX)
		RET

/* Desc: void v_clear24_mmx (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_clear24_mmx
_v_clear24_mmx:
#ifdef USE_MMX_ASM
		MOV_L	(REGOFF(4, ESP), EAX)
		MOV_L	(EAX, EDX)
		MOV_L	(EAX, ECX)
		SHL_L	(CONST(16), EDX)
		ROR_L	(CONST(8), ECX)
		MOV_W	(CX, DX)
		ROR_L	(CONST(16), ECX)
		MOV_B	(DH, CL)
		SHL_L	(CONST(8), EAX)
		MOV_B	(CH, AL)
		ROR_L	(CONST(8), EAX)

		PUSH_L	(EDX)
		PUSH_L	(EAX)
		MOVQ	(REGIND(ESP), MM0)
		PUSH_L	(ECX)
		MOVQ	(REGIND(ESP), MM1)
		PUSH_L	(EDX)
		MOVQ	(REGIND(ESP), MM2)

		MOV_L	(CONST(0xaaaaaaab), EAX)
		MUL_L	(CONTENT(_vl_current_bytes))
		MOV_L	(EDX, ECX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EDX)
		SHR_L	(CONST(4), ECX)
		ALIGNTEXT4
	TLBL(0):
		MOVQ	(MM0, REGIND(EDX))
		MOVQ	(MM1, REGOFF(8,EDX))
		MOVQ	(MM2, REGOFF(16,EDX))
		ADD_L	(CONST(24), EDX)
		DEC_L	(ECX)
		jnz	TLBL(0)
		EMMS
		ADD_L	(CONST(16), ESP)
#endif
		RET

/* Desc: void v_rect8 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_rect8
_v_rect8:
		CLD
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		MOV_L	(REGOFF(28, ESP), EAX)
		MOV_L	(CONTENT(_vl_current_width), ESI)
		MOV_L	(REGOFF(16, ESP), EDI)
		MOV_B	(AL, AH)
		MOV_L	(REGOFF(20, ESP), ECX)
		IMUL_L	(ESI, EDI)
		MOV_L	(REGOFF(24, ESP), EDX)
		SUB_L	(ECX, ESI)
		ADD_L	(REGOFF(12, ESP), EDI)
		PUSH_W	(AX)
		PUSH_W	(AX)
		PUSH_SR	(DS)
		POP_SR	(ES)
		ADD_L	(CONTENT(_vl_current_draw_buffer), EDI)
		POP_L	(EAX)
		PUSH_L	(EBX)
		MOV_L	(ECX, EBX)
		AND_L	(CONST(3), EBX)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		ALIGNTEXT4
	TLBL(1):
		SHR_L	(CONST(2), ECX)
		REP	STOS_L
		TEST_L	(EBX, EBX)
		jz	TLBL(2)
		MOV_L	(EBX, ECX)
		REP	STOS_B
		ALIGNTEXT4
	TLBL(2):
		POP_L	(ECX)
		ADD_L	(ESI, EDI)
		DEC_L	(EDX)
		jnz	TLBL(0)
		POP_L	(EBX)
		POP_L	(EDI)
		POP_L	(ESI)
		RET

/* Desc: void v_rect16 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_rect16
_v_rect16:
		CLD
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		MOV_L	(REGOFF(28, ESP), EAX)
		MOV_L	(CONTENT(_vl_current_width), ESI)
		MOV_L	(REGOFF(16, ESP), EDI)
		MOV_L	(REGOFF(20, ESP), ECX)
		IMUL_L	(ESI, EDI)
		MOV_L	(REGOFF(24, ESP), EDX)
		SUB_L	(ECX, ESI)
		ADD_L	(REGOFF(12, ESP), EDI)
		PUSH_W	(AX)
		SHL_L	(CONST(1), ESI)
		PUSH_W	(AX)
		SHL_L	(CONST(1), EDI)
		PUSH_SR	(DS)
		POP_SR	(ES)
		ADD_L	(CONTENT(_vl_current_draw_buffer), EDI)
		POP_L	(EAX)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		ALIGNTEXT4
	TLBL(1):
		SHR_L	(CONST(1), ECX)
		REP	STOS_L
		jnc	TLBL(2)
		STOS_W
		ALIGNTEXT4
	TLBL(2):
		POP_L	(ECX)
		ADD_L	(ESI, EDI)
		DEC_L	(EDX)
		jnz	TLBL(0)
		POP_L	(EDI)
		POP_L	(ESI)
		RET

/* Desc: void v_rect24 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_rect24
_v_rect24:
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		MOV_L	(REGOFF(28, ESP), EAX)
		MOV_L	(CONTENT(_vl_current_width), ESI)
		MOV_L	(REGOFF(16, ESP), EDI)
		MOV_L	(REGOFF(20, ESP), ECX)
		IMUL_L	(ESI, EDI)
		MOV_L	(REGOFF(24, ESP), EDX)
		SUB_L	(ECX, ESI)
		ADD_L	(REGOFF(12, ESP), EDI)
		LEA_L	(REGBIS(ESI, ESI, 2), ESI)
		PUSH_L	(EBX)
		LEA_L	(REGBIS(EDI, EDI, 2), EDI)
		MOV_L	(EAX, EBX)
		ADD_L	(CONTENT(_vl_current_draw_buffer), EDI)
		SHR_L	(CONST(16), EBX)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		ALIGNTEXT4
	TLBL(1):
		MOV_W	(AX, REGIND(EDI))
		MOV_B	(BL, REGOFF(2,EDI))
		ADD_L	(CONST(3), EDI)
		DEC_L	(ECX)
		jnz	TLBL(1)
		POP_L	(ECX)
		ADD_L	(ESI, EDI)
		DEC_L	(EDX)
		jnz	TLBL(0)
		POP_L	(EBX)
		POP_L	(EDI)
		POP_L	(ESI)
		RET

/* Desc: void v_rect32 (int color);
 *
 * In  : color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_rect32
_v_rect32:
		PUSH_L	(ESI)
		PUSH_L	(EDI)
		MOV_L	(CONTENT(_vl_current_width), ESI)
		MOV_L	(REGOFF(16, ESP), EDI)
		MOV_L	(REGOFF(20, ESP), ECX)
		IMUL_L	(ESI, EDI)
		MOV_L	(REGOFF(24, ESP), EDX)
		SUB_L	(ECX, ESI)
		ADD_L	(REGOFF(12, ESP), EDI)
		SHL_L	(CONST(2), ESI)
		SHL_L	(CONST(2), EDI)
		MOV_L	(REGOFF(28, ESP), EAX)
		ADD_L	(CONTENT(_vl_current_draw_buffer), EDI)
		ALIGNTEXT4
	TLBL(0):
		PUSH_L	(ECX)
		ALIGNTEXT4
	TLBL(1):
		MOV_L	(EAX, REGIND(EDI))
		ADD_L	(CONST(4), EDI)
		DEC_L	(ECX)
		jnz	TLBL(1)
		POP_L	(ECX)
		ADD_L	(ESI, EDI)
		DEC_L	(EDX)
		jnz	TLBL(0)
		POP_L	(EDI)
		POP_L	(ESI)
		RET

/* Desc: void v_putpixel8 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_putpixel8
_v_putpixel8:
		MOV_L	(REGOFF(8, ESP), ECX)
		MOV_L	(REGOFF(4, ESP), EDX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EAX)
		MOV_B	(CL, REGBI(EAX,EDX))
		RET

/* Desc: void v_putpixel16 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_putpixel16
_v_putpixel16:
		MOV_L	(REGOFF(8, ESP), ECX)
		MOV_L	(REGOFF(4, ESP), EDX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EAX)
		MOV_W	(CX, REGBIS(EAX,EDX,2))
		RET

/* Desc: void v_putpixel24 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_putpixel24
_v_putpixel24:
		MOV_L	(REGOFF(4, ESP), EAX)
		MOV_L	(REGOFF(8, ESP), EDX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), ECX)
		LEA_L	(REGBIS(EAX,EAX,2), EAX)
		MOV_W	(DX, REGBI(ECX,EAX))
		SHR_L	(CONST(16), EDX)
		MOV_B	(DL, REGBID(ECX,EAX,2))
		RET

/* Desc: void v_putpixel32 (unsigned int offset, int color);
 *
 * In  : offset within buffer, color
 * Out : -
 *
 * Note: uses current draw buffer
 */
		ALIGNTEXT32
		GLOBL	_v_putpixel32
_v_putpixel32:
		MOV_L	(REGOFF(8, ESP), ECX)
		MOV_L	(REGOFF(4, ESP), EDX)
		MOV_L	(CONTENT(_vl_current_draw_buffer), EAX)
		MOV_L	(ECX, REGBIS(EAX,EDX,4))
		RET
