/******************************************************************************

  Source File:  deskpan.cpp

  Main code for the advanced desktop Panning page

  Copyright (c) 1997-1998 by Microsoft Corporation

  Change History:

  12-16-97 AndreVa - Created It

******************************************************************************/


#include    "deskpan.h"

// OLE-Registry magic number
// 42071714-76d4-11d1-8b24-00a0c9068ff3
//
GUID g_CLSID_CplExt = { 0x42071714, 0x76d4, 0x11d1,
                        { 0x8b, 0x24, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}
                      };


DESK_EXTENSION_INTERFACE DeskInterface;

static const DWORD sc_PanningHelpIds[] =
{
   0, 0
};

///////////////////////////////////////////////////////////////////////////////
//
// Messagebox wrapper
//
///////////////////////////////////////////////////////////////////////////////


int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    DWORD dwTitleID,
    DWORD dwTextID)
{
    TCHAR Title[256];
    TCHAR Text[2000];

    LoadString(g_hInst, dwTextID, Text, SIZEOF(Text));
    LoadString(g_hInst, dwTitleID, Title, SIZEOF(Title));

    return (MessageBox(hwnd, Text, Title, fuStyle));
}


//---------------------------------------------------------------------------
//
// PropertySheeDlgProc()
//
//  The dialog procedure for the "Panning" property sheet page.
//
//---------------------------------------------------------------------------
BOOL
CALLBACK
PropertySheeDlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{

    return TRUE;
}
