/* $Id: asm-386.S,v 1.8 1997/12/17 00:50:51 brianp Exp $ */

/*
 * asm-386.S - special (hopefully faster) transformation functions for x86
 *
 * by Josh Vanderhoof
 *
 * This file is in the public domain.
 */

/*
 * $Log: asm-386.S,v $
 * Revision 1.8  1997/12/17 00:50:51  brianp
 * applied Josh's patch to fix texture coordinate transformation bugs
 *
 * Revision 1.7  1997/12/17 00:27:11  brianp
 * applied Josh's patch to fix bfris
 *
 * Revision 1.6  1997/12/01 01:02:41  brianp
 * added FreeBSD patches (Daniel J. O'Connor)
 *
 * Revision 1.5  1997/11/19 23:52:17  brianp
 * added missing "cld" instruction in asm_transform_points4_identity()
 *
 * Revision 1.4  1997/11/11 02:22:41  brianp
 * small change per Josh to ensure U/V pairing
 *
 * Revision 1.3  1997/11/07 03:37:24  brianp
 * added missing line from Stephane Rehel
 *
 * Revision 1.2  1997/11/07 03:30:37  brianp
 * added Josh's 11-5-97 patches
 *
 * Revision 1.1  1997/10/30 06:00:33  brianp
 * Initial revision
 */

#include <asm.inc>

#define S(x)    dword ptr [esi + 4*x]
#define D(x)    dword ptr [edi + 4*x]
#define M(x, y) dword ptr [edx + 16*x + 4*y]

.code

/*
 * void asm_transform_points3_general( GLuint n, GLfloat d[][4],
 *                                     GLfloat m[16], GLfloat s[][4] );
 */
PUBLIC _asm_transform_points3_general
_asm_transform_points3_general:
.align 4
	push esi
	push edi

	mov ecx, [esp + 12]	    /* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points3_general_end

.align 4
_asm_transform_points3_general_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(0)
	fmul M(0, 2)
	fld S(0)
	fmul M(0, 3)

	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(1)
	fmul M(1, 2)
	fld S(1)
	fmul M(1, 3)

	/*
	 * The FPU stack should now look like this:
	 *
	 * st(7) = S(0) * M(0, 0)
	 * st(6) = S(0) * M(0, 1)
	 * st(5) = S(0) * M(0, 2)
	 * st(4) = S(0) * M(0, 3)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(1) * M(1, 2)
	 * st(0) = S(1) * M(1, 3)
	 */

	fxch st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp st(7), st 	/* 1 2 0 4 5 6 7 */
	fxch st(1)		/* 2 1 0 4 5 6 7 */
	faddp st(5), st 	/* 1 0 4 5 6 7 */
	faddp st(3), st 	/* 0 4 5 6 7 */
	faddp st(1), st  	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 */

	fld S(2)
	fmul M(2, 0)
	fld S(2)
	fmul M(2, 1)
	fld S(2)
	fmul M(2, 2)
	fld S(2)
	fmul M(2, 3)

	/*
	 * st(7) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(6) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(5) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(4) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 * st(3) = S(2) * M(2, 0)
	 * st(2) = S(2) * M(2, 1)
	 * st(1) = S(2) * M(2, 2)
	 * st(0) = S(2) * M(2, 3)
	 */

	fxch st(3)          /* 3 1 2 0 4 5 6 7 */
	faddp st(7), st 	/* 1 2 0 4 5 6 7 */
	fxch st(1)          /* 2 1 0 4 5 6 7 */
	faddp st(5), st     /* 1 0 4 5 6 7 */
	faddp st(3), st     /* 0 4 5 6 7 */
	faddp st(1), st     /* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3)
	 */

	fxch st(3) 	/* 3 1 2 0 */
	fadd M(3, 0)
	fxch st(2) 	/* 2 1 3 0 */
	fadd M(3, 1)
	fxch st(1) 	/* 1 2 3 0 */
	fadd M(3, 2)
	fxch st(3) 	/* 0 2 3 1 */
	fadd M(3, 3)

	/*
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2) + M(3, 2)
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0) + M(3, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1) + M(3, 1)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3) + M(3, 3)
	 */

	fxch st(3) 	/* 3 1 2 0 */
	fstp D(2) 	/* 1 2 0 */
	fxch st(1) 	/* 2 1 0 */
	fstp D(0) 	/* 1 0 */
	lea esi, S(4)
	fstp D(1) 	/* 0 */
	dec ecx
	fstp D(3) 	/* */

	lea edi, D(4)

	jnz _asm_transform_points3_general_loop

_asm_transform_points3_general_end:
	pop edi
	pop esi
	ret


/*
 * void asm_transform_points3_identity( GLuint n, GLfloat d[][4],
 *                                      GLfloat s[][4] );
 */
PUBLIC _asm_transform_points3_identity
_asm_transform_points3_identity:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov esi, [esp + 20] 	/* esi = s */
	push ebx
	push ebp

	test ecx, ecx
	jz _asm_transform_points3_identity_end

	mov ebp, HEX(3f800000)

