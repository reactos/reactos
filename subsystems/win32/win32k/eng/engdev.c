/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engdev.c
 * PURPOSE:         Device Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
EngDeviceIoControl(
  IN HANDLE  hDevice,
  IN DWORD  dwIoControlCode,
  IN PVOID  lpInBuffer,
  IN DWORD  nInBufferSize,
  IN OUT PVOID  lpOutBuffer,
  IN DWORD  nOutBufferSize,
  OUT PDWORD  lpBytesReturned)
{
    UNIMPLEMENTED;
	return 0;
}

ULONG
APIENTRY
EngHangNotification(IN HDEV hDev,
                    IN PVOID Reserved)
{
    UNIMPLEMENTED;
	return EHN_ERROR;
}

BOOL
APIENTRY
EngQueryDeviceAttribute(
  IN HDEV  hdev,
  IN ENG_DEVICE_ATTRIBUTE  devAttr,
  IN VOID  *pvIn,
  IN ULONG  ulInSize,
  OUT VOID  *pvOut,
  OUT ULONG  ulOutSize)
{
    UNIMPLEMENTED;
	return FALSE;
}
