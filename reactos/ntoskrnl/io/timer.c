/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/timer.c
 * PURPOSE:         io timers
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Reimplemented 05/11/04 - Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_IO_TIMER      TAG('I', 'O', 'T', 'M')

/* Timer Database */
KSPIN_LOCK IopTimerLock;
LIST_ENTRY IopTimerQueueHead;

/* Timer Firing */
KDPC IopTimerDpc;
KTIMER IopTimer;

/* Keep count of how many timers we have */
ULONG IopTimerCount = 0;

/* FUNCTIONS *****************************************************************/

static VOID STDCALL
IopTimerDispatch(IN PKDPC Dpc,
                IN PVOID DeferredContext,
                IN PVOID SystemArgument1,
                IN PVOID SystemArgument2)
{
	KIRQL OldIrql;
	PLIST_ENTRY TimerEntry;
	PIO_TIMER Timer;
	ULONG i;
	
	DPRINT("Dispatching IO Timers. There are: %x \n", IopTimerCount);
	
	/* Check if any Timers are actualyl enabled as of now */
	if (IopTimerCount) {
	
		/* Lock the Timers */
		KeAcquireSpinLock(&IopTimerLock, &OldIrql);  
		
		/* Call the Timer Routine of each enabled Timer */
		for(TimerEntry = IopTimerQueueHead.Flink, i = IopTimerCount;
		    (TimerEntry != &IopTimerQueueHead) && i;
		    TimerEntry = TimerEntry->Flink) {

			Timer = CONTAINING_RECORD(TimerEntry, IO_TIMER, IoTimerList);
			if (Timer->TimerEnabled) {
				DPRINT("Dispatching a Timer Routine: %x for Device Object: %x \n", 
					Timer->TimerRoutine,
					Timer->DeviceObject);
				Timer->TimerRoutine(Timer->DeviceObject, Timer->Context);
				i--;
			}
		}
		
		/* Unlock the Timers */
		KeReleaseSpinLock(&IopTimerLock, OldIrql);
	}
}

VOID
STDCALL
IopRemoveTimerFromTimerList(
	IN PIO_TIMER Timer
)
{
	KIRQL OldIrql;

	/* Lock Timers */
	KeAcquireSpinLock(&IopTimerLock, &OldIrql);
	
	/* Remove Timer from the List and Drop the Timer Count if Enabled */
	RemoveEntryList(&Timer->IoTimerList);
	if (Timer->TimerEnabled) IopTimerCount--;
	
	/* Unlock the Timers */
	KeReleaseSpinLock(&IopTimerLock, OldIrql);
}

VOID
FASTCALL
IopInitTimerImplementation(VOID)
/* FUNCTION: Initializes the IO Timer Object Implementation
 * RETURNS: NOTHING
 */
{
	LARGE_INTEGER ExpireTime;
	
	/* Initialize Timer List Lock */
	KeInitializeSpinLock(&IopTimerLock);
	
	/* Initialize Timer List */
	InitializeListHead(&IopTimerQueueHead);
	
	/* Initialize the DPC/Timer which will call the other Timer Routines */
	ExpireTime.QuadPart = -10000000;
	KeInitializeDpc(&IopTimerDpc, IopTimerDispatch, NULL);
	KeInitializeTimerEx(&IopTimer, SynchronizationTimer);
	KeSetTimerEx(&IopTimer, ExpireTime, 1000, &IopTimerDpc);
}

/*
 * @implemented
 */
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
	DPRINT("IoInitializeTimer() called for Device Object: %x with Routine: %x \n", DeviceObject, TimerRoutine);
	 
	/* Allocate Timer */
	if (!DeviceObject->Timer) {
		DeviceObject->Timer = ExAllocatePoolWithTag(NonPagedPool,
							    sizeof(IO_TIMER),
							    TAG_IO_TIMER);
		if (!DeviceObject->Timer) return STATUS_INSUFFICIENT_RESOURCES;

		/* Set up the Timer Structure */
		DeviceObject->Timer->Type = IO_TYPE_TIMER;
		DeviceObject->Timer->DeviceObject = DeviceObject;
	}
	
	DeviceObject->Timer->TimerRoutine = TimerRoutine;
	DeviceObject->Timer->Context = Context;
	DeviceObject->Timer->TimerEnabled = FALSE;

	/* Add it to the Timer List */
	ExInterlockedInsertTailList(&IopTimerQueueHead,
				    &DeviceObject->Timer->IoTimerList,
				    &IopTimerLock);
   
	/* Return Success */	
	DPRINT("IoInitializeTimer() Completed\n");
	return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
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
	KIRQL OldIrql;
	
	DPRINT("IoStartTimer for Device Object: %x\n", DeviceObject);
		
	/* Lock Timers */
	KeAcquireSpinLock(&IopTimerLock, &OldIrql);
	
	/* If the timer isn't already enabled, enable it and increase IO Timer Count*/
	if (!DeviceObject->Timer->TimerEnabled) {
		DeviceObject->Timer->TimerEnabled = TRUE;
		IopTimerCount++;
	}
	
	/* Unlock Timers */
	KeReleaseSpinLock(&IopTimerLock, OldIrql);
	DPRINT("IoStartTimer Completed for Device Object: %x New Count: %x \n", DeviceObject, IopTimerCount);
}

/*
 * @implemented
 */
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
	KIRQL OldIrql;
	
	DPRINT("IoStopTimer for Device Object: %x\n", DeviceObject);
	
	/* Lock Timers */
	KeAcquireSpinLock(&IopTimerLock, &OldIrql);
	
	/* If the timer is enabled, disable it and decrease IO Timer Count*/
	if (DeviceObject->Timer->TimerEnabled) {
		DeviceObject->Timer->TimerEnabled = FALSE;
		IopTimerCount--;
	}
	
	/* Unlock Timers */
	KeReleaseSpinLock(&IopTimerLock, OldIrql);
	DPRINT("IoStopTimer Completed for Device Object: %x New Count: %x \n", DeviceObject, IopTimerCount);
}


/* EOF */
