/*
 * PROJECT:     ReactOS HID Stack
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/hid/mouhid/mouhid.c
 * PURPOSE:     Mouse HID Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "mouhid.h"

static USHORT MouHid_ButtonUpFlags[] =
{
    0xFF, /* unused */
    MOUSE_LEFT_BUTTON_DOWN,
    MOUSE_RIGHT_BUTTON_DOWN,
    MOUSE_MIDDLE_BUTTON_DOWN,
    MOUSE_BUTTON_4_DOWN,
    MOUSE_BUTTON_5_DOWN
};

static USHORT MouHid_ButtonDownFlags[] =
{
    0xFF, /* unused */
    MOUSE_LEFT_BUTTON_UP,
    MOUSE_RIGHT_BUTTON_UP,
    MOUSE_MIDDLE_BUTTON_UP,
    MOUSE_BUTTON_4_UP,
    MOUSE_BUTTON_5_UP
};


VOID
MouHid_GetButtonMove(
    IN PMOUHID_DEVICE_EXTENSION DeviceExtension,
    OUT PLONG LastX,
    OUT PLONG LastY)
{
    NTSTATUS Status;
    ULONG ValueX, ValueY;

    /* init result */
    *LastX = 0;
    *LastY = 0;

    if (!DeviceExtension->MouseAbsolute) 
    {
        /* get scaled usage value x */
        Status =  HidP_GetScaledUsageValue(HidP_Input,
                                       HID_USAGE_PAGE_GENERIC,
                                       HIDP_LINK_COLLECTION_UNSPECIFIED,
                                       HID_USAGE_GENERIC_X,
                                       LastX,
                                       DeviceExtension->PreparsedData,
                                       DeviceExtension->Report,
                                       DeviceExtension->ReportLength);

        if (Status != HIDP_STATUS_SUCCESS)
        {
            /* FIXME: handle more errors */
            if (Status == HIDP_STATUS_BAD_LOG_PHY_VALUES)
            {
                /* FIXME: assume it operates in absolute mode */
                DeviceExtension->MouseAbsolute = TRUE;

                /* get unscaled value */
                Status = HidP_GetUsageValue(HidP_Input,
                                        HID_USAGE_PAGE_GENERIC,
                                        HIDP_LINK_COLLECTION_UNSPECIFIED,
                                        HID_USAGE_GENERIC_X,
                                        &ValueX,
                                        DeviceExtension->PreparsedData,
                                        DeviceExtension->Report,
                                        DeviceExtension->ReportLength);

                /* FIXME handle error */
                ASSERT(Status == HIDP_STATUS_SUCCESS);

                /* absolute pointing devices values need be in range 0 - 0xffff */
                ASSERT(DeviceExtension->ValueCapsX.LogicalMax > 0);
                ASSERT(DeviceExtension->ValueCapsX.LogicalMax > DeviceExtension->ValueCapsX.LogicalMin);

                /* convert to logical range */
                *LastX = (ValueX * VIRTUAL_SCREEN_SIZE_X) / DeviceExtension->ValueCapsX.LogicalMax;
            }
        }
    }
    else
    {
        /* get unscaled value */
        Status = HidP_GetUsageValue(HidP_Input,
                                    HID_USAGE_PAGE_GENERIC,
                                    HIDP_LINK_COLLECTION_UNSPECIFIED,
                                    HID_USAGE_GENERIC_X,
                                    &ValueX,
                                    DeviceExtension->PreparsedData,
                                    DeviceExtension->Report,
                                    DeviceExtension->ReportLength);

        /* FIXME handle error */
        ASSERT(Status == HIDP_STATUS_SUCCESS);

        /* absolute pointing devices values need be in range 0 - 0xffff */
        ASSERT(DeviceExtension->ValueCapsX.LogicalMax > 0);
        ASSERT(DeviceExtension->ValueCapsX.LogicalMax > DeviceExtension->ValueCapsX.LogicalMin);

        /* convert to logical range */
        *LastX = (ValueX * VIRTUAL_SCREEN_SIZE_X) / DeviceExtension->ValueCapsX.LogicalMax;
    }

    if (!DeviceExtension->MouseAbsolute)
    {
        /* get scaled usage value y */
        Status =  HidP_GetScaledUsageValue(HidP_Input,
                                       HID_USAGE_PAGE_GENERIC,
                                       HIDP_LINK_COLLECTION_UNSPECIFIED,
                                       HID_USAGE_GENERIC_Y,
                                       LastY,
                                       DeviceExtension->PreparsedData,
                                       DeviceExtension->Report,
                                       DeviceExtension->ReportLength);

        if (Status != HIDP_STATUS_SUCCESS)
        {
            // FIXME: handle more errors
            if (Status == HIDP_STATUS_BAD_LOG_PHY_VALUES)
            {
                // assume it operates in absolute mode
                DeviceExtension->MouseAbsolute = TRUE;

                // get unscaled value
                Status = HidP_GetUsageValue(HidP_Input,
                                        HID_USAGE_PAGE_GENERIC,
                                        HIDP_LINK_COLLECTION_UNSPECIFIED,
                                        HID_USAGE_GENERIC_Y,
                                        &ValueY,
                                        DeviceExtension->PreparsedData,
                                        DeviceExtension->Report,
                                        DeviceExtension->ReportLength);

                /* FIXME handle error */
                ASSERT(Status == HIDP_STATUS_SUCCESS);

                /* absolute pointing devices values need be in range 0 - 0xffff */
                ASSERT(DeviceExtension->ValueCapsY.LogicalMax > 0);
                ASSERT(DeviceExtension->ValueCapsY.LogicalMax > DeviceExtension->ValueCapsY.LogicalMin);

                /* convert to logical range */
                *LastY = (ValueY * VIRTUAL_SCREEN_SIZE_Y) / DeviceExtension->ValueCapsY.LogicalMax;
            }
        }
    }
    else
    {
        // get unscaled value
        Status = HidP_GetUsageValue(HidP_Input,
                                HID_USAGE_PAGE_GENERIC,
                                HIDP_LINK_COLLECTION_UNSPECIFIED,
                                HID_USAGE_GENERIC_Y,
                                &ValueY,
                                DeviceExtension->PreparsedData,
                                DeviceExtension->Report,
                                DeviceExtension->ReportLength);

        /* FIXME handle error */
        ASSERT(Status == HIDP_STATUS_SUCCESS);

        /* absolute pointing devices values need be in range 0 - 0xffff */
        ASSERT(DeviceExtension->ValueCapsY.LogicalMax > 0);
        ASSERT(DeviceExtension->ValueCapsY.LogicalMax > DeviceExtension->ValueCapsY.LogicalMin);

        /* convert to logical range */
        *LastY = (ValueY * VIRTUAL_SCREEN_SIZE_Y) / DeviceExtension->ValueCapsY.LogicalMax;
    }
}

