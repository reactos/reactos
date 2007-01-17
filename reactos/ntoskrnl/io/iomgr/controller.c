/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/controller.c
 * PURPOSE:         I/O Wrappers (called Controllers) for Kernel Device Queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE IoControllerObjectType;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
IoAllocateController(IN PCONTROLLER_OBJECT ControllerObject,
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PDRIVER_CONTROL ExecutionRoutine,
                     IN PVOID Context)
{
    IO_ALLOCATION_ACTION Result;
    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    /* Initialize the Wait Context Block */
    DeviceObject->Queue.Wcb.DeviceContext = Context;
    DeviceObject->Queue.Wcb.DeviceRoutine = ExecutionRoutine;

    /* Insert the Device Queue */
    if (!KeInsertDeviceQueue(&ControllerObject->DeviceWaitQueue,
                             &DeviceObject->Queue.Wcb.WaitQueueEntry));
    {
        /* Call the execution routine */
        Result = ExecutionRoutine(DeviceObject,
                                  DeviceObject->CurrentIrp,
                                  NULL,
                                  Context);

        /* Free the controller if this was requested */
        if (Result == DeallocateObject) IoFreeController(ControllerObject);
    }
}

/*
 * @implemented
 */
PCONTROLLER_OBJECT
NTAPI
IoCreateController(IN ULONG Size)
{
   PCONTROLLER_OBJECT Controller;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE Handle;
   NTSTATUS Status;
   PAGED_CODE();

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
                            1,
                            (PVOID*)&Controller,
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
 */
VOID
NTAPI
IoDeleteController(IN PCONTROLLER_OBJECT ControllerObject)
{
    /* Just Dereference it */
    ObDereferenceObject(ControllerObject);
}

/*
 * @implemented
 */
VOID
NTAPI
IoFreeController(IN PCONTROLLER_OBJECT ControllerObject)
{
    PKDEVICE_QUEUE_ENTRY QueueEntry;
    PDEVICE_OBJECT DeviceObject;
    IO_ALLOCATION_ACTION Result;

    /* Remove the Queue */
    QueueEntry = KeRemoveDeviceQueue(&ControllerObject->DeviceWaitQueue);
    if (QueueEntry)
    {
        /* Get the Device Object */
        DeviceObject = CONTAINING_RECORD(QueueEntry,
                                         DEVICE_OBJECT,
                                         Queue.Wcb.WaitQueueEntry);

        /* Call the routine */
        Result = DeviceObject->Queue.Wcb.DeviceRoutine(DeviceObject,
                                                       DeviceObject->CurrentIrp,
                                                       NULL,
                                                       DeviceObject->
                                                       Queue.Wcb.DeviceContext);
        /* Free the controller if this was requested */
        if (Result == DeallocateObject) IoFreeController(ControllerObject);
    }
}

/* EOF */
