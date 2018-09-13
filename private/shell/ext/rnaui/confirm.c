//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       confirm.c
//  Content:    This file contains all the functions for connect confirmation
//              dialog.
//  History:
//      Tue 19-Mar-1996 09:25:38  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "rnaui.h"
#include "contain.h"
#include "subobj.h"
#include "rnahelp.h"

#define ID_CC_TIMER         100
#define CC_POLLING_INTERVAL 5000

typedef struct tagCCDlg {
    HRASCONN        hrasconn;
    LPSTR           pszEntry;
    UINT            idTimer;
    RASCONNSTATUS   rcs;
    HBITMAP         hbm;
    HFONT           hFont;
}   CCDLG, *PCCDLG;

LONG CALLBACK ConfirmConnectDlgProc (HWND hDlg, UINT message,
                                     WPARAM wParam, LPARAM lParam);

/*----------------------------------------------------------
Purpose: Display the connection confirmation dialog box

Returns: None

Cond:    --
*/
void NEAR PASCAL ConfirmConnection (HWND hwnd, LPSTR szEntry)
{
  RNASETTING si;

  // Check whether we should display the dialog box
  //
  RnaGetDialSettings(&si);
  if (si.dwDialUI & DIALUI_NO_CONFIRM)
    return;

  // Yes, we will display the dialog box
  //
  DialogBoxParam(ghInstance,
                 MAKEINTRESOURCE(IDD_CONFIRMCONNECT),
                 hwnd, ConfirmConnectDlgProc, (LPARAM)szEntry);
  return;
}

/*----------------------------------------------------------
Purpose: Initialize the dialog appearence

Returns: None

Cond:    --
*/
void NEAR PASCAL _InitRedCarpetDialog(HWND hwndDlg)
{
    HBITMAP hbm;
    HWND    hwndCtl;
    NONCLIENTMETRICS ncm;
    PCCDLG  pCCDlg;

    pCCDlg   =   (PCCDLG)GetWindowLong(hwndDlg, DWL_USER);

    if (hwndCtl = GetDlgItem(hwndDlg, IDC_WELCOME_MON))
    {
        RECT rc;

        // set the position so that dithered colors won't leave a border.
        GetWindowRect(hwndCtl, &rc);

        hbm = (HBITMAP)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_WELCOME_MON),
                                 IMAGE_BITMAP,
                                 rc.right - rc.left,
                                 rc.bottom - rc.top,
                                 LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
        if (hbm)
        {
            SendMessage(hwndCtl, STM_SETIMAGE, 0, (LPARAM)hbm);
            pCCDlg->hbm = hbm;
        }

    }

    // Make the You know font to be bold...
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        HFONT hfontTemp;

        // make the bold font
        ncm.lfMenuFont.lfWeight = FW_BOLD;
        hfontTemp = CreateFontIndirect(&ncm.lfMenuFont);
        if (hfontTemp)
        {
            SendDlgItemMessage(hwndDlg, IDC_CC_NAME,
                    WM_SETFONT, (WPARAM)hfontTemp, 0);
            pCCDlg->hFont = hfontTemp;
        }
    }
}

/*----------------------------------------------------------
Purpose: Initializes the connection confirmation dialog box

Returns: None

Cond:    --
*/
DWORD NEAR PASCAL InitConfirmConnectDlg (HWND hwnd, LPSTR pszEntry)
{
  PCCDLG pCCDlg;
  char   szFmt[MAXMESSAGE], szShortName[MAXNAME], szText[MAXSTRINGLEN];

  // Allocate buffer for the connection info
  //
  pCCDlg = (PCCDLG)LocalAlloc(LPTR, sizeof(*pCCDlg));
  SetWindowLong(hwnd, DWL_USER, (LPARAM)pCCDlg);
  if (pCCDlg == NULL)
  {
    return ERROR_OUTOFMEMORY;
  };

  // Get the connection handle
  //
  if ((pCCDlg->hrasconn = Remote_GetConnHandle(pszEntry)) == NULL)
  {
    return ERROR_INVALID_HANDLE;
  };

  // Display the connection name
  //
  pCCDlg->pszEntry = pszEntry;
  ShortenName(pszEntry, szShortName, sizeof(szShortName));
  GetDlgItemText(hwnd, IDC_CC_NAME, szFmt, sizeof(szFmt));
  wsprintf(szText, szFmt, szShortName);
  SetDlgItemText(hwnd, IDC_CC_NAME, szText);

  // Initialize the dialog appearence
  //
  _InitRedCarpetDialog(hwnd);

  // Set the polling timer
  //
  pCCDlg->rcs.dwSize = sizeof(RASCONNSTATUS);
  pCCDlg->idTimer = SetTimer(hwnd, ID_CC_TIMER, CC_POLLING_INTERVAL, NULL);
  return ERROR_SUCCESS;
}

/*----------------------------------------------------------
Purpose: Deinitializes the connection confirmation dialog box

Returns: None

Cond:    --
*/
void NEAR PASCAL DeinitConfirmConnectDlg (HWND hwnd)
{
  PCCDLG pCCDlg;

  pCCDlg = (PCCDLG)GetWindowLong(hwnd, DWL_USER);

  if (pCCDlg != NULL)
  {
    // Kill the polling timer
    //
    if (pCCDlg->idTimer != 0)
    {
      KillTimer(hwnd, pCCDlg->idTimer);
    };

    if (pCCDlg->hbm != NULL)
    {
      DeleteObject(pCCDlg->hbm);
    };

    if (pCCDlg->hFont != NULL)
    {
      DeleteObject(pCCDlg->hFont);
    };

    // Free the connection info buffer
    //
    LocalFree((HLOCAL)pCCDlg);
  };
  return;
}

/*----------------------------------------------------------
Purpose: Checks the connection status

Returns: None

Cond:    --
*/
DWORD NEAR PASCAL CheckConnectionStatus (HWND hwnd)
{
  PCCDLG pCCDlg;

  pCCDlg = (PCCDLG)GetWindowLong(hwnd, DWL_USER);

  // Get the connection status
  //
  return (RasGetConnectStatus(pCCDlg->hrasconn, &pCCDlg->rcs));
}

/*----------------------------------------------------------
Purpose: Display the connection confirmation dialog box

Returns: None

Cond:    --
*/
LONG CALLBACK ConfirmConnectDlgProc (HWND hDlg, UINT message, WPARAM wParam,
                                    LPARAM lParam)
{
  switch(message)
  {
    case WM_INITDIALOG:
        if (InitConfirmConnectDlg(hDlg, (LPSTR)lParam) != ERROR_SUCCESS)
        {
          EndDialog(hDlg, IDCANCEL);
        };
        return TRUE;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
          case IDC_CC_WHATSNEXT:
            WhatNextHelp (hDlg);
            break;

          case IDOK:
          {
            if (IsDlgButtonChecked(hDlg, IDC_CC_NO_CONFIRM))
            {
              RNASETTING si;

              // Do not prompt this dialog in the future
              //
              RnaGetDialSettings(&si);
              si.dwDialUI |= DIALUI_NO_CONFIRM;
              RnaSetDialSettings(&si);
            };

            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;
          }
          default:
            break;
        }
        break;

    case WM_TIMER:
        if (CheckConnectionStatus(hDlg) != ERROR_SUCCESS)
        {
          EndDialog(hDlg, IDCANCEL);
        };
        break;

    case WM_DESTROY:
        DeinitConfirmConnectDlg(hDlg);
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaConfirm, message, wParam, lParam);
      break;

    default:
        break;
  };
  return FALSE;
}