VOID
MouHid_GetButtonFlags(
    IN PMOUHID_DEVICE_EXTENSION DeviceExtension,
    OUT PUSHORT ButtonFlags,
    OUT PUSHORT Flags)
{
    NTSTATUS Status;
    USAGE Usage;
    ULONG Index;
    PUSAGE TempList;
    ULONG CurrentUsageListLength;

    /* init flags */
    *ButtonFlags = 0;
    *Flags = 0;

    /* get usages */
    CurrentUsageListLength = DeviceExtension->UsageListLength;
    Status = HidP_GetUsages(HidP_Input,
                            HID_USAGE_PAGE_BUTTON,
                            HIDP_LINK_COLLECTION_UNSPECIFIED,
                            DeviceExtension->CurrentUsageList,
                            &CurrentUsageListLength,
                            DeviceExtension->PreparsedData,
                            DeviceExtension->Report,
                            DeviceExtension->ReportLength);
    if (Status != HIDP_STATUS_SUCCESS)
    {
        DPRINT1("MouHid_GetButtonFlags failed to get usages with %x\n", Status);
        return;
    }

    /* extract usage list difference */
    Status = HidP_UsageListDifference(DeviceExtension->PreviousUsageList,
                                      DeviceExtension->CurrentUsageList,
                                      DeviceExtension->BreakUsageList,
                                      DeviceExtension->MakeUsageList,
                                      DeviceExtension->UsageListLength);
    if (Status != HIDP_STATUS_SUCCESS)
    {
        DPRINT1("MouHid_GetButtonFlags failed to get usages with %x\n", Status);
        return;
    }

    if (DeviceExtension->UsageListLength)
    {
        Index = 0;
        do
        {
            /* get usage */
            Usage = DeviceExtension->BreakUsageList[Index];
            if (!Usage)
                break;

            if (Usage <= 5)
            {
                /* max 5 buttons supported */
                *ButtonFlags |= MouHid_ButtonDownFlags[Usage];
            }

            /* move to next index*/
            Index++;
        }while(Index < DeviceExtension->UsageListLength);
    }

    if (DeviceExtension->UsageListLength)
    {
        Index = 0;
        do
        {
            /* get usage */
            Usage = DeviceExtension->MakeUsageList[Index];
            if (!Usage)
                break;

            if (Usage <= 5)
            {
                /* max 5 buttons supported */
                *ButtonFlags |= MouHid_ButtonUpFlags[Usage];
            }

            /* move to next index*/
            Index++;
        }while(Index < DeviceExtension->UsageListLength);
    }

    /* now switch the previous list with current list */
    TempList = DeviceExtension->CurrentUsageList;
    DeviceExtension->CurrentUsageList = DeviceExtension->PreviousUsageList;
    DeviceExtension->PreviousUsageList = TempList;

    if (DeviceExtension->MouseAbsolute)
    {
        // mouse operates absolute
        *Flags |= MOUSE_MOVE_ABSOLUTE;
    }
}

