#include "virtiosnd.h"

#include <ksmedia.h>
#include <stdunk.h>

PVOID
__cdecl
operator new(size_t Size, POOL_TYPE PoolType, ULONG Tag);

extern "C" const GUID CLSID_PortTopology =
    {0xb4c90a32, 0x5791, 0x11d0, {0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44}};

extern "C" const GUID IID_IMiniportTopology =
    {0xb4c90a31, 0x5791, 0x11d0, {0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44}};

#define VIRTIO_SND_TOPOLOGY_POOL_TAG 'tSnV'
#define VIRTIO_SND_VOLUME_ACCESS (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT)
#define VIRTIO_SND_MUTE_ACCESS (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT)
#define VIRTIO_SND_VOLUME_MINIMUM (-9600)
#define VIRTIO_SND_VOLUME_MAXIMUM 0
#define VIRTIO_SND_VOLUME_STEP 100

static NTSTATUS NTAPI
VirtioSndVolumeProperty(_In_ PPCPROPERTY_REQUEST PropertyRequest);

static NTSTATUS NTAPI
VirtioSndMuteProperty(_In_ PPCPROPERTY_REQUEST PropertyRequest);

static PCPROPERTY_ITEM g_VirtioSndVolumeProperties[] = {
    {&KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL, VIRTIO_SND_VOLUME_ACCESS, VirtioSndVolumeProperty}};

static PCPROPERTY_ITEM g_VirtioSndMuteProperties[] = {
    {&KSPROPSETID_Audio, KSPROPERTY_AUDIO_MUTE, VIRTIO_SND_MUTE_ACCESS, VirtioSndMuteProperty}};

DEFINE_PCAUTOMATION_TABLE_PROP(g_VirtioSndVolumeAutomation, g_VirtioSndVolumeProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(g_VirtioSndMuteAutomation, g_VirtioSndMuteProperties);

static KSDATARANGE g_VirtioSndAnalogRange = {
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)};

static PKSDATARANGE g_VirtioSndAnalogRanges[] = {&g_VirtioSndAnalogRange};

static PCPIN_DESCRIPTOR g_VirtioSndTopologyPins[] = {
    {0,
     0,
     0,
     NULL,
     {0, NULL, 0, NULL, SIZEOF_ARRAY(g_VirtioSndAnalogRanges), g_VirtioSndAnalogRanges, KSPIN_DATAFLOW_IN,
      KSPIN_COMMUNICATION_NONE, (GUID *)&KSCATEGORY_AUDIO, NULL, 0}},
    {0,
     0,
     0,
     NULL,
     {0, NULL, 0, NULL, SIZEOF_ARRAY(g_VirtioSndAnalogRanges), g_VirtioSndAnalogRanges, KSPIN_DATAFLOW_OUT,
      KSPIN_COMMUNICATION_NONE, (GUID *)&KSNODETYPE_SPEAKER, (GUID *)&KSAUDFNAME_VOLUME_CONTROL, 0}}};

static PCNODE_DESCRIPTOR g_VirtioSndTopologyNodes[] = {
    {0, &g_VirtioSndVolumeAutomation, &KSNODETYPE_VOLUME, &KSAUDFNAME_MASTER_VOLUME},
    {0, &g_VirtioSndMuteAutomation, &KSNODETYPE_MUTE, &KSAUDFNAME_MASTER_MUTE}};

static PCCONNECTION_DESCRIPTOR g_VirtioSndTopologyConnections[] = {
    {PCFILTER_NODE, 0, 0, 1},
    {0, 0, 1, 1},
    {1, 0, PCFILTER_NODE, 1}};

static PCFILTER_DESCRIPTOR g_VirtioSndTopologyFilter = {
    0,
    NULL,
    sizeof(PCPIN_DESCRIPTOR),
    SIZEOF_ARRAY(g_VirtioSndTopologyPins),
    g_VirtioSndTopologyPins,
    sizeof(PCNODE_DESCRIPTOR),
    SIZEOF_ARRAY(g_VirtioSndTopologyNodes),
    g_VirtioSndTopologyNodes,
    SIZEOF_ARRAY(g_VirtioSndTopologyConnections),
    g_VirtioSndTopologyConnections,
    0,
    NULL};

