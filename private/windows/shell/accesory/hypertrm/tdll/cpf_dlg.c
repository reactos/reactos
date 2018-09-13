/*	File: C:\WACKER\TDLL\CPF_DLG.C (Created: 12-Jan-94)
 *
 *	Copyright 1990,1993,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 3/26/99 8:07a $
 */
#include <windows.h>
#pragma hdrstop

#define	DO_RAW_MODE	1

#include "stdtyp.h"
#include "mc.h"

#include <term\res.h>
#include <tdll\tdll.h>
#include <tdll\misc.h>
#include <tdll\assert.h>
#include <tdll\globals.h>
#include <tdll\session.h>
#include <tdll\capture.h>
#include <tdll\load_res.h>
#include <tdll\open_msc.h>
#include <tdll\errorbox.h>
#include <tdll\hlptable.h>
#include "tchar.h"
#include "file_msc.h"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	/*
	 * Put in whatever else you might need to access later
	 */
	HSESSION hSession;
	};

typedef	struct stSaveDlgStuff SDS;

#define IDC_TF_FILE     100
#define	FNAME_EDIT		105
#define IDC_TF_DIR      106
#define	DIRECTORY_TEXT	107
#define BROWSE_BTN		123
#define IDC_PB_START    124

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CaptureFileDlg
 *
 * DESCRIPTION:
 *	This is the dialog proc for the capture to file dialog.  No suprises
 *	here.
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK CaptureFileDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	BOOL    fRc;
	DWORD dwMaxComponentLength;
	DWORD dwFileSystemFlags;
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	HSESSION hSession;
	LPTSTR  pszPtr;
	LPTSTR	pszStr;
	TCHAR   acBuffer[FNAME_LEN];
	static	DWORD aHlpTable[] = {FNAME_EDIT,		IDH_TERM_CAPT_FILENAME,
								 IDC_TF_FILE,		IDH_TERM_CAPT_FILENAME,
								 BROWSE_BTN,		IDH_BROWSE,
								 IDC_TF_DIR,		IDH_TERM_CAPT_DIRECTORY,
								 DIRECTORY_TEXT,	IDH_TERM_CAPT_DIRECTORY,
                                 IDC_PB_START,      IDH_TERM_CAPT_START,
                                 IDCANCEL,                           IDH_CANCEL,
                                 IDOK,                               IDH_OK,
								 0, 				0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		pS->hSession = (HSESSION)lPar;

		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

		hSession = pS->hSession;

		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		// Determine whether long filenames are supported.  JRJ	12/94
		fRc = GetVolumeInformation(NULL,  // pointer to root dir path buffer
								 NULL,	  // pointer to volume name buffer
								 0,		  // length of volume name buffer
								 NULL,    // pointer to volume serial number buffer
								 &dwMaxComponentLength,	// the prize - what I'm after
								 &dwFileSystemFlags,  // ptr to file system flag DWORD
								 NULL,	  // pointer to file system name buffer
								 0);	  // length of file system name buffer

		if(dwMaxComponentLength == 255)
			{
			// There is support for long file names.
			SendDlgItemMessage(hDlg, FNAME_EDIT, EM_SETLIMITTEXT, FNAME_LEN, 0);
			}
		else
			{
			// There IS NOT support for long file names. Limit to twelve.
			SendDlgItemMessage(hDlg, FNAME_EDIT, EM_SETLIMITTEXT, 12, 0);
			}

		/* Get the file name first */
		acBuffer[0] = TEXT('\0');
		cpfGetCaptureFilename(sessQueryCaptureFileHdl(hSession),
								acBuffer,
								sizeof(acBuffer) / sizeof(TCHAR));
		SetDlgItemText(hDlg, FNAME_EDIT, acBuffer);

		for (pszPtr = pszStr = acBuffer;
			*pszStr != TEXT('\0');
			pszStr = StrCharNext(pszStr))
			{
			if (*pszStr == TEXT('\\'))
				pszPtr = pszStr;
			}

		if (*pszPtr == TEXT('\\'))
			{
			*pszPtr = TEXT('\0');
			SetDlgItemText(hDlg, DIRECTORY_TEXT, acBuffer);
			}

		break;

	case WM_DESTROY:
		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:

		/*
		 * Did we plan to put a macro in here to do the parsing ?
		 */
		DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

		switch (nId)
			{
		case IDC_PB_START:
			{
			int nDef;

			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			assert(pS);

			hSession = pS->hSession;

			/*
			 * Do whatever saving is necessary
			 */
			nDef = TRUE;

			acBuffer[0] = TEXT('\0');
			GetDlgItemText(hDlg,
							FNAME_EDIT,
							acBuffer,
							sizeof(acBuffer) / sizeof(TCHAR));

			// Error check the user-supplied name.
			if(ValidateFileName(acBuffer) == 1)
				{
				cpfSetCaptureFilename(sessQueryCaptureFileHdl(hSession),
										acBuffer, nDef);


#if defined(DO_RAW_MODE)
				cpfSetCaptureMode(sessQueryCaptureFileHdl(hSession),
									CPF_MODE_RAW,
									FALSE);
#endif

				/*
				 * TODO: actually start the capture to file
				 */
				cpfSetCaptureState(sessQueryCaptureFileHdl(hSession),
									CPF_CAPTURE_ON);

				/* Free the storage */
				free(pS);
				pS = (SDS *)0;
				EndDialog(hDlg, TRUE);
				}
			else
				{
				// There were problems.
				//   e.g. The user specified a bad capture file name, or
				//        for some other reason the file couldn't be created.
				MessageBeep(MB_ICONHAND);

				// For now, I'm going to assume that whatever the problem is,
				//  the only thing the user can do to try again is to specify
				//  a different filename. So, I'm setting the focus back to
				//  the filename edit control.
				SetFocus(GetDlgItem(hDlg,FNAME_EDIT));
				}
			}
			break;

		case IDCANCEL:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			EndDialog(hDlg, FALSE);
			break;

		case BROWSE_BTN:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			if (pS)
				{
				LPTSTR pszRet;
				TCHAR acTitle[64];
				TCHAR acList[64];

				hSession = pS->hSession;

				pszRet = NULL;
				TCHAR_Fill(acBuffer,
							(TCHAR)0,
							sizeof(acBuffer)/sizeof(TCHAR));
				GetDlgItemText(hDlg,
								DIRECTORY_TEXT,
								acBuffer,
								(sizeof(acBuffer) / sizeof(TCHAR)) - 1);

				LoadString(glblQueryDllHinst(),
							IDS_CPF_DLG_FILE,
							acTitle,
							sizeof(acTitle) / sizeof(TCHAR));

				resLoadFileMask(glblQueryDllHinst(),
							IDS_CPF_FILES1,
							2,
							acList,
							sizeof(acList) / sizeof(TCHAR));

                //jmh 3/24/97 This was gnrcFindFileDialog, but this lets you
                // enter a non-existent file name, which is what we really want
				pszRet = gnrcSaveFileDialog(hDlg,
							acTitle,
							acBuffer,
							acList,
                            "");

				if (pszRet != NULL)
					{
					StrCharCopy(acBuffer, pszRet);
					SetDlgItemText(hDlg, FNAME_EDIT, pszRet);
					free(pszRet);
					pszRet = NULL;
					pszStr = acBuffer;
					pszRet = pszStr;
					for (; *pszStr != TEXT('\0'); pszStr = StrCharNext(pszStr))
						{
						if (*pszStr == TEXT('\\'))
							pszRet = pszStr;
						}
					pszRet = StrCharNext(pszRet);
					*pszRet = '\0';
					SetDlgItemText(hDlg, DIRECTORY_TEXT, acBuffer);
					}
				}
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}