VOID
MouHid_DispatchInputData(
    IN PMOUHID_DEVICE_EXTENSION DeviceExtension,
    IN PMOUSE_INPUT_DATA InputData)
{
    KIRQL OldIrql;
    ULONG InputDataConsumed;

    if (!DeviceExtension->ClassService)
        return;

    /* sanity check */
    ASSERT(DeviceExtension->ClassService);
    ASSERT(DeviceExtension->ClassDeviceObject);

    /* raise irql */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* dispatch input data */
    (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassService)(DeviceExtension->ClassDeviceObject, InputData, InputData + 1, &InputDataConsumed);

    /* lower irql to previous level */
    KeLowerIrql(OldIrql);
}

NTSTATUS
NTAPI
MouHid_ReadCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    USHORT ButtonFlags;
    LONG UsageValue;
    NTSTATUS Status;
    LONG LastX, LastY;
    MOUSE_INPUT_DATA MouseInputData;
    USHORT Flags;

    /* get device extension */
    DeviceExtension = Context;

    if (Irp->IoStatus.Status == STATUS_PRIVILEGE_NOT_HELD ||
        Irp->IoStatus.Status == STATUS_DEVICE_NOT_CONNECTED ||
        Irp->IoStatus.Status == STATUS_CANCELLED ||
        DeviceExtension->StopReadReport)
    {
        /* failed to read or should be stopped*/
        DPRINT1("[MOUHID] ReadCompletion terminating read Status %x\n", Irp->IoStatus.Status);

        /* report no longer active */
        DeviceExtension->ReadReportActive = FALSE;

        /* request stopping of the report cycle */
        DeviceExtension->StopReadReport = FALSE;

        /* signal completion event */
        KeSetEvent(&DeviceExtension->ReadCompletionEvent, 0, 0);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* get mouse change */
    MouHid_GetButtonMove(DeviceExtension, &LastX, &LastY);

    /* get mouse change flags */
    MouHid_GetButtonFlags(DeviceExtension, &ButtonFlags, &Flags);

    /* init input data */
    RtlZeroMemory(&MouseInputData, sizeof(MOUSE_INPUT_DATA));

    /* init input data */
    MouseInputData.ButtonFlags = ButtonFlags;
    MouseInputData.Flags = Flags;
    MouseInputData.LastX = LastX;
    MouseInputData.LastY = LastY;

    /* detect mouse wheel change */
    if (DeviceExtension->MouseIdentifier == WHEELMOUSE_HID_HARDWARE)
    {
        /* get usage */
        UsageValue = 0;
        Status = HidP_GetScaledUsageValue(HidP_Input,
                                          HID_USAGE_PAGE_GENERIC,
                                          HIDP_LINK_COLLECTION_UNSPECIFIED,
                                          HID_USAGE_GENERIC_WHEEL,
                                          &UsageValue,
                                          DeviceExtension->PreparsedData,
                                          DeviceExtension->Report,
                                          DeviceExtension->ReportLength);
        if (Status == HIDP_STATUS_SUCCESS && UsageValue != 0)
        {
            /* store wheel status */
            MouseInputData.ButtonFlags |= MOUSE_WHEEL;
            MouseInputData.ButtonData = (USHORT)(UsageValue * WHEEL_DELTA);
        }
        else
        {
            DPRINT("[MOUHID] failed to get wheel status with %x\n", Status);
        }
    }

    DPRINT("[MOUHID] ReportData %02x %02x %02x %02x %02x %02x %02x\n",
        DeviceExtension->Report[0] & 0xFF,
        DeviceExtension->Report[1] & 0xFF, DeviceExtension->Report[2] & 0xFF,
        DeviceExtension->Report[3] & 0xFF, DeviceExtension->Report[4] & 0xFF,
        DeviceExtension->Report[5] & 0xFF, DeviceExtension->Report[6] & 0xFF);

    DPRINT("[MOUHID] LastX %ld LastY %ld Flags %x ButtonFlags %x ButtonData %x\n", MouseInputData.LastX, MouseInputData.LastY, MouseInputData.Flags, MouseInputData.ButtonFlags, MouseInputData.ButtonData);

    /* dispatch mouse action */
    MouHid_DispatchInputData(DeviceExtension, &MouseInputData);

    /* re-init read */
    MouHid_InitiateRead(DeviceExtension);

    /* stop completion */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
MouHid_InitiateRead(
    IN PMOUHID_DEVICE_EXTENSION DeviceExtension)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* re-use irp */
    IoReuseIrp(DeviceExtension->Irp, STATUS_SUCCESS);

    /* init irp */
    DeviceExtension->Irp->MdlAddress = DeviceExtension->ReportMDL;

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(DeviceExtension->Irp);

    /* init stack location */
    IoStack->Parameters.Read.Length = DeviceExtension->ReportLength;
    IoStack->Parameters.Read.Key = 0;
    IoStack->Parameters.Read.ByteOffset.QuadPart = 0LL;
    IoStack->MajorFunction = IRP_MJ_READ;
    IoStack->FileObject = DeviceExtension->FileObject;

    /* set completion routine */
    IoSetCompletionRoutine(DeviceExtension->Irp, MouHid_ReadCompletion, DeviceExtension, TRUE, TRUE, TRUE);

    /* read is active */
    DeviceExtension->ReadReportActive = TRUE;

    /* start the read */
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, DeviceExtension->Irp);

    /* done */
    return Status;
}

