#pragma once

#include "windef.h"
#include "usb100.h"

#if !defined(_USBRPM_DRIVER_)
#define USBRPMAPI DECLSPEC_IMPORT
#else
#define USBRPMAPI
#endif

typedef struct _USBRPM_DEVICE_INFORMATION {
  ULONG64 HubId;
  ULONG ConnectionIndex;
  UCHAR DeviceClass;
  USHORT VendorId;
  USHORT ProductId;
  WCHAR ManufacturerString[MAXIMUM_USB_STRING_LENGTH];
  WCHAR ProductString[MAXIMUM_USB_STRING_LENGTH];
  WCHAR HubSymbolicLinkName[MAX_PATH];
} USBRPM_DEVICE_INFORMATION, *PUSBRPM_DEVICE_INFORMATION;

typedef struct _USBRPM_DEVICE_LIST {
  ULONG NumberOfDevices;
  USBRPM_DEVICE_INFORMATION Device[0];
} USBRPM_DEVICE_LIST, *PUSBRPM_DEVICE_LIST;

USBRPMAPI
NTSTATUS
NTAPI
RPMRegisterAlternateDriver(
  IN PDRIVER_OBJECT  DriverObject,
  IN LPCWSTR CompatibleId, 
  OUT PHANDLE RegisteredDriver);

USBRPMAPI
NTSTATUS
NTAPI
RPMUnregisterAlternateDriver(
  IN HANDLE RegisteredDriver);

USBRPMAPI
NTSTATUS
RPMGetAvailableDevices(
  IN HANDLE RegisteredDriver,
  IN USHORT Locale,
  OUT PUSBRPM_DEVICE_LIST *DeviceList);

USBRPMAPI
NTSTATUS
NTAPI
RPMLoadAlternateDriverForDevice(
  IN HANDLE RegisteredDriver,
  IN ULONG64 HubID,
  IN ULONG ConnectionIndex,
  IN OPTIONAL REFGUID OwnerGuid);

USBRPMAPI
NTSTATUS
NTAPI
RPMUnloadAlternateDriverForDevice(
  IN HANDLE RegisteredDriver,
  IN ULONG64 HubID,
  IN ULONG ConnectionIndex);
