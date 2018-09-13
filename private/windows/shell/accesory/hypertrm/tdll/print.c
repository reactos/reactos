/*	File: D:\WACKER\tdll\print.c (Created: 14-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */
#include <windows.h>
#pragma hdrstop

//#define DEBUGSTR
#include <term\res.h>

#include "stdtyp.h"
#include "mc.h"
#include "misc.h"
#include "assert.h"
#include "globals.h"
#include "session.h"
#include "print.h"
#include "print.hh"
#include "errorbox.h"
#include "tdll.h"
#include "term.h"
#include "tchar.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void printTellError(const HSESSION hSession, const HPRINT hPrint,
							const INT iStatus)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR achBuf[256];
	TCHAR achTitle[256];

	if (iStatus < 0)
		{
		if (iStatus & SP_NOTREPORTED)
			{
			LoadString(glblQueryDllHinst(),
						IDS_PRINT_TITLE,
						achTitle,
						sizeof(achTitle) / sizeof(TCHAR));

			switch (iStatus)
				{
			case SP_OUTOFDISK:
				LoadString(glblQueryDllHinst(),
							IDS_PRINT_NOMEM,
							achBuf,
							sizeof(achBuf) / sizeof(TCHAR));

				TimedMessageBox(sessQueryHwnd(hhPrint->hSession),
									achBuf,
									achTitle,
									MB_ICONEXCLAMATION | MB_OK,
									0);
				break;

			case SP_OUTOFMEMORY:
				LoadString(glblQueryDllHinst(),
							IDS_PRINT_CANCEL,
							achBuf,
							sizeof(achBuf) / sizeof(TCHAR));

				TimedMessageBox(sessQueryHwnd(hhPrint->hSession),
									achBuf,
									achTitle,
									MB_ICONEXCLAMATION | MB_OK,
									0);

				break;

			case SP_USERABORT:
				break;

			default:
				if (hhPrint == 0 || !hhPrint->fUserAbort)
					{
					LoadString(glblQueryDllHinst(),
								IDS_PRINT_ERROR,
								achBuf,
								sizeof(achBuf) / sizeof(TCHAR));

					TimedMessageBox(sessQueryHwnd(hhPrint->hSession),
										achBuf,
										achTitle,
										MB_ICONEXCLAMATION | MB_OK,
										0);

					}
				break;
				}
			}
		}

	return;
	}

//*jcm
#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PrintKillJob
 *
 * DESCRIPTION:
 *	Kills a print job.	Called when session is closing and stuff is
 *	printing.
 *
 * ARGUMENTS:
 *	HSESSION	hSession	- external session handle.
 *
 * RETURNS:
 *	VOID
 *
 */
