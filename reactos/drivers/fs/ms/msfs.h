#ifndef __SERVICES_FS_MS_MSFS_H
#define __SERVICES_FS_MS_MSFS_H

typedef struct _MSFS_DEVICE_EXTENSION
{
   LIST_ENTRY MailslotListHead;
   KMUTEX MailslotListLock;
} MSFS_DEVICE_EXTENSION, *PMSFS_DEVICE_EXTENSION;

typedef struct _MSFS_MAILSLOT
{
   UNICODE_STRING Name;
   LIST_ENTRY MailslotListEntry;
   KSPIN_LOCK FcbListLock;
   LIST_ENTRY FcbListHead;
   struct _MSFS_FCB *ServerFcb;
   ULONG ReferenceCount;
   LARGE_INTEGER TimeOut;
   KEVENT MessageEvent;
   ULONG MaxMessageSize;
   ULONG MessageCount;
   KSPIN_LOCK MessageListLock;
   LIST_ENTRY MessageListHead;
} MSFS_MAILSLOT, *PMSFS_MAILSLOT;

typedef struct _MSFS_FCB
{
   LIST_ENTRY FcbListEntry;
   PMSFS_MAILSLOT Mailslot;
} MSFS_FCB, *PMSFS_FCB;

typedef struct _MSFS_MESSAGE
{
   LIST_ENTRY MessageListEntry;
   ULONG Size;
   UCHAR Buffer[1];
} MSFS_MESSAGE, *PMSFS_MESSAGE;


#define KeLockMutex(x) KeWaitForSingleObject(x, \
                                             UserRequest, \
                                             KernelMode, \
                                             FALSE, \
                                             NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);

NTSTATUS STDCALL MsfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MsfsCreateMailslot(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MsfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL MsfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MsfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL MsfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MsfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL MsfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif /* __SERVICES_FS_NP_NPFS_H */
