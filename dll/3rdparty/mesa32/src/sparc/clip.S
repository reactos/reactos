/*
 * Clip testing in SPARC assembly
 */

#if __arch64__
#define LDPTR		ldx
#define V4F_DATA	0x00
#define V4F_START	0x08
#define V4F_COUNT	0x10
#define V4F_STRIDE	0x14
#define V4F_SIZE	0x18
#define V4F_FLAGS	0x1c
#else
#define LDPTR		ld
#define V4F_DATA	0x00
#define V4F_START	0x04
#define V4F_COUNT	0x08
#define V4F_STRIDE	0x0c
#define V4F_SIZE	0x10
#define V4F_FLAGS	0x14
#endif

#define VEC_SIZE_1   	1
#define VEC_SIZE_2   	3
#define VEC_SIZE_3   	7
#define VEC_SIZE_4   	15

#if defined(SVR4) || defined(__SVR4) || defined(__svr4__)
        /* Solaris requires this for 64-bit. */
        .register %g2, #scratch
        .register %g3, #scratch
        .register %g7, #scratch
#endif

	.text
	.align		64

one_dot_zero:
	.word		0x3f800000	/* 1.0f */

	/* This trick is shamelessly stolen from the x86
	 * Mesa asm.  Very clever, and we can do it too
	 * since we have the necessary add with carry
	 * instructions on Sparc.
	 */
clip_table:
	.byte	 0,  1,  0,  2,  4,  5,  4,  6
	.byte	 0,  1,  0,  2,  8,  9,  8, 10
	.byte	32, 33, 32, 34, 36, 37, 36, 38
	.byte	32, 33, 32, 34, 40, 41, 40, 42
	.byte	 0,  1,  0,  2,  4,  5,  4,  6
	.byte	 0,  1,  0,  2,  8,  9,  8, 10
	.byte	16, 17, 16, 18, 20, 21, 20, 22
	.byte	16, 17, 16, 18, 24, 25, 24, 26
	.byte	63, 61, 63, 62, 55, 53, 55, 54
	.byte	63, 61, 63, 62, 59, 57, 59, 58
	.byte	47, 45, 47, 46, 39, 37, 39, 38
	.byte	47, 45, 47, 46, 43, 41, 43, 42
	.byte	63, 61, 63, 62, 55, 53, 55, 54
	.byte	63, 61, 63, 62, 59, 57, 59, 58
	.byte	31, 29, 31, 30, 23, 21, 23, 22
	.byte	31, 29, 31, 30, 27, 25, 27, 26

/* GLvector4f *clip_vec, GLvector4f *proj_vec, 
   GLubyte clipMask[], GLubyte *orMask, GLubyte *andMask */

	.align		64
__pc_tramp:
	retl
	 nop

	.globl		_mesa_sparc_cliptest_points4
_mesa_sparc_cliptest_points4:
	save		%sp, -64, %sp
	call		__pc_tramp
	 sub		%o7, (. - one_dot_zero - 4), %g1
	ld		[%g1 + 0x0], %f4
	add		%g1, 0x4, %g1

	ld		[%i0 + V4F_STRIDE], %l1
	ld		[%i0 + V4F_COUNT], %g7
	LDPTR		[%i0 + V4F_START], %i0
	LDPTR		[%i1 + V4F_START], %i5
	ldub		[%i3], %g2
	ldub		[%i4], %g3
	sll		%g3, 8, %g3
	or		%g2, %g3, %g2

	ld		[%i1 + V4F_FLAGS], %g3
	or		%g3, VEC_SIZE_4, %g3
	st		%g3, [%i1 + V4F_FLAGS]
	mov		3, %g3
	st		%g3, [%i1 + V4F_SIZE]
	st		%g7, [%i1 + V4F_COUNT]
	clr		%l2
	clr		%l0

	/* l0:	i
	 * g7:	count
	 * l1:	stride
	 * l2:	c
	 * g2:	(tmpAndMask << 8) | tmpOrMask
	 * g1:	clip_table
	 * i0:	from[stride][i]
	 * i2:	clipMask
	 * i5:	vProj[4][i]
	 */

1:	ld		[%i0 + 0x0c], %f3	! LSU	Group
	ld		[%i0 + 0x0c], %g5	! LSU	Group
	ld		[%i0 + 0x08], %g4	! LSU	Group
	fdivs		%f4, %f3, %f8		! FGM
	addcc		%g5, %g5, %g5		! IEU1	Group
	addx		%g0, 0x0, %g3		! IEU1	Group
	addcc		%g4, %g4, %g4		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	subcc		%g5, %g4, %g0		! IEU1	Group
	ld		[%i0 + 0x04], %g4	! LSU	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	addcc		%g4, %g4, %g4		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	subcc		%g5, %g4, %g0		! IEU1	Group
	ld		[%i0 + 0x00], %g4	! LSU	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	addcc		%g4, %g4, %g4		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	subcc		%g5, %g4, %g0		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	ldub		[%g1 + %g3], %g3	! LSU	Group
	cmp		%g3, 0			! IEU1	Group, stall
	be		2f			! CTI
	 stb		%g3, [%i2]		! LSU
	sll		%g3, 8, %g4		! IEU1	Group
	add		%l2, 1, %l2		! IEU0
	st		%g0, [%i5 + 0x00]	! LSU
	or		%g4, 0xff, %g4		! IEU0	Group
	or		%g2, %g3, %g2		! IEU1
	st		%g0, [%i5 + 0x04]	! LSU
	and		%g2, %g4, %g2		! IEU0	Group
	st		%g0, [%i5 + 0x08]	! LSU
	b		3f			! CTI
	 st		%f4, [%i5 + 0x0c]	! LSU	Group
