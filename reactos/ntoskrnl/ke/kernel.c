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

#include <internal/ke.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeInit()
{
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   InitializeTimer();

   /*
    * Allow interrupts
    */
   KeLowerIrql(PASSIVE_LEVEL);
   
   KeCalibrateTimerLoop();
}
