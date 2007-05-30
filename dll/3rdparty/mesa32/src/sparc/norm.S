/* $Id: norm.S,v 1.5 2005/07/28 00:11:11 idr Exp $ */

#include "sparc_matrix.h"

#if defined(SVR4) || defined(__SVR4) || defined(__svr4__)
	/* Solaris requires this for 64-bit. */
	.register %g2, #scratch
	.register %g3, #scratch
#endif

	.text

#ifdef __arch64__
#define STACK_VAR_OFF	(2047 + (8 * 16))
#else
#define STACK_VAR_OFF	(4 * 16)
#endif

	/* Newton-Raphson approximation turns out to be slower
	 * (and less accurate) than direct fsqrts/fdivs.
	 */
#define ONE_DOT_ZERO	0x3f800000

	.globl	_mesa_sparc_transform_normalize_normals
_mesa_sparc_transform_normalize_normals:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */

	sethi	%hi(ONE_DOT_ZERO), %g2
	sub	%sp, 16, %sp
	st	%g2, [%sp + STACK_VAR_OFF+0x0]
	st	%o1, [%sp + STACK_VAR_OFF+0x4]
	ld	[%sp + STACK_VAR_OFF+0x0], %f12	! f12 = 1.0f
	ld	[%sp + STACK_VAR_OFF+0x4], %f15	! f15 = scale
	add	%sp, 16, %sp

	LDPTR	[%o0 + MAT_INV], %o0		! o0 = mat->inv
	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	LDMATRIX_0_1_2_4_5_6_8_9_10(%o0)

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 cmp	%o3, 0
	bne	4f
	 clr	%o4				! 'i' for STRIDE_LOOP

1:	/* LENGTHS == NULL */
	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* tx (f3) = (ux * m0) + (uy * m1) + (uz * m2)
	 * ty (f5) = (ux * m4) + (uy * m5) + (uz * m6)
	 * tz (f7) = (ux * m8) + (uy * m9) + (uz * m10)
	 */
	fmuls	%f0, M0, %f3			! FGM	Group
	fmuls	%f1, M1, %f4			! FGM	Group
	fmuls	%f0, M4, %f5			! FGM	Group
	fmuls	%f1, M5, %f6			! FGM	Group
	fmuls	%f0, M8, %f7			! FGM	Group	f3 available
	fmuls	%f1, M9, %f8			! FGM	Group	f4 available
	fadds	%f3, %f4, %f3			! FGA
	fmuls	%f2, M2, %f10			! FGM	Group	f5 available
	fmuls	%f2, M6, %f0			! FGM	Group	f6 available
	fadds	%f5, %f6, %f5			! FGA
	fmuls	%f2, M10, %f4			! FGM	Group	f7 available
	fadds	%f7, %f8, %f7			! FGA	Group	f8,f3 available
	fadds	%f3, %f10, %f3			! FGA	Group	f10 available
	fadds	%f5, %f0, %f5			! FGA	Group	stall f0,f5 available
	fadds	%f7, %f4, %f7			! FGA	Group	stall f4,f7 available

	/* f3=tx, f5=ty, f7=tz */

	/* len (f6) = (tx * tx) + (ty * ty) + (tz * tz) */
	fmuls	%f3, %f3, %f6			! FGM	Group	f3 available
	fmuls	%f5, %f5, %f8			! FGM	Group	f5 available
	fmuls	%f7, %f7, %f10			! FGM	Group	f7 available
	fadds	%f6, %f8, %f6			! FGA	Group	2cyc stall f6,f8 available
	fadds	%f6, %f10, %f6			! FGA	Group	4cyc stall f6,f10 available

	/* scale (f6) = 1.0 / sqrt(len) */
	fsqrts	%f6, %f6			! FDIV  20 cycles
	fdivs	%f12, %f6, %f6			! FDIV	14 cycles

	fmuls	%f3, %f6, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * scale
	fmuls	%f5, %f6, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * scale
	fmuls	%f7, %f6, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * scale

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

	ba	7f
	 nop

