//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       setting.h
//  Content:    This file contain all the declaration for the Dial-up setting
//              UI.
//  History:
//      Tue 01-Nov-1994 16:49:45  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

typedef struct tagSpinCtrl      {
    UINT    id;
    UINT    idArrow;
    DWORD   dwMax;
    DWORD   dwMin;
    UINT    idsErr;
} SPINCTRL, *PSPINCTRL, FAR *LPSPINCTRL;

//****************************************************************************
// Function Prototypes
//****************************************************************************

//
//  Property sheet callback functions
//
void CALLBACK Setting_ReleasePropSheet (LPPROPSHEETPAGE psp);

//
//  Dialog boxes callback procedures
//
int  CALLBACK _export SettingDlgProc  (HWND    hWnd,
                                       UINT    message,
                                       WPARAM  wParam,
                                       LPARAM  lParam);

BOOL FAR PASCAL Remote_Setting (HWND hWnd);

PROPSHEETPAGE* NEAR PASCAL Setting_GetPropSheet (PRNASETTING psi);

BOOL NEAR PASCAL InitSettingDlg (HWND hWnd, PRNASETTING pRnaSetting);
void NEAR PASCAL AdjustSettingDlg (HWND hWnd);
BOOL NEAR PASCAL IsInvalidSetting(HWND hWnd);
void NEAR PASCAL ApplySetting (HWND hWnd);
