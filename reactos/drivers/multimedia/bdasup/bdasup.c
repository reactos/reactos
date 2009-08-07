
#include <ntddk.h>
#include <windef.h>
#include <ks.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <bdasup.h>

#define NDEBUG
#include <debug.h>

STDMETHODIMP_(NTSTATUS) BdaCheckChanges(IN PIRP  Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaCommitChanges(IN PIRP  Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaCreateFilterFactory(
    IN PKSDEVICE  pKSDevice,
    IN const KSFILTER_DESCRIPTOR *pFilterDescriptor,
    IN const BDA_FILTER_TEMPLATE *pBdaFilterTemplate)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaCreateFilterFactoryEx(
    IN  PKSDEVICE pKSDevice,
    IN  const KSFILTER_DESCRIPTOR *pFilterDescriptor,
    IN  const BDA_FILTER_TEMPLATE *pBdaFilterTemplate,
    OUT PKSFILTERFACTORY  *ppKSFilterFactory)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaCreatePin(
    IN PKSFILTER pKSFilter,
    IN ULONG ulPinType,
    OUT ULONG *pulPinId)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaCreateTopology(
    IN PKSFILTER pKSFilter,
    IN ULONG InputPinId,
    IN ULONG OutputPinId)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaDeletePin(
    IN PKSFILTER pKSFilter,
    IN ULONG *pulPinId)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaFilterFactoryUpdateCacheData(
    IN PKSFILTERFACTORY pFilterFactory,
    IN const KSFILTER_DESCRIPTOR *pFilterDescriptor OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaGetChangeState(
    IN PIRP Irp,
    OUT BDA_CHANGE_STATE *pChangeState)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaInitFilter(
    IN PKSFILTER pKSFilter,
    IN const BDA_FILTER_TEMPLATE *pBdaFilterTemplate)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaMethodCreatePin(
    IN PIRP Irp,
    IN KSMETHOD *pKSMethod,
    OUT ULONG *pulPinFactoryID)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaMethodCreateTopology(
    IN PIRP Irp,
    IN KSMETHOD *pKSMethod,
    OPTIONAL PVOID pvIgnored)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaMethodDeletePin(
    IN PIRP Irp,
    IN KSMETHOD *pKSMethod,
    OPTIONAL PVOID pvIgnored)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyGetControllingPinId(
    IN PIRP Irp,
    IN KSP_BDA_NODE_PIN *pProperty,
    OUT ULONG *pulControllingPinId)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyGetPinControl(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyNodeDescriptors(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyNodeEvents(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyNodeMethods(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyNodeProperties(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyNodeTypes(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyPinTypes(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaPropertyTemplateConnections(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT KSTOPOLOGY_CONNECTION *pConnectionProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaStartChanges(IN PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaUninitFilter(IN PKSFILTER pKSFilter)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS) BdaValidateNodeProperty(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}
