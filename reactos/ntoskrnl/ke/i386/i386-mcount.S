/* i386-specific implemetation of profiling support.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* We need a special version of the `mcount' function since for ix86 it
   must not clobber any register.  This has several reasons:
     - there is a bug in gcc as of version 2.7.2.2 which prohibits the
       use of profiling together with nested functions
     - the ELF `fixup' function uses GCC's regparm feature
     - some (future) systems might want to pass parameters in registers.  */
/* dwelch: Alerted for ReactOS to output the files to port 0xe9 for processing
	by bochs. */

	
.globl _mcount
_mcount:	
	/* Save the caller-clobbered registers.  */
	pushl %eax
	pushl %ecx
	pushl %edx

	movw $0xe9, %dx
	movl 4(%ebp), %eax
	outl %eax, %dx
	movl 12(%esp), %eax
	outl %eax, %dx

	/* Pop the saved registers.  Please note that `mcount' has no
	   return value.  */
	popl %edx
	popl %ecx
	popl %eax
	ret



