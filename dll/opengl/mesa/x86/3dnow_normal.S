
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 * 3Dnow assembly code by Holger Waechtler
 */

#ifdef USE_3DNOW_ASM

#include "assyntax.h"
#include "matypes.h"
#include "norm_args.h"

        SEG_TEXT

#define M(i)    REGOFF(i * 4, ECX)
#define STRIDE  REGOFF(12, ESI)


ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_transform_normalize_normals)
HIDDEN(_mesa_3dnow_transform_normalize_normals)
GLNAME(_mesa_3dnow_transform_normalize_normals):

#define FRAME_OFFSET 12

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )
    PUSH_L     ( EBP )

    MOV_L      ( ARG_LENGTHS, EDI )
    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EBP ) /*  dest->count = in->count   */
    MOV_L      ( EBP, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( REGOFF(V4F_START, ESI), EDX ) /*  in->start    */
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( ARG_MAT, ECX )
    MOV_L      ( REGOFF(MATRIX_INV, ECX), ECX ) /*  mat->inv     */

    CMP_L      ( CONST(0), EBP )        /*   count > 0 ??  */
    JE         ( LLBL (G3TN_end) )

    MOV_L      ( REGOFF (V4F_COUNT, ESI), EBP )
    FEMMS

    PUSH_L     ( EBP )
    PUSH_L     ( EAX )
    PUSH_L     ( EDX )                  /*  save counter & pointer for   */
                                        /*  the normalize pass           */
#undef  FRAME_OFFSET
#define FRAME_OFFSET 24

    MOVQ       ( M(0), MM3 )            /*  m1              | m0         */
    MOVQ       ( M(4), MM4 )            /*  m5              | m4         */

    MOVD       ( M(2), MM5 )            /*                  | m2         */
    PUNPCKLDQ  ( M(6), MM5 )            /*  m6              | m2         */

    MOVQ       ( M(8), MM6 )            /*  m9              | m8         */
    MOVQ       ( M(10), MM7 )           /*                  | m10        */

    CMP_L      ( CONST(0), EDI )        /*  lengths == 0 ?                 */
    JNE        ( LLBL (G3TN_scale_end ) )

    MOVD       ( ARG_SCALE, MM0 )       /*               | scale           */
    PUNPCKLDQ  ( MM0, MM0 )             /* scale         | scale           */

    PFMUL      ( MM0, MM3 )             /* scale * m1    | scale * m0      */
    PFMUL      ( MM0, MM4 )             /* scale * m5    | scale * m4      */
    PFMUL      ( MM0, MM5 )             /* scale * m6    | scale * m2      */
    PFMUL      ( MM0, MM6 )             /* scale * m9    | scale * m8      */
    PFMUL      ( MM0, MM7 )             /*               | scale * m10     */

ALIGNTEXT32
LLBL (G3TN_scale_end):
LLBL (G3TN_transform):
    MOVQ       ( REGIND (EDX), MM0 )    /*  x1              | x0         */
    MOVD       ( REGOFF (8, EDX), MM2 ) /*                  | x2         */

    MOVQ       ( MM0, MM1 )             /*  x1              | x0           */
    PUNPCKLDQ  ( MM2, MM2 )             /*  x2              | x2           */

    PFMUL      ( MM3, MM0 )             /*  x1*m1           | x0*m0        */
    ADD_L      ( CONST(16), EAX )       /*  next r                         */

    PREFETCHW  ( REGIND(EAX) )

    PFMUL      ( MM4, MM1 )             /*  x1*m5           | x0*m4        */
    PFACC      ( MM1, MM0 )             /*  x0*m4+x1*m5     | x0*m0+x1*m1  */

    PFMUL      ( MM5, MM2 )             /*  x2*m6           | x2*m2        */
    PFADD      ( MM2, MM0 )             /* x0*m4+x1*m5+x2*m6| x0*m0+...+x2**/

    MOVQ       ( REGIND (EDX), MM1 )    /*  x1           | x0              */
    MOVQ       ( MM0, REGOFF(-16, EAX) ) /* write r0, r1                   */

    PFMUL      ( MM6, MM1 )             /* x1*m9         | x0*m8           */
    MOVD       ( REGOFF (8, EDX), MM2 ) /*               | x2              */

    PFMUL      ( MM7, MM2 )             /*               | x2*m10          */
    PFACC      ( MM1, MM1 )             /*  *not used*   | x0*m8+x1*m9     */

    PFADD      ( MM2, MM1 )             /*  *not used*   | x0*m8+x1*m9+x2*m*/
    ADD_L      ( STRIDE, EDX )          /*  next normal                    */

    PREFETCH   ( REGIND(EDX) )

    MOVD       ( MM1, REGOFF(-8, EAX) ) /*  write r2                       */
    SUB_L      ( CONST(1), EBP )                  /*  decrement normal counter       */
    JNZ        ( LLBL (G3TN_transform) )


    POP_L      ( EDX )                  /*  end of transform ---           */
    POP_L      ( EAX )                  /*    now normalizing ...          */
    POP_L      ( EBP )

    CMP_L      ( CONST(0), EDI )        /*  lengths == 0 ?                 */
    JE         ( LLBL (G3TN_norm ) )    /*  calculate lengths              */


ALIGNTEXT32
LLBL (G3TN_norm_w_lengths):

    PREFETCHW  ( REGOFF(12,EAX) )

    MOVQ       ( REGIND(EAX), MM0 )     /*  x1              | x0           */
    MOVD       ( REGOFF(8, EAX), MM1 )  /*                  | x2           */

    MOVD       ( REGIND (EDI), MM3 )    /*                  | length (x)   */
    PFMUL      ( MM3, MM1 )             /*                  | x2 (normalize*/

    PUNPCKLDQ  ( MM3, MM3 )             /*  length (x)      | length (x)   */
    PFMUL      ( MM3, MM0 )             /*  x1 (normalized) | x0 (normalize*/

    ADD_L      ( STRIDE, EDX )          /*  next normal                    */
    ADD_L      ( CONST(4), EDI )        /*  next length                    */

    PREFETCH   ( REGIND(EDI) )

    MOVQ       ( MM0, REGIND(EAX) )     /*  write new x0, x1               */
    MOVD       ( MM1, REGOFF(8, EAX) )  /*  write new x2                   */

    ADD_L      ( CONST(16), EAX )       /*  next r                         */
    SUB_L      ( CONST(1), EBP )        /*  decrement normal counter       */

    JNZ        ( LLBL (G3TN_norm_w_lengths) )
    JMP        ( LLBL (G3TN_exit_3dnow) )

ALIGNTEXT32
LLBL (G3TN_norm):

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND (EAX), MM0 )    /*  x1             | x0           */
    MOVD       ( REGOFF(8, EAX), MM1 )  /*                 | x2           */

    MOVQ       ( MM0, MM3 )             /*  x1              | x0           */
    MOVQ       ( MM1, MM4 )             /*                  | x2           */

    PFMUL      ( MM0, MM3 )             /*  x1*x1           | x0*x0        */
    ADD_L      ( CONST(16), EAX )       /*  next r                         */

    PFMUL      ( MM1, MM4 )             /*                  | x2*x2        */
    PFADD      ( MM4, MM3 )             /*                  | x0*x0+x2*x2  */

    PFACC      ( MM3, MM3 )             /* **not used**    | x0*x0+x1*x1+x2**/
    PFRSQRT    ( MM3, MM5 )             /*  1/sqrt (x0*x0+x1*x1+x2*x2)     */

    MOVQ       ( MM5, MM4 )
    PUNPCKLDQ  ( MM3, MM3 )

    SUB_L      ( CONST(1), EBP )                  /*  decrement normal counter       */
    PFMUL      ( MM5, MM5 )

    PFRSQIT1   ( MM3, MM5 )
    PFRCPIT2   ( MM4, MM5 )

    PFMUL      ( MM5, MM0 )             /*  x1 (normalized) | x0 (normalize*/

    MOVQ       ( MM0, REGOFF(-16, EAX) ) /*  write new x0, x1              */
    PFMUL      ( MM5, MM1 )             /*                 | x2 (normalize*/

    MOVD       ( MM1, REGOFF(-8, EAX) ) /*  write new x2                  */
    JNZ        ( LLBL (G3TN_norm) )

