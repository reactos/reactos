
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

#ifdef USE_SSE_ASM
#include "assyntax.h"
#include "matypes.h"
#include "xform_args.h"

	SEG_TEXT

#define FRAME_OFFSET	8

#define SRC(i)		REGOFF(i * 4, ESI)
#define DST(i)		REGOFF(i * 4, EDI)
#define MAT(i)		REGOFF(i * 4, EDX)

#define SELECT(r0, r1, r2, r3)	CONST( r0 * 64 + r1 * 16 + r2 * 4 + r3 )


ALIGNTEXT16
GLOBL GLNAME( _mesa_sse_transform_points4_general )
HIDDEN(_mesa_sse_transform_points4_general)
GLNAME( _mesa_sse_transform_points4_general ):

	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )			/* verify non-zero count */
	JE( LLBL( sse_general_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )	/* stride */
	OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) ) /* set dest flags */

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )	/* set dest count */
	MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) )/* set dest size */

	MOV_L( REGOFF(V4F_START, ESI), ESI )	/* ptr to first source vertex */
	MOV_L( REGOFF(V4F_START, EDI), EDI )	/* ptr to first dest vertex */

	PREFETCHT0( REGIND(ESI) )

	MOVAPS( MAT(0), XMM4 )			/* m3  | m2  | m1  | m0  */
	MOVAPS( MAT(4), XMM5 )			/* m7  | m6  | m5  | m4  */
	MOVAPS( MAT(8), XMM6 )			/* m11 | m10 | m9  | m8  */
	MOVAPS( MAT(12), XMM7 )			/* m15 | m14 | m13 | m12 */

ALIGNTEXT16
LLBL( sse_general_loop ):

	MOVSS( SRC(0), XMM0 )			/* ox */
	SHUFPS( CONST(0x0), XMM0, XMM0 )	/* ox | ox | ox | ox */
	MULPS( XMM4, XMM0 )			/* ox*m3 | ox*m2 | ox*m1 | ox*m0 */

	MOVSS( SRC(1), XMM1 )			/* oy */
	SHUFPS( CONST(0x0), XMM1, XMM1 )	/* oy | oy | oy | oy */
	MULPS( XMM5, XMM1 )			/* oy*m7 | oy*m6 | oy*m5 | oy*m4 */

	MOVSS( SRC(2), XMM2 )			/* oz */
	SHUFPS( CONST(0x0), XMM2, XMM2 )	/* oz | oz | oz | oz */
	MULPS( XMM6, XMM2 )			/* oz*m11 | oz*m10 | oz*m9 | oz*m8 */

	MOVSS( SRC(3), XMM3 )			/* ow */
	SHUFPS( CONST(0x0), XMM3, XMM3 )	/* ow | ow | ow | ow */
	MULPS( XMM7, XMM3 )			/* ow*m15 | ow*m14 | ow*m13 | ow*m12 */

	ADDPS( XMM1, XMM0 )			/* ox*m3+oy*m7 | ... */
	ADDPS( XMM2, XMM0 )			/* ox*m3+oy*m7+oz*m11 | ... */
	ADDPS( XMM3, XMM0 )			/* ox*m3+oy*m7+oz*m11+ow*m15 | ... */
	MOVAPS( XMM0, DST(0) )			/* ->D(3) | ->D(2) | ->D(1) | ->D(0) */

	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )

	DEC_L( ECX )
	JNZ( LLBL( sse_general_loop ) )

LLBL( sse_general_done ):

	POP_L( EDI )
	POP_L( ESI )
	RET




