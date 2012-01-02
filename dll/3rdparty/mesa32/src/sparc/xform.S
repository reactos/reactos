
	/* TODO
	 *
	 * 1) It would be nice if load/store double could be used
	 *    at least for the matrix parts.  I think for the matrices
	 *    it is safe, but for the vertices it probably is not due to
	 *    things like glInterleavedArrays etc.
	 *
	 *    UPDATE: Trying this now in sparc_matrix.h -DaveM_990624
	 *
	 * 2) One extremely slick trick would be if we could enclose
	 *    groups of xform calls on the same vertices such that
	 *    we just load the matrix into f16-->f31 before the calls
	 *    and then we would not have to do them here.  This may be
	 *    tricky and not much of a gain though.
	 */

#include "sparc_matrix.h"

#if defined(SVR4) || defined(__SVR4) || defined(__svr4__)
	/* Solaris requires this for 64-bit. */
        .register %g2, #scratch
        .register %g3, #scratch
#endif

	.text
	.align	64

__set_v4f_1:
	ld	[%o0 + V4F_FLAGS], %g2
	mov	1, %g1
	st	%g1, [%o0 + V4F_SIZE]
	or	%g2, VEC_SIZE_1, %g2
	retl
	 st	%g2, [%o0 + V4F_FLAGS]
__set_v4f_2:
	ld	[%o0 + V4F_FLAGS], %g2
	mov	2, %g1
	st	%g1, [%o0 + V4F_SIZE]
	or	%g2, VEC_SIZE_2, %g2
	retl
	 st	%g2, [%o0 + V4F_FLAGS]
__set_v4f_3:
	ld	[%o0 + V4F_FLAGS], %g2
	mov	3, %g1
	st	%g1, [%o0 + V4F_SIZE]
	or	%g2, VEC_SIZE_3, %g2
	retl
	 st	%g2, [%o0 + V4F_FLAGS]
__set_v4f_4:
	ld	[%o0 + V4F_FLAGS], %g2
	mov	4, %g1
	st	%g1, [%o0 + V4F_SIZE]
	or	%g2, VEC_SIZE_4, %g2
	retl
	 st	%g2, [%o0 + V4F_FLAGS]

	/* First the raw versions. */

	.globl	_mesa_sparc_transform_points1_general