LLBL (G3TN_exit_3dnow):
    FEMMS

LLBL (G3TN_end):
    POP_L      ( EBP )
    POP_L      ( ESI )
    POP_L      ( EDI )
    RET



ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_transform_normalize_normals_no_rot)
HIDDEN(_mesa_3dnow_transform_normalize_normals_no_rot)
GLNAME(_mesa_3dnow_transform_normalize_normals_no_rot):

#undef FRAME_OFFSET
#define FRAME_OFFSET 12

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )
    PUSH_L     ( EBP )

    MOV_L      ( ARG_LENGTHS, EDI )
    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EBP ) /*  dest->count = in->count   */
    MOV_L      ( EBP, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( ARG_MAT, ECX )
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(MATRIX_INV, ECX), ECX ) /*  mat->inv     */
    MOV_L      ( REGOFF(V4F_START, ESI), EDX ) /*  in->start    */

    CMP_L      ( CONST(0), EBP ) /*   count > 0 ??  */
    JE         ( LLBL (G3TNNR_end) )

    FEMMS

    MOVD       ( M(0), MM0 )            /*               | m0                 */
    PUNPCKLDQ  ( M(5), MM0 )            /* m5            | m0                 */

    MOVD       ( M(10), MM2 )           /*               | m10                */
    PUNPCKLDQ  ( MM2, MM2 )             /* m10           | m10                */

    CMP_L      ( CONST(0), EDI )        /*  lengths == 0 ?                    */
    JNE        ( LLBL (G3TNNR_scale_end ) )

    MOVD       ( ARG_SCALE, MM7 )       /*               | scale              */
    PUNPCKLDQ  ( MM7, MM7 )             /* scale         | scale              */

    PFMUL      ( MM7, MM0 )             /* scale * m5    | scale * m0         */
    PFMUL      ( MM7, MM2 )             /* scale * m10   | scale * m10        */

