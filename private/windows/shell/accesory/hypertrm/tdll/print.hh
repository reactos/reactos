/*	File: D:\WACKER\tdll\print.hh (Created: 19-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

#define PRINTSET_LOCAL		0x0001	// The printer is attached locally.
#define PRINTSET_SHARED 	0x0002	// The printer is shared (networked).

#define MAX_NUM_PRINT_DC	5		// Max number of slots in print 
									// control table.

typedef struct stPrintPrivate *HHPRINT;

struct stPrintPrivate
	{
	HSESSION	hSession;

	CRITICAL_SECTION	csPrint;		// For snychronizing access.

	PDEVMODE	pstDevMode; 			// Information from setup dialogs.
										// The printer name is contained
										// within the DEVMODE information.

	LPDEVNAMES	pstDevNames;			// Information from setup dialogs.
										// See printsetSetup for details
										// on usage of DEVNAMES.

	TCHAR		*pszPrinterPortName,	// The name of the printer port.
				*pszPrinterDriver,
				achDoc[80],
				achPrintToFileName[256],
				achPrinterName[80];

	ECHAR 		achPrnEchoLine[256];	// For session file use only.

	DWORD		nSelectionFlags,
				fLocation;

	HDC 		hDC;
	DOCINFO 	di;
	HFONT		hFont;
	LOGFONT 	lf;
	TEXTMETRIC	tm;
    RECT        margins;                // Margins in inches for the page setup dialog
    RECT        marginsDC;              // Margins in pixels for the current printer
    int         iFontPointSize;

	long		tmHeight;

	int 		nLinesPrinted,		// running count of lines printed (per page)
				nPage,				// current page number being printed
				nLinesPerPage,		// calculated in PrintMemoryBlock
				nLnIdx,
				nFlags,
				nStatus,
				cx, cy, 			// position to print from.
				fUserAbort,
				fError,
				nPrnMethod, // PRNECHO_BY_PAGE || PRNECHO_BY_JOB
				nPrnMode;	// PRNECHO_CHARS || PRNECHO_LINES || PRNECHO_SCREENS

	DLGPROC 	lpfnPrintDlgProc;
	ABORTPROC	lpfnPrintAbortProc;
	HWND		hwndPrintDlg;

	};

// From print.c

int 	printString(const HHPRINT hhPrint, LPCTSTR pachStr, int iLen);
BOOL	CALLBACK printAbortProc(HDC hDC, int nCode);
BOOL 	printSetFont(const HHPRINT hhPrint);
void	printQueryPrinterInfo(const HHPRINT hhSessPrint, HHPRINT hhPrint );
void 	printCreatePointFont(LOGFONT * pLogFont, HHPRINT hhPrint);
void    printSetMargins( HHPRINT hhPrint );

// From printdc.c

HDC 	printCtrlCreateDC(const HPRINT hPrint);
void	printCtrlDeleteDC(const HPRINT hPrint);
HPRINT	printCtrlLookupDC(const HDC hDC);
int 	printOpenDC(const HHPRINT hhPrint);