.align 4
_asm_transform_points3_identity_loop:
	mov eax, S(0)
	mov edx, S(1)
	mov ebx, S(2)
	lea esi, S(4)
	mov D(0), eax
	mov D(1), edx
	mov D(2), ebx
	mov D(3), ebp
	dec ecx
	lea edi, D(4)
	jnz _asm_transform_points3_identity_loop

_asm_transform_points3_identity_end:
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret


/*
 * void asm_transform_points3_2d( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                GLfloat s[][4] );
 */
PUBLIC _asm_transform_points3_2d
_asm_transform_points3_2d:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */
	push ebp

	mov ebp, HEX(3f800000)

	test cl, DEC(1)
	jz _asm_transform_points3_2d_step

	dec ecx

	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)

	/*
	 * st(3) = S(0) * M(0, 0)
	 * st(2) = S(0) * M(0, 1)
	 * st(1) = S(1) * M(1, 0)
	 * st(0) = S(1) * M(1, 1)
	 */

	fxch st(1) 	/* 1 0 2 3 */
	fadd M(3, 0)
	fxch st(1) 	/* 0 1 2 3 */
	fadd M(3, 1)
	fxch st(1) 	/* 1 0 2 3 */
	faddp st(3), st 	/* 0 2 3 */
	faddp st(1), st 	/* 2 3 */
	fstp D(1) 	/* 3 */
	fstp D(0) 	/* */
	mov eax, S(2)
	lea esi, S(4)
	mov D(3), ebp
	mov D(2), eax
	lea edi, D(4)

_asm_transform_points3_2d_step:
	test ecx, ecx
	jz _asm_transform_points3_2d_end

.align 4
_asm_transform_points3_2d_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(4)
	fmul M(0, 0)
	fld S(4)
	fmul M(0, 1)
	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(5)
	fmul M(1, 0)
	fld S(5)
	fmul M(1, 1)

	/*
	 * st(7) = S(0) * M(0, 0)
	 * st(6) = S(0) * M(0, 1)
	 * st(5) = S(4) * M(0, 0)
	 * st(4) = S(4) * M(0, 1)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(5) * M(1, 0)
	 * st(0) = S(5) * M(1, 1)
	 */

	fxch st(7) 	/* 7 1 2 3 4 5 6 0 */
	fadd M(3, 0)
	fxch st(6) 	/* 6 1 2 3 4 5 7 0 */
	fadd M(3, 1)
	fxch st(5) 	/* 5 1 2 3 4 6 7 0 */
	fadd M(3, 0)
	fxch st(4) 	/* 4 1 2 3 5 6 7 0 */
	fadd M(3, 1)

	mov eax, S(2)
	mov D(3), ebp
	mov D(2), eax
	mov eax, S(6)
	mov D(7), ebp
	mov D(6), eax
	lea esi, S(8)
	sub ecx, DEC(2)

	/*
	 * st(7) = S(5) * M(1, 1)
	 * st(6) = S(0) * M(0, 0) + M(3, 0)
	 * st(5) = S(0) * M(0, 1) + M(3, 1)
	 * st(4) = S(4) * M(0, 0) + M(3, 0)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(5) * M(1, 0)
	 * st(0) = S(4) * M(0, 1) + M(3, 1)
	 */

	faddp st(7), st 	/* 1 2 3 4 5 6 7 */
	faddp st(3), st 	/* 2 3 4 5 6 7 */
	faddp st(3), st 	/* 3 4 5 6 7 */
	faddp st(3), st 	/* 4 5 6 7 */
	fxch st(3) 	/* 7 5 6 4 */
	fstp D(5) 	/* 5 6 4 */
	fstp D(1) 	/* 6 4 */
	fstp D(0) 	/* 4 */
	fstp D(4) 	/* */

	lea edi, D(8)
	jnz _asm_transform_points3_2d_loop

_asm_transform_points3_2d_end:
	pop ebp
	pop edi
	pop esi
	ret


/*
 * void asm_transform_points3_2d_no_rot( GLuint n, GLfloat d[][4],
 *                                       GLfloat m[16], GLfloat s[][4] );
 *
 */
PUBLIC _asm_transform_points3_2d_no_rot
_asm_transform_points3_2d_no_rot:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */
	push ebp

	test ecx, ecx
	jz _asm_transform_points3_2d_no_rot_end

	mov ebp, HEX(3f800000)

.align 4
_asm_transform_points3_2d_no_rot_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(1)
	fmul M(1, 1)
	fxch st(1)
	fadd M(3, 0)
	fxch st(1)
	fadd M(3, 1)
	fxch st(1)
	fstp D(0)
	fstp D(1)

	mov eax, S(2)   /* cycle 1: U pipe */
	mov D(3), ebp   /*          V pipe */
	mov D(2), eax   /* cycle 2: U pipe */

	dec ecx
	lea esi, S(4)
	lea edi, D(4)
	jnz _asm_transform_points3_2d_no_rot_loop

_asm_transform_points3_2d_no_rot_end:
	pop ebp
	pop edi
	pop esi
	ret



/*
 * void asm_transform_points3_3d( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                GLfloat s[][4] );
 */
PUBLIC _asm_transform_points3_3d
_asm_transform_points3_3d:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points3_3d_end

	mov eax, HEX(3f800000)