ALIGNTEXT32
LLBL (G3TNNR_scale_end):
    CMP_L      ( CONST(0), EDI )        /* lengths == 0 ?                     */
    JE         ( LLBL (G3TNNR_norm) )   /* need to calculate lengths          */

    MOVD       ( REGIND(EDI), MM3 )     /*                 | length (x)       */


ALIGNTEXT32
LLBL (G3TNNR_norm_w_lengths):           /* use precalculated lengths          */

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND(EDX), MM6 )     /* x1            | x0                 */
    MOVD       ( REGOFF(8, EDX), MM7 )  /*               | x2                 */

    PFMUL      ( MM0, MM6 )             /* x1*m5         | x0*m0              */
    ADD_L      ( STRIDE, EDX )          /* next normal                        */

    PREFETCH   ( REGIND(EDX) )

    PFMUL      ( MM2, MM7 )             /*               | x2*m10             */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    PFMUL      ( MM3, MM7 )             /*               | x2 (normalized)  */
    PUNPCKLDQ  ( MM3, MM3 )             /* length (x)    | length (x)       */

    ADD_L      ( CONST(4), EDI )        /* next length                        */
    PFMUL      ( MM3, MM6 )             /* x1 (normalized) | x0 (normalized)  */

    SUB_L      ( CONST(1), EBP )        /* decrement normal counter           */
    MOVQ       ( MM6, REGOFF(-16, EAX) ) /* write r0, r1                      */

    MOVD       ( MM7, REGOFF(-8, EAX) ) /* write r2                           */
    MOVD       ( REGIND(EDI), MM3 )     /*                 | length (x)       */

    JNZ        ( LLBL (G3TNNR_norm_w_lengths) )
    JMP        ( LLBL (G3TNNR_exit_3dnow) )

