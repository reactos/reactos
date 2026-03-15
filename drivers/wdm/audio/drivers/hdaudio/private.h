/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Internal definitions
 * COPYRIGHT:       Copyright 2025 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#pragma once

#include <ntddk.h>
#include <ks.h>
#include <portcls.h>
#include <ksmedia.h>
#include <hdaudio.h>
#include <ddk/ntstrsafe.h>

#include "hda_verbs.h"
#include "tables.h"

#define TAG_HDAUDIO 'UADH'

PVOID
__cdecl
operator new(size_t Size, POOL_TYPE PoolType, ULONG Tag);

typedef struct
{
    UCHAR AudioFormatSupported32Bit : 1;
    UCHAR AudioFormatSupported24Bit : 1;
    UCHAR AudioFormatSupported20Bit : 1;
    UCHAR AudioFormatSupported16Bit : 1;
    UCHAR AudioFormatSupported8Bit : 1;
    UCHAR Supported8Khz : 1;
    UCHAR Supported11Khz : 1;
    UCHAR Supported16Khz : 1;
    UCHAR Supported22Khz : 1;
    UCHAR Supported32Khz : 1;
    UCHAR Supported44Khz : 1;
    UCHAR Supported48Khz : 1;
    UCHAR Supported88Khz : 1;
    UCHAR Supported96Khz : 1;
    UCHAR Supported176Khz : 1;
    UCHAR Supported192Khz : 1;
    UCHAR Supported384Khz : 1;
    UCHAR AC3FormatSupported : 1;
    UCHAR Float32FormatSupported : 1;
    UCHAR PCMFormatSupported : 1;
}NODE_PCM_RATES, *PNODE_PCM_RATES;

typedef struct
{
    UCHAR PortConnectivity : 2;
    UCHAR Location : 6;
    UCHAR DefaultDevice : 4;
    UCHAR ConnectionType :4 ;
    UCHAR Color : 4;
    UCHAR Misc : 4;
    UCHAR DefaultAssociation : 4;
    UCHAR Sequence : 4;

}PIN_CONFIGURATION_DEFAULT, *PPIN_CONFIGURATION_DEFAULT;

typedef struct
{
    UCHAR MuteCapable : 1;
    UCHAR Steps : 7;
    UCHAR NumSteps : 7;
    UCHAR Offset : 7;
}AMPLIFIER_CAPABILITIES, *PAMPLIFIER_CAPABILITIES;


typedef struct
{
    LIST_ENTRY ListEntry;
    UCHAR Visited;
    UCHAR Digital;
    UCHAR NodeId;
    UCHAR PinDefaultAssociation;
    UCHAR ChannelCount;
    UCHAR NodeType;
    UCHAR ConnectionCount;
    USHORT Connections[1];
}NODE_CONTEXT, *PNODE_CONTEXT;

typedef struct
{
    UCHAR EAPDCapable : 1;
    UCHAR InputCapable : 1;
    UCHAR OutputCapable : 1;
    UCHAR PresenceDetectCapable : 1;
} PIN_CAPABILITIES, *PPIN_CAPABILITIES;

template <typename... Interfaces> class CUnknownImpl : public Interfaces...
{
  private:
    volatile LONG m_Ref;

  protected:
    CUnknownImpl() : m_Ref(0)
    {
    }
    virtual ~CUnknownImpl()
    {
    }

  public:
    STDMETHODIMP_(ULONG) AddRef()
    {
        ULONG Ref = InterlockedIncrement(&m_Ref);
        ASSERT(Ref < 0x10000);
        return Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG Ref = InterlockedDecrement(&m_Ref);
        ASSERT(Ref < 0x10000);
        if (!Ref)
        {
            delete this;
            return 0;
        }
        return Ref;
    }
};

