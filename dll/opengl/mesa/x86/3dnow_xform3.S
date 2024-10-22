
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
#include "assyntax.h"
#include "matypes.h"
#include "xform_args.h"

    SEG_TEXT

#define FRAME_OFFSET	4


ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_general )
HIDDEN(_mesa_3dnow_transform_points3_general)
GLNAME( _mesa_3dnow_transform_points3_general ):

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

    MOVQ      ( REGIND(EAX), MM0 )	/* x1              | x0              */
    MOVD      ( REGOFF(8, EAX), MM2 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    MOVQ      ( MM0, MM1 )		/* x1              | x0              */
    PUNPCKLDQ ( MM2, MM2 )		/* x2              | x2              */

    PUNPCKLDQ ( MM0, MM0 )		/* x0              | x0              */
    MOVQ      ( MM2, MM5 )		/* x2              | x2              */

    PUNPCKHDQ ( MM1, MM1 )		/* x1              | x1              */
    PFMUL     ( REGOFF(32, ECX), MM2 )	/* x2*m9           | x2*m8           */

    MOVQ      ( MM0, MM3 )		/* x0              | x0              */
    PFMUL     ( REGOFF(40, ECX), MM5 )	/* x2*m11          | x2*m10          */

    MOVQ      ( MM1, MM4 )		/* x1              | x1              */
    PFMUL     ( REGIND(ECX), MM0 )	/* x0*m1           | x0*m0           */

    PFADD     ( REGOFF(48, ECX), MM2 )	/* x2*m9+m13       | x2*m8+m12       */
    PFMUL     ( REGOFF(16, ECX), MM1 )	/* x1*m5           | x1*m4           */

    PFADD     ( REGOFF(56, ECX), MM5 )	/* x2*m11+m15      | x2*m10+m14      */
    PFADD     ( MM0, MM1 )		/* x0*m1+x1*m5     | x0*m0+x1*m4     */

    PFMUL     ( REGOFF(8, ECX), MM3 )	/* x0*m3           | x0*m2           */
    PFADD     ( MM1, MM2 )		/* r1              | r0              */

    PFMUL     ( REGOFF(24, ECX), MM4 )	/* x1*m7           | x1*m6           */
    ADD_L     ( CONST(16), EDX )	/* next output vertex                */

    PFADD     ( MM3, MM4 )		/* x0*m3+x1*m7     | x0*m2+x1*m6     */
    MOVQ      ( MM2, REGOFF(-16, EDX) )	/* write r0, r1                      */

    PFADD     ( MM4, MM5 )		/* r3              | r2              */
    MOVQ      ( MM5, REGOFF(-8, EDX) )	/* write r2, r3                      */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TPGR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TPGR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_perspective )
HIDDEN(_mesa_3dnow_transform_points3_perspective)
GLNAME( _mesa_3dnow_transform_points3_perspective ):

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

    MOVQ      ( REGOFF(32, ECX), MM1 )	/* m21             | m20             */
    MOVD      ( REGOFF(40, ECX), MM2 )	/*                 | m22             */

    MOVD      ( REGOFF(56, ECX), MM3 )	/*                 | m32             */

ALIGNTEXT16
LLBL( G3TPPR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVD      ( REGOFF(8, EAX), MM5 )	/*                 | x2              */
    MOVQ      ( REGIND(EAX), MM4 )	/* x1              | x0              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    PXOR      ( MM7, MM7 )		/* 0               | 0               */
    MOVQ      ( MM5, MM6 )		/*                 | x2              */

    PFMUL     ( MM0, MM4 )		/* x1*m11          | x0*m00          */
    PFSUB     ( MM5, MM7 )		/*                 | -x2             */

    PFMUL     ( MM2, MM6 )		/*                 | x2*m22          */
    PUNPCKLDQ ( MM5, MM5 )		/* x2              | x2              */

    ADD_L     ( CONST(16), EDX )	/* next r                            */
    PFMUL     ( MM1, MM5 )		/* x2*m21          | x2*m20          */

    PFADD     ( MM3, MM6 )		/*                 | x2*m22+m32      */
    PFADD     ( MM4, MM5 )		/* x1*m11+x2*m21   | x0*m00+x2*m20   */

    MOVQ      ( MM5, REGOFF(-16, EDX) )	/* write r0, r1                      */
    MOVD      ( MM6, REGOFF(-8, EDX) )	/* write r2                          */

    MOVD      ( MM7, REGOFF(-4, EDX) )	/* write r3                          */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TPPR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TPPR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_3d )
