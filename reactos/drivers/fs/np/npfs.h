#ifndef __SERVICES_FS_NP_NPFS_H
#define __SERVICES_FS_NP_NPFS_H

typedef struct
{
   LIST_ENTRY PipeListHead;
   KMUTEX PipeListLock;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;

typedef struct
{
   UNICODE_STRING PipeName;
   LIST_ENTRY PipeListEntry;
   KSPIN_LOCK FcbListLock;
   LIST_ENTRY FcbListHead;
   ULONG ReferenceCount;
   ULONG MaxInstances;
   LARGE_INTEGER TimeOut;
} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_FCB
{
   LIST_ENTRY FcbListEntry;
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   ULONG InBufferSize;
   ULONG OutBufferSize;
   PNPFS_PIPE Pipe;
   struct _NPFS_FCB* OtherSide;
   BOOLEAN IsServer;
   KEVENT ConnectEvent;
} NPFS_FCB, *PNPFS_FCB;


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

NTSTATUS STDCALL NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryVolumeInformation (PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif /* __SERVICES_FS_NP_NPFS_H */