static NTSTATUS
VirtioSndBasicSupport(_In_ PPCPROPERTY_REQUEST PropertyRequest, _In_ ULONG AccessFlags, _In_ BOOLEAN HasSteppedRange)
{
    if (PropertyRequest->Value == NULL)
        return STATUS_BUFFER_TOO_SMALL;

    if (!HasSteppedRange)
    {
        if (PropertyRequest->ValueSize < sizeof(ULONG))
            return STATUS_BUFFER_TOO_SMALL;

        *(PULONG)PropertyRequest->Value = AccessFlags;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }

    if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
    {
        PKSPROPERTY_DESCRIPTION Description = (PKSPROPERTY_DESCRIPTION)PropertyRequest->Value;
        ULONG RequiredSize = sizeof(*Description) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG);

        RtlZeroMemory(Description, sizeof(*Description));
        Description->AccessFlags = AccessFlags;
        Description->DescriptionSize = RequiredSize;
        Description->PropTypeSet.Set = KSPROPTYPESETID_General;
        Description->PropTypeSet.Id = VT_I4;
        Description->MembersListCount = 1;

        if (PropertyRequest->ValueSize >= RequiredSize)
        {
            PKSPROPERTY_MEMBERSHEADER Members = (PKSPROPERTY_MEMBERSHEADER)(Description + 1);
            PKSPROPERTY_STEPPING_LONG Range = (PKSPROPERTY_STEPPING_LONG)(Members + 1);

            Members->MembersCount = 1;
            Members->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize = sizeof(*Range);
            Range->SteppingDelta = VIRTIO_SND_VOLUME_STEP;
            Range->Bounds.SignedMinimum = VIRTIO_SND_VOLUME_MINIMUM;
            Range->Bounds.SignedMaximum = VIRTIO_SND_VOLUME_MAXIMUM;
            PropertyRequest->ValueSize = RequiredSize;
        }
        else
        {
            PropertyRequest->ValueSize = sizeof(*Description);
        }

        return STATUS_SUCCESS;
    }

    if (PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        *(PULONG)PropertyRequest->Value = AccessFlags;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }

    return STATUS_BUFFER_TOO_SMALL;
}

static BOOLEAN
VirtioSndValidChannelInstance(_In_ PPCPROPERTY_REQUEST PropertyRequest)
{
    LONG Channel;

    if (PropertyRequest->Instance == NULL || PropertyRequest->InstanceSize < sizeof(Channel))
    {
        return FALSE;
    }

    Channel = *(PLONG)PropertyRequest->Instance;
    return Channel >= -1 && Channel <= 1;
}