ALIGNTEXT32
LLBL (G3TNNR_norm):                     /* need to calculate lengths          */

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND(EDX), MM6 )     /* x1              | x0               */
    MOVD       ( REGOFF(8, EDX), MM7 )  /*                 | x2               */

    PFMUL      ( MM0, MM6 )             /* x1*m5           | x0*m0            */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    PFMUL      ( MM2, MM7 )             /*                 | x2*m10           */
    MOVQ       ( MM6, MM3 )             /* x1 (transformed)| x0 (transformed) */

    MOVQ       ( MM7, MM4 )             /*                 | x2 (transformed) */
    PFMUL      ( MM6, MM3 )             /* x1*x1           | x0*x0            */


    PFMUL      ( MM7, MM4 )             /*                 | x2*x2            */
    PFACC      ( MM3, MM3 )             /* **not used**    | x0*x0+x1*x1      */

    PFADD      ( MM4, MM3 )             /*                 | x0*x0+x1*x1+x2*x2*/
    ADD_L      ( STRIDE, EDX )          /* next normal            */

    PREFETCH   ( REGIND(EDX) )

    PFRSQRT    ( MM3, MM5 )             /* 1/sqrt (x0*x0+x1*x1+x2*x2)         */
    MOVQ       ( MM5, MM4 )

    PUNPCKLDQ  ( MM3, MM3 )
    PFMUL      ( MM5, MM5 )

    PFRSQIT1   ( MM3, MM5 )
    SUB_L      ( CONST(1), EBP )        /* decrement normal counter           */

    PFRCPIT2   ( MM4, MM5 )
    PFMUL      ( MM5, MM6 )             /* x1 (normalized) | x0 (normalized)  */

    MOVQ       ( MM6, REGOFF(-16, EAX) ) /* write r0, r1                      */
    PFMUL      ( MM5, MM7 )             /*                 | x2 (normalized)  */

    MOVD       ( MM7, REGOFF(-8, EAX) ) /* write r2                           */
    JNZ        ( LLBL (G3TNNR_norm) )


LLBL (G3TNNR_exit_3dnow):
    FEMMS

LLBL (G3TNNR_end):
    POP_L      ( EBP )
    POP_L      ( ESI )
    POP_L      ( EDI )
    RET






ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_transform_rescale_normals_no_rot)
HIDDEN(_mesa_3dnow_transform_rescale_normals_no_rot)
GLNAME(_mesa_3dnow_transform_rescale_normals_no_rot):

#undef FRAME_OFFSET
#define FRAME_OFFSET 12

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )
    PUSH_L     ( EBP )

    MOV_L      ( ARG_IN, EAX )
    MOV_L      ( ARG_DEST, EDX )
    MOV_L      ( REGOFF(V4F_COUNT, EAX), EBP ) /*  dest->count = in->count   */
    MOV_L      ( EBP, REGOFF(V4F_COUNT, EDX) )
    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_MAT, ECX )
    MOV_L      ( REGOFF(MATRIX_INV, ECX), ECX ) /*  mat->inv     */
    MOV_L      ( REGOFF(V4F_START, EDX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(V4F_START, ESI), EDX ) /*  in->start    */

    CMP_L      ( CONST(0), EBP )
    JE         ( LLBL (G3TRNR_end) )

    FEMMS

    MOVD       ( ARG_SCALE, MM6 )       /*               | scale              */
    PUNPCKLDQ  ( MM6, MM6 )             /* scale         | scale              */

    MOVD       ( REGIND(ECX), MM0 )     /*               | m0                 */
    PUNPCKLDQ  ( REGOFF(20, ECX), MM0 ) /* m5            | m0                 */

    PFMUL      ( MM6, MM0 )             /* scale*m5      | scale*m0           */
    MOVD       ( REGOFF(40, ECX), MM2 ) /*               | m10                */

    PFMUL      ( MM6, MM2 )             /*               | scale*m10          */

ALIGNTEXT32
LLBL (G3TRNR_rescale):

    PREFETCHW  ( REGIND(EAX) )
	
    MOVQ       ( REGIND(EDX), MM4 )     /* x1            | x0                 */
    MOVD       ( REGOFF(8, EDX), MM5 )  /*               | x2                 */
	
    PFMUL      ( MM0, MM4 )             /* x1*m5         | x0*m0              */
    ADD_L      ( STRIDE, EDX )          /* next normal                        */

    PREFETCH   ( REGIND(EDX) )

    PFMUL      ( MM2, MM5 )             /*               | x2*m10             */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    SUB_L      ( CONST(1), EBP )        /* decrement normal counter           */
    MOVQ       ( MM4, REGOFF(-16, EAX) ) /* write r0, r1                      */

    MOVD       ( MM5, REGOFF(-8, EAX) ) /* write r2                           */
    JNZ        ( LLBL (G3TRNR_rescale) ) /* cnt > 0 ? -> process next normal  */

    FEMMS

LLBL (G3TRNR_end):
    POP_L      ( EBP )
    POP_L      ( ESI )
    POP_L      ( EDI )
    RET





ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_transform_rescale_normals)
HIDDEN(_mesa_3dnow_transform_rescale_normals)
GLNAME(_mesa_3dnow_transform_rescale_normals):