HIDDEN(_mesa_3dnow_transform_points3_3d)
GLNAME( _mesa_3dnow_transform_points3_3d ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(3), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, ECX) )
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

    PREFETCH  ( REGIND(EAX) )
    PREFETCH  ( REGIND(EDX) )

    MOVD      ( REGOFF(8, ECX), MM7 )	/*                 | m2              */
    PUNPCKLDQ ( REGOFF(24, ECX), MM7 )	/* m6              | m2              */


ALIGNTEXT16
LLBL( G3TP3R_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM0 )	/* x1              | x0              */
    MOVD      ( REGOFF(8, EAX), MM1 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    MOVQ      ( MM0, MM2 )		/* x1              | x0              */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    PUNPCKLDQ ( MM2, MM2 )		/* x0              | x0              */
    MOVQ      ( MM0, MM3 )		/* x1              | x0              */

    PFMUL     ( REGIND(ECX), MM2 )	/* x0*m1           | x0*m0           */
    PUNPCKHDQ ( MM3, MM3 )		/* x1              | x1              */

    MOVQ      ( MM1, MM4 )		/*                 | x2              */
    PFMUL     ( REGOFF(16, ECX), MM3 )	/* x1*m5           | x1*m4           */

    PUNPCKLDQ ( MM4, MM4 )		/* x2              | x2              */
    PFADD     ( MM2, MM3 )		/* x0*m1+x1*m5     | x0*m0+x1*m4     */

    PFMUL     ( REGOFF(32, ECX), MM4 )	/* x2*m9           | x2*m8           */
    PFADD     ( REGOFF(48, ECX), MM3 )	/* x0*m1+...+m11   | x0*m0+x1*m4+m12 */

    PFMUL     ( MM7, MM0 )		/* x1*m6           | x0*m2           */
    PFADD     ( MM4, MM3 )		/* r1              | r0              */

    PFMUL     ( REGOFF(40, ECX), MM1 )	/*                 | x2*m10          */
    PUNPCKLDQ ( REGOFF(56, ECX), MM1 )	/* m14             | x2*m10          */

    PFACC     ( MM0, MM1 )

    MOVQ      ( MM3, REGOFF(-16, EDX) )	/* write r0, r1                      */
    PFACC     ( MM1, MM1 )		/*                 | r2              */

    MOVD      ( MM1, REGOFF(-8, EDX) )	/* write r2                          */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TP3R_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP3R_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_3d_no_rot )
HIDDEN(_mesa_3dnow_transform_points3_3d_no_rot)
GLNAME( _mesa_3dnow_transform_points3_3d_no_rot ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(3), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, ECX) )
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

    PREFETCH  ( REGIND(EAX) )
    PREFETCHW ( REGIND(EDX) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM0 )	/* m11             | m00             */

    MOVD      ( REGOFF(40, ECX), MM2 )	/*                 | m22             */
    PUNPCKLDQ ( MM2, MM2 )		/* m22             | m22             */

    MOVQ      ( REGOFF(48, ECX), MM1 )	/* m31             | m30             */
    MOVD      ( REGOFF(56, ECX), MM3 )	/*                 | m32             */

    PUNPCKLDQ ( MM3, MM3 )		/* m32             | m32             */


