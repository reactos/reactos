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
/* $Id$
 *
 * PROJECT:         ReactOS System Control Panel
 * FILE:            lib/cpl/system/computer.c
 * PURPOSE:         Computer settings for networking
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */
#include <windows.h>
#include <stdlib.h>
#include <lm.h>
#include "resource.h"
#include "sysdm.h"

/* Property page dialog callback */
INT_PTR CALLBACK
ComputerPageProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  LPWKSTA_INFO_101 wki;

  UNREFERENCED_PARAMETER(lParam);
  UNREFERENCED_PARAMETER(wParam);

  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      /* Display computer name */
      DWORD Size = MAX_COMPUTERNAME_LENGTH + 1;
      TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
      if (GetComputerName(ComputerName,&Size))
      {
          SendDlgItemMessage(hwndDlg,IDC_COMPUTERNAME,WM_SETTEXT,0,(LPARAM)ComputerName);
      }
      if (NetWkstaGetInfo(NULL,101,(LPBYTE*)&wki) == NERR_Success)
      {
        SendDlgItemMessage(hwndDlg,IDC_WORKGROUPDOMAIN_NAME,WM_SETTEXT,0,(LPARAM)wki->wki101_langroup);
        NetApiBufferFree(&wki);
      }
      break;
    }
  }
  return FALSE;
}

