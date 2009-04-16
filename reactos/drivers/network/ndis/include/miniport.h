/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.h
 * PURPOSE:     Definitions for routines used by NDIS miniport drivers
 */

#ifndef __MINIPORT_H
#define __MINIPORT_H

#include <ndis.h>
#include <ndissys.h>

struct _ADAPTER_BINDING;

typedef struct _HARDWARE_ADDRESS {
    union {
        UCHAR Medium802_3[ETH_LENGTH_OF_ADDRESS];
    } Type;
} HARDWARE_ADDRESS, *PHARDWARE_ADDRESS;

/* Information about a miniport */
typedef struct _NDIS_M_DRIVER_BLOCK {
    LIST_ENTRY                      ListEntry;                /* Entry on global list */
    KSPIN_LOCK                      Lock;                     /* Protecting spin lock */
    NDIS_MINIPORT_CHARACTERISTICS   MiniportCharacteristics;  /* Miniport characteristics */
    WORK_QUEUE_ITEM                 WorkItem;                 /* Work item */
    PDRIVER_OBJECT                  DriverObject;             /* Driver object of miniport */
    LIST_ENTRY                      DeviceList;               /* Adapters created by miniport */
    PUNICODE_STRING                 RegistryPath;             /* SCM Registry key */
#if !defined(_MSC_VER) && defined(__NDIS_H)
} NDIS_M_DRIVER_BLOCK_COMPATIBILITY_HACK_DONT_USE;
#else
} NDIS_M_DRIVER_BLOCK, *PNDIS_M_DRIVER_BLOCK;
#endif

/* resources allocated on behalf on the miniport */
#define MINIPORT_RESOURCE_TYPE_MEMORY 0
typedef struct _MINIPORT_RESOURCE {
    LIST_ENTRY     ListEntry;
    ULONG          ResourceType;
    PVOID          Resource;
} MINIPORT_RESOURCE, *PMINIPORT_RESOURCE;

/* Configuration context */
typedef struct _MINIPORT_CONFIGURATION_CONTEXT {
    NDIS_HANDLE    Handle;
    LIST_ENTRY     ResourceListHead;
    KSPIN_LOCK     ResourceLock;
} MINIPORT_CONFIGURATION_CONTEXT, *PMINIPORT_CONFIGURATION_CONTEXT;

/* Bugcheck callback context */
typedef struct _MINIPORT_BUGCHECK_CONTEXT {
    PVOID                       DriverContext;
    ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler;
    PKBUGCHECK_CALLBACK_RECORD  CallbackRecord;
} MINIPORT_BUGCHECK_CONTEXT, *PMINIPORT_BUGCHECK_CONTEXT;

/* a miniport's shared memory */
typedef struct _MINIPORT_SHARED_MEMORY {
    PDMA_ADAPTER          AdapterObject;
    ULONG                 Length;
    PHYSICAL_ADDRESS      PhysicalAddress;
    PVOID                 VirtualAddress;
    BOOLEAN               Cached;
    PNDIS_MINIPORT_BLOCK  Adapter;
    PVOID                 Context;
} MINIPORT_SHARED_MEMORY, *PMINIPORT_SHARED_MEMORY;

/* A structure of WrapperConfigurationContext (not compatible with the
   Windows one). */
typedef struct _NDIS_WRAPPER_CONTEXT {
    HANDLE            RegistryHandle;
    PDEVICE_OBJECT    DeviceObject;
    ULONG             BusNumber;
} NDIS_WRAPPER_CONTEXT, *PNDIS_WRAPPER_CONTEXT;

#define GET_MINIPORT_DRIVER(Handle)((PNDIS_M_DRIVER_BLOCK)Handle)

/* Information about a logical adapter */
typedef struct _LOGICAL_ADAPTER
{
    NDIS_MINIPORT_BLOCK         NdisMiniportBlock;      /* NDIS defined fields */
    PNDIS_MINIPORT_WORK_ITEM    WorkQueueHead;          /* Head of work queue */
    PNDIS_MINIPORT_WORK_ITEM    WorkQueueTail;          /* Tail of work queue */
    LIST_ENTRY                  ListEntry;              /* Entry on global list */
    LIST_ENTRY                  MiniportListEntry;      /* Entry on miniport driver list */
    LIST_ENTRY                  ProtocolListHead;       /* List of bound protocols */
    ULONG                       MediumHeaderSize;       /* Size of medium header */
    HARDWARE_ADDRESS            Address;                /* Hardware address of adapter */
    ULONG                       AddressLength;          /* Length of hardware address */
    PMINIPORT_BUGCHECK_CONTEXT  BugcheckContext;        /* Adapter's shutdown handler */
} LOGICAL_ADAPTER, *PLOGICAL_ADAPTER;

#define GET_LOGICAL_ADAPTER(Handle)((PLOGICAL_ADAPTER)Handle)

extern LIST_ENTRY MiniportListHead;
extern KSPIN_LOCK MiniportListLock;
extern LIST_ENTRY AdapterListHead;
extern KSPIN_LOCK AdapterListLock;


#ifdef DBG
VOID
MiniDisplayPacket(
    PNDIS_PACKET Packet);
#endif /* DBG */

VOID
MiniIndicateData(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_HANDLE         MacReceiveContext,
    PVOID               HeaderBuffer,
    UINT                HeaderBufferSize,
    PVOID               LookaheadBuffer,
    UINT                LookaheadBufferSize,
    UINT                PacketSize);

BOOLEAN
MiniAdapterHasAddress(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet);

PLOGICAL_ADAPTER
MiniLocateDevice(
    PNDIS_STRING AdapterName);

NDIS_STATUS
MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PVOID               Buffer,
    PULONG              BytesWritten);

VOID
FASTCALL
MiniQueueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE WorkItemType,
    PVOID               WorkItemContext,
    BOOLEAN             Top);

NDIS_STATUS
FASTCALL
MiniDequeueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE *WorkItemType,
    PVOID               *WorkItemContext);

NDIS_STATUS
MiniDoRequest(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_REQUEST NdisRequest);

BOOLEAN
NdisFindDevice(
    UINT   VendorID,
    UINT   DeviceID,
    PUINT  BusNumber,
    PUINT  SlotNumber);

VOID
NdisStartDevices();

VOID
NTAPI
MiniportWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID WorkItem);

VOID NTAPI
MiniSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status);

BOOLEAN
MiniIsBusy(
    PLOGICAL_ADAPTER Adapter,
    NDIS_WORK_ITEM_TYPE Type);

#endif /* __MINIPORT_H */

/* EOF */

