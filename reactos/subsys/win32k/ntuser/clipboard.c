/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Clipboard routines
 * FILE:             subsys/win32k/ntuser/clipboard.c
 * PROGRAMER:        Filip Navara <xnavara@volny.cz>
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#if 0
PW32THREAD ClipboardThread;
HWND ClipboardWindow;
#endif

ULONG FASTCALL
IntGetClipboardFormatName(UINT format, PUNICODE_STRING FormatName)
{
   return IntGetAtomName((RTL_ATOM)format, FormatName->Buffer,
      FormatName->MaximumLength);
}

UINT FASTCALL
IntEnumClipboardFormats(UINT format)
{
   UNIMPLEMENTED;
   return 0;
}

BOOL STDCALL
NtUserOpenClipboard(HWND hWnd, DWORD Unknown1)
{
#if 0
   if (ClipboardThread && ClipboardThread != PsGetWin32Thread())
   {
      SetLastWin32Error(ERROR_LOCKED);
      return FALSE;
   }
   ClipboardWindow = hWnd;
   ClipboardThread = PsGetWin32Thread();
   return TRUE;
#else
   DPRINT1("NtUserOpenClipboard is unimplemented\n");
   return FALSE;
#endif
}

BOOL STDCALL
NtUserCloseClipboard(VOID)
{
#if 0
   if (ClipboardThread && ClipboardThread != PsGetWin32Thread())
   {
      SetLastWin32Error(ERROR_LOCKED);
      return FALSE;
   }
   ClipboardWindow = 0;
   ClipboardThread = NULL;
   return TRUE;
#else
   UNIMPLEMENTED
   return FALSE;
#endif
}

/*
 * @unimplemented
 */
HWND STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
   UNIMPLEMENTED
   return 0;
}

BOOL STDCALL
NtUserChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext)
{
   UNIMPLEMENTED
   return 0;
}

DWORD STDCALL
NtUserCountClipboardFormats(VOID)
{
   UNIMPLEMENTED
   return 0;
}

DWORD STDCALL
NtUserEmptyClipboard(VOID)
{
   UNIMPLEMENTED
   return 0;
}

HANDLE STDCALL
NtUserGetClipboardData(UINT uFormat, DWORD Unknown1)
{
   UNIMPLEMENTED
   return 0;
}

INT STDCALL
NtUserGetClipboardFormatName(UINT format, PUNICODE_STRING FormatName,
   INT cchMaxCount)
{
#if 1
  NTSTATUS Status;
  PWSTR Buf;
  UNICODE_STRING SafeFormatName, BufFormatName;
  ULONG Ret;
  
  if((cchMaxCount < 1) || !FormatName)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  /* copy the FormatName UNICODE_STRING structure */
  Status = MmCopyFromCaller(&SafeFormatName, FormatName, sizeof(UNICODE_STRING));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  /* Allocate memory for the string */
  Buf = ExAllocatePoolWithTag(NonPagedPool, cchMaxCount * sizeof(WCHAR), TAG_STRING);
  if(!Buf)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }
  
  /* Setup internal unicode string */
  BufFormatName.Length = 0;
  BufFormatName.MaximumLength = min(cchMaxCount * sizeof(WCHAR), SafeFormatName.MaximumLength);
  BufFormatName.Buffer = Buf;
  
  if(BufFormatName.MaximumLength < sizeof(WCHAR))
  {
    ExFreePool(Buf);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  Ret = IntGetClipboardFormatName(format, &BufFormatName);
  
  /* copy the UNICODE_STRING buffer back to the user */
  Status = MmCopyToCaller(SafeFormatName.Buffer, BufFormatName.Buffer, BufFormatName.MaximumLength);
  if(!NT_SUCCESS(Status))
  {
    ExFreePool(Buf);
    SetLastNtError(Status);
    return 0;
  }
  
  BufFormatName.MaximumLength = SafeFormatName.MaximumLength;
  BufFormatName.Buffer = SafeFormatName.Buffer;
  
  /* update the UNICODE_STRING structure (only the Length member should change) */
  Status = MmCopyToCaller(FormatName, &BufFormatName, sizeof(UNICODE_STRING));
  if(!NT_SUCCESS(Status))
  {
    ExFreePool(Buf);
    SetLastNtError(Status);
    return 0;
  }
  
  ExFreePool(Buf);
  return Ret;
#else
   return IntGetClipboardFormatName(format, FormatName);
#endif
}

HWND STDCALL
NtUserGetClipboardOwner(VOID)
{
   UNIMPLEMENTED
   return 0;
}

DWORD STDCALL
NtUserGetClipboardSequenceNumber(VOID)
{
   UNIMPLEMENTED
   return 0;
}

HWND STDCALL
NtUserGetClipboardViewer(VOID)
{
   UNIMPLEMENTED
   return 0;
}

INT STDCALL
NtUserGetPriorityClipboardFormat(UINT *paFormatPriorityList, INT cFormats)
{
   UNIMPLEMENTED
   return 0;
}

BOOL STDCALL
NtUserIsClipboardFormatAvailable(UINT format)
{
   DPRINT1("FIXME: NtUserIsClipboardFormatAvailable() is not implemented\n");
   return 0;
}

HANDLE STDCALL
NtUserSetClipboardData(UINT uFormat, HANDLE hMem, DWORD Unknown2)
{
   UNIMPLEMENTED
   return 0;
}

HWND STDCALL
NtUserSetClipboardViewer(HWND hWndNewViewer)
{
   UNIMPLEMENTED
   return 0;
}

/* EOF */
