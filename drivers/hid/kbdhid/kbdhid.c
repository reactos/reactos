/*
 * PROJECT:     ReactOS HID Stack
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/hid/kbdhid/kbdhid.c
 * PURPOSE:     Keyboard HID Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "kbdhid.h"

/* This structure starts with the same layout as KEYBOARD_INDICATOR_TRANSLATION */
typedef struct _LOCAL_KEYBOARD_INDICATOR_TRANSLATION {
    USHORT NumberOfIndicatorKeys;
    INDICATOR_LIST IndicatorList[3];
} LOCAL_KEYBOARD_INDICATOR_TRANSLATION, *PLOCAL_KEYBOARD_INDICATOR_TRANSLATION;

static LOCAL_KEYBOARD_INDICATOR_TRANSLATION IndicatorTranslation = { 3, {
    {0x3A, KEYBOARD_CAPS_LOCK_ON},
    {0x45, KEYBOARD_NUM_LOCK_ON},
    {0x46, KEYBOARD_SCROLL_LOCK_ON}}};


VOID
KbdHid_DispatchInputData(
    IN PKBDHID_DEVICE_EXTENSION DeviceExtension,
    IN PKEYBOARD_INPUT_DATA InputData)
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

BOOLEAN
NTAPI
KbdHid_InsertScanCodes(
    IN PVOID  Context,
    IN PCHAR  NewScanCodes,
    IN ULONG  Length)
{
    KEYBOARD_INPUT_DATA InputData;
    ULONG Index;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;
    CHAR Prefix = 0;

    /* get device extension */
    DeviceExtension = Context;

    for(Index = 0; Index < Length; Index++)
    {
        DPRINT("[KBDHID] ScanCode Index %lu ScanCode %x\n", Index, NewScanCodes[Index] & 0xFF);

        /* check if this is E0 or E1 prefix */
        if (NewScanCodes[Index] == (CHAR)0xE0 || NewScanCodes[Index] == (CHAR)0xE1)
        {
            Prefix = NewScanCodes[Index];
            continue;
        }

        /* init input data */
        RtlZeroMemory(&InputData, sizeof(KEYBOARD_INPUT_DATA));

        /* use keyboard unit id */
        InputData.UnitId = DeviceExtension->KeyboardTypematic.UnitId;

        if (NewScanCodes[Index] & 0x80)
        {
            /* scan codes with 0x80 flag are a key break */
            InputData.Flags |= KEY_BREAK;
        }

        /* set a prefix if needed */
        if (Prefix)
        {
            InputData.Flags |= (Prefix == (CHAR)0xE0 ? KEY_E0 : KEY_E1);
            Prefix = 0;
        }

        /* store key code */
        InputData.MakeCode = NewScanCodes[Index] & 0x7F;

        /* dispatch scan codes */
        KbdHid_DispatchInputData(Context, &InputData);
    }

    /* done */
    return TRUE;
}


