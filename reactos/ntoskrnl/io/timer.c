/* $Id: timer.c,v 1.6 2001/03/07 16:48:42 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/timer.c
 * PURPOSE:         io timers
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLBOALS *******************************************************************/

#define TAG_IO_TIMER      TAG('I', 'O', 'T', 'M')

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
IoInitializeTimer(PDEVICE_OBJECT DeviceObject, 
			   PIO_TIMER_ROUTINE TimerRoutine,
			   PVOID Context)
/*
 * FUNCTION: Sets up a driver-supplied IoTimer routine associated with a given
 * device object
 * ARGUMENTS:
 *     DeviceObject = Device object whose timer is be initialized
 *     TimerRoutine = Driver supplied routine which will be called once per
 *                    second if the timer is active
 *     Context = Driver supplied context to be passed to the TimerRoutine
 * RETURNS: Status
 */
{
   DeviceObject->Timer = ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_TIMER),
					       TAG_IO_TIMER);
   KeInitializeTimer(&(DeviceObject->Timer->timer));
   KeInitializeDpc(&(DeviceObject->Timer->dpc),
		   (PKDEFERRED_ROUTINE)TimerRoutine,Context);
   
   return(STATUS_SUCCESS);
}

VOID
STDCALL
IoStartTimer(PDEVICE_OBJECT DeviceObject)
/*
 * FUNCTION: Starts a timer so the driver-supplied IoTimer routine will be
 * called once per second
 * ARGUMENTS:
 *       DeviceObject = Device whose timer is to be started
 */
{
   long long int lli;
   LARGE_INTEGER li;
   
   lli = -1000000;
   li = *(LARGE_INTEGER *)&lli;
      
   KeSetTimerEx(&DeviceObject->Timer->timer,
                li,
                1000,
                &(DeviceObject->Timer->dpc));
}

VOID
STDCALL
IoStopTimer(PDEVICE_OBJECT DeviceObject)
/*
 * FUNCTION: Disables for a specified device object so the driver-supplied
 * IoTimer is not called
 * ARGUMENTS:
 *        DeviceObject = Device whose timer is to be stopped
 */
{
   KeCancelTimer(&(DeviceObject->Timer->timer));
}


/* EOF */