NTSTATUS
NTAPI
MouHid_CreateCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    KeSetEvent(Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
NTAPI
MouHid_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    KEVENT Event;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    DPRINT("MOUHID: IRP_MJ_CREATE\n");

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* copy stack location to next */
    IoCopyCurrentIrpStackLocationToNext(Irp);

    /* init event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* prepare irp */
    IoSetCompletionRoutine(Irp, MouHid_CreateCompletion, &Event, TRUE, TRUE, TRUE);

    /* call lower driver */
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* request pending */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* is the driver already in use */
    if (DeviceExtension->FileObject == NULL)
    {
         /* did the caller specify correct attributes */
         ASSERT(IoStack->Parameters.Create.SecurityContext);
         if (IoStack->Parameters.Create.SecurityContext->DesiredAccess)
         {
             /* store file object */
             DeviceExtension->FileObject = IoStack->FileObject;

             /* reset event */
             KeResetEvent(&DeviceExtension->ReadCompletionEvent);

             /* initiating read */
             Status = MouHid_InitiateRead(DeviceExtension);
             DPRINT("[MOUHID] MouHid_InitiateRead: status %x\n", Status);
             if (Status == STATUS_PENDING)
             {
                 /* report irp is pending */
                 Status = STATUS_SUCCESS;
             }
         }
    }

    /* complete request */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
NTAPI
MouHid_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    DPRINT("[MOUHID] IRP_MJ_CLOSE ReadReportActive %x\n", DeviceExtension->ReadReportActive);

    if (DeviceExtension->ReadReportActive)
    {
        /* request stopping of the report cycle */
        DeviceExtension->StopReadReport = TRUE;

        /* wait until the reports have been read */
        KeWaitForSingleObject(&DeviceExtension->ReadCompletionEvent, Executive, KernelMode, FALSE, NULL);

        /* cancel irp */
        IoCancelIrp(DeviceExtension->Irp);
    }

    DPRINT("[MOUHID] IRP_MJ_CLOSE ReadReportActive %x\n", DeviceExtension->ReadReportActive);

    /* remove file object */
    DeviceExtension->FileObject = NULL;

    /* skip location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* pass irp to down the stack */
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
MouHid_InternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PMOUSE_ATTRIBUTES Attributes;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    PCONNECT_DATA Data;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("[MOUHID] InternalDeviceControl %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* handle requests */
    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_MOUSE_QUERY_ATTRIBUTES:
         /* verify output buffer length */
         if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUSE_ATTRIBUTES))
         {
             /* invalid request */
             DPRINT1("[MOUHID] IOCTL_MOUSE_QUERY_ATTRIBUTES Buffer too small\n");
             Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return STATUS_BUFFER_TOO_SMALL;
         }

         /* get output buffer */
         Attributes = Irp->AssociatedIrp.SystemBuffer;

         /* type of mouse */
         Attributes->MouseIdentifier = DeviceExtension->MouseIdentifier;

         /* number of buttons */
         Attributes->NumberOfButtons = DeviceExtension->UsageListLength;

         /* sample rate not used for usb */
         Attributes->SampleRate = 0;

         /* queue length */
         Attributes->InputDataQueueLength = 2;

         DPRINT("[MOUHID] MouseIdentifier %x\n", Attributes->MouseIdentifier);
         DPRINT("[MOUHID] NumberOfButtons %x\n", Attributes->NumberOfButtons);
         DPRINT("[MOUHID] SampleRate %x\n", Attributes->SampleRate);
         DPRINT("[MOUHID] InputDataQueueLength %x\n", Attributes->InputDataQueueLength);

         /* complete request */
         Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
         Irp->IoStatus.Status = STATUS_SUCCESS;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;

    case IOCTL_INTERNAL_MOUSE_CONNECT:
         /* verify input buffer length */
         if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONNECT_DATA))
         {
             /* invalid request */
             Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return STATUS_INVALID_PARAMETER;
         }

         /* is it already connected */
         if (DeviceExtension->ClassService)
         {
             /* already connected */
             Irp->IoStatus.Status = STATUS_SHARING_VIOLATION;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return STATUS_SHARING_VIOLATION;
         }

         /* get connect data */
         Data = IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

         /* store connect details */
         DeviceExtension->ClassDeviceObject = Data->ClassDeviceObject;
         DeviceExtension->ClassService = Data->ClassService;

         /* completed successfully */
         Irp->IoStatus.Status = STATUS_SUCCESS;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;

    case IOCTL_INTERNAL_MOUSE_DISCONNECT:
        /* not supported */
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_IMPLEMENTED;

    case IOCTL_INTERNAL_MOUSE_ENABLE:
        /* not supported */
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    case IOCTL_INTERNAL_MOUSE_DISABLE:
        /* not supported */
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    DPRINT1("[MOUHID] Unknown DeviceControl %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
    /* unknown request not supported */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
MouHid_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* skip stack location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* pass and forget */
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
MouHid_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
MouHid_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
MouHid_SubmitRequest(
    PDEVICE_OBJECT DeviceObject,
    ULONG IoControlCode,
    ULONG InputBufferSize,
    PVOID InputBuffer,
    ULONG OutputBufferSize,
    PVOID OutputBuffer)
{
    KEVENT Event;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    PIRP Irp;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* init event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* build request */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceExtension->NextDeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        OutputBufferSize,
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (!Irp)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* send request */
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* wait for request to complete */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    /* done */
    return Status;
}

NTSTATUS
NTAPI
MouHid_StartDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    ULONG Buttons;
    HID_COLLECTION_INFORMATION Information;
    PVOID PreparsedData;
    HIDP_CAPS Capabilities;
    USHORT ValueCapsLength;
    HIDP_VALUE_CAPS ValueCaps;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    PUSAGE Buffer;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* query collection information */
    Status = MouHid_SubmitRequest(DeviceObject,
                                  IOCTL_HID_GET_COLLECTION_INFORMATION,
                                  0,
                                  NULL,
                                  sizeof(HID_COLLECTION_INFORMATION),
                                  &Information);
    if (!NT_SUCCESS(Status))
    {
        /* failed to query collection information */
        DPRINT1("[MOUHID] failed to obtain collection information with %x\n", Status);
        return Status;
    }

    /* lets allocate space for preparsed data */
    PreparsedData = ExAllocatePoolWithTag(NonPagedPool, Information.DescriptorSize, MOUHID_TAG);
    if (!PreparsedData)
    {
        /* no memory */
        DPRINT1("[MOUHID] no memory size %u\n", Information.DescriptorSize);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* now obtain the preparsed data */
    Status = MouHid_SubmitRequest(DeviceObject,
                                  IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                  0,
                                  NULL,
                                  Information.DescriptorSize,
                                  PreparsedData);
    if (!NT_SUCCESS(Status))
    {
        /* failed to get preparsed data */
        DPRINT1("[MOUHID] failed to obtain collection information with %x\n", Status);
        ExFreePoolWithTag(PreparsedData, MOUHID_TAG);
        return Status;
    }

    /* lets get the caps */
    Status = HidP_GetCaps(PreparsedData, &Capabilities);
    if (Status != HIDP_STATUS_SUCCESS)
    {
        /* failed to get capabilities */
        DPRINT1("[MOUHID] failed to obtain caps with %x\n", Status);
        ExFreePoolWithTag(PreparsedData, MOUHID_TAG);
        return Status;
    }

    DPRINT("[MOUHID] Usage %x UsagePage %x InputReportLength %lu\n", Capabilities.Usage, Capabilities.UsagePage, Capabilities.InputReportByteLength);

    /* verify capabilities */
    if ((Capabilities.Usage != HID_USAGE_GENERIC_POINTER && Capabilities.Usage != HID_USAGE_GENERIC_MOUSE) || Capabilities.UsagePage != HID_USAGE_PAGE_GENERIC)
    {
        /* not supported */
        ExFreePoolWithTag(PreparsedData, MOUHID_TAG);
        return STATUS_UNSUCCESSFUL;
    }

    /* init input report */
    DeviceExtension->ReportLength = Capabilities.InputReportByteLength;
    ASSERT(DeviceExtension->ReportLength);
    DeviceExtension->Report = ExAllocatePoolWithTag(NonPagedPool, DeviceExtension->ReportLength, MOUHID_TAG);
    ASSERT(DeviceExtension->Report);
    RtlZeroMemory(DeviceExtension->Report, DeviceExtension->ReportLength);

    /* build mdl */
    DeviceExtension->ReportMDL = IoAllocateMdl(DeviceExtension->Report,
                                               DeviceExtension->ReportLength,
                                               FALSE,
                                               FALSE,
                                               NULL);
    ASSERT(DeviceExtension->ReportMDL);

    /* init mdl */
    MmBuildMdlForNonPagedPool(DeviceExtension->ReportMDL);

    /* get max number of buttons */
    Buttons = HidP_MaxUsageListLength(HidP_Input,
                                      HID_USAGE_PAGE_BUTTON,
                                      PreparsedData);
    DPRINT("[MOUHID] Buttons %lu\n", Buttons);
    ASSERT(Buttons > 0);

    /* now allocate an array for those buttons */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, sizeof(USAGE) * 4 * Buttons, MOUHID_TAG);
    if (!Buffer)
    {
        /* no memory */
        ExFreePoolWithTag(PreparsedData, MOUHID_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DeviceExtension->UsageListBuffer = Buffer;

    /* init usage lists */
    RtlZeroMemory(Buffer, sizeof(USAGE) * 4 * Buttons);
    DeviceExtension->CurrentUsageList = Buffer;
    Buffer += Buttons;
    DeviceExtension->PreviousUsageList = Buffer;
    Buffer += Buttons;
    DeviceExtension->MakeUsageList = Buffer;
    Buffer += Buttons;
    DeviceExtension->BreakUsageList = Buffer;

    /* store number of buttons */
    DeviceExtension->UsageListLength = (USHORT)Buttons;

    /* store preparsed data */
    DeviceExtension->PreparsedData = PreparsedData;

    ValueCapsLength = 1;
    HidP_GetSpecificValueCaps(HidP_Input,
                              HID_USAGE_PAGE_GENERIC,
                              HIDP_LINK_COLLECTION_UNSPECIFIED,
                              HID_USAGE_GENERIC_X,
                              &DeviceExtension->ValueCapsX,
                              &ValueCapsLength,
                              PreparsedData);

    ValueCapsLength = 1;
    HidP_GetSpecificValueCaps(HidP_Input,
                              HID_USAGE_PAGE_GENERIC,
                              HIDP_LINK_COLLECTION_UNSPECIFIED,
                              HID_USAGE_GENERIC_Y,
                              &DeviceExtension->ValueCapsY,
                              &ValueCapsLength,
                              PreparsedData);

    /* now check for wheel mouse support */
    ValueCapsLength = 1;
    Status = HidP_GetSpecificValueCaps(HidP_Input,
                                       HID_USAGE_PAGE_GENERIC,
                                       HIDP_LINK_COLLECTION_UNSPECIFIED,
                                       HID_USAGE_GENERIC_WHEEL,
                                       &ValueCaps,
                                       &ValueCapsLength,
                                       PreparsedData);
    if (Status == HIDP_STATUS_SUCCESS )
    {
        /* mouse has wheel support */
        DeviceExtension->MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
        DeviceExtension->WheelUsagePage = ValueCaps.UsagePage;
        DPRINT("[MOUHID] mouse wheel support detected\n", Status);
    }
    else
    {
        /* check if the mouse has z-axis */
        ValueCapsLength = 1;
        Status = HidP_GetSpecificValueCaps(HidP_Input,
                                           HID_USAGE_PAGE_GENERIC,
                                           HIDP_LINK_COLLECTION_UNSPECIFIED,
                                           HID_USAGE_GENERIC_Z,
                                           &ValueCaps,
                                           &ValueCapsLength,
                                           PreparsedData);
        if (Status == HIDP_STATUS_SUCCESS && ValueCapsLength == 1)
        {
            /* wheel support */
            DeviceExtension->MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
            DeviceExtension->WheelUsagePage = ValueCaps.UsagePage;
            DPRINT("[MOUHID] mouse wheel support detected with z-axis\n", Status);
        }
    }

    /* check if mice is absolute */
    if (DeviceExtension->ValueCapsY.LogicalMax > DeviceExtension->ValueCapsY.LogicalMin ||
        DeviceExtension->ValueCapsX.LogicalMax > DeviceExtension->ValueCapsX.LogicalMin)
    {
        /* mice is absolute */
        DeviceExtension->MouseAbsolute = TRUE;
    }

    /* completed successfully */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MouHid_StartDeviceCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    KeSetEvent(Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
MouHid_FreeResources(
    IN PDEVICE_OBJECT DeviceObject)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* free resources */
    if (DeviceExtension->PreparsedData)
    {
        ExFreePoolWithTag(DeviceExtension->PreparsedData, MOUHID_TAG);
        DeviceExtension->PreparsedData = NULL;
    }

    if (DeviceExtension->UsageListBuffer)
    {
        ExFreePoolWithTag(DeviceExtension->UsageListBuffer, MOUHID_TAG);
        DeviceExtension->UsageListBuffer = NULL;
        DeviceExtension->CurrentUsageList = NULL;
        DeviceExtension->PreviousUsageList = NULL;
        DeviceExtension->MakeUsageList = NULL;
        DeviceExtension->BreakUsageList = NULL;
    }

    if (DeviceExtension->ReportMDL)
    {
        IoFreeMdl(DeviceExtension->ReportMDL);
        DeviceExtension->ReportMDL = NULL;
    }

    if (DeviceExtension->Report)
    {
        ExFreePoolWithTag(DeviceExtension->Report, MOUHID_TAG);
        DeviceExtension->Report = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MouHid_Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* skip current stack location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* change request to hid flush queue request */
    IoStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_FLUSH_QUEUE;

    /* call device */
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
MouHid_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    NTSTATUS Status;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[MOUHID] IRP_MJ_PNP Request: %x\n", IoStack->MinorFunction);

    switch (IoStack->MinorFunction)
    {
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        /* free resources */
        MouHid_FreeResources(DeviceObject);
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
        /* indicate success */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

    case IRP_MN_REMOVE_DEVICE:
        /* FIXME synchronization */

        /* request stop */
        DeviceExtension->StopReadReport = TRUE;

        /* cancel irp */
        IoCancelIrp(DeviceExtension->Irp);

        /* free resources */
        MouHid_FreeResources(DeviceObject);

        /* indicate success */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

        /* wait for completion of stop event */
        KeWaitForSingleObject(&DeviceExtension->ReadCompletionEvent, Executive, KernelMode, FALSE, NULL);

        /* free irp */
        IoFreeIrp(DeviceExtension->Irp);

        /* detach device */
        IoDetachDevice(DeviceExtension->NextDeviceObject);

        /* delete device */
        IoDeleteDevice(DeviceObject);

        /* done */
        return Status;

    case IRP_MN_START_DEVICE:
        /* init event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* copy stack location */
        IoCopyCurrentIrpStackLocationToNext (Irp);

        /* set completion routine */
        IoSetCompletionRoutine(Irp, MouHid_StartDeviceCompletion, &Event, TRUE, TRUE, TRUE);
        Irp->IoStatus.Status = 0;

        /* pass request */
        Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        if (!NT_SUCCESS(Status))
        {
            /* failed */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        /* lets start the device */
        Status = MouHid_StartDevice(DeviceObject);
        DPRINT("MouHid_StartDevice %x\n", Status);

        /* complete request */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        /* done */
        return Status;

    default:
        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
MouHid_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject, NextDeviceObject;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    POWER_STATE State;

    /* create device object */
    Status = IoCreateDevice(DriverObject,
                            sizeof(MOUHID_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_MOUSE,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create device object */
        return Status;
    }

    /* now attach it */
    NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    if (!NextDeviceObject)
    {
        /* failed to attach */
        IoDeleteDevice(DeviceObject);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* zero extension */
    RtlZeroMemory(DeviceExtension, sizeof(MOUHID_DEVICE_EXTENSION));

    /* init device extension */
    DeviceExtension->MouseIdentifier = MOUSE_HID_HARDWARE;
    DeviceExtension->WheelUsagePage = 0;
    DeviceExtension->NextDeviceObject = NextDeviceObject;
    KeInitializeEvent(&DeviceExtension->ReadCompletionEvent, NotificationEvent, FALSE);
    DeviceExtension->Irp = IoAllocateIrp(NextDeviceObject->StackSize, FALSE);

    /* FIXME handle allocation error */
    ASSERT(DeviceExtension->Irp);

    /* FIXME query parameter 'FlipFlopWheel', 'WheelScalingFactor' */

    /* set power state to D0 */
    State.DeviceState =  PowerDeviceD0;
    PoSetPowerState(DeviceObject, DevicePowerState, State);

    /* init device object */
    DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags  &= ~DO_DEVICE_INITIALIZING;

    /* completed successfully */
    return STATUS_SUCCESS;
}

VOID
NTAPI
MouHid_Unload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
}


NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegPath)
{
    /* FIXME check for parameters 'UseOnlyMice', 'TreatAbsoluteAsRelative', 'TreatAbsolutePointerAsAbsolute' */

    /* initialize driver object */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MouHid_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MouHid_Close;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = MouHid_Flush;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MouHid_DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MouHid_InternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = MouHid_Power;
    DriverObject->MajorFunction[IRP_MJ_PNP] = MouHid_Pnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = MouHid_SystemControl;
    DriverObject->DriverUnload = MouHid_Unload;
    DriverObject->DriverExtension->AddDevice = MouHid_AddDevice;

    /* done */
    return STATUS_SUCCESS;
}
