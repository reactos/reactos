/*      File: D:\WACKER\tdll\upgrddlg.c (Created: 02-Dec-1996 by cab)
 *
 *      Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *  Description:
 *      Implements the "Upgrade Information" dialog. This dialog
 *      displays a simple rich text edit control and displays
 *      information on how to upgrade to our latest and greatest
 *      product.
 *
 *      $Revision: 2 $
 *      $Date: 2/05/99 3:21p $
 */

#include <windows.h>
#include <io.h>
#pragma hdrstop

#include <commctrl.h>
#include <term\res.h>

#include "mc.h"
#include "globals.h"
#include "features.h"
#include "richedit.h"
#include "shellapi.h"
#include "registry.h"

// Data structure to copy text into rich text edit control.
//
typedef struct esInfo_
    {
    LPTSTR lptstrText;
    LONG lBytesRead;
    } ESINFO;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      EditStreamCallback
 *
 * DESCRIPTION:
 *  This callback is used to read .rtf text into the rich edit control.
 *  Its a bit complicated to fill a rich edit control.  The callback
 *  mechanism for this control really was designed to work with file
 *  I/O but is generic enough for reading from any source.  The difficultly
 *  arises in that the callback may be called multiple times since
 *  the data being read may be much larger than the buffer allocated in
 *  pbBuff.  Thus, you have to keep a running count of how may bytes have
 *  been copied from your source buffer so that subsequent calls know
 *  where in the buffer to continue from.  Stick UNICODE into the
 *  equation and you have yourself a nice little puzzle.  Note, you have
 *  to think in bytes here since this is what the rich edit control
 *  thinks in.  That's why the ESINFO structure is used. - mrw
 *
 */
DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff,
    LONG cb, LONG *pcb)
    {
    int iLen;
    ESINFO *pstInfo = (ESINFO *)dwCookie;
    BYTE *pbText = (BYTE *)pstInfo->lptstrText + pstInfo->lBytesRead;

    iLen = lstrlen(pbText) + 1;     // get len in chars (DBCS returns bytes)
    iLen *= sizeof(TCHAR);          // adjust for UNICODE
    *pcb = min(cb, iLen);           // decide how much to copy
    MemCopy(pbBuff, pbText, *pcb);   // copy specified amount into buffer
    pstInfo->lBytesRead += *pcb;    // record how far we got.
    return (cb >= iLen) ? 0 : *pcb; // return bytes read or zero when done.
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      UpgradeDlgProc
 *
 * DESCRIPTION:
 *  Dialog procedure that displays a rich edit control and an OK button.
 *
 */
INT_PTR CALLBACK UpgradeDlgProc(HWND hDlg, UINT uMsg, WPARAM wPar, LPARAM lPar)
    {
    #define IDC_RICHED 100

    int    i;
    TCHAR  ach[257*2];
    static HANDLE hResource;
    static LPTSTR pachUpgrade;
    static EDITSTREAM es;
    static ESINFO esInfo;
    TCHAR  achUseRTF[80];
    BOOL   bUseRTF = TRUE;

    switch (uMsg)
	{
    case WM_INITDIALOG:

        if (LoadString (glblQueryDllHinst(),
                        IDS_USE_RTF,
                        achUseRTF,
                        sizeof(achUseRTF)))
            {

            // If IDS_USE_RTF changed to anything else by localizer, we
            //  assume upgrade text is already localized.

            if (0 != strcmp(achUseRTF, "True"))
                bUseRTF = FALSE;
            }


        if (!bUseRTF) {

            if ((pachUpgrade = malloc(257*2*50 * sizeof(TCHAR))) == 0)
                break;

            pachUpgrade[0] = TEXT('\0');

            for (i = IDS_UPGRADE ; i < IDS_UPGRADE + 50 ; ++i)
                {
                if (LoadString(glblQueryDllHinst(), i, ach, sizeof(ach)) == 0)
                    break;

                strcat(pachUpgrade, ach);
                }
        } else {

            // Upgrade text is a private resource.
            //
            hResource = LoadResource(glblQueryDllHinst(),
                FindResource(glblQueryDllHinst(),
                MAKEINTRESOURCE(IDR_UPGRADE_TEXT), "TEXT"));

            pachUpgrade = LockResource(hResource);

        }

	// Build a small structure that we'll give to the a callback
	// that fills the rich edit control.
	//
	esInfo.lptstrText = pachUpgrade;
	esInfo.lBytesRead = 0;

	// Setup the EDITSTREAM structure.
	//
	es.dwCookie = (DWORD_PTR)&esInfo;
	es.dwError = 0;
	es.pfnCallback = EditStreamCallback;

	// This message does not return until the control has
	// read all the text which makes it possible to release
	// the resource immediately after this call.
	//

        if (!bUseRTF) {
            SetDlgItemText(hDlg, IDC_RICHED, pachUpgrade);
            free(pachUpgrade);
            pachUpgrade = NULL;

        } else {
            SendDlgItemMessage(hDlg, IDC_RICHED, EM_STREAMIN, SF_RTF,
                (LPARAM)&es);

            // Kill the resource.
            //
            UnlockResource(hResource);
            FreeResource(hResource);
        }
        break;

    case WM_COMMAND:
	switch (wPar)
	    {
	case IDOK:
	case IDCANCEL:
	    EndDialog(hDlg, TRUE);
	    break;

	default:
	    break;
	    }
	break;

    default:
	return FALSE;
	}

    return TRUE;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      DoUpgradeDialog
 *
 * DESCRIPTION:
 *  Displays a modal "Upgrade Information" dialog box.
 *
 * ARGUMENTS:
 *  hwndParent - Handle of the parent window.
 *
 * AUTHOR:  C. Baumgartner, 12/02/96
 *          M. Thompson, 07/15/97 Attempt to launch htm file first
 */
INT_PTR DoUpgradeDialog(HWND hwndParent)
    {
    //check to see if we should launch browser with htm or do the rtf thing - mpt 07/15/97
	INT_PTR result;
    CHAR acRegistryData[MAX_PATH * 2];
	CHAR acExePath[MAX_PATH];
	CHAR acHTMFile[MAX_PATH];
    DWORD dwSize = sizeof(acRegistryData);
	BOOL bLaunchBrowser = FALSE;
	CHAR achHTMRegKey[] = TEXT(".htm");
	LPTSTR pszPtr;

	struct _finddata_t c_file;
	long hFile;

    //get .htm file type
	if ( regQueryValue(HKEY_CLASSES_ROOT, achHTMRegKey,
		TEXT(""), acRegistryData, &dwSize) == 0 )
			bLaunchBrowser = TRUE;


	if ( bLaunchBrowser )
		{
		// Get the path name of HyperTerminal and build path to HTM file
		//
		acExePath[0] = TEXT('\0');
		result = GetModuleFileName(glblQueryHinst(), acExePath, MAX_PATH);
		//strip off executable
		if (result != 0)
			{
			pszPtr = strrchr(acExePath, TEXT('\\'));
			*pszPtr = TEXT('\0');
			}
		
		//build path to htm
		acHTMFile[0] = TEXT('\0');
		strcat(acHTMFile, acExePath);
		strcat(acHTMFile, TEXT("\\"));
		strcat(acHTMFile, TEXT("readme.htm"));

		//check if file exists

		hFile = (long)_findfirst( acHTMFile, &c_file );
		if ( hFile != -1 )
			{
			//tell shell to start the htm file, since we are at this point
			//we know there is a registry association for htm files, whether it works
			//or not is beyond our control.
			ShellExecute(NULL, "open", acHTMFile, NULL, NULL, SW_SHOW);
			return 0;
			}

		}
	
		result = DialogBox(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_UPGRADE_INFO), hwndParent, UpgradeDlgProc);
	
	return result;
    }
