/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    find.h

Abstract:

    This module contains the header information for the Win32 find dialog.

Revision History:

--*/



//
//  Include Files.
//

#include <help.h>




//
//  Constant Declarations.
//

//
//  Length of "Close" string in chars.
//
#define CCHCLOSE        9

//
//  Dialog Box PROPERTY slots defined.
//
//  Note: If each app does indeed have a copy of the dll's global
//        variable space, then there is no reason to stick properties
//        onto the window like this.
//
#define FINDREPLACEPROP (LPCTSTR) 0xA000L

//
//  Overload Dialog Type.
//
#define DLGT_FIND       0x0
#define DLGT_REPLACE    0x1

#define cbFindMax       1024




//
//  Typedef Declarations.
//

typedef struct {
   UINT           ApiType;
   UINT           DlgType;
   LPFINDREPLACE  pFR;
} FINDREPLACEINFO;

typedef FINDREPLACEINFO *PFINDREPLACEINFO;




//
//  Global Variables.
//

static UINT wFRMessage;
static UINT wHelpMessage;
static TCHAR szClose [CCHCLOSE];

LPFRHOOKPROC glpfnFindHook = 0;




//
//  Context Help IDs.
//

const static DWORD aFindReplaceHelpIDs[] =       // Context Help IDs
{
    edt1,    IDH_FIND_SEARCHTEXT,
    edt2,    IDH_REPLACE_REPLACEWITH,
    chx1,    IDH_FIND_WHOLE,
    chx2,    IDH_FIND_CASE,
    IDOK,    IDH_FIND_NEXT_BUTTON,
    psh1,    IDH_REPLACE_REPLACE,
    psh2,    IDH_REPLACE_REPLACE_ALL,
    pshHelp, IDH_HELP,
    grp1,    IDH_FIND_DIRECTION,
    rad1,    IDH_FIND_DIRECTION,
    rad2,    IDH_FIND_DIRECTION,

    0, 0
};




//
//  Function Prototypes.
//

HWND
CreateFindReplaceDlg(
    LPFINDREPLACE pFR,
    UINT DlgType,
    UINT ApiType);

BOOL
SetupOK(
   LPFINDREPLACE pFR,
   UINT DlgType,
   UINT ApiType);

HANDLE
GetDlgTemplate(
    LPFINDREPLACE pFR,
    UINT DlgType,
    UINT ApiType);

BOOL_PTR CALLBACK
FindReplaceDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

VOID
EndDlgSession(
   HWND hDlg,
   LPFINDREPLACE pFR);

VOID
InitControlsWithFlags(
   HWND hDlg,
   LPFINDREPLACE pFR,
   UINT DlgType,
   UINT ApiType);

VOID
UpdateTextAndFlags(
    HWND hDlg,
    LPFINDREPLACE pFR,
    DWORD dwActionFlag,
    UINT DlgType,
    UINT ApiType);

LRESULT
NotifyUpdateTextAndFlags(
    LPFINDREPLACE pFR);
