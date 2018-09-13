//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       setting.c
//  Content:    This file contain all the functions that handle dial-up
//              settings.
//  History:
//      Tue 01-Nov-1994 16:32:11  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include "setting.h"
#include "rnahelp.h"

#pragma data_seg(DATASEG_READONLY)
UINT const aidNoRedial[] = { IDC_SET_RDCNTLABEL,
                       IDC_SET_RDCNT,
                       IDC_SET_RDCNT_ARRW,
                       IDC_SET_RDC_UNIT,
                       IDC_SET_RDW_LABEL,
                       IDC_SET_RDWMIN,
                       IDC_SET_RDWMIN_ARRW,
                       IDC_SET_RDW_UNIT1,
                       IDC_SET_RDWSEC,
                       IDC_SET_RDWSEC_ARRW,
                       IDC_SET_RDW_UNIT2};

SPINCTRL const aSpinCtrl[] = {{IDC_SET_RDCNT,     IDC_SET_RDCNT_ARRW,
                            MAX_REDIAL_COUNT,  MIN_REDIAL_COUNT,
                            IDS_ERR_INV_RDCNT},
                           {IDC_SET_RDWMIN,    IDC_SET_RDWMIN_ARRW,
                            MAX_REDIAL_MINUTE, MIN_REDIAL_MINUTE,
                            IDS_ERR_INV_RDWMIN},
                           {IDC_SET_RDWSEC,    IDC_SET_RDWSEC_ARRW,
                            MAX_REDIAL_SECOND, MIN_REDIAL_SECOND,
                            IDS_ERR_INV_RDWSEC}};
#pragma data_seg()

//****************************************************************************
// BOOL FAR PASCAL Remote_Setting (HWND)
//
// This function is called when the global Settings menu is selected.
//
// History:
//   01-Nov-1994 16:34:34  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL FAR PASCAL Remote_Setting (HWND hWnd)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE  *pPropPage;
    HPROPSHEETPAGE hpage, rgPages;
    RNASETTING     si;
    BOOL           bRet = FALSE;

    // Prepare a property sheet
    //
    psh.dwSize     = sizeof(psh);
    psh.dwFlags    = PSP_DEFAULT | PSH_NOAPPLYNOW;
    psh.hwndParent = hWnd;
    psh.hInstance  = ghInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_CAP_REMOTE);
    psh.nPages     = 0;
    psh.nStartPage = 0;
    psh.phpage     = &rgPages;

    // Get the initial dial-settings
    //
    if (RnaGetDialSettings(&si) == ERROR_SUCCESS)
    {
      // Allocate PROPSHEETINFO structure and initialize its content
      if ((pPropPage = Setting_GetPropSheet (&si)) != NULL)
      {
        // Create the property sheet for the modem object
        hpage = CreatePropertySheetPage(pPropPage);
        psh.phpage[psh.nPages++] = hpage;

        // Display property sheet
        //
	PropertySheet(&psh);
	Setting_ReleasePropSheet(pPropPage);
        bRet = TRUE;
      };
    };
    return bRet;
}

//****************************************************************************
// PROPSHEETINFO* NEAR PASCAL Setting_GetPropSheet (PRNASETTING)
//
// This function allocates the PROPSHEET staructure and initializes it.
//
// History:
//  Tue 23-Feb-1993 12:05:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

PROPSHEETPAGE* NEAR PASCAL Setting_GetPropSheet (PRNASETTING  psi)
{
  PROPSHEETPAGE *psp;

  // Allocate the PROPSHEETPAGE structure
  if ((psp = (PROPSHEETPAGE *)LocalAlloc(LMEM_FIXED,
                                         sizeof(PROPSHEETPAGE))) != NULL)
  {
    // Initialize its content
    psp->dwSize      = sizeof(*psp);
    psp->dwFlags     = PSP_DEFAULT;
    psp->hInstance   = ghInstance;
    psp->pszTemplate = MAKEINTRESOURCE(IDD_RNA_SETTING);
    psp->pfnDlgProc  = SettingDlgProc;
    psp->pszTitle    = NULL;
    psp->lParam      = (LPARAM)psi;
    psp->pfnCallback = NULL;
  }

  return psp;
}

