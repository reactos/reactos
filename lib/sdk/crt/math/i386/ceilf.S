/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#include <asm.inc>

.code
.align 4

PUBLIC _ceilf
_ceilf:

	fld dword ptr [esp + 4]
	sub esp, 8

	fstcw [esp + 4]			/* store fpu control word */

	/* We use here %edx although only the low 1 bits are defined.
	   But none of the operations should care and they are faster
	   than the 16 bit operations.  */
	mov edx, [esp + 4]
	or edx,  HEX(0800)		/* round towards +oo */
	and edx, HEX(fbff)
	mov [esp], edx
	fldcw [esp]			/* load modified control word */

	frndint				/* round */

	fldcw [esp + 4]			/* restore original control word */

	add esp, 8
	ret

END
