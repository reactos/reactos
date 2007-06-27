/* $Id: appearance.c 13406 2005-02-04 20:39:10Z weiden $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS License Manager
 * FILE:            lib/cpl/liccpa
 * PURPOSE:         License Manager GUI
 * 
 * PROGRAMMERS:     Steven Edwards (steven_ed4153@yahoo.com)
 *
 * NOTES:
 * This application does almost nothing and its really good at it.
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"
#include "liccpa.h"

HINSTANCE hApplet = 0;

INT_PTR CALLBACK
DlgMainProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  TCHAR szString[256];

  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;

    case WM_COMMAND:
    {
      switch(HIWORD(wParam))
      {
        case LBN_DBLCLK:
        {
          switch(LOWORD(wParam))
          {
          }
          break;
        }
        default:
        {
          switch(LOWORD(wParam))
          {
            case IDC_OK:
            {
              break;
            }
            case IDC_CANCEL:
            {
              EndDialog(hwndDlg, IDC_CANCEL);
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case WM_CLOSE:
    {
      EndDialog(hwndDlg, IDC_CANCEL);
      return TRUE;
    }
  }
  return FALSE;
}

LONG CALLBACK
CPlApplet(
	HWND hwndCPl,
	UINT uMsg,
	LPARAM lParam1,
	LPARAM lParam2)
{
  switch(uMsg)
  {
    case CPL_INIT:
    {
      return TRUE;
    }
    case CPL_GETCOUNT:
    {
      return 1;
    }
    case CPL_INQUIRE:
    {
      CPLINFO *CPlInfo = (CPLINFO*)lParam2;
      CPlInfo->lData = 0;
      CPlInfo->idIcon = IDC_CPLICON_1;
      CPlInfo->idName = IDS_CPLNAME_1;
      CPlInfo->idInfo = IDS_CPLDESCRIPTION_1;
      break;
    }
    case CPL_DBLCLK:
    {
      DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_PROPPAGE1), hwndCPl, DlgMainProc, WM_INITDIALOG);
      break;
    }
  }
  return FALSE;
}


BOOL STDCALL
DllMain(
	HINSTANCE hinstDLL,
	DWORD     dwReason,
	LPVOID    lpvReserved)
{
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
      hApplet = hinstDLL;
      break;
  }
  return TRUE;
}
