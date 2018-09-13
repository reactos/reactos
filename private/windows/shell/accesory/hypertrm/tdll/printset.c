/*	File: D:\WACKER\tdll\printset.c (Created: 02-Feb-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 8/18/99 10:52a $
 */
#include <windows.h>
#pragma hdrstop

#include <stdio.h>
#include <limits.h>
#include <term\res.h>

#include "stdtyp.h"
#include "mc.h"
#include "misc.h"
#include "assert.h"
#include "print.h"
#include "print.hh"
#include "globals.h"
#include "session.h"
#include "term.h"
#include "tdll.h"
#include "tchar.h"
#include "load_res.h"
#include "open_msc.h"
#include "open_msc.h"
#include "sf.h"
#include "file_msc.h"

static int printsetPrintToFile(const HPRINT hPrint);
static UINT_PTR APIENTRY printPageSetupHook( HWND hdlg, UINT uiMsg, WPARAM wParam,
                                         LPARAM lParam );

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printsetSetup
 *
 * DESCRIPTION:
 *	This function is used to display the common print dialogs.
 *
 * ARGUMENTS:
 *	hPrint	-	An external print handle.
 *	hwnd	-	owner window handle
 *
 * RETURNS:
 *	void
 *
 */
void printsetSetup(const HPRINT hPrint, const HWND hwnd)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
#ifdef INCL_USE_NEWPRINTDLG
    PRINTDLGEX pd;
    HRESULT hResult;
#else
    PRINTDLG pd;
#endif
	LPDEVNAMES	pstDevNames;
	PDEVMODE	pstDevMode;

	TCHAR	*pszPrinterName;
	TCHAR	*pTemp;
	DWORD	dwSize;
	DWORD	dwError;

	#if defined(_DEBUG)
	TCHAR ach[100];
	#endif

	if (hhPrint == 0)
		{
		assert(FALSE);
		return;
		}

	// Initialize basic structure elements.
	//
#ifdef INCL_USE_NEWPRINTDLG
	memset (&pd, 0, sizeof(PRINTDLGEX));
	pd.lStructSize = sizeof(PRINTDLGEX);
    pd.Flags2 = 0;
    pd.nStartPage = START_PAGE_GENERAL;
    pd.dwResultAction = 0;
#else
    memset (&pd, 0, sizeof(PRINTDLG));
	pd.lStructSize = sizeof(PRINTDLG);
#endif
	pd.Flags = PD_NOWARNING | PD_NOPAGENUMS;
	pd.hwndOwner = hwnd;

	if (SendMessage(sessQueryHwndTerminal(hhPrint->hSession),
						WM_TERM_Q_MARKED, 0, 0))
		{
		pd.Flags |= PD_SELECTION;
		}

	// Use the previously stored information to initialize the print
	// common dialogs.	printGetDefaults initializes this information
	// when a print handle is created.
	//
	if (hhPrint->pstDevMode)
		{
		// Allocate memory for the DEVMODE information and then
		// initialize it with the stored values from the Print Handle.
		//
		dwSize = hhPrint->pstDevMode->dmSize +
					hhPrint->pstDevMode->dmDriverExtra;

		if ((pd.hDevMode = GlobalAlloc(GMEM_MOVEABLE, dwSize)))
			{
			if ((pstDevMode = GlobalLock(pd.hDevMode)))
				{
                if (dwSize)
				    MemCopy(pstDevMode, hhPrint->pstDevMode, dwSize);
				GlobalUnlock(pd.hDevMode);
				}
			}
		}

	if (hhPrint->pstDevNames)
		{
		// Allocate memory for the DEVNAMES structure in pd, then
		// initialize it with the stored values from the Print Handle.
		// This sequence determines the variable structure size of
		// DEVNAMES.
		//
		pTemp = (TCHAR *)hhPrint->pstDevNames;
		pTemp += hhPrint->pstDevNames->wOutputOffset;
		pTemp += StrCharGetByteCount(pTemp) + sizeof(TCHAR);

		dwSize = (DWORD)(pTemp - (TCHAR*)hhPrint->pstDevNames);

		if ((pd.hDevNames = GlobalAlloc(GMEM_MOVEABLE, dwSize)))
			{
			if ((pstDevNames = GlobalLock(pd.hDevNames)))
				{
                if (dwSize)
				    MemCopy(pstDevNames, hhPrint->pstDevNames, dwSize);
				GlobalUnlock(pd.hDevNames);
				}
			}
		}

	// Initialize the PrintToFilename array every time before we go
	// into the dialog.
	//
	TCHAR_Fill(hhPrint->achPrintToFileName,
				TEXT('\0'),
				sizeof(hhPrint->achPrintToFileName) / sizeof(TCHAR));

	// Display the dialog.
	//