static NTSTATUS NTAPI
VirtioSndVolumeProperty(_In_ PPCPROPERTY_REQUEST PropertyRequest)
{
    PAGED_CODE();

    if (PropertyRequest == NULL)
        return STATUS_INVALID_PARAMETER;

    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) != 0)
    {
        return VirtioSndBasicSupport(PropertyRequest, VIRTIO_SND_VOLUME_ACCESS, TRUE);
    }

    if (!VirtioSndValidChannelInstance(PropertyRequest) || PropertyRequest->Value == NULL ||
        PropertyRequest->ValueSize < sizeof(LONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) != 0)
    {
        *(PLONG)PropertyRequest->Value = VirtioSndGetMasterVolume();
        PropertyRequest->ValueSize = sizeof(LONG);
        return STATUS_SUCCESS;
    }

    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) != 0)
    {
        return VirtioSndSetMasterVolume(*(PLONG)PropertyRequest->Value);
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS NTAPI
VirtioSndMuteProperty(_In_ PPCPROPERTY_REQUEST PropertyRequest)
{
    PAGED_CODE();

    if (PropertyRequest == NULL)
        return STATUS_INVALID_PARAMETER;

    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) != 0)
    {
        return VirtioSndBasicSupport(PropertyRequest, VIRTIO_SND_MUTE_ACCESS, FALSE);
    }

    if (!VirtioSndValidChannelInstance(PropertyRequest) || PropertyRequest->Value == NULL ||
        PropertyRequest->ValueSize < sizeof(ULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) != 0)
    {
        *(PULONG)PropertyRequest->Value = VirtioSndGetMasterMute() ? TRUE : FALSE;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }

    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) != 0)
    {
        ULONG Muted = *(PULONG)PropertyRequest->Value;
        if (Muted > TRUE)
            return STATUS_INVALID_PARAMETER;

        VirtioSndSetMasterMute(Muted != FALSE);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

class VirtioSndTopologyMiniport : public IMiniportTopology, public CUnknown
{
  public:
    VirtioSndTopologyMiniport(PUNKNOWN OuterUnknown) : CUnknown(OuterUnknown)
    {
    }

    DECLARE_STD_UNKNOWN();
    IMP_IMiniportTopology;
};

STDMETHODIMP_(NTSTATUS)
VirtioSndTopologyMiniport::NonDelegatingQueryInterface(_In_ REFIID Interface, _Out_ PVOID *Object)
{
    if (Object == NULL)
        return STATUS_INVALID_PARAMETER;

    *Object = NULL;
    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = static_cast<IUnknown *>(static_cast<IMiniportTopology *>(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportTopology))
    {
        *Object = static_cast<IMiniportTopology *>(this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = static_cast<IMiniport *>(this);
    }

    if (*Object == NULL)
        return STATUS_INVALID_PARAMETER;

    NonDelegatingAddRef();
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndTopologyMiniport::GetDescription(_Out_ PPCFILTER_DESCRIPTOR *Description)
{
    if (Description == NULL)
        return STATUS_INVALID_PARAMETER;

    *Description = &g_VirtioSndTopologyFilter;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndTopologyMiniport::DataRangeIntersection(
    _In_ ULONG PinId,
    _In_ PKSDATARANGE DataRange,
    _In_ PKSDATARANGE MatchingDataRange,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultantFormatLength) PVOID ResultantFormat,
    _Out_ PULONG ResultantFormatLength)
{
    UNREFERENCED_PARAMETER(PinId);
    UNREFERENCED_PARAMETER(DataRange);
    UNREFERENCED_PARAMETER(MatchingDataRange);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ResultantFormat);
    UNREFERENCED_PARAMETER(ResultantFormatLength);
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndTopologyMiniport::Init(
    _In_opt_ PUNKNOWN UnknownAdapter,
    _In_opt_ PRESOURCELIST ResourceList,
    _In_ PPORTTOPOLOGY Port)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(UnknownAdapter);
    UNREFERENCED_PARAMETER(ResourceList);
    return Port != NULL ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

static NTSTATUS
VirtioSndCreateTopologyMiniport(
    _Out_ PUNKNOWN *Unknown,
    _In_ REFCLSID ClassId,
    _In_opt_ PUNKNOWN UnknownOuter,
    _In_ POOL_TYPE PoolType)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(ClassId);

    STD_CREATE_BODY_WITH_TAG_(
        VirtioSndTopologyMiniport, Unknown, UnknownOuter, PoolType, VIRTIO_SND_TOPOLOGY_POOL_TAG, PMINIPORTTOPOLOGY);
}

NTSTATUS
VirtioSndInstallTopologySubdevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PRESOURCELIST ResourceList,
    _Out_ PUNKNOWN *TopologyUnknown)
{
    PPORT Port = NULL;
    PUNKNOWN Miniport = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    if (DeviceObject == NULL || Irp == NULL || ResourceList == NULL || TopologyUnknown == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *TopologyUnknown = NULL;
    static WCHAR TopologySubdeviceName[] = L"Topology";
    Status = PcNewPort(&Port, CLSID_PortTopology);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = VirtioSndCreateTopologyMiniport(&Miniport, CLSID_PortTopology, NULL, NonPagedPool);
    if (NT_SUCCESS(Status))
    {
        Status = Port->Init(DeviceObject, Irp, Miniport, NULL, ResourceList);
    }

    if (NT_SUCCESS(Status))
    {
        Status = PcRegisterSubdevice(DeviceObject, TopologySubdeviceName, Port);
    }

    if (NT_SUCCESS(Status))
    {
        *TopologyUnknown = (PUNKNOWN)Port;
        (*TopologyUnknown)->AddRef();
    }

    if (Miniport != NULL)
        Miniport->Release();
    Port->Release();

    if (!NT_SUCCESS(Status))
        DbgPrint("virtiosnd: Topology subdevice registration failed (%08lx)\n", (ULONG)Status);

    return Status;
}
