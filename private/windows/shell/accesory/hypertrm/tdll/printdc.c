/*	File: D:\WACKER\tdll\printdc.c (Created: 26-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */

#include <windows.h>
#pragma hdrstop

#include <term\res.h>
#include "stdtyp.h"
#include "assert.h"
#include "print.h"
#include "print.hh"
#include "session.h"
#include "tdll.h"
#include "tchar.h"
#include "globals.h"

#define MAX_NUM_PRINT_DC	5

struct stPrintTable
	{
	HPRINT	hPrintHdl;
	HDC 	hDCPrint;
	};

static struct stPrintTable stPrintCtrlTbl[MAX_NUM_PRINT_DC];

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printCtrlCreateDC
 *
 * DESCRIPTION:
 *	This function is used to create the Printer DC for the supplied Print
 *	handle.  It is done in this function so a table that contains both the
 *	DC and the Print Handle can be maintained.	This is necessary so the
 *	PrintAbortProc function (which receives an HDC only) can determine
 *	which Print Handle is associated with the HDC.
 *
 *	The DC created by this function uses the Printer Name in the supplied
 *	Printer Handle.  If this name is not supplied, the function returns 0;
 *
 * ARGUMENTS:
 *	HPRINT	-	The External Print handle.
 *
 * RETURNS:
 *	HDC 	-	A device context if successful, otherwise 0.
 *
 */
