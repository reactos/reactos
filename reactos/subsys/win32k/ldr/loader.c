/* $Id: loader.c,v 1.8 2002/09/07 15:13:11 chorns Exp $
 *
 */

#define NTOS_KERNEL_MODE
#include <ntos.h>
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
