/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: general.c,v 1.2 2004/06/30 10:53:05 ekohl Exp $
 *
 * PROJECT:         ReactOS System Control Panel
 * FILE:            lib/cpl/system/general.c
 * PURPOSE:         General System Information
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

#include "resource.h"
#include "sysdm.h"

void
ShowLastWin32Error(HWND hWndOwner)
{
  LPTSTR lpMsg;
  DWORD LastError;
  
  LastError = GetLastError();
  
  if((LastError == 0) || !FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM, NULL, LastError, 
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR)&lpMsg, 0, 
                    NULL))
  {
    return;
  }
  
  MessageBox(hWndOwner, lpMsg, NULL, MB_OK | MB_ICONERROR);
  
  LocalFree((LPVOID)lpMsg);
}

typedef struct
{
  HWND hDlg;
} OSITINFO, *POSITINFO;

DWORD WINAPI
ObtainSystemInformationThread(POSITINFO posit)
{
  HRSRC hResInfo;
  HGLOBAL hResMem;
  WCHAR *LicenseText;
  
  /* wait a bit */
  Sleep(100);
  
  /* load license from resource */
  if(!(hResInfo = FindResource(hApplet, MAKEINTRESOURCE(RC_LICENSE), 
                               MAKEINTRESOURCE(RTDATA))) ||
     !(hResMem = LoadResource(hApplet, hResInfo)) ||
     !(LicenseText = LockResource(hResMem)))
  {
    ShowLastWin32Error(posit->hDlg);
    goto LoadSystemInfo;
  }
  /* insert the license into the edit control (unicode!) */
  SetDlgItemText(posit->hDlg, IDC_LICENSEMEMO, LicenseText);
  SendDlgItemMessage(posit->hDlg, IDC_LICENSEMEMO, EM_SETSEL, 0, 0);
  
  LoadSystemInfo:
  /* FIXME */
  /*free:*/
  HeapFree(GetProcessHeap(), 0, posit);
  
  return 0;
}

/* Property page dialog callback */
BOOL CALLBACK
GeneralPageProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      HANDLE Thread;
      DWORD ThreadId;
      POSITINFO posit;
      
      posit = (POSITINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OSITINFO));
      if(!posit)
      {
        ShowLastWin32Error(hwndDlg);
        return FALSE;
      }
      posit->hDlg = hwndDlg;
      Thread = CreateThread(NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)ObtainSystemInformationThread,
                            (PVOID)posit,
                            0,
                            &ThreadId);
      if(Thread)
      {
        CloseHandle(Thread);
        return FALSE;
      }
      
      break;
    }
  }
  return FALSE;
}

