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

NTSTATUS
AddToPropertyTable(
    IN OUT SUBDEVICE_DESCRIPTOR * Descriptor,
    IN KSPROPERTY_SET * FilterProperty)
{
    if (Descriptor->FilterPropertySet.FreeKsPropertySetOffset >= Descriptor->FilterPropertySet.MaxKsPropertySetCount)
    {
        DPRINT1("FIXME\n");
        return STATUS_UNSUCCESSFUL;
    }

    RtlMoveMemory(&Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset],
                  FilterProperty,
                  sizeof(KSPROPERTY_SET));

    if (FilterProperty->PropertiesCount)
    {
        Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertyItem = AllocateItem(NonPagedPool,
                                                                                                                                    sizeof(KSPROPERTY_ITEM) * FilterProperty->PropertiesCount,
                                                                                                                                   TAG_PORTCLASS);

        if (!Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertyItem)
        {
            Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertiesCount = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory((PVOID)Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertyItem,
                      FilterProperty->PropertyItem,
                      sizeof(KSPROPERTY_ITEM) * FilterProperty->PropertiesCount);
    }

    Descriptor->FilterPropertySet.FreeKsPropertySetOffset++;

    return STATUS_SUCCESS;
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

    if (FilterPropertiesCount)
    {

       /// FIXME
       /// handle driver properties
       Descriptor->FilterPropertySet.Properties = AllocateItem(NonPagedPool, sizeof(KSPROPERTY_SET) * FilterPropertiesCount, TAG_PORTCLASS);
       if (! Descriptor->FilterPropertySet.Properties)
           goto cleanup;

       Descriptor->FilterPropertySet.MaxKsPropertySetCount = FilterPropertiesCount;
       for(Index = 0; Index < FilterPropertiesCount; Index++)
       {
           Status = AddToPropertyTable(Descriptor, &FilterProperties[Index]);
           if (!NT_SUCCESS(Status))
               goto cleanup;
       }
    }


    if (FilterDescription->PinCount)
    {
        Descriptor->Factory.KsPinDescriptor = AllocateItem(NonPagedPool, FilterDescription->PinSize * FilterDescription->PinCount, TAG_PORTCLASS);
        if (!Descriptor->Factory.KsPinDescriptor)
            goto cleanup;

        Descriptor->Factory.PinDescriptorCount = FilterDescription->PinCount;
        Descriptor->Factory.PinDescriptorSize = FilterDescription->PinSize;

        /* copy pin factories */
        for(Index = 0; Index < FilterDescription->PinCount; Index++)
            RtlMoveMemory(&Descriptor->Factory.KsPinDescriptor[Index], &FilterDescription->Pins[Index].KsPinDescriptor, FilterDescription->PinSize);
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
