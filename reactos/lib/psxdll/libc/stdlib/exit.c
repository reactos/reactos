/* $Id: exit.c,v 1.1 1999/10/12 21:19:40 ea Exp $
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
}


/* EOF */
