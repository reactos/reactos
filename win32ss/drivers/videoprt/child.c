/*
 * VideoPort driver
 *
 * Copyright (C) 2012 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"
#include <stdio.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
IntVideoPortGetMonitorId(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    IN OUT PWCHAR Buffer)
{
    USHORT Manufacturer, Model;

    /* This must be valid to call this function */
    ASSERT(ChildExtension->EdidValid);

    /* 3 letters 5-bit ANSI manufacturer code (big endian) */
    /* Letters encoded as A=1 to Z=26 */
    Manufacturer = ((USHORT)ChildExtension->ChildDescriptor[8] << 8) +
                   (USHORT)ChildExtension->ChildDescriptor[9];

    /* Model number (16-bit little endian) */
    Model = ((USHORT)ChildExtension->ChildDescriptor[11] << 8) +
             (USHORT)ChildExtension->ChildDescriptor[10];

    /* Convert the Monitor ID to a readable form */
    swprintf(Buffer,
             L"%C%C%C%04hx",
             (WCHAR)((Manufacturer >> 10 & 0x001F) + 'A' - 1),
             (WCHAR)((Manufacturer >> 5 & 0x001F) + 'A' - 1),
             (WCHAR)((Manufacturer & 0x001F) + 'A' - 1),
             Model);

    /* And we're done */
    return TRUE;
}

BOOLEAN
NTAPI
IntVideoPortSearchDescriptor(
    IN PUCHAR Descriptor,
    IN UCHAR DescriptorID,
    OUT PUCHAR* pDescriptorData)
{
    if (Descriptor[0] != 0 || Descriptor[1] != 0 || Descriptor[2] != 0)
        return FALSE;
    if (Descriptor[3] != DescriptorID)
        return FALSE;

    *pDescriptorData = Descriptor + 4;
    return TRUE;
}

BOOLEAN
NTAPI
IntVideoPortSearchDescriptors(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    IN UCHAR DescriptorID,
    OUT PUCHAR* pDescriptorData)
{
    if (!ChildExtension->EdidValid)
        return FALSE;

    if (IntVideoPortSearchDescriptor(ChildExtension->ChildDescriptor + 0x36, DescriptorID, pDescriptorData))
        return TRUE;
    if (IntVideoPortSearchDescriptor(ChildExtension->ChildDescriptor + 0x48, DescriptorID, pDescriptorData))
        return TRUE;
    if (IntVideoPortSearchDescriptor(ChildExtension->ChildDescriptor + 0x5A, DescriptorID, pDescriptorData))
        return TRUE;
    if (IntVideoPortSearchDescriptor(ChildExtension->ChildDescriptor + 0x6C, DescriptorID, pDescriptorData))
        return TRUE;

    /* FIXME: search in extension? */
    return FALSE;
}

BOOLEAN
NTAPI
IntVideoPortGetMonitorDescription(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    OUT PCHAR* pMonitorDescription)
{
    PUCHAR MonitorDescription;

    if (!IntVideoPortSearchDescriptors(ChildExtension, 0xFC, &MonitorDescription))
        return FALSE;

    *pMonitorDescription = (PCHAR)MonitorDescription;
    return TRUE;
}

