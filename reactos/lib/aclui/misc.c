/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/misc.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      07/01/2005  Created
 */
#define INITGUID
#include <windows.h>
#include <aclui.h>

#include <precomp.h>

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
  HRSRC hrSrc;
  HGLOBAL hRes;
  LPWSTR lpName, lpStr;

  if(hInst == NULL)
  {
    return -1;
  }

  /* There are always blocks of 16 strings */
  lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

  /* Find the string table block */
  if((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
     (hRes = LoadResource(hInst, hrSrc)) &&
     (lpStr = LockResource(hRes)))
  {
    UINT x;

    /* Find the string we're looking for */
    uID &= 0xF; /* position in the block, same as % 16 */
    for(x = 0; x < uID; x++)
    {
      lpStr += (*lpStr) + 1;
    }

    /* Found the string */
    return (int)(*lpStr);
  }
  return -1;
}

static INT
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
  INT ln;

  ln = LengthOfStrResource(hInst,
                           uID);
  if(ln++ > 0)
  {
    (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                     ln * sizeof(WCHAR));
    if((*lpTarget) != NULL)
    {
      INT Ret;
      if(!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
      {
        LocalFree((HLOCAL)(*lpTarget));
      }
      return Ret;
    }
  }
  return 0;
}

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...)
{
  DWORD Ret = 0;
  LPWSTR lpFormat;
  va_list lArgs;

  if(AllocAndLoadString(&lpFormat,
                        hInstance,
                        uID) > 0)
  {
    va_start(lArgs, lpTarget);
    /* let's use FormatMessage to format it because it has the ability to allocate
       memory automatically */
    Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         lpFormat,
                         0,
                         0,
                         (LPWSTR)lpTarget,
                         0,
                         &lArgs);
    va_end(lArgs);

    LocalFree((HLOCAL)lpFormat);
  }

  return Ret;
}

BOOL
OpenLSAPolicyHandle(IN LPWSTR SystemName,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PLSA_HANDLE PolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES LsaObjectAttributes;
    LSA_UNICODE_STRING LsaSystemName, *psn;
    NTSTATUS Status;

    ZeroMemory(&LsaObjectAttributes, sizeof(LSA_OBJECT_ATTRIBUTES));

    if (SystemName != NULL)
    {
        LsaSystemName.Buffer = SystemName;
        LsaSystemName.Length = wcslen(SystemName) * sizeof(WCHAR);
        LsaSystemName.MaximumLength = LsaSystemName.Length + sizeof(WCHAR);
        psn = &LsaSystemName;
    }
    else
    {
        psn = NULL;
    }

    Status = LsaOpenPolicy(psn,
                           &LsaObjectAttributes,
                           DesiredAccess,
                           PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    return TRUE;
}

LPARAM
ListViewGetSelectedItemData(IN HWND hwnd)
{
    int Index;

    Index = ListView_GetNextItem(hwnd,
                                 -1,
                                 LVNI_SELECTED);
    if (Index != -1)
    {
        LVITEM li;

        li.mask = LVIF_PARAM;
        li.iItem = Index;
        li.iSubItem = 0;

        if (ListView_GetItem(hwnd,
                             &li))
        {
            return li.lParam;
        }
    }

    return 0;
}

BOOL
ListViewSelectItem(IN HWND hwnd,
                   IN INT Index)
{
    LVITEM li;

    li.mask = LVIF_STATE;
    li.iItem = Index;
    li.iSubItem = 0;
    li.state = LVIS_SELECTED;
    li.stateMask = LVIS_SELECTED;

    return ListView_SetItem(hwnd,
                            &li);
}