ALIGNTEXT4
GLOBL GLNAME( _mesa_sse_transform_points4_3d )
HIDDEN(_mesa_sse_transform_points4_3d)
GLNAME( _mesa_sse_transform_points4_3d ):

	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( ARG_SOURCE, ESI )		/* ptr to source GLvector4f */
	MOV_L( ARG_DEST, EDI )			/* ptr to dest GLvector4f */

	MOV_L( ARG_MATRIX, EDX )		/* ptr to matrix */
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )	/* source count */

	TEST_L( ECX, ECX)
	JZ( LLBL(K_GTP43P3DR_finish) )		/* count was zero; go to finish */

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )	/* stride */
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) ) /* set dest flags */

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )	/* set dest count */
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) )/* set dest size */

	SHL_L( CONST(4), ECX )			/* count *= 16 */
	MOV_L( REGOFF(V4F_START, ESI), ESI )	/* ptr to first source vertex */

	MOV_L( REGOFF(V4F_START, EDI), EDI )	/* ptr to first dest vertex */
	ADD_L( EDI, ECX )			/* count += dest ptr */

	MOVAPS( MAT(0), XMM0 )			/* m3  | m2  | m1  |  m0 */
	MOVAPS( MAT(4), XMM1 )			/* m7  | m6  | m5  |  m4 */
	MOVAPS( MAT(8), XMM2 )			/* m11 | m10 | m9  |  m8 */
	MOVAPS( MAT(12), XMM3 )			/* m15 | m14 | m13 | m12 */

ALIGNTEXT32
LLBL( K_GTP43P3DR_top ):
	MOVSS( SRC(0), XMM4 )			/* ox */
	SHUFPS( CONST(0x0), XMM4, XMM4 )	/* ox | ox | ox | ox */
	MULPS( XMM0, XMM4 )			/* ox*m3 | ox*m2 | ox*m1 | ox*m0 */

	MOVSS( SRC(1), XMM5 )			/* oy */
	SHUFPS( CONST(0x0), XMM5, XMM5 )	/* oy | oy | oy | oy */
	MULPS( XMM1, XMM5 )			/* oy*m7 | oy*m6 | oy*m5 | oy*m4 */

	MOVSS( SRC(2), XMM6 )			/* oz */
	SHUFPS( CONST(0x0), XMM6, XMM6 )	/* oz | oz | oz | oz */
	MULPS( XMM2, XMM6 )			/* oz*m11 | oz*m10 | oz*m9 | oz*m8 */

	MOVSS( SRC(3), XMM7 )			/* ow */
	SHUFPS( CONST(0x0), XMM7, XMM7 )	/* ow | ow | ow | ow */
	MULPS( XMM3, XMM7 )			/* ow*m15 | ow*m14 | ow*m13 | ow*m12 */

	ADDPS( XMM5, XMM4 )			/* ox*m3+oy*m7 | ... */
	ADDPS( XMM6, XMM4 )			/* ox*m3+oy*m7+oz*m11 | ... */
	ADDPS( XMM7, XMM4 )			/* ox*m3+oy*m7+oz*m11+ow*m15 | ... */
	MOVAPS( XMM4, DST(0) )			/* ->D(3) | ->D(2) | ->D(1) | ->D(0) */

	MOVSS( SRC(3), XMM4 )			/* ow */
	MOVSS( XMM4, DST(3) )			/* ->D(3) */

LLBL( K_GTP43P3DR_skip ):
	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL(K_GTP43P3DR_top) )

LLBL( K_GTP43P3DR_finish ):
	POP_L( EDI )
	POP_L( ESI )
	RET


ALIGNTEXT16
GLOBL GLNAME( _mesa_sse_transform_points4_identity )
HIDDEN(_mesa_sse_transform_points4_identity)
GLNAME( _mesa_sse_transform_points4_identity ):

	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( ARG_SOURCE, ESI )
	MOV_L( ARG_DEST, EDI )

	MOV_L( ARG_MATRIX, EDX )
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX )

	TEST_L( ECX, ECX )			/* verify non-zero count */
	JE( LLBL( sse_identity_done ) )

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX )	/* stride */
	OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) ) /* set dest flags */

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) )	/* set dest count */
	MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) )/* set dest size */

	MOV_L( REGOFF(V4F_START, ESI), ESI )	/* ptr to first source vertex */
	MOV_L( REGOFF(V4F_START, EDI), EDI )	/* ptr to first dest vertex */

ALIGNTEXT16
LLBL( sse_identity_loop ):

	PREFETCHNTA( REGOFF(32, ESI) )

	MOVAPS( REGIND(ESI), XMM0 )
	ADD_L( EAX, ESI )

	MOVAPS( XMM0, REGIND(EDI) )
	ADD_L( CONST(16), EDI )

	DEC_L( ECX )
	JNZ( LLBL( sse_identity_loop ) )

LLBL( sse_identity_done ):

	POP_L( EDI )
	POP_L( ESI )
	RET
#endif
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
