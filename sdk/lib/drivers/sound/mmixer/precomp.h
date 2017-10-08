#ifndef _MMIXER_PCH_
#define _MMIXER_PCH_

#include <wdm.h>
#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmsystem.h>

#include "mmixer.h"

typedef struct __TOPOLOGY_NODE__
{
    GUID NodeType;
    ULONG NodeIndex;

    ULONG NodeConnectedToCount;
    struct __TOPOLOGY_NODE__ ** NodeConnectedTo;

    ULONG NodeConnectedFromCount;
    struct __TOPOLOGY_NODE__ ** NodeConnectedFrom;
    PULONG LogicalPinNodeConnectedFrom;

    ULONG PinConnectedFromCount;
    PULONG PinConnectedFrom;

    ULONG PinConnectedToCount;
    PULONG PinConnectedTo;

    ULONG Visited;
    ULONG Reserved;
}TOPOLOGY_NODE, *PTOPOLOGY_NODE;

typedef struct
{
    ULONG PinId;

    ULONG NodesConnectedToCount;
    PTOPOLOGY_NODE * NodesConnectedTo;

    ULONG NodesConnectedFromCount;
    PTOPOLOGY_NODE * NodesConnectedFrom;

    ULONG PinConnectedFromCount;
    PULONG PinConnectedFrom;

    ULONG PinConnectedToCount;
    PULONG PinConnectedTo;

    ULONG Visited;
    ULONG Reserved;
}PIN, *PPIN;


typedef struct
{
    ULONG TopologyPinsCount;
    PPIN TopologyPins;

    ULONG TopologyNodesCount;
    PTOPOLOGY_NODE TopologyNodes;

}TOPOLOGY, *PTOPOLOGY;

typedef struct
{
    LIST_ENTRY    Entry;
    MIXERCAPSW    MixCaps;
    LIST_ENTRY    LineList;
    ULONG         ControlId;
    LIST_ENTRY    EventList;
}MIXER_INFO, *LPMIXER_INFO;

typedef struct
{
    LIST_ENTRY Entry;
    MIXERCONTROLW Control;
    ULONG NodeID;
    HANDLE hDevice;
    PVOID ExtraData;
}MIXERCONTROL_EXT, *LPMIXERCONTROL_EXT;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG PinId;
    MIXERLINEW Line;
    LIST_ENTRY ControlsList;

}MIXERLINE_EXT, *LPMIXERLINE_EXT;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG dwControlID;
}MIXERCONTROL_DATA, *LPMIXERCONTROL_DATA;

typedef struct
{
    MIXERCONTROL_DATA Header;
    LONG SignedMinimum;
    LONG SignedMaximum;
    LONG SteppingDelta;
    ULONG InputSteppingDelta;
    ULONG ValuesCount;
    PLONG Values;
}MIXERVOLUME_DATA, *LPMIXERVOLUME_DATA;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG DeviceId;
    HANDLE hDevice;
    HANDLE hDeviceInterfaceKey;
    LPWSTR DeviceName;
    PTOPOLOGY Topology;
    LPMIXER_INFO MixerInfo;
}MIXER_DATA, *LPMIXER_DATA;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG DeviceId;
    ULONG PinId;
    union
    {
        WAVEOUTCAPSW OutCaps;
        WAVEINCAPSW  InCaps;
    }u;
}WAVE_INFO, *LPWAVE_INFO;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG DeviceId;
    ULONG PinId;
    union
    {
        MIDIOUTCAPSW OutCaps;
        MIDIINCAPSW InCaps;
    }u;

}MIDI_INFO, *LPMIDI_INFO;

typedef struct
{
    ULONG MixerListCount;
    LIST_ENTRY MixerList;

    ULONG MixerDataCount;
    LIST_ENTRY MixerData;

    ULONG WaveInListCount;
    LIST_ENTRY WaveInList;

    ULONG WaveOutListCount;
    LIST_ENTRY WaveOutList;

    ULONG MidiInListCount;
    LIST_ENTRY MidiInList;

    ULONG MidiOutListCount;
    LIST_ENTRY MidiOutList;
}MIXER_LIST, *PMIXER_LIST;