//****************************************************************************
// void CALLBACK Setting_ReleasePropSheet (LPPROPSHEETPAGE)
//
// This function releases the resource allocated for the modem property sheet.
//
// History:
//  Tue 23-Feb-1993 10:29:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void CALLBACK Setting_ReleasePropSheet (LPPROPSHEETPAGE   psp)
{
  // Free the PROPSHEETINFO structure
  LocalFree((HLOCAL)OFFSETOF(psp));
}

//****************************************************************************
// BOOL CALLBACK _export SettingDlgProc (HWND, UINT, WPARAM, LPARAM)
//
// This function displays the modal connection entry setting dialog box.
//
// History:
//  Wed 17-Mar-1993 07:52:16  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL    CALLBACK _export SettingDlgProc (HWND    hWnd,
                                         UINT    message,
                                         WPARAM  wParam,
                                         LPARAM  lParam)
{
  PRNASETTING   pRnaSetting;

  switch (message)
  {
    case WM_INITDIALOG:
      // Remember the pointer to the setting structure
      pRnaSetting   = (PRNASETTING)(((LPPROPSHEETPAGE)lParam)->lParam);

      SetWindowLong(hWnd, DWL_USER, (LONG)pRnaSetting);

      // Initialize the appearance of the dialog box
      return (InitSettingDlg(hWnd, pRnaSetting));

    case WM_NOTIFY:
      switch(((NMHDR FAR *)lParam)->code)
      {
        case PSN_KILLACTIVE:
          //
          // Validate the connection information
          //
          SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)IsInvalidSetting(hWnd));
          return TRUE;

        case PSN_APPLY:
          //
          // The setting was validated, we can apply it permanently.
          //
          ApplySetting(hWnd);
          return FALSE;

        default:
          break;
      };
      break;

    case WM_COMMAND:
    {
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
        case IDC_SET_REDIAL:
          AdjustSettingDlg (hWnd);
          break;

        default:
          break;
      };
      break;
    }

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaSettings, message, wParam, lParam);
      break;
  }
  return FALSE;
}

//****************************************************************************
// BOOL NEAR PASCAL InitSettingDlg (HWND, PRNASETTING)
//
// This function initializes the appearance on the setting dialog.
//
// History:
//  Tue 01-Nov-1994 17:45:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL InitSettingDlg (HWND hWnd, PRNASETTING pRnaSetting)
{
  int i;

  // Initialize the redial options
  //
  CheckDlgButton(hWnd, IDC_SET_REDIAL, pRnaSetting->fRedial);

  for (i = 0; i < (sizeof(aSpinCtrl)/sizeof(aSpinCtrl[0])); i++)
  {
    SendDlgItemMessage(hWnd, aSpinCtrl[i].idArrow, UDM_SETRANGE, 0,
                       MAKELPARAM(aSpinCtrl[i].dwMax, aSpinCtrl[i].dwMin));
  };
  SetDlgItemInt(hWnd, IDC_SET_RDCNT,  pRnaSetting->cRetry, FALSE);
  SetDlgItemInt(hWnd, IDC_SET_RDWMIN, pRnaSetting->dwMin,  FALSE);
  SetDlgItemInt(hWnd, IDC_SET_RDWSEC, pRnaSetting->dwSec,  FALSE);

  // Initialize the Implicit Connection options
  //
  CheckDlgButton(hWnd,
                 pRnaSetting->fImplicit ? IDC_SET_ENIMPLICIT : IDC_SET_DISIMPLICIT,
                 TRUE);

  // Initialize the Dialing options
  //
  CheckDlgButton(hWnd,
                 IDC_SET_TRAY,
                 pRnaSetting->dwDialUI & DIALUI_NO_TRAY ? FALSE : TRUE);
  CheckDlgButton(hWnd,
                 IDC_SET_PROMPT,
                 pRnaSetting->dwDialUI & DIALUI_NO_PROMPT ? FALSE : TRUE);
  CheckDlgButton(hWnd,
                 IDC_SET_CONFIRM,
                 pRnaSetting->dwDialUI & DIALUI_NO_CONFIRM ? FALSE : TRUE);

  // Adjust the control appearance
  AdjustSettingDlg(hWnd);

  return TRUE;
}