#ifdef INCL_USE_NEWPRINTDLG
    pd.Flags2 = 0;
    hResult = PrintDlgEx( &pd );
    if ( hResult != S_OK)
#else
    if (!PrintDlg(&pd))
#endif
        {
		dwError = CommDlgExtendedError();

		#if defined(_DEBUG)
		if (dwError != 0)
			{
			wsprintf(ach, "PrintDlg error 0x%x", dwError);
			MessageBox(hwnd, ach, "Debug", MB_ICONINFORMATION | MB_OK);
			}
		#endif


		// If user canceled, we're done
		//
		if (dwError == 0)
			goto Cleanup;

		// Some error occured, try to bring-up dialog with default
		// data.
		//
		if (pd.hDevNames)
			{
			GlobalFree(pd.hDevNames);
			pd.hDevNames = 0;
			}

		if (pd.hDevMode)
			{
			GlobalFree(pd.hDevMode);
			pd.hDevMode = 0;
			}

		pd.Flags &= ~PD_NOWARNING;

#ifdef INCL_USE_NEWPRINTDLG
        hResult = PrintDlgEx(&pd);
        if ( hResult != S_OK)
#else
        if (!PrintDlg(&pd))
#endif
			{
			#if defined(_DEBUG)
			dwError = CommDlgExtendedError();

			if (dwError != 0)
				{
				wsprintf(ach, "PrintDlg error 0x%x", dwError);
				MessageBox(hwnd, ach, "Debug", MB_ICONINFORMATION | MB_OK);
				}
			#endif

			goto Cleanup;
			}
		}

#ifdef INCL_USE_NEWPRINTDLG
    // in the NT 5 print dialog, if the user cancels, the print dialog returns S_OK.
    // So we need to check the result code to see if we should save the settings.
    if ( pd.dwResultAction == PD_RESULT_CANCEL )
        goto Cleanup;
#endif

    // Store the flags returned from the dialog in the Print Handle.
	// This has several pieces of info that will be used by the actual
	// printing  routines (i.e. print all, print selected).
	//
	hhPrint->nSelectionFlags = pd.Flags;

	// Store the printer name and location in the Print Handle
	// every time.
	//
	pstDevNames = GlobalLock(pd.hDevNames);
	if (!pstDevNames)
		{
		assert(FALSE);
		GlobalUnlock(pd.hDevMode);
		goto Cleanup;
		}
	pszPrinterName = (TCHAR *)pstDevNames;
	pszPrinterName += pstDevNames->wDeviceOffset;
	StrCharCopy(hhPrint->achPrinterName, pszPrinterName);
	GlobalUnlock(pd.hDevNames);

	// Save the DEVMODE information in the Print Handle.  This memory
	// must be freed and allocated every time as the size of the
	// DEVMODE structure changes.
	//
	pstDevMode = GlobalLock(pd.hDevMode);
	dwSize = pstDevMode->dmSize + pstDevMode->dmDriverExtra;

	if (hhPrint->pstDevMode)
		free(hhPrint->pstDevMode);

	hhPrint->pstDevMode = (LPDEVMODE)malloc(dwSize);

	if (hhPrint->pstDevMode == 0)
		{
		assert(FALSE);
		GlobalUnlock(pd.hDevMode);
		goto Cleanup;
		}

    if (dwSize)
        MemCopy(hhPrint->pstDevMode, pstDevMode, dwSize);
	GlobalUnlock(pd.hDevMode);

	// Save the DEVNAMES information in the Print Handle.  Because the
	// size of the information in this structure varies, it is freed and
	// allocated each time it is saved.
	//
	pstDevNames = GlobalLock(pd.hDevNames);

	// Determine the size of the structure.
	//
	pTemp = (TCHAR *)pstDevNames;
	pTemp += pstDevNames->wOutputOffset;
	pTemp += StrCharGetByteCount(pTemp) + sizeof(TCHAR);

	dwSize = (DWORD)(pTemp - (TCHAR*)pstDevNames);

	if (hhPrint->pstDevNames)
		{
		free(hhPrint->pstDevNames);
		hhPrint->pstDevNames = NULL;
		}

	hhPrint->pstDevNames = (LPDEVNAMES)malloc(dwSize);

	if (hhPrint->pstDevNames == 0)
		{
		assert(0);
		GlobalUnlock(pd.hDevNames);
		goto Cleanup;
		}

    if (dwSize)
        MemCopy(hhPrint->pstDevNames, pstDevNames, dwSize);
	GlobalUnlock(pd.hDevNames);

	// Has the user selected Print To File?  Yes, you do need to look
	// for the string "FILE:" to determinae this!  If so, we will put
	// up our own common dialog to get the save as file name.
	//
	pTemp = (CHAR *)hhPrint->pstDevNames +
				hhPrint->pstDevNames->wOutputOffset;

	if (StrCharCmp("FILE:", pTemp) == 0)
		{
		if (printsetPrintToFile(hPrint) != 0)
			{
			goto Cleanup;
			}
		}

