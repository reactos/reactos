/* $Id: 3dnow_xform4.S,v 1.5 2006/04/17 18:58:24 krh Exp $ */

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

#ifdef USE_3DNOW_ASM
#include "matypes.h"
#include "xform_args.h"

    SEG_TEXT

#define FRAME_OFFSET	4


ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_general )
HIDDEN(_mesa_3dnow_transform_points4_general)
GLNAME( _mesa_3dnow_transform_points4_general ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TPGR_2 ) )

    PREFETCHW ( REGIND(EDX) )

ALIGNTEXT16
LLBL( G3TPGR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM0 )	/* x1            | x0                */
    MOVQ      ( REGOFF(8, EAX), MM4 )	/* x3            | x2                */
	
    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )
	
    MOVQ      ( MM0, MM2 )		/* x1              | x0              */
    MOVQ      ( MM4, MM6 )		/* x3              | x2              */

    PUNPCKLDQ ( MM0, MM0 )		/* x0              | x0              */
    PUNPCKHDQ ( MM2, MM2 )		/* x1              | x1              */

    MOVQ      ( MM0, MM1 )		/* x0              | x0              */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    PFMUL     ( REGIND(ECX), MM0 )	/* x0*m1           | x0*m0           */
    MOVQ      ( MM2, MM3 )		/* x1              | x1              */

    PFMUL     ( REGOFF(8, ECX), MM1 )	/* x0*m3           | x0*m2           */
    PUNPCKLDQ ( MM4, MM4 )		/* x2              | x2              */

    PFMUL     ( REGOFF(16, ECX), MM2 )	/* x1*m5           | x1*m4           */
    MOVQ      ( MM4, MM5 )		/* x2              | x2              */

    PFMUL     ( REGOFF(24, ECX), MM3 )	/* x1*m7           | x1*m6           */
    PUNPCKHDQ ( MM6, MM6 )		/* x3              | x3              */

    PFMUL     ( REGOFF(32, ECX), MM4 )	/* x2*m9           | x2*m8           */
    MOVQ      ( MM6, MM7 )		/* x3              | x3              */

    PFMUL     ( REGOFF(40, ECX), MM5 )	/* x2*m11          | x2*m10          */
    PFADD     ( MM0, MM2 )

    PFMUL     ( REGOFF(48, ECX), MM6 )	/* x3*m13          | x3*m12          */
    PFADD     ( MM1, MM3 )

    PFMUL     ( REGOFF(56, ECX), MM7 )	/* x3*m15          | x3*m14          */
    PFADD     ( MM4, MM6 )

    PFADD     ( MM5, MM7 )
    PFADD     ( MM2, MM6 )

    PFADD     ( MM3, MM7 )
    MOVQ      ( MM6, REGOFF(-16, EDX) )

    MOVQ      ( MM7, REGOFF(-8, EDX) )

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TPGR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TPGR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_perspective )
HIDDEN(_mesa_3dnow_transform_points4_perspective)
GLNAME( _mesa_3dnow_transform_points4_perspective ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TPPR_2 ) )

    PREFETCH  ( REGIND(EAX) )
    PREFETCHW ( REGIND(EDX) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM0 )	/* m11             | m00             */

    MOVD      ( REGOFF(40, ECX), MM1 )	/*                 | m22             */
    PUNPCKLDQ ( REGOFF(56, ECX), MM1 )	/* m32             | m22             */

    MOVQ      ( REGOFF(32, ECX), MM2 )	/* m21             | m20             */
    PXOR      ( MM7, MM7 )		/* 0               | 0               */

ALIGNTEXT16
LLBL( G3TPPR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM4 )	/* x1              | x0              */
    MOVQ      ( REGOFF(8, EAX), MM5 )	/* x3              | x2              */
    MOVD      ( REGOFF(8, EAX), MM3 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGOFF(32, EAX) )	/* hopefully stride is zero          */

    MOVQ      ( MM5, MM6 )		/* x3              | x2              */
    PFMUL     ( MM0, MM4 )		/* x1*m11          | x0*m00          */

    PUNPCKLDQ ( MM5, MM5 )		/* x2              | x2              */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    PFMUL     ( MM2, MM5 )		/* x2*m21          | x2*m20          */
    PFSUBR    ( MM7, MM3 )		/*                 | -x2             */

    PFMUL     ( MM1, MM6 )		/* x3*m32          | x2*m22          */
    PFADD     ( MM4, MM5 )		/* x1*m11+x2*m21   | x0*m00+x2*m20   */

    PFACC     ( MM3, MM6 )		/* -x2             | x2*m22+x3*m32   */
    MOVQ      ( MM5, REGOFF(-16, EDX) )	/* write r0, r1                      */

    MOVQ      ( MM6, REGOFF(-8, EDX) )	/* write r2, r3                      */
    DEC_L     ( ESI )			/* decrement vertex counter          */

    JNZ       ( LLBL( G3TPPR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TPPR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_3d )
HIDDEN(_mesa_3dnow_transform_points4_3d)
GLNAME( _mesa_3dnow_transform_points4_3d ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TP3R_2 ) )

    MOVD      ( REGOFF(8, ECX), MM6 )	/*                 | m2              */
    PUNPCKLDQ ( REGOFF(24, ECX), MM6 )	/* m6              | m2              */

    MOVD      ( REGOFF(40, ECX), MM7 )	/*                 | m10             */
    PUNPCKLDQ ( REGOFF(56, ECX), MM7 )	/* m14             | m10             */

