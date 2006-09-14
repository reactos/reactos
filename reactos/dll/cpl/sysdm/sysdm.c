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
  UNREFERENCED_PARAMETER(hwndDlg);
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
  PROPSHEETPAGE psp[4];
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

  InitPropSheetPage(&psp[0], IDD_PROPPAGEGENERAL, (DLGPROC) GeneralPageProc);
  InitPropSheetPage(&psp[1], IDD_PROPPAGECOMPUTER, (DLGPROC) ComputerPageProc);
  InitPropSheetPage(&psp[2], IDD_PROPPAGEHARDWARE, (DLGPROC) HardwarePageProc);
  InitPropSheetPage(&psp[3], IDD_PROPPAGEADVANCED, (DLGPROC) AdvancedPageProc);

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

