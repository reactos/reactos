/** FILE: addfile.c ******** Module Header ********************************
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

// Utility library
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

//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================

BOOL AddFileDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    TCHAR szString[200], szStr2[200];

    switch (message)
    {
    case WM_INITDIALOG:
        if (nDisk)
        {
            GetWindowText (hDlg,  szString, CharSizeOf(szString));
            if (nDisk < 0)     /* is this international??  */
            {
                nDisk = -nDisk;
                LoadString (hModule, INSTALLIT+2, szStr2, CharSizeOf(szStr2));
                LoadString (hModule, INITS+15, szSetupDir, CharSizeOf(szSetupDir));
            }
            else
            {
                LoadString (hModule, INSTALLIT+3, szStr2, CharSizeOf(szStr2));
            }
            lstrcat (szString, szStr2);
            SetWindowText (hDlg, szString);

            LoadString (hModule, INSTALLIT, szString, CharSizeOf(szString));
            wsprintf (szStr2, szString, szNull, nDisk, szDrv);
        }
        else
            LoadString (hModule, INSTALLIT+1, szStr2, CharSizeOf(szStr2));

        SetDlgItemText (hDlg, IDRETRY,  szStr2);
        SendDlgItemMessage (hDlg, COLOR_SAVE, EM_LIMITTEXT, PATHMAX - 20, 0L);
        SetDlgItemText (hDlg, COLOR_SAVE, szSetupDir);
        SendDlgItemMessage (hDlg, COLOR_SAVE, EM_SETSEL, 0, 32767);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            GetDlgItemText (hDlg, COLOR_SAVE,  szDirOfSrc, CharSizeOf(szDirOfSrc));

        case IDCANCEL:
            EndDialog (hDlg, wParam == IDOK);
            return TRUE;
        }

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
            return TRUE;
        }
        else
            return FALSE;
    }
    return FALSE;                        // Didn't process a message

    UNREFERENCED_PARAMETER(lParam);
}

