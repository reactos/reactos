/* $Id: pagfault.c,v 1.5 2003/07/10 21:05:03 royce Exp $ */
#include <ddk/ntddk.h>
#include <internal/ps.h>

/*
 * @implemented
 */
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