4:	/* LENGTHS != NULL */
	fmuls	M0, %f15, M0
	fmuls	M1, %f15, M1
	fmuls	M2, %f15, M2
	fmuls	M4, %f15, M4
	fmuls	M5, %f15, M5
	fmuls	M6, %f15, M6
	fmuls	M8, %f15, M8
	fmuls	M9, %f15, M9
	fmuls	M10, %f15, M10

5:
	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* tx (f3) = (ux * m0) + (uy * m1) + (uz * m2)
	 * ty (f5) = (ux * m4) + (uy * m5) + (uz * m6)
	 * tz (f7) = (ux * m8) + (uy * m9) + (uz * m10)
	 */
	fmuls	%f0, M0, %f3			! FGM	Group
	fmuls	%f1, M1, %f4			! FGM	Group
	fmuls	%f0, M4, %f5			! FGM	Group
	fmuls	%f1, M5, %f6			! FGM	Group
	fmuls	%f0, M8, %f7			! FGM	Group	f3 available
	fmuls	%f1, M9, %f8			! FGM	Group	f4 available
	fadds	%f3, %f4, %f3			! FGA
	fmuls	%f2, M2, %f10			! FGM	Group	f5 available
	fmuls	%f2, M6, %f0			! FGM	Group	f6 available
	fadds	%f5, %f6, %f5			! FGA
	fmuls	%f2, M10, %f4			! FGM	Group	f7 available
	fadds	%f7, %f8, %f7			! FGA	Group	f8,f3 available
	fadds	%f3, %f10, %f3			! FGA	Group	f10 available
	ld	[%o3], %f13			! LSU
	fadds	%f5, %f0, %f5			! FGA	Group	stall f0,f5 available
	add	%o3, 4, %o3			! IEU0
	fadds	%f7, %f4, %f7			! FGA	Group	stall f4,f7 available

	/* f3=tx, f5=ty, f7=tz, f13=lengths[i] */

	fmuls	%f3, %f13, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * len
	fmuls	%f5, %f13, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * len
	fmuls	%f7, %f13, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * len

	cmp	%o4, %g1			! continue if (i < count)
	bl	5b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop
	
	.globl	_mesa_sparc_transform_normalize_normals_no_rot
_mesa_sparc_transform_normalize_normals_no_rot:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */

	sethi	%hi(ONE_DOT_ZERO), %g2
	sub	%sp, 16, %sp
	st	%g2, [%sp + STACK_VAR_OFF+0x0]
	st	%o1, [%sp + STACK_VAR_OFF+0x4]
	ld	[%sp + STACK_VAR_OFF+0x0], %f12	! f12 = 1.0f
	ld	[%sp + STACK_VAR_OFF+0x4], %f15	! f15 = scale
	add	%sp, 16, %sp

	LDPTR	[%o0 + MAT_INV], %o0		! o0 = mat->inv
	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	LDMATRIX_0_5_10(%o0)

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 cmp	%o3, 0
	bne	4f
	 clr	%o4				! 'i' for STRIDE_LOOP

1:	/* LENGTHS == NULL */
	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* tx (f3) = (ux * m0)
	 * ty (f5) = (uy * m5)
	 * tz (f7) = (uz * m10)
	 */
	fmuls	%f0, M0, %f3			! FGM	Group
	fmuls	%f1, M5, %f5			! FGM	Group
	fmuls	%f2, M10, %f7			! FGM	Group

	/* f3=tx, f5=ty, f7=tz */

	/* len (f6) = (tx * tx) + (ty * ty) + (tz * tz) */
	fmuls	%f3, %f3, %f6			! FGM	Group	stall, f3 available
	fmuls	%f5, %f5, %f8			! FGM	Group	f5 available
	fmuls	%f7, %f7, %f10			! FGM	Group	f7 available
	fadds	%f6, %f8, %f6			! FGA	Group	2cyc stall f6,f8 available
	fadds	%f6, %f10, %f6			! FGA	Group	4cyc stall f6,f10 available

	/* scale (f6) = 1.0 / sqrt(len) */
	fsqrts	%f6, %f6			! FDIV  20 cycles
	fdivs	%f12, %f6, %f6			! FDIV	14 cycles

	fmuls	%f3, %f6, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * scale
	fmuls	%f5, %f6, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * scale
	fmuls	%f7, %f6, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * scale

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

	ba	7f
	 nop

