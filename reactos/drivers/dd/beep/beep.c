/* $Id: beep.c,v 1.5 2000/07/02 10:54:12 ekohl Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/beep/beep.c
 * PURPOSE:              BEEP device driver
 * PROGRAMMER:           Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                       30/01/99 Created
 *                       16/10/99 Minor fixes
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntddbeep.h>

#define NDEBUG
#include <internal/debug.h>


/* TYEPEDEFS ***************************************************************/

typedef struct tagBEEP_DEVICE_EXTENSION
{
    KDPC   Dpc;
    KTIMER Timer;
    KEVENT Event;
    BOOL   BeepOn;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* FUNCTIONS ***************************************************************/


VOID BeepDPC (PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PDEVICE_EXTENSION DeviceExtension = DeferredContext;

    DPRINT ("BeepDPC() called!\n");
    HalMakeBeep (0);
    DeviceExtension->BeepOn = FALSE;
    KeSetEvent (&(DeviceExtension->Event), 0, TRUE);

    DPRINT ("BeepDPC() finished!\n");
}


NTSTATUS BeepCreate (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    if (Stack->MajorFunction == IRP_MJ_CREATE)
    {
        DPRINT ("BeepCreate() called!\n");
        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
    }
    else
        status = STATUS_NOT_IMPLEMENTED;

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp,IO_NO_INCREMENT);
    return (status);
}


NTSTATUS BeepClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;

    switch (Stack->MajorFunction)
    {
        case IRP_MJ_CLOSE:
            DPRINT ("BeepClose() called!\n");
            Irp->IoStatus.Information = 0;
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return (status);
}


NTSTATUS BeepCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;

    if (Stack->MajorFunction == IRP_MJ_CLEANUP)
    {
        DPRINT ("BeepCleanup() called!\n");
        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
    }
    else
        status = STATUS_NOT_IMPLEMENTED;

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return (status);
}


NTSTATUS BeepDeviceControl (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *                       DeviceObject = Device for request
 *                       Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension;
    PBEEP_SET_PARAMETERS pbsp;
    NTSTATUS status;

    DeviceExtension = DeviceObject->DeviceExtension;

    DPRINT ("BeepDeviceControl() called!\n");
    if (Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_BEEP_SET)
    {
        Irp->IoStatus.Information = 0;
        if (Stack->Parameters.DeviceIoControl.InputBufferLength == sizeof(BEEP_SET_PARAMETERS))
        {
            pbsp = (PBEEP_SET_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;

            if (pbsp->Frequency >= BEEP_FREQUENCY_MINIMUM &&
                pbsp->Frequency <= BEEP_FREQUENCY_MAXIMUM)
            {
                LARGE_INTEGER DueTime;

                DueTime.QuadPart = 0;

                /* do the beep!! */
                DPRINT ("Beep:\n  Freq: %lu Hz\n  Dur: %lu ms\n",
                        pbsp->Frequency, pbsp->Duration);

                if (pbsp->Duration >= 0)
                {
                    DueTime.QuadPart = (LONGLONG)pbsp->Duration * -10000;

                    KeSetTimer (&DeviceExtension->Timer,
                                DueTime,
                                &DeviceExtension->Dpc);

                    HalMakeBeep (pbsp->Frequency);
                    DeviceExtension->BeepOn = TRUE;
                    KeWaitForSingleObject (&(DeviceExtension->Event),
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);
                }
                else if (pbsp->Duration == (DWORD)-1)
                {
                    if (DeviceExtension->BeepOn)
                    {
                        HalMakeBeep (0);
                        DeviceExtension->BeepOn = FALSE;
                    }
                    else
                    {
                        HalMakeBeep (pbsp->Frequency);
                        DeviceExtension->BeepOn = TRUE;
                    }
                }

                DPRINT ("Did the beep!\n");

                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return (status);
}


NTSTATUS BeepUnload(PDRIVER_OBJECT DriverObject)
{
    DPRINT ("BeepUnload() called!\n");
    return (STATUS_SUCCESS);
}


NTSTATUS
STDCALL
DriverEntry (PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION:  Called by the system to initalize the driver
 * ARGUMENTS:
 *            DriverObject = object describing this driver
 *            RegistryPath = path to our configuration entries
 * RETURNS:   Success or failure
 */
{
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymlinkName;
    NTSTATUS Status;

    DbgPrint ("Beep Device Driver 0.0.2\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = BeepCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BeepClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BeepCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BeepDeviceControl;
    DriverObject->DriverUnload = BeepUnload;

    /* set up device extension */
    DeviceExtension = DeviceObject->DeviceExtension;
    DeviceExtension->BeepOn = FALSE;

    KeInitializeDpc (&(DeviceExtension->Dpc),
                     BeepDPC,
                     DeviceExtension);
    KeInitializeTimer (&(DeviceExtension->Timer));
    KeInitializeEvent (&(DeviceExtension->Event),
                       SynchronizationEvent,
                       FALSE);

    RtlInitUnicodeString (&DeviceName, L"\\Device\\Beep");
    Status = IoCreateDevice (DriverObject,
                             sizeof(DEVICE_EXTENSION),
                             &DeviceName,
                             FILE_DEVICE_BEEP,
                             0,
                             FALSE,
                             &DeviceObject);
    if (NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString (&SymlinkName, L"\\??\\Beep");
    IoCreateSymbolicLink (&SymlinkName, &DeviceName);

    return (STATUS_SUCCESS);
}

/* EOF */