/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 *
 * Removed header file dependency for use in libmingwex.a by
 *   Danny Smith <dannysmith@users.sourceforge.net>
 */

#include <asm.inc>

.code
.align 4

PUBLIC _floorf
_floorf:

	fld dword ptr [esp + 4]
	sub esp, 8

	fstcw [esp + 4]		/* store fpu control word */

	/* We use here %edx although only the low 1 bits are defined.
	   But none of the operations should care and they are faster
	   than the 16 bit operations.  */
	mov edx, [esp + 4]
	or edx, HEX(0400)		/* round towards -oo */
	and edx, HEX(0f7ff)
	mov [esp], edx
	fldcw [esp]			/* load modified control word */

	frndint				/* round */

	fldcw [esp + 4]			/* restore original control word */

	add esp, 8
	ret

END
