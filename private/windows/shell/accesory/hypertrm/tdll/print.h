/*	File: D:\WACKER\tdll\print.h (Created: 14-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */
#define _MAX_PATH	260

#define WM_PRINT_NEWPAGE		WM_USER+0x100

#define PRNECHO_BY_JOB			0x0001	 // Don't print until session closes.
#define PRNECHO_BY_PAGE 		0x0002	 // Print each page separately.
#define PRNECHO_CHARS			0x0010
#define PRNECHO_LINES			0x0020
#define PRNECHO_SCREENS 		0x0040
#define PRNECHO_IS_ON			0x0100	 /* capture is currently on */
#define PRNECHO_PAUSE			0x0200	 /* capture is paused */

// From printhdl.c
HPRINT	printCreateHdl(const HSESSION hSession);
void	printDestroyHdl(const HPRINT hPrint);
void	printSaveHdl(const HPRINT hPrint);
int 	printInitializeHdl(const HPRINT hPrint);

// From print.c
void	printTellError(const HSESSION hSession, const HPRINT hPrint, const int iStatus);
int 	printQueryStatus(const HPRINT hPrint);
void	printSetStatus(const HPRINT hPrint, const int fSetting);
void	printStatusToggle(const HPRINT hPrint);
int 	printVerifyPrinter(const HPRINT hPrint);


// From prnecho.c
int 	printEchoChar(const HPRINT hPrint, const ECHAR tChar);
void	printEchoLine(const HPRINT hPrint, ECHAR *achLine, int iLen);
int 	printEchoClose(HPRINT hPrint);
int 	printEchoStart(HPRINT hPrint);
int 	printEchoString(HPRINT hPrint, ECHAR *achStr, int iLen);
int 	printEchoScreen(HPRINT hPrint, ECHAR *achStr, int iLen);
int     printEchoRaw(HPRINT hPrint, ECHAR *pszPrintStr, int nLen);

// From printset.c
void	printsetSetup(const HPRINT hPrint, const HWND hwnd);
int 	printPageSetup(const HPRINT hPrint, const HWND hwnd);
void	printsetPrint(const HPRINT hPrint);
BOOL	CALLBACK printsetAbortProc(HDC hdcPrn, INT nCode);
LRESULT CALLBACK printsetDlgProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);
