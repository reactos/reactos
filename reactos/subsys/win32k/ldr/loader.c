/* $Id: loader.c,v 1.6 2001/03/31 15:35:07 jfilby Exp $
 *
 */

#include <ddk/ntddk.h>
#include <ddk/winddi.h>

HANDLE
STDCALL
EngLoadImage (LPWSTR DriverName)
{
  SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
  NTSTATUS Status;

  RtlInitUnicodeString(&GdiDriverInfo.DriverName, DriverName);
  Status = ZwSetSystemInformation (SystemLoadGdiDriverInformation, &GdiDriverInfo, sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
  if (!NT_SUCCESS(Status)) return NULL;

  return (HANDLE)GdiDriverInfo.ImageAddress;
}


HANDLE
STDCALL
EngLoadModule(LPWSTR ModuleName)
{
  SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
  NTSTATUS Status;

  // FIXME: should load as readonly

  RtlInitUnicodeString (&GdiDriverInfo.DriverName, ModuleName);
  Status = ZwSetSystemInformation (SystemLoadGdiDriverInformation, &GdiDriverInfo, sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
  if (!NT_SUCCESS(Status)) return NULL;

  return (HANDLE)GdiDriverInfo.ImageAddress;
}

/* EOF */
