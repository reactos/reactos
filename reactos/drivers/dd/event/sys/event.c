/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Event.c

Abstract:

    This sample demonstrates one way that a Windows NT kernel-mode driver
    can share and explicitly signal an Event Object with a Win32 application.

    This sample uses the following method:
        The application creates an event object using CreateEvent().
        The app passes the event handle to the driver in a private IOCTL.
        The driver is running in the app's thread context during the IOCTL so 
        there is a valid user-mode handle at that time.
        The driver dereferences the user-mode handle into system space & saves 
        the new system handle for later use.
        The driver signals the event via KeSetEvent() at IRQL <= DISPATCH_LEVEL.
        The driver MUST delete any driver references to the event object at 
        Unload time.

    An alternative method would be to create a named event in the driver via 
    IoCreateNotificationEvent and then open the event in user mode. This API 
    however is new to Windows NT 4.0 so you can not use this method in your 
    NT 3.5x drivers.
    
    Note that this sample's event can be signalled (almost) at will from within 
    the driver. A different event signal can be set when the driver is setup to 
    do asynchronous I/O, and it is opened with FILE_FLAG_OVERLAPPED, and an 
    event handle is passed down in an OVERLAPPED struct from the app's Read, 
    Write, or DeviceIoControl. This different event signal is set by the I/O 
    Manager when the driver calls IoCompleteRequest on a pending Irp. This type 
    of Irp completion signal is not the purpose of this sample, hence the lack of
    Irp queing.

Author:

    Jeff Midkiff    (jeffmi)    23-Jul-96

Enviroment:

    Kernel Mode Only

Revision History:

--*/


//
// INCLUDES
//
#include "ntddk.h"
#include "event.h"

//
// DEFINES
//
#define USER_NAME       L"\\DosDevices\\EVENT"
#define SYSTEM_NAME     L"\\Device\\EVENT"

//
// DATA
//
typedef struct _DEVICE_EXTENSION {
    KDPC    Dpc;
    KTIMER  Timer;
    HANDLE  hEvent;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// PROTOS
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CustomTimerDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine gets called by the system to initialize the driver.

Arguments:

    DriverObject    - the system supplied driver object.
    RegistryPath    - the system supplied registry path for this driver.

Return Value:

    NTSTATUS

--*/

{

    PDEVICE_OBJECT      pDeviceObject;
    PDEVICE_EXTENSION   pDeviceExtension;

    UNICODE_STRING      usSystemName;
    UNICODE_STRING      usUserName;

    NTSTATUS            status;


    KdPrint(("Event!DriverEntry - IN\n"));

    //
    // create the device object
    //
    RtlInitUnicodeString( &usSystemName, SYSTEM_NAME );

    status = IoCreateDevice( 
                DriverObject,               // DriverObject 
                sizeof( DEVICE_EXTENSION ), // DeviceExtensionSize
                &usSystemName,              // DeviceName
                FILE_DEVICE_UNKNOWN,        // DeviceType
                0,                          // DeviceCharacteristics
                TRUE,                       // Exclusive
                &pDeviceObject              // DeviceObject
                );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(("\tIoCreateDevice returned 0x%x\n", status));

        return( status );
    }

    //
    // Set up dispatch entry points for the driver.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]          =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = Dispatch;
    DriverObject->DriverUnload = Unload;

    //
    // Create a symbolic link into user mode for the driver.
    //
    RtlInitUnicodeString( &usUserName, USER_NAME );
    status = IoCreateSymbolicLink( &usUserName, &usSystemName );

    if ( !NT_SUCCESS(status) ) {
        IoDeleteDevice( pDeviceObject );
        KdPrint(("\tIoCreateSymbolicLink returned 0x%x\n", status));

        return( status );
    }

    //
    // establish user-buffer access method
    //
    pDeviceObject->Flags |= DO_BUFFERED_IO;


    //
    // setup the device extension
    //
    pDeviceExtension = pDeviceObject->DeviceExtension;

    KeInitializeDpc(
        &pDeviceExtension->Dpc, // Dpc
        CustomTimerDPC,         // DeferredRoutine
        pDeviceObject           // DeferredContext
        ); 

    KeInitializeTimer(
        &pDeviceExtension->Timer  // Timer
        );

    pDeviceExtension->hEvent = NULL;

    KdPrint(("Event!DriverEntry - OUT\n"));

    return( status );
}



NTSTATUS
Unload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine gets called to remove the driver from the system.

Arguments:

    DriverObject    - the system supplied driver object.

Return Value:

    NTSTATUS

--*/