4:	/* LENGTHS != NULL */
	fmuls	M0, %f15, M0
	fmuls	M5, %f15, M5
	fmuls	M10, %f15, M10

5:
	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* tx (f3) = (ux * m0)
	 * ty (f5) = (uy * m5)
	 * tz (f7) = (uz * m10)
	 */
	fmuls	%f0, M0, %f3			! FGM	Group
	ld	[%o3], %f13			! LSU
	fmuls	%f1, M5, %f5			! FGM	Group
	add	%o3, 4, %o3			! IEU0
	fmuls	%f2, M10, %f7			! FGM	Group

	/* f3=tx, f5=ty, f7=tz, f13=lengths[i] */

	fmuls	%f3, %f13, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * len
	fmuls	%f5, %f13, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * len
	fmuls	%f7, %f13, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * len

	cmp	%o4, %g1			! continue if (i < count)
	bl	5b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop

	.globl	_mesa_sparc_transform_rescale_normals_no_rot
_mesa_sparc_transform_rescale_normals_no_rot:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */
	sub	%sp, 16, %sp
	st	%o1, [%sp + STACK_VAR_OFF+0x0]
	ld	[%sp + STACK_VAR_OFF+0x0], %f15	! f15 = scale
	add	%sp, 16, %sp

	LDPTR	[%o0 + MAT_INV], %o0		! o0 = mat->inv
	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	LDMATRIX_0_5_10(%o0)

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 clr	%o4				! 'i' for STRIDE_LOOP

	fmuls	M0, %f15, M0
	fmuls	M5, %f15, M5
	fmuls	M10, %f15, M10

1:	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* tx (f3) = (ux * m0)
	 * ty (f5) = (uy * m5)
	 * tz (f7) = (uz * m10)
	 */
	fmuls	%f0, M0, %f3			! FGM	Group
	st	%f3, [%g3 + 0x00]		! LSU
	fmuls	%f1, M5, %f5			! FGM	Group
	st	%f5, [%g3 + 0x04]		! LSU
	fmuls	%f2, M10, %f7			! FGM	Group
	st	%f7, [%g3 + 0x08]		! LSU

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop

	.globl	_mesa_sparc_transform_rescale_normals
_mesa_sparc_transform_rescale_normals:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */
	sub	%sp, 16, %sp
	st	%o1, [%sp + STACK_VAR_OFF+0x0]
	ld	[%sp + STACK_VAR_OFF+0x0], %f15	! f15 = scale
	add	%sp, 16, %sp

	LDPTR	[%o0 + MAT_INV], %o0		! o0 = mat->inv
	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	LDMATRIX_0_1_2_4_5_6_8_9_10(%o0)

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 clr	%o4				! 'i' for STRIDE_LOOP

	fmuls	M0, %f15, M0
	fmuls	M1, %f15, M1
	fmuls	M2, %f15, M2
	fmuls	M4, %f15, M4
	fmuls	M5, %f15, M5
	fmuls	M6, %f15, M6
	fmuls	M8, %f15, M8
	fmuls	M9, %f15, M9
	fmuls	M10, %f15, M10

