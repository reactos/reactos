/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/kmrtint/kmrtint.c
 * PURPOSE:         Kernel-mode regression testing driver user-mode interface
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"
#include "kmregtests.h"

#define NDEBUG
#include <debug.h>

HANDLE
OpenDevice()
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  IO_STATUS_BLOCK Iosb;
  HANDLE DeviceHandle;
  NTSTATUS Status;

  RtlInitUnicodeString(&DeviceName,
    L"\\Device\\KMRegTests");
	InitializeObjectAttributes(
    &ObjectAttributes,
    &DeviceName,
    0,
    NULL,
    NULL);

  Status = NtCreateFile(
    &DeviceHandle,
    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
    &ObjectAttributes,
    &Iosb,
    NULL,
		0,
		0,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
    NULL,
    0);

  if (!NT_SUCCESS(Status))
    {
      return INVALID_HANDLE_VALUE;
    }
  return DeviceHandle;
}

VOID STDCALL
RegTestMain()
{
  IO_STATUS_BLOCK Iosb;
  HANDLE DeviceHandle;
  NTSTATUS Status;

  DeviceHandle = OpenDevice();
  if (DeviceHandle != INVALID_HANDLE_VALUE)
    {
      Status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
    		NULL,
    		NULL,
    		&Iosb,
    		IOCTL_KMREGTESTS_RUN,
    		NULL,
    		0,
    		NULL,
    		0);
      if (Status == STATUS_PENDING) {
     		Status = NtWaitForSingleObject(DeviceHandle, FALSE, NULL);
      }

      NtClose(DeviceHandle);
    }
  else
    {
      DPRINT("Cannot open KMRegTests device.\n");
    }
}