ALIGNTEXT16
LLBL( G3TP3R_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */
    PREFETCH  ( REGOFF(32, EAX) )	/* hopefully array is tightly packed */

    MOVQ      ( REGIND(EAX), MM2 )	/* x1              | x0              */
    MOVQ      ( REGOFF(8, EAX), MM3 )	/* x3              | x2              */

    MOVQ      ( MM2, MM0 )		/* x1              | x0              */
    MOVQ      ( MM3, MM4 )		/* x3              | x2              */

    MOVQ      ( MM0, MM1 )		/* x1              | x0              */
    MOVQ      ( MM4, MM5 )		/* x3              | x2              */

    PUNPCKLDQ ( MM0, MM0 )		/* x0              | x0              */
    PUNPCKHDQ ( MM1, MM1 )		/* x1              | x1              */

    PFMUL     ( REGIND(ECX), MM0 )	/* x0*m1           | x0*m0           */
    PUNPCKLDQ ( MM3, MM3 )		/* x2              | x2              */

    PFMUL     ( REGOFF(16, ECX), MM1 )	/* x1*m5           | x1*m4           */
    PUNPCKHDQ ( MM4, MM4 )		/* x3              | x3              */

    PFMUL     ( MM6, MM2 )		/* x1*m6           | x0*m2           */
    PFADD     ( MM0, MM1 )		/* x0*m1+x1*m5     | x0*m0+x1*m4     */

    PFMUL     ( REGOFF(32, ECX), MM3 )	/* x2*m9           | x2*m8           */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    PFMUL     ( REGOFF(48, ECX), MM4 )	/* x3*m13          | x3*m12          */
    PFADD     ( MM1, MM3 )		/* x0*m1+..+x2*m9  | x0*m0+...+x2*m8 */

    PFMUL     ( MM7, MM5 )		/* x3*m14          | x2*m10          */
    PFADD     ( MM3, MM4 )		/* r1              | r0              */

    PFACC     ( MM2, MM5 )		/* x0*m2+x1*m6     | x2*m10+x3*m14   */
    MOVD      ( REGOFF(12, EAX), MM0 )	/*                 | x3              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PFACC     ( MM0, MM5 )		/* r3              | r2              */

    MOVQ      ( MM4, REGOFF(-16, EDX) )	/* write r0, r1                      */
    MOVQ      ( MM5, REGOFF(-8, EDX) )	/* write r2, r3                      */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TP3R_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP3R_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_3d_no_rot )
HIDDEN(_mesa_3dnow_transform_points4_3d_no_rot)
GLNAME( _mesa_3dnow_transform_points4_3d_no_rot ):

    PUSH_L    ( ESI )
    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TP3NRR_2 ) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM0 )	/* m11             | m00             */

    MOVD      ( REGOFF(40, ECX), MM2 )	/*                 | m22             */
    PUNPCKLDQ ( REGOFF(56, ECX), MM2 )	/* m32             | m22             */

    MOVQ      ( REGOFF(48, ECX), MM1 )	/* m31             | m30             */

