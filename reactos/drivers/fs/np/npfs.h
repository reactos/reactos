#ifndef __SERVICES_FS_NP_NPFS_H
#define __SERVICES_FS_NP_NPFS_H

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;

typedef struct
{
   PVOID Buffer;
   ULONG Size;
   LIST_ENTRY ListEntry;
} NPFS_MSG, *PNPFS_MSG;

typedef struct
{
   LIST_ENTRY ListEntry;
   PWSTR Name;
   ULONG FileAttributes;
   ULONG OpenMode;
   ULONG PipeType;
   ULONG PipeRead;
   ULONG PipeWait;
   ULONG MaxInstances;
   ULONG InBufferSize;
   ULONG OutBufferSize;
   LARGE_INTEGER Timeout;
   KSPIN_LOCK MsgListLock;
   LIST_ENTRY MsgListHead;
} NPFS_FSCONTEXT, *PNPFS_FSCONTEXT;

extern LIST_ENTRY PipeListHead;
extern KSPIN_LOCK PipeListLock;

#endif /* __SERVICES_FS_NP_NPFS_H */
