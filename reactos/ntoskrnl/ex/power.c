/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/power.c
 * PURPOSE:         Power managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Added reboot support 30/01/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal/io.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtSetSystemPowerState(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtShutdownSystem(IN SHUTDOWN_ACTION Action)
{
   return(ZwShutdownSystem(Action));
}

static void kb_wait(void)
{
   int i;
   
   for (i=0; i<10000; i++)
     {
	if ((inb_p(0x64) & 0x02) == 0)
	  {
	     return;
	  }
     }
}

NTSTATUS STDCALL ZwShutdownSystem(IN SHUTDOWN_ACTION Action)
/*
 * FIXME: Does a reboot only
 */
{
   int i, j;
   
   for (;;)
     {
	for (i=0; i<100; i++)
	  {
	     kb_wait();
	     for (j=0; j<500; j++);
	     outb(0xfe, 0x64);
	     for (j=0; j<500; j++);
	  }
     }
}
