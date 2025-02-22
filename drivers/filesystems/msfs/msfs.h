/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/msfs/msfs.h
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl
 *             Nikita Pechenkin (n.pechenkin@mail.ru)
 */

#ifndef __DRIVERS_FS_MS_MSFS_H
#define __DRIVERS_FS_MS_MSFS_H

#include <ntifs.h>

#define DEFAULTAPI NTAPI

typedef struct _MSFS_DEVICE_EXTENSION
{
    LIST_ENTRY FcbListHead;
    KMUTEX FcbListLock;
} MSFS_DEVICE_EXTENSION, *PMSFS_DEVICE_EXTENSION;


typedef struct _MSFS_FCB
{
    FSRTL_COMMON_FCB_HEADER RFCB;
    UNICODE_STRING Name;
    LIST_ENTRY FcbListEntry;
    KSPIN_LOCK CcbListLock;
    LIST_ENTRY CcbListHead;
    struct _MSFS_CCB *ServerCcb;
    ULONG ReferenceCount;
    LARGE_INTEGER TimeOut;
    ULONG MaxMessageSize;
    ULONG MessageCount;
    KSPIN_LOCK MessageListLock;
    LIST_ENTRY MessageListHead;
    IO_CSQ CancelSafeQueue;
    KSPIN_LOCK QueueLock;
    LIST_ENTRY PendingIrpQueue;
} MSFS_FCB, *PMSFS_FCB;


typedef struct _MSFS_DPC_CTX
{
    KTIMER Timer;
    KDPC Dpc;
    PIO_CSQ Csq;
    KEVENT Event;
    IO_CSQ_IRP_CONTEXT CsqContext;
} MSFS_DPC_CTX, *PMSFS_DPC_CTX;


typedef struct _MSFS_CCB
{
    LIST_ENTRY CcbListEntry;
    PMSFS_FCB Fcb;
} MSFS_CCB, *PMSFS_CCB;


typedef struct _MSFS_MESSAGE
{
    LIST_ENTRY MessageListEntry;
    ULONG Size;
    UCHAR Buffer[1];
} MSFS_MESSAGE, *PMSFS_MESSAGE;


#define KeLockMutex(x) KeWaitForSingleObject(x, \
                                             Executive, \
                                             KernelMode, \
                                             FALSE, \
                                             NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);

DRIVER_DISPATCH MsfsCreate;
NTSTATUS DEFAULTAPI MsfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsCreateMailslot;
NTSTATUS DEFAULTAPI MsfsCreateMailslot(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsClose;
NTSTATUS DEFAULTAPI MsfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsQueryInformation;
NTSTATUS DEFAULTAPI MsfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsSetInformation;
NTSTATUS DEFAULTAPI MsfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsRead;
NTSTATUS DEFAULTAPI MsfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsWrite;
NTSTATUS DEFAULTAPI MsfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH MsfsFileSystemControl;
NTSTATUS DEFAULTAPI MsfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath);

IO_CSQ_INSERT_IRP MsfsInsertIrp;
VOID NTAPI
MsfsInsertIrp(PIO_CSQ Csq, PIRP Irp);

IO_CSQ_REMOVE_IRP MsfsRemoveIrp;
VOID NTAPI
MsfsRemoveIrp(PIO_CSQ Csq, PIRP Irp);

IO_CSQ_PEEK_NEXT_IRP MsfsPeekNextIrp;
PIRP NTAPI
MsfsPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext);

IO_CSQ_ACQUIRE_LOCK MsfsAcquireLock;
VOID NTAPI
MsfsAcquireLock(PIO_CSQ Csq, PKIRQL Irql);

IO_CSQ_RELEASE_LOCK MsfsReleaseLock;
VOID NTAPI
MsfsReleaseLock(PIO_CSQ Csq, KIRQL Irql);

IO_CSQ_COMPLETE_CANCELED_IRP MsfsCompleteCanceledIrp;
VOID NTAPI
MsfsCompleteCanceledIrp(PIO_CSQ pCsq, PIRP Irp);

KDEFERRED_ROUTINE MsfsTimeout;
VOID NTAPI
MsfsTimeout(PKDPC Dpc,
            PVOID DeferredContext,
            PVOID SystemArgument1,
            PVOID SystemArgument2);

#endif /* __DRIVERS_FS_MS_MSFS_H */