#undef  FRAME_OFFSET
#define FRAME_OFFSET 8

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )

    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( ARG_MAT, ECX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EDI ) /*  dest->count = in->count   */
    MOV_L      ( EDI, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(V4F_START, ESI), EDX ) /*  in->start    */
    MOV_L      ( REGOFF(MATRIX_INV, ECX), ECX ) /*  mat->inv     */

    CMP_L      ( CONST(0), EDI )
    JE         ( LLBL (G3TR_end) )

    FEMMS

    MOVQ       ( REGIND(ECX), MM3 )     /* m1            | m0                 */

    MOVQ       ( REGOFF(16,ECX), MM4 )  /* m5            | m4                 */
    MOVD       ( ARG_SCALE, MM0 )       /* scale       */

    MOVD       ( REGOFF(8,ECX), MM5 )   /*               | m2                 */
    PUNPCKLDQ  ( MM0, MM0 )             /* scale         | scale              */

    PUNPCKLDQ  ( REGOFF(24, ECX), MM5 )
    PFMUL      ( MM0, MM3 )             /* scale*m1      | scale*m0           */

    MOVQ       ( REGOFF(32, ECX), MM6 ) /* m9            | m8*/
    PFMUL      ( MM0, MM4 )             /* scale*m5      | scale*m4           */

    MOVD       ( REGOFF(40, ECX), MM7 ) /*               | m10                */
    PFMUL      ( MM0, MM5 )             /* scale*m6      | scale*m2           */

    PFMUL      ( MM0, MM6 )             /* scale*m9      | scale*m8           */

    PFMUL      ( MM0, MM7 )             /*               | scale*m10          */

ALIGNTEXT32
LLBL (G3TR_rescale):

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND(EDX), MM0 )     /* x1            | x0                 */
    MOVD       ( REGOFF(8, EDX), MM2 )  /*               | x2                 */

    MOVQ       ( MM0, MM1 )             /* x1            | x0                 */
    PUNPCKLDQ  ( MM2, MM2 )             /* x2            | x2                 */

    PFMUL      ( MM3, MM0 )             /* x1*m1         | x0*m0              */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    PFMUL      ( MM4, MM1 )             /* x1*m5         | x0*m4              */
    PFACC      ( MM1, MM0 )             /* x0*m4+x1*m5   | x0*m0+x1*m1        */

    MOVQ       ( REGIND(EDX), MM1 )     /* x1            | x0                 */

    PFMUL      ( MM5, MM2 )             /* x2*m6         | x2*m2              */
    PFADD      ( MM2, MM0 )             /* x0*m4...+x2*m6| x0*m0+x1*m1+x2*m2  */

    MOVD       ( REGOFF(8, EDX), MM2 )  /*               | x2                 */
    ADD_L      ( STRIDE, EDX )          /* next normal                    */

    PREFETCH   ( REGIND(EDX) )

    MOVQ       ( MM0, REGOFF(-16, EAX) ) /* write r0, r1                      */
    PFMUL      ( MM6, MM1 )             /* x1*m9         | x0*m8              */

    PFMUL      ( MM7, MM2 )             /*               | x2*m10             */
    PFACC      ( MM1, MM1 )             /* *not used*    | x0*m8+x1*m9        */

    PFADD      ( MM2, MM1 )             /* *not used*    | x0*m8+x1*m9+x2*m10 */
    MOVD       ( MM1, REGOFF(-8, EAX) ) /* write r2                           */

    SUB_L      ( CONST(1), EDI )        /* decrement normal counter           */
    JNZ        ( LLBL (G3TR_rescale) )

    FEMMS

