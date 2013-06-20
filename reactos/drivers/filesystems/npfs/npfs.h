#ifndef __DRIVERS_FS_NP_NPFS_H
#define __DRIVERS_FS_NP_NPFS_H

#include <ntifs.h>
#include <ndk/iotypes.h>
#include <pseh/pseh2.h>

#define TAG_NPFS_CCB 'cFpN'
#define TAG_NPFS_CCB_DATA 'iFpN' /* correct? */
#define TAG_NPFS_FCB 'FFpN'
#define TAG_NPFS_NAMEBLOCK 'nFpN'
#define TAG_NPFS_THREAD_CONTEXT 'tFpN'

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

typedef enum _FCB_TYPE
{
    FCB_INVALID,
    FCB_DEVICE,
    FCB_DIRECTORY,
    FCB_PIPE
} FCB_TYPE;

typedef enum _CCB_TYPE
{
    CCB_INVALID,
    CCB_DEVICE,
    CCB_DIRECTORY,
    CCB_PIPE
} CCB_TYPE;

/* Volume Control Block (VCB) aka Device Extension */
typedef struct _NPFS_VCB
{
    LIST_ENTRY PipeListHead;
    LIST_ENTRY ThreadListHead;
    KMUTEX PipeListLock;
    ULONG EmptyWaiterCount;
    ULONG MinQuota;
    ULONG DefaultQuota;
    ULONG MaxQuota;
    struct _NPFS_FCB *DeviceFcb;
    struct _NPFS_FCB *RootFcb;
} NPFS_VCB, *PNPFS_VCB;

typedef struct _NPFS_FCB
{
    FCB_TYPE Type;
    PNPFS_VCB Vcb;
    volatile LONG RefCount;
    UNICODE_STRING PipeName;
    LIST_ENTRY PipeListEntry;
    KMUTEX CcbListLock;
    LIST_ENTRY ServerCcbListHead;
    LIST_ENTRY ClientCcbListHead;
    LIST_ENTRY WaiterListHead;
    LIST_ENTRY EmptyBufferListHead;
    ULONG PipeType;
    ULONG ClientReadMode;
    ULONG ServerReadMode;
    ULONG CompletionMode;
    ULONG PipeConfiguration;
    ULONG MaximumInstances;
    ULONG CurrentInstances;
    ULONG InboundQuota;
    ULONG OutboundQuota;
    LARGE_INTEGER TimeOut;
} NPFS_FCB, *PNPFS_FCB;


typedef struct _NPFS_CCB_DIRECTORY_DATA
{
    UNICODE_STRING SearchPattern;
    ULONG FileIndex;
} NPFS_CCB_DIRECTORY_DATA, *PNPFS_CCB_DIRECTORY_DATA;


typedef struct _NPFS_CCB
{
    LIST_ENTRY CcbListEntry;
    CCB_TYPE Type;
    PNPFS_FCB Fcb;
    PFILE_OBJECT FileObject;

    struct _NPFS_CCB* OtherSide;
    struct ETHREAD *Thread;
    KEVENT ConnectEvent;
    KEVENT ReadEvent;
    KEVENT WriteEvent;
    ULONG PipeEnd;
    ULONG PipeState;
    ULONG ReadDataAvailable;
    ULONG WriteQuotaAvailable;
    volatile LONG RefCount;

    LIST_ENTRY ReadRequestListHead;

    PVOID Data;
    PVOID ReadPtr;
    PVOID WritePtr;
    ULONG MaxDataLength;

    FAST_MUTEX DataListLock;    /* Data queue lock */

    union
    {
        NPFS_CCB_DIRECTORY_DATA Directory;
    } u;

} NPFS_CCB, *PNPFS_CCB;

typedef struct _NPFS_CONTEXT
{
    LIST_ENTRY ListEntry;
    PKEVENT WaitEvent;
} NPFS_CONTEXT, *PNPFS_CONTEXT;

typedef struct _NPFS_THREAD_CONTEXT
{
    ULONG Count;
    KEVENT Event;
    PNPFS_VCB Vcb;
    LIST_ENTRY ListEntry;
    PVOID WaitObjectArray[MAXIMUM_WAIT_OBJECTS];
    KWAIT_BLOCK WaitBlockArray[MAXIMUM_WAIT_OBJECTS];
    PIRP WaitIrpArray[MAXIMUM_WAIT_OBJECTS];
} NPFS_THREAD_CONTEXT, *PNPFS_THREAD_CONTEXT;

typedef struct _NPFS_WAITER_ENTRY
{
    LIST_ENTRY Entry;
    PNPFS_CCB Ccb;
} NPFS_WAITER_ENTRY, *PNPFS_WAITER_ENTRY;


extern NPAGED_LOOKASIDE_LIST NpfsPipeDataLookasideList;


#define KeLockMutex(x) KeWaitForSingleObject(x, \
    UserRequest, \
    KernelMode, \
    FALSE, \
    NULL);

#define KeUnlockMutex(x) KeReleaseMutex(x, FALSE);

#define PAGE_ROUND_UP(x) ( (((ULONG_PTR)x)%PAGE_SIZE) ? ((((ULONG_PTR)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG_PTR)x) )

DRIVER_DISPATCH NpfsCreate;
NTSTATUS NTAPI NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsCreateNamedPipe;
NTSTATUS NTAPI NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsCleanup;
NTSTATUS NTAPI NpfsCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsClose;
NTSTATUS NTAPI NpfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsDirectoryControl;
NTSTATUS NTAPI NpfsDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsRead;
NTSTATUS NTAPI NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsWrite;
NTSTATUS NTAPI NpfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsFlushBuffers;
NTSTATUS NTAPI NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsFileSystemControl;
NTSTATUS NTAPI NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsQueryInformation;
NTSTATUS NTAPI NpfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsSetInformation;
NTSTATUS NTAPI NpfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsQueryVolumeInformation;
NTSTATUS NTAPI NpfsQueryVolumeInformation (PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath);

VOID
NpfsDereferenceFcb(PNPFS_FCB Fcb);

PNPFS_FCB
NpfsFindPipe(PNPFS_VCB Vcb,
             PUNICODE_STRING PipeName);

FCB_TYPE
NpfsGetFcb(PFILE_OBJECT FileObject,
           PNPFS_FCB *Fcb);

CCB_TYPE
NpfsGetCcb(PFILE_OBJECT FileObject,
           PNPFS_CCB *Ccb);

#endif /* __DRIVERS_FS_NP_NPFS_H */
