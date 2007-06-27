/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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


	.p2align	2	/* force 4-byte alignment */
EXTERN(i386idt)
	/* Exception 0 - Divide By Zero */
	.word	i386DivideByZero			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Flags, Zero Byte */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 1 - Debug Exception */
	.word	i386DebugException			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 2 - NMI */
	.word	i386NMIException			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 3 - Breakpoint (INT 3) */
	.word	i386Breakpoint				/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 4 - Overflow (INTO with EFLAGS[OF] set) */
	.word	i386Overflow				/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 5 - Bound Exception */
	.word	i386BoundException			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 6 - Invalid Opcode */
	.word	i386InvalidOpcode			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 7 - FPU Not Available */
	.word	i386FPUNotAvailable			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 8 - Double Fault */
	.word	i386DoubleFault				/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 9 - Coprocessor Segment Overrun */
	.word	i386CoprocessorSegment		/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 10 (0x0A) - Invalid TSS */
	.word	i386InvalidTSS				/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 11 (0x0B) - Segment Not Present */
	.word	i386SegmentNotPresent		/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 12 (0x0C) - Stack Exception */
	.word	i386StackException			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 13 (0x0D) - General Protection Fault */
	.word	i386GeneralProtectionFault	/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 14 (0x0E) - Page Fault */
	.word	i386PageFault				/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 15 (0x0F) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 16 (0x10) - Coprocessor Error */
	.word	i386CoprocessorError		/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 17 (0x11) - Alignment Check */
	.word	i386AlignmentCheck			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 18 (0x12) - Machine Check */
	.word	i386MachineCheck			/* Offset 0 - 15 */
	.word	0x0008						/* Selector */
	.word	0x8e00						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 19 (0x13) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 20 (0x14) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 21 (0x15) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 22 (0x16) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 23 (0x17) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 24 (0x18) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 25 (0x19) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 26 (0x1A) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 27 (0x1B) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 28 (0x1C) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 29 (0x1D) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 30 (0x1E) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

	/* Exception 31 (0x1F) - Reserved */
	.word	0x0000						/* Offset 0 - 15 */
	.word	0x0000						/* Selector */
	.word	0x0000						/* Zero byte, flags */
	.word	0x0000						/* Offset 16 - 31 */

/* IDT table pointer */
EXTERN(i386idtptr)
	.word	(i386idtptr-i386idt)		/* Limit */
	.long	i386idt						/* Base Address */
