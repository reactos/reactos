/* $Id: exit.c,v 1.2 1999/11/07 08:03:27 ea Exp $
 *
 * reactos/lib/psxdll/libc/stdlib/exit.c
 *
 * POSIX+ Subsystem client shared library
 */
#define NTOS_MODE_USER
#include <ntos.h>


void 
_exit (
	int	code
	)
{
	/* FIXME: call atexit registered functions */
	NtTerminateProcess (
		NtCurrentProcess(),
		code
		);
	/* FIXME: notify psxss.exe we died */
}


/* EOF */
