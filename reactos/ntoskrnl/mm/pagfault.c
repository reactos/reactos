/* $Id: pagfault.c,v 1.6 2004/04/10 22:35:25 gdalsnes Exp $ */
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

   return ( Thread->DisablePageFaultClustering
            | Thread->ForwardClusterOnly
          );
}


/* EOF */
