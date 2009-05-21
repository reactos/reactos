/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndissys.h
 * PURPOSE:     NDIS library definitions
 * NOTES:       Spin lock acquire order:
 *                - Miniport list lock
 *                - Adapter list lock
 */
#ifndef __NDISSYS_H
#define __NDISSYS_H

typedef unsigned long NDIS_STATS;

#include <ntifs.h>
#include <ndis.h>
#include <xfilter.h>
#include <afilter.h>
#include <atm.h>

struct _ADAPTER_BINDING;

typedef struct _NDISI_PACKET_POOL {
  NDIS_SPIN_LOCK  SpinLock;
  struct _NDIS_PACKET *FreeList;
  UINT  PacketLength;
  UCHAR  Buffer[1];
} NDISI_PACKET_POOL, * PNDISI_PACKET_POOL;

/* WDK Compatibility. Taken from w32api DDK */
#ifndef __NDIS_H
typedef struct _X_FILTER FDDI_FILTER, *PFDDI_FILTER;

typedef VOID
(NTAPI *FDDI_RCV_COMPLETE_HANDLER)(
  IN PFDDI_FILTER  Filter);

typedef VOID
(NTAPI *FDDI_RCV_INDICATE_HANDLER)(
  IN PFDDI_FILTER  Filter,
  IN NDIS_HANDLE  MacReceiveContext,
  IN PCHAR  Address,
  IN UINT  AddressLength,
  IN PVOID  HeaderBuffer,
  IN UINT  HeaderBufferSize,
  IN PVOID  LookaheadBuffer,
  IN UINT  LookaheadBufferSize,
  IN UINT  PacketSize);

typedef enum _NDIS_WORK_ITEM_TYPE {
  NdisWorkItemRequest,
  NdisWorkItemSend,
  NdisWorkItemReturnPackets,
  NdisWorkItemResetRequested,
  NdisWorkItemResetInProgress,
  NdisWorkItemHalt,
  NdisWorkItemSendLoopback,
  NdisWorkItemMiniportCallback,
  NdisMaxWorkItems
} NDIS_WORK_ITEM_TYPE, *PNDIS_WORK_ITEM_TYPE;

#define NUMBER_OF_WORK_ITEM_TYPES         NdisMaxWorkItems
#define NUMBER_OF_SINGLE_WORK_ITEMS       6

typedef struct _NDIS_MINIPORT_WORK_ITEM {
    SINGLE_LIST_ENTRY  Link;
    NDIS_WORK_ITEM_TYPE  WorkItemType;
    PVOID  WorkItemContext;
} NDIS_MINIPORT_WORK_ITEM, *PNDIS_MINIPORT_WORK_ITEM;

typedef VOID (NTAPI *W_MINIPORT_CALLBACK)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PVOID  CallbackContext);

typedef struct _NDIS_BIND_PATHS {
	UINT  Number;
	NDIS_STRING  Paths[1];
} NDIS_BIND_PATHS, *PNDIS_BIND_PATHS;

#if ARCNET
#define FILTERDBS_ARCNET_S \
  PARC_FILTER  ArcDB;
#else /* !ARCNET */
#define FILTERDBS_ARCNET_S \
  PVOID  XXXDB;
#endif /* !ARCNET */

#define FILTERDBS_S \
  union { \
    PETH_FILTER  EthDB; \
    PNULL_FILTER  NullDB; \
  }; \
  PTR_FILTER  TrDB; \
  PFDDI_FILTER  FddiDB; \
  FILTERDBS_ARCNET_S

typedef struct _NDIS_LOG {
  PNDIS_MINIPORT_BLOCK  Miniport;
  KSPIN_LOCK  LogLock;
  PIRP  Irp;
  UINT  TotalSize;
  UINT  CurrentSize;
  UINT  InPtr;
  UINT  OutPtr;
  UCHAR  LogBuf[1];
} NDIS_LOG, *PNDIS_LOG;

typedef enum _NDIS_PNP_DEVICE_STATE {
  NdisPnPDeviceAdded,
  NdisPnPDeviceStarted,
  NdisPnPDeviceQueryStopped,
  NdisPnPDeviceStopped,
  NdisPnPDeviceQueryRemoved,
  NdisPnPDeviceRemoved,
  NdisPnPDeviceSurpriseRemoved
} NDIS_PNP_DEVICE_STATE;