{

    PDEVICE_OBJECT       pDeviceObject = DriverObject->DeviceObject;
    PDEVICE_EXTENSION    pDeviceExtension = pDeviceObject->DeviceExtension;
    UNICODE_STRING       usUserName;


    KdPrint(("Event!Unload\n"));

    //
    // dereference the event object or it will NEVER go away until reboot
    //
    if ( pDeviceExtension->hEvent )
    ObDereferenceObject( pDeviceExtension->hEvent );

    // Delete the user-mode symbolic link.
    RtlInitUnicodeString( &usUserName, USER_NAME );
    IoDeleteSymbolicLink( &usUserName );

    // Delete the DeviceObject
    IoDeleteDevice( pDeviceObject );

    return( STATUS_SUCCESS );
}



NTSTATUS
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles IOCTLs.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NTSTATUS

--*/

{

    PDEVICE_EXTENSION   pDeviceExtension;
    PIO_STACK_LOCATION  pIrpStack;
    PSET_EVENT          pSetEvent;

    ULONG               ulInformation = 0L;
    NTSTATUS            status = STATUS_NOT_IMPLEMENTED;


    KdPrint(("Event!Dispatch - IN\n"));

    pDeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( Irp );

    switch( pIrpStack->MajorFunction )
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            KdPrint(("\t%s\n", (IRP_MJ_CREATE == pIrpStack->MajorFunction) ? "IRP_MJ_CREATE" : "IRP_MJ_CLOSE"));

            status = STATUS_SUCCESS;
            break;

        case IRP_MJ_DEVICE_CONTROL:
            switch( pIrpStack->Parameters.DeviceIoControl.IoControlCode )
            {
                case IOCTL_SET_EVENT:
                    KdPrint(("\tIOCTL_SET_EVENT\n"));

                    if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength <  SIZEOF_SETEVENT ) {
                        // Parameters are invalid
                        KdPrint(("\tSTATUS_INVALID_PARAMETER\n"));

                        status = STATUS_INVALID_PARAMETER;
                    } else {
                        pSetEvent = (PSET_EVENT)Irp->AssociatedIrp.SystemBuffer;
                        KdPrint(("\tuser-mode HANDLE = 0x%x\n",  pSetEvent->hEvent ));

                        status = ObReferenceObjectByHandle( pSetEvent->hEvent,
                                                            SYNCHRONIZE,
                                                            NULL,
                                                            KernelMode,
                                                            &pDeviceExtension->hEvent,
                                                            NULL
                                                            );
                        if ( !NT_SUCCESS(status) ) {

                            KdPrint(("\tUnable to reference User-Mode Event object, Error = 0x%x\n", status));

                        } else {
                        
                            KdPrint(("\tkernel-mode HANDLE = 0x%x\n",  pDeviceExtension->hEvent ));

                            //
                            // Start the timer to run the CustomTimerDPC in DueTime seconds to
                            // simulate an interrupt (which would queue a DPC).
                            // The user's event object is signaled in the DPC.
                            //

                            // ensure relative time for this sample
                            if ( pSetEvent->DueTime.QuadPart > 0 )
                                pSetEvent->DueTime.QuadPart = -(pSetEvent->DueTime.QuadPart);
                            KdPrint(("\tDueTime  = %d\n",  pSetEvent->DueTime.QuadPart ));

                            KeSetTimer(
                                &pDeviceExtension->Timer,   // Timer
                                pSetEvent->DueTime,         // DueTime
                                &pDeviceExtension->Dpc      // Dpc  
                                );

                           status = STATUS_SUCCESS;
                        }
                    }
                    break;

                default:
                    // should never hit this
                    ASSERT(0);
                    status = STATUS_NOT_IMPLEMENTED;
                    break;

            } // switch IoControlCode
            break;

        default:
            // should never hit this
            ASSERT(0);
            status = STATUS_NOT_IMPLEMENTED;
            break;

    } // switch MajorFunction


    //
    // complete the Irp
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = ulInformation;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    KdPrint(("Event!Dispatch - OUT\n"));
    return status;
}



VOID
CustomTimerDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the DPC associated with this drivers Timer object setup in DriverEntry.

Arguments:

    Dpc             -   our DPC object associated with our Timer
    DeferredContext -   Context for the DPC that we setup in DriverEntry
    SystemArgument1 -
    SystemArgument2 -

Return Value:

    Nothing.

--*/

{

    PDEVICE_OBJECT      pDeviceObject = DeferredContext;
    PDEVICE_EXTENSION   pDeviceExtension = pDeviceObject->DeviceExtension;


    KdPrint(("Event!CustomTimerDPC - IN\n"));

    //
    // Signal the Event created user-mode
    //
    // Note:  
    // Do not call KeSetEvent from your ISR;
    // you must call it at IRQL <= DISPATCH_LEVEL.
    // Your ISR should queue a DPC and the DPC can
    // then call KeSetEvent on the ISR's behalf.
    //
    KeSetEvent((PKEVENT)pDeviceExtension->hEvent,// Event
                0,                                   // Increment
                FALSE                                // Wait
                );

    // there is no Irp to complete here

    KdPrint(("Event!CustomTimerDPC - OUT\n"));

    return;
}


// EOF