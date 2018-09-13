//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnawiz.h
//  Content:    This file contains the declaration for Rna Wizard
//  History:
//      Tue 08-Feb-1994 08:14:33  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifndef _RNAWIZ_H_
#define _RNAWIZ_H_

//****************************************************************************
// Registry string definitions
//****************************************************************************

#define REGSTR_VAL_WIZARD   "wizard"

//****************************************************************************
// Constant Definitions
//****************************************************************************

#define BUFFER_SIZE         0x00008000
#define DUP_SUFFIX_START    2
#define DUP_SUFFIX_MAX      20

#define IDB_WIZ_LOGO        150

#define IDC_WZBMP           10

#define IDD_WIZ_SCREEN      5000
#define IDD_WIZ_INTRO       5000
#define IDD_WIZ_CLIENT_1    5001
#define IDD_WIZ_CLIENT_2    5002
#define IDD_WIZ_CLIENT_3    5003

#define IDC_WIZ_CTRL        (IDD_WIZ_SCREEN+100)
#define IDC_WC_INST         (IDC_WIZ_CTRL+2)

#define MAX_WIZ_PAGES       (IDD_WIZ_CLIENT_3 - IDD_WIZ_SCREEN + 1)

#define IDS_WIZ_BASE        IDS_BASE+5000
#define IDS_WIZ_INST_MODEM  IDS_WIZ_BASE

#define IDS_WIZ_ERR           IDS_WIZ_BASE+1000
#define IDS_WIZ_NOCONN_NODRV  IDS_WIZ_ERR+1
#define IDS_WIZ_NO_INSTALL    IDS_WIZ_ERR+2

//****************************************************************************
// Private type definition
//****************************************************************************

typedef BOOL (NEAR PASCAL * INITPROC)(HWND);
typedef BOOL (NEAR PASCAL * ENDPROC)(HWND, int);

typedef struct tagFirstScreen {
    LPSTR       szCmd;
    int         index;
    UINT        idIcon;
}   FIRSTSCREEN;

typedef struct tagWizInfo {
    UINT        uFirstPage;
    BOOL        fActivateRNA;
    CONNENTDLG  ConnEntDlg;
    char        szNewName[RAS_MaxEntryName+1];
}   WIZINFO, FAR* LPWIZINFO;

#define NO_INTRO            INTRO_WIZ

#define INTRO_FIRST_SCREEN  (IDD_WIZ_INTRO - IDD_WIZ_SCREEN)
#define CLIENT_FIRST_SCREEN (IDD_WIZ_CLIENT_1 - IDD_WIZ_SCREEN)

//****************************************************************************
// Function Declarations
//****************************************************************************

DWORD NEAR PASCAL GetWizardSettings (HWND hwnd, LPDWORD lpdwSettings);
DWORD NEAR PASCAL SetWizardSettings (HWND hwnd, DWORD dwSettings);

DWORD NEAR PASCAL CheckRnaSetup (HWND hwnd, LPSTR szDeviceName, UINT idErrMsg);

BOOL NEAR PASCAL  InstallDevice(HWND hWnd);

DWORD WINAPI          RnaWizard (HWND hWnd,         HINSTANCE hAppInstance,
                                 LPSTR lpszCmdLine, int   nCmdShow);

DWORD NEAR PASCAL RnaWizardSequence(HWND hWnd, UINT uFirstPage);
void  NEAR PASCAL AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn,
                          LPWIZINFO lpwi);
void  NEAR PASCAL DoWizard(HWND hwnd, UINT nStartPage, LPWIZINFO lpwi);

DWORD NEAR PASCAL InitClientWizard(HWND hwnd, LPWIZINFO lpwi);
BOOL  NEAR PASCAL DeinitClientWizard(HWND hwnd, LPWIZINFO lpwi);

BOOL CALLBACK    IntroDlgProc(HWND hDlg, UINT message ,
                              WPARAM wParam, LPARAM lParam);

BOOL CALLBACK    Client1DlgProc(HWND hDlg, UINT message ,
                                WPARAM wParam, LPARAM lParam);
void NEAR PASCAL AdjustClient1Dlg(HWND hDlg);
BOOL NEAR PASCAL CommitClient1Dlg(HWND hDlg);

BOOL CALLBACK    Client2DlgProc(HWND hDlg, UINT message ,
                                WPARAM wParam, LPARAM lParam);
BOOL NEAR PASCAL CommitClient2Dlg(HWND hDlg);

BOOL CALLBACK    Client3DlgProc(HWND hDlg, UINT message ,
                                WPARAM wParam, LPARAM lParam);

BOOL  NEAR PASCAL SaveClientEntry(HWND hDlg, PCONNENTRY pConnEntry);
DWORD NEAR PASCAL ImportRasPhonebook (HWND hDlg, LPCSTR szPhonebook,
                                      PCONNENTRY pConnEntry);
DWORD NEAR PASCAL FinalRnaClientSetup (HWND hDlg,  LPWIZINFO lpwi);

#endif  //_RNAWIZ_H_
