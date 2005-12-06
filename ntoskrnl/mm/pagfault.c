/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pagfault.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */


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
