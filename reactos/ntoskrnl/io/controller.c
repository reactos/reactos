/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/contrller.c
 * PURPOSE:         Implements the controller object
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE IoControllerObjectType;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 *
 * FUNCTION: Sets up a call to a driver-supplied ControllerControl routine
 * as soon as the device controller, represented by the given controller
 * object, is available to carry out an I/O operation for the target device,
 * represented by the given device object.
 * ARGUMENTS:
 *       ControllerObject = Driver created controller object
 *       DeviceObject = Target device for the current irp
 *       ExecutionRoutine = Routine to be called when the device is available
 *       Context = Driver supplied context to be passed on to the above routine
 */
VOID
STDCALL
IoAllocateController(PCONTROLLER_OBJECT ControllerObject,
                     PDEVICE_OBJECT DeviceObject,
                     PDRIVER_CONTROL ExecutionRoutine,
                     PVOID Context)
{
    IO_ALLOCATION_ACTION Result;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Initialize the Wait Context Block */
    DeviceObject->Queue.Wcb.DeviceContext = Context;
    DeviceObject->Queue.Wcb.DeviceRoutine = ExecutionRoutine;
    
    /* Insert the Device Queue */
    if (!KeInsertDeviceQueue(&ControllerObject->DeviceWaitQueue,
                             &DeviceObject->Queue.Wcb.WaitQueueEntry));
    {
        Result = ExecutionRoutine(DeviceObject,
                                  DeviceObject->CurrentIrp,
                                  NULL,
                                  Context); 
    }
       
    if (Result == DeallocateObject)
    {
        IoFreeController(ControllerObject);
    }
}

/*
 * @implemented
 *
 * FUNCTION: Allocates memory and initializes a controller object
 * ARGUMENTS:
 *        Size = Size (in bytes) to be allocated for the controller extension
 * RETURNS: A pointer to the created object
 */
PCONTROLLER_OBJECT
STDCALL
IoCreateController(ULONG Size)

{
   PCONTROLLER_OBJECT Controller;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE Handle;
   NTSTATUS Status;
   ASSERT_IRQL(PASSIVE_LEVEL);

   /* Initialize an empty OBA */
   InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
   
   /* Create the Object */
   Status = ObCreateObject(KernelMode,
                           IoControllerObjectType,
                           &ObjectAttributes,
                           KernelMode,
                           NULL,
                           sizeof(CONTROLLER_OBJECT) + Size,
                           0,
                           0,
                           (PVOID*)&Controller);
    if (!NT_SUCCESS(Status)) return NULL;
    
    /* Insert it */
    Status = ObInsertObject(Controller,
                            NULL,
                            FILE_READ_DATA | FILE_WRITE_DATA,
                            0,
                            NULL,
                            &Handle);
   if (!NT_SUCCESS(Status)) return NULL;
   
    /* Close the dummy handle */
    NtClose(Handle);
        
    /* Zero the Object and set its data */
    RtlZeroMemory(Controller, sizeof(CONTROLLER_OBJECT) + Size);
    Controller->Type = IO_TYPE_CONTROLLER;
    Controller->Size = sizeof(CONTROLLER_OBJECT) + Size;
    Controller->ControllerExtension = (Controller + 1);
    
    /* Initialize its Queue */
    KeInitializeDeviceQueue(&Controller->DeviceWaitQueue);

    /* Return Controller */
    return Controller;
}

/*
 * @implemented
 *
 * FUNCTION: Removes a given controller object from the system
 * ARGUMENTS:
 *        ControllerObject = Controller object to be released
 */
VOID
STDCALL
IoDeleteController(PCONTROLLER_OBJECT ControllerObject)

{
    /* Just Dereference it */
    ObDereferenceObject(ControllerObject);
}

/*
 * @implemented
 *
 * FUNCTION: Releases a previously allocated controller object when a 
 * device has finished an I/O request
 * ARGUMENTS:
 *       ControllerObject = Controller object to be released
 */
VOID
STDCALL
IoFreeController(PCONTROLLER_OBJECT ControllerObject)
{
    PKDEVICE_QUEUE_ENTRY QueueEntry;
    PDEVICE_OBJECT DeviceObject;
    IO_ALLOCATION_ACTION Result;

    /* Remove the Queue */
    if ((QueueEntry = KeRemoveDeviceQueue(&ControllerObject->DeviceWaitQueue)))
    {
        /* Get the Device Object */
        DeviceObject = CONTAINING_RECORD(QueueEntry, 
                                         DEVICE_OBJECT, 
                                         Queue.Wcb.WaitQueueEntry);
        /* Call the routine */
        Result = DeviceObject->Queue.Wcb.DeviceRoutine(DeviceObject,
                                                       DeviceObject->CurrentIrp,
                                                       NULL,
                                                       DeviceObject->Queue.Wcb.DeviceContext);
        /* Check the result */
        if (Result == DeallocateObject)
        {
            /* Free it again */
            IoFreeController(ControllerObject);
        }
    }
}


/* EOF */
