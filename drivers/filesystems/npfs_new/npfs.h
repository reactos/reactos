#ifndef __DRIVERS_FS_NP_NPFS_H
#define __DRIVERS_FS_NP_NPFS_H

#include <ntifs.h>
#include <ndk/iotypes.h>

#include <stdarg.h>
#include <stdio.h>
#include <debug.h>

#define DPFLTR_NPFS_ID DPFLTR_FASTFAT_ID

/* Node type codes for NPFS */
#define NPFS_NODETYPE_VCB 0x401
#define NPFS_NODETYPE_DCB 0x402
#define NPFS_NODETYPE_FCB 0x404
#define NPFS_NODETYPE_CCB 0x406

typedef struct _NPFS_VCB
{
	/* Common node header */
	CSHORT NodeTypeCode;
	CSHORT NodeByteSize;

} NPFS_VCB, *PNPFS_VCB;

typedef struct _NPFS_FCB
{
	FSRTL_COMMON_FCB_HEADER RFCB; /* Includes common node header */
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
	/* Common node header */
	CSHORT NodeTypeCode;
	CSHORT NodeByteSize;

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

typedef struct _NPFS_DEVICE_EXTENSION
{
	LIST_ENTRY PipeListHead;
	LIST_ENTRY ThreadListHead;
	KMUTEX PipeListLock;
	ULONG EmptyWaiterCount;
	ULONG MinQuota;
	ULONG DefaultQuota;
	ULONG MaxQuota;

	NPFS_VCB Vcb;
} NPFS_DEVICE_EXTENSION, *PNPFS_DEVICE_EXTENSION;


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
DRIVER_DISPATCH NpfsCreateNamedPipe;
DRIVER_DISPATCH NpfsCleanup;
DRIVER_DISPATCH NpfsClose;

DRIVER_DISPATCH NpfsRead;
DRIVER_DISPATCH NpfsWrite;

DRIVER_DISPATCH NpfsFlushBuffers;

DRIVER_DISPATCH NpfsFileSystemControl;
DRIVER_DISPATCH NpfsDirectoryControl;

DRIVER_DISPATCH NpfsQueryInformation;
DRIVER_DISPATCH NpfsSetInformation;

DRIVER_DISPATCH NpfsQueryVolumeInformation;

DRIVER_INITIALIZE DriverEntry;

#endif /* __DRIVERS_FS_NP_NPFS_H */