typedef struct _OID_LIST    OID_LIST, *POID_LIST;

struct _NDIS_MINIPORT_BLOCK {
  PVOID  Signature;
  PNDIS_MINIPORT_BLOCK  NextMiniport;
  PNDIS_M_DRIVER_BLOCK  DriverHandle;
  NDIS_HANDLE  MiniportAdapterContext;
  UNICODE_STRING  MiniportName;
  PNDIS_BIND_PATHS  BindPaths;
  NDIS_HANDLE  OpenQueue;
  REFERENCE  ShortRef;
  NDIS_HANDLE  DeviceContext;
  UCHAR  Padding1;
  UCHAR  LockAcquired;
  UCHAR  PmodeOpens;
  UCHAR  AssignedProcessor;
  KSPIN_LOCK  Lock;
  PNDIS_REQUEST  MediaRequest;
  PNDIS_MINIPORT_INTERRUPT  Interrupt;
  ULONG  Flags;
  ULONG  PnPFlags;
  LIST_ENTRY  PacketList;
  PNDIS_PACKET  FirstPendingPacket;
  PNDIS_PACKET  ReturnPacketsQueue;
  ULONG  RequestBuffer;
  PVOID  SetMCastBuffer;
  PNDIS_MINIPORT_BLOCK  PrimaryMiniport;
  PVOID  WrapperContext;
  PVOID  BusDataContext;
  ULONG  PnPCapabilities;
  PCM_RESOURCE_LIST  Resources;
  NDIS_TIMER  WakeUpDpcTimer;
  UNICODE_STRING  BaseName;
  UNICODE_STRING  SymbolicLinkName;
  ULONG  CheckForHangSeconds;
  USHORT  CFHangTicks;
  USHORT  CFHangCurrentTick;
  NDIS_STATUS  ResetStatus;
  NDIS_HANDLE  ResetOpen;
  FILTERDBS_S
  FILTER_PACKET_INDICATION_HANDLER  PacketIndicateHandler;
  NDIS_M_SEND_COMPLETE_HANDLER  SendCompleteHandler;
  NDIS_M_SEND_RESOURCES_HANDLER  SendResourcesHandler;
  NDIS_M_RESET_COMPLETE_HANDLER  ResetCompleteHandler;
  NDIS_MEDIUM  MediaType;
  ULONG  BusNumber;
  NDIS_INTERFACE_TYPE  BusType;
  NDIS_INTERFACE_TYPE  AdapterType;
  PDEVICE_OBJECT  DeviceObject;
  PDEVICE_OBJECT  PhysicalDeviceObject;
  PDEVICE_OBJECT  NextDeviceObject;
  PMAP_REGISTER_ENTRY  MapRegisters;
  PNDIS_AF_LIST  CallMgrAfList;
  PVOID  MiniportThread;
  PVOID  SetInfoBuf;
  USHORT  SetInfoBufLen;
  USHORT  MaxSendPackets;
  NDIS_STATUS  FakeStatus;
  PVOID  LockHandler;
  PUNICODE_STRING  pAdapterInstanceName;
  PNDIS_MINIPORT_TIMER  TimerQueue;
  UINT  MacOptions;
  PNDIS_REQUEST  PendingRequest;
  UINT  MaximumLongAddresses;
  UINT  MaximumShortAddresses;
  UINT  CurrentLookahead;
  UINT  MaximumLookahead;
  W_HANDLE_INTERRUPT_HANDLER  HandleInterruptHandler;
  W_DISABLE_INTERRUPT_HANDLER  DisableInterruptHandler;
  W_ENABLE_INTERRUPT_HANDLER  EnableInterruptHandler;
  W_SEND_PACKETS_HANDLER  SendPacketsHandler;
  NDIS_M_START_SENDS  DeferredSendHandler;
  ETH_RCV_INDICATE_HANDLER  EthRxIndicateHandler;
  TR_RCV_INDICATE_HANDLER  TrRxIndicateHandler;
  FDDI_RCV_INDICATE_HANDLER  FddiRxIndicateHandler;
  ETH_RCV_COMPLETE_HANDLER  EthRxCompleteHandler;
  TR_RCV_COMPLETE_HANDLER  TrRxCompleteHandler;
  FDDI_RCV_COMPLETE_HANDLER  FddiRxCompleteHandler;
  NDIS_M_STATUS_HANDLER  StatusHandler;
  NDIS_M_STS_COMPLETE_HANDLER  StatusCompleteHandler;
  NDIS_M_TD_COMPLETE_HANDLER  TDCompleteHandler;
  NDIS_M_REQ_COMPLETE_HANDLER  QueryCompleteHandler;
  NDIS_M_REQ_COMPLETE_HANDLER  SetCompleteHandler;
  NDIS_WM_SEND_COMPLETE_HANDLER  WanSendCompleteHandler;
  WAN_RCV_HANDLER  WanRcvHandler;
  WAN_RCV_COMPLETE_HANDLER  WanRcvCompleteHandler;
#if defined(NDIS_WRAPPER)
  PNDIS_MINIPORT_BLOCK  NextGlobalMiniport;
  SINGLE_LIST_ENTRY  WorkQueue[NUMBER_OF_WORK_ITEM_TYPES];
  SINGLE_LIST_ENTRY  SingleWorkItems[NUMBER_OF_SINGLE_WORK_ITEMS];
  UCHAR  SendFlags;
  UCHAR  TrResetRing;
  UCHAR  ArcnetAddress;
  UCHAR  XState;
  union {
#if ARCNET
    PNDIS_ARC_BUF  ArcBuf;
#endif
    PVOID  BusInterface;
  };
  PNDIS_LOG  Log;
  ULONG  SlotNumber;
  PCM_RESOURCE_LIST  AllocatedResources;
  PCM_RESOURCE_LIST  AllocatedResourcesTranslated;
  SINGLE_LIST_ENTRY  PatternList;
  NDIS_PNP_CAPABILITIES  PMCapabilities;
  DEVICE_CAPABILITIES  DeviceCaps;
  ULONG  WakeUpEnable;
  DEVICE_POWER_STATE  CurrentDevicePowerState;
  PIRP  pIrpWaitWake;
  SYSTEM_POWER_STATE  WaitWakeSystemState;
  LARGE_INTEGER  VcIndex;
  KSPIN_LOCK  VcCountLock;
  LIST_ENTRY  WmiEnabledVcs;
  PNDIS_GUID  pNdisGuidMap;
  PNDIS_GUID  pCustomGuidMap;
  USHORT  VcCount;
  USHORT  cNdisGuidMap;
  USHORT  cCustomGuidMap;
  USHORT  CurrentMapRegister;
  PKEVENT  AllocationEvent;
  USHORT  BaseMapRegistersNeeded;
  USHORT  SGMapRegistersNeeded;
  ULONG  MaximumPhysicalMapping;
  NDIS_TIMER  MediaDisconnectTimer;
  USHORT  MediaDisconnectTimeOut;
  USHORT  InstanceNumber;
  NDIS_EVENT  OpenReadyEvent;
  NDIS_PNP_DEVICE_STATE  PnPDeviceState;
  NDIS_PNP_DEVICE_STATE  OldPnPDeviceState;
  PGET_SET_DEVICE_DATA  SetBusData;
  PGET_SET_DEVICE_DATA  GetBusData;
  KDPC  DeferredDpc;
#if 0
  /* FIXME: */
  NDIS_STATS  NdisStats;
#else
  ULONG  NdisStats;
#endif
  PNDIS_PACKET  IndicatedPacket[MAXIMUM_PROCESSORS];
  PKEVENT  RemoveReadyEvent;
  PKEVENT  AllOpensClosedEvent;
  PKEVENT  AllRequestsCompletedEvent;
  ULONG  InitTimeMs;
  NDIS_MINIPORT_WORK_ITEM  WorkItemBuffer[NUMBER_OF_SINGLE_WORK_ITEMS];
  PDMA_ADAPTER  SystemAdapterObject;
  ULONG  DriverVerifyFlags;
  POID_LIST  OidList;
  USHORT  InternalResetCount;
  USHORT  MiniportResetCount;
  USHORT  MediaSenseConnectCount;
  USHORT  MediaSenseDisconnectCount;
  PNDIS_PACKET  *xPackets;
  ULONG  UserModeOpenReferences;
  union {
    PVOID  SavedSendHandler;
    PVOID  SavedWanSendHandler;
  };
  PVOID  SavedSendPacketsHandler;
  PVOID  SavedCancelSendPacketsHandler;
  W_SEND_PACKETS_HANDLER  WSendPacketsHandler;
  ULONG  MiniportAttributes;
  PDMA_ADAPTER  SavedSystemAdapterObject;
  USHORT  NumOpens;
  USHORT  CFHangXTicks;
  ULONG  RequestCount;
  ULONG  IndicatedPacketsCount;
  ULONG  PhysicalMediumType;
  PNDIS_REQUEST  LastRequest;
  LONG  DmaAdapterRefCount;
  PVOID  FakeMac;
  ULONG  LockDbg;
  ULONG  LockDbgX;
  PVOID  LockThread;
  ULONG  InfoFlags;
  KSPIN_LOCK  TimerQueueLock;
  PKEVENT  ResetCompletedEvent;
  PKEVENT  QueuedBindingCompletedEvent;
  PKEVENT  DmaResourcesReleasedEvent;
  FILTER_PACKET_INDICATION_HANDLER  SavedPacketIndicateHandler;
  ULONG  RegisteredInterrupts;
  PNPAGED_LOOKASIDE_LIST  SGListLookasideList;
  ULONG  ScatterGatherListSize;
#endif /* _NDIS_ */
};