LLBL (G3TR_end):
    POP_L       ( ESI )
    POP_L       ( EDI )
    RET







ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_transform_normals_no_rot)
HIDDEN(_mesa_3dnow_transform_normals_no_rot)
GLNAME(_mesa_3dnow_transform_normals_no_rot):

#undef  FRAME_OFFSET
#define FRAME_OFFSET 8

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )

    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( ARG_MAT, ECX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EDI ) /*  dest->count = in->count   */
    MOV_L      ( EDI, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(V4F_START, ESI), EDX ) /*  in->start    */
    MOV_L      ( REGOFF(MATRIX_INV, ECX), ECX ) /*  mat->inv     */

    CMP_L      ( CONST(0), EDI )
    JE         ( LLBL (G3TNR_end) )

    FEMMS

    MOVD       ( REGIND(ECX), MM0 )     /*               | m0                 */
    PUNPCKLDQ  ( REGOFF(20, ECX), MM0 ) /* m5            | m0                 */

    MOVD       ( REGOFF(40, ECX), MM2 ) /*               | m10                */
    PUNPCKLDQ  ( MM2, MM2 )             /* m10           | m10                */

ALIGNTEXT32
LLBL (G3TNR_transform):

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND(EDX), MM4 )     /* x1            | x0                 */
    MOVD       ( REGOFF(8, EDX), MM5 )  /*               | x2                 */

    PFMUL      ( MM0, MM4 )             /* x1*m5         | x0*m0              */
    ADD_L      ( STRIDE, EDX)           /* next normal      */

    PREFETCH   ( REGIND(EDX) )

    PFMUL      ( MM2, MM5 )             /*               | x2*m10             */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    SUB_L      ( CONST(1), EDI )        /* decrement normal counter           */
    MOVQ       ( MM4, REGOFF(-16, EAX) ) /* write r0, r1                      */

    MOVD       ( MM5, REGOFF(-8, EAX) ) /* write r2                           */
    JNZ        ( LLBL (G3TNR_transform) )

    FEMMS

LLBL (G3TNR_end):
    POP_L       ( ESI )
    POP_L       ( EDI )
    RET








ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_transform_normals)
HIDDEN(_mesa_3dnow_transform_normals)
GLNAME(_mesa_3dnow_transform_normals):

#undef  FRAME_OFFSET
#define FRAME_OFFSET 8

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )

    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( ARG_MAT, ECX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EDI ) /*  dest->count = in->count   */
    MOV_L      ( EDI, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(V4F_START, ESI), EDX ) /*  in->start    */
    MOV_L      ( REGOFF(MATRIX_INV, ECX), ECX ) /*  mat->inv     */

    CMP_L      ( CONST(0), EDI )        /* count > 0 ??                       */
    JE         ( LLBL (G3T_end) )

    FEMMS

    MOVQ       ( REGIND(ECX), MM3 )     /* m1            | m0                 */
    MOVQ       ( REGOFF(16, ECX), MM4 ) /* m5            | m4                 */

    MOVD       ( REGOFF(8, ECX), MM5 )  /*               | m2                 */
    PUNPCKLDQ  ( REGOFF(24, ECX), MM5 ) /* m6            | m2                 */

    MOVQ       ( REGOFF(32, ECX), MM6 ) /* m9            | m8                 */
    MOVD       ( REGOFF(40, ECX), MM7 ) /*               | m10                */

