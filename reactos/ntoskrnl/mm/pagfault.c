/* $Id: pagfault.c,v 1.2 2000/07/04 08:52:45 dwelch Exp $ */
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
