/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        ndisuio.h
 * PURPOSE:     NDISUIO definitions
 */
#ifndef __NDISUIO_H
#define __NDISUIO_H

#include <wdm.h>
#include <ndis.h>
//#include <nuiouser.h>
#include <ndistapi.h>
#include <ndisguid.h>

struct _NDISUIO_ADAPTER_CONTEXT
{
    /* Asynchronous completion */
    NDIS_STATUS AsyncStatus;
    KEVENT AsyncEvent;

    /* NDIS binding information */
    NDIS_HANDLE BindingHandle;
    
    /* Reference count information */
    ULONG OpenCount;
    LIST_ENTRY OpenEntryList;

    /* Receive packet list */
    LIST_ENTRY PacketList;
    KEVENT PacketReadEvent;

    /* Global list entry */
    LIST_ENTRY ListEntry;

    /* Spin lock */
    KSPIN_LOCK Spinlock;
} NDISUIO_ADAPTER_CONTEXT, *PNDISUIO_ADAPTER_CONTEXT;

struct _NDISUIO_OPEN_ENTRY
{
    /* File object */
    PFILE_OBJECT FileObject;
    
    /* List entry */
    LIST_ENTRY ListEntry;
} NDISUIO_OPEN_ENTRY, *PNDISUIO_OPEN_ENTRY;

/* NDIS version info */
#define NDIS_MAJOR_VERISON 5
#define NDIS_MINOR_VERSION 0

#endif /* __NDISUIO_H */