ALIGNTEXT32
LLBL (G3T_transform):

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND(EDX), MM0 )     /* x1            | x0                 */
    MOVD       ( REGOFF(8, EDX), MM2 )  /*               | x2                 */

    MOVQ       ( MM0, MM1 )             /* x1            | x0                 */
    PUNPCKLDQ  ( MM2, MM2 )             /* x2            | x2                 */

    PFMUL      ( MM3, MM0 )             /* x1*m1         | x0*m0              */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    PFMUL      ( MM4, MM1 )             /* x1*m5         | x0*m4              */
    PFACC      ( MM1, MM0 )             /* x0*m4+x1*m5   | x0*m0+x1*m1        */

    PFMUL      ( MM5, MM2 )             /* x2*m6         | x2*m2              */
    PFADD      ( MM2, MM0 )             /* x0*m4...+x2*m6| x0*m0+x1*m1+x2*m2  */

    MOVQ       ( REGIND(EDX), MM1 )     /* x1            | x0                 */
    MOVQ       ( MM0, REGOFF(-16, EAX) ) /* write r0, r1                      */

    PFMUL      ( MM6, MM1 )             /* x1*m9         | x0*m8              */
    MOVD       ( REGOFF(8, EDX), MM2 )  /*               | x2                 */

    PFMUL      ( MM7, MM2 )             /*               | x2*m10             */
    ADD_L      ( STRIDE, EDX )          /* next normal               */

    PREFETCH   ( REGIND(EDX) )

    PFACC      ( MM1, MM1 )             /* *not used*    | x0*m8+x1*m9        */
    PFADD      ( MM2, MM1 )             /* *not used*    | x0*m8+x1*m9+x2*m10 */

    MOVD       ( MM1, REGOFF(-8, EAX) ) /* write r2                           */
    SUB_L      ( CONST(1), EDI )        /* decrement normal counter           */

    JNZ        ( LLBL (G3T_transform) )

    FEMMS

LLBL (G3T_end):
    POP_L  ( ESI )
    POP_L  ( EDI )
    RET






ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_normalize_normals)
HIDDEN(_mesa_3dnow_normalize_normals)
GLNAME(_mesa_3dnow_normalize_normals):

#undef  FRAME_OFFSET
#define FRAME_OFFSET 12

    PUSH_L     ( EDI )
    PUSH_L     ( ESI )
    PUSH_L     ( EBP )

    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EBP ) /*  dest->count = in->count   */
    MOV_L      ( EBP, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(V4F_START, ESI), ECX ) /*  in->start    */
    MOV_L      ( ARG_LENGTHS, EDX )

    CMP_L      ( CONST(0), EBP ) /* count > 0 ?? */
    JE         ( LLBL (G3N_end) )

    FEMMS

    CMP_L      ( CONST(0), EDX )        /* lengths == 0 ?                     */
    JE         ( LLBL (G3N_norm2) )     /* calculate lengths                  */

ALIGNTEXT32
LLBL (G3N_norm1):                       /* use precalculated lengths          */

    PREFETCH   ( REGIND(EAX) )

    MOVQ       ( REGIND(ECX), MM0 )     /* x1              | x0               */
    MOVD       ( REGOFF(8, ECX), MM1 )  /*                 | x2               */

    MOVD       ( REGIND(EDX), MM3 )     /*                 | length (x)       */
    PFMUL      ( MM3, MM1 )             /*                 | x2 (normalized)  */

    PUNPCKLDQ  ( MM3, MM3 )             /* length (x)      | length (x)       */
    ADD_L      ( STRIDE, ECX )          /* next normal            */

    PREFETCH   ( REGIND(ECX) )

    PFMUL      ( MM3, MM0 )             /* x1 (normalized) | x0 (normalized)  */
    MOVQ       ( MM0, REGIND(EAX) )     /* write new x0, x1                   */

    MOVD       ( MM1, REGOFF(8, EAX) )  /* write new x2                       */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    ADD_L      ( CONST(4), EDX )        /* next length                        */
    SUB_L      ( CONST(1), EBP )        /* decrement normal counter           */

    JNZ        ( LLBL (G3N_norm1) )

    JMP        ( LLBL (G3N_end1) )

