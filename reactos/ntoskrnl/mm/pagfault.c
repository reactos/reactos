/* $Id: pagfault.c,v 1.1 2000/04/02 13:32:41 ea Exp $ */
#include <ddk/ntddk.h>

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
