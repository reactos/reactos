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
/* $Id: sysdm.c,v 1.3 2004/07/01 20:27:50 ekohl Exp $
 *
 * PROJECT:         ReactOS System Control Panel
 * FILE:            lib/cpl/system/sysdm.c
 * PURPOSE:         ReactOS System Control Panel
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */
#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <cpl.h>
#include <stdlib.h>

#include "resource.h"
#include "sysdm.h"

#define NUM_APPLETS	(1)

LONG CALLBACK SystemApplet(VOID);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

static void
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
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
SystemApplet(VOID)
{
  PROPSHEETPAGE psp[5];
  PROPSHEETHEADER psh;
  TCHAR Caption[1024];
  
  LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));
  
  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE; /* | PSH_USECALLBACK */
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;
  psh.pfnCallback = NULL; /* PropSheetProc; */
  
  InitPropSheetPage(&psp[0], IDD_PROPPAGEGENERAL, GeneralPageProc);
  InitPropSheetPage(&psp[1], IDD_PROPPAGECOMPUTER, ComputerPageProc);
  InitPropSheetPage(&psp[2], IDD_PROPPAGEHARDWARE, HardwarePageProc);
  InitPropSheetPage(&psp[3], IDD_PROPPAGEUSERPROFILE, UserProfilePageProc);
  InitPropSheetPage(&psp[4], IDD_PROPPAGEADVANCED, AdvancedPageProc);
  
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

