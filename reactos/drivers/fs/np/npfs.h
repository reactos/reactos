/* $Id: npfs.h,v 1.13 2002/09/08 10:22:11 chorns Exp $ */

#ifndef __SERVICES_FS_NP_NPFS_H
#define __SERVICES_FS_NP_NPFS_H


typedef struct
{
  LIST_ENTRY PipeListHead;
  KMUTEX PipeListLock;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;

typedef struct
{
  LIST_ENTRY ListEntry;
  ULONG Size;
  PVOID Data;
  ULONG Offset;
} NPFS_PIPE_DATA, *PNPFS_PIPE_DATA;

typedef struct
{
  UNICODE_STRING PipeName;
  LIST_ENTRY PipeListEntry;
  KSPIN_LOCK FcbListLock;
  LIST_ENTRY ServerFcbListHead;
  LIST_ENTRY ClientFcbListHead;
  ULONG ReferenceCount;
  ULONG PipeType;
  ULONG PipeReadMode;
  ULONG PipeWriteMode;
  ULONG PipeBlockMode;
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
  PNPFS_PIPE Pipe;
  KEVENT ConnectEvent;
  KEVENT ReadEvent;
  ULONG PipeEnd;
  ULONG PipeState;
  ULONG ReadDataAvailable;
  ULONG WriteQuotaAvailable;

  LIST_ENTRY DataListHead;	/* Data queue */
  KSPIN_LOCK DataListLock;	/* Data queue lock */
} NPFS_FCB, *PNPFS_FCB;


extern NPAGED_LOOKASIDE_LIST NpfsPipeDataLookasideList;


#define KeLockMutex(x) KeWaitForSingleObject(x, \
                                             UserRequest, \
                                             KernelMode, \
                                             FALSE, \
                                             NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);

#define CP DPRINT("\n");

static inline VOID
NpfsFreePipeData(PNPFS_PIPE_DATA PipeData)
{
  if (PipeData->Data)
    {
      ExFreePool(PipeData->Data);
    }
  ExFreeToNPagedLookasideList(&NpfsPipeDataLookasideList, PipeData);
}


NTSTATUS STDCALL NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryVolumeInformation (PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif /* __SERVICES_FS_NP_NPFS_H */
