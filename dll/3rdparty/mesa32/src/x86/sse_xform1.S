/* $Id: sse_xform1.S,v 1.4 2006/04/17 18:58:24 krh Exp $ */

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

/** TODO:
  * - insert PREFETCH instructions to avoid cache-misses !
  * - some more optimizations are possible...
  * - for 40-50% more performance in the SSE-functions, the
  *   data (trans-matrix, src_vert, dst_vert) needs to be 16byte aligned !
  */

#ifdef USE_SSE_ASM
#include "matypes.h"
#include "xform_args.h"

   SEG_TEXT

#define S(i) 	REGOFF(i * 4, ESI)
#define D(i) 	REGOFF(i * 4, EDI)
#define M(i) 	REGOFF(i * 4, EDX)


ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_general)
HIDDEN( _mesa_sse_transform_points1_general )
GLNAME( _mesa_sse_transform_points1_general ):

#define FRAME_OFFSET 8
    PUSH_L    ( ESI )
    PUSH_L    ( EDI )

    MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI ) 	/* ptr to source GLvector4f */
    MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

    MOV_L( ARG_MATRIX, EDX ) 			/* ptr to matrix */
    MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

    CMP_L( CONST(0), ECX )			/* count == 0 ? */
    JE( LLBL(K_GTP1GR_finish) )			/* yes -> nothing to do. */

    MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
    OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

    MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
    MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

    SHL_L( CONST(4), ECX ) 			/* count *= 16 */
    MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

    MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
    ADD_L( EDI, ECX ) 				/* count += dest ptr */


ALIGNTEXT32
    MOVAPS( M(0), XMM0 )			/* m3  | m2  | m1  | m0  */
    MOVAPS( M(12), XMM1 )			/* m15 | m14 | m13 | m12 */

ALIGNTEXT32
LLBL(K_GTP1GR_top):
    MOVSS( S(0), XMM2 )				/* ox */
    SHUFPS( CONST(0x0), XMM2, XMM2 )		/* ox | ox | ox | ox */
    MULPS( XMM0, XMM2 )				/* ox*m3 | ox*m2 | ox*m1 | ox*m0 */
    ADDPS( XMM1, XMM2 )				/* + | + | + | + */
    MOVUPS( XMM2, D(0) )

LLBL(K_GTP1GR_skip):
    ADD_L     ( CONST(16), EDI )
    ADD_L     ( EAX, ESI )
    CMP_L     ( ECX, EDI )
    JNE       ( LLBL(K_GTP1GR_top) )

LLBL(K_GTP1GR_finish):
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET
#undef FRAME_OFFSET



ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_identity)
HIDDEN(_mesa_sse_transform_points1_identity)
GLNAME( _mesa_sse_transform_points1_identity ):

#define FRAME_OFFSET 8
    PUSH_L    ( ESI )
    PUSH_L    ( EDI )

    MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI ) 	/* ptr to source GLvector4f */
    MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

    MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

    TEST_L( ECX, ECX)
    JZ( LLBL(K_GTP1IR_finish) ) 		/* count was zero; go to finish */

    MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
    OR_L( CONST(VEC_SIZE_1), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

    MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
    MOV_L( CONST(1), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

    SHL_L( CONST(4), ECX ) 			/* count *= 16 */
    MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

    MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
    ADD_L( EDI, ECX ) 				/* count += dest ptr */

    CMP_L( ESI, EDI )
    JE( LLBL(K_GTP1IR_finish) )


ALIGNTEXT32
LLBL(K_GTP1IR_top):
    MOV_L( S(0), EDX )
    MOV_L( EDX, D(0) )

LLBL(K_GTP1IR_skip):
    ADD_L     ( CONST(16), EDI )
    ADD_L     ( EAX, ESI )
    CMP_L     ( ECX, EDI )
    JNE       ( LLBL(K_GTP1IR_top) )

LLBL(K_GTP1IR_finish):
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET
#undef FRAME_OFFSET



ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_3d_no_rot)
HIDDEN(_mesa_sse_transform_points1_3d_no_rot)
GLNAME(_mesa_sse_transform_points1_3d_no_rot):

#define FRAME_OFFSET 8
    PUSH_L( ESI )
    PUSH_L( EDI )

    MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI )	/* ptr to source GLvector4f */
    MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

    MOV_L( ARG_MATRIX, EDX ) 			/* ptr to matrix */
    MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

    TEST_L( ECX, ECX)
    JZ( LLBL(K_GTP13DNRR_finish) ) 		/* count was zero; go to finish */

    MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
    OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

    MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
    MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

    SHL_L( CONST(4), ECX ) 			/* count *= 16 */
    MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

    MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
    ADD_L( EDI, ECX ) 				/* count += dest ptr */