VOID PrintKillJob(HSESSION hSession)
	{
	HHPRINT hPr;
	INT 	iStatus = 0;
	HGLOBAL hMem;

	assert(hSession);

	// It is possible that the print job ended by the time we got
	// here so if the handle is 0, return quietly.

	hPr = (HHPRINT)mGetPrintHdl(hSession);

	if (hPr == (HHPRINT)0)
		return;

	/* -------------- Kill this print job ------------- */

	TimerDestroy(&hPr->hTimer);
	DbgOutStr("\r\nTimer Destroy in PrintKillJob\r\n", 0, 0, 0, 0, 0);

	if (hPr->hDC)
		{
		// Check if we issued an EndPage() for this page yet.

		if (hPr->nLines > 0)
			{
			if (HA5G.fIsWin30)
				iStatus = Escape(hPr->hDC, NEWFRAME, 0, NULL, NULL);

			else
				iStatus = EndPage(hPr->hDC);

			DbgOutStr("EndPage = %d\r\n", iStatus, 0, 0, 0, 0);
			PrintTellError(hSession, (HPRINT)hPr, iStatus);
			}

		if (iStatus >= 0)
			{
			if (HA5G.fIsWin30)
				iStatus = Escape(hPr->hDC, ENDDOC, 0, (LPTSTR)0, NULL);

			else
				iStatus = EndDoc(hPr->hDC);

			DbgOutStr("EndDoc = %d\r\n", iStatus, 0, 0, 0, 0);
			PrintTellError(hSession, (HPRINT)hPr, iStatus);
			}

		if (IsWindow(hPr->hwndPrintAbortDlg))
			DestroyWindow(hPr->hwndPrintAbortDlg);

		FreeProcInstance((FARPROC)hPr->lpfnPrintAbortDlg);
		FreeProcInstance((FARPROC)hPr->lpfnPrintAbortProc);
		DeleteDC(hPr->hDC);
		}

	else
		{
		nb_close(hPr->hPrn);
		}

	FreeProcInstance(hPr->lpfnTimerCallback);
	hMem = (HANDLE)GlobalHandle(HIWORD(hPr->pach));
	GlobalUnlock(hMem);
	GlobalFree(hMem);
	free(hPr);
	hPr = NULL;
	mSetPrintHdl(hSession, (HPRINT)0);
	return;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printAbortProc
 *
 * DESCRIPTION:
 *	Enables print-manager to unspool stuff when system is low on disk
 *	space.	Is also called whenever EndPage() is called.
 *
 * ARGUMENTS:
 *	HDC hdcPrn	- DC of printer
 *	INT 		- nCode
 *
 * RETURNS:
 *	Stuff
 *
 */
BOOL CALLBACK printAbortProc(HDC hDC, INT nCode)
	{
	MSG msg;
	//cost HHPRINT hhPrint = printCtrlLookupDC(hDC);

	//*HCLOOP hCLoop = sessQueryCLoopHdl(hhPrint->hSession);

	DbgOutStr("\r\nprintAbortProc : %d\r\n", nCode, 0, 0, 0, 0);

	//*if (hCLoop == 0)
	//*    {
	//*    assert(FALSE);
	//*    return FALSE;
	//*    }

	// Need to quit processing characters to the emulator at this
	// point or a recursion condition occurs which results in a
	// run-away condtion.

	//*CLoopRcvControl(hCLoop, CLOOP_SUSPEND, CLOOP_RB_PRINTING);
	//*CLoopSndControl(hCLoop, CLOOP_SUSPEND, CLOOP_SB_PRINTING);

	while (PeekMessage((LPMSG)&msg, (HWND)0, 0, 0, PM_REMOVE))
		{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		}

	//*CLoopRcvControl(hCLoop, CLOOP_RESUME, CLOOP_RB_PRINTING);
	//*CLoopSndControl(hCLoop, CLOOP_RESUME, CLOOP_SB_PRINTING);

	DbgOutStr("Exiting printAbortProc", 0, 0, 0, 0, 0);

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printString
 *
 * DESCRIPTION:
 *	Workhorse print-echo function.	Takes care of counting lines and
 *	paginating.  Also calls printOpenDC() if necessary to get a printer
 *	DC.
 *
 * ARGUMENTS:
 *	HHPRINT 	hhPrint 	- The Internal printer handle
 *	LPCTSTR 	pachStr 	- A pointer to the string to print.
 *	int 		iLen		- The length of the string to print.
 *
 * RETURNS:
 *	TRUE = OK, FALSE = error
 *
 */
int printString(const HHPRINT hhPrint, LPCTSTR pachStr, int iLen)
	{
	int 	nCharCount;
	int 	nIdx;
    int iPrintableWidth;

	SIZE	stStringSize;
	LPCTSTR pszTemp;
	TCHAR   achBuf[512];
    RECT stRect;

    //
    // get a device context if we do not already have one
    //

	if (hhPrint->hDC == 0)
		{
		if (printOpenDC(hhPrint) == FALSE)
			{
			printEchoClose((HPRINT)hhPrint);
			return FALSE;
			}
		}
 
  
	for (nCharCount = nIdx = 0, pszTemp = pachStr ;
			nIdx < iLen ;
				++nCharCount, ++nIdx, pszTemp = StrCharNext(pszTemp))
		{
		if (IsDBCSLeadByte((BYTE)*pszTemp))
			nCharCount++;

		switch (*pszTemp)
			{
		case TEXT('\r'):
            if ( nCharCount )
 			    MemCopy(achBuf, pachStr, nCharCount);
			achBuf[nCharCount] = TEXT('\0');

            GetTextExtentPoint(hhPrint->hDC, achBuf, 
                               StrCharGetByteCount(achBuf), &stStringSize);

		    if ( nCharCount > 1 )
                {
                //
                // calculate a print rect for the current margins
                //
    
                iPrintableWidth = GetDeviceCaps( hhPrint->hDC, HORZRES );
                iPrintableWidth -= hhPrint->marginsDC.right;

                stRect.left   = hhPrint->cx;
                stRect.right  = iPrintableWidth;
                stRect.top    = hhPrint->cy;
                stRect.bottom = hhPrint->cy + stStringSize.cy;

                ExtTextOut( hhPrint->hDC, hhPrint->cx, hhPrint->cy, 
                            ETO_CLIPPED, &stRect, achBuf, 
                            StrCharGetByteCount(achBuf), NULL );
                }

			memset(achBuf, '\0', sizeof(achBuf));
			hhPrint->cx = hhPrint->marginsDC.left;
			pachStr = StrCharNext(pszTemp);
			nCharCount = 0;

			break;

		case TEXT('\f'):
			hhPrint->nLinesPrinted = hhPrint->nLinesPerPage;

			/* --- Fall thru to case '\n' --- */

		case TEXT('\n'):
            if (nCharCount)
                MemCopy(achBuf, pachStr,nCharCount);
		    achBuf[nCharCount] = TEXT('\0');

   			GetTextExtentPoint(hhPrint->hDC,
   			                   achBuf,
                               StrCharGetByteCount(achBuf),
                               &stStringSize);

            if ( nCharCount > 1 )
                {
                iPrintableWidth = GetDeviceCaps( hhPrint->hDC, HORZRES );
                iPrintableWidth -= hhPrint->marginsDC.right;

                stRect.left   = hhPrint->cx;
                stRect.right  = iPrintableWidth;
                stRect.top    = hhPrint->cy;
                stRect.bottom = hhPrint->cy + stStringSize.cy;

                ExtTextOut( hhPrint->hDC, hhPrint->cx, hhPrint->cy, 
                            ETO_CLIPPED, &stRect, achBuf, 
                            StrCharGetByteCount(achBuf), NULL );
                }

			hhPrint->cy += stStringSize.cy;
			pachStr = StrCharNext(pszTemp);
			nCharCount = 0;

            //
            // check if we need a new page
            //
            
			hhPrint->nLinesPrinted += 1;

			if (hhPrint->nLinesPrinted > hhPrint->nLinesPerPage)
				{
				if (hhPrint->nFlags & PRNECHO_BY_PAGE)
					{
					printEchoClose((HPRINT)hhPrint);
					hhPrint->nFlags |= PRNECHO_IS_ON;
					return TRUE;
					}

					hhPrint->nStatus = EndPage(hhPrint->hDC);

					if (hhPrint->nStatus < 0)
						{
						printEchoClose((HPRINT)hhPrint);
						return FALSE;
						}

					hhPrint->nStatus = StartPage(hhPrint->hDC);
                    printSetFont( hhPrint );

					if (hhPrint->nStatus <= 0)
						{
						printEchoClose((HPRINT)hhPrint);
						return FALSE;
						}

					hhPrint->nLinesPrinted = 0;
					hhPrint->cx = hhPrint->marginsDC.left;
                    hhPrint->cy = hhPrint->marginsDC.top;
				}
			break;

		default:
			break;
			}
		}

	/* -------------- Left over portion of a line? ------------- */

	if ((nCharCount > 0) && (*pachStr != '\0'))
		{
		DbgOutStr("o", 0, 0, 0, 0, 0);

		MemCopy(achBuf, pachStr,nCharCount);
		achBuf[nCharCount] = TEXT('\0');

		GetTextExtentPoint(hhPrint->hDC,
							achBuf,
							StrCharGetByteCount(achBuf),
							&stStringSize);

        iPrintableWidth = GetDeviceCaps( hhPrint->hDC, HORZRES );
        iPrintableWidth -= hhPrint->marginsDC.right;

        stRect.left   = hhPrint->cx;
        stRect.right  = iPrintableWidth;
        stRect.top    = hhPrint->cy;
        stRect.bottom = hhPrint->cy + stStringSize.cy;

        ExtTextOut( hhPrint->hDC, hhPrint->cx, hhPrint->cy, 
                    ETO_CLIPPED, &stRect, achBuf, 
                    StrCharGetByteCount(achBuf), NULL );

//		TextOut(hhPrint->hDC,
//					hhPrint->cx,
//					hhPrint->cy,
//					achBuf, StrCharGetByteCount(achBuf));

		memset(achBuf, '\0', sizeof(achBuf));
		hhPrint->cx += stStringSize.cx;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printQueryStatus
 *
 * DESCRIPTION: This function is used to determine if printing has been
 *				turned on for the supplied print handle.
 *
 * ARGUMENTS:	hPrint	- The external printer handle.
 *
 * RETURNS: 	TRUE	- If printing is on.
 *				FALSE	- If printing is off.
 *
 */
int printQueryStatus(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;

	if (hPrint == 0)
		assert(FALSE);

	return (bittest(hhPrint->nFlags, PRNECHO_IS_ON));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printStatusToggle
 *
 * DESCRIPTION:
 *	Toggles the status (on/off) of the supplied print handle.
 *
 * ARGUMENTS:	hPrint	- The external printer handle.
 *
 * RETURNS: 	nothing
 *
 */
void printStatusToggle(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;

	if (hPrint == 0)
		assert(FALSE);

	if (bittest(hhPrint->nFlags, PRNECHO_IS_ON))
        {
		bitclear(hhPrint->nFlags, PRNECHO_IS_ON);
        }
	else
		{
		bitset(hhPrint->nFlags, PRNECHO_IS_ON);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printSetStatus
 *
 * DESCRIPTION:
 *	Turns priniting on or off for the supplied handle.
 *
 * ARGUMENTS:	hPrint		- The external printer handle.
 *				fSetting	- True or False to turn printing on/off.
 *
 * RETURNS: 	nothing
 *
 *
 */
void printSetStatus(const HPRINT hPrint, const int fSetting)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;

	if (hPrint == 0)
		assert(FALSE);

	if (fSetting)
		bitset(hhPrint->nFlags, PRNECHO_IS_ON);
	else
		bitclear(hhPrint->nFlags, PRNECHO_IS_ON);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printQueryPrinterInfo
 *
 * DESCRIPTION:
 *	This function copies five pieces of information (pszPrinter, pDevNames,
 *	pDevMode, lf, and margins ) from the Session HHPRINT handle, to the supplied
 *	HHPRINT handle.  The objective is to copy the contents of the Session's 
 *  HPRINT handle to another HPRINT handle (from the emulators).  Remember that
 *  the Session's HPRINT handle is the one that contains the stored printer name and
 *	setup information.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void printQueryPrinterInfo( const HHPRINT hhSessPrint, HHPRINT hhPrint )
	{
	TCHAR *pTemp;
	DWORD dwSize;

	// Copy the printer name.
	//
	StrCharCopy(hhPrint->achPrinterName, hhSessPrint->achPrinterName);

	// Copy the DEVNAMES structure.
	//
	if (hhSessPrint->pstDevNames)
		{
		if (hhPrint->pstDevNames)
			{
			free(hhPrint->pstDevNames);
			hhPrint->pstDevNames = NULL;
			}

		pTemp = (TCHAR *)hhSessPrint->pstDevNames;
		pTemp += hhSessPrint->pstDevNames->wOutputOffset;
		pTemp += StrCharGetByteCount(pTemp) + 1;

		dwSize = (DWORD)(pTemp - (TCHAR*)hhSessPrint->pstDevNames);

		hhPrint->pstDevNames = malloc(dwSize);
		if (hhPrint->pstDevNames == 0)
			{
			assert(FALSE);
			return;
			}

        if (dwSize)
            MemCopy(hhPrint->pstDevNames, hhSessPrint->pstDevNames, dwSize);
		}

	// Copy the DEVMODE structure.
	//
	if (hhSessPrint->pstDevMode)
		{
		if (hhPrint->pstDevMode)
			{
			free(hhPrint->pstDevMode);
			hhPrint->pstDevMode = NULL;
			}

		dwSize = hhSessPrint->pstDevMode->dmSize +
					hhSessPrint->pstDevMode->dmDriverExtra;

		hhPrint->pstDevMode = malloc(dwSize);
		if (hhPrint->pstDevMode == 0)
			{
			assert(FALSE);
			return;
			}

        if (dwSize)
		    MemCopy(hhPrint->pstDevMode, hhSessPrint->pstDevMode, dwSize);
		}


	// Copy the font and margin information
	//
	
    MemCopy( &hhPrint->margins, &hhSessPrint->margins, sizeof(RECT) );
    MemCopy( &hhPrint->lf, &hhSessPrint->lf, sizeof(LOGFONT) );
    hhPrint->iFontPointSize = hhSessPrint->iFontPointSize;

    return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printVerifyPrinter
 *
 * DESCRIPTION:
 *	This routine is used to determine if a printer (any printer) is
 *	installed.
 *
 * ARGUMENTS:
 *	hPrint	-	An external print handle.
 *
 * RETURNS:
 * 0 if successful, otherwise -1.
 *
 */
int printVerifyPrinter(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR achBuf[256];
	TCHAR *pszString;
	HANDLE	hPrinter = NULL;
	BOOL	fRet;

	// Check to see if the printer that has been saved with the
	// session information is still available.	If it is, simply
	// return a zero, indicating everything is OK.
	//
	fRet = OpenPrinter((LPTSTR)hhPrint->achPrinterName, &hPrinter, NULL);

	if (fRet)
		{
		ClosePrinter(hPrinter);
		return(0);
		}

	// If we're here, it's time to locate the default printer, whatever
	// it is.  If the default printer is selected here, the print handle's
	// name is initialized to that value.
	//
	if (GetProfileString("Windows", "Device", ",,,", achBuf,
					sizeof(achBuf)) && (pszString = strtok(achBuf, ",")))
		{
		StrCharCopy(hhPrint->achPrinterName, pszString);
		return (0);
		}

	// A printer is NOT available.	Display the text for telling the
	// user how to install one.  It should be the same as the text that
	// appears in the printDlg call when this happens.
	//
	LoadString(glblQueryDllHinst(),
				IDS_PRINT_NO_PRINTER,
				achBuf,
				sizeof(achBuf) / sizeof(TCHAR));

	MessageBeep(MB_ICONEXCLAMATION);

	TimedMessageBox(sessQueryHwnd(hhPrint->hSession),
					achBuf,
					0,
					MB_ICONEXCLAMATION | MB_OK,
					0);
	return -1;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printSetFont
 *
 * DESCRIPTION:
 *	Sets the terminal font to the given font.  If hFont is zero,
 *	termSetFont() trys to create a default font.
 *
 * ARGUMENTS:
 *	hhTerm	- internal term handle.
 *	plf 	- pointer to logfont
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL printSetFont(const HHPRINT hhPrint)
	{
    LOGFONT lf;

    lf = hhPrint->lf;
    lf.lfHeight = hhPrint->iFontPointSize;

    printCreatePointFont( &lf, hhPrint );

    SelectObject( hhPrint->hDC, hhPrint->hFont ); 
	
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printCreatePointFont
 *
 * DESCRIPTION:
 *	Creates a hFont based on the log font structure given.  This function assumes
 *  the height member of the log font structure to be in 1/10 of a point 
 *  increments.  A 12 point font would be represented as 120.  The hFont is stored
 *  in the print handle provided.
 *
 * ARGUMENTS:
 *	pLogFont   - A pointer to a log font structure.
 *  hhPrint    - A print handle to store the HFONT into.
 *
 * RETURNS:
 *	void
 *
 */

void printCreatePointFont( LOGFONT * pLogFont, HHPRINT hhPrint )
    {
    POINT pt;
    POINT ptOrg = { 0, 0 };

    if (hhPrint->hFont)
        {
    	DeleteObject(hhPrint->hFont);
        }

    pt.y = GetDeviceCaps(hhPrint->hDC, LOGPIXELSY) * pLogFont->lfHeight;
    pt.y /= 720; 
    
    DPtoLP(hhPrint->hDC, &pt, 1);
    DPtoLP(hhPrint->hDC, &ptOrg, 1);
    
    pLogFont->lfHeight = -abs(pt.y - ptOrg.y);
    
    hhPrint->hFont = CreateFontIndirect( pLogFont );

    return;
    }

/*******************************************************************************
 * FUNCTION:
 *    printSetMargins
 *
 * DESCRIPTION:
 *    Sets the margins for the print handle, by converting from the values 
 *    returned by the page setup dialog.
 *
 * ARGUMENTS:
 *    aMargins - A RECT structure that contains the margins in inches.
 *
 * Return:
 *    void
 *
 *  Author: dmn:02/19/97
 *
 */

void printSetMargins( HHPRINT hhPrint )
    {
    int iPixelsPerInchX;  
    int iPixelsPerInchY;  
    int iPhysicalOffsetX; 
    int iPhysicalOffsetY; 

    if ( hhPrint->hDC )
        {
        hhPrint->marginsDC = hhPrint->margins;

        //
        // convert the margins to pixels
        //

        iPixelsPerInchX = GetDeviceCaps( hhPrint->hDC, LOGPIXELSX );
        iPixelsPerInchY = GetDeviceCaps( hhPrint->hDC, LOGPIXELSY );

        hhPrint->marginsDC.left   = ( hhPrint->marginsDC.left   * iPixelsPerInchX ) / 1000;
        hhPrint->marginsDC.right  = ( hhPrint->margins.right  * iPixelsPerInchX ) / 1000;
        hhPrint->marginsDC.top    = ( hhPrint->marginsDC.top    * iPixelsPerInchY ) / 1000;
        hhPrint->marginsDC.bottom = ( hhPrint->marginsDC.bottom * iPixelsPerInchY ) / 1000;

        iPhysicalOffsetX = GetDeviceCaps( hhPrint->hDC ,PHYSICALOFFSETX );
        iPhysicalOffsetY = GetDeviceCaps( hhPrint->hDC, PHYSICALOFFSETY );

        hhPrint->marginsDC.left   = max( 0, hhPrint->marginsDC.left   - iPhysicalOffsetX );
        hhPrint->marginsDC.right  = max( 0, hhPrint->marginsDC.right  - iPhysicalOffsetX );
        hhPrint->marginsDC.top    = max( 0, hhPrint->marginsDC.top    - iPhysicalOffsetY );
        hhPrint->marginsDC.bottom = max( 0, hhPrint->marginsDC.bottom - iPhysicalOffsetY );
        }

    return;
    }

