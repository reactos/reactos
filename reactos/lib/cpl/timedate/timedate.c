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
/* $Id: timedate.c,v 1.1 2004/10/30 19:15:31 ekohl Exp $
 *
 * PROJECT:         ReactOS Timedate Control Panel
 * FILE:            lib/cpl/timedate/timedate.c
 * PURPOSE:         ReactOS Timedate Control Panel
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"
#include "timedate.h"


#define NUM_APPLETS	(1)

LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);


HINSTANCE hApplet = 0;


/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};


/* Property page dialog callback */
INT_PTR CALLBACK
DateTimePageProc(HWND hwndDlg,
		 UINT uMsg,
		 WPARAM wParam,
		 LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      break;
  }

  return FALSE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
TimeZonePageProc(HWND hwndDlg,
		 UINT uMsg,
		 WPARAM wParam,
		 LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      break;
  }

  return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}


LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
  PROPSHEETHEADER psh;
  PROPSHEETPAGE psp[3];
  TCHAR Caption[256];

  LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;

  InitPropSheetPage(&psp[0], IDD_DATETIMEPAGE, DateTimePageProc);
  InitPropSheetPage(&psp[1], IDD_TIMEZONEPAGE, TimeZonePageProc);

  return (LONG)(PropertySheet(&psh) != -1);
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
	  UINT uMsg,
	  LPARAM lParam1,
	  LPARAM lParam2)
{
  int i = (int)lParam1;
  
  switch (uMsg)
  {
    case CPL_INIT:
      return TRUE;

    case CPL_GETCOUNT:
      return NUM_APPLETS;

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
      Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
      break;
    }
  }
  return FALSE;
}


BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
	DWORD dwReason,
	LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
      hApplet = hinstDLL;
      break;
  }

  return TRUE;
}

/* EOF */