#if 1
/* FIXME: */
typedef PVOID QUEUED_CLOSE;
#endif

#if defined(NDIS_WRAPPER)
#define NDIS_COMMON_OPEN_BLOCK_WRAPPER_S \
  ULONG  Flags; \
  ULONG  References; \
  KSPIN_LOCK  SpinLock; \
  NDIS_HANDLE  FilterHandle; \
  ULONG  ProtocolOptions; \
  USHORT  CurrentLookahead; \
  USHORT  ConnectDampTicks; \
  USHORT  DisconnectDampTicks; \
  W_SEND_HANDLER  WSendHandler; \
  W_TRANSFER_DATA_HANDLER  WTransferDataHandler; \
  W_SEND_PACKETS_HANDLER  WSendPacketsHandler; \
  W_CANCEL_SEND_PACKETS_HANDLER  CancelSendPacketsHandler; \
  ULONG  WakeUpEnable; \
  PKEVENT  CloseCompleteEvent; \
  QUEUED_CLOSE  QC; \
  ULONG  AfReferences; \
  PNDIS_OPEN_BLOCK  NextGlobalOpen;
#else
#define NDIS_COMMON_OPEN_BLOCK_WRAPPER_S
#endif

#define NDIS_COMMON_OPEN_BLOCK_S \
  PVOID  MacHandle; \
  NDIS_HANDLE  BindingHandle; \
  PNDIS_MINIPORT_BLOCK  MiniportHandle; \
  PNDIS_PROTOCOL_BLOCK  ProtocolHandle; \
  NDIS_HANDLE  ProtocolBindingContext; \
  PNDIS_OPEN_BLOCK  MiniportNextOpen; \
  PNDIS_OPEN_BLOCK  ProtocolNextOpen; \
  NDIS_HANDLE  MiniportAdapterContext; \
  BOOLEAN  Reserved1; \
  BOOLEAN  Reserved2; \
  BOOLEAN  Reserved3; \
  BOOLEAN  Reserved4; \
  PNDIS_STRING  BindDeviceName; \
  KSPIN_LOCK  Reserved5; \
  PNDIS_STRING  RootDeviceName; \
  union { \
    SEND_HANDLER  SendHandler; \
    WAN_SEND_HANDLER  WanSendHandler; \
  }; \
  TRANSFER_DATA_HANDLER  TransferDataHandler; \
  SEND_COMPLETE_HANDLER  SendCompleteHandler; \
  TRANSFER_DATA_COMPLETE_HANDLER  TransferDataCompleteHandler; \
  RECEIVE_HANDLER  ReceiveHandler; \
  RECEIVE_COMPLETE_HANDLER  ReceiveCompleteHandler; \
  WAN_RECEIVE_HANDLER  WanReceiveHandler; \
  REQUEST_COMPLETE_HANDLER  RequestCompleteHandler; \
  RECEIVE_PACKET_HANDLER  ReceivePacketHandler; \
  SEND_PACKETS_HANDLER  SendPacketsHandler; \
  RESET_HANDLER  ResetHandler; \
  REQUEST_HANDLER  RequestHandler; \
  RESET_COMPLETE_HANDLER  ResetCompleteHandler; \
  STATUS_HANDLER  StatusHandler; \
  STATUS_COMPLETE_HANDLER  StatusCompleteHandler; \
  NDIS_COMMON_OPEN_BLOCK_WRAPPER_S

