/* $Id: loader.c,v 1.7 2002/06/14 07:47:40 ekohl Exp $
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
