/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.data
yint: 
	.word   0,0
LCW1:
	.word	0
LCW2:
	.word	0

	.text
LC0:
	.double	0d1.0e+00

frac:
	fstcww	LCW1
	fstcww	LCW2
	fwait
	andw	$0xf3ff,LCW2
	orw	$0x0400,LCW2
	fldcww	LCW2
	fldl	%st(0)
	frndint
	fldcww	LCW1
	fxch	%st(1)
	fsub	%st(1),%st
	ret

Lpow2:
	call    frac
	f2xm1
	faddl	LC0
	fscale
	fstp	%st(1)
	ret

	.globl	_pow
_pow:
	fldl	12(%esp)
	fldl	4(%esp)
	ftst	
	fnstsww	%ax
	sahf
	jbe	xltez
	fyl2x
	jmp	Lpow2
xltez:
	jb	xltz
	fstp	%st(0)
	ftst
	fnstsww	%ax
	sahf
	ja	ygtz
	jb	error
	fstp	%st(0) 
	fld1l
	fchs
error:
	fsqrt
	ret
ygtz:
	fstp	%st(0)
	fldzl
	ret
xltz:
	fabs
	fxch    %st(1)
	call	frac
	ftst
	fnstsww	%ax
	fstp	%st(0)
	sahf
	je	yisint
	fstp	%st(0)
	fchs
	jmp	error
yisint:
	fistl	yint
	fxch    %st(1)
	fyl2x
	call	Lpow2
	andl	$1,yint
	jz	yeven
	fchs
yeven:
	ret