NTSTATUS NTAPI
IntVideoPortChildQueryId(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PWCHAR Buffer = NULL, StaticBuffer;
    UNICODE_STRING UnicodeStr;

    switch (IrpSp->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            switch (ChildExtension->ChildType)
            {
                case Monitor:
                    if (ChildExtension->EdidValid)
                    {
                        StaticBuffer = L"DISPLAY\\";
                        Buffer = ExAllocatePool(PagedPool, (wcslen(StaticBuffer) + 8) * sizeof(WCHAR));
                        if (!Buffer) return STATUS_NO_MEMORY;

                        /* Write the static portion */
                        RtlCopyMemory(Buffer, StaticBuffer, wcslen(StaticBuffer) * sizeof(WCHAR));

                        /* Add the dynamic portion */
                        IntVideoPortGetMonitorId(ChildExtension,
                                                 &Buffer[wcslen(StaticBuffer)]);
                    }
                    else
                    {
                        StaticBuffer = L"DISPLAY\\Default_Monitor";
                        Buffer = ExAllocatePool(PagedPool, (wcslen(StaticBuffer) + 1) * sizeof(WCHAR));
                        if (!Buffer) return STATUS_NO_MEMORY;

                        /* Copy the default id */
                        RtlCopyMemory(Buffer, StaticBuffer, (wcslen(StaticBuffer) + 1) * sizeof(WCHAR));
                    }
                    break;
                default:
                    ASSERT(FALSE);
                    break;
            }
            break;
        case BusQueryInstanceID:
            Buffer = ExAllocatePool(PagedPool, 5 * sizeof(WCHAR));
            if (!Buffer) return STATUS_NO_MEMORY;

            UnicodeStr.Buffer = Buffer;
            UnicodeStr.Length = 0;
            UnicodeStr.MaximumLength = 4 * sizeof(WCHAR);
            RtlIntegerToUnicodeString(ChildExtension->ChildId, 16, &UnicodeStr);
            break;
        case BusQueryHardwareIDs:
            switch (ChildExtension->ChildType)
            {
                case Monitor:
                    if (ChildExtension->EdidValid)
                    {
                        StaticBuffer = L"MONITOR\\";
                        Buffer = ExAllocatePool(PagedPool, (wcslen(StaticBuffer) + 9) * sizeof(WCHAR));
                        if (!Buffer) return STATUS_NO_MEMORY;

                        /* Write the static portion */
                        RtlCopyMemory(Buffer, StaticBuffer, wcslen(StaticBuffer) * sizeof(WCHAR));

                        /* Add the dynamic portion */
                        IntVideoPortGetMonitorId(ChildExtension,
                                                 &Buffer[wcslen(StaticBuffer)]);

                        /* Add the second null termination char */
                        Buffer[wcslen(StaticBuffer) + 8] = UNICODE_NULL;
                    }
                    else
                    {
                        StaticBuffer = L"MONITOR\\Default_Monitor";
                        Buffer = ExAllocatePool(PagedPool, (wcslen(StaticBuffer) + 2) * sizeof(WCHAR));
                        if (!Buffer) return STATUS_NO_MEMORY;

                        /* Copy the default id */
                        RtlCopyMemory(Buffer, StaticBuffer, (wcslen(StaticBuffer) + 1) * sizeof(WCHAR));

                        /* Add the second null terminator */
                        Buffer[wcslen(StaticBuffer) + 1] = UNICODE_NULL;
                    }
                    break;
                default:
                    ASSERT(FALSE);
                    break;
            }
            break;
        case BusQueryCompatibleIDs:
            switch (ChildExtension->ChildType)
            {
                case Monitor:
                    if (ChildExtension->EdidValid)
                    {
                        StaticBuffer = L"*PNP09FF";
                        Buffer = ExAllocatePool(PagedPool, (wcslen(StaticBuffer) + 2) * sizeof(WCHAR));
                        if (!Buffer) return STATUS_NO_MEMORY;

                        RtlCopyMemory(Buffer, StaticBuffer, (wcslen(StaticBuffer) + 1) * sizeof(WCHAR));

                        Buffer[wcslen(StaticBuffer)+1] = UNICODE_NULL;
                    }
                    else
                    {
                        /* No PNP ID for non-PnP monitors */
                        return Irp->IoStatus.Status;
                    }
                    break;
                default:
                    ASSERT(FALSE);
                    break;
            }
            break;
        default:
            return Irp->IoStatus.Status;
    }

    INFO_(VIDEOPRT, "Reporting ID: %S\n", Buffer);
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
IntVideoPortChildQueryText(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    NTSTATUS Status;

    if (IrpSp->Parameters.QueryDeviceText.DeviceTextType != DeviceTextDescription)
        return Irp->IoStatus.Status;

    switch (ChildExtension->ChildType)
    {
        case Monitor:
            if (IntVideoPortGetMonitorDescription(ChildExtension,
                                                  &StringA.Buffer))
            {
                StringA.Buffer++; /* Skip reserved byte */
                StringA.MaximumLength = 13;
                for (StringA.Length = 0;
                     StringA.Length < StringA.MaximumLength && StringA.Buffer[StringA.Length] != '\n';
                     StringA.Length++)
                    ;
            }
            else
                RtlInitAnsiString(&StringA, "Monitor");
            break;

        case VideoChip:
            /* FIXME: No idea what we return here */
            RtlInitAnsiString(&StringA, "Video chip");
            break;

        default: /* Other */
            RtlInitAnsiString(&StringA, "Other device");
            break;
    }

    Status = RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    INFO_(VIDEOPRT, "Reporting description: %S\n", StringU.Buffer);
    Irp->IoStatus.Information = (ULONG_PTR)StringU.Buffer;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
IntVideoPortChildQueryRelations(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_RELATIONS DeviceRelations;

    if (IrpSp->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
    {
        WARN_(VIDEOPRT, "Unsupported device relations type\n");
        return Irp->IoStatus.Status;
    }

    DeviceRelations = ExAllocatePool(NonPagedPool, sizeof(DEVICE_RELATIONS));
    if (!DeviceRelations) return STATUS_NO_MEMORY;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = ChildExtension->PhysicalDeviceObject;

    ObReferenceObject(DeviceRelations->Objects[0]);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
IntVideoPortChildQueryCapabilities(
    IN PVIDEO_PORT_CHILD_EXTENSION ChildExtension,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_CAPABILITIES DeviceCaps = IrpSp->Parameters.DeviceCapabilities.Capabilities;
    ULONG i;

    /* Set some values */
    DeviceCaps->LockSupported = FALSE;
    DeviceCaps->EjectSupported = FALSE;
    DeviceCaps->DockDevice = FALSE;
    DeviceCaps->UniqueID = FALSE;
    DeviceCaps->RawDeviceOK = FALSE;
    DeviceCaps->WakeFromD0 = FALSE;
    DeviceCaps->WakeFromD1 = FALSE;
    DeviceCaps->WakeFromD2 = FALSE;
    DeviceCaps->WakeFromD3 = FALSE;
    DeviceCaps->HardwareDisabled = FALSE;
    DeviceCaps->NoDisplayInUI = FALSE;

    /* Address and UI number are set by default */

    DeviceCaps->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    for (i = 1; i < POWER_SYSTEM_MAXIMUM; i++)
    {
        DeviceCaps->DeviceState[i] = PowerDeviceD3;
    }

    DeviceCaps->SystemWake = PowerSystemUnspecified;
    DeviceCaps->DeviceWake = PowerDeviceUnspecified;

    /* FIXME: Device power states */
    DeviceCaps->DeviceD1 = FALSE;
    DeviceCaps->DeviceD2 = FALSE;
    DeviceCaps->D1Latency = 0;
    DeviceCaps->D2Latency = 0;
    DeviceCaps->D3Latency = 0;

    switch (ChildExtension->ChildType)
    {
        case VideoChip:
            /* FIXME: Copy capabilities from parent */
            ASSERT(FALSE);
            break;

        case NonPrimaryChip: /* Reserved */
            ASSERT(FALSE);
            break;

        case Monitor:
            DeviceCaps->SilentInstall = TRUE;
            DeviceCaps->Removable = TRUE;
            DeviceCaps->SurpriseRemovalOK = TRUE;
            break;

        default: /* Other */
            DeviceCaps->SilentInstall = FALSE;
            DeviceCaps->Removable = FALSE;
            DeviceCaps->SurpriseRemovalOK = FALSE;
            break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
IntVideoPortDispatchPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = Irp->IoStatus.Status;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        case IRP_MN_STOP_DEVICE:
            /* Nothing to do */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            /* None (keep old status) */
            break;

        case IRP_MN_QUERY_ID:
            /* Call our helper */
            Status = IntVideoPortChildQueryId(DeviceObject->DeviceExtension,
                                              Irp,
                                              IrpSp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            /* Call our helper */
            Status = IntVideoPortChildQueryCapabilities(DeviceObject->DeviceExtension,
                                                        Irp,
                                                        IrpSp);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoDeleteDevice(DeviceObject);
            return STATUS_SUCCESS;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            /* Call our helper */
            Status = IntVideoPortChildQueryRelations(DeviceObject->DeviceExtension,
                                                     Irp,
                                                     IrpSp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            /* Call our helper */
            Status = IntVideoPortChildQueryText(DeviceObject->DeviceExtension,
                                                Irp,
                                                IrpSp);
            break;

        default:
            break;
    }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