2:	ld		[%i0 + 0x00], %f0	! LSU	Group
	ld		[%i0 + 0x04], %f1	! LSU	Group
	ld		[%i0 + 0x08], %f2	! LSU	Group
	fmuls		%f0, %f8, %f0		! FGM
	st		%f0, [%i5 + 0x00]	! LSU	Group
	fmuls		%f1, %f8, %f1		! FGM
	st		%f1, [%i5 + 0x04]	! LSU	Group
	fmuls		%f2, %f8, %f2		! FGM
	st		%f2, [%i5 + 0x08]	! LSU	Group
	st		%f8, [%i5 + 0x0c]	! LSU	Group
3:	add		%i5, 0x10, %i5		! IEU1
	add		%l0, 1, %l0		! IEU0	Group
	add		%i2, 1, %i2		! IEU0	Group
	cmp		%l0, %g7		! IEU1	Group
	bne		1b			! CTI
	 add		%i0, %l1, %i0		! IEU0	Group
	stb		%g2, [%i3]		! LSU
	srl		%g2, 8, %g3		! IEU0	Group
	cmp		%l2, %g7		! IEU1	Group
	bl,a		1f			! CTI
	 clr		%g3			! IEU0
1:	stb		%g3, [%i4]		! LSU	Group
	ret					! CTI	Group
	 restore	%i1, 0x0, %o0

	.globl		_mesa_sparc_cliptest_points4_np
_mesa_sparc_cliptest_points4_np:
	save		%sp, -64, %sp

	call		__pc_tramp
	 sub		%o7, (. - one_dot_zero - 4), %g1
	add		%g1, 0x4, %g1

	ld		[%i0 + V4F_STRIDE], %l1
	ld		[%i0 + V4F_COUNT], %g7
	LDPTR		[%i0 + V4F_START], %i0
	LDPTR		[%i1 + V4F_START], %i5
	ldub		[%i3], %g2
	ldub		[%i4], %g3
	sll		%g3, 8, %g3
	or		%g2, %g3, %g2

	ld		[%i1 + V4F_FLAGS], %g3
	or		%g3, VEC_SIZE_4, %g3
	st		%g3, [%i1 + V4F_FLAGS]
	mov		3, %g3
	st		%g3, [%i1 + V4F_SIZE]
	st		%g7, [%i1 + V4F_COUNT]
	clr		%l2
	clr		%l0

	/* l0:	i
	 * g7:	count
	 * l1:	stride
	 * l2:	c
	 * g2:	(tmpAndMask << 8) | tmpOrMask
	 * g1:	clip_table
	 * i0:	from[stride][i]
	 * i2:	clipMask
	 */

1:	ld		[%i0 + 0x0c], %g5	! LSU	Group
	ld		[%i0 + 0x08], %g4	! LSU	Group
	addcc		%g5, %g5, %g5		! IEU1	Group
	addx		%g0, 0x0, %g3		! IEU1	Group
	addcc		%g4, %g4, %g4		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	subcc		%g5, %g4, %g0		! IEU1	Group
	ld		[%i0 + 0x04], %g4	! LSU	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	addcc		%g4, %g4, %g4		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	subcc		%g5, %g4, %g0		! IEU1	Group
	ld		[%i0 + 0x00], %g4	! LSU	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	addcc		%g4, %g4, %g4		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	subcc		%g5, %g4, %g0		! IEU1	Group
	addx		%g3, %g3, %g3		! IEU1	Group
	ldub		[%g1 + %g3], %g3	! LSU	Group
	cmp		%g3, 0			! IEU1	Group, stall
	be		2f			! CTI
	 stb		%g3, [%i2]		! LSU
	sll		%g3, 8, %g4		! IEU1	Group
	add		%l2, 1, %l2		! IEU0
	or		%g4, 0xff, %g4		! IEU0	Group
	or		%g2, %g3, %g2		! IEU1
	and		%g2, %g4, %g2		! IEU0	Group
2:	add		%l0, 1, %l0		! IEU0	Group
	add		%i2, 1, %i2		! IEU0	Group
	cmp		%l0, %g7		! IEU1	Group
	bne		1b			! CTI
	 add		%i0, %l1, %i0		! IEU0	Group
	stb		%g2, [%i3]		! LSU
	srl		%g2, 8, %g3		! IEU0	Group
	cmp		%l2, %g7		! IEU1	Group
	bl,a		1f			! CTI
	 clr		%g3			! IEU0
1:	stb		%g3, [%i4]		! LSU	Group
	ret					! CTI	Group
	 restore	%i1, 0x0, %o0