ALIGNTEXT32
LLBL (G3N_norm2):                       /* need to calculate lengths          */

    PREFETCHW  ( REGIND(EAX) )

    PREFETCH   ( REGIND(ECX) )

    MOVQ       ( REGIND(ECX), MM0 )     /* x1              | x0               */
    MOVD       ( REGOFF(8, ECX), MM1 )  /*                 | x2               */

    MOVQ       ( MM0, MM3 )             /* x1              | x0               */
    ADD_L      ( STRIDE, ECX )          /* next normal    */

    PFMUL      ( MM0, MM3 )             /* x1*x1           | x0*x0            */
    MOVQ       ( MM1, MM4 )             /*                 | x2               */

    ADD_L      ( CONST(16), EAX )       /* next r                             */
    PFMUL      ( MM1, MM4 )             /*                 | x2*x2            */

    PFADD      ( MM4, MM3 )             /*                 | x0*x0+x2*x2      */
    PFACC      ( MM3, MM3 )             /* x0*x0+...+x2*x2 | x0*x0+x1*x1+x2*x2*/

    PFRSQRT    ( MM3, MM5 )             /* 1/sqrt (x0*x0+x1*x1+x2*x2)         */
    MOVQ       ( MM5, MM4 )

    PUNPCKLDQ  ( MM3, MM3 )
    PFMUL      ( MM5, MM5 )

    PFRSQIT1   ( MM3, MM5 )
    SUB_L      ( CONST(1), EBP )        /* decrement normal counter           */

    PFRCPIT2   ( MM4, MM5 )

    PFMUL      ( MM5, MM0 )             /* x1 (normalized) | x0 (normalized)  */
    MOVQ       ( MM0, REGOFF(-16, EAX) ) /* write new x0, x1                  */

    PFMUL      ( MM5, MM1 )             /*                 | x2 (normalized)  */
    MOVD       ( MM1, REGOFF(-8, EAX) ) /* write new x2                       */

    JNZ        ( LLBL (G3N_norm2) )

LLBL (G3N_end1):
    FEMMS

LLBL (G3N_end):
    POP_L      ( EBP )
    POP_L      ( ESI )
    POP_L      ( EDI )
    RET






ALIGNTEXT16
GLOBL GLNAME(_mesa_3dnow_rescale_normals)
HIDDEN(_mesa_3dnow_rescale_normals)
GLNAME(_mesa_3dnow_rescale_normals):

#undef  FRAME_OFFSET
#define FRAME_OFFSET 8
    PUSH_L     ( EDI )
    PUSH_L     ( ESI )

    MOV_L      ( ARG_IN, ESI )
    MOV_L      ( ARG_DEST, EAX )
    MOV_L      ( REGOFF(V4F_COUNT, ESI), EDX ) /*  dest->count = in->count   */
    MOV_L      ( EDX, REGOFF(V4F_COUNT, EAX) )
    MOV_L      ( REGOFF(V4F_START, EAX), EAX ) /*  dest->start  */
    MOV_L      ( REGOFF(V4F_START, ESI), ECX ) /*  in->start    */

    CMP_L      ( CONST(0), EDX )
    JE         ( LLBL (G3R_end) )

    FEMMS

    MOVD       ( ARG_SCALE, MM0 )       /* scale                              */
    PUNPCKLDQ  ( MM0, MM0 )

ALIGNTEXT32
LLBL (G3R_rescale):

    PREFETCHW  ( REGIND(EAX) )

    MOVQ       ( REGIND(ECX), MM1 )     /* x1            | x0                 */
    MOVD       ( REGOFF(8, ECX), MM2 )  /*               | x2                 */

    PFMUL      ( MM0, MM1 )             /* x1*scale      | x0*scale           */
    ADD_L      ( STRIDE, ECX )          /* next normal                  */

    PREFETCH   ( REGIND(ECX) )

    PFMUL      ( MM0, MM2 )             /*               | x2*scale           */
    ADD_L      ( CONST(16), EAX )       /* next r                             */

    MOVQ       ( MM1, REGOFF(-16, EAX) ) /* write r0, r1                      */
    MOVD       ( MM2, REGOFF(-8, EAX) ) /* write r2                           */

    SUB_L      ( CONST(1), EDX )        /* decrement normal counter           */
    JNZ        ( LLBL (G3R_rescale) )

    FEMMS

LLBL (G3R_end):
    POP_L      ( ESI )
    POP_L      ( EDI )
    RET

#endif
	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
