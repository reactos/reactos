/* $Id$
 *
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/powershemes.c
 * PURPOSE:         initialization of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 *                  Martin Rottensteiner
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"
#include "powercfg.h"

#define NUM_APPLETS	(1)

static LONG APIENTRY Applet1(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK powershemesProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK alarmsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK advancedProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK hibernateProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HINSTANCE hApplet = 0;
GLOBAL_POWER_POLICY gGPP;
TCHAR langSel[255];

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
static LONG APIENTRY
Applet1(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)	
{
  PROPSHEETPAGE psp[5];
  PROPSHEETHEADER psh;
  TCHAR Caption[1024];
  SYSTEM_POWER_CAPABILITIES spc;
  INT i=0;
 
  UNREFERENCED_PARAMETER(hwnd);
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  memset(Caption, 0x0, sizeof(Caption));
  LoadString(hApplet, IDS_CPLNAME_1, Caption, sizeof(Caption) / sizeof(TCHAR));
  
  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON_1));
  psh.pszCaption = Caption;
  psh.nPages = 3;
  psh.nStartPage = 0;
  psh.ppsp = psp;
  psh.pfnCallback = PropSheetProc;
  
  InitPropSheetPage(&psp[i++], IDD_PROPPAGEPOWERSHEMES, (DLGPROC) powershemesProc);
  if (GetPwrCapabilities(&spc))
  {
    if (spc.SystemBatteriesPresent)
	{
	  InitPropSheetPage(&psp[i++], IDD_PROPPAGEALARMS, (DLGPROC) alarmsProc);
	  psh.nPages += 1;
	}
  }
  InitPropSheetPage(&psp[i++], IDD_PROPPAGEADVANCED, (DLGPROC) advancedProc);
  InitPropSheetPage(&psp[i++], IDD_PROPPAGEHIBERNATE, (DLGPROC) hibernateProc);
  
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
      Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
      break;
    }
  }
  return FALSE;
}


BOOLEAN
WINAPI
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