1:	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	fmuls	%f0, M0, %f3			! FGM	Group
	fmuls	%f1, M1, %f4			! FGM	Group
	fmuls	%f0, M4, %f5			! FGM	Group
	fmuls	%f1, M5, %f6			! FGM	Group
	fmuls	%f0, M8, %f7			! FGM	Group	f3 available
	fmuls	%f1, M9, %f8			! FGM	Group	f4 available
	fadds	%f3, %f4, %f3			! FGA
	fmuls	%f2, M2, %f10			! FGM	Group	f5 available
	fmuls	%f2, M6, %f0			! FGM	Group	f6 available
	fadds	%f5, %f6, %f5			! FGA
	fmuls	%f2, M10, %f4			! FGM	Group	f7 available
	fadds	%f7, %f8, %f7			! FGA	Group	f8,f3 available
	fadds	%f3, %f10, %f3			! FGA	Group	f10 available
	st	%f3, [%g3 + 0x00]		! LSU
	fadds	%f5, %f0, %f5			! FGA	Group	stall f0,f5 available
	st	%f5, [%g3 + 0x04]		! LSU
	fadds	%f7, %f4, %f7			! FGA	Group	stall f4,f7 available
	st	%f7, [%g3 + 0x08]		! LSU

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop

	.globl	_mesa_sparc_transform_normals_no_rot
_mesa_sparc_transform_normals_no_rot:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */
	LDPTR	[%o0 + MAT_INV], %o0		! o0 = mat->inv
	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	LDMATRIX_0_5_10(%o0)

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 clr	%o4				! 'i' for STRIDE_LOOP

1:	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* tx (f3) = (ux * m0)
	 * ty (f5) = (uy * m5)
	 * tz (f7) = (uz * m10)
	 */
	fmuls	%f0, M0, %f3			! FGM	Group
	st	%f3, [%g3 + 0x00]		! LSU
	fmuls	%f1, M5, %f5			! FGM	Group
	st	%f5, [%g3 + 0x04]		! LSU
	fmuls	%f2, M10, %f7			! FGM	Group
	st	%f7, [%g3 + 0x08]		! LSU

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop

	.globl	_mesa_sparc_transform_normals
_mesa_sparc_transform_normals:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */
	LDPTR	[%o0 + MAT_INV], %o0		! o0 = mat->inv
	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	LDMATRIX_0_1_2_4_5_6_8_9_10(%o0)

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 clr	%o4				! 'i' for STRIDE_LOOP

1:	ld	[%o5 + 0x00], %f0		! ux = from[0]
	ld	[%o5 + 0x04], %f1		! uy = from[1]
	ld	[%o5 + 0x08], %f2		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	fmuls	%f0, M0, %f3			! FGM	Group
	fmuls	%f1, M1, %f4			! FGM	Group
	fmuls	%f0, M4, %f5			! FGM	Group
	fmuls	%f1, M5, %f6			! FGM	Group
	fmuls	%f0, M8, %f7			! FGM	Group	f3 available
	fmuls	%f1, M9, %f8			! FGM	Group	f4 available
	fadds	%f3, %f4, %f3			! FGA
	fmuls	%f2, M2, %f10			! FGM	Group	f5 available
	fmuls	%f2, M6, %f0			! FGM	Group	f6 available
	fadds	%f5, %f6, %f5			! FGA
	fmuls	%f2, M10, %f4			! FGM	Group	f7 available
	fadds	%f7, %f8, %f7			! FGA	Group	f8,f3 available
	fadds	%f3, %f10, %f3			! FGA	Group	f10 available
	st	%f3, [%g3 + 0x00]		! LSU
	fadds	%f5, %f0, %f5			! FGA	Group	stall f0,f5 available
	st	%f5, [%g3 + 0x04]		! LSU
	fadds	%f7, %f4, %f7			! FGA	Group	stall f4,f7 available
	st	%f7, [%g3 + 0x08]		! LSU

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop

	.globl	_mesa_sparc_normalize_normals
_mesa_sparc_normalize_normals:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */

	sethi	%hi(ONE_DOT_ZERO), %g2
	sub	%sp, 16, %sp
	st	%g2, [%sp + STACK_VAR_OFF+0x0]
	ld	[%sp + STACK_VAR_OFF+0x0], %f12	! f12 = 1.0f
	add	%sp, 16, %sp

	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 cmp	%o3, 0
	bne	4f
	 clr	%o4				! 'i' for STRIDE_LOOP

