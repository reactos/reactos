/* $Id: pagfault.c,v 1.7 2004/08/15 16:39:08 chorns Exp $ */

#include <ntoskrnl.h>

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
