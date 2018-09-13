//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       mlink.h
//  Content:    This file contain all the declaration for the connection entry
//              process UI.
//  History:
//      Fri 26-Mar-1993 12:28:38  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

// Sub-entry information
//
#define ML_COL_BASE                 0
#define ML_COL_DEVICE               (ML_COL_BASE)
#define ML_COL_PHONE                (ML_COL_BASE+1)

#define MAX_USER_COLS               2

//****************************************************************************
// Function Prototypes
//****************************************************************************

BOOL CALLBACK _export MLDlgProc (HWND    hWnd,
                                 UINT    message,
                                 WPARAM  wParam,
                                 LPARAM  lParam);
BOOL CALLBACK _export EditMLInfoDlg(HWND hDlg,
                                    UINT message,
                                    WPARAM wParam,
                                    LPARAM lParam);

BOOL  NEAR PASCAL InitMLDlg (HWND hWnd);
void  NEAR PASCAL DeInitMLDlg (HWND hWnd);
BOOL  NEAR PASCAL IsInvalidMLEntry(HWND hWnd);
DWORD NEAR PASCAL GetMLSetting (HWND hWnd);

void  NEAR PASCAL InitMLList (HWND hDlg, LPSTR szEntryName, PMLINFO pmli);
void  NEAR PASCAL DeinitMLList (HWND hDlg);
void  NEAR PASCAL SaveMLList(HWND hDlg, LPSTR szEntryName);
void  NEAR PASCAL AdjustMLControls(HWND hWnd);

DWORD NEAR PASCAL AddMLItem (HWND hDlg);
BOOL  NEAR PASCAL RemoveMLItem (HWND hDlg, UINT id);
BOOL  NEAR PASCAL EditMLItem (HWND hDlg, UINT id);

BOOL NEAR PASCAL InitEditML (HWND hDlg);
void  NEAR PASCAL DeinitEditML(HWND hDlg);
void  NEAR PASCAL AdjustMLDeviceList(HWND hDlg);
DWORD NEAR PASCAL SaveEditMLInfo (HWND hDlg);
void  NEAR PASCAL ModifyMLDeviceList (HWND hwnd, DWORD dwCommand);
