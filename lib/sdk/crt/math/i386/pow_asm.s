/* ix87 specific implementation of pow function.
   Copyright (C) 1996, 1997, 1998, 1999, 2001, 2004, 2005, 2007
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* ReactOS modifications */
#include <asm.inc>

#define ALIGNARG(log2) log2
#define ASM_TYPE_DIRECTIVE(name,typearg)
#define ASM_SIZE_DIRECTIVE(name)
#define cfi_adjust_cfa_offset(x)

PUBLIC _pow

.data
ASSUME nothing

	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(infinity,@object)

inf_zero:
infinity:
	.byte 0, 0, 0, 0, 0, 0, HEX(f0), HEX(7f)
	ASM_SIZE_DIRECTIVE(infinity)
	ASM_TYPE_DIRECTIVE(zero,@object)
zero:
	.double 0.0
	ASM_SIZE_DIRECTIVE(zero)
	ASM_TYPE_DIRECTIVE(minf_mzero,@object)

minf_mzero:
minfinity:
	.byte 0, 0, 0, 0, 0, 0, HEX(f0), HEX(ff)

mzero:
	.byte 0, 0, 0, 0, 0, 0, 0, HEX(80)
	ASM_SIZE_DIRECTIVE(minf_mzero)
	ASM_TYPE_DIRECTIVE(one,@object)

one:
	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	ASM_TYPE_DIRECTIVE(limit,@object)

limit:
	.double 0.29
	ASM_SIZE_DIRECTIVE(limit)
	ASM_TYPE_DIRECTIVE(p63,@object)

p63:
	.byte 0, 0, 0, 0, 0, 0, HEX(e0), HEX(43)
	ASM_SIZE_DIRECTIVE(p63)

#ifdef PIC
#define MO(op) op##@GOTOFF(%ecx)
#define MOX(op,x,f) op##@GOTOFF(%ecx,x,f)
#else
#define MO(op) op
#define MOX(op,x,f) op[x*f]
#endif

.code
_pow:
	fld qword ptr [esp + 12]	// y
	fxam

#ifdef	PIC
	LOAD_PIC_REG (cx)
#endif

	fnstsw ax
	mov dl, ah
	and ah, HEX(045)
	cmp	ah, HEX(040)	// is y == 0 ?
	je	L11

	cmp ah, 5	// is y == ±inf ?
	je	L12

	cmp ah, 1	// is y == NaN ?
	je	L30

	fld qword ptr [esp + 4]	// x : y

	sub esp, 8
	cfi_adjust_cfa_offset (8)

	fxam
	fnstsw ax
	mov dh, ah
	and ah, HEX(45)
	cmp ah, HEX(040)
	je	L20		// x is ±0

	cmp ah, 5
	je	L15		// x is ±inf

	fxch st(1)			// y : x

	/* fistpll raises invalid exception for |y| >= 1L<<63.  */
	fld	st		// y : y : x
	fabs			// |y| : y : x
	fcomp qword ptr ds:MO(p63)		// y : x
	fnstsw ax
	sahf
	jnc	L2

	/* First see whether `y' is a natural number.  In this case we
	   can use a more precise algorithm.  */
	fld	st		// y : y : x
	fistp qword ptr [esp]		// y : x
	fild qword ptr [esp]		// int(y) : y : x
	fucomp st(1)		// y : x
	fnstsw ax
	sahf
	jne	L2

	/* OK, we have an integer value for y.  */
	pop eax
	cfi_adjust_cfa_offset (-4)
	pop	edx
	cfi_adjust_cfa_offset (-4)
	or edx, 0
	fstp st		// x
	jns	L4		// y >= 0, jump
	fdivr qword ptr MO(one)		// 1/x		(now referred to as x)
	neg eax
	adc edx, 0
	neg edx
L4:	fld qword ptr MO(one)		// 1 : x
	fxch st(1)

L6:	shrd eax, edx, 1
	jnc	L5
	fxch st(1)
	fmul st, st(1)		// x : ST*x
	fxch st(1)
L5:	fmul st, st	// x*x : ST*x
	shr edx, 1
	mov ecx, eax
	or ecx, edx
	jnz	L6
	fstp st		// ST*x
	ret

	/* y is ±NAN */
L30:
	fld qword ptr [esp + 4]		// x : y
	fld qword ptr MO(one)		// 1.0 : x : y
	fucomp st(1)		// x : y
	fnstsw ax
	sahf
	je	L31
	fxch st(1)			// y : x
L31:fstp st(1)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
L2:	/* y is a real number.  */
	fxch st(1)			// x : y
	fld qword ptr MO(one)		// 1.0 : x : y
	fld qword ptr MO(limit)	// 0.29 : 1.0 : x : y
	fld	st(2)		// x : 0.29 : 1.0 : x : y
	fsub st, st(2)		// x-1 : 0.29 : 1.0 : x : y
	fabs			// |x-1| : 0.29 : 1.0 : x : y
	fucompp			// 1.0 : x : y
	fnstsw ax
	fxch st(1)			// x : 1.0 : y
	sahf
	ja	L7
	fsub st, st(1)		// x-1 : 1.0 : y
	fyl2xp1			// log2(x) : y
	jmp	L8

