#ifndef __DRIVERS_FS_NP_NPFS_H
#define __DRIVERS_FS_NP_NPFS_H

#include <ntifs.h>

#if defined(__GNUC__)
#include <ndk/iotypes.h>
#define EXTENDED_IO_STACK_LOCATION IO_STACK_LOCATION
#define PEXTENDED_IO_STACK_LOCATION PIO_STACK_LOCATION

#elif defined(_MSC_VER)
#define STDCALL
#define KEBUGCHECK KeBugCheck
typedef struct _NAMED_PIPE_CREATE_PARAMETERS
{
  ULONG           NamedPipeType;
  ULONG           ReadMode;
  ULONG           CompletionMode;
  ULONG           MaximumInstances;
  ULONG           InboundQuota;
  ULONG           OutboundQuota;
  LARGE_INTEGER   DefaultTimeout;
  BOOLEAN         TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;
typedef struct _EXTENDED_IO_STACK_LOCATION {

    /* Included for padding */
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    union {

        struct {
            PIO_SECURITY_CONTEXT            SecurityContext;
            ULONG                           Options;
            USHORT                          Reserved;
            USHORT                          ShareAccess;
            PNAMED_PIPE_CREATE_PARAMETERS   Parameters;
        } CreatePipe;

    } Parameters;
    PDEVICE_OBJECT  DeviceObject;
    PFILE_OBJECT  FileObject;
    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID  Context;

} EXTENDED_IO_STACK_LOCATION, *PEXTENDED_IO_STACK_LOCATION;


#else
#error Unknown compiler
#endif

typedef struct _NPFS_DEVICE_EXTENSION
{
  LIST_ENTRY PipeListHead;
  LIST_ENTRY ThreadListHead;
  KMUTEX PipeListLock;
  ULONG EmptyWaiterCount;
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
  LIST_ENTRY EmptyBufferListHead;
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
  KEVENT ReadEvent;
  KEVENT WriteEvent;
  ULONG PipeEnd;
  ULONG PipeState;
  ULONG ReadDataAvailable;
  ULONG WriteQuotaAvailable;

  LIST_ENTRY ReadRequestListHead;

  PVOID Data;
  PVOID ReadPtr;
  PVOID WritePtr;
  ULONG MaxDataLength;

  FAST_MUTEX DataListLock;	/* Data queue lock */
} NPFS_FCB, *PNPFS_FCB;

typedef struct _NPFS_CONTEXT
{
  LIST_ENTRY ListEntry;
  PKEVENT WaitEvent;
} NPFS_CONTEXT, *PNPFS_CONTEXT;

typedef struct _NPFS_THREAD_CONTEXT
{
  ULONG Count;
  KEVENT Event;
  PNPFS_DEVICE_EXTENSION DeviceExt;
  LIST_ENTRY ListEntry;
  PVOID WaitObjectArray[MAXIMUM_WAIT_OBJECTS];
  KWAIT_BLOCK WaitBlockArray[MAXIMUM_WAIT_OBJECTS];
  PIRP WaitIrpArray[MAXIMUM_WAIT_OBJECTS];
} NPFS_THREAD_CONTEXT, *PNPFS_THREAD_CONTEXT;

typedef struct _NPFS_WAITER_ENTRY
{
  LIST_ENTRY Entry;
  PNPFS_FCB Fcb;
} NPFS_WAITER_ENTRY, *PNPFS_WAITER_ENTRY;


extern NPAGED_LOOKASIDE_LIST NpfsPipeDataLookasideList;


#define KeLockMutex(x) KeWaitForSingleObject(x, \
                                             UserRequest, \
                                             KernelMode, \
                                             FALSE, \
                                             NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);

#define PAGE_ROUND_UP(x) ( (((ULONG_PTR)x)%PAGE_SIZE) ? ((((ULONG_PTR)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG_PTR)x) )

NTSTATUS STDCALL NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL NpfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL NpfsQueryVolumeInformation (PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif /* __DRIVERS_FS_NP_NPFS_H */