typedef struct
{
    LIST_ENTRY Entry;
    PVOID MixerEventContext;
    PMIXER_EVENT MixerEventRoutine;

}EVENT_NOTIFICATION_ENTRY, *PEVENT_NOTIFICATION_ENTRY;

#define DESTINATION_LINE (0xFFFF0000)
#define SOURCE_LINE (0x10000)
ULONG
MMixerGetFilterPinCount(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer);

LPGUID
MMixerGetNodeType(
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG Index);

MIXER_STATUS
MMixerGetNodeIndexes(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG NodeIndex,
    IN ULONG bNode,
    IN ULONG bFrom,
    OUT PULONG NodeReferenceCount,
    OUT PULONG *NodeReference);

PKSTOPOLOGY_CONNECTION
MMixerGetConnectionByIndex(
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG Index);

ULONG
MMixerGetControlTypeFromTopologyNode(
    IN LPGUID NodeType);

LPMIXERLINE_EXT
MMixerGetSourceMixerLineByLineId(
    LPMIXER_INFO MixerInfo,
    DWORD dwLineID);

MIXER_STATUS
MMixerGetFilterTopologyProperty(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PropertyId,
    OUT PKSMULTIPLE_ITEM * OutMultipleItem);

VOID
MMixerFreeMixerInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo);

MIXER_STATUS
MMixerGetPhysicalConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PinId,
    OUT PKSPIN_PHYSICALCONNECTION *OutConnection);

MIXER_STATUS
MMixerSetupFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PULONG DeviceCount);

PKSPIN_CONNECT
MMixerAllocatePinConnect(
    IN PMIXER_CONTEXT MixerContext,
    ULONG DataFormatSize);

MIXER_STATUS
MMixerGetAudioPinDataRanges(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hDevice,
    IN ULONG PinId,
    IN OUT PKSMULTIPLE_ITEM * OutMultipleItem);

VOID
MMixerInitializeMidiForFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PTOPOLOGY Topology);

MIXER_STATUS
MMixerVerifyContext(
    IN PMIXER_CONTEXT MixerContext);

LPMIXER_INFO
MMixerGetMixerInfoByIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerIndex);

LPMIXERLINE_EXT
MMixerGetSourceMixerLineByComponentType(
    LPMIXER_INFO MixerInfo,
    DWORD dwComponentType);

MIXER_STATUS
MMixerGetMixerControlById(
    LPMIXER_INFO MixerInfo,
    DWORD dwControlID,
    LPMIXERLINE_EXT *MixerLine,
    LPMIXERCONTROL_EXT *MixerControl,
    PULONG NodeId);

MIXER_STATUS
MMixerSetGetMuteControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN LPMIXERCONTROL_EXT MixerControl,
    IN ULONG dwLineID,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    IN ULONG bSet);

MIXER_STATUS
MMixerSetGetVolumeControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG NodeId,
    IN ULONG bSet,
    LPMIXERCONTROL_EXT MixerControl,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    LPMIXERLINE_EXT MixerLine);

MIXER_STATUS
MMixerSetGetMuxControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG NodeId,
    IN ULONG bSet,
    IN ULONG Flags,
    LPMIXERCONTROL_EXT MixerControl,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    LPMIXERLINE_EXT MixerLine);


MIXER_STATUS
MMixerSetGetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    IN ULONG bSet,
    IN ULONG PropertyId,
    IN ULONG Channel,
    IN PLONG InputValue);

LPMIXER_DATA
MMixerGetDataByDeviceId(
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId);

LPMIXER_DATA
MMixerGetDataByDeviceName(
    IN PMIXER_LIST MixerList,
    IN LPWSTR DeviceName);

