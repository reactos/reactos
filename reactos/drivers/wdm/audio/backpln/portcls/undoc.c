/*
    Undocumented PortCls exports
*/

#include "private.h"
#include <portcls.h>

NTSTATUS
KsoDispatchCreateWithGenericFactory(
    LONG Unknown,
    PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

void
KsoGetIrpTargetFromFileObject(
    LONG Unknown)
{
    UNIMPLEMENTED;
	return;
}

void
KsoGetIrpTargetFromIrp(
    LONG Unknown)
{
    UNIMPLEMENTED;
	return;
}

void
PcAcquireFormatResources(
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    LONG Unknown4)
{
    UNIMPLEMENTED;
	return;
}

NTSTATUS
PcAddToEventTable(
    PVOID Ptr,
    LONG Unknown2,
    ULONG Length,
    LONG Unknown3,
    LONG Unknown4,
    LONG Unknown5,
    LONG Unknown6,
    LONG Unknown7)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PcAddToPropertyTable(
    PVOID Ptr,
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    CHAR Unknown4)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PcCaptureFormat(
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    LONG Unknown4)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor(
    OUT SUBDEVICE_DESCRIPTOR ** OutSubdeviceDescriptor,
    IN ULONG InterfaceCount,
    IN GUID * InterfaceGuids,
    IN ULONG IdentifierCount,
    IN KSIDENTIFIER *Identifier,
    IN ULONG FilterPropertiesCount,
    IN KSPROPERTY_SET * FilterProperties,
    IN ULONG Unknown1,
    IN ULONG Unknown2,
    IN ULONG PinPropertiesCount,
    IN KSPROPERTY_SET * PinProperties,
    IN ULONG EventSetCount,
    IN KSEVENT_SET * EventSet,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    Descriptor = AllocateItem(NonPagedPool, sizeof(SUBDEVICE_DESCRIPTOR), TAG_PORTCLASS);
    if (!Descriptor)
        return STATUS_INSUFFICIENT_RESOURCES;

    Descriptor->Interfaces = AllocateItem(NonPagedPool, sizeof(GUID) * InterfaceCount, TAG_PORTCLASS);
    if (!Descriptor->Interfaces)
        goto cleanup;

    /* copy interface guids */
    RtlCopyMemory(Descriptor->Interfaces, InterfaceGuids, sizeof(GUID) * InterfaceCount);
    Descriptor->InterfaceCount = InterfaceCount;

    *OutSubdeviceDescriptor = Descriptor;
    return STATUS_SUCCESS;

cleanup:
    if (Descriptor)
    {
        if (Descriptor->Interfaces)
            FreeItem(Descriptor->Interfaces, TAG_PORTCLASS);

        FreeItem(Descriptor, TAG_PORTCLASS);
    }
    return Status;
}

/* PcDeleteSubdeviceDescriptor */

/* PcFreeEventTable */

/* PcFreePropertyTable */

/* PcGenerateEventDeferredRoutine */

/* PcGenerateEventList */

/* PcHandleDisableEventWithTable */

/* PcHandleEnableEventWithTable */

/* PcHandlePropertyWithTable */

/* PcPinPropertyHandler */

/* PcTerminateConnection */

/* PcValidateConnectRequest */

/* PcVerifyFilterIsReady */
