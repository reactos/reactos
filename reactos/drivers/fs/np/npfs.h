/* $Id$ */

#ifndef __DRIVERS_FS_NP_NPFS_H
#define __DRIVERS_FS_NP_NPFS_H

typedef struct _NPFS_DEVICE_EXTENSION
{
  LIST_ENTRY PipeListHead;
  KMUTEX PipeListLock;
  ULONG MinQuota;
  ULONG DefaultQuota;
  ULONG MaxQuota;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;

typedef struct _NPFS_PIPE
{
  UNICODE_STRING PipeName;
  LIST_ENTRY PipeListEntry;
  KMUTEX FcbListLock;
  LIST_ENTRY ServerFcbListHead;
  LIST_ENTRY ClientFcbListHead;
  LIST_ENTRY WaiterListHead;
  ULONG PipeType;
  ULONG ReadMode;
  ULONG WriteMode;
  ULONG CompletionMode;
  ULONG PipeConfiguration;
  ULONG MaximumInstances;
  ULONG CurrentInstances;
  ULONG InboundQuota;
  ULONG OutboundQuota;
  LARGE_INTEGER TimeOut;
} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_FCB
{
  LIST_ENTRY FcbListEntry;
  struct _NPFS_FCB* OtherSide;
  struct ETHREAD *Thread;
  PNPFS_PIPE Pipe;
  KEVENT ConnectEvent;
  KEVENT Event;
  ULONG PipeEnd;
  ULONG PipeState;
  ULONG ReadDataAvailable;
  ULONG WriteQuotaAvailable;

  PVOID Data;
  PVOID ReadPtr;
  PVOID WritePtr;
  ULONG MaxDataLength;

  KSPIN_LOCK DataListLock;	/* Data queue lock */
} NPFS_FCB, *PNPFS_FCB;

typedef struct _NPFS_WAITER_ENTRY
{
  LIST_ENTRY Entry;
  PIRP Irp;
  PNPFS_PIPE Pipe;
  PNPFS_FCB Fcb;
} NPFS_WAITER_ENTRY, *PNPFS_WAITER_ENTRY;


extern NPAGED_LOOKASIDE_LIST NpfsPipeDataLookasideList;


#define KeLockMutex(x) KeWaitForSingleObject(x, \
                                             UserRequest, \
                                             KernelMode, \
                                             FALSE, \
                                             NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);


NTSTATUS STDCALL NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryVolumeInformation (PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif /* __DRIVERS_FS_NP_NPFS_H */
