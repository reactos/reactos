/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/sysdm.c
 * PURPOSE:     dll entry file
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *
 */

#include "precomp.h"

LONG CALLBACK SystemApplet(VOID);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
  {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

#define MAX_SYSTEM_PAGES    8

static BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_SYSTEM_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}

static BOOL
InitPropSheetPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC DlgProc)
{
    HPROPSHEETPAGE hPage;
    PROPSHEETPAGE psp;

    if (ppsh->nPages < MAX_SYSTEM_PAGES)
    {
        ZeroMemory(&psp, sizeof(psp));
        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hApplet;
        psp.pszTemplate = MAKEINTRESOURCE(idDlg);
        psp.pfnDlgProc = DlgProc;

        hPage = CreatePropertySheetPage(&psp);
        if (hPage != NULL)
        {
            return PropSheetAddPage(hPage, (LPARAM)ppsh);
        }
    }

    return FALSE;
}

/* First Applet */

LONG CALLBACK
SystemApplet(VOID)
{
  HPROPSHEETPAGE hpsp[MAX_SYSTEM_PAGES];
  PROPSHEETHEADER psh;
  TCHAR Caption[128];
  static INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_LINK_CLASS};

  if (!InitCommonControlsEx(&icc))
      return 0;

  LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
  psh.pszCaption = Caption;
  psh.nPages = 0;
  psh.nStartPage = 0;
  psh.phpage = hpsp;
  psh.pfnCallback = NULL;

  InitPropSheetPage(&psh, IDD_PROPPAGEGENERAL, (DLGPROC) GeneralPageProc);
  InitPropSheetPage(&psh, IDD_PROPPAGECOMPUTER, (DLGPROC) ComputerPageProc);
  InitPropSheetPage(&psh, IDD_PROPPAGEHARDWARE, (DLGPROC) HardwarePageProc);
  InitPropSheetPage(&psh, IDD_PROPPAGEADVANCED, (DLGPROC) AdvancedPageProc);

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
  UNREFERENCED_PARAMETER(hwndCPl);

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
  UNREFERENCED_PARAMETER(lpvReserved);
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
      hApplet = hinstDLL;
      break;
  }
  return TRUE;
}

