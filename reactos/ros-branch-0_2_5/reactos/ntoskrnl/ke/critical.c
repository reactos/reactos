/* $Id: critical.c,v 1.11 2004/11/21 18:33:54 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/critical.c
 * PURPOSE:         Implement critical regions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL KeEnterCriticalRegion (VOID)
{
   PKTHREAD Thread = KeGetCurrentThread(); 
   
   DPRINT("KeEnterCriticalRegion()\n");
   
   if (!Thread) return; /* <-Early in the boot process the current thread is obseved to be NULL */

   Thread->KernelApcDisable--;
}

/*
 * @implemented
 */
VOID STDCALL KeLeaveCriticalRegion (VOID)
{
  PKTHREAD Thread = KeGetCurrentThread(); 

  DPRINT("KeLeaveCriticalRegion()\n");
  
  if (!Thread) return; /* <-Early in the boot process the current thread is obseved to be NULL */

  /* Reference: http://www.ntfsd.org/archive/ntfsd0104/msg0203.html */
  if(++Thread->KernelApcDisable == 0) 
  { 
    if (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) 
    { 
      Thread->ApcState.KernelApcPending = TRUE; 
      HalRequestSoftwareInterrupt(APC_LEVEL); 
    } 
  } 

}

/* EOF */