NTSTATUS
NTAPI
KbdHid_ReadCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    PKBDHID_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    ULONG ButtonLength;

    /* get device extension */
    DeviceExtension = Context;

    if (Irp->IoStatus.Status == STATUS_PRIVILEGE_NOT_HELD ||
        Irp->IoStatus.Status == STATUS_DEVICE_NOT_CONNECTED ||
        Irp->IoStatus.Status == STATUS_CANCELLED ||
        DeviceExtension->StopReadReport)
    {
        /* failed to read or should be stopped*/
        DPRINT1("[KBDHID] ReadCompletion terminating read Status %x\n", Irp->IoStatus.Status);

        /* report no longer active */
        DeviceExtension->ReadReportActive = FALSE;

        /* request stopping of the report cycle */
        DeviceExtension->StopReadReport = FALSE;

        /* signal completion event */
        KeSetEvent(&DeviceExtension->ReadCompletionEvent, 0, 0);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // print out raw report
    //
    ASSERT(DeviceExtension->ReportLength >= 9);
    DPRINT("[KBDHID] ReadCompletion %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", DeviceExtension->Report[0], DeviceExtension->Report[1], DeviceExtension->Report[2],
        DeviceExtension->Report[3], DeviceExtension->Report[4], DeviceExtension->Report[5],
        DeviceExtension->Report[6], DeviceExtension->Report[7], DeviceExtension->Report[8]);


    /* get current usages */
    ButtonLength = DeviceExtension->UsageListLength;
    Status = HidP_GetUsagesEx(HidP_Input,
                              HIDP_LINK_COLLECTION_UNSPECIFIED,
                              DeviceExtension->CurrentUsageList,
                              &ButtonLength,
                              DeviceExtension->PreparsedData,
                              DeviceExtension->Report,
                              DeviceExtension->ReportLength);
    ASSERT(Status == HIDP_STATUS_SUCCESS);

    /* FIXME check if needs mapping */

    /* get usage difference */
    Status = HidP_UsageAndPageListDifference(DeviceExtension->PreviousUsageList,
                                             DeviceExtension->CurrentUsageList,
                                             DeviceExtension->BreakUsageList,
                                             DeviceExtension->MakeUsageList,
                                             DeviceExtension->UsageListLength);
    ASSERT(Status == HIDP_STATUS_SUCCESS);

    /* replace previous usage list with current list */
    RtlMoveMemory(DeviceExtension->PreviousUsageList,
                  DeviceExtension->CurrentUsageList,
                  sizeof(USAGE_AND_PAGE) * DeviceExtension->UsageListLength);

    /* translate break usage list */
    HidP_TranslateUsageAndPagesToI8042ScanCodes(DeviceExtension->BreakUsageList,
                                                DeviceExtension->UsageListLength,
                                                HidP_Keyboard_Break,
                                                &DeviceExtension->ModifierState,
                                                KbdHid_InsertScanCodes,
                                                DeviceExtension);
    ASSERT(Status == HIDP_STATUS_SUCCESS);

    /* translate new usage list */
    HidP_TranslateUsageAndPagesToI8042ScanCodes(DeviceExtension->MakeUsageList,
                                                DeviceExtension->UsageListLength,
                                                HidP_Keyboard_Make,
                                                &DeviceExtension->ModifierState,
                                                KbdHid_InsertScanCodes,
                                                DeviceExtension);
    ASSERT(Status == HIDP_STATUS_SUCCESS);

    /* re-init read */
    KbdHid_InitiateRead(DeviceExtension);

    /* stop completion */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
KbdHid_InitiateRead(
    IN PKBDHID_DEVICE_EXTENSION DeviceExtension)
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
    IoSetCompletionRoutine(DeviceExtension->Irp, KbdHid_ReadCompletion, DeviceExtension, TRUE, TRUE, TRUE);

    /* read is active */
    DeviceExtension->ReadReportActive = TRUE;

    /* start the read */
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, DeviceExtension->Irp);

    /* done */
    return Status;
}