//****************************************************************************
// void NEAR PASCAL AdjustSettingDlg (HWND)
//
// This function adjusts the appearance on the setting dialog based on the
// current setting.
//
// History:
//  Tue 01-Nov-1994 17:55:33  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL AdjustSettingDlg (HWND hWnd)
{
  BOOL fRedial;
  int  i;

  // Cache the device type
  fRedial = IsDlgButtonChecked(hWnd, IDC_SET_REDIAL);

  // Disable the fields irrelevant to a null modem
  for (i = 0; i < (sizeof(aidNoRedial)/sizeof(aidNoRedial[0])); i++)
  {
    HWND   hCtrl;

    if ((hCtrl = GetDlgItem(hWnd, aidNoRedial[i])) != NULL)
    {
      EnableWindow(hCtrl, fRedial);
    };
  };
}

//****************************************************************************
// BOOL NEAR PASCAL IsInvalidSetting(HWND)
//
// This function validates the current setting.
//
// History:
//  Wed 02-Nov-1994 08:01:34  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL NEAR PASCAL IsInvalidSetting(HWND hWnd)
{
  BOOL        fValid;

  // Need to check only the redial option
  if (IsDlgButtonChecked(hWnd, IDC_SET_REDIAL))
  {
    int  i;

    // Check the limit of each redial setting
    //
    fValid = TRUE;
    for (i = 0; fValid && (i < (sizeof(aSpinCtrl)/sizeof(aSpinCtrl[0]))); i++)
    {
      UINT   uSet;

      uSet = (UINT)GetDlgItemInt(hWnd, aSpinCtrl[i].id, &fValid, FALSE);

      if ((!fValid) || (uSet > aSpinCtrl[i].dwMax) || (uSet < aSpinCtrl[i].dwMin))
      {
        HWND hCtrl = GetDlgItem(hWnd, aSpinCtrl[i].id);

        RuiUserMessage(hWnd, aSpinCtrl[i].idsErr, MB_OK | MB_ICONEXCLAMATION);
        SetFocus(hCtrl);
        Edit_SetSel(hCtrl, 0, 0x7FFFF);
        fValid = FALSE;
      };
    };
  }
  else
    fValid = TRUE;

  return (!fValid);
}

//****************************************************************************
// void NEAR PASCAL ApplySetting(HWND)
//
// This function gets the current setting from UI.
//
// History:
//  Wed 02-Nov-1994 08:24:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void NEAR PASCAL ApplySetting (HWND hWnd)
{
  PRNASETTING psi;

  // Get the checkbox options
  //
  psi = (PRNASETTING)GetWindowLong(hWnd, DWL_USER);
  psi->fRedial   = IsDlgButtonChecked(hWnd, IDC_SET_REDIAL);
  psi->fImplicit = IsDlgButtonChecked(hWnd, IDC_SET_ENIMPLICIT);

  if (IsDlgButtonChecked(hWnd, IDC_SET_TRAY))
  {
    psi->dwDialUI &= ~DIALUI_NO_TRAY;
  }
  else
  {
    psi->dwDialUI |= DIALUI_NO_TRAY;
  };

  if (IsDlgButtonChecked(hWnd, IDC_SET_PROMPT))
  {
    psi->dwDialUI &= ~DIALUI_NO_PROMPT;
  }
  else
  {
    psi->dwDialUI |= DIALUI_NO_PROMPT;
  };

  if (IsDlgButtonChecked(hWnd, IDC_SET_CONFIRM))
  {
    psi->dwDialUI &= ~DIALUI_NO_CONFIRM;
  }
  else
  {
    psi->dwDialUI |= DIALUI_NO_CONFIRM;
  };

  // If the redial option is set
  if (psi->fRedial)
  {
    BOOL fValid;

    // Get the redial settings
    // Note that all the setting must have been verified already
    //
    psi->cRetry = GetDlgItemInt(hWnd, IDC_SET_RDCNT, &fValid, FALSE);
    ASSERT(fValid);
    psi->dwMin = GetDlgItemInt(hWnd, IDC_SET_RDWMIN, &fValid, FALSE);
    ASSERT(fValid);
    psi->dwSec = GetDlgItemInt(hWnd, IDC_SET_RDWSEC, &fValid, FALSE);
    ASSERT(fValid);
  }
  else
  {
    psi->cRetry = MIN_REDIAL_COUNT;
    psi->dwMin  = MIN_REDIAL_MINUTE;
    psi->dwSec  = MIN_REDIAL_SECOND;
  };

  // Apply the setting permanently
  //
  RnaSetDialSettings(psi);
  return;
}
