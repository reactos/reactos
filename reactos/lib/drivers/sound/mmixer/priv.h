#pragma once

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

#include <stdio.h>
#define YDEBUG
#include <debug.h>

typedef struct
{
    KSEVENTDATA EventData;
    LIST_ENTRY Entry;
}EVENT_ITEM, *LPEVENT_ITEM;

typedef struct
{
    LIST_ENTRY    Entry;
    MIXERCAPSW    MixCaps;
    HANDLE        hMixer;
    LIST_ENTRY    LineList;
    ULONG         ControlId;
    LIST_ENTRY    EventList;
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

typedef struct
{
    LIST_ENTRY Entry;
    ULONG DeviceId;
    HANDLE hDevice;
    HANDLE hDeviceInterfaceKey;
    LPWSTR DeviceName;
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
    ULONG MixerListCount;
    LIST_ENTRY MixerList;
    ULONG MixerDataCount;
    LIST_ENTRY MixerData;
    ULONG WaveInListCount;
    LIST_ENTRY WaveInList;
    ULONG WaveOutListCount;
    LIST_ENTRY WaveOutList;
}MIXER_LIST, *PMIXER_LIST;

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
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PULONG DeviceCount);

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
    LPMIXERCONTROLW *MixerControl,
    PULONG NodeId);

MIXER_STATUS
MMixerSetGetMuteControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    IN ULONG dwLineID,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    IN ULONG bSet);

MIXER_STATUS
MMixerSetGetVolumeControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    IN ULONG bSet,
    LPMIXERCONTROLW MixerControl,
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
MMixerGetDeviceName(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hKey);

MIXER_STATUS
MMixerInitializeWaveInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN LPWSTR DeviceName,
    IN ULONG bWaveIn,
    IN ULONG PinId);

MIXER_STATUS
MMixerAddEvents(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo);
