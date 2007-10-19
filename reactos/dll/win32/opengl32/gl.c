/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/gl.c
 * PURPOSE:              OpenGL32 lib, glXXX functions
 * PROGRAMMER:           Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 2, 2004: Created
 */

/* On a x86 we call the ICD functions in a special-way:
 *
 * For every glXXX function we export a glXXX entry-point which loads the
 * matching "real" function pointer from the NtCurrentTeb()->glDispatchTable
 * for gl functions in teblist.h and for others it gets the pointer from
 * NtCurrentTeb()->glTable and jmps to the address, leaving the stack alone and
 * letting the "real" function return for us.
 * Royce has implemented this in NASM =D
 *
 * On other machines we use C to forward the calls (slow...)
 */

#define WIN32_LEANER_AND_MEANER
#define WIN32_NO_STATUS
#include <windows.h>
#include "teb.h"

#include "opengl32.h"

int STDCALL glEmptyFunc0() { return 0; }
int STDCALL glEmptyFunc4( long l1 ) { return 0; }
int STDCALL glEmptyFunc8( long l1, long l2 ) { return 0; }
int STDCALL glEmptyFunc12( long l1, long l2, long l3 ) { return 0; }
int STDCALL glEmptyFunc16( long l1, long l2, long l3, long l4 ) { return 0; }
int STDCALL glEmptyFunc20( long l1, long l2, long l3, long l4, long l5 )
                           { return 0; }
int STDCALL glEmptyFunc24( long l1, long l2, long l3, long l4, long l5,
                           long l6 ) { return 0; }
int STDCALL glEmptyFunc28( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7 ) { return 0; }
int STDCALL glEmptyFunc32( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8 ) { return 0; }
int STDCALL glEmptyFunc36( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8, long l9 ) { return 0; }
int STDCALL glEmptyFunc40( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8, long l9, long l10 )
                           { return 0; }
int STDCALL glEmptyFunc44( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8, long l9, long l10,
                           long l11 ) { return 0; }
int STDCALL glEmptyFunc48( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8, long l9, long l10,
                           long l11, long l12 ) { return 0; }
int STDCALL glEmptyFunc52( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8, long l9, long l10,
                           long l11, long l12, long l13 ) { return 0; }
int STDCALL glEmptyFunc56( long l1, long l2, long l3, long l4, long l5,
                           long l6, long l7, long l8, long l9, long l10,
                           long l11, long l12, long l13, long l14 )
                           { return 0; }

#if defined(_M_IX86)
# define FOO(x) #x
# define X(func, ret, typeargs, args, icdidx, tebidx, stack)          \
__asm__(".align 4"                                    "\n\t"          \
        ".globl _"#func"@"#stack                      "\n\t"          \
        "_"#func"@"#stack":"                          "\n\t"          \
        "       movl %fs:0x18, %eax"                  "\n\t"          \
        "       movl 0xbe8(%eax), %eax"               "\n\t"          \
        "       jmp *"FOO((icdidx*4))"(%eax)"         "\n\t");

GLFUNCS_MACRO
# undef FOO
# undef X
#else /* defined(_M_IX86) */
# define X(func, ret, typeargs, args, icdidx, tebidx, stack)          \
ret STDCALL func typeargs                                             \
{                                                                     \
	PROC *table;                                                  \
	PROC fn;                                                      \
	if (tebidx >= 0 && 0)                                         \
	{                                                             \
		table = (PROC *)NtCurrentTeb()->glDispatchTable;      \
		fn = table[tebidx];                                   \
	}                                                             \
	else                                                          \
	{                                                             \
		table = (PROC *)NtCurrentTeb()->glTable;              \
		fn = table[icdidx];                                   \
	}                                                             \
	return (ret)((ret(*)typeargs)fn)args;                         \
}

GLFUNCS_MACRO

# undef X
#endif /* !defined(_M_IX86) */

/* EOF */