class CAdapterCommon : public CUnknownImpl<IAdapterPowerManagement>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    NTSTATUS NTAPI Initialize(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PRESOURCELIST ResourceList);
    NTSTATUS NTAPI AcquireBusInterface(IN PDEVICE_OBJECT DeviceObject);
    VOID NTAPI GetResourceInformation();
    NTSTATUS NTAPI GetVendorRevisionId();
    NTSTATUS NTAPI TransferInitVerbs(IN PDEVICE_OBJECT DeviceObject);
    NTSTATUS NTAPI ReadRegistryKey(IN PDEVICE_OBJECT DeviceObject, IN LPWSTR KeyName, IN ULONG BufferLength, OUT PREGISTRYKEY * ParentKey, OUT PULONG Type, OUT PVOID Value);
    NTSTATUS NTAPI TransferVerb(ULONG Verb, PULONG Response);
    NTSTATUS NTAPI InstallDevice(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN LPWSTR Name,
        IN BOOLEAN bTopology,
        IN REFGUID PortClassId,
        IN PRESOURCELIST ResourceList,
        IN ULONG AssociatedPinsCount,
        IN PULONG AssociatedPinIds,
        IN PVOID FunctionGroupNode,
        IN PPCFILTER_DESCRIPTOR FilterDescription,
        OUT PUNKNOWN *OutPortUnknown);
    NTSTATUS NTAPI ChooseSubdeviceName(UCHAR DefaultDevice, UCHAR Wave, OUT LPWSTR *OutSubdeviceName);
    NTSTATUS NTAPI
    BuildWaveOutFormat(
        IN ULONG AssociatedPinCount,
        IN PULONG AssociatedPins,
        IN PVOID Node,
        IN UCHAR Digital);
    NTSTATUS NTAPI
    BuildWaveInFormat(
        IN ULONG AssociatedPinCount,
        IN PULONG AssociatedPins,
        IN PVOID Node);
    NTSTATUS NTAPI
    BuildWaveFormat(
        IN PVOID Node,
        IN ULONG NodeCount,
        IN PULONG TargetWidgets);
    NTSTATUS NTAPI
    BuildInstallFilter(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN ULONG bOutput,
        IN PVOID OutNode,
        IN PRESOURCELIST ResourceList,
        IN ULONG AssociatedPinCount,
        IN PULONG AssociatedPins,
        IN UCHAR Digital);
    VOID NTAPI ClearRef(IN ULONG RefValue, IN ULONG NodeCount, IN PULONG Nodes);
    NTSTATUS
    NTAPI
    ProcessOutputNodes(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PRESOURCELIST ResourceList,
        IN ULONG PinNodeCount,
        IN PULONG PinNodes,
        IN PVOID Node);
    NTSTATUS
    NTAPI
    ProcessInputNodes(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PRESOURCELIST ResourceList,
        IN ULONG PinNodeCount,
        IN PULONG PinNodes,
        IN PVOID Node,
        ULONG InputNodeCount,
        PULONG InputNodes);

    NTSTATUS
    NTAPI
    AssociatePins(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PRESOURCELIST ResourceList,
        IN UCHAR DefaultAssociation,
        IN PVOID Node,
        IN ULONG PinNodeCount,
        IN PULONG PinNodes,
        IN UCHAR Digital);

    NTSTATUS
    NTAPI
    IsInputNodeConnectedToPin(
        IN PVOID Node,
        IN UCHAR Digital,
        ULONG InputNodeId,
        ULONG PinNodeId,
        ULONG PinNodeCount,
        OUT PULONG Result);

    VOID
    NTAPI
    CollectPinWithType(
        IN ULONG DeviceType,
        IN ULONG PinNodeCount,
        IN PULONG PinIds,
        IN PULONG DefaultDeviceTypeList,
        OUT PULONG ResultListCount,
        OUT PULONG ResultList);


    NTSTATUS
    NTAPI
    FindConnectedWidgets(
        IN ULONG NodeType,
        IN ULONG PinNodeId,
        IN ULONG NodeId,
        IN PVOID Node,
        IN ULONG NodeIndex,
        IN ULONG NodeCount,
        OUT PULONG OutNodeCount,
        OUT PULONG OutNodes,
        IN UCHAR Digital);

    NTSTATUS NTAPI
    GetInterface(PHDAUDIO_BUS_INTERFACE_V2 Interface);

    UCHAR
    GetCodecAddress()
    {
        return m_CodecAddress;
    }

    IMP_IAdapterPowerManagement;
    CAdapterCommon(IUnknown* OuterUnknown)
    {
    }
    virtual ~CAdapterCommon()
    {
    }

    friend NTSTATUS NTAPI PropertyHandler_JackDescription(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_ChannelConfig(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_SpeakerGeometry(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_Volume(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_Mute(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI EventHandler_Volume(IN PPCEVENT_REQUEST EventRequest);

  private:
    HDAUDIO_BUS_INTERFACE_V2 Interface;
    UCHAR m_CodecAddress;
    UCHAR m_FunctionGroupStartNode;
    UINT16 RevisionId;
    UINT16 VendorId;
    UCHAR m_Tags[64];
    PUNKNOWN m_TopoInPortUnknown;
    PUNKNOWN m_WaveRTInPortUnknown;
    PUNKNOWN m_TopoOutPortUnknown;
    PUNKNOWN m_WaveRTOutPortUnknown;
};

class CFunctionGroupNode
{
  public:
    CFunctionGroupNode(UCHAR CodecAddress, UCHAR NodeId, CAdapterCommon *AdapterCommon)
        : m_Adapter(AdapterCommon), m_StartNodeId(NodeId), m_CodecAddress(CodecAddress), m_SubNodeCount(0)
    {
        InitializeListHead(&m_Nodes);
    }
    virtual ~CFunctionGroupNode()
    {
        while (!IsListEmpty(&m_Nodes))
        {
            PLIST_ENTRY Entry = RemoveHeadList(&m_Nodes);
            PNODE_CONTEXT NodeContext = CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);
            ExFreePoolWithTag(NodeContext, TAG_HDAUDIO);
        }
    }

    NTSTATUS NTAPI QueryWidgetCount();
    NTSTATUS NTAPI EnumWidgets();
    NTSTATUS NTAPI EnumConnections();
    VOID NTAPI PrintNodeInfo(UCHAR NodeType, ULONG NodeId);
    NTSTATUS NTAPI GetNodesWithType(IN UCHAR NodeType, OUT PULONG NodeCount, OUT PULONG *NodesAddress);
    NTSTATUS NTAPI GetSupportedPCMSizeRates(ULONG NodeId, OUT PNODE_PCM_RATES OutRates);
    NTSTATUS NTAPI GetPinConfigurationDefault(IN ULONG NodeId, IN PPIN_CONFIGURATION_DEFAULT PinConfiguration);
    NTSTATUS NTAPI AddNode(ULONG NodeId);
    PNODE_CONTEXT NTAPI FindNodeId(ULONG NodeId);
    NTSTATUS NTAPI GetAmplifierDetails(IN ULONG NodeId, IN ULONG Input, OUT PAMPLIFIER_CAPABILITIES Caps);
    NTSTATUS NTAPI GetPinCapabilities(IN ULONG NodeId, OUT PPIN_CAPABILITIES PinCaps);
    NTSTATUS NTAPI GetPinSense(IN ULONG NodeId, IN PULONG DevicePresent);
    NTSTATUS NTAPI GetVolume(IN ULONG NodeId, OUT PUCHAR Direct, OUT PLONG Volume);
    NTSTATUS NTAPI SetVolume(IN ULONG NodeId, IN UCHAR Direct, IN LONG Volume);
    NTSTATUS NTAPI GetVolumeCapabilities(IN ULONG NodeId, IN PUCHAR Delta, IN PUCHAR NumSteps);
    NTSTATUS NTAPI GetPinNodesWithDefaultAssociation(IN UCHAR DefaultAssociation, IN UCHAR Digital, OUT PULONG NodeCount, OUT PULONG * Nodes);
    VOID NTAPI ClearVisitedState();

    NTSTATUS
    NTAPI
    SetStreamFormat(IN ULONG NodeId, IN USHORT Format);

    NTSTATUS
    NTAPI
    SetConverterStream(IN ULONG NodeId, IN UCHAR StreamId);

    NTSTATUS
    NTAPI
    SetEAPD(IN ULONG NodeId, IN UCHAR Enable);

    UCHAR
    GetStartNodeId()
    {
        return m_StartNodeId;
    }

  private:
    CAdapterCommon *m_Adapter;
    UCHAR m_StartNodeId;
    UCHAR m_CodecAddress;
    ULONG m_StartSubNode;
    ULONG m_SubNodeCount;
    LIST_ENTRY m_Nodes;
};

class CMiniportTopology : public CUnknownImpl<IMiniportTopology>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    IMP_IMiniportTopology;
    CMiniportTopology(
        IUnknown *OuterUnknown,
        ULONG AssociatedPinCount,
        PULONG AssociatedPinIds,
        CFunctionGroupNode *Node,
        CAdapterCommon *Adapter,
        PPCFILTER_DESCRIPTOR Filter)
        : m_Port(0), m_AssociatedPinCount(AssociatedPinCount), m_AssociatedPins(AssociatedPinIds), m_Node(Node),
          m_Adapter(Adapter), m_FilterDescription(Filter)
    {
    }
    virtual ~CMiniportTopology()
    {
    }

    CFunctionGroupNode*
    GetNode()
    {
        return m_Node;
    }

  private:
    PPORTTOPOLOGY m_Port;
    ULONG m_AssociatedPinCount;
    PULONG m_AssociatedPins;
    CFunctionGroupNode *m_Node;
    CAdapterCommon *m_Adapter;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
    PIN_CONFIGURATION_DEFAULT m_PinConfiguration;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
};

class CMiniportWaveRT : public CUnknownImpl<IMiniportWaveRT>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    IMP_IMiniportWaveRT;
    CMiniportWaveRT(
        IUnknown *OuterUnknown,
        ULONG AssociatedPinCount,
        PULONG AssociatedPinIds,
        CFunctionGroupNode *Node,
        CAdapterCommon *Adapter,
        PPCFILTER_DESCRIPTOR Filter)
        : m_Port(0), m_AssociatedPinCount(AssociatedPinCount), m_AssociatedPins(AssociatedPinIds), m_Node(Node),
          m_Adapter(Adapter), m_FilterDescription(Filter)
    {
    }
    virtual ~CMiniportWaveRT()
    {
    }

    CFunctionGroupNode*
    GetNode()
    {
        return m_Node;
    }

  private:
    PPORTWAVERT m_Port;
    ULONG m_AssociatedPinCount;
    PULONG m_AssociatedPins;
    CFunctionGroupNode *m_Node;
    CAdapterCommon *m_Adapter;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
    PIN_CONFIGURATION_DEFAULT m_PinConfiguration;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
};

class CMiniportWaveRTStream : public CUnknownImpl<IMiniportWaveRTStreamNotification>
{
  public:
    STDMETHODIMP
    QueryInterface(REFIID InterfaceId, PVOID *Interface);

    CMiniportWaveRTStream(IUnknown *OuterUnknown, CAdapterCommon * Adapter, CFunctionGroupNode * OutNode,
        ULONG Pin,
        BOOLEAN Capture,
        PHDAUDIO_STREAM_FORMAT StreamFormat,
        PHDAUDIO_CONVERTER_FORMAT Converter,
        HANDLE DmaEngine,
        ULONG NodeCount,
        PULONG Nodes,
        PPCFILTER_DESCRIPTOR FilterDescription)
        : m_Adapter(Adapter), m_OutNode(OutNode), m_Pin(Pin), m_Capture(Capture), m_DmaEngine(DmaEngine),
          m_NodeCount(NodeCount), m_Nodes(Nodes), m_FilterDescription(FilterDescription)
    {
       NTSTATUS Status = m_Adapter->GetInterface(&m_Interface);
       ASSERT(NT_SUCCESS(Status));
       RtlCopyMemory(&m_StreamFormat, StreamFormat, sizeof(HDAUDIO_STREAM_FORMAT));
       RtlCopyMemory(&m_Converter, Converter, sizeof(HDAUDIO_CONVERTER_FORMAT));

    }
    virtual ~CMiniportWaveRTStream()
    {
        NTSTATUS Status;
        HDAUDIO_BUS_INTERFACE_V2 Interface;

        Status = m_Adapter->GetInterface(&Interface);
        if (NT_SUCCESS(Status))
        {
            Interface.FreeDmaEngine(Interface.Context, m_DmaEngine);
            return;
        }
    }

    IMP_IMiniportWaveRTStreamNotification;

  private:
    CAdapterCommon *m_Adapter;
    CFunctionGroupNode *m_OutNode;
    ULONG m_Pin;
    BOOLEAN m_Capture;
    HDAUDIO_STREAM_FORMAT m_StreamFormat;
    HDAUDIO_CONVERTER_FORMAT m_Converter;
    HANDLE m_DmaEngine;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
    UCHAR m_StreamId;
    ULONG m_FifoSize;
    ULONG m_NodeCount;
    PULONG m_Nodes;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
};

NTSTATUS
HDAUDIO_AllocateCommonAdapter(
    OUT CAdapterCommon ** OutAdapter);

NTSTATUS
HDAUDIO_InitializeCommonAdapter(
    CAdapterCommon* CommonAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PRESOURCELIST ResourceList
);

typedef struct
{
    ULONG_PTR Reserved[4]; // used by portcls
    CAdapterCommon *AdapterCommon;
}HDAUDIO_DEVICE_EXTENSION, *PHDAUDIO_DEVICE_EXTENSION;

NTSTATUS
HDAUDIO_NewMiniportTopology(
    OUT PMINIPORTTOPOLOGY* OutMiniport,
    IN ULONG AssociatedPinsCount,
    IN PULONG AssociatedPinIds,
    IN CFunctionGroupNode * Node,
    IN CAdapterCommon * Adapter,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

NTSTATUS
HDAUDIO_NewMiniportWaveRT(
    OUT PMINIPORTWAVERT *OutMiniport,
    IN ULONG AssociatedPinsCount,
    IN PULONG AssociatedPinIds,
    IN CFunctionGroupNode * Node,
    IN CAdapterCommon * Adapter,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

NTSTATUS
HDAUDIO_EnumFunctionGroupWidgets(
    IN UCHAR CodecAddress,
    IN UCHAR FunctionGroupStartNodeId,
    IN CAdapterCommon *Adapter,
    OUT CFunctionGroupNode **OutFunctionGroupNode);

NTSTATUS
HDAUDIO_AllocateStream(
    OUT PMINIPORTWAVERTSTREAM *Stream,
    IN CAdapterCommon *Adapter,
    IN CFunctionGroupNode *Node,
    IN ULONG Pin,
    IN BOOLEAN Capture,
    IN PKSDATAFORMAT DataFormat,
    IN ULONG NodeCount,
    IN PULONG Nodes,
    IN PPCFILTER_DESCRIPTOR FilterDescription);
