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
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/intl.c
 * PURPOSE:         Property sheet code
 * PROGRAMMER:      Eric Kohl
 */
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <setupapi.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"


#define NUM_APPLETS	(1)

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);


HINSTANCE hApplet = 0;
HINF hSetupInf = INVALID_HANDLE_VALUE;
DWORD IsUnattendedSetupEnabled = 0;
DWORD UnattendLCID = 0;


/* Applets */
APPLET Applets[NUM_APPLETS] =
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};


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

BOOL
OpenSetupInf(VOID)
{
  LPTSTR lpCmdLine;
  LPTSTR lpSwitch;
  size_t len;

  lpCmdLine = GetCommandLine();

  lpSwitch = _tcsstr(lpCmdLine, _T("/f:\""));

  if(!lpSwitch)
  {
    return FALSE;
  }

  len = _tcslen(lpSwitch);
  if (len < 5)
  {
    return FALSE;
  }

  if(lpSwitch[len-1] != _T('\"'))
  {
    return FALSE;
  }

  lpSwitch[len-1] = _T('\0');

  hSetupInf = SetupOpenInfFile(&lpSwitch[4],
                               NULL,
                               INF_STYLE_OLDNT,
                               NULL);

  return (hSetupInf != INVALID_HANDLE_VALUE);
}

VOID
ParseSetupInf(VOID)
{
  INFCONTEXT InfContext;
  TCHAR szBuffer[30];

  if (!SetupFindFirstLine(hSetupInf,
              _T("Unattend"),
              _T("LocaleID"),
              &InfContext))
  {
    SetupCloseInfFile(hSetupInf);
    return;
  }

  if (!SetupGetStringField(&InfContext,
                        1,
                        szBuffer,
                        sizeof(szBuffer) / sizeof(TCHAR),
                        NULL))
  {
    SetupCloseInfFile(hSetupInf);
    return;
  }

  UnattendLCID = _tcstoul(szBuffer, NULL, 16);
  IsUnattendedSetupEnabled = 1;
  SetupCloseInfFile(hSetupInf);
}

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
  PROPSHEETPAGE psp[3];
  PROPSHEETHEADER psh;
  TCHAR Caption[256];

  if (OpenSetupInf())
  {
    ParseSetupInf();
  }

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
  InitPropSheetPage(&psp[1], IDD_LANGUAGESPAGE, LanguagesPageProc);
  InitPropSheetPage(&psp[2], IDD_ADVANCEDPAGE, AdvancedPageProc);

  return (LONG)(PropertySheet(&psh) != -1);
}


/* Control Panel Callback */
LONG APIENTRY
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

