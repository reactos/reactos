/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/irq.c
 * PURPOSE:         IRQ handling
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/
typedef struct _IO_INTERRUPT
{
    KINTERRUPT FirstInterrupt;
    PKINTERRUPT Interrupt[MAXIMUM_PROCESSORS];
    KSPIN_LOCK SpinLock;
} IO_INTERRUPT, *PIO_INTERRUPT;

/* FUNCTIONS *****************************************************************/

/*
 * FUNCTION: Registers a driver's isr to be called when its device interrupts
 * ARGUMENTS:
 *        InterruptObject (OUT) = Points to the interrupt object created on
 *                                return
 *        ServiceRoutine = Routine to be called when the device interrupts
 *        ServiceContext = Parameter to be passed to ServiceRoutine
 *        SpinLock = Initalized spinlock that will be used to synchronize
 *                   access between the isr and other driver routines. This is
 *                   required if the isr handles more than one vector or the
 *                   driver has more than one isr
 *        Vector = Interrupt vector to allocate
 *                 (returned from HalGetInterruptVector)
 *        Irql = DIRQL returned from HalGetInterruptVector
 *        SynchronizeIrql = DIRQL at which the isr will execute. This must
 *                          be the highest of all the DIRQLs returned from
 *                          HalGetInterruptVector if the driver has multiple
 *                          isrs
 *        InterruptMode = Specifies if the interrupt is LevelSensitive or
 *                        Latched
 *        ShareVector = Specifies if the vector can be shared
 *        ProcessorEnableMask = Processors on the isr can run
 *        FloatingSave = TRUE if the floating point stack should be saved when
 *                       the isr runs. Must be false for x86 drivers
 * RETURNS: Status
 * IRQL: PASSIVE_LEVEL
 *
 * @implemented
 */
NTSTATUS 
STDCALL
IoConnectInterrupt(PKINTERRUPT* InterruptObject,
                   PKSERVICE_ROUTINE ServiceRoutine,
                   PVOID ServiceContext,
                   PKSPIN_LOCK SpinLock,
                   ULONG Vector,
                   KIRQL Irql,
                   KIRQL SynchronizeIrql,
                   KINTERRUPT_MODE InterruptMode,
                   BOOLEAN ShareVector,
                   KAFFINITY ProcessorEnableMask,
                   BOOLEAN FloatingSave)
{
    PKINTERRUPT Interrupt;
    PKINTERRUPT InterruptUsed;
    PIO_INTERRUPT IoInterrupt;
    PKSPIN_LOCK SpinLockUsed;
    BOOLEAN FirstRun = TRUE;
    ULONG i, count;
    
    PAGED_CODE();

    DPRINT1("IoConnectInterrupt(Vector %x)\n",Vector);

    /* Convert the Mask */
    ProcessorEnableMask &= ((1 << KeNumberProcessors) - 1);

    /* Make sure at least one CPU is on it */
    if (!ProcessorEnableMask) return STATUS_INVALID_PARAMETER;

    /* Determine the allocation */
    for (i = 0, count = 0; i < KeNumberProcessors; i++)
    {
        if (ProcessorEnableMask & (1 << i)) count++;
    }
    
    /* Allocate the array of I/O Interrupts */
    IoInterrupt = ExAllocatePoolWithTag(NonPagedPool,
                                        (count - 1)* sizeof(KINTERRUPT) +
                                        sizeof(IO_INTERRUPT),
                                        TAG_KINTERRUPT);
    if (!IoInterrupt) return(STATUS_INSUFFICIENT_RESOURCES);

    /* Select which Spinlock to use */
    if (SpinLock)
    {
        SpinLockUsed = SpinLock;
    }
    else
    {
        SpinLockUsed = &IoInterrupt->SpinLock;
    }
    
    /* We first start with a built-in Interrupt inside the I/O Structure */
    *InterruptObject = &IoInterrupt->FirstInterrupt;
    Interrupt = (PKINTERRUPT)(IoInterrupt + 1);
    FirstRun = TRUE;
    
    /* Start with a fresh structure */
    RtlZeroMemory(IoInterrupt, sizeof(IO_INTERRUPT));
    
    /* Now create all the interrupts */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Check if it's enabled for this CPU */
        if (ProcessorEnableMask & (1 << i))
        {
            /* Check which one we will use */
            InterruptUsed = FirstRun ? &IoInterrupt->FirstInterrupt : Interrupt;
            
            /* Initialize it */
            KeInitializeInterrupt(InterruptUsed,
                                  ServiceRoutine,
                                  ServiceContext,
                                  SpinLockUsed,
                                  Vector,
                                  Irql,
                                  SynchronizeIrql,
                                  InterruptMode,
                                  ShareVector,
                                  i,
                                  FloatingSave);
                                  
            /* Connect it */
            if (!KeConnectInterrupt(InterruptUsed))
            {
                /* Check how far we got */
                if (FirstRun)
                {
                    /* We failed early so just free this */
                    ExFreePool(IoInterrupt);
                }
                else
                {
                    /* Far enough, so disconnect everything */
                    IoDisconnectInterrupt(&IoInterrupt->FirstInterrupt);
                }
                return STATUS_INVALID_PARAMETER;
            }
            
            /* Now we've used up our First Run */
            if (FirstRun)
            {
                FirstRun = FALSE;
            }
            else
            {
                /* Move on to the next one */
                IoInterrupt->Interrupt[i] = Interrupt++;
            }
        }
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Releases a drivers isr
 * ARGUMENTS:
 *        InterruptObject = isr to release
 *
 * @implemented
 */
VOID 
STDCALL
IoDisconnectInterrupt(PKINTERRUPT InterruptObject)

{
    ULONG i;
    PIO_INTERRUPT IoInterrupt;
    
    PAGED_CODE();
    
    /* Get the I/O Interrupt */
    IoInterrupt = CONTAINING_RECORD(InterruptObject, 
                                    IO_INTERRUPT, 
                                    FirstInterrupt);
                                    
    /* Disconnect the first one */
    KeDisconnectInterrupt(&IoInterrupt->FirstInterrupt);

    /* Now disconnect the others */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        if (IoInterrupt->Interrupt[i])
        {
            KeDisconnectInterrupt(&InterruptObject[i]);
        }
    }
    
    /* Free the I/O Interrupt */
    ExFreePool(IoInterrupt);
}

/* EOF */
