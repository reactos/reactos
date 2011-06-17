/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/sacdrv.h
 * PURPOSE:		 Header for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/
#include <ntddk.h>

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
