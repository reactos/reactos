#ifndef __SERVICES_FS_NP_NPFS_H
#define __SERVICES_FS_NP_NPFS_H

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;

typedef struct
{
   LIST_ENTRY ListEntry;
   PVOID Buffer;
   ULONG Length;
} NPFS_MSG, *PNPFS_MSG;

typedef struct
{
   LIST_ENTRY DirectoryListEntry;
   LIST_ENTRY GlobalListEntry;
   ULONG Flags;
   LIST_ENTRY MsgListHead;
   KPSIN_LOCK MsgListLock;
} NPFS_FSCONTEXT, *PNPFS_FSCONTEXT;

#endif /* __SERVICES_FS_NP_NPFS_H */
