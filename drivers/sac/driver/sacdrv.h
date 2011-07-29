/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/sacdrv.h
 * PURPOSE:		 Header for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/
#include <ntifs.h>
#include <ntoskrnl/include/internal/hdl.h>

#define SAC_DBG_ENTRY_EXIT		0x01
#define SAC_DBG_INIT			0x04
#define SAC_DBG_MM				0x1000

#define SAC_DBG(x, ...)						\
	if (SACDebug & x)						\
	{										\
		DbgPrint("SAC %s: ", __FUNCTION__);	\
		DbgPrint(__VA_ARGS__);				\
	}

#define CHECK_PARAMETER_WITH_STATUS(Parameter, Status)	\
{	\
	ASSERT((Parameter)); \
	if (!Parameter)	\
	{	\
		return Status; \
	}	\
}
#define CHECK_PARAMETER(x)		\
	CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER)
#define CHECK_PARAMETER1(x)		\
	CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_1)
#define CHECK_PARAMETER2(x)		\
	CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_2)
#define CHECK_PARAMETER3(x)		\
	CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_3)
#define CHECK_ALLOCATION(x)		\
	CHECK_PARAMETER_WITH_STATUS(x, STATUS_OUT_OF_MEMORY)
	
#define SacAllocatePool(Length, Tag)	\
	MyAllocatePool(Length, Tag, __FILE__, __LINE__)

#define ChannelLock(Channel, x)	\
{	\
	KeWaitForSingleObject(	\
		&(Channel)->x.Lock,	\
		Executive,	\
		KernelMode,	\
		FALSE,	\
		NULL);	\
	ASSERT((Channel)->x.RefCount == 0);	\
	InterlockedIncrement(&(Channel)->x.RefCount);	\
}

#define ChannelUnlock(Channel, x)	\
{	\
	ASSERT((Channel)->x.RefCount == 1);	\
	InterlockedDecrement(&(Channel)->x.RefCount);	\
	KeReleaseSemaphore(	\
		&(Channel)->x.Lock,	\
		SEMAPHORE_INCREMENT,	\
		1,	\
		FALSE);	\
}

#define ChannelLockOBuffer(Channel)			ChannelLock(Channel, ChannelOBufferLock);
#define ChannelUnlockOBuffer(Channel)		ChannelUnlock(Channel, ChannelOBufferLock);
#define ChannelLockIBuffer(Channel)			ChannelLock(Channel, ChannelIBufferLock);
#define ChannelUnlockIBuffer(Channel)		ChannelUnlock(Channel, ChannelIBufferLock);
#define ChannelLockAttributes(Channel)		ChannelLock(Channel, ChannelAttributesLock);
#define ChannelUnlockAttributes(Channel)	ChannelUnlock(Channel, ChannelAttributesLock);

#define ChannelInitializeEvent(Channel, Attributes, x)	\
{	\
	PVOID Object, WaitObject;	\
	if (Attributes->x)	\
	{	\
		if (!VerifyEventWaitable(Attributes->x, &Object, &WaitObject))	\
		{	\
			goto FailChannel;	\
		} \
		Channel->x = Attributes->x;	\
		Channel->x##ObjectBody = Object;	\
		Channel->x##WaitObjectBody = WaitObject;	\
	}	\
}

