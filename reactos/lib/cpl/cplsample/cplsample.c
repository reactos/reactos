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
/* $Id: cplsample.c,v 1.2 2004/03/08 14:20:14 weiden Exp $
 *
 * PROJECT:         ReactOS Sample Control Panel
 * FILE:            lib/cpl/cplsample/cplsample.c
 * PURPOSE:         ReactOS Sample Control Panel
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      05-01-2004  Created
 */
#include <windows.h>
#include <stdlib.h>
#include "resource.h"
#include "cplsample.h"

#define NUM_APPLETS	(1)

LONG CALLBACK Applet1(VOID);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDC_CPLICON_1, IDS_CPLNAME_1, IDS_CPLDESCRIPTION_1, Applet1}
};

static void
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->u1.pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}

/* Property page dialog callback */
BOOL CALLBACK
Page1Proc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
  }
  return FALSE;
}

/* Property page dialog callback */
BOOL CALLBACK
Page2Proc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
  }
  return FALSE;
}

/* Property page dialog callback */
BOOL CALLBACK
Page3Proc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
  }
  return FALSE;
}

/* Property Sheet Callback */
int CALLBACK
PropSheetProc(
	HWND hwndDlg,
	UINT uMsg,
	LPARAM lParam
)
{
  switch(uMsg)
  {
    case PSCB_BUTTONPRESSED:
      switch(lParam)
      {
        case PSBTN_OK: /* OK */
          break;
        case PSBTN_CANCEL: /* Cancel */
          break;
        case PSBTN_APPLYNOW: /* Apply now */
          break;
        case PSBTN_FINISH: /* Close */
          break;
        default:
          return FALSE;
      }
      break;
      
    case PSCB_INITIALIZED:
      break;
  }
  return TRUE;
}

/* First Applet */

LONG CALLBACK
Applet1(VOID)
{
  PROPSHEETPAGE psp[3];
  PROPSHEETHEADER psh;
  TCHAR Caption[1024];
  
  LoadString(hApplet, IDS_CPLNAME_1, Caption, sizeof(Caption) / sizeof(TCHAR));
  
  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.u1.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON_1));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETHEADER);
  psh.u2.nStartPage = 0;
  psh.u3.ppsp = psp;
  psh.pfnCallback = PropSheetProc;
  
  InitPropSheetPage(&psp[0], IDD_PROPPAGE1, Page1Proc);
  InitPropSheetPage(&psp[1], IDD_PROPPAGE2, Page2Proc);
  InitPropSheetPage(&psp[2], IDD_PROPPAGE3, Page3Proc);
  
  return (LONG)(PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(
	HWND hwndCPl,
	UINT uMsg,
	LPARAM lParam1,
	LPARAM lParam2)
{
  int i = (int)lParam1;
  
  switch(uMsg)
  {
    case CPL_INIT:
    {
      return TRUE;
    }
    case CPL_GETCOUNT:
    {
      return NUM_APPLETS;
    }
    case CPL_INQUIRE:
    {
      CPLINFO *CPlInfo = (CPLINFO*)lParam2;
      CPlInfo->lData = 0;
      CPlInfo->idIcon = Applets[i].idIcon;
      CPlInfo->idName = Applets[i].idName;
      CPlInfo->idInfo = Applets[i].idDescription;
      break;
    }
    case CPL_DBLCLK:
    {
      Applets[i].AppletProc();
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

