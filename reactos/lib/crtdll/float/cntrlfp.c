/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/float.h>

unsigned int	_controlfp (unsigned int unNew, unsigned int unMask)
{	
	return _control87(unNew,unMask);
}

unsigned int	_control87 (unsigned int unNew, unsigned int unMask)
{	

register unsigned int __res;
__asm__ __volatile__ (
	"pushl	%%eax \n\t"		/* make room on stack */
	"fstcw	(%%esp) \n\t"
	"fwait \n\t"
	"popl	%%eax \n\t"
	"andl	$0xffff, %%eax	\n\t"   /* OK;  we have the old value ready */

	"movl	%1, %%ecx \n\t"
	"notl	%%ecx \n\t"
	"andl	%%eax, %%ecx \n\t"	/* the bits we want to keep */

	"movl	%2, %%edx \n\t"
	"andl	%1, %%edx \n\t"	/* the bits we want to change */

	"orl	%%ecx, %%edx\n\t"		/* the new value */
	"pushl	%%edx \n\t"
	"fldcw	(%%esp) \n\t"
	"popl	%%edx \n\t"


	:"=a" (__res):"c" (unNew),"d" (unMask):"ax", "dx", "cx");

	return __res;
}