.align 4
_asm_transform_points3_3d_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(0)
	fmul M(0, 2)

	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(1)
	fmul M(1, 2)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(0, 1)
	 * st(3) = S(0) * M(0, 2)
	 * st(2) = S(1) * M(1, 0)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(1, 2)
	 */

	fxch st(2) 		/* 2 1 0 3 4 5 */
	faddp st(5), st	/* 1 0 3 4 5 */
	faddp st(3), st /* 0 3 4 5 */
	faddp st(1), st /* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 */

	fld S(2)
	fmul M(2, 0)
	fld S(2)
	fmul M(2, 1)
	fld S(2)
	fmul M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(4) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(2) = S(2) * M(2, 0)
	 * st(1) = S(2) * M(2, 1)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch st(2) 		/* 2 1 0 3 4 5 */
	faddp st(5), st	/* 1 0 3 4 5 */
	faddp st(3), st	/* 0 3 4 5 */
	faddp st(1), st	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 */

	fxch st(2) 	/* 2 1 0 */
	fadd M(3, 0)
	fxch st(1) 	/* 1 2 0 */
	fadd M(3, 1)
	fxch st(2) 	/* 0 2 1 */
	fadd M(3, 2)

	fxch st(1) 	/* 2 0 1 */
	fstp D(0) 	/* 0 1 */
	fstp D(2) 	/* 1 */
	fstp D(1) 	/* */
	mov D(3), eax

	lea esi, S(4)
	dec ecx

	lea edi, D(4)

	jnz _asm_transform_points3_3d_loop

_asm_transform_points3_3d_end:
	pop edi
	pop esi
	ret



/*
 * void asm_transform_points4_general( GLuint n, GLfloat d[][4],
 *                                     GLfloat m[16], GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_general
_asm_transform_points4_general:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points4_general_end

.align 4
_asm_transform_points4_general_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(0)
	fmul M(0, 2)
	fld S(0)
	fmul M(0, 3)

	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(1)
	fmul M(1, 2)
	fld S(1)
	fmul M(1, 3)

	/*
	 * st(7) = S(0) * M(0, 0)
	 * st(6) = S(0) * M(0, 1)
	 * st(5) = S(0) * M(0, 2)
	 * st(4) = S(0) * M(0, 3)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(1) * M(1, 2)
	 * st(0) = S(1) * M(1, 3)
	 */

	fxch st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp st(7), st /* 1 2 0 4 5 6 7 */
	fxch st(1)		/* 2 1 0 4 5 6 7 */
	faddp st(5), st	/* 1 0 4 5 6 7 */
	faddp st(3), st	/* 0 4 5 6 7 */
	faddp st(1), st	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 */

	fld S(2)
	fmul M(2, 0)
	fld S(2)
	fmul M(2, 1)
	fld S(2)
	fmul M(2, 2)
	fld S(2)
	fmul M(2, 3)

	/*
	 * st(7) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(6) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(5) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(4) = S(0) * M(0, 3) + S(1) * M(1, 3)
	 * st(3) = S(2) * M(2, 0)
	 * st(2) = S(2) * M(2, 1)
	 * st(1) = S(2) * M(2, 2)
	 * st(0) = S(2) * M(2, 3)
	 */

	fxch st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp st(7), st	/* 1 2 0 4 5 6 7 */
	fxch st(1)		/* 2 1 0 4 5 6 7 */
	faddp st(5), st	/* 1 0 4 5 6 7 */
	faddp st(3), st	/* 0 4 5 6 7 */
	faddp st(1), st	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3)
	 */

	fld S(3)
	fmul M(3, 0)
	fld S(3)
	fmul M(3, 1)
	fld S(3)
	fmul M(3, 2)
	fld S(3)
	fmul M(3, 3)

	/*
	 * st(7) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(6) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(5) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(4) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3)
	 * st(3) = S(3) * M(3, 0)
	 * st(2) = S(3) * M(3, 1)
	 * st(1) = S(3) * M(3, 2)
	 * st(0) = S(3) * M(3, 3)
	 */

	fxch st(3)		/* 3 1 2 0 4 5 6 7 */
	faddp st(7), st	/* 1 2 0 4 5 6 7 */
	fxch st(1)		/* 2 1 0 4 5 6 7 */
	faddp st(5), st	/* 1 0 4 5 6 7 */
	faddp st(3), st	/* 0 4 5 6 7 */

	lea esi, S(4)
	dec ecx

	faddp st(1), st 	/* 4 5 6 7 */

	/*
	 * st(3) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0) + S(3) * M(3, 0)
	 * st(2) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1) + S(3) * M(3, 1)
	 * st(1) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2) + S(3) * M(3, 2)
	 * st(0) = S(0) * M(0, 3) + S(1) * M(1, 3) + S(2) * M(2, 3) + S(3) * M(3, 3)
	 */

	fxch st(3) 	/* 3 1 2 0 */
	fstp D(0) 	/* 1 2 0 */
	fxch st(1) 	/* 2 1 0 */
	fstp D(1) 	/* 1 0 */
	fstp D(2) 	/* 0 */
	fstp D(3) 	/* */

	lea edi, D(4)

	jnz _asm_transform_points4_general_loop

