/* $Id: pagfault.c,v 1.4 2002/09/08 10:23:36 chorns Exp $ */
#include <ddk/ntddk.h>
#include <internal/ps.h>

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
