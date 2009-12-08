#ifndef PRIV_H__
#define PRIV_H__

#include <pseh/pseh2.h>
#include <ntddk.h>

#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "mmixer.h"

#include <debug.h>

typedef struct
{
    MIXERCAPSW    MixCaps;
    HANDLE        hMixer;
    LIST_ENTRY    LineList;
    ULONG         ControlId;
}MIXER_INFO, *LPMIXER_INFO;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG PinId;
    HANDLE hDevice;
    MIXERLINEW Line;
    LPMIXERCONTROLW LineControls;
    PULONG          NodeIds;
    LIST_ENTRY LineControlsExtraData;
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


#define DESTINATION_LINE 0xFFFF0000

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
MMixerGetTargetPins(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN ULONG NodeIndex,
    IN ULONG bUpDirection,
    OUT PULONG Pins,
    IN ULONG PinCount);

MIXER_STATUS
MMixerGetPhysicalConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PinId,
    OUT PKSPIN_PHYSICALCONNECTION *OutConnection);

ULONG
MMixerGetIndexOfGuid(
    PKSMULTIPLE_ITEM MultipleItem,
    LPCGUID NodeType);

MIXER_STATUS
MMixerSetupFilter(
    IN PMIXER_CONTEXT MixerContext, 
    IN HANDLE hMixer,
    IN PULONG DeviceCount,
    IN LPWSTR DeviceName);

MIXER_STATUS
MMixerGetTargetPinsByNodeConnectionIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG bUpDirection,
    IN ULONG NodeConnectionIndex,
    OUT PULONG Pins);

MIXER_STATUS
MMixerGetControlsFromPin(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG PinId,
    IN ULONG bUpDirection,
    OUT PULONG Nodes);

#endif
