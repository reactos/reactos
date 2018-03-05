/*
	vfddrv.h

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: local header

	Copyright(C) 2003-2005 Ken Kato
*/

#ifndef _VFDDRV_H_
#define _VFDDRV_H_

#ifdef __cplusplus
extern "C" {
#pragma message("Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#include "vfdtypes.h"
#include "vfdio.h"
#include "vfdver.h"

//
//	Tag used for ExAllocatePoolWithTag
//
#define VFD_POOL_TAG			'DFVx'

//
//	PnP driver specific stuff
//
#ifdef	VFD_PNP

#if	(VER_PRODUCTBUILD < 2195)
#error Cannot build a PnP version with the Windows NT DDK
#endif	//	(VER_PRODUCTBUILD < 2195)

//
//	device state enumeration
//
typedef enum  _DEVICE_STATE
{
	VFD_STOPPED,				// Dvice stopped
	VFD_WORKING,				// Started and working
	VFD_PENDINGSTOP,			// Stop pending
	VFD_PENDINGREMOVE,			// Remove pending
	VFD_SURPRISEREMOVED,		// Surprise removed
	VFD_REMOVED,				// Removed
	VFD_MAX_STATE				// Unknown state -Some error
}
DEVICE_STATE, *PDEVICE_STATE;

//
//	use the address of the DriverEntry functions as the
//	driver extension identifier
//
#define VFD_DRIVER_EXTENSION_ID	((PVOID)DriverEntry)

#endif	// VFD_PNP

//
//	driver extension for the VFD driver
//
typedef struct _VFD_DRIVER_EXTENSION
{
	UNICODE_STRING				RegistryPath;
	ULONG						NumberOfDevices;
}
VFD_DRIVER_EXTENSION, *PVFD_DRIVER_EXTENSION;

//
//	device extension for Virtual FD device
//
typedef struct _DEVICE_EXTENSION
{
	//	back pointer to the device object
	PDEVICE_OBJECT				DeviceObject;

	//	device information
	UNICODE_STRING				DeviceName;		// \Device\Floppy<n>
	ULONG						DeviceNumber;	// \??\VirtualFD<n>
	CHAR						DriveLetter;	// \DosDevices\<x>:

	//	Security context to access files on network drive
	PSECURITY_CLIENT_CONTEXT	SecurityContext;

	//	IRP queue list
	LIST_ENTRY					ListHead;
	KSPIN_LOCK					ListLock;

	//	device thread
	KEVENT						RequestEvent;
	PVOID						ThreadPointer;
	BOOLEAN						TerminateThread;

	//	drive information
	ULONG						MediaChangeCount;

	//	media information
	VFD_MEDIA					MediaType;
	VFD_FLAGS					MediaFlags;
	VFD_FILETYPE				FileType;
	ULONG						ImageSize;
	ANSI_STRING					FileName;

	const DISK_GEOMETRY			*Geometry;
	ULONG						Sectors;

	HANDLE						FileHandle;
	PUCHAR						FileBuffer;

#ifdef VFD_PNP
	DEVICE_STATE				DeviceState;		// Current device state
	IO_REMOVE_LOCK				RemoveLock;			// avoid abnormal removal
	PDEVICE_OBJECT				PhysicalDevice;
	PDEVICE_OBJECT				TargetDevice;
	UNICODE_STRING				InterfaceName;
#else	// VFD_PNP
	PVFD_DRIVER_EXTENSION		DriverExtension;
#endif	// VFD_PNP
}
DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
//	Stanard driver routines
//
NTSTATUS
NTAPI
DriverEntry(
	IN	PDRIVER_OBJECT			DriverObject,
	IN	PUNICODE_STRING			RegistryPath);

VOID
NTAPI
VfdUnloadDriver(
	IN	PDRIVER_OBJECT			DriverObject);

NTSTATUS
NTAPI
VfdCreateClose(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP					Irp);

NTSTATUS
NTAPI
VfdReadWrite(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP					Irp);

NTSTATUS
NTAPI
VfdDeviceControl(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP					Irp);

#ifdef VFD_PNP

NTSTATUS
NTAPI
VfdPlugAndPlay(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP					Irp);

NTSTATUS
NTAPI
VfdPowerControl(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP					Irp);

NTSTATUS
NTAPI
VfdSystemControl(
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	PIRP					Irp);

NTSTATUS
NTAPI
VfdAddDevice(
	IN		PDRIVER_OBJECT		DriverObject,
	IN OUT	PDEVICE_OBJECT		PhysicalDevice);

#endif	// VFD_PNP

//
//	Prototypes for private routines
//

//
//	vfddrv.c
//
extern ULONG OsMajorVersion;
extern ULONG OsMinorVersion;
extern ULONG OsBuildNumber;

VOID
NTAPI
VfdDeviceThread(
	IN	PVOID					ThreadContext);

PWSTR
VfdCopyUnicode(
	OUT	PUNICODE_STRING			dst,
	IN	PUNICODE_STRING			src);

VOID
VfdFreeUnicode(
	IN OUT	PUNICODE_STRING		str);

//
//	vfddev.c
//
NTSTATUS
VfdCreateDevice(
	IN	PDRIVER_OBJECT			DriverObject,
	OUT	PVOID					Parameter);

VOID
VfdDeleteDevice(
	IN	PDEVICE_OBJECT			DeviceObject);

//
//	vfdioctl.c
//
VOID
VfdIoCtlThread(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PIRP					Irp,
	IN	ULONG					ControlCode);

//
//	vfdimg.c
//
NTSTATUS
VfdOpenCheck(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PVFD_IMAGE_INFO			ImageInfo,
	IN	ULONG					InputLength);

NTSTATUS
VfdOpenImage(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PVFD_IMAGE_INFO			ImageInfo);

VOID
VfdCloseImage(
	IN	PDEVICE_EXTENSION		DeviceExtension);

NTSTATUS
VfdQueryImage(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	OUT	PVFD_IMAGE_INFO			ImageInfo,
	IN	ULONG					BufferLength,
#ifndef __REACTOS__
	OUT	PULONG					ReturnLength);
#else
	OUT	PSIZE_T					ReturnLength);
