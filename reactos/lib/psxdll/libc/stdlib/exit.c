/* $Id: exit.c,v 1.3 1999/12/26 16:36:44 ea Exp $
 *
 * reactos/lib/psxdll/libc/stdlib/exit.c
 *
 * POSIX+ Subsystem client shared library
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include "../../misc/psxdll.h"

void _exit (int code)
{
	NTSTATUS	Status;
	
	/* FIXME: call atexit registered functions */
#if 0
	/* FIXME: notify psxss.exe we died */
	Status = NtRequestWaitReplyPort (
			__PdxApiPort,
			...
			);
#endif
	NtTerminateProcess (
		NtCurrentProcess(),
		code
		);
}


/* EOF */