_mesa_sparc_transform_points1_general:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_3_12_13_14_15(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f8	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f1		! FGM	Group	1-cycle stall on %f0
	fmuls	%f0, M1, %f2		! FGM	Group
	fmuls	%f0, M2, %f3		! FGM	Group
	fmuls	%f0, M3, %f4		! FGM	Group
	fmuls	%f8, M0, %f9		! FGM	Group	f1 available
	fadds	%f1, M12, %f1		! FGA
	st	%f1, [%g2 + 0x00]	! LSU
	fmuls	%f8, M1, %f10		! FGM	Group	f2 available
	fadds	%f2, M13, %f2		! FGA
	st	%f2, [%g2 + 0x04]	! LSU
	fmuls	%f8, M2, %f11		! FGM	Group	f3 available
	fadds	%f3, M14, %f3		! FGA
	st	%f3, [%g2 + 0x08]	! LSU
	fmuls	%f8, M3, %f12		! FGM	Group	f4 available
	fadds	%f4, M15, %f4		! FGA
	st	%f4, [%g2 + 0x0c]	! LSU
	fadds	%f9, M12, %f9		! FGA	Group	f9 available
	st	%f9, [%g2 + 0x10]	! LSU
	fadds	%f10, M13, %f10		! FGA	Group	f10 available
	st	%f10, [%g2 + 0x14]	! LSU
	fadds	%f11, M14, %f11		! FGA	Group	f11 available
	st	%f11, [%g2 + 0x18]	! LSU
	fadds	%f12, M15, %f12		! FGA	Group	f12 available
	st	%f12, [%g2 + 0x1c]	! LSU
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0	! LSU	Group
	fmuls	%f0, M0, %f1		! FGM	Group	1-cycle stall on %f0
	fmuls	%f0, M1, %f2		! FGM	Group
	fmuls	%f0, M2, %f3		! FGM	Group
	fmuls	%f0, M3, %f4		! FGM	Group
	fadds	%f1, M12, %f1		! FGA	Group
	st	%f1, [%g2 + 0x00]	! LSU
	fadds	%f2, M13, %f2		! FGA	Group
	st	%f2, [%g2 + 0x04]	! LSU
	fadds	%f3, M14, %f3		! FGA	Group
	st	%f3, [%g2 + 0x08]	! LSU
	fadds	%f4, M15, %f4		! FGA	Group
	st	%f4, [%g2 + 0x0c]	! LSU

3:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points1_identity
_mesa_sparc_transform_points1_identity:
	cmp	%o0, %o2
	be	4f
	 ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f1	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	st	%f0, [%g2 + 0x00]	! LSU	Group
	cmp	%o1, %o2		! IEU1
	st	%f1, [%g2 + 0x10]	! LSU	Group
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0
	addx	%g0, %g0, %g0
	st	%f0, [%g2 + 0x00]

3:
	ba	__set_v4f_1
	 nop

4:	retl
	 nop

	.globl	_mesa_sparc_transform_points1_2d
_mesa_sparc_transform_points1_2d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_12_13(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f8	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f1		! FGM	Group
	fmuls	%f0, M1, %f2		! FGM	Group
	fmuls	%f8, M0, %f9		! FGM	Group
	fmuls	%f8, M1, %f10		! FGM	Group
	fadds	%f1, M12, %f3		! FGA	Group	f1 available
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f2, M13, %f4		! FGA	Group	f2 available
	st	%f4, [%g2 + 0x04]	! LSU
	fadds	%f9, M12, %f11		! FGA	Group	f9 available
	st	%f11, [%g2 + 0x10]	! LSU
	fadds	%f10, M13, %f12		! FGA	Group	f10 available
	st	%f12, [%g2 + 0x14]	! LSU
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0
	fmuls	%f0, M0, %f1
	fmuls	%f0, M1, %f2
	fadds	%f1, M12, %f3
	st	%f3, [%g2 + 0x00]
	fadds	%f2, M13, %f4
	st	%f4, [%g2 + 0x04]

3:
	ba	__set_v4f_2
	 nop

	.globl	_mesa_sparc_transform_points1_2d_no_rot
_mesa_sparc_transform_points1_2d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_12_13(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f4	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f1		! FGM	Group
	fmuls	%f4, M0, %f5		! FGM	Group
	fadds	%f1, M12, %f3		! FGA	Group, 2 cycle stall, f1 available
	st	%f3, [%g2 + 0x00]	! LSU
	st	M13, [%g2 + 0x04]	! LSU	Group, f5 available
	fadds	%f5, M12, %f6		! FGA
	st	%f6, [%g2 + 0x10]	! LSU	Group
	st	M13, [%g2 + 0x14]	! LSU	Group
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0
	fmuls	%f0, M0, %f1
	fadds	%f1, M12, %f3
	st	%f3, [%g2 + 0x00]
	st	M13, [%g2 + 0x04]

3:
	ba	__set_v4f_2
	 nop

	.globl	_mesa_sparc_transform_points1_3d
_mesa_sparc_transform_points1_3d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_12_13_14(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f4	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f1		! FGM	Group
	fmuls	%f0, M1, %f2		! FGM	Group
	fmuls	%f0, M2, %f3		! FGM	Group
	fmuls	%f4, M0, %f5		! FGM	Group
	fadds	%f1, M12, %f1		! FGA	Group, f1 available
	st	%f1, [%g2 + 0x00]	! LSU
	fmuls	%f4, M1, %f6		! FGM
	fadds	%f2, M13, %f2		! FGA	Group, f2 available
	st	%f2, [%g2 + 0x04]	! LSU
	fmuls	%f4, M2, %f7		! FGM
	fadds	%f3, M14, %f3		! FGA	Group, f3 available
	st	%f3, [%g2 + 0x08]	! LSU
	fadds	%f5, M12, %f5		! FGA	Group, f5 available
	st	%f5, [%g2 + 0x10]	! LSU
	fadds	%f6, M13, %f6		! FGA	Group, f6 available
	st	%f6, [%g2 + 0x14]	! LSU
	fadds	%f7, M14, %f7		! FGA	Group, f7 available
	st	%f7, [%g2 + 0x18]	! LSU
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0
	fmuls	%f0, M0, %f1
	fmuls	%f0, M1, %f2
	fmuls	%f0, M2, %f3
	fadds	%f1, M12, %f1
	st	%f1, [%g2 + 0x00]
	fadds	%f2, M13, %f2
	st	%f2, [%g2 + 0x04]
	fadds	%f3, M14, %f3
	st	%f3, [%g2 + 0x08]

3:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points1_3d_no_rot
_mesa_sparc_transform_points1_3d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_12_13_14(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f2	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f1		! FGM	Group
	fmuls	%f2, M0, %f3		! FGM	Group
	fadds	%f1, M12, %f1		! FGA	Group, 2 cycle stall, f1 available
	st	%f1, [%g2 + 0x00]	! LSU
	fadds	%f3, M12, %f3		! FGA	Group, f3 available
	st	M13, [%g2 + 0x04]	! LSU
	st	M14, [%g2 + 0x08]	! LSU	Group
	st	%f3, [%g2 + 0x10]	! LSU	Group
	st	M13, [%g2 + 0x14]	! LSU	Group
	st	M14, [%g2 + 0x18]	! LSU	Group
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0
	fmuls	%f0, M0, %f1
	fadds	%f1, M12, %f1
	st	%f1, [%g2 + 0x00]
	st	M13, [%g2 + 0x04]
	st	M14, [%g2 + 0x08]

3:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points1_perspective
_mesa_sparc_transform_points1_perspective:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_14(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	ld	[%g1 + 0x00], %f2	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f1		! FGM	Group
	st	%f1, [%g2 + 0x00]	! LSU
	fmuls	%f2, M0, %f3		! FGM	Group
	st	%g0, [%g2 + 0x04]	! LSU
	st	M14, [%g2 + 0x08]	! LSU	Group
	st	%g0, [%g2 + 0x0c]	! LSU	Group
	st	%f3, [%g2 + 0x10]	! LSU	Group
	st	%g0, [%g2 + 0x14]	! LSU	Group
	st	M14, [%g2 + 0x18]	! LSU	Group
	st	%g0, [%g2 + 0x1c]	! LSU	Group
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0
	fmuls	%f0, M0, %f1
	st	%f1, [%g2 + 0x00]
	st	%g0, [%g2 + 0x04]
	st	M14, [%g2 + 0x08]
	st	%g0, [%g2 + 0x0c]

3:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points2_general
_mesa_sparc_transform_points2_general:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_3_4_5_6_7_12_13_14_15(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f2		! FGM	Group
	fmuls	%f0, M1, %f3		! FGM	Group
	fmuls	%f0, M2, %f4		! FGM	Group
	fmuls	%f0, M3, %f5		! FGM	Group
	fadds	%f2, M12, %f2		! FGA	Group	f2 available
	fmuls	%f1, M4, %f6		! FGM
	fadds	%f3, M13, %f3		! FGA	Group	f3 available
	fmuls	%f1, M5, %f7		! FGM
	fadds	%f4, M14, %f4		! FGA	Group	f4 available
	fmuls	%f1, M6, %f8		! FGM
	fadds	%f5, M15, %f5		! FGA	Group	f5 available
	fmuls	%f1, M7, %f9		! FGM
	fadds	%f2, %f6, %f2		! FGA	Group	f6 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, %f7, %f3		! FGA	Group	f7 available
	st	%f3, [%g2 + 0x04]	! LSU
	fadds	%f4, %f8, %f4		! FGA	Group	f8 available
	st	%f4, [%g2 + 0x08]	! LSU
	fadds	%f5, %f9, %f5		! FGA	Group	f9 available
	st	%f5, [%g2 + 0x0c]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points2_identity
_mesa_sparc_transform_points2_identity:
	cmp	%o2, %o0
	be	3f
	 ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	ld	[%g1 + 0x04], %f1	! LSU	Group
	add	%g1, %o5, %g1		! IEU0
	cmp	%o1, %g3		! IEU1
	st	%f0, [%g2 + 0x00]	! LSU	Group
	st	%f1, [%g2 + 0x04]	! LSU	Group
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0
2:
	ba	__set_v4f_2
	 nop

3:	retl
	 nop

	.globl	_mesa_sparc_transform_points2_2d
_mesa_sparc_transform_points2_2d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_4_5_12_13(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f2		! FGM
	ld	[%g1 + 0x00], %f8	! LSU	Group
	fmuls	%f0, M1, %f3		! FGM
	ld	[%g1 + 0x04], %f9	! LSU	Group
	fmuls	%f1, M4, %f6		! FGM
	fmuls	%f1, M5, %f7		! FGM	Group
	add	%g1, %o5, %g1		! IEU0
	fmuls	%f8, M0, %f10		! FGM	Group	f2 available
	fadds	%f2, M12, %f2		! FGA
	fmuls	%f8, M1, %f11		! FGM	Group	f3 available
	fadds	%f3, M13, %f3		! FGA
	fmuls	%f9, M4, %f12		! FGM	Group
	fmuls	%f9, M5, %f13		! FGM	Group
	fadds	%f10, M12, %f10		! FGA	Group	f2, f10 available
	fadds	%f2, %f6, %f2		! FGA	Group	f3, f11 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f11, M13, %f11		! FGA	Group	f12 available
	fadds	%f3, %f7, %f3		! FGA	Group	f13 available
	st	%f3, [%g2 + 0x04]	! LSU
	fadds	%f10, %f12, %f10	! FGA	Group	f10 available
	st	%f10, [%g2 + 0x10]	! LSU
	fadds	%f11, %f13, %f11	! FGA	Group	f11 available
	st	%f11, [%g2 + 0x14]	! LSU
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM	Group
	fmuls	%f0, M1, %f3		! FGM	Group
	fmuls	%f1, M4, %f6		! FGM	Group
	fmuls	%f1, M5, %f7		! FGM	Group
	fadds	%f2, M12, %f2		! FGA	Group	f2 available
	fadds	%f3, M13, %f3		! FGA	Group	f3 available
	fadds	%f2, %f6, %f2		! FGA	Group	2 cycle stall, f2 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, %f7, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x04]	! LSU

3:
	ba	__set_v4f_2
	 nop

	.globl	_mesa_sparc_transform_points2_2d_no_rot
_mesa_sparc_transform_points2_2d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_12_13(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	ld	[%g1 + 0x00], %f4	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM
	ld	[%g1 + 0x04], %f5	! LSU	Group
	fmuls	%f1, M5, %f3		! FGM
	fmuls	%f4, M0, %f6		! FGM	Group
	add	%g1, %o5, %g1		! IEU0
	fmuls	%f5, M5, %f7		! FGM	Group
	fadds	%f2, M12, %f2		! FGA	Group	f2 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, M13, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x04]	! LSU
	fadds	%f6, M12, %f6		! FGA	Group	f6 available
	st	%f6, [%g2 + 0x10]	! LSU
	fadds	%f7, M13, %f7		! FGA	Group	f7 available
	st	%f7, [%g2 + 0x14]	! LSU
	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM	Group
	fmuls	%f1, M5, %f3		! FGM	Group
	fadds	%f2, M12, %f2		! FGA	Group, 2 cycle stall, f2 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, M13, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x04]	! LSU

3:
	ba	__set_v4f_2
	 nop

	/* orig: 12 cycles */
	.globl	_mesa_sparc_transform_points2_3d
_mesa_sparc_transform_points2_3d:
	ld	[%o2 + V4F_STRIDE], %o5
	ld	[%o2 + V4F_START], %g1
	ld	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_3_4_5_6_12_13_14(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o1

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	add	%o1, 2, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	ld	[%g1 + 0x00], %f9	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM
	ld	[%g1 + 0x04], %f10	! LSU	Group
	fmuls	%f0, M1, %f3		! FGM
	fmuls	%f0, M2, %f4		! FGM	Group
	add	%g1, %o5, %g1		! IEU0
	fmuls	%f1, M4, %f6		! FGM	Group
	fmuls	%f1, M5, %f7		! FGM	Group	f2 available
	fadds	%f2, M12, %f2		! FGA
	fmuls	%f1, M6, %f8		! FGM	Group	f3 available
	fadds	%f3, M13, %f3		! FGA
	fmuls	%f9, M0, %f11		! FGM	Group	f4 available
	fadds	%f4, M14, %f4		! FGA
	fmuls	%f9, M1, %f12		! FGM	Group	f6 available
	fmuls	%f9, M2, %f13		! FGM	Group	f2, f7 available
	fadds	%f2, %f6, %f2		! FGA
	st	%f2, [%g2 + 0x00]	! LSU
	fmuls	%f10, M4, %f14		! FGM	Group	f3, f8 available
	fadds	%f3, %f7, %f3		! FGA
	st	%f3, [%g2 + 0x04]	! LSU
	fmuls	%f10, M5, %f15		! FGM	Group	f4, f11 available
	fadds	%f11, M12, %f11		! FGA
	fmuls	%f10, M6, %f0		! FGM	Group	f12 available
	fadds	%f12, M13, %f12		! FGA
	fadds	%f13, M14, %f13		! FGA	Group	f13 available
	fadds	%f4, %f8, %f4		! FGA	Group	f14 available
	st	%f4, [%g2 + 0x08]	! LSU
	fadds	%f11, %f14, %f11	! FGA	Group	f15, f11 available
	st	%f11, [%g2 + 0x10]	! LSU
	fadds	%f12, %f15, %f12	! FGA	Group	f0, f12 available
	st	%f12, [%g2 + 0x14]	! LSU
	fadds	%f13, %f0, %f13		! FGA	Group	f13 available
	st	%f13, [%g2 + 0x18]	! LSU

	cmp	%o1, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o1, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM	Group
	fmuls	%f0, M1, %f3		! FGM	Group
	fmuls	%f0, M2, %f4		! FGM	Group
	fmuls	%f1, M4, %f6		! FGM	Group
	fmuls	%f1, M5, %f7		! FGM	Group	f2 available
	fadds	%f2, M12, %f2		! FGA
	fmuls	%f1, M6, %f8		! FGM	Group	f3 available
	fadds	%f3, M13, %f3		! FGA
	fadds	%f4, M14, %f4		! FGA	Group	f4 available
	fadds	%f2, %f6, %f2		! FGA	Group	stall, f2, f6, f7 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, %f7, %f3		! FGA	Group	f3, f8 available
	st	%f3, [%g2 + 0x04]	! LSU
	fadds	%f4, %f8, %f4		! FGA	Group	f4 available
	st	%f4, [%g2 + 0x08]	! LSU

3:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points2_3d_no_rot
_mesa_sparc_transform_points2_3d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_12_13_14(%o1)

	cmp	%g3, 1
	st	%g3, [%o0 + V4F_COUNT]
	bl	3f
	 clr	%o3

	be	2f
	 andn	%g3, 1, %o2

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	add	%o3, 2, %o3		! IEU0
	add	%g1, %o5, %g1		! IEU1
	ld	[%g1 + 0x00], %f4	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM
	ld	[%g1 + 0x04], %f5	! LSU	Group
	fmuls	%f1, M5, %f3		! FGM
	fmuls	%f4, M0, %f6		! FGM	Group
	add	%g1, %o5, %g1		! IEU0
	fmuls	%f5, M5, %f7		! FGM	Group
	fadds	%f2, M12, %f2		! FGA	Group	f2 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, M13, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x04]	! LSU
	fadds	%f6, M12, %f6		! FGA	Group	f6 available
	st	M14, [%g2 + 0x08]	! LSU
	fadds	%f7, M13, %f7		! FGA	Group	f7 available
	st	%f6, [%g2 + 0x10]	! LSU
	st	%f7, [%g2 + 0x14]	! LSU	Group
	st	M14, [%g2 + 0x18]	! LSU	Group
	cmp	%o3, %o2		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x20, %g2		! IEU0	Group

	cmp	%o3, %g3
	be	3f
	 nop

2:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	fmuls	%f0, M0, %f2		! FGM	Group
	fmuls	%f1, M5, %f3		! FGM	Group
	fadds	%f2, M12, %f2		! FGA	Group, 2 cycle stall, f2 available
	st	%f2, [%g2 + 0x00]	! LSU
	fadds	%f3, M13, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x04]	! LSU
	st	M14, [%g2 + 0x08]	! LSU	Group

3:	ld	[%o1 + (14 * 0x4)], %g3
	cmp	%g3, 0
	bne	__set_v4f_3
	 nop
	ba	__set_v4f_2
	 nop

	.globl	_mesa_sparc_transform_points2_perspective
_mesa_sparc_transform_points2_perspective:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0
	ld	[%g1 + 0x04], %f1
	add	%o1, 1, %o1
	add	%g1, %o5, %g1
	fmuls	%f0, M0, %f2
	st	%f2, [%g2 + 0x00]
	fmuls	%f1, M5, %f3
	st	%f3, [%g2 + 0x04]
	st	M14, [%g2 + 0x08]
	st	%g0, [%g2 + 0x0c]
	cmp	%o1, %g3
	bne	1b
	 add	%g2, 0x10, %g2
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points3_general
_mesa_sparc_transform_points3_general:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f3		! FGM
	fmuls	%f1, M4, %f7		! FGM	Group
	fmuls	%f0, M1, %f4		! FGM	Group
	fmuls	%f1, M5, %f8		! FGM	Group
	fmuls	%f0, M2, %f5		! FGM	Group	f3 available
	fmuls	%f1, M6, %f9		! FGM	Group	f7 available
	fadds	%f3, %f7, %f3		! FGA
	fmuls	%f0, M3, %f6		! FGM	Group	f4 available
	fmuls	%f1, M7, %f10		! FGM	Group	f8 available
	fadds	%f4, %f8, %f4		! FGA
	fmuls	%f2, M8, %f7		! FGM	Group	f5 available
	fmuls	%f2, M9, %f8		! FGM	Group	f9,f3 available
	fadds	%f5, %f9, %f5		! FGA
	fmuls	%f2, M10, %f9		! FGM	Group	f6 available
	fadds	%f6, %f10, %f6		! FGA	Group	f10,f4 available
	fmuls	%f2, M11, %f10		! FGM
	fadds	%f3, M12, %f3		! FGA	Group	f7 available
	fadds	%f4, M13, %f4		! FGA	Group	f8,f5 available
	fadds	%f5, M14, %f5		! FGA	Group	f9 available
	fadds	%f6, M15, %f6		! FGA	Group	f10,f6 available
	fadds	%f3, %f7, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f4, %f8, %f4		! FGA	Group	f4 available
	st	%f4, [%g2 + 0x04]	! LSU
	fadds	%f5, %f9, %f5		! FGA	Group	f5 available
	st	%f5, [%g2 + 0x08]	! LSU
	fadds	%f6, %f10, %f6		! FGA	Group	f6 available
	st	%f6, [%g2 + 0x0c]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points3_identity
_mesa_sparc_transform_points3_identity:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0
	ld	[%g1 + 0x04], %f1
	ld	[%g1 + 0x08], %f2
	add	%o1, 1, %o1
	add	%g1, %o5, %g1
	cmp	%o1, %g3
	st	%f0, [%g2 + 0x00]
	st	%f1, [%g2 + 0x04]
	st	%f2, [%g2 + 0x08]
	bne	1b
	 add	%g2, 0x10, %g2
2:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points3_2d
_mesa_sparc_transform_points3_2d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_4_5_12_13(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f3		! FGM
	fmuls	%f0, M1, %f4		! FGM	Group
	fmuls	%f1, M4, %f6		! FGM	Group
	fmuls	%f1, M5, %f7		! FGM	Group
	fadds	%f3, M12, %f3		! FGA	Group	f3 available
	fadds	%f4, M13, %f4		! FGA	Group	f4 available
	fadds	%f3, %f6, %f3		! FGA	Group	f6 available
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f4, %f7, %f4		! FGA	Group	f7 available
	st	%f4, [%g2 + 0x04]	! LSU
	st	%f2, [%g2 + 0x08]	! LSU	Group
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points3_2d_no_rot
_mesa_sparc_transform_points3_2d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_12_13(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f3		! FGM
	fmuls	%f1, M5, %f4		! FGM	Group
	st	%f2, [%g2 + 0x08]	! LSU
	fadds	%f3, M12, %f3		! FGA	Group
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f4, M13, %f4		! FGA	Group
	st	%f4, [%g2 + 0x04]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points3_3d
_mesa_sparc_transform_points3_3d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_4_5_6_8_9_10_12_13_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f3		! FGM
	fmuls	%f1, M4, %f6		! FGM	Group
	fmuls	%f0, M1, %f4		! FGM	Group
	fmuls	%f1, M5, %f7		! FGM	Group
	fmuls	%f0, M2, %f5		! FGM	Group	f3 available
	fmuls	%f1, M6, %f8		! FGM	Group	f6 available
	fadds	%f3, %f6, %f3		! FGA
	fmuls	%f2, M8, %f9		! FGM	Group	f4 available
	fmuls	%f2, M9, %f10		! FGM	Group	f7 available
	fadds	%f4, %f7, %f4		! FGA
	fmuls	%f2, M10, %f11		! FGM	Group	f5 available
	fadds	%f5, %f8, %f5		! FGA	Group	f8, f3 available
	fadds	%f3, %f9, %f3		! FGA	Group	f9 available
	fadds	%f4, %f10, %f4		! FGA	Group	f10, f4 available
	fadds	%f5, %f11, %f5		! FGA	Group	stall, f11, f5 available
	fadds	%f3, M12, %f3		! FGA	Group	f3 available
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f4, M13, %f4		! FGA	Group	f4 available
	st	%f4, [%g2 + 0x04]	! LSU
	fadds	%f5, M14, %f5		! FGA	Group	f5 available
	st	%f5, [%g2 + 0x08]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points3_3d_no_rot
_mesa_sparc_transform_points3_3d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_10_12_13_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	cmp	%o1, %g3		! IEU1	Group
	fmuls	%f0, M0, %f3		! FGM
	fmuls	%f1, M5, %f4		! FGM	Group
	fmuls	%f2, M10, %f5		! FGM	Group
	fadds	%f3, M12, %f3		! FGA	Group, stall, f3 available
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f4, M13, %f4		! FGA	Group, f4 available
	st	%f4, [%g2 + 0x04]	! LSU
	fadds	%f5, M14, %f5		! FGA	Group, f5 available
	st	%f5, [%g2 + 0x08]	! LEU
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_3
	 nop

	.globl	_mesa_sparc_transform_points3_perspective
_mesa_sparc_transform_points3_perspective:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_8_9_10_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f3		! FGM
	fmuls	%f2, M8, %f6		! FGM	Group
	fmuls	%f1, M5, %f4		! FGM	Group
	fmuls	%f2, M9, %f7		! FGM	Group
	fmuls	%f2, M10, %f5		! FGM	Group	f3 available
	fadds	%f3, %f6, %f3		! FGA	Group	f6 available
	st	%f3, [%g2 + 0x00]	! LSU
	fadds	%f4, %f7, %f4		! FGA	Group	stall, f4, f7 available
	st	%f4, [%g2 + 0x04]	! LSU
	fadds	%f5, M14, %f5		! FGA	Group
	st	%f5, [%g2 + 0x08]	! LSU
	fnegs	%f2, %f6		! FGA	Group
	st	%f6, [%g2 + 0x0c]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_general
_mesa_sparc_transform_points4_general:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	ld	[%g1 + 0x0c], %f3	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f4		! FGM	Group
	fmuls	%f1, M4, %f8		! FGM	Group
	fmuls	%f0, M1, %f5		! FGM	Group
	fmuls	%f1, M5, %f9		! FGM	Group
	fmuls	%f0, M2, %f6		! FGM	Group	f4 available
	fmuls	%f1, M6, %f10		! FGM	Group	f8 available
	fadds	%f4, %f8, %f4		! FGA
	fmuls	%f0, M3, %f7		! FGM	Group	f5 available
	fmuls	%f1, M7, %f11		! FGM	Group	f9 available
	fadds	%f5, %f9, %f5		! FGA
	fmuls	%f2, M8, %f12		! FGM	Group	f6 available
	fmuls	%f2, M9, %f13		! FGM	Group	f10, f4 available
	fadds	%f6, %f10, %f6		! FGA
	fmuls	%f2, M10, %f14		! FGM	Group	f7 available
	fmuls	%f2, M11, %f15		! FGM	Group	f11, f5 available
	fadds	%f7, %f11, %f7		! FGA
	fmuls	%f3, M12, %f8		! FGM	Group	f12 available
	fadds	%f4, %f12, %f4		! FGA
	fmuls	%f3, M13, %f9		! FGM	Group	f13, f6 available
	fadds	%f5, %f13, %f5		! FGA
	fmuls	%f3, M14, %f10		! FGM	Group	f14 available
	fadds	%f6, %f14, %f6		! FGA
	fmuls	%f3, M15, %f11		! FGM	Group	f15, f7 available
	fadds	%f7, %f15, %f7		! FGA
	fadds	%f4, %f8, %f4		! FGA	Group	f8, f4 available
	st	%f4, [%g2 + 0x00]	! LSU
	fadds	%f5, %f9, %f5		! FGA	Group	f9, f5 available
	st	%f5, [%g2 + 0x04]	! LSU
	fadds	%f6, %f10, %f6		! FGA	Group	f10, f6 available
	st	%f6, [%g2 + 0x08]	! LSU
	fadds	%f7, %f11, %f7		! FGA	Group	f11, f7 available
	st	%f7, [%g2 + 0x0c]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_identity
_mesa_sparc_transform_points4_identity:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0
	ld	[%g1 + 0x04], %f1
	ld	[%g1 + 0x08], %f2
	add	%o1, 1, %o1
	ld	[%g1 + 0x0c], %f3
	add	%g1, %o5, %g1
	st	%f0, [%g2 + 0x00]
	st	%f1, [%g2 + 0x04]
	st	%f2, [%g2 + 0x08]
	cmp	%o1, %g3
	st	%f3, [%g2 + 0x0c]
	bne	1b
	 add	%g2, 0x10, %g2
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_2d
_mesa_sparc_transform_points4_2d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_4_5_12_13(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	ld	[%g1 + 0x0c], %f3	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f4		! FGM
	fmuls	%f1, M4, %f8		! FGM	Group
	fmuls	%f0, M1, %f5		! FGM	Group
	fmuls	%f1, M5, %f9		! FGM	Group	f4 available
	fmuls	%f3, M12, %f12		! FGM	Group
	fmuls	%f3, M13, %f13		! FGM	Group	f8 available
	fadds	%f4, %f8, %f4		! FGA
	fadds	%f5, %f9, %f5		! FGA	Group	stall, f5, f9 available
	fadds	%f4, %f12, %f4		! FGA	Group	2 cycle stall, f4, f12, f13 avail
	st	%f4, [%g2 + 0x00]	! LSU
	fadds	%f5, %f13, %f5		! FGA	Group	f5 available
	st	%f5, [%g2 + 0x04]	! LSU
	st	%f2, [%g2 + 0x08]	! LSU	Group
	st	%f3, [%g2 + 0x0c]	! LSU	Group
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_2d_no_rot
_mesa_sparc_transform_points4_2d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_4_5_12_13(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0
	ld	[%g1 + 0x04], %f1
	ld	[%g1 + 0x08], %f2
	ld	[%g1 + 0x0c], %f3
	add	%o1, 1, %o1
	add	%g1, %o5, %g1
	fmuls	%f0, M0, %f4
	fmuls	%f3, M12, %f8
	fmuls	%f1, M5, %f5
	fmuls	%f3, M13, %f9
	fadds	%f4, %f8, %f4
	st	%f4, [%g2 + 0x00]
	fadds	%f5, %f9, %f5
	st	%f5, [%g2 + 0x04]
	st	%f2, [%g2 + 0x08]
	st	%f3, [%g2 + 0x0c]
	cmp	%o1, %g3
	bne	1b
	 add	%g2, 0x10, %g2
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_3d
_mesa_sparc_transform_points4_3d:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_1_2_4_5_6_8_9_10_12_13_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	ld	[%g1 + 0x0c], %f3	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f4		! FGM
	fmuls	%f1, M4, %f7		! FGM	Group
	fmuls	%f0, M1, %f5		! FGM	Group
	fmuls	%f1, M5, %f8		! FGM	Group
	fmuls	%f0, M2, %f6		! FGM	Group	f4 available
	fmuls	%f1, M6, %f9		! FGM	Group	f7 available
	fadds	%f4, %f7, %f4		! FGA
	fmuls	%f2, M8, %f10		! FGM	Group	f5 available
	fmuls	%f2, M9, %f11		! FGM	Group	f8 available
	fadds	%f5, %f8, %f5		! FGA
	fmuls	%f2, M10, %f12		! FGM	Group	f6 available
	fmuls	%f3, M12, %f13		! FGM	Group	f9, f4 available
	fadds	%f6, %f9, %f6		! FGA
	fmuls	%f3, M13, %f14		! FGM	Group	f10 available
	fadds	%f4, %f10, %f4		! FGA
	fmuls	%f3, M14, %f15		! FGM	Group	f11, f5 available
	fadds	%f5, %f11, %f5		! FGA
	fadds	%f6, %f12, %f6		! FGA	Group	stall, f12, f13, f6 available
	fadds	%f4, %f13, %f4		! FGA	Group	f14, f4 available
	st	%f4, [%g2 + 0x00]	! LSU
	fadds	%f5, %f14, %f5		! FGA	Group	f15, f5 available
	st	%f5, [%g2 + 0x04]	! LSU
	fadds	%f6, %f15, %f6		! FGA	Group	f6 available
	st	%f6, [%g2 + 0x08]	! LSU
	st	%f3, [%g2 + 0x0c]	! LSU	Group
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_3d_no_rot
_mesa_sparc_transform_points4_3d_no_rot:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_10_12_13_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	ld	[%g1 + 0x0c], %f3	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f4		! FGM
	fmuls	%f3, M12, %f7		! FGM	Group
	fmuls	%f1, M5, %f5		! FGM	Group
	fmuls	%f3, M13, %f8		! FGM	Group
	fmuls	%f2, M10, %f6		! FGM	Group	f4 available
	fmuls	%f3, M14, %f9		! FGM	Group	f7 available
	fadds	%f4, %f7, %f4		! FGA
	st	%f4, [%g2 + 0x00]	! LSU
	fadds	%f5, %f8, %f5		! FGA	Group	stall, f5, f8 available
	st	%f5, [%g2 + 0x04]	! LSU
	fadds	%f6, %f9, %f6		! FGA	Group	stall, f6, f9 available
	st	%f6, [%g2 + 0x08]	! LSU
	st	%f3, [%g2 + 0x0c]	! LSU	Group
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop

	.globl	_mesa_sparc_transform_points4_perspective
_mesa_sparc_transform_points4_perspective:
	ld	[%o2 + V4F_STRIDE], %o5
	LDPTR	[%o2 + V4F_START], %g1
	LDPTR	[%o0 + V4F_START], %g2
	ld	[%o2 + V4F_COUNT], %g3

	LDMATRIX_0_5_8_9_10_14(%o1)

	cmp	%g3, 0
	st	%g3, [%o0 + V4F_COUNT]
	be	2f
	 clr	%o1

1:	ld	[%g1 + 0x00], %f0	! LSU	Group
	ld	[%g1 + 0x04], %f1	! LSU	Group
	ld	[%g1 + 0x08], %f2	! LSU	Group
	ld	[%g1 + 0x0c], %f3	! LSU	Group
	add	%o1, 1, %o1		! IEU0
	add	%g1, %o5, %g1		! IEU1
	fmuls	%f0, M0, %f4		! FGM
	fmuls	%f2, M8, %f7		! FGM	Group
	fmuls	%f1, M5, %f5		! FGM	Group
	fmuls	%f2, M9, %f8		! FGM	Group
	fmuls	%f2, M10, %f6		! FGM	Group	f4 available
	fmuls	%f3, M14, %f9		! FGM	Group	f7 available
	fadds	%f4, %f7, %f4		! FGA
	st	%f4, [%g2 + 0x00]	! LSU
	fadds	%f5, %f8, %f5		! FGA	Group	stall, f5, f8 available
	st	%f5, [%g2 + 0x04]	! LSU
	fadds	%f6, %f9, %f6		! FGA	Group	stall, f6, f9 available
	st	%f6, [%g2 + 0x08]	! LSU
	fnegs	%f2, %f7		! FGA	Group
	st	%f7, [%g2 + 0x0c]	! LSU
	cmp	%o1, %g3		! IEU1
	bne	1b			! CTI
	 add	%g2, 0x10, %g2		! IEU0	Group
2:
	ba	__set_v4f_4
	 nop