_asm_transform_points4_general_end:
	pop edi
	pop esi
	ret



/*
 * void asm_transform_points4_identity( GLuint n, GLfloat d[][4],
 *                                      GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_identity
_asm_transform_points4_identity:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov esi, [esp + 20] 	/* esi = s */

	lea ecx,  [ecx * 4]

	cld
	rep movsd

	pop edi
	pop esi
	ret



/*
 * void asm_transform_points4_2d( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_2d
_asm_transform_points4_2d:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points4_2d_end

	push ebx

.align 4
_asm_transform_points4_2d_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(3)
	fmul M(3, 0)
	fld S(3)
	fmul M(3, 1)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(0, 1)
	 * st(3) = S(1) * M(1, 0)
	 * st(2) = S(1) * M(1, 1)
	 * st(1) = S(3) * M(3, 0)
	 * st(0) = S(3) * M(3, 1)
	 */

	mov eax, S(2)
	mov ebx, S(3)
	lea esi, S(4)
	dec ecx
	mov D(2), eax
	mov D(3), ebx
	faddp st(4), st
	faddp st(4), st
	faddp st(2), st
	faddp st(2), st
	fstp D(1)
	fstp D(0)
	lea edi, D(4)
	jnz _asm_transform_points4_2d_loop

	pop ebx

_asm_transform_points4_2d_end:
	pop edi
	pop esi
	ret



/*
 * void asm_transform_points4_2d_no_rot( GLuint n, GLfloat d[][4],
 *                                       GLfloat m[16], GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_2d_no_rot
_asm_transform_points4_2d_no_rot:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points4_2d_no_rot_end
	push ebx

.align 4
_asm_transform_points4_2d_no_rot_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(3)
	fmul M(3, 0)
	fld S(3)
	fmul M(3, 1)
	mov eax, S(2)
	mov ebx, S(3)
	lea esi, S(4)
	dec ecx
	mov D(2), eax
	mov D(3), ebx
	faddp st(2), st
	faddp st(2), st
	fstp D(1)
	fstp D(0)
	lea edi, D(4)
	jnz _asm_transform_points4_2d_no_rot_loop

	pop ebx

_asm_transform_points4_2d_no_rot_end:
	pop edi
	pop esi
	ret



/*
 * void asm_transform_points4_3d( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_3d
_asm_transform_points4_3d:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points4_3d_end

.align 4
_asm_transform_points4_3d_loop:
	fld S(3)

	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(0, 1)
	fld S(0)
	fmul M(0, 2)

	fld S(1)
	fmul M(1, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(1)
	fmul M(1, 2)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(0, 1)
	 * st(3) = S(0) * M(0, 2)
	 * st(2) = S(1) * M(1, 0)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(1, 2)
	 */

	fxch st(2) 		/* 2 1 0 3 4 5 */
	faddp st(5), st 	/* 1 0 3 4 5 */
	faddp st(3), st 	/* 0 3 4 5 */
	faddp st(1), st 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 */

	fld S(2)
	fmul M(2, 0)
	fld S(2)
	fmul M(2, 1)
	fld S(2)
	fmul M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(1, 0)
	 * st(4) = S(0) * M(0, 1) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2)
	 * st(2) = S(2) * M(2, 0)
	 * st(1) = S(2) * M(2, 1)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch st(2) 		/* 2 1 0 3 4 5 */
	faddp st(5), st 	/* 1 0 3 4 5 */
	faddp st(3), st 	/* 0 3 4 5 */
	faddp st(1), st 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 */

	fld S(3)
	fmul M(3, 0)
	fld S(3)
	fmul M(3, 1)
	fld S(3)
	fmul M(3, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0)
	 * st(4) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1)
	 * st(3) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2)
	 * st(2) = S(3) * M(3, 0)
	 * st(1) = S(3) * M(3, 1)
	 * st(0) = S(3) * M(3, 2)
	 */

	fxch st(2)      /* 2 1 0 3 4 5 */
	faddp st(5), st	/* 1 0 3 4 5 */
	faddp st(3), st	/* 0 3 4 5 */

	lea esi, S(4)
	dec ecx

	faddp st(1), st 	/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(1, 0) + S(2) * M(2, 0) + S(3) * M(3, 0)
	 * st(1) = S(0) * M(0, 1) + S(1) * M(1, 1) + S(2) * M(2, 1) + S(3) * M(3, 1)
	 * st(0) = S(0) * M(0, 2) + S(1) * M(1, 2) + S(2) * M(2, 2) + S(3) * M(3, 2)
	 */

	fxch st(2) 	/* 2 1 0 */
	fstp D(0) 	/* 1 0 */
	fstp D(1) 	/* 0 */
	fstp D(2) 	/* */
	fstp D(3)

	lea edi, D(4)

	jnz _asm_transform_points4_3d_loop

_asm_transform_points4_3d_end:
	pop edi
	pop esi
	ret

