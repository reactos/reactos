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

#define CHECK_LOCK                                                      \
        if (ClipboardThread && ClipboardThread != PsGetWin32Thread())   \
        {                                                               \
        SetLastWin32Error(ERROR_LOCKED);                                \
        return FALSE;                                                   \
        }

PW32THREAD ClipboardThread;
HWND ClipboardWindow;
HWND tempClipboardWindow;
HANDLE hCBData;
UINT   uCBFormat;

ULONG FASTCALL
IntGetClipboardFormatName(UINT format, PUNICODE_STRING FormatName)
{

   return IntGetAtomName((RTL_ATOM)format, FormatName->Buffer,
      FormatName->MaximumLength);
}

UINT FASTCALL
IntEnumClipboardFormats(UINT format)
{

   CHECK_LOCK

   if (!hCBData)
       return FALSE;
   //UNIMPLEMENTED;
   return 1;
}

BOOL STDCALL
NtUserOpenClipboard(HWND hWnd, DWORD Unknown1)
{
   CHECK_LOCK

   tempClipboardWindow = hWnd;
   ClipboardThread = PsGetWin32Thread();
   return TRUE;
}

BOOL STDCALL
NtUserCloseClipboard(VOID)
{
   CHECK_LOCK

   ClipboardWindow = 0;
   ClipboardThread = NULL;
   return TRUE;
}

/*
 * @unimplemented
 */
HWND STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
   /*
   UNIMPLEMENTED
   return 0;
   */
   return ClipboardWindow;
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
   CHECK_LOCK

//   if (!hCBData)
//       return FALSE;

//   FIXME!
//   GlobalUnlock(hCBData);
//   GlobalFree(hCBData);
   hCBData = NULL;
   uCBFormat = 0;
   ClipboardWindow = tempClipboardWindow;

   return TRUE;
}

HANDLE STDCALL
NtUserGetClipboardData(UINT uFormat, DWORD Unknown1)
{
   CHECK_LOCK

   if ((uFormat==1 && uCBFormat==13) || (uFormat==13 && uCBFormat==1))
       uCBFormat = uFormat;

   if (uFormat != uCBFormat)
       return FALSE;

   return hCBData;
}

INT STDCALL
NtUserGetClipboardFormatName(UINT format, PUNICODE_STRING FormatName,
   INT cchMaxCount)
{
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
  Buf = ExAllocatePoolWithTag(PagedPool, cchMaxCount * sizeof(WCHAR), TAG_STRING);
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

  if (format >= 0xC000)
  {
     Ret = IntGetClipboardFormatName(format, &BufFormatName);
  }
  else
  {
     SetLastNtError(NO_ERROR);
     return 0;
  }

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
   //UNIMPLEMENTED

   if (format != 1 && format != 13) {
      DbgPrint("Clipboard Format unavailable (%d)\n", format);
      return FALSE;
   }

   if ((format==1 && uCBFormat==13) || (format==13 && uCBFormat==1))
       uCBFormat = format;

   if (format != uCBFormat)
     return FALSE;

   return TRUE;
}

//SetClipboardData(CF_UNICODETEXT, hdst);
HANDLE STDCALL
NtUserSetClipboardData(UINT uFormat, HANDLE hMem, DWORD Unknown2)
{
//    LPVOID pMem;
    CHECK_LOCK


   if (uFormat != 1 && uFormat != 13) {
      DbgPrint("Clipboard unsupported format (%d)\n", uFormat);
      return FALSE;
   }

    if (hMem)
    {
        uCBFormat = uFormat;
        hCBData = hMem;
        //pMem = GlobalLock(hMem);
        /*
        switch (uFormat) {
            default:
                DbgPrint("Clipboard unsupported format (%d)\n", uFormat);
                return FALSE;
            case CF_TEXT:             // 1
                break;
            case CF_UNICODETEXT:      // 13
                break;
            case CF_BITMAP:           // 2
                break;
            case CF_OEMTEXT:          // 7
                break;
        } */
    }
    else
    {
    //the window provides data in the specified format
    }
    return hMem;
}

HWND STDCALL
NtUserSetClipboardViewer(HWND hWndNewViewer)
{
   UNIMPLEMENTED
   return 0;
}

/* EOF */
