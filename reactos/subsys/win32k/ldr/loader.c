/* $Id: loader.c,v 1.9 2002/09/08 10:23:50 chorns Exp $
 *
 */

#include <ddk/ntddk.h>
#include <ddk/winddi.h>

HANDLE
STDCALL
EngLoadImage (LPWSTR DriverName)
{
  SYSTEM_LOAD_IMAGE GdiDriverInfo;
  NTSTATUS Status;

  RtlInitUnicodeString(&GdiDriverInfo.ModuleName, DriverName);
  Status = ZwSetSystemInformation(SystemLoadImage, &GdiDriverInfo, sizeof(SYSTEM_LOAD_IMAGE));
  if (!NT_SUCCESS(Status)) return NULL;

  return (HANDLE)GdiDriverInfo.ModuleBase;
}


HANDLE
STDCALL
EngLoadModule(LPWSTR ModuleName)
{
  SYSTEM_LOAD_IMAGE GdiDriverInfo;
  NTSTATUS Status;

  // FIXME: should load as readonly

  RtlInitUnicodeString (&GdiDriverInfo.ModuleName, ModuleName);
  Status = ZwSetSystemInformation (SystemLoadImage, &GdiDriverInfo, sizeof(SYSTEM_LOAD_IMAGE));
  if (!NT_SUCCESS(Status)) return NULL;

  return (HANDLE)GdiDriverInfo.ModuleBase;
}

/* EOF */