/*
 * void asm_transform_points4_ortho( GLuint n, GLfloat d[][4],
 *                                   GLfloat m[16], GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_ortho
_asm_transform_points4_ortho:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points4_ortho_end

.align 4
_asm_transform_points4_ortho_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(2)
	fmul M(2, 2)

	fld S(3)
	fmul M(3, 0)
	fld S(3)
	fmul M(3, 1)
	fld S(3)
	fmul M(3, 2)

	mov eax, S(3)
	lea esi, S(4)
	dec ecx
	mov D(3), eax

	faddp st(3), st
	faddp st(3), st
	faddp st(3), st

	fstp D(2)
	fstp D(1)
	fstp D(0)

	lea edi, D(4)
	jnz _asm_transform_points4_ortho_loop

_asm_transform_points4_ortho_end:
	pop edi
	pop esi
	ret

/*
 * void asm_transform_points4_perspective( GLuint n, GLfloat d[][4],
 *                                         GLfloat m[16], GLfloat s[][4] );
 */
PUBLIC _asm_transform_points4_perspective
_asm_transform_points4_perspective:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _asm_transform_points4_perspective_end

.align 4
_asm_transform_points4_perspective_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(1)
	fmul M(1, 1)
	fld S(2)
	fmul M(2, 2)

	fld S(2)
	fmul M(2, 0)
	fld S(2)
	fmul M(2, 1)
	fld S(3)
	fmul M(3, 2)

	mov eax, S(2)
	lea esi, S(4)
	xor eax, HEX(80000000)
	dec ecx

	faddp st(3), st
	faddp st(3), st
	faddp st(3), st

	fstp D(2)
	fstp D(1)
	fstp D(0)

	mov D(3), eax
	lea edi, D(4)
	jnz _asm_transform_points4_perspective_loop

_asm_transform_points4_perspective_end:
	pop edi
	pop esi
	ret



/*
 * Table for clip test.
 *
 * 	bit6 = S(3) < 0
 * 	bit5 = S(2) < 0
 * 	bit4 = abs(S(2)) > abs(S(3))
 * 	bit3 = S(1) < 0
 * 	bit2 = abs(S(1)) > abs(S(3))
 * 	bit1 = S(0) < 0
 * 	bit0 = abs(S(0)) > abs(S(3))
 */

/* Vertex buffer clipping flags (from vb.h) */
#if 0

#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_USER_BIT    0x40
#define CLIP_ALL_BITS    0x3f

#define MAGN_X(i) 	(~(((i) & 1) - 1))
#define SIGN_X(i) 	(~((((i) >> 1) & 1) - 1))
#define MAGN_Y(i) 	(~((((i) >> 2) & 1) - 1))
#define SIGN_Y(i) 	(~((((i) >> 3) & 1) - 1))
#define MAGN_Z(i) 	(~((((i) >> 4) & 1) - 1))
#define SIGN_Z(i) 	(~((((i) >> 5) & 1) - 1))
#define SIGN_W(i) 	(~((((i) >> 6) & 1) - 1))

#define CLIP_VALUE(i) 						\
	 (CLIP_RIGHT_BIT 					\
	  & ((~SIGN_X(i) & SIGN_W(i)) 				\
	     | (~SIGN_X(i) & ~SIGN_W(i) & MAGN_X(i)) 		\
	     | (SIGN_X(i) & SIGN_W(i) & ~MAGN_X(i)))) 		\
	 | (CLIP_LEFT_BIT 					\
	    & ((SIGN_X(i) & SIGN_W(i)) 				\
	       | (~SIGN_X(i) & SIGN_W(i) & ~MAGN_X(i)) 		\
	       | (SIGN_X(i) & ~SIGN_W(i) & MAGN_X(i)))) 	\
	 | (CLIP_TOP_BIT 					\
	    & ((~SIGN_Y(i) & SIGN_W(i)) 			\
	       | (~SIGN_Y(i) & ~SIGN_W(i) & MAGN_Y(i)) 		\
	       | (SIGN_Y(i) & SIGN_W(i) & ~MAGN_Y(i)))) 	\
	 | (CLIP_BOTTOM_BIT 					\
	    & ((SIGN_Y(i) & SIGN_W(i)) 				\
	       | (~SIGN_Y(i) & SIGN_W(i) & ~MAGN_Y(i)) 		\
	       | (SIGN_Y(i) & ~SIGN_W(i) & MAGN_Y(i)))) 	\
	 | (CLIP_FAR_BIT 					\
	    & ((~SIGN_Z(i) & SIGN_W(i)) 			\
	       | (~SIGN_Z(i) & ~SIGN_W(i) & MAGN_Z(i)) 		\
	       | (SIGN_Z(i) & SIGN_W(i) & ~MAGN_Z(i)))) 	\
	 | (CLIP_NEAR_BIT 					\
	    & ((SIGN_Z(i) & SIGN_W(i)) 				\
	       | (~SIGN_Z(i) & SIGN_W(i) & ~MAGN_Z(i)) 		\
	       | (SIGN_Z(i) & ~SIGN_W(i) & MAGN_Z(i))))

#define CLIP_VALUE8(i) \
	CLIP_VALUE(i + 0), CLIP_VALUE(i + 1), CLIP_VALUE(i + 2), CLIP_VALUE(i + 3), \
	CLIP_VALUE(i + 4), CLIP_VALUE(i + 5), CLIP_VALUE(i + 6), CLIP_VALUE(i + 7)