HDC printCtrlCreateDC(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR	szPrinter[256];
	TCHAR  *szDriver, *szOutput;
	int 	nIdx,
			iSize;
	HDC 	hDC;

	if (hPrint == 0)
		{
		assert(FALSE);
		return 0;
		}

	if (hhPrint->achPrinterName[0] == 0)
		{
		assert(FALSE);
		return 0;
		}

	for (nIdx = 0; nIdx < MAX_NUM_PRINT_DC; nIdx++)
		{
		if (stPrintCtrlTbl[nIdx].hPrintHdl == 0)
			{
			GetProfileString("Devices", hhPrint->achPrinterName, "",
				szPrinter, sizeof(szPrinter));

			hDC = 0;

			if ((szDriver = strtok(szPrinter, ",")) &&
				(szOutput = strtok(NULL,	  ",")))
				{
				hDC = CreateDC(szDriver, hhPrint->achPrinterName, szOutput,
						hhPrint->pstDevMode);
				}

			if (hDC == 0)
				{
				assert(FALSE);
				return 0;
				}

			if (hhPrint->pszPrinterPortName != 0)
				{
				free(hhPrint->pszPrinterPortName);
				hhPrint->pszPrinterPortName = NULL;
				}

			iSize = StrCharGetByteCount(szOutput) + 1;

			hhPrint->pszPrinterPortName = malloc((unsigned int)iSize);

			StrCharCopy(hhPrint->pszPrinterPortName, szOutput);

			stPrintCtrlTbl[nIdx].hDCPrint = hDC;
			stPrintCtrlTbl[nIdx].hPrintHdl = hPrint;
			return (hDC);
			}
		}

	assert(FALSE);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printCtrlDeleteDC
 *
 * DESCRIPTION:
 *	This function will destroy the print HDC accociated with the
 *	Printer Handle passed as the argument.	See printCtrlCreateDC for
 *	more information.
 *
 * ARGUMENTS:
 *	HPRINT	-	The External Printer Handle.
 *
 * RETURNS:
 *	void
 *
 */
void printCtrlDeleteDC(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
	int   nIdx;

	if (hPrint == 0)
		assert(FALSE);

	if (hhPrint->hDC == 0)
		assert(FALSE);

	for (nIdx = 0; nIdx < MAX_NUM_PRINT_DC; nIdx++)
		{
		if (stPrintCtrlTbl[nIdx].hPrintHdl == hPrint)
			{
			if (DeleteDC(hhPrint->hDC) == TRUE)
				{
				stPrintCtrlTbl[nIdx].hPrintHdl = 0;
				stPrintCtrlTbl[nIdx].hDCPrint = 0;
				hhPrint->hDC = 0;
				return;
				}
			else
				{
				assert(FALSE);
				}
			}
		}

	assert(FALSE);
	return;

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printCtrlLookupDC
 *
 * DESCRIPTION:
 *	This function returns the External Print Handle that includes the
 *	supplied HDC.  This function was designed to be called by the
 *	PrintAbortProc routine.  See printCtrlCreateDC for more info.
 *
 * ARGUMENTS:
 *	HDC 	hDC -	A (printer) device context.
 *
 * RETURNS:
 *	HPRINT		-	An External print handle.
 *
 */
HPRINT printCtrlLookupDC(const HDC hDC)
	{
	int nIdx;

	for (nIdx = 0; nIdx < MAX_NUM_PRINT_DC; nIdx++)
		{
		if (stPrintCtrlTbl[nIdx].hDCPrint == hDC)
			return stPrintCtrlTbl[nIdx].hPrintHdl;
		}

	assert(FALSE);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printOpenDC
 *
 * DESCRIPTION:
 *	Does the nasty work of opening a printer DC and initializing it.
 *
 * ARGUMENTS:
 *	HHPRINT hhPrint - Internal print handle.
 *
 * RETURNS:
 *	TRUE on success.
 *
 */
int printOpenDC(const HHPRINT hhPrint)
	{
    HHPRINT hhSessPrint;
    int iLineHeight;
    int iVertRes;
    TCHAR achDocTitle[80];
     	
    if (hhPrint == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhPrint->hDC)
		return TRUE;

	// Get the printer information from the session print handle.  This
	// includes the printer name and other attributes that may have been
	// setup by the common print dialogs.
	//

    hhSessPrint = (HHPRINT) sessQueryPrintHdl(hhPrint->hSession);
    printQueryPrinterInfo( hhSessPrint, hhPrint );

	// Create the DC.
	//
	hhPrint->hDC = printCtrlCreateDC((HPRINT)hhPrint);
	if (hhPrint->hDC == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	printSetFont(hhPrint);
	printSetMargins(hhPrint);

	hhPrint->cx = hhPrint->marginsDC.left;							 
	hhPrint->cy = hhPrint->marginsDC.top;


	/* -------------- Figure out how many lines per page ------------- */

	GetTextMetrics(hhPrint->hDC, &hhPrint->tm);
	hhPrint->tmHeight = hhPrint->tm.tmHeight;

    iLineHeight = hhPrint->tm.tmHeight + hhPrint->tm.tmExternalLeading;
    iVertRes = GetDeviceCaps(hhPrint->hDC, VERTRES);
    iVertRes -= (hhPrint->marginsDC.top + hhPrint->marginsDC.bottom);

	if (iLineHeight == 0) //need to prevent a divide by zero error
		iLineHeight = 1;

   	hhPrint->nLinesPerPage = max( iVertRes / iLineHeight, 1);
	hhPrint->nLinesPrinted = 0;

    if (LoadString(glblQueryDllHinst(), IDS_PRINT_CAPTURE_DOC, 
	    achDocTitle, sizeof(achDocTitle)/sizeof(TCHAR)))
        {
    	lstrcpy(hhPrint->achDoc, achDocTitle);
        }

	/* -------------- Setup the Print Abort Proc ------------- */

	hhPrint->nStatus = SetAbortProc(hhPrint->hDC, (ABORTPROC)printAbortProc);

	/* -------------- Open printer ------------- */

	hhPrint->di.cbSize = sizeof(DOCINFO);
	hhPrint->di.lpszDocName = hhPrint->achDoc;
	hhPrint->di.lpszOutput = (LPTSTR)NULL;

	// StartDoc.
	//
	hhPrint->nStatus = StartDoc(hhPrint->hDC, &hhPrint->di);
	DbgOutStr("\r\nStartDoc: %d", hhPrint->nStatus, 0, 0, 0, 0);

	// StartPage.
	//
	if (hhPrint->nStatus > 0)
		{
		hhPrint->nStatus = StartPage(hhPrint->hDC);
        printSetFont( hhPrint );
		DbgOutStr("\r\nStartPage: %d", hhPrint->nStatus, 0, 0, 0, 0);
		}
	else
		{
		return FALSE;
		}

	if (hhPrint->nStatus <= 0)
		return FALSE;

	return TRUE;
	}