ALIGNTEXT16
LLBL( G3TP3NRR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM4 )	/* x1              | x0              */
    MOVQ      ( REGOFF(8, EAX), MM5 )	/* x3              | x2              */
    MOVD      ( REGOFF(12, EAX), MM7 )	/*                 | x3              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGOFF(32, EAX) )	/* hopefully stride is zero          */

    MOVQ      ( MM5, MM6 )		/* x3              | x2              */
    PFMUL     ( MM0, MM4 )		/* x1*m11          | x0*m00          */

    PUNPCKHDQ ( MM6, MM6 )		/* x3              | x3              */
    PFMUL     ( MM2, MM5 )		/* x3*m32          | x2*m22          */

    PFMUL     ( MM1, MM6 )		/* x3*m31          | x3*m30          */
    PFACC     ( MM7, MM5 )		/* x3              | x2*m22+x3*m32   */

    PFADD     ( MM6, MM4 )		/* x1*m11+x3*m31   | x0*m00+x3*m30   */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    MOVQ      ( MM4, REGOFF(-16, EDX) )	/* write r0, r1                      */
    MOVQ      ( MM5, REGOFF(-8, EDX) )	/* write r2, r3                      */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TP3NRR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP3NRR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_2d )
HIDDEN(_mesa_3dnow_transform_points4_2d)
GLNAME( _mesa_3dnow_transform_points4_2d ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TP2R_2 ) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(16, ECX), MM0 )	/* m10             | m00             */

    MOVD      ( REGOFF(4, ECX), MM1 )	/*                 | m01             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM1 )	/* m11             | m01             */

    MOVQ      ( REGOFF(48, ECX), MM2 )	/* m31             | m30             */

ALIGNTEXT16
LLBL( G3TP2R_1 ):

    PREFETCHW ( REGOFF(32, EDX) )       /* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM3 )	/* x1              | x0              */
    MOVQ      ( REGOFF(8, EAX), MM5 )	/* x3              | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    MOVQ      ( MM3, MM4 )		/* x1              | x0              */
    MOVQ      ( MM5, MM6 )		/* x3              | x2              */

    PFMUL     ( MM1, MM4 )		/* x1*m11          | x0*m01          */
    PUNPCKHDQ ( MM6, MM6 )		/* x3              | x3              */

    PFMUL     ( MM0, MM3 )		/* x1*m10          | x0*m00          */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    PFACC     ( MM4, MM3 )		/* x0*m01+x1*m11   | x0*m00+x1*m10   */
    PFMUL     ( MM2, MM6 )		/* x3*m31          | x3*m30          */

    PFADD     ( MM6, MM3 )		/* r1              | r0              */
    MOVQ      ( MM5, REGOFF(-8, EDX) )	/* write r2, r3                      */

    MOVQ      ( MM3, REGOFF(-16, EDX) )	/* write r0, r1                      */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TP2R_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP2R_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_2d_no_rot )
HIDDEN(_mesa_3dnow_transform_points4_2d_no_rot)
GLNAME( _mesa_3dnow_transform_points4_2d_no_rot ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TP2NRR_3 ) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM0 )	/* m11             | m00             */

    MOVQ      ( REGOFF(48, ECX), MM1 )	/* m31             | m30             */

ALIGNTEXT16
LLBL( G3TP2NRR_2 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM4 )	/* x1              | x0              */
    MOVQ      ( REGOFF(8, EAX), MM5 )	/* x3              | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    PFMUL     ( MM0, MM4 )		/* x1*m11          | x0*m00          */
    MOVQ      ( MM5, MM6 )		/* x3              | x2              */

    ADD_L     ( CONST(16), EDX )	/* next r                            */
    PUNPCKHDQ ( MM6, MM6 )		/* x3              | x3              */

    PFMUL     ( MM1, MM6 )		/* x3*m31          | x3*m30          */
    PFADD     ( MM4, MM6 )		/* x1*m11+x3*m31   | x0*m00+x3*m30   */

    MOVQ      ( MM6, REGOFF(-16, EDX) )	/* write r0, r1                      */
    MOVQ      ( MM5, REGOFF(-8, EDX) )	/* write r2, r3                      */

    DEC_L     ( ESI )			/* decrement vertex counter          */

    JNZ       ( LLBL( G3TP2NRR_2 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP2NRR_3 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points4_identity )
HIDDEN(_mesa_3dnow_transform_points4_identity)
GLNAME( _mesa_3dnow_transform_points4_identity ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(4), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_4), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TPIR_2 ) )

ALIGNTEXT16
LLBL( G3TPIR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )       /* prefetch 2 vertices ahead         */
	
    MOVQ      ( REGIND(EAX), MM0 )	/* x1              | x0              */
    MOVQ      ( REGOFF(8, EAX), MM1 )	/* x3              | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    ADD_L     ( CONST(16), EDX )	/* next r                            */
    MOVQ      ( MM0, REGOFF(-16, EDX) )	/* r1              | r0              */

    MOVQ      ( MM1, REGOFF(-8, EDX) )	/* r3              | r2              */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TPIR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TPIR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET
#endif
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