.rodata

clip_table:
	.byte CLIP_VALUE8(0x00)
	.byte CLIP_VALUE8(0x08)
	.byte CLIP_VALUE8(0x10)
	.byte CLIP_VALUE8(0x18)
	.byte CLIP_VALUE8(0x20)
	.byte CLIP_VALUE8(0x28)
	.byte CLIP_VALUE8(0x30)
	.byte CLIP_VALUE8(0x38)
	.byte CLIP_VALUE8(0x40)
	.byte CLIP_VALUE8(0x48)
	.byte CLIP_VALUE8(0x50)
	.byte CLIP_VALUE8(0x58)
	.byte CLIP_VALUE8(0x60)
	.byte CLIP_VALUE8(0x68)
	.byte CLIP_VALUE8(0x70)
	.byte CLIP_VALUE8(0x78)
#else

.const
ASSUME NOTHING

clip_table:
	.byte HEX(0), HEX(1), HEX(0), HEX(2), HEX(4), HEX(5), HEX(4), HEX(6)
	.byte HEX(0), HEX(1), HEX(0), HEX(2), HEX(8), HEX(9), HEX(8), HEX(a)
	.byte HEX(20), HEX(21), HEX(20), HEX(22), HEX(24), HEX(25), HEX(24), HEX(26)
	.byte HEX(20), HEX(21), HEX(20), HEX(22), HEX(28), HEX(29), HEX(28), HEX(2a)
	.byte HEX(0), HEX(1), HEX(0), HEX(2), HEX(4), HEX(5), HEX(4), HEX(6)
	.byte HEX(0), HEX(1), HEX(0), HEX(2), HEX(8), HEX(9), HEX(8), HEX(a)
	.byte HEX(10), HEX(11), HEX(10), HEX(12), HEX(14), HEX(15), HEX(14), HEX(16)
	.byte HEX(10), HEX(11), HEX(10), HEX(12), HEX(18), HEX(19), HEX(18), HEX(1a)
	.byte HEX(3f), HEX(3d), HEX(3f), HEX(3e), HEX(37), HEX(35), HEX(37), HEX(36)
	.byte HEX(3f), HEX(3d), HEX(3f), HEX(3e), HEX(3b), HEX(39), HEX(3b), HEX(3a)
	.byte HEX(2f), HEX(2d), HEX(2f), HEX(2e), HEX(27), HEX(25), HEX(27), HEX(26)
	.byte HEX(2f), HEX(2d), HEX(2f), HEX(2e), HEX(2b), HEX(29), HEX(2b), HEX(2a)
	.byte HEX(3f), HEX(3d), HEX(3f), HEX(3e), HEX(37), HEX(35), HEX(37), HEX(36)
	.byte HEX(3f), HEX(3d), HEX(3f), HEX(3e), HEX(3b), HEX(39), HEX(3b), HEX(3a)
	.byte HEX(1f), HEX(1d), HEX(1f), HEX(1e), HEX(17), HEX(15), HEX(17), HEX(16)
	.byte HEX(1f), HEX(1d), HEX(1f), HEX(1e), HEX(1b), HEX(19), HEX(1b), HEX(1a)

#endif

.code

/*
 * cliptest -
 *
 * inputs:
 * 	ecx = # points
 * 	esi = points
 * 	edi = clipmask[]
 *
 * inputs/outputs:
 * 	al = ormask
 * 	ah = andmask
 */

cliptest:
	test ecx, ecx
	jz cliptest_end

	push ebp
	push ebx

.align 4
cliptest_loop:
	mov ebp, S(3)
	mov ebx, S(2)

	xor edx, edx
	add ebp, ebp	/* %ebp = abs(S(3))*2 ; carry = sign of S(3) */

	adc edx, edx
	add ebx, ebx	/* %ebx = abs(S(2))*2 ; carry = sign of S(2) */

	adc edx, edx
	cmp ebp, ebx	/* carry = abs(S(2))*2 > abs(S(3))*2 */

	adc edx, edx
	mov ebx, S(1)

	add ebx, ebx	/* %ebx = abs(S(1))*2 ; carry = sign of S(1) */

	adc edx, edx
	cmp ebp, ebx	/* carry = abs(S(1))*2 > abs(S(3))*2 */

	adc edx, edx
	mov ebx, S(0)

	add ebx, ebx	/* %ebx = abs(S(0))*2 ; carry = sign of S(0) */

	adc edx, edx
	cmp ebp, ebx	/* carry = abs(S(0))*2 > abs(S(3))*2 */

	adc edx, edx

	lea esi, S(4)

	mov bl, byte ptr [edi]
	mov dl, byte ptr [clip_table + edx]

	or bl, dl
	or al, dl

	and ah, dl
	mov [edi], bl

	inc edi
	dec ecx

	jnz cliptest_loop

	pop ebx
	pop ebp
cliptest_end:
	ret

/*
 * void asm_project_and_cliptest_general( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                        GLfloat s[][4], GLubyte clipmask[],
 *                                        GLubyte *ormask, GLubyte *andmask );
 */
