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
    ULONG Index;
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


    if (FilterDescription->PinCount)
    {
        /// FIXME
        /// handle extra size
        ASSERT(FilterDescription->PinSize == sizeof(KSPIN_DESCRIPTOR));
        Descriptor->Factory.KsPinDescriptor = AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR) * FilterDescription->PinCount, TAG_PORTCLASS);
        if (!Descriptor->Factory.KsPinDescriptor)
            goto cleanup;

        Descriptor->Factory.PinDescriptorCount = FilterDescription->PinCount;
        Descriptor->Factory.PinDescriptorSize = FilterDescription->PinSize;

        /* copy pin factories */
        for(Index = 0; Index < FilterDescription->PinCount; Index++)
            RtlMoveMemory(&Descriptor->Factory.KsPinDescriptor[Index], &FilterDescription->Pins[Index].KsPinDescriptor, sizeof(KSPIN_DESCRIPTOR));
    }

    *OutSubdeviceDescriptor = Descriptor;
    return STATUS_SUCCESS;

cleanup:
    if (Descriptor)
    {
        if (Descriptor->Interfaces)
            FreeItem(Descriptor->Interfaces, TAG_PORTCLASS);

        if (Descriptor->Factory.KsPinDescriptor)
            FreeItem(Descriptor->Factory.KsPinDescriptor, TAG_PORTCLASS);

        FreeItem(Descriptor, TAG_PORTCLASS);
    }
    return Status;
}

/*
 * @implemented
 */

NTSTATUS
NTAPI
PcValidateConnectRequest(
    IN PIRP Irp,
    IN KSPIN_FACTORY * Factory,
    OUT PKSPIN_CONNECT * Connect)
{
    return KsValidateConnectRequest(Irp, Factory->PinDescriptorCount, Factory->KsPinDescriptor, Connect);
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
