#pragma once

#if (NTDDI_VERSION >= NTDDI_WINXP)

#if defined(__cplusplus)
extern "C" {
#endif

#define STDMETHODCALLTYPE __stdcall

#ifndef _WDMDDK_
typedef GUID *PGUID;
#endif

/* Types */

typedef ULONG BDA_TOPOLOGY_JOINT, *PBDA_TOPOLOGY_JOINT;

typedef struct _BDA_PIN_PAIRING {
  ULONG ulInputPin;
  ULONG ulOutputPin;
  ULONG ulcMaxInputsPerOutput;
  ULONG ulcMinInputsPerOutput;
  ULONG ulcMaxOutputsPerInput;
  ULONG ulcMinOutputsPerInput;
  ULONG ulcTopologyJoints;
  const ULONG *pTopologyJoints;
} BDA_PIN_PAIRING, *PBDA_PIN_PAIRING;

typedef struct _BDA_FILTER_TEMPLATE {
  const KSFILTER_DESCRIPTOR *pFilterDescriptor;
  ULONG ulcPinPairs;
  const BDA_PIN_PAIRING *pPinPairs;
} BDA_FILTER_TEMPLATE, *PBDA_FILTER_TEMPLATE;

typedef struct _KSM_PIN_PAIR {
  KSMETHOD Method;
  ULONG InputPinId;
  ULONG OutputPinId;
  ULONG Reserved;
} KSM_PIN_PAIR, * PKSM_PIN_PAIR;

typedef struct _KSM_PIN {
  KSMETHOD Method;
  __GNU_EXTENSION union {
    ULONG PinId;
    ULONG PinType;
  };
  ULONG Reserved;
} KSM_PIN, * PKSM_PIN;

/* Functions */

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaCheckChanges(
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaCommitChanges(
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaCreateFilterFactory(
  _In_ PKSDEVICE pKSDevice,
  _In_ const KSFILTER_DESCRIPTOR *pFilterDescriptor,
  _In_ const BDA_FILTER_TEMPLATE *pBdaFilterTemplate);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaCreateFilterFactoryEx(
  _In_ PKSDEVICE pKSDevice,
  _In_ const KSFILTER_DESCRIPTOR *pFilterDescriptor,
  _In_ const BDA_FILTER_TEMPLATE *pBdaFilterTemplate,
  _Out_opt_ PKSFILTERFACTORY *ppKSFilterFactory);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaCreatePin(
  _In_ PKSFILTER pKSFilter,
  _In_ ULONG ulPinType,
  _Out_opt_ ULONG *pulPinId);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaCreateTopology(
  _In_ PKSFILTER pKSFilter,
  _In_ ULONG InputPinId,
  _In_ ULONG OutputPinId);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaDeletePin(
  _In_ PKSFILTER pKSFilter,
  _Out_opt_ ULONG *pulPinId);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaFilterFactoryUpdateCacheData(
  _In_ PKSFILTERFACTORY pFilterFactory,
  _In_opt_ const KSFILTER_DESCRIPTOR *pFilterDescriptor);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaGetChangeState(
  _In_ PIRP Irp,
  _Out_opt_ BDA_CHANGE_STATE *pChangeState);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaInitFilter(
  _In_ PKSFILTER pKSFilter,
  _In_ const BDA_FILTER_TEMPLATE *pBdaFilterTemplate);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaMethodCreatePin(
  _In_ PIRP Irp,
  _In_ KSMETHOD *pKSMethod,
  _Out_opt_ ULONG *pulPinFactoryID);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaMethodCreateTopology(
  _In_ PIRP Irp,
  _In_ KSMETHOD *pKSMethod,
  PVOID pvIgnored);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaMethodDeletePin(
  _In_ PIRP Irp,
  _In_ KSMETHOD *pKSMethod,
  PVOID pvIgnored);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyGetControllingPinId(
  _In_ PIRP Irp,
  _In_ KSP_BDA_NODE_PIN *pProperty,
  _Out_opt_ ULONG *pulControllingPinId);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyGetPinControl(
  _In_ PIRP Irp,
  _In_ KSPROPERTY *pKSProperty,
  _Out_opt_ ULONG *pulProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeDescriptors(
  _In_ PIRP Irp,
  _In_ KSPROPERTY *pKSProperty,
  _Out_opt_ BDANODE_DESCRIPTOR *pNodeDescriptorProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeEvents(
  _In_ PIRP Irp,
  _In_ KSP_NODE *pKSProperty,
  _Out_opt_ GUID *pguidProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeMethods(
  _In_ PIRP Irp,
  _In_ KSP_NODE *pKSProperty,
  _Out_opt_ GUID *pguidProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeProperties(
  _In_ PIRP Irp,
  _In_ KSP_NODE *pKSProperty,
  _Out_opt_ GUID *pguidProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeTypes(
  _In_ PIRP Irp,
  _In_ KSPROPERTY *pKSProperty,
  _Out_writes_bytes_(OutputBufferLenFromIrp(Irp)) ULONG *pulProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyPinTypes(
  _In_ PIRP Irp,
  _In_ KSPROPERTY *pKSProperty,
  _Out_writes_bytes_(OutputBufferLenFromIrp(Irp)) ULONG *pulProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaPropertyTemplateConnections(
  _In_ PIRP Irp,
  _In_ KSPROPERTY *pKSProperty,
  _Out_opt_ KSTOPOLOGY_CONNECTION *pConnectionProperty);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaStartChanges(
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaUninitFilter(
  _In_ PKSFILTER pKSFilter);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
STDMETHODIMP_(NTSTATUS)
BdaValidateNodeProperty(
  _In_ PIRP Irp,
  _In_ KSPROPERTY *pKSProperty);

#if defined(__cplusplus)
}
#endif

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */
