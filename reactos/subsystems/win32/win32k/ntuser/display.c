/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/display.c
 * PURPOSE:         display settings
 * COPYRIGHT:       Copyright 2007 ReactOS
 *
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
APIENTRY
NtUserEnumDisplaySettings(
   PUNICODE_STRING pusDeviceName,
   DWORD iModeNum,
   LPDEVMODEW lpDevMode, /* FIXME is this correct? */
   DWORD dwFlags )
{
    NTSTATUS Status;
    LPDEVMODEW pSafeDevMode;
    PUNICODE_STRING pusSafeDeviceName = NULL;
    UNICODE_STRING usSafeDeviceName;
    USHORT Size = 0, ExtraSize = 0;

    /* Copy the devmode */
    _SEH_TRY
    {
        ProbeForRead(lpDevMode, sizeof(DEVMODEW), 1);
        Size = lpDevMode->dmSize;
        ExtraSize = lpDevMode->dmDriverExtra;
    }
    _SEH_HANDLE
    {
        DPRINT("FIXME ? : Out of range of DEVMODEW size \n");
        _SEH_YIELD(return _SEH_GetExceptionCode());
    }
    _SEH_END;

    if (Size != sizeof(DEVMODEW))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pSafeDevMode = ExAllocatePool(PagedPool, Size + ExtraSize);
    if (pSafeDevMode == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    pSafeDevMode->dmSize = Size;
    pSafeDevMode->dmDriverExtra = ExtraSize;

    /* Copy the device name */
    if (pusDeviceName != NULL)
    {
        Status = IntSafeCopyUnicodeString(&usSafeDeviceName, pusDeviceName);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(pSafeDevMode);
            return Status;
        }
        pusSafeDeviceName = &usSafeDeviceName;
    }

    /* Call internal function */
    Status = IntEnumDisplaySettings(pusSafeDeviceName, iModeNum, pSafeDevMode, dwFlags);

    if (pusSafeDeviceName != NULL)
        RtlFreeUnicodeString(pusSafeDeviceName);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(pSafeDevMode);
        return Status;
    }

    /* Copy some information back */
    _SEH_TRY
    {
        ProbeForWrite(lpDevMode,Size + ExtraSize, 1);
        lpDevMode->dmPelsWidth = pSafeDevMode->dmPelsWidth;
        lpDevMode->dmPelsHeight = pSafeDevMode->dmPelsHeight;
        lpDevMode->dmBitsPerPel = pSafeDevMode->dmBitsPerPel;
        lpDevMode->dmDisplayFrequency = pSafeDevMode->dmDisplayFrequency;
        lpDevMode->dmDisplayFlags = pSafeDevMode->dmDisplayFlags;

        /* output private/extra driver data */
        if (ExtraSize > 0)
        {
            memcpy((PCHAR)lpDevMode + Size, (PCHAR)pSafeDevMode + Size, ExtraSize);
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    ExFreePool(pSafeDevMode);
    return Status;
}


LONG
APIENTRY
NtUserChangeDisplaySettings(
   PUNICODE_STRING lpszDeviceName,
   LPDEVMODEW lpDevMode,
   HWND hwnd,
   DWORD dwflags,
   LPVOID lParam)
{
   NTSTATUS Status;
   DEVMODEW DevMode;
   PUNICODE_STRING pSafeDeviceName = NULL;
   UNICODE_STRING SafeDeviceName;
   LONG Ret;

   /* Check arguments */
#ifdef CDS_VIDEOPARAMETERS

   if (dwflags != CDS_VIDEOPARAMETERS && lParam != NULL)
#else

   if (lParam != NULL)
#endif

   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return DISP_CHANGE_BADPARAM;
   }
   if (hwnd != NULL)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return DISP_CHANGE_BADPARAM;
   }

   /* Copy devmode */
   Status = MmCopyFromCaller(&DevMode.dmSize, &lpDevMode->dmSize, sizeof (DevMode.dmSize));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return DISP_CHANGE_BADPARAM;
   }
   DevMode.dmSize = min(sizeof (DevMode), DevMode.dmSize);
   Status = MmCopyFromCaller(&DevMode, lpDevMode, DevMode.dmSize);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return DISP_CHANGE_BADPARAM;
   }
   if (DevMode.dmDriverExtra > 0)
   {
      DbgPrint("(%s:%i) WIN32K: %s lpDevMode->dmDriverExtra is IGNORED!\n", __FILE__, __LINE__, __FUNCTION__);
      DevMode.dmDriverExtra = 0;
   }

   /* Copy the device name */
   if (lpszDeviceName != NULL)
   {
      Status = IntSafeCopyUnicodeString(&SafeDeviceName, lpszDeviceName);
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         return DISP_CHANGE_BADPARAM;
      }
      pSafeDeviceName = &SafeDeviceName;
   }

   /* Call internal function */
   Ret = IntChangeDisplaySettings(pSafeDeviceName, &DevMode, dwflags, lParam);

   if (pSafeDeviceName != NULL)
      RtlFreeUnicodeString(pSafeDeviceName);

   return Ret;
}

