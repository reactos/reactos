/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kernel.c
 * PURPOSE:         Initializes the kernel
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>

//#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeInit(VOID)
{
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();

   /*
    * Allow interrupts
    */
   CHECKPOINT;
   KeLowerIrql(PASSIVE_LEVEL);
   
   CHECKPOINT;
   KeCalibrateTimerLoop();
}