#endif

//
//	vfdrdwr.c
//
VOID
VfdReadData(
	IN		PDEVICE_EXTENSION	DeviceExtension,
	IN OUT	PIRP				Irp,
	IN		ULONG				Length,
	IN		PLARGE_INTEGER		Offset);

VOID
VfdWriteData(
	IN		PDEVICE_EXTENSION	DeviceExtension,
	IN OUT	PIRP				Irp,
	IN		ULONG				Length,
	IN		PLARGE_INTEGER		Offset);

//
//	vfdlink.c
//
NTSTATUS
VfdSetLink(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	CHAR					DriveLetter);

NTSTATUS
VfdStoreLink(
	IN	PDEVICE_EXTENSION		DeviceExtension);

NTSTATUS
VfdLoadLink(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PWSTR					RegistryPath);

//
//	vfdfmt.c
//
extern const DISK_GEOMETRY geom_tbl[VFD_MEDIA_MAX];

NTSTATUS
VfdFormatCheck(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PFORMAT_PARAMETERS		FormatParams,
	IN	ULONG					InputLength,
	IN	ULONG					ControlCode);

NTSTATUS
VfdFormatTrack(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PFORMAT_PARAMETERS		FormatParams);

//
//	vfdmnt.c
//
#ifdef VFD_MOUNT_MANAGER
/*
NTSTATUS
VfdRegisterMountManager(
	IN	PDEVICE_EXTENSION		DeviceExtension);
*/

NTSTATUS
VfdMountMgrNotifyVolume(
	IN	PDEVICE_EXTENSION		DeviceExtension);

NTSTATUS
VfdMountMgrMountPoint(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	CHAR					DriveLetter);

NTSTATUS
VfdMountDevUniqueId(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	OUT	PMOUNTDEV_UNIQUE_ID		UniqueId,
	IN	ULONG					OutputLength,
	OUT	PIO_STATUS_BLOCK		IoStatus);

NTSTATUS
VfdMountDevDeviceName(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	OUT	PMOUNTDEV_NAME			DeviceName,
	IN	ULONG					OutputLength,
	OUT	PIO_STATUS_BLOCK		IoStatus);

NTSTATUS
VfdMountDevSuggestedLink(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	OUT	PMOUNTDEV_SUGGESTED_LINK_NAME	LinkName,
	IN	ULONG					OutputLength,
	OUT	PIO_STATUS_BLOCK		IoStatus);

NTSTATUS
VfdMountDevLinkModified(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PMOUNTDEV_NAME			LinkName,
	IN	ULONG					InputLength,
	IN	ULONG					ControlCode);

#endif	//	VFD_MOUNT_MANAGER

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _VFDDRV_H_
