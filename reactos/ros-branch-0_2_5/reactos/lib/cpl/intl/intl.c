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
/* $Id: intl.c,v 1.1 2004/10/30 12:33:23 ekohl Exp $
 *
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/intl.c
 * PURPOSE:         Property sheet code
 * PROGRAMMER:      Eric Kohl
 */
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"


#define NUM_APPLETS	(2)

LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);


HINSTANCE hApplet = 0;


/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};


VOID
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
  PROPSHEETPAGE psp[6];
  PROPSHEETHEADER psh;
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

  InitPropSheetPage(&psp[0], IDD_GENERALPAGE, GeneralPageProc);
  InitPropSheetPage(&psp[1], IDD_NUMBERSPAGE, NumbersPageProc);
  InitPropSheetPage(&psp[2], IDD_CURRENCYPAGE, CurrencyPageProc);
  InitPropSheetPage(&psp[3], IDD_TIMEPAGE, TimePageProc);
  InitPropSheetPage(&psp[4], IDD_DATEPAGE, DatePageProc);
  InitPropSheetPage(&psp[5], IDD_LOCALEPAGE, LocalePageProc);

  return (LONG)(PropertySheet(&psh) != -1);
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
	  UINT uMsg,
	  LPARAM lParam1,
	  LPARAM lParam2)
{
  switch(uMsg)
  {
    case CPL_INIT:
      return TRUE;

    case CPL_GETCOUNT:
      return NUM_APPLETS;

    case CPL_INQUIRE:
    {
      CPLINFO *CPlInfo = (CPLINFO*)lParam2;
      UINT uAppIndex = (UINT)lParam1;

      CPlInfo->lData = 0;
      CPlInfo->idIcon = Applets[uAppIndex].idIcon;
      CPlInfo->idName = Applets[uAppIndex].idName;
      CPlInfo->idInfo = Applets[uAppIndex].idDescription;
      break;
    }

    case CPL_DBLCLK:
    {
      UINT uAppIndex = (UINT)lParam1;
      Applets[uAppIndex].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
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
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
      hApplet = hinstDLL;
      break;
  }

  return TRUE;
}

