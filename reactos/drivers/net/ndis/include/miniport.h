/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.h
 * PURPOSE:     Definitions for routines used by NDIS miniport drivers
 */

#ifndef __MINIPORT_H
#define __MINIPORT_H

#include <ndissys.h>


/* Information about a miniport */
typedef struct _MINIPORT_DRIVER {
    LIST_ENTRY                      ListEntry;          /* Entry on global list */
    KSPIN_LOCK                      Lock;               /* Protecting spin lock */
    ULONG                           RefCount;           /* Reference count */
    NDIS_MINIPORT_CHARACTERISTICS   Chars;              /* Miniport characteristics */
    WORK_QUEUE_ITEM                 WorkItem;           /* Work item */
    PDRIVER_OBJECT                  DriverObject;       /* Driver object of miniport */
    NDIS_STRING                     RegistryPath;       /* Registry path of miniport */
    LIST_ENTRY                      AdapterListHead;    /* Adapters created by miniport */
} MINIPORT_DRIVER, *PMINIPORT_DRIVER;

#define GET_MINIPORT_DRIVER(Handle)((PMINIPORT_DRIVER)Handle)

/* Information about a logical adapter */
typedef struct _LOGICAL_ADAPTER {
    LIST_ENTRY                  ListEntry;              /* Entry on global list */
    LIST_ENTRY                  MiniportListEntry;      /* Entry on miniport driver list */
    KSPIN_LOCK                  Lock;                   /* Protecting spin lock */
    ULONG                       RefCount;               /* Reference count */
    PMINIPORT_DRIVER            Miniport;               /* Miniport owning this adapter */
    UNICODE_STRING              DeviceName;             /* Device name of this adapter */
    PDEVICE_OBJECT              DeviceObject;           /* Device object of adapter */
    PVOID                       MiniportAdapterContext; /* Adapter context for miniport */
    ULONG                       Attributes;             /* Attributes of adapter */
    NDIS_INTERFACE_TYPE         AdapterType;            /* Type of adapter interface */
    /* TRUE if the miniport has called NdisSetAttributes(Ex) for this adapter */
    BOOLEAN                     AttributesSet;
    PNDIS_MINIPORT_INTERRUPT    InterruptObject;        /* Interrupt object for adapter */
    PVOID                       QueryBuffer;            /* Buffer to use for queries */
    ULONG                       QueryBufferLength;      /* Length of QueryBuffer */
} LOGICAL_ADAPTER, *PLOGICAL_ADAPTER;

#define GET_LOGICAL_ADAPTER(Handle)((PLOGICAL_ADAPTER)Handle)


extern LIST_ENTRY MiniportListHead;
extern KSPIN_LOCK MiniportListLock;
extern LIST_ENTRY AdapterListHead;
extern KSPIN_LOCK AdapterListLock;

PLOGICAL_ADAPTER MiniLocateDevice(
    PNDIS_STRING AdapterName);

NDIS_STATUS MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PULONG              BytesWritten);

#endif /* __MINIPORT_H */

/* EOF */