typedef struct _NDIS_COMMON_OPEN_BLOCK {
  NDIS_COMMON_OPEN_BLOCK_S
} NDIS_COMMON_OPEN_BLOCK;

struct _NDIS_OPEN_BLOCK
{
#ifdef __cplusplus
  NDIS_COMMON_OPEN_BLOCK NdisCommonOpenBlock;
#else
  NDIS_COMMON_OPEN_BLOCK_S
#endif
#if defined(NDIS_WRAPPER)
  struct _NDIS_OPEN_CO
  {
    struct _NDIS_CO_AF_BLOCK *  NextAf;
    W_CO_CREATE_VC_HANDLER      MiniportCoCreateVcHandler;
    W_CO_REQUEST_HANDLER        MiniportCoRequestHandler;
    CO_CREATE_VC_HANDLER        CoCreateVcHandler;
    CO_DELETE_VC_HANDLER        CoDeleteVcHandler;
    PVOID                       CmActivateVcCompleteHandler;
    PVOID                       CmDeactivateVcCompleteHandler;
    PVOID                       CoRequestCompleteHandler;
    LIST_ENTRY                  ActiveVcHead;
    LIST_ENTRY                  InactiveVcHead;
    LONG                        PendingAfNotifications;
    PKEVENT                     AfNotifyCompleteEvent;
  };
#endif /* _NDIS_ */
};

