/*---------------------------------------------------------------------------*\
| DIALOGS HEADER
|   This module contains the definitions for the dialog object.
|
|
| Copyright (c) Microsoft Corp., 1990-1993
|
| created: 01-Nov-91
| history: 01-Nov-91 <clausgi>  created.
|          29-Dec-92 <chriswil> port to NT, cleanup.
|          19-Oct-93 <chriswil> unicode enhancements from a-dianeo.
|
\*---------------------------------------------------------------------------*/

#define IDD_CONNECT         100
#define IDC_CONNECTNAME     101
#define IDD_PREFERENCES     200
#define ID_SIDEBYSIDE       201
#define ID_ANSONRESTORE     202
#define ID_TOPANDBOTTOM     203
#define ID_RECEIVEPFONT     204
#define ID_RECEIVEOFONT     205

/*
 * Context sensitive help IDs
 */
#define IDH_TOPANDBOTTOM    302
#define IDH_SIDEBYSIDE      303
#define IDH_RECEIVEPFONT    304
#define IDH_RECEIVEOFONT    305
#define IDH_OKBUTTON        306
#define IDH_CANCELBUTTON    307


INT_PTR  FAR  dlgDisplayBox(HINSTANCE,HWND,LPTSTR,DLGPROC,LPARAM);
BOOL CALLBACK dlgPreferencesProc(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK dlgConnectProc(HWND,UINT,WPARAM,LPARAM);