1:	/* LENGTHS == NULL */
	ld	[%o5 + 0x00], %f3		! ux = from[0]
	ld	[%o5 + 0x04], %f5		! uy = from[1]
	ld	[%o5 + 0x08], %f7		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* f3=tx, f5=ty, f7=tz */

	/* len (f6) = (tx * tx) + (ty * ty) + (tz * tz) */
	fmuls	%f3, %f3, %f6			! FGM	Group	f3 available
	fmuls	%f5, %f5, %f8			! FGM	Group	f5 available
	fmuls	%f7, %f7, %f10			! FGM	Group	f7 available
	fadds	%f6, %f8, %f6			! FGA	Group	2cyc stall f6,f8 available
	fadds	%f6, %f10, %f6			! FGA	Group	4cyc stall f6,f10 available

	/* scale (f6) = 1.0 / sqrt(len) */
	fsqrts	%f6, %f6			! FDIV  20 cycles
	fdivs	%f12, %f6, %f6			! FDIV	14 cycles

	fmuls	%f3, %f6, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * scale
	fmuls	%f5, %f6, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * scale
	fmuls	%f7, %f6, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * scale

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

	ba	7f
	 nop

4:	/* LENGTHS != NULL */

5:
	ld	[%o5 + 0x00], %f3		! ux = from[0]
	ld	[%o5 + 0x04], %f5		! uy = from[1]
	ld	[%o5 + 0x08], %f7		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	ld	[%o3], %f13			! LSU
	add	%o3, 4, %o3			! IEU0

	/* f3=tx, f5=ty, f7=tz, f13=lengths[i] */

	fmuls	%f3, %f13, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * len
	fmuls	%f5, %f13, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * len
	fmuls	%f7, %f13, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * len

	cmp	%o4, %g1			! continue if (i < count)
	bl	5b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop

	.globl	_mesa_sparc_rescale_normals
_mesa_sparc_rescale_normals:
	/* o0=mat o1=scale o2=in o3=lengths o4=dest */

	sethi	%hi(ONE_DOT_ZERO), %g2
	sub	%sp, 16, %sp
	st	%o1, [%sp + STACK_VAR_OFF+0x0]
	ld	[%sp + STACK_VAR_OFF+0x0], %f15	! f15 = scale
	add	%sp, 16, %sp

	LDPTR	[%o2 + V4F_START], %o5		! o5 = 'from' in->start
	ld	[%o2 + V4F_COUNT], %g1		! g1 = in->count
	ld	[%o2 + V4F_STRIDE], %g2		! g2 = in->stride
	LDPTR	[%o4 + V4F_START], %g3		! g3 = 'out' dest->start

	/* dest->count = in->count */
	st	%g1, [%o4 + V4F_COUNT]

	cmp	%g1, 1
	bl	7f
	 clr	%o4				! 'i' for STRIDE_LOOP

1:
	ld	[%o5 + 0x00], %f3		! ux = from[0]
	ld	[%o5 + 0x04], %f5		! uy = from[1]
	ld	[%o5 + 0x08], %f7		! uz = from[2]
	add	%o5, %g2, %o5			! STRIDE_F(from, stride)
	add	%o4, 1, %o4			! i++

	/* f3=tx, f5=ty, f7=tz */

	fmuls	%f3, %f15, %f3
	st	%f3, [%g3 + 0x00]		! out[i][0] = tx * scale
	fmuls	%f5, %f15, %f5
	st	%f5, [%g3 + 0x04]		! out[i][1] = ty * scale
	fmuls	%f7, %f15, %f7
	st	%f7, [%g3 + 0x08]		! out[i][2] = tz * scale

	cmp	%o4, %g1			! continue if (i < count)
	bl	1b
	 add	%g3, 0x0c, %g3			! advance out vector pointer

7:	retl
	 nop