PUBLIC _asm_project_and_cliptest_general
_asm_project_and_cliptest_general:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	push esi
	push edx
	push edi
	push ecx
	call _asm_transform_points4_general
	add esp, DEC(16)

	mov edi, [esp + 32] 	/* ormask */
	mov esi, [esp + 36] 	/* andmask */
	mov al, [edi]
	mov ah, [esi]

	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 28] 	/* edi = clipmask */
	mov esi, [esp + 16] 	/* esi = d */

	call cliptest

	mov edi, [esp + 32] 	/* ormask */
	mov esi, [esp + 36] 	/* andmask */
	mov [edi], al
	mov [esi], ah

	pop edi
	pop esi
	ret


/*
 * void asm_project_and_cliptest_identity( GLuint n, GLfloat d[][4],
 *                                         GLfloat s[][4], GLubyte clipmask[],
 *                                         GLubyte *ormask, GLubyte *andmask );
 */
PUBLIC _asm_project_and_cliptest_identity
_asm_project_and_cliptest_identity:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov esi, [esp + 20] 	/* esi = s */

	push esi
	push edi
	push ecx

	call _asm_transform_points4_identity

	add esp, DEC(12)

	mov edi, [esp + 28] 	/* ormask */
	mov esi, [esp + 32] 	/* andmask */
	mov al, [edi]
	mov ah, [esi]

	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 24] 	/* edi = clipmask */
	mov esi, [esp + 16] 	/* esi = d */

	call cliptest

	mov edi, [esp + 28] 	/* ormask */
	mov esi, [esp + 32] 	/* andmask */
	mov [edi], al
	mov [esi], ah

	pop edi
	pop esi
	ret

/*
 * void asm_project_and_cliptest_ortho( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                      GLfloat s[][4], GLubyte clipmask[],
 *                                      GLubyte *ormask, GLubyte *andmask );
 */
PUBLIC _asm_project_and_cliptest_ortho
_asm_project_and_cliptest_ortho:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	push esi
	push edx
	push edi
	push ecx

	call _asm_transform_points4_ortho

	add esp, DEC(16)

	mov edi, [esp + 32] 	/* ormask */
	mov esi, [esp + 36] 	/* andmask */
	mov al, [edi]
	mov ah, [esi]

	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 28] 	/* edi = clipmask */
	mov esi, [esp + 16] 	/* esi = d */

	call cliptest

	mov edi, [esp + 32] 	/* ormask */
	mov esi, [esp + 36] 	/* andmask */
	mov [edi], al
	mov [esi], ah

	pop edi
	pop esi
	ret

/*
 * void asm_project_and_cliptest_perspective( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                                            GLfloat s[][4], GLubyte clipmask[],
 *                                            GLubyte *ormask, GLubyte *andmask );
 */
PUBLIC _asm_project_and_cliptest_perspective
_asm_project_and_cliptest_perspective:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	push esi
	push edx
	push edi
	push ecx

	call _asm_transform_points4_perspective

	add esp, DEC(16)

	mov edi, [esp + 32] 	/* ormask */
	mov esi, [esp + 36] 	/* andmask */
	mov al, [edi]
	mov ah, [esi]

	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 28] 	/* edi = clipmask */
	mov esi, [esp + 16] 	/* esi = d */

	call cliptest

	mov edi, [esp + 32]     /* ormask */
	mov esi, [esp + 36]		/* andmask */
	mov byte ptr [edi], al
	mov byte ptr [esi], ah

	pop edi
	pop esi
	ret


/*
 * unsigned int inverse_nofp( float f );
 *
 * Calculate the inverse of a float without using the FPU.
 * This function returns a float in eax, so it's return
 * type should be 'int' when called from C (and converted
 * to float with pointer/union abuse).
 */
.align 4
inverse_nofp:

	/* get mantissa in eax */
	mov ecx, [esp + 4]
	and ecx, HEX(7fffff)

	/* set implicit integer */
	or ecx, HEX(800000)

	/* div 0x10000:0x00000000 by mantissa */
	xor eax, eax
	mov edx, HEX(10000)

	div ecx
	
	/* round result */
	shr eax, DEC(1)
	adc eax, DEC(0)

	/* get exponent in ecx */
	mov ecx, HEX(7f800000)
	mov edx, [esp + 4]
	and ecx, edx

	/* negate exponent and decrement it */
	mov edx, HEX(7E800000)
	sub edx, ecx

	/* if bit 24 is set, shift and adjust exponent */
	test eax, HEX(1000000)
	jz inverse_nofp_combine

	shr eax, HEX(1)
	add edx, HEX(800000)

	/* combine mantissa and exponent, then set sign */
inverse_nofp_combine:
	and eax, HEX(7fffff)
	mov ecx, [esp + 4]
	or eax, edx
	and ecx, HEX(80000000)
	or eax, ecx

	ret


/*
 * void gl_xform_normals_3fv( GLuint n, GLfloat d[][4], GLfloat m[16],
 *                             GLfloat s[][4], GLboolean normalize );
 */
