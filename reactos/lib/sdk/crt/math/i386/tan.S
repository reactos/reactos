/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

	.file	"tan.S"
	.text
#ifdef _WIN64
	.align 8
#else
	.align 4
#endif
.globl _tan
	.def	_tan;	.scl	2;	.type	32;	.endef
_tan:
#ifdef _WIN64
	fldt	(%rdx)
	fptan
	fnstsw	%ax
	testl	$0x400,%eax
	jnz	1f
	fstp	%st(0)
	movq	%rcx,%rax
	movq	$0,8(%rcx)
	fstpt	(%rcx)
	ret
1:	fldpi
	fadd	%st(0)
	fxch	%st(1)
2:	fprem1
	fstsw	%ax
	testl	$0x400,%eax
	jnz	2b
	fstp	%st(1)
	fptan
	fstp	%st(0)
	movq	%rcx,%rax
	movq	$0,8(%rcx)
	fstpt	(%rcx)
	ret
#else
	fldt	4(%esp)
	fptan
	fnstsw	%ax
	testl	$0x400,%eax
	jnz	1f
	fstp	%st(0)
	ret
1:	fldpi
	fadd	%st(0)
	fxch	%st(1)
2:	fprem1
	fstsw	%ax
	testl	$0x400,%eax
	jnz	2b
	fstp	%st(1)
	fptan
	fstp	%st(0)
	ret
#endif
