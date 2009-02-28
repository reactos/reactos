
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
#include "assyntax.h"
#include "matypes.h"
#include "norm_args.h"

   SEG_TEXT

#define M(i)    REGOFF(i * 4, EDX)
#define S(i)	REGOFF(i * 4, ESI)
#define D(i)	REGOFF(i * 4, EDI)
#define STRIDE  REGOFF(12, ESI)


ALIGNTEXT16
GLOBL GLNAME(_mesa_sse_transform_rescale_normals_no_rot)
HIDDEN(_mesa_sse_transform_rescale_normals_no_rot)
GLNAME(_mesa_sse_transform_rescale_normals_no_rot):

#define FRAME_OFFSET 8
	PUSH_L  ( ESI )
	PUSH_L  ( EDI )

	MOV_L	( ARG_IN, ESI )				/* ptr to source GLvector3f */
	MOV_L	( ARG_DEST, EDI )			/* ptr to dest GLvector3f */

	MOV_L	( ARG_MAT, EDX )			/* ptr to matrix */
	MOV_L	( REGOFF(MATRIX_INV, EDX), EDX)		/* matrix->inv */

	MOV_L	( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

	TEST_L	( ECX, ECX )
	JZ( LLBL(K_G3TRNNRR_finish) )			/* count was zero; go to finish */

	MOV_L	( STRIDE, EAX )				/* stride */
	MOV_L	( ECX, REGOFF(V4F_COUNT, EDI) )		/* set dest-count */

	IMUL_L( CONST(16), ECX )			/* count *= 16 */
	MOV_L( REGOFF(V4F_START, ESI), ESI )		/* ptr to first source vertex */

	MOV_L( REGOFF(V4F_START, EDI), EDI )		/* ptr to first dest vertex */
	ADD_L( EDI, ECX ) 				/* count += dest ptr */

ALIGNTEXT32
	MOVSS	( M(0), XMM1 )				/* m0 */
	MOVSS	( M(5), XMM2 )				/* m5 */
	UNPCKLPS( XMM2, XMM1 )				/* m5 | m0 */
	MOVSS	( ARG_SCALE, XMM0 )			/* scale */
	SHUFPS	( CONST(0x0), XMM0, XMM0 )		/* scale | scale */
	MULPS	( XMM0, XMM1 )				/* m5*scale | m0*scale */
	MULSS	( M(10), XMM0 )				/* m10*scale */

ALIGNTEXT32
LLBL(K_G3TRNNRR_top):
	MOVLPS	( S(0), XMM2 )				/* uy | ux */
	MULPS	( XMM1, XMM2 )				/* uy*m5*scale | ux*m0*scale */
	MOVLPS	( XMM2, D(0) )				/* ->D(1) | D(0) */

	MOVSS	( S(2), XMM2 )				/* uz */
	MULSS	( XMM0, XMM2 )				/* uz*m10*scale */
	MOVSS	( XMM2, D(2) )				/* ->D(2) */

LLBL(K_G3TRNNRR_skip):
	ADD_L	( CONST(16), EDI )
	ADD_L	( EAX, ESI )
	CMP_L	( ECX, EDI )
	JNE	( LLBL(K_G3TRNNRR_top) )

LLBL(K_G3TRNNRR_finish):
	POP_L	( EDI )
	POP_L	( ESI )
	RET
#undef FRAME_OFFSET



ALIGNTEXT16
GLOBL GLNAME(_mesa_sse_transform_rescale_normals)
HIDDEN(_mesa_sse_transform_rescale_normals)
GLNAME(_mesa_sse_transform_rescale_normals):

#define FRAME_OFFSET 8
	PUSH_L  ( ESI )
	PUSH_L  ( EDI )

	MOV_L	( ARG_IN, ESI )				/* ptr to source GLvector3f */
	MOV_L	( ARG_DEST, EDI )			/* ptr to dest GLvector3f */

	MOV_L	( ARG_MAT, EDX )			/* ptr to matrix */
	MOV_L	( REGOFF(MATRIX_INV, EDX), EDX)		/* matrix->inv */

	MOV_L	( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

	TEST_L	( ECX, ECX )
	JZ( LLBL(K_G3TRNR_finish) )			/* count was zero; go to finish */

	MOV_L	( STRIDE, EAX )				/* stride */
	MOV_L	( ECX, REGOFF(V4F_COUNT, EDI) )		/* set dest-count */

	IMUL_L( CONST(16), ECX )			/* count *= 16 */
	MOV_L( REGOFF(V4F_START, ESI), ESI )		/* ptr to first source vertex */

	MOV_L( REGOFF(V4F_START, EDI), EDI )		/* ptr to first dest vertex */
	ADD_L( EDI, ECX ) 				/* count += dest ptr */

ALIGNTEXT32
	MOVSS	( M(0), XMM0 )				/* m0 */
	MOVSS	( M(4), XMM1 )				/* m4 */
	UNPCKLPS( XMM1, XMM0 )				/* m4 | m0 */

	MOVSS	( ARG_SCALE, XMM4 )			/* scale */
	SHUFPS	( CONST(0x0), XMM4, XMM4 )		/* scale | scale */

	MULPS	( XMM4, XMM0 )				/* m4*scale | m0*scale */
	MOVSS	( M(1), XMM1 )				/* m1 */
	MOVSS	( M(5), XMM2 )				/* m5 */
	UNPCKLPS( XMM2, XMM1 )				/* m5 | m1 */
	MULPS	( XMM4, XMM1 )				/* m5*scale | m1*scale */
	MOVSS	( M(2), XMM2 )				/* m2 */
	MOVSS	( M(6), XMM3 )				/* m6 */
	UNPCKLPS( XMM3, XMM2 )				/* m6 | m2 */
	MULPS	( XMM4, XMM2 )				/* m6*scale | m2*scale */

	MOVSS	( M(8), XMM6 )				/* m8 */
	MULSS	( ARG_SCALE, XMM6 )			/* m8*scale */
	MOVSS	( M(9), XMM7 )				/* m9 */
	MULSS	( ARG_SCALE, XMM7 )			/* m9*scale */

ALIGNTEXT32
LLBL(K_G3TRNR_top):
	MOVSS	( S(0), XMM3 )				/* ux */
	SHUFPS	( CONST(0x0), XMM3, XMM3 )		/* ux | ux */
	MULPS	( XMM0, XMM3 )				/* ux*m4 | ux*m0 */
	MOVSS	( S(1), XMM4 )				/* uy */
	SHUFPS	( CONST(0x0), XMM4, XMM4 )		/* uy | uy */
	MULPS	( XMM1, XMM4 )				/* uy*m5 | uy*m1 */
	MOVSS	( S(2), XMM5 )				/* uz */
	SHUFPS	( CONST(0x0), XMM5, XMM5 )		/* uz | uz */
	MULPS	( XMM2, XMM5 )				/* uz*m6 | uz*m2 */

	ADDPS	( XMM4, XMM3 )
	ADDPS	( XMM5, XMM3 )
	MOVLPS	( XMM3, D(0) )

	MOVSS	( M(10), XMM3 )				/* m10 */
	MULSS	( ARG_SCALE, XMM3 )			/* m10*scale */
	MULSS	( S(2), XMM3 )				/* m10*scale*uz */
	MOVSS	( S(1), XMM4 )				/* uy */
	MULSS	( XMM7, XMM4 )				/* uy*m9*scale */
	MOVSS	( S(0), XMM5 )				/* ux */
	MULSS	( XMM6, XMM5 )				/* ux*m8*scale */

	ADDSS	( XMM4, XMM3 )
	ADDSS	( XMM5, XMM3 )
	MOVSS	( XMM3, D(2) )

LLBL(K_G3TRNR_skip):
	ADD_L	( CONST(16), EDI )
	ADD_L	( EAX, ESI )
	CMP_L	( ECX, EDI )
	JNE	( LLBL(K_G3TRNR_top) )

LLBL(K_G3TRNR_finish):
	POP_L	( EDI )
	POP_L	( ESI )
	RET
#undef FRAME_OFFSET


ALIGNTEXT16
GLOBL GLNAME(_mesa_sse_transform_normals_no_rot)
HIDDEN(_mesa_sse_transform_normals_no_rot)
GLNAME(_mesa_sse_transform_normals_no_rot):

#define FRAME_OFFSET 8
	PUSH_L  ( ESI )
	PUSH_L  ( EDI )

	MOV_L	( ARG_IN, ESI )				/* ptr to source GLvector3f */
	MOV_L	( ARG_DEST, EDI )			/* ptr to dest GLvector3f */

	MOV_L	( ARG_MAT, EDX )			/* ptr to matrix */
	MOV_L	( REGOFF(MATRIX_INV, EDX), EDX)		/* matrix->inv */

	MOV_L	( REGOFF(V4F_COUNT, ESI), ECX ) 	/* source count */

	TEST_L	( ECX, ECX )
	JZ( LLBL(K_G3TNNRR_finish) )			/* count was zero; go to finish */

	MOV_L	( STRIDE, EAX )				/* stride */
	MOV_L	( ECX, REGOFF(V4F_COUNT, EDI) )		/* set dest-count */

	IMUL_L( CONST(16), ECX )			/* count *= 16 */
	MOV_L( REGOFF(V4F_START, ESI), ESI )		/* ptr to first source vertex */

	MOV_L( REGOFF(V4F_START, EDI), EDI )		/* ptr to first dest vertex */
	ADD_L( EDI, ECX ) 				/* count += dest ptr */

ALIGNTEXT32
	MOVSS( M(0), XMM0 )				/* m0 */
	MOVSS( M(5), XMM1 )				/* m5 */
	UNPCKLPS( XMM1, XMM0 )				/* m5 | m0 */
	MOVSS( M(10), XMM1 )				/* m10 */

ALIGNTEXT32
LLBL(K_G3TNNRR_top):
	MOVLPS( S(0), XMM2 )				/* uy | ux */
	MULPS( XMM0, XMM2 )				/* uy*m5 | ux*m0 */
	MOVLPS( XMM2, D(0) )

	MOVSS( S(2), XMM2 )				/* uz */
	MULSS( XMM1, XMM2 )				/* uz*m10 */
	MOVSS( XMM2, D(2) )

LLBL(K_G3TNNRR_skip):
	ADD_L	( CONST(16), EDI )
	ADD_L	( EAX, ESI )
	CMP_L	( ECX, EDI )
	JNE	( LLBL(K_G3TNNRR_top) )

LLBL(K_G3TNNRR_finish):
	POP_L	( EDI )
	POP_L	( ESI )
	RET
#undef FRAME_OFFSET
#endif
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