#define NDIS30_PROTOCOL_CHARACTERISTICS_S \
  UCHAR  MajorNdisVersion; \
  UCHAR  MinorNdisVersion; \
  USHORT  Filler; \
  union { \
    UINT  Reserved; \
    UINT  Flags; \
  }; \
  OPEN_ADAPTER_COMPLETE_HANDLER  OpenAdapterCompleteHandler; \
  CLOSE_ADAPTER_COMPLETE_HANDLER  CloseAdapterCompleteHandler; \
  union { \
    SEND_COMPLETE_HANDLER  SendCompleteHandler; \
    WAN_SEND_COMPLETE_HANDLER  WanSendCompleteHandler; \
  }; \
  union { \
    TRANSFER_DATA_COMPLETE_HANDLER  TransferDataCompleteHandler; \
    WAN_TRANSFER_DATA_COMPLETE_HANDLER  WanTransferDataCompleteHandler; \
  }; \
  RESET_COMPLETE_HANDLER  ResetCompleteHandler; \
  REQUEST_COMPLETE_HANDLER  RequestCompleteHandler; \
  union { \
    RECEIVE_HANDLER	 ReceiveHandler; \
    WAN_RECEIVE_HANDLER  WanReceiveHandler; \
  }; \
  RECEIVE_COMPLETE_HANDLER  ReceiveCompleteHandler; \
  STATUS_HANDLER  StatusHandler; \
  STATUS_COMPLETE_HANDLER  StatusCompleteHandler; \
  NDIS_STRING  Name;

typedef struct _NDIS30_PROTOCOL_CHARACTERISTICS {
  NDIS30_PROTOCOL_CHARACTERISTICS_S
} NDIS30_PROTOCOL_CHARACTERISTICS, *PNDIS30_PROTOCOL_CHARACTERISTICS;

