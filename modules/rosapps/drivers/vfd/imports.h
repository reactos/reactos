/*
	imports.h

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: imported elements from various sources

	Copyright (C) 2003-2005 Ken Kato

	This file contains:

	a)	#include directive for system headers

	b)	Stuff imported from newer DDKs so that the driver built with older
		DDKs can run on newer Windows.

	c)	Stuff imported from ntifs.h (http://www.acc.umu.se/~bosse/) so that
		the driver can be compiled without it.

	d)	Prototypes of standard functions which are exported from ntoskrnl.exe
		but not declared in regular DDK header files.
*/

#ifndef	_IMPORTS_H_
#define _IMPORTS_H_

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(push,3)
#endif
#include <ntddk.h>
#include <ntdddisk.h>
#include <ntverp.h>
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(pop)

// disable unwanted (and trivial) warnings :
//	4054 - type cast from a function pointer to a data pointer
//	4201 - anonymous structure
//	4514 - unreferenced inline function
#pragma warning(disable: 4054 4201 4514)
#endif

#if (VER_PRODUCTBUILD >= 2195)
#include <mountdev.h>
#else	// (VER_PRODUCTBUILD < 2195)
//
// Imports from Windows 2000 DDK <ntddk.h>
//
typedef enum _MM_PAGE_PRIORITY {
	LowPagePriority		= 0,
	NormalPagePriority	= 16,
	HighPagePriority	= 32
} MM_PAGE_PRIORITY;

#define FILE_ATTRIBUTE_ENCRYPTED			0x00004000

#define FILE_DEVICE_MASS_STORAGE			0x0000002d

//
//	Imports from Windows 2000 DDK <ntddstor.h>
//
#define IOCTL_STORAGE_CHECK_VERIFY2			CTL_CODE(				\
												IOCTL_STORAGE_BASE, \
												0x0200,				\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

//
//	Imports from Windows 2000 DDK <mountmgr.h>, <mountdev.h>
//
#define MOUNTMGR_DEVICE_NAME				L"\\Device\\MountPointManager"
#define MOUNTMGRCONTROLTYPE					((ULONG) 'm')
#define MOUNTDEVCONTROLTYPE					((ULONG) 'M')

