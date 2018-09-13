/** FILE: conflict.c ******* Module Header ********************************
 *
 *  Control panel utlitiy library routine to manage conflict dialog box.
 *
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *
 *  Copyright (C) 1990-1991 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

//==========================================================================
//                            Local Definitions
//==========================================================================

//==========================================================================
//                            External Declarations
//==========================================================================


//==========================================================================
//                            Data Declarations
//==========================================================================
WORD nConfID;

//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================

BOOL ConflictDlg(HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    TCHAR szString[200];
    TCHAR szStr2[200];

    switch (message)
    {
    case WM_INITDIALOG:
        LoadString (hModule, nConfID, szString, CharSizeOf(szString));
        LoadString (hModule, (WORD) (nConfID+1), szStr2, CharSizeOf(szStr2));
        lstrcat (szString, szStr2);
        SetDlgItemText (hDlg, IDOK,  szString);
        LoadString (hModule, (WORD) (nConfID+2), szString, CharSizeOf(szString));
        SetWindowText (hDlg, szString);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
             goto DoHelp;

        case IDYES:
        case IDNO:
        case IDCANCEL:
             EndDialog(hDlg, wParam);
             break;
        }
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
            return TRUE;
        }
        else
            return FALSE;
        break;
    }
    return TRUE;

    UNREFERENCED_PARAMETER(lParam);
}