#define NDIS30_MINIPORT_CHARACTERISTICS_S \
  UCHAR  MajorNdisVersion; \
  UCHAR  MinorNdisVersion; \
  USHORT Filler; \
  UINT  Reserved; \
  W_CHECK_FOR_HANG_HANDLER  CheckForHangHandler; \
  W_DISABLE_INTERRUPT_HANDLER  DisableInterruptHandler; \
  W_ENABLE_INTERRUPT_HANDLER  EnableInterruptHandler; \
  W_HALT_HANDLER  HaltHandler; \
  W_HANDLE_INTERRUPT_HANDLER  HandleInterruptHandler; \
  W_INITIALIZE_HANDLER  InitializeHandler; \
  W_ISR_HANDLER  ISRHandler; \
  W_QUERY_INFORMATION_HANDLER  QueryInformationHandler; \
  W_RECONFIGURE_HANDLER  ReconfigureHandler; \
  W_RESET_HANDLER  ResetHandler; \
  union { \
    W_SEND_HANDLER  SendHandler; \
    WM_SEND_HANDLER  WanSendHandler; \
  }u1; \
  W_SET_INFORMATION_HANDLER  SetInformationHandler; \
  union { \
    W_TRANSFER_DATA_HANDLER  TransferDataHandler; \
    WM_TRANSFER_DATA_HANDLER  WanTransferDataHandler; \
  }u2;

typedef struct _NDIS30_MINIPORT_CHARACTERISTICS {
  NDIS30_MINIPORT_CHARACTERISTICS_S
} NDIS30_MINIPORT_CHARACTERISTICS, *PSNDIS30_MINIPORT_CHARACTERISTICS;

#ifdef __cplusplus

#define NDIS40_MINIPORT_CHARACTERISTICS_S \
  NDIS30_MINIPORT_CHARACTERISTICS  Ndis30Chars; \
  W_RETURN_PACKET_HANDLER  ReturnPacketHandler; \
  W_SEND_PACKETS_HANDLER  SendPacketsHandler; \
  W_ALLOCATE_COMPLETE_HANDLER  AllocateCompleteHandler;

#else /* !__cplusplus */

#define NDIS40_MINIPORT_CHARACTERISTICS_S \
  NDIS30_MINIPORT_CHARACTERISTICS_S \
  W_RETURN_PACKET_HANDLER  ReturnPacketHandler; \
  W_SEND_PACKETS_HANDLER  SendPacketsHandler; \
  W_ALLOCATE_COMPLETE_HANDLER  AllocateCompleteHandler;

#endif /* !__cplusplus */

typedef struct _NDIS40_MINIPORT_CHARACTERISTICS {
  NDIS40_MINIPORT_CHARACTERISTICS_S
} NDIS40_MINIPORT_CHARACTERISTICS, *PNDIS40_MINIPORT_CHARACTERISTICS;

#endif

#include "miniport.h"
#include "protocol.h"

#include <debug.h>

/* Exported functions */
#ifndef EXPORT
#define EXPORT NTAPI
#endif

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define NDIS_TAG  0x4e4d4953

#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#define ExInterlockedRemoveEntryList(_List,_Lock) \
 { KIRQL OldIrql; \
   KeAcquireSpinLock(_Lock, &OldIrql); \
   RemoveEntryList(_List); \
   KeReleaseSpinLock(_Lock, OldIrql); \
 }

/* missing protypes */
VOID
NTAPI
ExGetCurrentProcessorCounts(
  PULONG ThreadKernelTime,
   PULONG ThreadKernelTime,
   PULONG TotalCpuTime,
   PULONG ProcessorNumber);

VOID
NTAPI
ExGetCurrentProcessorCpuUsage(
    PULONG CpuUsage);

/* portability fixes */
#ifdef _M_AMD64
#define KfReleaseSpinLock KeReleaseSpinLock
#define KefAcquireSpinLockAtDpcLevel KeAcquireSpinLockAtDpcLevel
#define KefReleaseSpinLockFromDpcLevel KeReleaseSpinLockFromDpcLevel
#endif

#endif /* __NDISSYS_H */

/* EOF */