#define IOCTL_MOUNTDEV_QUERY_UNIQUE_ID		CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												0,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY						\
											CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												1,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME	CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												2,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME					\
											CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												3,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_MOUNTDEV_LINK_CREATED			CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												4,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_MOUNTDEV_LINK_DELETED			CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												5,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_MOUNTMGR_CREATE_POINT			CTL_CODE(				\
												MOUNTMGRCONTROLTYPE,\
												0,					\
												METHOD_BUFFERED,	\
												FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_MOUNTMGR_DELETE_POINTS		CTL_CODE(				\
												MOUNTMGRCONTROLTYPE,\
												1,					\
												METHOD_BUFFERED,	\
												FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION					\
											CTL_CODE(				\
												MOUNTMGRCONTROLTYPE,\
												11,					\
												METHOD_BUFFERED,	\
												FILE_READ_ACCESS)

typedef struct _MOUNTDEV_UNIQUE_ID {
	USHORT	UniqueIdLength;
	UCHAR	UniqueId[1];
} MOUNTDEV_UNIQUE_ID, *PMOUNTDEV_UNIQUE_ID;

typedef struct _MOUNTDEV_NAME {
	USHORT	NameLength;
	WCHAR	Name[1];
} MOUNTDEV_NAME, *PMOUNTDEV_NAME;

typedef struct _MOUNTDEV_SUGGESTED_LINK_NAME {
	BOOLEAN UseOnlyIfThereAreNoOtherLinks;
	USHORT	NameLength;
	WCHAR	Name[1];
} MOUNTDEV_SUGGESTED_LINK_NAME, *PMOUNTDEV_SUGGESTED_LINK_NAME;

typedef struct _MOUNTMGR_TARGET_NAME {
	USHORT	DeviceNameLength;
	WCHAR	DeviceName[1];
} MOUNTMGR_TARGET_NAME, *PMOUNTMGR_TARGET_NAME;

typedef struct _MOUNTMGR_CREATE_POINT_INPUT {
	USHORT	SymbolicLinkNameOffset;
	USHORT	SymbolicLinkNameLength;
	USHORT	DeviceNameOffset;
	USHORT	DeviceNameLength;
} MOUNTMGR_CREATE_POINT_INPUT, *PMOUNTMGR_CREATE_POINT_INPUT;

typedef struct _MOUNTMGR_MOUNT_POINT {
	ULONG	SymbolicLinkNameOffset;
	USHORT	SymbolicLinkNameLength;
	ULONG	UniqueIdOffset;
	USHORT	UniqueIdLength;
	ULONG	DeviceNameOffset;
	USHORT	DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS {
	ULONG					Size;
	ULONG					NumberOfMountPoints;
	MOUNTMGR_MOUNT_POINT	MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

#endif	// (VER_PRODUCTBUILD < 2195)

// __REACTOS__: NTAPI added on some functions in this file, vfddrv.h and some *.c.

#if (VER_PRODUCTBUILD < 2600)
//
// Imports from Windows XP DDK <ntdddisk.h>
//
#define IOCTL_DISK_GET_PARTITION_INFO_EX	CTL_CODE(				\
												IOCTL_DISK_BASE,	\
												0x0012,				\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

#define IOCTL_DISK_GET_LENGTH_INFO			CTL_CODE(				\
												IOCTL_DISK_BASE,	\
												0x0017,				\
												METHOD_BUFFERED,	\
												FILE_READ_ACCESS)

typedef unsigned __int64 ULONG64, *PULONG64;

typedef enum _PARTITION_STYLE {
	PARTITION_STYLE_MBR,
	PARTITION_STYLE_GPT
} PARTITION_STYLE;

typedef struct _PARTITION_INFORMATION_MBR {
	UCHAR	PartitionType;
	BOOLEAN	BootIndicator;
	BOOLEAN	RecognizedPartition;
	ULONG	HiddenSectors;
} PARTITION_INFORMATION_MBR, *PPARTITION_INFORMATION_MBR;

typedef struct _PARTITION_INFORMATION_GPT {
	GUID	PartitionType;
	GUID	PartitionId;
	ULONG64	Attributes;
	WCHAR	Name[36];
} PARTITION_INFORMATION_GPT, *PPARTITION_INFORMATION_GPT;

typedef struct _PARTITION_INFORMATION_EX {
	PARTITION_STYLE PartitionStyle;
	LARGE_INTEGER	StartingOffset;
	LARGE_INTEGER	PartitionLength;
	ULONG			PartitionNumber;
	BOOLEAN			RewritePartition;
	union {
		PARTITION_INFORMATION_MBR Mbr;
		PARTITION_INFORMATION_GPT Gpt;
	};
} PARTITION_INFORMATION_EX, *PPARTITION_INFORMATION_EX;

typedef struct _GET_LENGTH_INFORMATION {
	LARGE_INTEGER	Length;
} GET_LENGTH_INFORMATION, *PGET_LENGTH_INFORMATION;

//
// Imports from Windows XP DDK <ntddstor.h>
//
#define IOCTL_STORAGE_GET_HOTPLUG_INFO		CTL_CODE(				\
												IOCTL_STORAGE_BASE,	\
												0x0305,				\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

typedef struct _STORAGE_HOTPLUG_INFO {
	ULONG 	Size;
	BOOLEAN	MediaRemovable;
	BOOLEAN	MediaHotplug;
	BOOLEAN	DeviceHotplug;
	BOOLEAN	WriteCacheEnableOverride;
} STORAGE_HOTPLUG_INFO, *PSTORAGE_HOTPLUG_INFO;

//
// Imports from Windows XP DDK <mountdev.h>
//
#define IOCTL_MOUNTDEV_QUERY_STABLE_GUID	CTL_CODE(				\
												MOUNTDEVCONTROLTYPE,\
												6,					\
												METHOD_BUFFERED,	\
												FILE_ANY_ACCESS)

typedef struct _MOUNTDEV_STABLE_GUID {
	GUID	StableGuid;
} MOUNTDEV_STABLE_GUID, *PMOUNTDEV_STABLE_GUID;

#endif // (VER_PRODUCTBUILD < 2600)

//
// Imports from ntifs.h
//
#define TOKEN_SOURCE_LENGTH 8

typedef enum _TOKEN_TYPE {
	TokenPrimary = 1,
	TokenImpersonation
} TOKEN_TYPE;

typedef struct _TOKEN_SOURCE {
	CCHAR	SourceName[TOKEN_SOURCE_LENGTH];
	LUID	SourceIdentifier;
} TOKEN_SOURCE, *PTOKEN_SOURCE;

typedef struct _TOKEN_CONTROL {
	LUID			TokenId;
	LUID			AuthenticationId;
	LUID			ModifiedId;
	TOKEN_SOURCE	TokenSource;
} TOKEN_CONTROL, *PTOKEN_CONTROL;

typedef struct _SECURITY_CLIENT_CONTEXT {
	SECURITY_QUALITY_OF_SERVICE	SecurityQos;
	PACCESS_TOKEN				ClientToken;
	BOOLEAN						DirectlyAccessClientToken;
	BOOLEAN						DirectAccessEffectiveOnly;
	BOOLEAN						ServerIsRemote;
	TOKEN_CONTROL				ClientTokenControl;
} SECURITY_CLIENT_CONTEXT, *PSECURITY_CLIENT_CONTEXT;

#define PsDereferenceImpersonationToken(T)	\
	if (ARGUMENT_PRESENT(T)) (ObDereferenceObject((T)))

#define PsDereferencePrimaryToken(T) (ObDereferenceObject((T)))

NTKERNELAPI
VOID
NTAPI
PsRevertToSelf (
	VOID
);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurity (
	IN PETHREAD						Thread,
	IN PSECURITY_QUALITY_OF_SERVICE	QualityOfService,
	IN BOOLEAN						RemoteClient,
	OUT PSECURITY_CLIENT_CONTEXT	ClientContext
);

#define SeDeleteClientSecurity(C)							\
{															\
	if (SeTokenType((C)->ClientToken) == TokenPrimary) {	\
		PsDereferencePrimaryToken((C)->ClientToken);		\
	}														\
	else {													\
		PsDereferenceImpersonationToken((C)->ClientToken);	\
	}														\
}

NTKERNELAPI
VOID
NTAPI
SeImpersonateClient (
	IN PSECURITY_CLIENT_CONTEXT	ClientContext,
	IN PETHREAD					ServerThread OPTIONAL
);

NTKERNELAPI
TOKEN_TYPE
NTAPI
SeTokenType (
	IN PACCESS_TOKEN Token
);

//
// Functions exported by ntoskrnl.exe, but not declared in DDK headers
//
int _snprintf(char *buffer, size_t count, const char *format, ...);
int _snwprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...);
int sprintf(char *buffer, const char *format, ...);
int _swprintf(wchar_t *buffer, const wchar_t *format, ...);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _IMPORTS_H_