NTSTATUS
NTAPI
KbdHid_CreateCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    KeSetEvent((PKEVENT)Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
NTAPI
KbdHid_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    KEVENT Event;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    DPRINT("[KBDHID]: IRP_MJ_CREATE\n");

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* copy stack location to next */
    IoCopyCurrentIrpStackLocationToNext(Irp);

    /* init event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* prepare irp */
    IoSetCompletionRoutine(Irp, KbdHid_CreateCompletion, &Event, TRUE, TRUE, TRUE);

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
             KeClearEvent(&DeviceExtension->ReadCompletionEvent);

             /* initiating read */
             Status = KbdHid_InitiateRead(DeviceExtension);
             DPRINT("[KBDHID] KbdHid_InitiateRead: status %x\n", Status);
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
KbdHid_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    DPRINT("[KBDHID] IRP_MJ_CLOSE ReadReportActive %x\n", DeviceExtension->ReadReportActive);

    if (DeviceExtension->ReadReportActive)
    {
        /* request stopping of the report cycle */
        DeviceExtension->StopReadReport = TRUE;

        /* wait until the reports have been read */
        KeWaitForSingleObject(&DeviceExtension->ReadCompletionEvent, Executive, KernelMode, FALSE, NULL);

        /* cancel irp */
        IoCancelIrp(DeviceExtension->Irp);
    }

    DPRINT("[KBDHID] IRP_MJ_CLOSE ReadReportActive %x\n", DeviceExtension->ReadReportActive);

    /* remove file object */
    DeviceExtension->FileObject = NULL;

    /* skip location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* pass irp to down the stack */
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
KbdHid_InternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;
    PCONNECT_DATA Data;
    PKEYBOARD_ATTRIBUTES Attributes;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("[KBDHID] InternalDeviceControl %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
            /* verify output buffer length */
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KEYBOARD_ATTRIBUTES))
            {
                /* invalid request */
                DPRINT1("[KBDHID] IOCTL_KEYBOARD_QUERY_ATTRIBUTES Buffer too small\n");
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }

            /* get output buffer */
            Attributes = Irp->AssociatedIrp.SystemBuffer;

            /* copy attributes */
            RtlCopyMemory(Attributes,
                          &DeviceExtension->Attributes,
                          sizeof(KEYBOARD_ATTRIBUTES));

            /* complete request */
            Irp->IoStatus.Information = sizeof(KEYBOARD_ATTRIBUTES);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_INTERNAL_KEYBOARD_CONNECT:
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

        case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:
            /* not implemented */
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;

        case IOCTL_INTERNAL_KEYBOARD_ENABLE:
            /* not supported */
            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_SUPPORTED;

        case IOCTL_INTERNAL_KEYBOARD_DISABLE:
            /* not supported */
            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_SUPPORTED;

        case IOCTL_KEYBOARD_QUERY_INDICATORS:
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS))
            {
                /* buffer too small */
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }

            /* copy indicators */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          &DeviceExtension->KeyboardIndicator,
                          sizeof(KEYBOARD_INDICATOR_PARAMETERS));

            /* complete request */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;

        case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KEYBOARD_TYPEMATIC_PARAMETERS))
            {
                /* buffer too small */
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }

            /* copy indicators */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          &DeviceExtension->KeyboardTypematic,
                          sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));

            /* done */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(KEYBOARD_TYPEMATIC_PARAMETERS);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_KEYBOARD_SET_INDICATORS:
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS))
            {
                /* invalid parameter */
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            /* copy indicators */
            RtlCopyMemory(&DeviceExtension->KeyboardIndicator,
                          Irp->AssociatedIrp.SystemBuffer,
                          sizeof(KEYBOARD_INDICATOR_PARAMETERS));

            /* done */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_KEYBOARD_SET_TYPEMATIC:
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KEYBOARD_TYPEMATIC_PARAMETERS))
            {
                /* invalid parameter */
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            /* copy indicators */
            RtlCopyMemory(&DeviceExtension->KeyboardTypematic,
                          Irp->AssociatedIrp.SystemBuffer,
                          sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));

            /* done */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION))
            {
                /* buffer too small */
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            /* copy translations */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          &IndicatorTranslation,
                          sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));

            /* done */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
    }

    /* unknown control code */
    DPRINT1("[KBDHID] Unknown DeviceControl %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
    /* unknown request not supported */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
KbdHid_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* skip stack location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* pass and forget */
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
KbdHid_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
KbdHid_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
KbdHid_SubmitRequest(
    PDEVICE_OBJECT DeviceObject,
    ULONG IoControlCode,
    ULONG InputBufferSize,
    PVOID InputBuffer,
    ULONG OutputBufferSize,
    PVOID OutputBuffer)
{
    KEVENT Event;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;
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
KbdHid_StartDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    ULONG Buttons;
    HID_COLLECTION_INFORMATION Information;
    PHIDP_PREPARSED_DATA PreparsedData;
    HIDP_CAPS Capabilities;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;
    PUSAGE_AND_PAGE Buffer;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* query collection information */
    Status = KbdHid_SubmitRequest(DeviceObject,
                                  IOCTL_HID_GET_COLLECTION_INFORMATION,
                                  0,
                                  NULL,
                                  sizeof(HID_COLLECTION_INFORMATION),
                                  &Information);
    if (!NT_SUCCESS(Status))
    {
        /* failed to query collection information */
        DPRINT1("[KBDHID] failed to obtain collection information with %x\n", Status);
        return Status;
    }

    /* lets allocate space for preparsed data */
    PreparsedData = ExAllocatePoolWithTag(NonPagedPool, Information.DescriptorSize, KBDHID_TAG);
    if (!PreparsedData)
    {
        /* no memory */
        DPRINT1("[KBDHID] no memory size %u\n", Information.DescriptorSize);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* now obtain the preparsed data */
    Status = KbdHid_SubmitRequest(DeviceObject,
                                  IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                  0,
                                  NULL,
                                  Information.DescriptorSize,
                                  PreparsedData);
    if (!NT_SUCCESS(Status))
    {
        /* failed to get preparsed data */
        DPRINT1("[KBDHID] failed to obtain collection information with %x\n", Status);
        ExFreePoolWithTag(PreparsedData, KBDHID_TAG);
        return Status;
    }

    /* lets get the caps */
    Status = HidP_GetCaps(PreparsedData, &Capabilities);
    if (Status != HIDP_STATUS_SUCCESS)
    {
        /* failed to get capabilities */
        DPRINT1("[KBDHID] failed to obtain caps with %x\n", Status);
        ExFreePoolWithTag(PreparsedData, KBDHID_TAG);
        return Status;
    }

    DPRINT("[KBDHID] Usage %x UsagePage %x InputReportLength %lu\n", Capabilities.Usage, Capabilities.UsagePage, Capabilities.InputReportByteLength);

    /* init input report */
    DeviceExtension->ReportLength = Capabilities.InputReportByteLength;
    ASSERT(DeviceExtension->ReportLength);
    DeviceExtension->Report = ExAllocatePoolWithTag(NonPagedPool, DeviceExtension->ReportLength, KBDHID_TAG);
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
    Buttons = HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_KEYBOARD, PreparsedData);
    DPRINT("[KBDHID] Buttons %lu\n", Buttons);
    ASSERT(Buttons > 0);

    /* now allocate an array for those buttons */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, sizeof(USAGE_AND_PAGE) * 4 * Buttons, KBDHID_TAG);
    if (!Buffer)
    {
        /* no memory */
        ExFreePoolWithTag(PreparsedData, KBDHID_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DeviceExtension->UsageListBuffer = Buffer;

    /* init usage lists */
    RtlZeroMemory(Buffer, sizeof(USAGE_AND_PAGE) * 4 * Buttons);
    DeviceExtension->CurrentUsageList = Buffer;
    Buffer += Buttons;
    DeviceExtension->PreviousUsageList = Buffer;
    Buffer += Buttons;
    DeviceExtension->MakeUsageList = Buffer;
    Buffer += Buttons;
    DeviceExtension->BreakUsageList = Buffer;

    //
    // FIMXE: implement device hacks
    //
    // UsageMappings
    // KeyboardTypeOverride
    // KeyboardSubTypeOverride
    // KeyboardNumberTotalKeysOverride
    // KeyboardNumberFunctionKeysOverride
    // KeyboardNumberIndicatorsOverride

    /* store number of buttons */
    DeviceExtension->UsageListLength = (USHORT)Buttons;

    /* store preparsed data */
    DeviceExtension->PreparsedData = PreparsedData;

    /* completed successfully */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KbdHid_StartDeviceCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    KeSetEvent((PKEVENT)Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
KbdHid_FreeResources(
    IN PDEVICE_OBJECT DeviceObject)
{
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* free resources */
    if (DeviceExtension->PreparsedData)
    {
        ExFreePoolWithTag(DeviceExtension->PreparsedData, KBDHID_TAG);
        DeviceExtension->PreparsedData = NULL;
    }

    if (DeviceExtension->UsageListBuffer)
    {
        ExFreePoolWithTag(DeviceExtension->UsageListBuffer, KBDHID_TAG);
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
        ExFreePoolWithTag(DeviceExtension->Report, KBDHID_TAG);
        DeviceExtension->Report = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KbdHid_Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

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
KbdHid_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    NTSTATUS Status;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[KBDHID] IRP_MJ_PNP Request: %x\n", IoStack->MinorFunction);

    switch (IoStack->MinorFunction)
    {
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        /* free resources */
        KbdHid_FreeResources(DeviceObject);
        /* fall through */
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

        /* cancel irp */
        IoCancelIrp(DeviceExtension->Irp);

        /* free resources */
        KbdHid_FreeResources(DeviceObject);

        /* indicate success */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

        IoFreeIrp(DeviceExtension->Irp);
        IoDetachDevice(DeviceExtension->NextDeviceObject);
        IoDeleteDevice(DeviceObject);
        return Status;

    case IRP_MN_START_DEVICE:
        /* init event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* copy stack location */
        IoCopyCurrentIrpStackLocationToNext (Irp);

        /* set completion routine */
        IoSetCompletionRoutine(Irp, KbdHid_StartDeviceCompletion, &Event, TRUE, TRUE, TRUE);
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
        Status = KbdHid_StartDevice(DeviceObject);
        DPRINT("KbdHid_StartDevice %x\n", Status);

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
KbdHid_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject, NextDeviceObject;
    PKBDHID_DEVICE_EXTENSION DeviceExtension;
    POWER_STATE State;

    /* create device object */
    Status = IoCreateDevice(DriverObject,
                            sizeof(KBDHID_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_KEYBOARD,
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
    RtlZeroMemory(DeviceExtension, sizeof(KBDHID_DEVICE_EXTENSION));

    /* init device extension */
    DeviceExtension->NextDeviceObject = NextDeviceObject;
    KeInitializeEvent(&DeviceExtension->ReadCompletionEvent, NotificationEvent, FALSE);

    /* init keyboard attributes */
    DeviceExtension->Attributes.KeyboardIdentifier.Type = KEYBOARD_TYPE_UNKNOWN;
    DeviceExtension->Attributes.KeyboardIdentifier.Subtype = MICROSOFT_KBD_101_TYPE;
    DeviceExtension->Attributes.NumberOfFunctionKeys = MICROSOFT_KBD_FUNC;
    DeviceExtension->Attributes.NumberOfIndicators = 3; // caps, num lock, scroll lock
    DeviceExtension->Attributes.NumberOfKeysTotal = 101;
    DeviceExtension->Attributes.InputDataQueueLength = 1;
    DeviceExtension->Attributes.KeyRepeatMinimum.Rate = KEYBOARD_TYPEMATIC_RATE_MINIMUM;
    DeviceExtension->Attributes.KeyRepeatMinimum.Delay = KEYBOARD_TYPEMATIC_DELAY_MINIMUM;
    DeviceExtension->Attributes.KeyRepeatMaximum.Rate = KEYBOARD_TYPEMATIC_RATE_DEFAULT;
    DeviceExtension->Attributes.KeyRepeatMaximum.Delay = KEYBOARD_TYPEMATIC_DELAY_MAXIMUM;

    /* allocate irp */
    DeviceExtension->Irp = IoAllocateIrp(NextDeviceObject->StackSize, FALSE);

    /* FIXME handle allocation error */
    ASSERT(DeviceExtension->Irp);

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
KbdHid_Unload(
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
    /* initialize driver object */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = KbdHid_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KbdHid_Close;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = KbdHid_Flush;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KbdHid_DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = KbdHid_InternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = KbdHid_Power;
    DriverObject->MajorFunction[IRP_MJ_PNP] = KbdHid_Pnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KbdHid_SystemControl;
    DriverObject->DriverUnload = KbdHid_Unload;
    DriverObject->DriverExtension->AddDevice = KbdHid_AddDevice;

    /* done */
    return STATUS_SUCCESS;
}
