/* $Id: class2.h,v 1.1 2001/07/23 06:12:34 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/include/class2.h
 * PURPOSE:         SCSI class driver definitions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#ifndef __STORAGE_INCLUDE_CLASS2_H
#define __STORAGE_INCLUDE_CLASS2_H

#include "ntddscsi.h"
#include "srb.h"

struct _CLASS_INIT_DATA;

typedef VOID STDCALL
(*PCLASS_ERROR)(IN PDEVICE_OBJECT DeviceObject,
		IN PSCSI_REQUEST_BLOCK Srb,
		IN OUT NTSTATUS *Status,
		IN OUT BOOLEAN *Retry);

typedef BOOLEAN STDCALL
(*PCLASS_DEVICE_CALLBACK)(IN PINQUIRYDATA);

typedef NTSTATUS STDCALL
(*PCLASS_READ_WRITE)(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

typedef BOOLEAN STDCALL
(*PCLASS_FIND_DEVICES)(IN PDRIVER_OBJECT DriverObject,
		       IN PUNICODE_STRING RegistryPath,
		       IN struct _CLASS_INIT_DATA *InitializationData,
		       IN PDEVICE_OBJECT PortDeviceObject,
		       IN ULONG PortNumber);

typedef NTSTATUS STDCALL
(*PCLASS_DEVICE_CONTROL)(IN PDEVICE_OBJECT DeviceObject,
			 IN PIRP Irp);

typedef NTSTATUS STDCALL
(*PCLASS_SHUTDOWN_FLUSH)(IN PDEVICE_OBJECT DeviceObject,
			 IN PIRP Irp);

typedef NTSTATUS STDCALL
(*PCLASS_CREATE_CLOSE)(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);


typedef struct _CLASS_INIT_DATA
{
  ULONG InitializationDataSize;
  ULONG DeviceExtensionSize;
  DEVICE_TYPE DeviceType;
  ULONG DeviceCharacteristics;
  PCLASS_ERROR ClassError;
  PCLASS_READ_WRITE ClassReadWriteVerification;
  PCLASS_DEVICE_CALLBACK ClassFindDeviceCallBack;
  PCLASS_FIND_DEVICES ClassFindDevices;
  PCLASS_DEVICE_CONTROL ClassDeviceControl;
  PCLASS_SHUTDOWN_FLUSH ClassShutdownFlush;
  PCLASS_CREATE_CLOSE ClassCreateClose;
  PDRIVER_STARTIO ClassStartIo;
} CLASS_INIT_DATA, *PCLASS_INIT_DATA;


typedef struct _DEVICE_EXTENSION
{
  ULONG Dummy;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* FUNCTIONS ****************************************************************/

NTSTATUS
STDCALL
ScsiClassAsynchronousCompletion (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP		Irp,
	PVOID		Context
	);

VOID
STDCALL
ScsiClassBuildRequest (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP		Irp
	);

NTSTATUS
STDCALL
ScsiClassClaimDevice (
	IN	PDEVICE_OBJECT		PortDeviceObject,
	IN	PSCSI_INQUIRY_DATA	LunInfo,
	IN	BOOLEAN			Release,
	OUT	PDEVICE_OBJECT		*NewPortDeviceObject OPTIONAL
	);

NTSTATUS
STDCALL
ScsiClassCreateDeviceObject (
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PCCHAR			ObjectNameBuffer,
	IN	PDEVICE_OBJECT		PhysicalDeviceObject OPTIONAL,
	IN OUT	PDEVICE_OBJECT		*DeviceObject,
	IN	PCLASS_INIT_DATA	InitializationData
	);

NTSTATUS
STDCALL
ScsiClassDeviceControl (
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
	);




#endif /* __STORAGE_INCLUDE_CLASS2_H */

/* EOF */