ALIGNTEXT16
LLBL( G3TP3NRR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM4 )	/* x1              | x0              */
    MOVD      ( REGOFF(8, EAX), MM5 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCHW ( REGIND(EAX) )
	
    PFMUL     ( MM0, MM4 )		/* x1*m11          | x0*m00          */

    PFADD     ( MM1, MM4 )		/* x1*m11+m31      | x0*m00+m30      */
    PFMUL     ( MM2, MM5 )		/*                 | x2*m22          */

    PFADD     ( MM3, MM5 )		/*                 | x2*m22+m32      */
    MOVQ      ( MM4, REGIND(EDX) )	/* write r0, r1                      */

    ADD_L     ( CONST(16), EDX )	/* next r                            */
    DEC_L     ( ESI )			/* decrement vertex counter          */

    MOVD      ( MM5, REGOFF(-8, EDX) )	/* write r2                          */
    JNZ       ( LLBL( G3TP3NRR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP3NRR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_2d )
HIDDEN(_mesa_3dnow_transform_points3_2d)
GLNAME( _mesa_3dnow_transform_points3_2d ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(3), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TP2R_3) )

    PREFETCH  ( REGIND(EAX) )
    PREFETCHW ( REGIND(EDX) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(16, ECX), MM0 )	/* m10             | m00             */

    MOVD      ( REGOFF(4, ECX), MM1 )	/*                 | m01             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM1 )	/* m11             | m01             */

    MOVQ      ( REGOFF(48, ECX), MM2 )	/* m31             | m30             */

ALIGNTEXT16
LLBL( G3TP2R_2 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM3 )	/* x1              | x0              */
    MOVD      ( REGOFF(8, EAX), MM5 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    MOVQ      ( MM3, MM4 )		/* x1              | x0              */
    PFMUL     ( MM0, MM3 )		/* x1*m10          | x0*m00          */

    ADD_L     ( CONST(16), EDX )	/* next r                            */
    PFMUL     ( MM1, MM4 )		/* x1*m11          | x0*m01          */

    PFACC     ( MM4, MM3 )		/* x0*m00+x1*m10   | x0*m01+x1*m11   */
    MOVD      ( MM5, REGOFF(-8, EDX) )	/* write r2 (=x2)                    */

    PFADD     ( MM2, MM3 )		/* x0*...*m10+m30  | x0*...*m11+m31  */
    MOVQ      ( MM3, REGOFF(-16, EDX) )	/* write r0, r1                      */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TP2R_2 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP2R_3 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_2d_no_rot )
HIDDEN(_mesa_3dnow_transform_points3_2d_no_rot)
GLNAME( _mesa_3dnow_transform_points3_2d_no_rot ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(3), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, ECX) )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), EDX )
    MOV_L     ( EDX, REGOFF(V4F_COUNT, ECX) )

    PUSH_L    ( EDI )

    MOV_L     ( REGOFF(V4F_START, ECX), EDX )
    MOV_L     ( ESI, ECX )
    MOV_L     ( REGOFF(V4F_COUNT, EAX), ESI )
    MOV_L     ( REGOFF(V4F_STRIDE, EAX), EDI )
    MOV_L     ( REGOFF(V4F_START, EAX), EAX )

    TEST_L    ( ESI, ESI )
    JZ        ( LLBL( G3TP2NRR_2 ) )

    PREFETCH  ( REGIND(EAX) )
    PREFETCHW ( REGIND(EDX) )

    MOVD      ( REGIND(ECX), MM0 )	/*                 | m00             */
    PUNPCKLDQ ( REGOFF(20, ECX), MM0 )	/* m11             | m00             */

    MOVQ      ( REGOFF(48, ECX), MM1 )	/* m31             | m30             */


ALIGNTEXT16
LLBL( G3TP2NRR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )	/* prefetch 2 vertices ahead         */

    MOVQ      ( REGIND(EAX), MM4 )	/* x1              | x0              */
    MOVD      ( REGOFF(8, EAX), MM5 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    PREFETCH  ( REGIND(EAX) )

    PFMUL     ( MM0, MM4 )		/* x1*m11          | x0*m00          */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    PFADD     ( MM1, MM4 )		/* x1*m11+m31      | x0*m00+m30      */

    MOVQ      ( MM4, REGOFF(-16, EDX) )	/* write r0, r1                      */
    MOVD      ( MM5, REGOFF(-8, EDX) )	/* write r2 (=x2)                    */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    JNZ       ( LLBL( G3TP2NRR_1 ) )	/* cnt > 0 ? -> process next vertex  */

LLBL( G3TP2NRR_2 ):

    FEMMS
    POP_L     ( EDI )
    POP_L     ( ESI )
    RET




ALIGNTEXT16
GLOBL GLNAME( _mesa_3dnow_transform_points3_identity )
HIDDEN(_mesa_3dnow_transform_points3_identity)
GLNAME( _mesa_3dnow_transform_points3_identity ):

    PUSH_L    ( ESI )

    MOV_L     ( ARG_DEST, ECX )
    MOV_L     ( ARG_MATRIX, ESI )
    MOV_L     ( ARG_SOURCE, EAX )
    MOV_L     ( CONST(3), REGOFF(V4F_SIZE, ECX) )
    OR_B      ( CONST(VEC_SIZE_3), REGOFF(V4F_FLAGS, ECX) )
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

    PREFETCHW ( REGIND(EDX) )

ALIGNTEXT16
LLBL( G3TPIR_1 ):

    PREFETCHW ( REGOFF(32, EDX) )

    MOVQ      ( REGIND(EAX), MM0 )	/* x1              | x0              */
    MOVD      ( REGOFF(8, EAX), MM1 )	/*                 | x2              */

    ADD_L     ( EDI, EAX )		/* next vertex                       */
    ADD_L     ( CONST(16), EDX )	/* next r                            */

    DEC_L     ( ESI )			/* decrement vertex counter          */
    MOVQ      ( MM0, REGOFF(-16, EDX) )	/* r1              | r0              */

    MOVD      ( MM1, REGOFF(-8, EDX) )	/*                 | r2              */
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
