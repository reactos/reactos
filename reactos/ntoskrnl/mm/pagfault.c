/* $Id$ */

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