#define ChannelSetEvent(Channel, x)	\
{	\
	ASSERT(Channel->x);	\
	ASSERT(Channel->x##ObjectBody);	\
	ASSERT(Channel->x##WaitObjectBody);	\
	if (Channel->x)	\
	{	\
		KeSetEvent(Channel->x, EVENT_INCREMENT, FALSE);	\
		Status = STATUS_SUCCESS;	\
	}	\
	else	\
	{	\
		Status = STATUS_UNSUCCESSFUL;	\
	}	\
}

#define ChannelClearEvent(Channel, x)	\
{	\
	ASSERT(Channel->x);	\
	ASSERT(Channel->x##ObjectBody);	\
	ASSERT(Channel->x##WaitObjectBody);	\
	if (Channel->x)	\
	{	\
		KeClearEvent(Channel->x);	\
		Status = STATUS_SUCCESS;	\
	}	\
	else	\
	{	\
		Status = STATUS_UNSUCCESSFUL;	\
	}	\
}

//Rcp? - sacdrv.sys - SAC Driver (Headless)
//RcpA - sacdrv.sys -     Internal memory mgr alloc block
//RcpI - sacdrv.sys -     Internal memory mgr initial heap block
//RcpS - sacdrv.sys -     Security related block
#define GENERIC_TAG				'?pcR'
#define ALLOC_BLOCK_TAG			'ApcR'
#define INITIAL_BLOCK_TAG		'IpcR'
#define SECURITY_BLOCK_TAG		'SpcR'
#define FREE_POOL_TAG			'FpcR'

#define LOCAL_MEMORY_SIGNATURE 	'SSEL'
#define GLOBAL_MEMORY_SIGNATURE	'DAEH'

#define SAC_MEMORY_LIST_SIZE	(1 * 1024 * 1024)

#define SAC_OBUFFER_SIZE		(2 * 1024)

#define SAC_CHANNEL_FLAG_CLOSE_EVENT		0x2
#define SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT	0x4
#define SAC_CHANNEL_FLAG_LOCK_EVENT			0x8
#define SAC_CHANNEL_FLAG_REDRAW_EVENT		0x10

typedef struct _SAC_MEMORY_ENTRY
{
	ULONG Signature;
	ULONG Tag;
	ULONG Size;
} SAC_MEMORY_ENTRY, *PSAC_MEMORY_ENTRY;

typedef struct _SAC_MEMORY_LIST
{
	ULONG Signature;
	PSAC_MEMORY_ENTRY LocalDescriptor;
	ULONG Size;
	struct _SAC_MEMORY_LIST* Next;
} SAC_MEMORY_LIST, *PSAC_MEMORY_LIST;

typedef enum _SAC_CHANNEL_TYPE
{
	VtUtf8,
	Cmd,
	Raw
} SAC_CHANNEL_TYPE;

typedef enum _SAC_CHANNEL_STATUS
{
	Inactive,
	Active
} SAC_CHANNEL_STATUS, *PSAC_CHANNEL_STATUS;

typedef struct _SAC_CHANNEL_ID
{
	GUID ChannelGuid;
	ULONG ChannelId;
} SAC_CHANNEL_ID, *PSAC_CHANNEL_ID;

typedef struct _SAC_CHANNEL_LOCK
{
	LONG RefCount;
	KSEMAPHORE Lock;
} SAC_CHANNEL_LOCK, *PSAC_CHANNEL_LOCK;

struct _SAC_CHANNEL;

typedef
NTSTATUS
(*PSAC_CHANNEL_CREATE)(
	IN struct _SAC_CHANNEL* Channel
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_DESTROY)(
	IN struct _SAC_CHANNEL* Channel
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_OREAD)(
	IN struct _SAC_CHANNEL* Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize,
	OUT PULONG ByteCount
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_OECHO)(
	IN struct _SAC_CHANNEL* Channel,
	IN PWCHAR String,
	IN ULONG Length
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_OFLUSH)(
	IN struct _SAC_CHANNEL* Channel
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_OWRITE)(
	IN struct _SAC_CHANNEL* Channel,
	IN PWCHAR String,
	IN ULONG Length
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_IREAD)(
	IN struct _SAC_CHANNEL* Channel,
	IN PWCHAR Buffer,
	IN ULONG BufferSize,
	IN PULONG ReturnBufferSize
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_IBUFFER_FULL)(
	IN struct _SAC_CHANNEL* Channel,
	OUT PBOOLEAN BufferStatus
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_IBUFFER_LENGTH)(
	IN struct _SAC_CHANNEL* Channel
	);

typedef
CHAR
(*PSAC_CHANNEL_IREAD_LAST)(
	IN struct _SAC_CHANNEL* Channel
	);

typedef
NTSTATUS
(*PSAC_CHANNEL_IWRITE)(
	IN struct _SAC_CHANNEL* Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize
	);

typedef struct _SAC_CHANNEL
{
	ULONG Index;
	SAC_CHANNEL_ID ChannelId;
	HANDLE CloseEvent;
	PVOID CloseEventObjectBody;
	PKEVENT CloseEventWaitObjectBody;
	HANDLE HasNewDataEvent;
	PVOID HasNewDataEventObjectBody;
	PKEVENT HasNewDataEventWaitObjectBody;
	HANDLE LockEvent;
	PVOID LockEventObjectBody;
	PKEVENT LockEventWaitObjectBody;
	HANDLE RedrawEvent;
	PVOID RedrawEventObjectBody;
	PKEVENT RedrawEventWaitObjectBody;
	PFILE_OBJECT FileObject;
	SAC_CHANNEL_TYPE ChannelType;
	SAC_CHANNEL_STATUS ChannelStatus;
	WCHAR NameBuffer[64 + 1];
	WCHAR DescriptionBuffer[256 + 1];
	ULONG Flags;
	GUID ApplicationType;
	BOOLEAN WriteEnabled;
	ULONG IBufferIndex;
	PVOID IBuffer;
	BOOLEAN ChannelHasNewIBufferData;
	UCHAR CursorRow;
	UCHAR CursorCol;
	UCHAR CursorY;
	UCHAR CursorX;
	UCHAR CursorVisible;
	PVOID OBuffer;
	ULONG OBufferIndex;
	ULONG OBufferFirstGoodIndex;
	BOOLEAN ChannelHasNewOBufferData;
	PSAC_CHANNEL_CREATE ChannelCreate;
	PSAC_CHANNEL_DESTROY ChannelDestroy;
	PSAC_CHANNEL_OFLUSH OBufferFlush;
	PSAC_CHANNEL_OECHO OBufferEcho;
	PSAC_CHANNEL_OWRITE OBufferWrite;
	PSAC_CHANNEL_OREAD OBufferRead;
	PSAC_CHANNEL_OWRITE IBufferWrite;
	PSAC_CHANNEL_IREAD IBufferRead;
	PSAC_CHANNEL_IREAD_LAST IBufferReadLast;
	PSAC_CHANNEL_IBUFFER_FULL IBufferIsFull;
	PSAC_CHANNEL_IBUFFER_LENGTH IBufferLength;
	SAC_CHANNEL_LOCK ChannelAttributeLock;
	SAC_CHANNEL_LOCK ChannelOBufferLock;
	SAC_CHANNEL_LOCK ChannelIBufferLock;
} SAC_CHANNEL, *PSAC_CHANNEL;

typedef struct _SAC_DEVICE_EXTENSION
{
	PDEVICE_OBJECT DeviceObject;
	BOOLEAN Initialized;
	BOOLEAN Rundown;
	BOOLEAN PriorityFail;
	KPRIORITY PriorityBoost;
	PEPROCESS Process;
	KSPIN_LOCK Lock;
	KEVENT RundownEvent;
	KEVENT Event;
	HANDLE WorkerThreadHandle;
	KEVENT WorkerThreadEvent;
	KTIMER Timer;
	KDPC Dpc;
	LIST_ENTRY List;
} SAC_DEVICE_EXTENSION, *PSAC_DEVICE_EXTENSION;

typedef struct _SAC_CHANNEL_ATTRIBUTES
{
	SAC_CHANNEL_TYPE ChannelType;
	WCHAR NameBuffer[64 + 1];
	WCHAR DescriptionBuffer[256 + 1];
	ULONG Flag;
	PKEVENT CloseEvent;
	PKEVENT HasNewDataEvent;
	PKEVENT LockEvent;
	PKEVENT RedrawEvent;
	GUID ChannelId;
} SAC_CHANNEL_ATTRIBUTES, *PSAC_CHANNEL_ATTRIBUTES;

NTSTATUS
Dispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
);

NTSTATUS
NTAPI
DispatchDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
);

NTSTATUS
DispatchShutdownControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
);

VOID
UnloadHandler(
	IN PDRIVER_OBJECT DriverObject
);

VOID
FreeGlobalData(
	VOID
);

BOOLEAN
InitializeDeviceData(
	IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
InitializeGlobalData(
	IN PUNICODE_STRING RegistryPath,
	IN PDRIVER_OBJECT DriverObject
);

extern ULONG SACDebug;

