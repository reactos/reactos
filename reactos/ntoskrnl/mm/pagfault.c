/* $Id: pagfault.c,v 1.3 2002/09/07 15:13:00 chorns Exp $ */

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


BOOLEAN
STDCALL
MmIsRecursiveIoFault (
	VOID
	)
{
	PETHREAD Thread = PsGetCurrentThread ();

	return (	Thread->DisablePageFaultClustering
			| Thread->ForwardClusterOnly
			);
}


/* EOF */