#ifdef INCL_USE_NEWPRINTDLG
    // in the NT 5 print dialog, if the user cancels or click 'apply', the print dialog returns S_OK.
    // So we need to check the result code to see if we should now print
    if ( pd.dwResultAction != PD_RESULT_PRINT )
        goto Cleanup;
#endif

    // Print the selected text.
	//
	printsetPrint(hPrint);

	// Cleanup any memory that may have been allocated.
	//
	Cleanup:

	if (pd.hDevNames)
		GlobalFree(pd.hDevNames);

	if (pd.hDevMode)
		GlobalFree(pd.hDevMode);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printsetPrint
 *
 * DESCRIPTION:
 *	This function prints the selected text from then terninal and/or the
 *	backscroll buffer.
 *
 * ARGUMENTS:
 *	hPrint	-	An external Print handle.
 *
 * RETURNS:
 *	void
 *
 */
void printsetPrint(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
    int iLineHeight;
    int iVertRes;
	int 	iStatus,
			nRet;
    int iPrintableWidth;

    RECT stRect;

	DWORD	dwCnt, dwTime1, dwTime2;
	POINT	pX, pY;

	ECHAR *pechBuf;
	TCHAR	*pV,
			*pLS,
			*pLE,
			*pEnd,
			*pNext;

	if (hPrint == 0)
		{
		assert(FALSE);
		return;
		}

	// Get the text to print for selected text.
	//
	if (hhPrint->nSelectionFlags & PD_SELECTION)
		{
		if (!CopyMarkedTextFromTerminal(hhPrint->hSession,
											&pV,
											&dwCnt,
											FALSE))
			{
			if (pV)
				{
				free(pV);
				pV = NULL;
				}
			assert(FALSE);
			return;
			}

		if (dwCnt == 0)
			{
			if (pV)
				{
				free(pV);
				pV = NULL;
				}
			assert(FALSE);
			return;
			}
		}

	// Get the text to print into a buffer for an ALL selection.
	//
	else
		{
		pX.x = 0;
		pX.y = INT_MIN;

		pY.x = INT_MAX;
		pY.y = INT_MAX;

		nRet = CopyTextFromTerminal(hhPrint->hSession,
								&pX,
								&pY,
								&pechBuf,
								&dwCnt,
								FALSE);
		if (nRet == FALSE)
			{
			if (pechBuf)
				{
				free(pechBuf);
				pechBuf = NULL;
				}

			assert(FALSE);
			return;
			}

		// Strip Out Any repeated Characters in the string
		StrCharStripDBCSString(pechBuf,
		    (long)StrCharGetEcharByteCount(pechBuf), pechBuf);

		// hMem currently points to an array of ECHAR's, convert this to
		// TCHARS before giving the results to the caller.
		pV = malloc((ULONG)StrCharGetEcharByteCount(pechBuf) + 1);

		CnvrtECHARtoMBCS(pV, (ULONG)StrCharGetEcharByteCount(pechBuf) + 1,
				pechBuf,StrCharGetEcharByteCount(pechBuf)+1); // mrw:5/17/95

		free(pechBuf);
		pechBuf = NULL;
		dwCnt = (ULONG)StrCharGetByteCount(pV);
		}

	// Create the DC.
	//
	hhPrint->hDC = printCtrlCreateDC(hPrint);

	if (hhPrint->hDC == 0)
		{
		if (pV)
			{
			free(pV);
			pV = NULL;
			}

		assert(FALSE);
		return;
		}

    printSetFont( hhPrint );
    printSetMargins( hhPrint );

	// Initialize the DC.  Set the abort flag, determine the number
	// of lines per page, get the title of the window which will be used
	// as the name of the document to print.
	//
	hhPrint->fError = FALSE;
	hhPrint->fUserAbort = FALSE;
	hhPrint->nLinesPrinted = 0;
	hhPrint->nPage = 1;
	EnableWindow(sessQueryHwnd(hhPrint->hSession), FALSE);

	GetTextMetrics(hhPrint->hDC, &hhPrint->tm);

    iLineHeight = hhPrint->tm.tmHeight + hhPrint->tm.tmExternalLeading;
    iVertRes = GetDeviceCaps(hhPrint->hDC, VERTRES);
    iVertRes -= (hhPrint->marginsDC.top + hhPrint->marginsDC.bottom);

	if (iLineHeight == 0) //need to prevent a divide by zero error
		iLineHeight = 1;

	hhPrint->nLinesPerPage = max( iVertRes / iLineHeight, 1);

	GetWindowText(sessQueryHwnd(hhPrint->hSession),
					hhPrint->achDoc,
					sizeof(hhPrint->achDoc));

	// Create the Print Abort Dialog.
	//
	hhPrint->lpfnPrintDlgProc = printsetDlgProc;

	hhPrint->hwndPrintDlg = CreateDialogParam(glblQueryDllHinst(),
								MAKEINTRESOURCE(IDD_PRINTABORT),
								sessQueryHwnd(hhPrint->hSession),
								hhPrint->lpfnPrintDlgProc,
								(LPARAM)hhPrint);

	// Setup the Print Abort Procedure.
	//
	hhPrint->lpfnPrintAbortProc = printsetAbortProc;

	nRet = SetAbortProc(hhPrint->hDC,
					(ABORTPROC)hhPrint->lpfnPrintAbortProc);

	// Initialize and start the document.
	//
	hhPrint->di.cbSize = sizeof(DOCINFO);
	hhPrint->di.lpszDocName = hhPrint->achDoc;
	hhPrint->di.lpszDatatype = NULL;
	hhPrint->di.fwType = 0;

	// Initialize di.lpszOutput for either printing to a file,
	// or to a printer.
	//
	if (hhPrint->achPrintToFileName[0] == TEXT('\0'))
		{
		hhPrint->di.lpszOutput = (LPTSTR)NULL;
		}
	else
		{
		hhPrint->di.lpszOutput = (LPTSTR)hhPrint->achPrintToFileName;
		}

	// StartDoc.
	//
	iStatus = StartDoc(hhPrint->hDC, &hhPrint->di);
	DbgOutStr("\r\nStartDoc: %d", iStatus, 0, 0, 0, 0);
	if (iStatus == SP_ERROR)
		{
		printCtrlDeleteDC(hPrint);

		if (IsWindow(hhPrint->hwndPrintDlg))
				DestroyWindow(hhPrint->hwndPrintDlg);

		printTellError(hhPrint->hSession, hPrint, iStatus);

		assert(FALSE);
		return;
		}

	// StartPage.
	// Get more info on this.
	//
	iStatus = StartPage(hhPrint->hDC);
	DbgOutStr("\r\nStartPage: %d", iStatus, 0, 0, 0, 0);
    printSetFont( hhPrint );

	if (iStatus == SP_ERROR)
		{
		assert(FALSE);
		}

	// Move through the buffer that contins the text to print, and
	// get it done.
	//
	// pLS	= pointerLineStart
	// pLE	= pointerLineEnd
	// pEnd = pointerEndOfBuffer
	//
	pLS = pV;
	pLE = pV;
	pEnd = pV + (dwCnt - 1);

	while ((pLE <= pEnd) && !hhPrint->fError && !hhPrint->fUserAbort)
		{
		if (*pLE == TEXT('\r') || *pLE == TEXT('\0'))
			{
			pNext = pLE;

			// Remove trailing CR\LF\NULL as these mean nothing to
			// a Windows DC.
			//
			while (pLE >= pLS)
				{
				if (*pLE == TEXT('\r') || *pLE == TEXT('\n') ||
						*pLE == TEXT('\0'))
					{
					pLE--;
					continue;
					}

				break;
				}

			// Send the text out to the printer, bump the line count.
			//

            hhPrint->cx = hhPrint->marginsDC.left;
            hhPrint->cy = hhPrint->nLinesPrinted * hhPrint->tm.tmHeight +
                          hhPrint->marginsDC.top;

            iPrintableWidth = GetDeviceCaps( hhPrint->hDC, HORZRES );
            iPrintableWidth -= hhPrint->marginsDC.right;

            stRect.left   = hhPrint->cx;
            stRect.right  = iPrintableWidth;
            stRect.top    = hhPrint->cy;
            stRect.bottom = hhPrint->cy + hhPrint->tm.tmHeight;

            ExtTextOut( hhPrint->hDC, hhPrint->cx, hhPrint->cy, ETO_CLIPPED,
                        &stRect, pLS, (UINT)((pLE - pLS) + 1), NULL );

			hhPrint->nLinesPrinted += 1;

			if (hhPrint->nLinesPrinted == 1)
				{
				DbgOutStr("\r\nPost WM_PRINT_NEWPAGE", 0, 0, 0, 0, 0);
				PostMessage(hhPrint->hwndPrintDlg,
								WM_PRINT_NEWPAGE,
								0,
								(LPARAM)hhPrint);
				}

			// Check for a new page condition.
			//
			if ((hhPrint->nLinesPrinted >= hhPrint->nLinesPerPage))
				{
				hhPrint->nLinesPrinted = 0;
				hhPrint->nPage++;

				iStatus = EndPage(hhPrint->hDC);
				if (iStatus < 0)
					{
					hhPrint->fError = TRUE;
					printTellError(hhPrint->hSession, hPrint, iStatus);
					}
				else
					{
					iStatus = StartPage(hhPrint->hDC);
					DbgOutStr("\r\nStartPage: %d", iStatus, 0, 0, 0, 0);
                    printSetFont( hhPrint );

					if (iStatus <= 0)
						{
						DbgShowLastError();
						printTellError(hhPrint->hSession, hPrint, iStatus);
						}
					}
				}

			pLS = pLE = (pNext + 1);
			continue;
			}

		pLE++;
		}

	// Did we issue an EndPage for this page yet?
	//
	if (hhPrint->nLinesPrinted > 0)
		{
		iStatus = EndPage(hhPrint->hDC);
		DbgOutStr("\r\nEndPage: %d", iStatus, 0, 0, 0, 0);
		if (iStatus <= 0)
			{
			DbgShowLastError();
			printTellError(hhPrint->hSession, hPrint, iStatus);
			}
		}

	// Call EndDoc.
	//
	iStatus = EndDoc(hhPrint->hDC);
	DbgShowLastError();
	DbgOutStr("\r\nEndDoc: %d", iStatus, 0, 0, 0, 0);
	printTellError(hhPrint->hSession, hPrint, iStatus);

	// Final cleanup before exit.
	//
	if (!hhPrint->fUserAbort)
		{
		dwTime1 = (DWORD)GetWindowLongPtr(hhPrint->hwndPrintDlg, GWLP_USERDATA);
		dwTime2 = GetTickCount();
		if (dwTime2 - dwTime1 < 3000)
			Sleep( 3000 - (dwTime2 - dwTime1));

		EnableWindow(sessQueryHwnd(hhPrint->hSession), TRUE);
		DestroyWindow(hhPrint->hwndPrintDlg);
		}

	printCtrlDeleteDC(hPrint);

	if (pV)
		{
		free(pV);
		pV = NULL;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printsetAbortProc
 *
 * DESCRIPTION:
 *	This is the print abort procedure used when printing selected text.
 *	Note that this abort proc is not used for print echo, or for host
 *	directed printing.
 *
 * ARGUMENTS:
 *	HDC 	-	A printer DC.
 *	nCode	-	The status of the call.
 *
 * RETURNS:
 *	The abort status.
 *
 */
BOOL CALLBACK printsetAbortProc(HDC hdcPrn, INT nCode)
	{
	MSG msg;

	HHPRINT hhPrint = (HHPRINT)printCtrlLookupDC(hdcPrn);

	DbgOutStr("\r\nprintsetAbortProc Code: %d", nCode, 0, 0, 0, 0);

	while (!hhPrint->fUserAbort &&
				PeekMessage((LPMSG)&msg, (HWND)0, 0, 0, PM_REMOVE))
		{
		if (!hhPrint->hwndPrintDlg ||
				!IsDialogMessage(hhPrint->hwndPrintDlg, &msg))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		}

	return !hhPrint->fUserAbort;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printsetDlgProc
 *
 * DESCRIPTION:
 *	This is the dialog procedure for printing selected text.  It contains
 *	a CANCEL button that may be used to abort the printing process.
 *
 * ARGUMENTS:
 *	Note that a print handle is passed into this procedure in the lPar
 *	variable.
 *
 * RETURNS:
 *
 */
LRESULT CALLBACK printsetDlgProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	TCHAR achBuf[80];
	TCHAR achMessage[80];
	DWORD dwTime;
	HHPRINT hhPrint;
	LPTSTR	acPtrs[3];

	switch (uMsg)
		{
		case WM_INITDIALOG:
			DbgOutStr("\r\nprintsetDlgProc", 0, 0, 0, 0, 0);
			hhPrint = (HHPRINT)lPar;
			SetWindowLongPtr(hwnd, DWLP_USER, lPar);

			mscCenterWindowOnWindow(hwnd, sessQueryHwnd(hhPrint->hSession));
			LoadString(glblQueryDllHinst(),
						IDS_PRINT_NOW_PRINTING,
						achBuf,
						sizeof(achBuf) / sizeof(TCHAR));
			wsprintf(achMessage, achBuf, 1);
			SetDlgItemText(hwnd, 101, achMessage);
			LoadString(glblQueryDllHinst(),
							IDS_PRINT_OF_DOC,
							achBuf,
							sizeof(achBuf) / sizeof(TCHAR));
			wsprintf(achMessage, achBuf, hhPrint->achDoc);
			SetDlgItemText(hwnd, 102, achMessage);
			LoadString(glblQueryDllHinst(),
							IDS_PRINT_ON_DEV,
							achBuf,
							sizeof(achBuf) / sizeof(TCHAR));

			acPtrs[0] = hhPrint->achPrinterName;
			acPtrs[1] = (TCHAR *)hhPrint->pstDevNames +
						hhPrint->pstDevNames->wOutputOffset;

			FormatMessage(
				FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				achBuf,
				0,				  /* Message ID, ignored */
				0,				  /* also ignored */
				achMessage, 	  /* result */
				sizeof(achMessage),
				(va_list *)&acPtrs[0]);

			SetDlgItemText(hwnd, 103, achMessage);

			dwTime = GetTickCount();
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)dwTime);
			return TRUE;

		case WM_COMMAND:
			DbgOutStr("\r\nprintsetDlgProc - CANCEL", 0, 0, 0, 0, 0);
			hhPrint = (HHPRINT)GetWindowLongPtr(hwnd, DWLP_USER);
			hhPrint->fUserAbort = TRUE;
			EnableWindow(sessQueryHwnd(hhPrint->hSession), TRUE);
			DestroyWindow(hwnd);
			hhPrint->hwndPrintDlg = 0;
			return TRUE;

		case WM_PRINT_NEWPAGE:
			DbgOutStr("\r\nprintsetDlgProc", 0, 0, 0, 0, 0);
			hhPrint = (HHPRINT)GetWindowLongPtr(hwnd, DWLP_USER);
			LoadString(glblQueryDllHinst(),
						IDS_PRINT_NOW_PRINTING,
						achBuf,
						sizeof(achBuf) / sizeof(TCHAR));
			wsprintf(achMessage, achBuf, hhPrint->nPage);
			SetDlgItemText(hwnd, 101, achMessage);
			return TRUE;

		default:
			break;
		}

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printPageSetup
 *
 * DESCRIPTION:
 *	Invokes the common page-setup dialog
 *
 * ARGUMENTS:
 *	HPRINT	hPrint	- public print handle
 *	HWND	hwnd	- window handle used for parent
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
int printPageSetup(const HPRINT hPrint, const HWND hwnd)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;
	PAGESETUPDLG psd;
	LPDEVNAMES	pstDevNames;
	PDEVMODE	pstDevMode;

	TCHAR	*pszPrinterName;
	TCHAR	*pTemp;
	DWORD	dwSize;
    RECT    stMinMargins = {0,0,0,0};

	if (hPrint == 0)
		return -1;

	memset(&psd, 0, sizeof(psd));

	psd.lStructSize = sizeof(psd);
	psd.hwndOwner   = hwnd;
    psd.hInstance   = glblQueryDllHinst();

    psd.Flags       = PSD_ENABLEPAGESETUPTEMPLATE | PSD_ENABLEPAGESETUPHOOK |
                       PSD_MINMARGINS | PSD_MARGINS;

    psd.rtMargin    = hhPrint->margins;
    psd.rtMinMargin = stMinMargins;
    psd.lCustData   = (LPARAM)hhPrint;

    psd.lpPageSetupTemplateName = MAKEINTRESOURCE(IDD_CUSTOM_PAGE_SETUP);
    psd.lpfnPageSetupHook       = printPageSetupHook;

	// Use the previously stored information to initialize the print
	// common dialogs.	printGetDefaults initializes this information
	// when a print handle is created.
	//
	if (hhPrint->pstDevMode)
		{
		// Allocate memory for the DEVMODE information and then
		// initialize it with the stored values from the Print Handle.
		//
		dwSize = hhPrint->pstDevMode->dmSize +
					hhPrint->pstDevMode->dmDriverExtra;

		if ((psd.hDevMode = GlobalAlloc(GMEM_MOVEABLE, dwSize)))
			{
			if ((pstDevMode = GlobalLock(psd.hDevMode)))
				{
                if (dwSize)
				    MemCopy(pstDevMode, hhPrint->pstDevMode, dwSize);
				GlobalUnlock(psd.hDevMode);
				}
			}
		}

	if (hhPrint->pstDevNames)
		{
		// Allocate memory for the DEVNAMES structure in pd, then
		// initialize it with the stored values from the Print Handle.
		// This sequence determines the variable structure size of
		// DEVNAMES.
		//
		pTemp = (TCHAR *)hhPrint->pstDevNames;
		pTemp += hhPrint->pstDevNames->wOutputOffset;
		pTemp += StrCharGetByteCount(pTemp) + sizeof(TCHAR);

		dwSize = (DWORD)(pTemp - (TCHAR*)hhPrint->pstDevNames);

		if ((psd.hDevNames = GlobalAlloc(GMEM_MOVEABLE, dwSize)))
			{
			if ((pstDevNames = GlobalLock(psd.hDevNames)))
				{
                if (dwSize)
				    MemCopy(pstDevNames, hhPrint->pstDevNames, dwSize);
				GlobalUnlock(psd.hDevNames);
				}
			}
		}

	if (!PageSetupDlg(&psd))
		{
		#if defined(_DEBUG)
	    TCHAR ach[100];
		DWORD dwError = CommDlgExtendedError();

		if (dwError != 0)
			{
			wsprintf(ach, "PrintDlg error 0x%x", dwError);
			MessageBox(hwnd, ach, "Debug", MB_ICONINFORMATION | MB_OK);
			}
		#endif

		return -2;
		}

    // store the margin settings in the print handle.
    //
    hhPrint->margins = psd.rtMargin;

	// Store the printer name and location in the Print Handle
	// every time.
	//
	pstDevNames = GlobalLock(psd.hDevNames);
	pszPrinterName = (TCHAR *)pstDevNames;
	pszPrinterName += pstDevNames->wDeviceOffset;
	StrCharCopy(hhPrint->achPrinterName, pszPrinterName);
	GlobalUnlock(psd.hDevNames);

	// Save the DEVMODE information in the Print Handle.  This memory
	// must be freed and allocated every time as the size of the
	// DEVMODE structure changes.
	//
	pstDevMode = GlobalLock(psd.hDevMode);
	dwSize = pstDevMode->dmSize + pstDevMode->dmDriverExtra;

	if (hhPrint->pstDevMode)
		{
		free(hhPrint->pstDevMode);
		hhPrint->pstDevMode = NULL;
		}

	hhPrint->pstDevMode = (LPDEVMODE)malloc(dwSize);

	if (hhPrint->pstDevMode == 0)
		{
		assert(FALSE);
		GlobalUnlock(psd.hDevMode);
		goto Cleanup;
		}

    if (dwSize)
        MemCopy(hhPrint->pstDevMode, pstDevMode, dwSize);
	GlobalUnlock(psd.hDevMode);

	// Save the DEVNAMES information in the Print Handle.  Because the
	// size of the information in this structure varies, it is freed and
	// allocated each time it is saved.
	//
	pstDevNames = GlobalLock(psd.hDevNames);

	// Determine the size of the structure.
	//
	pTemp = (TCHAR *)pstDevNames;
	pTemp += pstDevNames->wOutputOffset;
	pTemp += StrCharGetByteCount(pTemp) + sizeof(TCHAR);

	dwSize = (DWORD)(pTemp - (TCHAR*)pstDevNames);

	if (hhPrint->pstDevNames)
		{
		free(hhPrint->pstDevNames);
		hhPrint->pstDevNames = NULL;
		}

	hhPrint->pstDevNames = (LPDEVNAMES)malloc(dwSize);

	if (hhPrint->pstDevNames == 0)
		{
		assert(0);
		GlobalUnlock(psd.hDevNames);
		goto Cleanup;
		}
    if (dwSize)
	    MemCopy(hhPrint->pstDevNames, pstDevNames, dwSize);
	GlobalUnlock(psd.hDevNames);


	// Cleanup any memory that may have been allocated.
	//
    Cleanup:

	if (psd.hDevNames)
		GlobalFree(psd.hDevNames);

	if (psd.hDevMode)
		GlobalFree(psd.hDevMode);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	  printPageSetupHook
 *
 * DESCRIPTION:
 *    A hook function to process the font button on the page setup dialog
 *
 * ARGUMENTS:
 *   hdlg   - handle to the dialog box window
 *   uiMsg  - message identifier
 *   wParam - message parameter
 *   lParam - message parameter
 *	
 * RETURNS:
 *	0 = message not processed 1 = message processeed
 *
 * Author:
 *  Dwayne Newsome 02/19/97
 *
 */

static UINT_PTR APIENTRY printPageSetupHook( HWND hdlg, UINT uiMsg, WPARAM wParam,
                                         LPARAM lParam )
    {
    static HHPRINT  hhPrint;
    static PAGESETUPDLG * pPageSetup;

    UINT processed = 0;

	LPDEVNAMES	pstDevNames;
	TCHAR *     pszPrinterName;

    //
    // on the init dialog message save a pointer to the pagesetup dialog and
    // save the print handle
    //

    if ( uiMsg == WM_INITDIALOG )
        {
        pPageSetup = ( PAGESETUPDLG *) lParam;
        hhPrint = (HHPRINT) pPageSetup->lCustData;
        }

    //
    // Looking for the font button click here, if we get it set the currently
    // selected printers name in the saved print handle and display the font
    // dialog.  We save the printer name so the font dialog can show the
    // correct fonts for the currently selected printer.
    //

    else if ( uiMsg == WM_COMMAND )
        {
        if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == 1027 )
            {
            processed = 1;
        	pstDevNames = GlobalLock(pPageSetup->hDevNames);
        	pszPrinterName = (TCHAR *)pstDevNames;
        	pszPrinterName += pstDevNames->wDeviceOffset;
        	StrCharCopy(hhPrint->achPrinterName, pszPrinterName);
        	GlobalUnlock(pPageSetup->hDevNames);

            DisplayFontDialog( hhPrint->hSession, TRUE );
            }
        }

    return processed;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printsetPrintToFile
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *	HPRINT	hPrint	- public print handle
 *
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
static int printsetPrintToFile(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;

	TCHAR		acTitle[64],
				acMask[128],
				acDir[256],
				acFile[256];

	LPTSTR		pszPrintFile;

	HWND		hWnd;
	HINSTANCE	hInst;

	LPTSTR		pszStr;

	TCHAR_Fill(acTitle, 	TEXT('\0'), sizeof(acTitle) / sizeof(TCHAR));
	TCHAR_Fill(acMask,		TEXT('\0'), sizeof(acMask) / sizeof(TCHAR));
	TCHAR_Fill(acDir,		TEXT('\0'), sizeof(acDir) / sizeof(TCHAR));
	TCHAR_Fill(acFile,		TEXT('\0'), sizeof(acFile) / sizeof(TCHAR));

	hWnd = glblQueryHwndFrame();
	hInst = glblQueryDllHinst();

	LoadString(hInst,
				IDS_PRINT_TOFILE,
				acTitle,
				sizeof(acTitle) / sizeof(TCHAR));

	LoadString(hInst,
				IDS_PRINT_FILENAME,
				acFile,
				sizeof(acFile) / sizeof(TCHAR));

	resLoadFileMask(hInst,
						IDS_PRINT_FILTER_1,
						2,
						acMask,
						sizeof(acMask) / sizeof(TCHAR));

	// Figure out which directory to propose to the user for the 'print to'
	// file.  If we have a session file, use that session files directory,
	// otherwise use the current directory.
	//
	if (sfGetSessionFileName(sessQuerySysFileHdl(hhPrint->hSession),
								sizeof(acDir) / sizeof(TCHAR),
								acDir) == SF_OK)
		{
		mscStripName(acDir);
		}
	else
		{
		//Changed to use working folder rather than current folder - mpt 8-18-99
		if ( !GetWorkingDirectory( acDir, sizeof(acDir) / sizeof(TCHAR)) )
			{
			GetCurrentDirectory(sizeof(acDir) / sizeof(TCHAR), acDir);
			}
		}

	pszStr = StrCharLast(acDir);

	// Remove trailing backslash from the directory name if there is one.
	//
	if (pszStr && *pszStr == TEXT('\\'))
		*pszStr = TEXT('\0');

	pszPrintFile = gnrcSaveFileDialog(hWnd,
										(LPCTSTR)acTitle,
										(LPCTSTR)acDir,
										(LPCTSTR)acMask ,
										(LPCTSTR)acFile);

	if (pszPrintFile == NULL)
		{
		return(1);
		}

	// pszPrintFile gets allocated in gnrcSaveFileDlg.
	//
	StrCharCopy(hhPrint->achPrintToFileName, pszPrintFile);
	free(pszPrintFile);
	pszPrintFile = NULL;

	return(0);

	}
