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
	KSEMAPHORE Semaphore;
} SAC_CHANNEL_LOCK, *PSAC_CHANNEL_LOCK;

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
	//PSAC_CHANNEL_CREATE ChannelCreate;
	//PSAC_CHANNEL_DESTROY ChannelDestroy;
	//PSAC_CHANNEL_OFLUSH ChannelOutputFlush;
	//PSAC_CHANNEL_OECHO ChannelOutputEcho;
	//PSAC_CHANNEL_OWRITE ChannelOutputWrite;
	//PSAC_CHANNEL_OREAD ChannelOutputRead;
	//PSAC_CHANNEL_OWRITE ChannelInputWrite;
	//PSAC_CHANNEL_IREAD ChannelInputRead;
	//PSAC_CHANNEL_IREAD_LAST ChannelInputReadLast;
	//PSAC_CHANNEL_IBUFFER_FULL ChannelInputBufferIsFull;
	//PSAC_CHANNEL_IBUFFER_LENGTH IBufferLength;
	SAC_CHANNEL_LOCK ChannelAttributeLock;
	SAC_CHANNEL_LOCK ChannelOBufferLock;
	SAC_CHANNEL_LOCK ChannelBufferLock;
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

