/*
 *  FreeLoader
 *  Copyright (C) 2003  Eric Kohl  <ekohl@rz-online.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	.text
	.code16

#define ASM

#include <arch.h>

/*
 * U32 CpuidSupported(VOID);
 *
 * RETURNS:
 *    0x00000001: CPU supports the CPUID instruction
 *    0x00000300: Found 80386 CPU
 *    0x00000400: Found 80486 CPU without CPUID support
 */

EXTERN(_CpuidSupported)
	.code32

	pushl	%ecx			/* save ECX */

	pushfl					/* push original EFLAGS */
	popl	%eax			/* get original EFLAGS */
	movl	%eax,%ecx		/* save original EFLAGS */
	xorl	$0x40000,%eax	/* flip AC bit in EFLAGS */
	pushl	%eax			/* save new EFLAGS value on stack */
	popfl					/* replace current EFLAGS value */

	pushfl					/* get new EFLAGS */
	popl	%eax			/* store new EFLAGS in EAX */
	xorl	%ecx, %eax		/* can't toggle AC bit, processor=80386 */

	movl	$0x300,%eax		/* return processor id */
    jz		NoCpuid			/* jump if 80386 processor */

	pushl	%ecx
	popfl					/* restore AC bit in EFLAGS first */

	movl	%ecx,%eax		/* get original EFLAGS */
	xorl	$0x200000,%eax	/* flip ID bit in EFLAGS */
	pushl	%eax			/* save new EFLAGS value on stack */
	popfl					/* replace current EFLAGS value */
	pushfl					/* get new EFLAGS */
	popl	%eax			/* store new EFLAGS in EAX */
	xorl	%ecx,%eax		/* can't toggle ID bit, */

	movl	$0x400,%eax		/* return processor id */
	je		NoCpuid			/* processor=80486 */

	movl	$1,%eax			/* CPUID supported */

NoCpuid:
	pushl	%ecx
	popfl					/* restore EFLAGS */
	popl	%ecx			/* retore ECX */

	ret


/*
 * VOID GetCpuid(U32 Level, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx);
 */

EXTERN(_GetCpuid)
	.code32

	pushl	%ebp
	movl	%esp,%ebp

	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi

	movl	0x08(%ebp),%eax

	cpuid

	movl	0x0C(%ebp),%esi
	movl	%eax, (%esi)

	movl	0x10(%ebp),%esi
	movl	%ebx, (%esi)

	movl	0x14(%ebp),%esi
	movl	%ecx, (%esi)

	movl	0x18(%ebp),%esi
	movl	%edx, (%esi)

	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax

	movl	%ebp,%esp
	popl	%ebp
	ret

/* EOF */
