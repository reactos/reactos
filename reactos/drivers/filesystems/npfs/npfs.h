#ifndef __DRIVERS_FS_NP_NPFS_H
#define __DRIVERS_FS_NP_NPFS_H

#include <ntifs.h>
#include <ndk/iotypes.h>

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

typedef struct _NPFS_FCB
{
	FSRTL_COMMON_FCB_HEADER RFCB;
	UNICODE_STRING PipeName;
	LIST_ENTRY PipeListEntry;
	KMUTEX CcbListLock;
	LIST_ENTRY ServerCcbListHead;
	LIST_ENTRY ClientCcbListHead;
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
} NPFS_FCB, *PNPFS_FCB;

typedef struct _NPFS_CCB
{
	LIST_ENTRY CcbListEntry;
	struct _NPFS_CCB* OtherSide;
	struct ETHREAD *Thread;
	PNPFS_FCB Fcb;
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
	PNPFS_DEVICE_EXTENSION DeviceExt;
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
NTSTATUS STDCALL NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsCreateNamedPipe;
NTSTATUS STDCALL NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsCleanup;
NTSTATUS STDCALL NpfsCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsClose;
NTSTATUS STDCALL NpfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsRead;
NTSTATUS STDCALL NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsWrite;
NTSTATUS STDCALL NpfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsFlushBuffers;
NTSTATUS STDCALL NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsFileSystemControl;
NTSTATUS STDCALL NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsQueryInformation;
NTSTATUS STDCALL NpfsQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsSetInformation;
NTSTATUS STDCALL NpfsSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

DRIVER_DISPATCH NpfsQueryVolumeInformation;
NTSTATUS STDCALL NpfsQueryVolumeInformation (PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
			PUNICODE_STRING RegistryPath);

#endif /* __DRIVERS_FS_NP_NPFS_H */
