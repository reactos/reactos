/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/main/display.c
 * PURPOSE:         ReactOS display settings support
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>


NTSTATUS
APIENTRY
RosUserEnumDisplaySettings(
   PUNICODE_STRING pusDeviceName,
   DWORD iModeNum,
   LPDEVMODEW lpDevMode,
   DWORD dwFlags )
{
    NTSTATUS Status;
    LPDEVMODEW pSafeDevMode = NULL;
    PUNICODE_STRING pusSafeDeviceName = NULL;
    UNICODE_STRING usSafeDeviceName;
    USHORT ExtraSize = 0;

    _SEH2_TRY
    {
        ProbeForRead(lpDevMode, sizeof(DEVMODEW), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT("FIXME: DEVMODEW size out of range\n");
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (lpDevMode->dmSize != sizeof(DEVMODEW))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pSafeDevMode = ExAllocatePool(PagedPool, sizeof(DEVMODEW) + lpDevMode->dmDriverExtra);

    if (pSafeDevMode == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    pSafeDevMode->dmSize = sizeof(DEVMODEW);
    pSafeDevMode->dmDriverExtra = lpDevMode->dmDriverExtra;

    ExtraSize = pSafeDevMode->dmDriverExtra;

    /* Copy the device name */
    if (pusDeviceName != NULL)
    {
        _SEH2_TRY
        {
            RtlInitUnicodeString(&usSafeDeviceName, pusDeviceName->Buffer);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ExFreePool(pSafeDevMode);
            _SEH2_YIELD(return STATUS_UNSUCCESSFUL);
        }
        _SEH2_END;
        pusSafeDeviceName = &usSafeDeviceName;
    }

    /* Call internal function */
    Status = GreEnumDisplaySettings(pusSafeDeviceName, iModeNum, pSafeDevMode, dwFlags);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(pSafeDevMode);
        return Status;
    }

    /* Copy some information back */
    _SEH2_TRY
    {
        ProbeForWrite(lpDevMode,sizeof(DEVMODEW) + ExtraSize, 1);
        lpDevMode->dmPelsWidth = pSafeDevMode->dmPelsWidth;
        lpDevMode->dmPelsHeight = pSafeDevMode->dmPelsHeight;
        lpDevMode->dmBitsPerPel = pSafeDevMode->dmBitsPerPel;
        lpDevMode->dmDisplayFrequency = pSafeDevMode->dmDisplayFrequency;
        lpDevMode->dmDisplayFlags = pSafeDevMode->dmDisplayFlags;

        /* output private/extra driver data */
        if (ExtraSize > 0)
        {
            memcpy((PCHAR)lpDevMode + sizeof(DEVMODEW), (PCHAR)pSafeDevMode + sizeof(DEVMODEW), ExtraSize);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ExFreePool(pSafeDevMode);
    return Status;
}

LONG
APIENTRY
RosUserChangeDisplaySettings(
   PUNICODE_STRING pusDeviceName,
   LPDEVMODEW lpDevMode,
   HWND hwnd,
   DWORD dwflags,
   LPVOID lParam)
{
    LPDEVMODEW pSafeDevMode = NULL;
    PUNICODE_STRING pusSafeDeviceName = NULL;
    UNICODE_STRING usSafeDeviceName;
    LONG Ret;
    WORD Size;

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
       _SEH2_TRY
    {
        ProbeForRead(lpDevMode, sizeof(DEVMODEW), 1);
        Size = lpDevMode->dmSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT("FIXME: DEVMODEW size out of range\n");
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (Size != sizeof(DEVMODEW))
    {
      SetLastNtError(STATUS_BUFFER_TOO_SMALL);
      return DISP_CHANGE_BADPARAM;
    }

    pSafeDevMode = ExAllocatePool(PagedPool, Size);

    if (pSafeDevMode == NULL)
    {
        return DISP_CHANGE_BADPARAM;
    }

    pSafeDevMode->dmSize = Size;

    if(lpDevMode->dmDriverExtra != 0)
        DPRINT1("ignoring DriverExtra\n");

    pSafeDevMode->dmDriverExtra = 0;

    RtlCopyMemory(pSafeDevMode, lpDevMode, pSafeDevMode->dmSize);

    /* Copy the device name */
    if (pusDeviceName != NULL)
    {
       _SEH2_TRY
        {
            RtlInitUnicodeString(&usSafeDeviceName, pusDeviceName->Buffer);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ExFreePool(pSafeDevMode);
            _SEH2_YIELD(return DISP_CHANGE_BADPARAM);
        }
        _SEH2_END;
        pusSafeDeviceName = &usSafeDeviceName;
    }

    /* Call internal function */
    Ret = GreChangeDisplaySettings(pusSafeDeviceName, pSafeDevMode, dwflags, lParam);

    return Ret;
}
