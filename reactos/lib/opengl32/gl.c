/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/gl.c
 * PURPOSE:              OpenGL32 lib, glXXX functions
 * PROGRAMMER:           Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 2, 2004: Created
 */

/* FIXME: everything in this file ;-) */

/* On a x86 we call the ICD functions in a special-way:
 *
 * For every glXXX function we export a glXXX entry-point which loads the
 * matching "real" function pointer from the NtCurrentTeb()->glDispatch table
 * and jmps to the address, leaving the stack alone and letting the "real"
 * function return for us.
 *
 * On other machines we use C to forward the calls (slow...)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/*#include <GL/gl.h>*/
/* -- complains about conflicting types of GL functions we export */
/* either fix that or put the GLxxx typedefs in here */

#include "opengl32.h"

/*#if defined(_M_IX86)*/
#if 1
#define X(func, ret, args)                                        \
void WINAPI func ()                                               \
{                                                                 \
	__asm__(                                                      \
		"movl	%%fs:0x18,	%%eax"		"\n\t"                    \
		"addl	%0,			%%eax"		"\n\t"                    \
		"jmpl	*(%%eax)"				"\n\t"                    \
		:                                                         \
		: "n"(0x714+(GLIDX_##func*sizeof(PVOID))) );              \
}

#else /* defined(_M_IX86) */

/* FIXME: need more info for X (to pass on arguments) */
/*
#define X(func, ret, args)                                        \

ret func args
{
	PVOID fn = (PVOID)( ((char *)NtCurrentTeb()) +
	                    (0x714+(GLIDX_func*sizeof(PVOID))) );
	return (ret)((ret (*func) args)fn)();
}*/

#endif /* !defined(_M_IX86) */

GLFUNCS_MACRO
#undef X

/* EOF */
