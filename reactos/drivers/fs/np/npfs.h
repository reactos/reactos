#ifndef __SERVICES_FS_NP_NPFS_H
#define __SERVICES_FS_NP_NPFS_H

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;

typedef struct
{
   PWCHAR Name;
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
} NPFS_FCB, *PNPFS_FCB;

VOID NpfsPipeList(VOID);

#define KeLockMutex(x) KeWaitForSingleObject(x, \
                                             UserRequest, \
                                             KernelMode, \
                                             FALSE, \
                                             NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);

#endif /* __SERVICES_FS_NP_NPFS_H */