PUBLIC _gl_xform_normals_3fv
_gl_xform_normals_3fv:
.align 4
	push esi
	push edi
	mov ecx, [esp + 12] 	/* ecx = n */
	mov edi, [esp + 16] 	/* edi = d */
	mov edx, [esp + 20] 	/* edx = m */
	mov esi, [esp + 24] 	/* esi = s */

	test ecx, ecx
	jz _gl_xform_normals_3fv_end

.align 4
_gl_xform_normals_3fv_loop:
	fld S(0)
	fmul M(0, 0)
	fld S(0)
	fmul M(1, 0)
	fld S(0)
	fmul M(2, 0)

	fld S(1)
	fmul M(0, 1)
	fld S(1)
	fmul M(1, 1)
	fld S(1)
	fmul M(2, 1)

	/*
	 * st(5) = S(0) * M(0, 0)
	 * st(4) = S(0) * M(1, 0)
	 * st(3) = S(0) * M(2, 0)
	 * st(2) = S(1) * M(0, 1)
	 * st(1) = S(1) * M(1, 1)
	 * st(0) = S(1) * M(2, 1)
	 */

	fxch st(2)			/* 2 1 0 3 4 5 */
	faddp st(5), st		/* 1 0 3 4 5 */
	faddp st(3), st		/* 0 3 4 5 */
	faddp st(1), st		/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(0, 1)
	 * st(1) = S(0) * M(1, 0) + S(1) * M(1, 1)
	 * st(0) = S(0) * M(2, 0) + S(1) * M(2, 1)
	 */

	fld S(2)
	fmul M(0, 2)
	fld S(2)
	fmul M(1, 2)
	fld S(2)
	fmul M(2, 2)

	/*
	 * st(5) = S(0) * M(0, 0) + S(1) * M(0, 1)
	 * st(4) = S(0) * M(1, 0) + S(1) * M(1, 1)
	 * st(3) = S(0) * M(2, 0) + S(1) * M(2, 1)
	 * st(2) = S(2) * M(0, 2)
	 * st(1) = S(2) * M(1, 2)
	 * st(0) = S(2) * M(2, 2)
	 */

	fxch st(2)			/* 2 1 0 3 4 5 */
	faddp st(5), st		/* 1 0 3 4 5 */
	faddp st(3), st		/* 0 3 4 5 */
	faddp st(1), st		/* 3 4 5 */

	/*
	 * st(2) = S(0) * M(0, 0) + S(1) * M(0, 1) + S(2) * M(0, 2)
	 * st(1) = S(0) * M(1, 0) + S(1) * M(1, 1) + S(2) * M(1, 2)
	 * st(0) = S(0) * M(2, 0) + S(1) * M(2, 1) + S(2) * M(2, 2)
	 */

	fxch st(2) 	/* 2 1 0 */
	fstp D(0) 	/* 1 0 */
	fstp D(1) 	/* 0 */
	fstp D(2) 	/* */

	lea esi, S(3)

	dec ecx
	lea edi, D(3)

	jnz _gl_xform_normals_3fv_loop

	/*
	 * Skip normalize if it isn't needed
	 */
	cmp dword ptr [esp + 28], DEC(0)
	jz _gl_xform_normals_3fv_end

	/* Normalize required */

	mov esi, [esp + 12]		/* esi = n */
	mov edi, [esp + 16]		/* edi = d */

	sub esp, DEC(4)	/* temp var for 1.0 / len */

	/*
	 * (%esp) = length of first normal
	 */
	fld D(0)
	fmul D(0)
	fld D(1)
	fmul D(1)
	fld D(2)
	fmul D(2)
	fxch st(2)
	faddp st(1), st
	faddp st(1), st
	fsqrt
	fstp dword ptr [esp]

	jmp _gl_xform_normals_3fv_loop2_end

.align 4
_gl_xform_normals_3fv_loop2:
	/* %st(0) = length of next normal */
	fld D(3)
	fmul D(3)
	fld D(4)
	fmul D(4)
	fld D(5)
	fmul D(5)
	fxch st(2)
	faddp st(1), st
	faddp st(1), st
	fsqrt

	/*
	 * inverse the length of the current normal, which is
	 * already at (%esp).  This should overlap the prev
	 * fsqrt nicely.
	 */
	call inverse_nofp
	mov [esp], eax

	/* multiply normal by 1/len */
	fld D(0)
	fmul dword ptr [esp]
	fld D(1)
	fmul dword ptr [esp]
	fld D(2)
	fmul dword ptr [esp]
	fxch st(3)
	fstp dword ptr [esp] 	/* store length of next normal */
	fstp D(1)
	fstp D(0)
	fstp D(2)
	lea edi, D(3)

_gl_xform_normals_3fv_loop2_end:
	dec esi
	jnz _gl_xform_normals_3fv_loop2

	/* finish up the last normal */
	call inverse_nofp
	mov [esp], eax
	fld D(0)
	fmul dword ptr [esp]
	fld D(1)
	fmul dword ptr [esp]
	fld D(2)
	fmul dword ptr [esp]
	fxch st(2)
	fstp D(0)
	fstp D(1)
	fstp D(2)

	add esp, DEC(4)

_gl_xform_normals_3fv_end:
	pop edi
	pop esi
	ret

END