ALIGNTEXT32
    MOVSS( M(0), XMM0 )				/* m0 */
    MOVSS( M(12), XMM1 )			/* m12 */
    MOVSS( M(13), XMM2 )			/* m13 */
    MOVSS( M(14), XMM3 )			/* m14 */

ALIGNTEXT32
LLBL(K_GTP13DNRR_top):
    MOVSS( S(0), XMM4 )				/* ox */
    MULSS( XMM0, XMM4 )				/* ox*m0 */
    ADDSS( XMM1, XMM4 )				/* ox*m0+m12 */
    MOVSS( XMM4, D(0) )

    MOVSS( XMM2, D(1) )
    MOVSS( XMM3, D(2) )

LLBL(K_GTP13DNRR_skip):
    ADD_L    ( CONST(16), EDI )
    ADD_L    ( EAX, ESI )
    CMP_L    ( ECX, EDI )
    JNE      ( LLBL(K_GTP13DNRR_top) )

LLBL(K_GTP13DNRR_finish):
    POP_L    ( EDI )
    POP_L    ( ESI )
    RET
#undef FRAME_OFFSET



ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_perspective)
HIDDEN(_mesa_sse_transform_points1_perspective)
GLNAME(_mesa_sse_transform_points1_perspective):

#define FRAME_OFFSET 8
    PUSH_L   ( ESI )
    PUSH_L   ( EDI )

    MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI ) 	/* ptr to source GLvector4f */
    MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

    MOV_L( ARG_MATRIX, EDX ) 			/* ptr to matrix */
    MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

    TEST_L( ECX, ECX)
    JZ( LLBL(K_GTP13PR_finish) )		/* count was zero; go to finish */

    MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
    OR_L( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

    MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
    MOV_L( CONST(4), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

    SHL_L( CONST(4), ECX ) 			/* count *= 16 */
    MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

    MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
    ADD_L( EDI, ECX ) 				/* count += dest ptr */


ALIGNTEXT32
    XORPS( XMM0, XMM0 )				/* 0 | 0 | 0 | 0 */
    MOVSS( M(0), XMM1 )				/* m0 */
    MOVSS( M(14), XMM2 )			/* m14 */

ALIGNTEXT32
LLBL(K_GTP13PR_top):
    MOVSS( S(0), XMM3 )				/* ox */
    MULSS( XMM1, XMM3 )				/* ox*m0 */
    MOVSS( XMM3, D(0) )				/* ox*m0->D(0) */
    MOVSS( XMM2, D(2) )				/* m14->D(2) */

    MOVSS( XMM0, D(1) )
    MOVSS( XMM0, D(3) )

LLBL(K_GTP13PR_skip):
    ADD_L( CONST(16), EDI )
    ADD_L( EAX, ESI )
    CMP_L( ECX, EDI )
    JNE( LLBL(K_GTP13PR_top) )

LLBL(K_GTP13PR_finish):
    POP_L    ( EDI )
    POP_L    ( ESI )
    RET
#undef FRAME_OFFSET


ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_2d)
HIDDEN(_mesa_sse_transform_points1_2d)
GLNAME(_mesa_sse_transform_points1_2d):

#define FRAME_OFFSET 8
    PUSH_L( ESI )
    PUSH_L( EDI )

    MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI ) 	/* ptr to source GLvector4f */
    MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

    MOV_L( ARG_MATRIX, EDX ) 			/* ptr to matrix */
    MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

    TEST_L( ECX, ECX)
    JZ( LLBL(K_GTP13P2DR_finish) ) 		/* count was zero; go to finish */

    MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
    OR_L( CONST(VEC_SIZE_2), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

    MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
    MOV_L( CONST(2), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

    SHL_L( CONST(4), ECX ) 			/* count *= 16 */
    MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

    MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
    ADD_L( EDI, ECX ) 				/* count += dest ptr */

ALIGNTEXT32
    MOVLPS( M(0), XMM0 )			/* m1  | m0  */
    MOVLPS( M(12), XMM1 )			/* m13 | m12 */

ALIGNTEXT32
LLBL(K_GTP13P2DR_top):
    MOVSS( S(0), XMM2 )				/* ox */
    SHUFPS( CONST(0x0), XMM2, XMM2 )		/* ox | ox | ox | ox */
    MULPS( XMM0, XMM2 )				/* - | - | ox*m1 | ox*m0 */
    ADDPS( XMM1, XMM2 )				/* - | - | ox*m1+m13 | ox*m0+m12 */
    MOVLPS( XMM2, D(0) )

LLBL(K_GTP13P2DR_skip):
    ADD_L    ( CONST(16), EDI )
    ADD_L    ( EAX, ESI )
    CMP_L    ( ECX, EDI )
    JNE      ( LLBL(K_GTP13P2DR_top) )

LLBL(K_GTP13P2DR_finish):
    POP_L    ( EDI )
    POP_L    ( ESI )
    RET
#undef FRAME_OFFSET


ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_2d_no_rot)
HIDDEN(_mesa_sse_transform_points1_2d_no_rot)
GLNAME(_mesa_sse_transform_points1_2d_no_rot):

#define FRAME_OFFSET 8
	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI ) 	/* ptr to source GLvector4f */
	MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

	MOV_L( ARG_MATRIX, EDX ) 		/* ptr to matrix */
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

	TEST_L( ECX, ECX)
	JZ( LLBL(K_GTP13P2DNRR_finish) ) 	/* count was zero; go to finish */

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
	OR_L( CONST(VEC_SIZE_2), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
	MOV_L( CONST(2), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

	SHL_L( CONST(4), ECX ) 			/* count *= 16 */
	MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

	MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
	ADD_L( EDI, ECX ) 			/* count += dest ptr */

ALIGNTEXT32
	MOVSS( M(0), XMM0 )			/* m0 */
	MOVSS( M(12), XMM1 )			/* m12 */
	MOVSS( M(13), XMM2 )			/* m13 */

ALIGNTEXT32
LLBL(K_GTP13P2DNRR_top):
	MOVSS( S(0), XMM3 )			/* ox */
	MULSS( XMM0, XMM3 )			/* ox*m0 */
	ADDSS( XMM1, XMM3 )			/* ox*m0+m12 */
	MOVSS( XMM3, D(0) )
	MOVSS( XMM2, D(1) )

LLBL(K_GTP13P2DNRR_skip):
	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL(K_GTP13P2DNRR_top) )

LLBL(K_GTP13P2DNRR_finish):
	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET



ALIGNTEXT4
GLOBL GLNAME(_mesa_sse_transform_points1_3d)
HIDDEN(_mesa_sse_transform_points1_3d)
GLNAME(_mesa_sse_transform_points1_3d):

#define FRAME_OFFSET 8
	PUSH_L( ESI )
	PUSH_L( EDI )

	MOV_L( REGOFF(OFFSET_SOURCE+8, ESP), ESI ) 	/* ptr to source GLvector4f */
	MOV_L( REGOFF(OFFSET_DEST+8, ESP), EDI ) 	/* ptr to dest GLvector4f */

	MOV_L( ARG_MATRIX, EDX ) 		/* ptr to matrix */
	MOV_L( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

	TEST_L( ECX, ECX)
	JZ( LLBL(K_GTP13P3DR_finish) ) 	/* count was zero; go to finish */

	MOV_L( REGOFF(V4F_STRIDE, ESI), EAX ) 	/* stride */
	OR_L( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, EDI) ) 	/* set dest flags */

	MOV_L( ECX, REGOFF(V4F_COUNT, EDI) ) 	/* set dest count */
	MOV_L( CONST(3), REGOFF(V4F_SIZE, EDI) ) 	/* set dest size */

	SHL_L( CONST(4), ECX ) 			/* count *= 16 */
	MOV_L( REGOFF(V4F_START, ESI), ESI ) 	/* ptr to first source vertex */

	MOV_L( REGOFF(V4F_START, EDI), EDI ) 	/* ptr to first dest vertex */
	ADD_L( EDI, ECX ) 			/* count += dest ptr */


ALIGNTEXT32
	MOVAPS( M(0), XMM0 )			/* m3  | m2  | m1  |  m0 */
	MOVAPS( M(12), XMM1 )			/* m15 | m14 | m13 | m12 */

ALIGNTEXT32
LLBL(K_GTP13P3DR_top):
	MOVSS( S(0), XMM2 )			/* ox */
	SHUFPS( CONST(0x0), XMM2, XMM2 )	/* ox | ox | ox | ox */
	MULPS( XMM0, XMM2 )			/* ox*m3 | ox*m2 | ox*m1 | ox*m0 */
	ADDPS( XMM1, XMM2 )			/* +m15  | +m14  | +m13  | +m12  */
	MOVLPS( XMM2, D(0) )			/*   -   |   -   | ->D(1)| ->D(0)*/
	UNPCKHPS( XMM2, XMM2 )			/* ox*m3+m15 | ox*m3+m15 | ox*m2+m14 | ox*m2+m14 */
	MOVSS( XMM2, D(2) )

LLBL(K_GTP13P3DR_skip):
	ADD_L( CONST(16), EDI )
	ADD_L( EAX, ESI )
	CMP_L( ECX, EDI )
	JNE( LLBL(K_GTP13P3DR_top) )

LLBL(K_GTP13P3DR_finish):
	POP_L( EDI )
	POP_L( ESI )
	RET
#undef FRAME_OFFSET
#endif
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
