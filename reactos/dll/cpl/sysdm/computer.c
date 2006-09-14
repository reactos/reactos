/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/computer.c
 * PURPOSE:     Computer settings for networking
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *
 */

#include "precomp.h"

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