L7:	fyl2x			// log2(x) : y
L8:	fmul st, st(1)		// y*log2(x) : y
	fst st(1)		// y*log2(x) : y*log2(x)
	frndint			// int(y*log2(x)) : y*log2(x)
	fsub st(1), st	// int(y*log2(x)) : fract(y*log2(x))
	fxch			// fract(y*log2(x)) : int(y*log2(x))
	f2xm1			// 2^fract(y*log2(x))-1 : int(y*log2(x))
	fadd qword ptr MO(one)		// 2^fract(y*log2(x)) : int(y*log2(x))
	fscale			// 2^fract(y*log2(x))*2^int(y*log2(x)) : int(y*log2(x))
	add esp, 8
	cfi_adjust_cfa_offset (-8)
	fstp st(1)		// 2^fract(y*log2(x))*2^int(y*log2(x))
	ret


	// pow(x,±0) = 1
	.align ALIGNARG(4)
L11:fstp st(0)		// pop y
	fld qword ptr MO(one)
	ret

	// y == ±inf
	.align ALIGNARG(4)
L12:	fstp st(0)		// pop y
	fld qword ptr MO(one)		// 1
	fld qword ptr [esp + 4]		// x : 1
	fabs			// abs(x) : 1
	fucompp			// < 1, == 1, or > 1
	fnstsw ax
	and ah, HEX(45)
	cmp ah, HEX(45)
	je	L13		// jump if x is NaN

	cmp ah, HEX(40)
	je	L14		// jump if |x| == 1

	shl ah, 1
	xor dl, ah
	and edx, 2
	fld qword ptr MOX(inf_zero, edx, 4)
	ret

	.align ALIGNARG(4)
L14:fld qword ptr MO(one)
	ret

	.align ALIGNARG(4)
L13:fld qword ptr [esp + 4]		// load x == NaN
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
	// x is ±inf
L15:	fstp st(0)		// y
	test dh, 2
	jz	L16		// jump if x == +inf

	// We must find out whether y is an odd integer.
	fld	st		// y : y
	fistp qword ptr [esp]		// y
	fild qword ptr [esp]		// int(y) : y
	fucompp			// <empty>
	fnstsw ax
	sahf
	jne	L17

	// OK, the value is an integer, but is the number of bits small
	// enough so that all are coming from the mantissa?
	pop eax
	cfi_adjust_cfa_offset (-4)
	pop edx
	cfi_adjust_cfa_offset (-4)
	and al, 1
	jz	L18		// jump if not odd
	mov eax, edx
	or edx, edx
	jns	L155
	neg eax
L155:
	cmp eax, HEX(000200000)
	ja	L18		// does not fit in mantissa bits
	// It's an odd integer.
	shr edx, 31
	fld qword ptr MOX(minf_mzero, edx, 8)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
L16:fcomp qword ptr ds:MO(zero)
	add esp, 8
	cfi_adjust_cfa_offset (-8)
	fnstsw ax
	shr eax, 5
	and eax, 8
	fld qword ptr MOX(inf_zero, eax, 1)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
L17:	shl edx, 30	// sign bit for y in right position
	add esp, 8
	cfi_adjust_cfa_offset (-8)
L18:	shr edx, 31
	fld qword ptr MOX(inf_zero, edx, 8)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
	// x is ±0
L20:	fstp st(0)		// y
	test dl, 2
	jz	L21		// y > 0

	// x is ±0 and y is < 0.  We must find out whether y is an odd integer.
	test dh, 2
	jz	L25

	fld st		// y : y
	fistp qword ptr [esp]		// y
	fild qword ptr [esp]		// int(y) : y
	fucompp			// <empty>
	fnstsw ax
	sahf
	jne	L26

	// OK, the value is an integer, but is the number of bits small
	// enough so that all are coming from the mantissa?
	pop eax
	cfi_adjust_cfa_offset (-4)
	pop edx
	cfi_adjust_cfa_offset (-4)
	and al, 1
	jz	L27		// jump if not odd
	cmp edx, HEX(0ffe00000)
	jbe	L27		// does not fit in mantissa bits
	// It's an odd integer.
	// Raise divide-by-zero exception and get minus infinity value.
	fld qword ptr MO(one)
	fdiv qword ptr MO(zero)
	fchs
	ret

	cfi_adjust_cfa_offset (8)
L25:	fstp st(0)
L26:	add esp, 8
	cfi_adjust_cfa_offset (-8)
L27:	// Raise divide-by-zero exception and get infinity value.
	fld qword ptr MO(one)
	fdiv qword ptr MO(zero)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
	// x is ±0 and y is > 0.  We must find out whether y is an odd integer.
L21:test dh, 2
	jz	L22

	fld st		// y : y
	fistp qword ptr [esp]		// y
	fild qword ptr [esp]		// int(y) : y
	fucompp			// <empty>
	fnstsw ax
	sahf
	jne	L23

	// OK, the value is an integer, but is the number of bits small
	// enough so that all are coming from the mantissa?
	pop eax
	cfi_adjust_cfa_offset (-4)
	pop edx
	cfi_adjust_cfa_offset (-4)
	and al, 1
	jz	L24		// jump if not odd
	cmp edx, HEX(0ffe00000)
	jae	L24		// does not fit in mantissa bits
	// It's an odd integer.
	fld qword ptr MO(mzero)
	ret

	cfi_adjust_cfa_offset (8)
L22:	fstp st(0)
L23:	add esp, 8	// Don't use 2 x pop
	cfi_adjust_cfa_offset (-8)
L24:	fld qword ptr MO(zero)
	ret

END


