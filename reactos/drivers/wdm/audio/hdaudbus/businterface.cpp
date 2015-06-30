/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/hdaudbus.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/
#include "hdaudbus.h"

NTSTATUS
HDA_PDOHandleQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    UNICODE_STRING GuidString;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    RtlStringFromGUID(*IoStack->Parameters.QueryInterface.InterfaceType, &GuidString);

    DPRINT1("hda: requesting interface %wZ Version %u Size %u", &GuidString, IoStack->Parameters.QueryInterface.Version, IoStack->Parameters.QueryInterface.Size);
    return STATUS_NOT_IMPLEMENTED;
}
