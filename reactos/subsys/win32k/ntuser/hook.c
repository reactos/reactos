/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window hooks
 * FILE:             subsys/win32k/ntuser/hook.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <ddk/ntddk.h>
#include <win32k/win32k.h>

#define NDEBUG
#include <debug.h>

DWORD
STDCALL
NtUserCallNextHookEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetWindowsHookAW(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetWindowsHookEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetWinEventHook(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUnhookWindowsHookEx(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUnhookWinEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