MIXER_STATUS
MMixerCreateMixerData(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId,
    IN LPWSTR DeviceName,
    IN HANDLE hDevice,
    IN HANDLE hKey);

MIXER_STATUS
MMixerInitializeWaveInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN LPWSTR DeviceName,
    IN ULONG bWaveIn,
    IN ULONG PinCount,
    IN PULONG Pins);

MIXER_STATUS
MMixerAddEvent(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN PVOID MixerEvent,
    IN PMIXER_EVENT MixerEventRoutine);

MIXER_STATUS
MMixerRemoveEvent(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN PVOID MixerEventContext,
    IN PMIXER_EVENT MixerEventRoutine);

MIXER_STATUS
MMixerGetDeviceName(
    IN PMIXER_CONTEXT MixerContext,
    OUT LPWSTR DeviceName,
    IN HANDLE hKey);

MIXER_STATUS
MMixerGetDeviceNameWithComponentId(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    OUT LPWSTR DeviceName);

VOID
MMixerInitializePinConnect(
    IN OUT PKSPIN_CONNECT PinConnect,
    IN ULONG PinId);

MIXER_STATUS
MMixerGetPinDataFlowAndCommunication(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hDevice,
    IN ULONG PinId,
    OUT PKSPIN_DATAFLOW DataFlow,
    OUT PKSPIN_COMMUNICATION Communication);

VOID
MMixerHandleAlternativeMixers(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PTOPOLOGY Topology);

MIXER_STATUS
MMixerGetMixerByName(
    IN PMIXER_LIST MixerList,
    IN LPWSTR MixerName,
    OUT LPMIXER_INFO *MixerInfo);

/* topology.c */

MIXER_STATUS
MMixerCreateTopology(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG PinCount,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    OUT PTOPOLOGY *OutTopology);

VOID
MMixerGetAllUpOrDownstreamPinsFromNodeIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bUpStream,
    OUT PULONG OutPinsCount,
    OUT PULONG OutPins);

MIXER_STATUS
MMixerGetAllUpOrDownstreamPinsFromPinIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinIndex,
    IN ULONG bUpStream,
    OUT PULONG OutPinsCount,
    OUT PULONG OutPins);

VOID
MMixerGetNextNodesFromPinIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes);

MIXER_STATUS
MMixerAllocateTopologyPinArray(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    OUT PULONG * OutPins);

MIXER_STATUS
MMixerAllocateTopologyNodeArray(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    OUT PULONG * OutPins);

VOID
MMixerGetAllUpOrDownstreamNodesFromPinIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes);

VOID
MMixerIsNodeTerminator(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    OUT ULONG * bTerminator);

VOID
MMixerGetNextNodesFromNodeIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes);

LPGUID
MMixerGetNodeTypeFromTopology(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex);

MIXER_STATUS
MMixerGetAllUpOrDownstreamNodesFromNodeIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes);

MIXER_STATUS
MMixerIsNodeConnectedToPin(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG PinId,
    IN ULONG bUpStream,
    OUT PULONG bConnected);

ULONG
MMixerGetNodeIndexFromGuid(
    IN PTOPOLOGY Topology,
    IN const GUID *NodeType);

VOID
MMixerSetTopologyNodeReserved(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex);

VOID
MMixerIsTopologyNodeReserved(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    OUT PULONG bReserved);

VOID
MMixerSetTopologyPinReserved(
    IN PTOPOLOGY Topology,
    IN ULONG PinId);

VOID
MMixerIsTopologyPinReserved(
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    OUT PULONG bReserved);

VOID
MMixerGetTopologyPinCount(
    IN PTOPOLOGY Topology,
    OUT PULONG PinCount);

VOID
MMixerGetConnectedFromLogicalTopologyPins(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    OUT PULONG OutPinCount,
    OUT PULONG OutPins);

VOID
MMixerPrintTopology();

#endif /* _MMIXER_PCH_ */
