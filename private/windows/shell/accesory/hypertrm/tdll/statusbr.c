/*	File: D:\WACKER\tdll\statusbr.c (Created: 02-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <time.h>

#include <term\res.h>

#include "assert.h"
#include "stdtyp.h"
#include "session.h"
#include "globals.h"
#include "statusbr.h"
#include "cnct.h"
#include "capture.h"
#include "print.h"
#include "tdll.h"
#include "tchar.h"
#include "mc.h"
#include "load_res.h"
#include "timers.h"
#include "com.h"

#include <emu\emuid.h>
#include <emu\emu.h>
#include <emu\emudlgs.h>
#include <xfer\itime.h>

// Static function prototypes...
//
STATIC_FUNC void sbrSubclassStatusbarWindow(HWND hwnd, HSESSION hSession);
STATIC_FUNC void sbrCnctStatus		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrSetToNoParts	(const HWND hwnd, LPTSTR pszStr);
STATIC_FUNC void sbrRefresh 		(const HWND hwnd, const int iPart, const pSBR pSBRData);
STATIC_FUNC BOOL sbrNeedToSetParts	(const HWND hwnd);
STATIC_FUNC void sbrSetParts		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrTimerRefresh	(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrDrawCnctPart	(const HWND hwnd, const int iCnctStatus,
									 LPTSTR pszStr);
STATIC_FUNC void sbrSetPartsOnce	(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC int  sbrGetSizeInPixels (const HWND hwnd, LPTSTR pszStr);
STATIC_FUNC int  sbrCalcPartSize	(const HWND hwnd, const int iId);
STATIC_FUNC void sbrCachString		(pSBR pSBRData,
									 unsigned int iPart,
									 LPTSTR pach);
STATIC_FUNC BOOL sbrCnctTimeToSystemTime(const HWND hwnd,
									 LPSYSTEMTIME lpSysTime,
									 const pSBR pSBRData);

STATIC_FUNC void sbrEmulatorName	(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrScrolLock		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrCapsLock		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrNumLock 		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrCapture 		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrPrintEcho		(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC BOOL sbrCreateTimer 	(const HWND hwnd, const pSBR pSBRData);
STATIC_FUNC void sbrCom 			(const HWND hwnd, const pSBR pSBRData);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrCreateSessionStatusbar
 *
 * DESCRIPTION:
 *	Not much now but will get more complicated later.
 *
 * ARGUMENTS:
 *	hwndSession - session window handle
 *
 * RETURNS:
 *	Handle to status window or zero on error.
 *
 */
HWND sbrCreateSessionStatusbar(HSESSION hSession)
	{
	HWND 		hwnd = (HWND)0;
	//int		  aBorders[3];
	HDC  		hDC;
	TEXTMETRIC 	tm;
	HWND		hwndSession = (HWND)0;

	hwndSession = sessQueryHwnd(hSession);
	if (!IsWindow(hwndSession))
		return (HWND)0;

	hwnd = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP,
							  0,
							  hwndSession,
							  IDC_STATUS_WIN);

	if (IsWindow(hwnd))
		{
		#if 0
		aBorders[0] = GetSystemMetrics(SM_CXEDGE);
		aBorders[1] = GetSystemMetrics(SM_CYEDGE);
		aBorders[2] = GetSystemMetrics(SM_CXEDGE);

		SendMessage(hwnd, SB_SETBORDERS, 0, (LPARAM)aBorders);
		#endif

		hDC = GetDC(hwnd);
		GetTextMetrics(hDC, &tm);
		ReleaseDC(hwnd, hDC);

		SendMessage(hwnd, SB_SETMINHEIGHT, (WPARAM)tm.tmHeight, 0);
		ShowWindow(hwnd, SW_SHOWNA);

		sbrSubclassStatusbarWindow(hwnd, hSession);
		}
	return hwnd;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrSubclassStatusbarWindow
 *
 * DESCRIPTION:
 *	Subclass the Status Bar and init the data structure.
 *
 * ARGUMENTS:
 * 	hwnd 		- window handle.
 *	hSession 	- the session handle.
 *
 * RETURNS:
 *
 */
STATIC_FUNC void sbrSubclassStatusbarWindow(HWND hwnd, HSESSION hSession)
	{
	ATOM				atom = (ATOM)0;
	pSBR				pSBRData;

	atom = AddAtom((LPCTSTR)SBR_ATOM_NAME);

	if (atom == 0)
		{
		assert(FALSE);
		return;
		}

	pSBRData = (pSBR)LocalAlloc(LPTR, sizeof(SBR));

	if (pSBRData == 0)
		{
		assert(FALSE);
		return;
		}

	// Initialize statusbar data structure...
	//
	pSBRData->hSession = hSession;
	pSBRData->hwnd     = hwnd;
	pSBRData->hTimer   = (HTIMER)0;

	pSBRData->pachCNCT = (LPTSTR)0;
	pSBRData->pachCAPL = (LPTSTR)0;
	pSBRData->pachNUML = (LPTSTR)0;
	pSBRData->pachSCRL = (LPTSTR)0;
	pSBRData->pachCAPT = (LPTSTR)0;
	pSBRData->pachPECHO = (LPTSTR)0;

	// Do the subclass...
	//
	pSBRData->wpOrigStatusbarWndProc =
		(WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)sbrWndProc);

	SetProp(hwnd, (LPCTSTR)atom, (HANDLE)pSBRData);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrWndProc
 *
 * DESCRIPTION:
 *	Our own statusbar window proc.
 *
 * ARGUMENTS:
 *  Standard window proc parameters.
 *
 * RETURNS:
 *  Standard return value.
 *
 */
LRESULT APIENTRY sbrWndProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	ATOM	atom = (ATOM)0;
	pSBR	pSBRData;
	int		nRet;

	atom = FindAtom((LPCTSTR)SBR_ATOM_NAME);
	pSBRData = (pSBR)GetProp(hwnd, (LPCTSTR)atom);

	switch (uMsg)
		{
		case SBR_NTFY_INITIALIZE:
			sbrCreateTimer(hwnd, pSBRData);
			sbrSetPartsOnce(hwnd, pSBRData);
			sbrRefresh(hwnd, SBR_MAX_PARTS, pSBRData);
			break;

		case SBR_NTFY_REFRESH:
			if (IsWindowVisible(hwnd))
				sbrRefresh(hwnd, LOWORD(wPar), pSBRData);

			/* Fall through */

		case SBR_NTFY_TIMER:
			if (IsWindowVisible(hwnd) && !sbrNeedToSetParts(hwnd))
				sbrTimerRefresh(hwnd, pSBRData);
			return 0;

		case SBR_NTFY_NOPARTS:
			if (IsWindowVisible(hwnd))
				sbrSetToNoParts(hwnd, (LPTSTR)lPar);
			return 0;

		case WM_DESTROY:
			if (atom)
				{
				RemoveProp(hwnd, (LPCTSTR)atom);
				DeleteAtom(atom);
				}

			// Remove subclass from the statusbar window
			//
			SetWindowLongPtr(hwnd,	GWLP_WNDPROC, (LONG_PTR)pSBRData->wpOrigStatusbarWndProc);

			// Destroy the timer...
			//
			if (pSBRData->hTimer)
				{
				nRet = TimerDestroy(&pSBRData->hTimer);
				assert(nRet == TIMER_OK);
				}

			if (pSBRData->pachCNCT)
				{
				free(pSBRData->pachCNCT);
				pSBRData->pachCNCT = NULL;
				}
			if (pSBRData->pachCAPL)
				{
				free(pSBRData->pachCAPL);
				pSBRData->pachCAPL = NULL;
				}
			if (pSBRData->pachNUML)
				{
				free(pSBRData->pachNUML);
				pSBRData->pachNUML = NULL;
				}
			if (pSBRData->pachSCRL)
				{
				free(pSBRData->pachSCRL);
				pSBRData->pachSCRL = NULL;
				}
			if (pSBRData->pachCAPT)
				{
				free(pSBRData->pachCAPT);
				pSBRData->pachCAPT = NULL;
				}
			if (pSBRData->pachPECHO)
				{
				free(pSBRData->pachPECHO);
				pSBRData->pachPECHO = NULL;
				}

			LocalFree(pSBRData);
			pSBRData = NULL;
			return 0;

		default:
			break;
		}

	return CallWindowProc(pSBRData->wpOrigStatusbarWndProc, hwnd, uMsg,
								wPar, lPar);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrSetToNoParts
 *
 * DESCRIPTION:
 * 	Show status bar with no parts. This is usefull in showing help for
 *	menu items on the statusbar.
 *
 * ARGUMENTS:
 *  hwnd 	- window handle.
 *	pszStr 	- text to display on the status bar.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrSetToNoParts(const HWND hwnd, LPTSTR pszStr)
	{
	int  aWidths[1];

	if (hwnd)
		{
		aWidths[0] = -1;
		SendMessage(hwnd, SB_SETPARTS, (WPARAM)1, (LPARAM)aWidths);
		SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)pszStr);
		ShowWindow(hwnd, SW_SHOWNA);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrRefresh
 *
 * DESCRIPTION:
 *	Figure out what has changed and display status info appropriately.
 *
 * ARGUMENTS:
 *  hwnd  - window handle.
 *  iPart - a part number for a part we want to explicitly refresh.
 *			It can be SBR_MAX_PARTS, in which case all parts are refreshed and
 *			it can be SBR_KEY_PARTS, in which case all KEY parts are refreshed.
 *			In some cases it makes sense to just refresh only one part instead
 *			of refreshing all of them.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrRefresh(const HWND hwnd, const int iPart, const pSBR pSBRData)
	{
	if (pSBRData == 0)
		return;

	// Make sure we are displaying the correct # of parts
	//
	if (sbrNeedToSetParts(hwnd))
		sbrSetParts(hwnd, pSBRData);

	switch (iPart)
		{
		case SBR_KEY_PARTS:
			sbrScrolLock(hwnd, pSBRData);
			sbrCapsLock(hwnd, pSBRData);
			sbrNumLock(hwnd, pSBRData);
			break;

		case SBR_CNCT_PART_NO:
			sbrCnctStatus(hwnd, pSBRData);
			break;

		case SBR_EMU_PART_NO:
			sbrEmulatorName(hwnd, pSBRData);
			break;

		case SBR_COM_PART_NO:
			sbrCom(hwnd, pSBRData);
			break;

		case SBR_SCRL_PART_NO:
			sbrScrolLock(hwnd, pSBRData);
			break;

		case SBR_CAPL_PART_NO:
			sbrCapsLock(hwnd, pSBRData);
			break;

		case SBR_NUML_PART_NO:
			sbrNumLock(hwnd, pSBRData);
			break;

		case SBR_CAPT_PART_NO:
			sbrCapture(hwnd, pSBRData);
			break;

		case SBR_PRNE_PART_NO:
			sbrPrintEcho(hwnd, pSBRData);
			break;

		default:
			sbrCnctStatus(hwnd, pSBRData);
			sbrEmulatorName(hwnd, pSBRData);
			sbrCom(hwnd, pSBRData);
			sbrScrolLock(hwnd, pSBRData);
			sbrCapsLock(hwnd, pSBRData);
			sbrNumLock(hwnd, pSBRData);
			sbrCapture(hwnd, pSBRData);
			sbrPrintEcho(hwnd, pSBRData);
			break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrNeedToSetParts
 *
 * DESCRIPTION:
 *  Check the number of parts shown in the status bar.  If it is less then
 *	expected, i.e., the maximum parts we usually show, then we need to reset
 *	the status bar parts.  This happens, for example, when help text is
 * 	displayed for menu items, i.e., parts are set to 0.
 *
 * ARGUMENTS:
 * 	hwnd - window handle.
 *
 * RETURNS:
 *  TRUE if parts need to be set/re-set.
 *	FALSE otherwise.
 *
 */
STATIC_FUNC BOOL sbrNeedToSetParts(const HWND hwnd)
	{
	int nParts = 0;
	int aWidths[SBR_MAX_PARTS+1];

	memset(aWidths, 0, (SBR_MAX_PARTS+1) * sizeof(int));

	nParts = (int)SendMessage(hwnd, SB_GETPARTS, (WPARAM)SBR_MAX_PARTS+1,
		(LPARAM)aWidths);

	if (nParts != SBR_MAX_PARTS)
		return TRUE;

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrSetParts
 *
 * DESCRIPTION:
 *	Set parts in the statusbar.
 *
 * ARGUMENTS:
 * 	hwnd - window handle.
 *
 * RETURNS:
 *	void.
 *
 */
STATIC_FUNC void sbrSetParts(const HWND hwnd, const pSBR pSBRData)
	{
	SendMessage(hwnd, SB_SETPARTS, (WPARAM)SBR_MAX_PARTS,
		(LPARAM)pSBRData->aWidths);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrSetPartsOnce
 *
 * DESCRIPTION:
 *	Set parts in the statusbar according to the length of strings to be
 *	displayed in the appropriate parts.  This function should be called only
 *	once to figure out the part lengths, from then on the part rigth edges are
 *  stored in the statusbar data structure.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 * 	void
 */
STATIC_FUNC void sbrSetPartsOnce(const HWND hwnd, const pSBR pSBRData)
	{
	int 		 aWidths[SBR_MAX_PARTS] = {1, 1, 1, 1, 1, 1, 1, 1};
	int			 iNewWidth = 0, i;
	TCHAR		 ach[256];
	unsigned int iPart = 0;
	int aiBorders[3];

	// To make sure that we always have enough space to display the text
	// in the appropriate statusbar part read the text corresponding to each
	// part and check its length, adjust the size of the part if needed.
	//
	for (i = 0; i < SBR_MAX_PARTS; i++)
		{
		switch (i)
			{
			default:
			case SBR_CNCT_PART_NO:
			case SBR_EMU_PART_NO:
				iNewWidth = sbrCalcPartSize(hwnd, i);
				break;

			case SBR_COM_PART_NO:	iPart = IDS_STATUSBR_COM;	break;
			case SBR_SCRL_PART_NO:	iPart = IDS_STATUSBR_SCRL;	break;
			case SBR_CAPL_PART_NO:	iPart = IDS_STATUSBR_CAPL;	break;
			case SBR_NUML_PART_NO:	iPart = IDS_STATUSBR_NUML;	break;
			case SBR_CAPT_PART_NO:	iPart = IDS_STATUSBR_CAPTUREON; break;
			case SBR_PRNE_PART_NO:	iPart = IDS_STATUSBR_PRINTECHOON; break;
			}

		if (i != SBR_CNCT_PART_NO && i != SBR_EMU_PART_NO)
			{
			// Get the width for the current part string...
			//
			iNewWidth = 0;

			LoadString(glblQueryDllHinst(), iPart, ach,
				sizeof(ach) / sizeof(TCHAR));

			sbrCachString(pSBRData, iPart, ach);
			iNewWidth = (int)sbrGetSizeInPixels(hwnd, ach);
			}

		aWidths[i] = iNewWidth;
		}

	// When computing widths, we need to take the borders into account.
	//
	memset(aiBorders, 0, sizeof(aiBorders));
	SendMessage(hwnd, SB_GETBORDERS, 0, (LPARAM)aiBorders);

	// Calculate right edges of the statusbar parts...
	// put them back into aWidths.
	//
	aWidths[0] += aiBorders[1];

	for (i = 1; i < SBR_MAX_PARTS; i++)
		aWidths[i] += aWidths[i-1] + aiBorders[2];

	MemCopy(pSBRData->aWidths, aWidths, SBR_MAX_PARTS * sizeof(int));

	SendMessage(hwnd, SB_SETPARTS, (WPARAM)SBR_MAX_PARTS,
		(LPARAM)pSBRData->aWidths);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrCachString
 *
 * DESCRIPTION:
 *	Save the string we've just loaded from the resource file in our internal
 *  statusbar structure for future use.  This way we minimize the
 *	LoadStirng() calls.
 *
 * ARGUMENTS:
 *  pSBRData - handle to internal structure.
 *	iPart	 - part identifier.
 *	pach	 - label for that part.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrCachString(pSBR pSBRData, unsigned int iPart, LPTSTR pach)
	{
	switch (iPart)
		{
	case IDS_STATUSBR_CONNECT_FORMAT:
		if (pSBRData->pachCNCT)
			{
			free(pSBRData->pachCNCT);
			pSBRData->pachCNCT = NULL;
			}
		pSBRData->pachCNCT = malloc(
			(unsigned int)(StrCharGetByteCount(pach) + 1) *
			sizeof(TCHAR));
		StrCharCopy(pSBRData->pachCNCT, pach);
		break;

	case IDS_STATUSBR_SCRL:
		if (pSBRData->pachSCRL)
			{
			free(pSBRData->pachSCRL);
			pSBRData->pachSCRL = NULL;
			}
		pSBRData->pachSCRL = malloc(
			(unsigned int)(StrCharGetByteCount(pach) + 1) *
			sizeof(TCHAR));
		StrCharCopy(pSBRData->pachSCRL, pach);
		break;

	case IDS_STATUSBR_CAPL:
		if (pSBRData->pachCAPL)
			{
			free(pSBRData->pachCAPL);
			pSBRData->pachCAPL = NULL;
			}
		pSBRData->pachCAPL = malloc(
			(unsigned int)(StrCharGetByteCount(pach) + 1) *
			sizeof(TCHAR));
		StrCharCopy(pSBRData->pachCAPL, pach);
		break;

	case IDS_STATUSBR_NUML:
		if (pSBRData->pachNUML)
			{
			free(pSBRData->pachNUML);
			pSBRData->pachNUML = NULL;
			}
		pSBRData->pachNUML = malloc(
			(unsigned int)(StrCharGetByteCount(pach) + 1) *
			sizeof(TCHAR));
		StrCharCopy(pSBRData->pachNUML, pach);
		break;

	case IDS_STATUSBR_CAPTUREON:
		if (pSBRData->pachCAPT)
			{
			free(pSBRData->pachCAPT);
			pSBRData->pachCAPT = NULL;
			}
		pSBRData->pachCAPT = malloc(
			(unsigned int)(StrCharGetByteCount(pach) + 1) *
			sizeof(TCHAR));
		StrCharCopy(pSBRData->pachCAPT, pach);
		break;

	case IDS_STATUSBR_PRINTECHOON:
		if (pSBRData->pachPECHO)
			{
			free(pSBRData->pachPECHO);
			pSBRData->pachPECHO = NULL;
			}
		pSBRData->pachPECHO = malloc(
			(unsigned int)(StrCharGetByteCount(pach) + 1) *
			sizeof(TCHAR));
		StrCharCopy(pSBRData->pachPECHO, pach);
		break;

	default:
		break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrGetSizeInPixels
 *
 * DESCRIPTION:
 *  Caluclate the length of the string in pixels.  Adjust the
 *	length by some extra space to appear on the right of the string in the
 *	statusbar part.
 *
 * ARGUMENTS:
 *  hwnd 	- window handle.
 *	pszStr 	- pointer to a string.
 *
 * RETURNS:
 *  sz.cx 	- the size of the strings in pixels.
 *
 */
STATIC_FUNC int sbrGetSizeInPixels(const HWND hwnd, LPTSTR pszStr)
	{
	HDC	  hDC;
	SIZE  sz;

	// Select the font of the statusbar...
	//
	hDC = GetDC(hwnd);

	GetTextExtentPoint32(hDC,
						(LPCTSTR)pszStr,
						StrCharGetStrLength(pszStr),
						&sz);
	sz.cx += (EXTRASPACE * GetSystemMetrics(SM_CXBORDER));

	ReleaseDC(hwnd, hDC);
	return (sz.cx);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrCalcPartSize
 *
 * DESCRIPTION:
 *	Calculate the largest string that may be displayed in a given
 *	statusbar part.  This function is called to calculate the length of the
 * 	emulator part and the connection status part.  Depending on the translation
 *	of the strings in the resources these parts will have to be of different
 *	length.
 *
 * ARGUMENTS:
 *	hwnd 	- window handle
 *	iId	 	- part identifier
 *
 * RETURNS:
 *	iLongest - the maximum size, in pixels, for the given part
 */
STATIC_FUNC int sbrCalcPartSize(const HWND hwnd, const int iId)
	{
	unsigned int aCnctTable[]= {IDS_STATUSBR_CONNECT,
								IDS_STATUSBR_CONNECT_FORMAT,
								IDS_STATUSBR_CONNECT_FORMAT_X,
								IDS_STATUSBR_DISCONNECT,
								IDS_STATUSBR_CONNECTING,
								IDS_STATUSBR_DISCONNECTING};
	TCHAR	ach[100],
			achText[100];
	int 	i, iRet = 0, iLongest = 0, nLen, nLimit;
	//BYTE	  *pv;

	if (iId == SBR_CNCT_PART_NO)
		{
		TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
		nLimit = sizeof(aCnctTable) / sizeof(int);

		for (i = 0; i < nLimit; i++)
			{
			LoadString(glblQueryDllHinst(), aCnctTable[i], ach, sizeof(ach) / sizeof(TCHAR));
			if ((iRet = sbrGetSizeInPixels(hwnd, ach)) > iLongest)
				iLongest = iRet;
			}
		}
	else
		{

		#if 0
		if (resLoadDataBlock(glblQueryDllHinst(), IDT_EMU_NAMES,
				(LPVOID *)&pv, &nLen))
			{
			return 0;
			}

		nEmuCount = *(RCDATA_TYPE *)pv;
		pv += sizeof(RCDATA_TYPE);

		for (i = 0 ; i < nEmuCount ; i++)
			{
			if ((nLen = StrCharGetByteCount((LPTSTR)pv)+(int)sizeof(BYTE)) == 0)
				return 0;

			if ((iRet = sbrGetSizeInPixels(hwnd, pv)) > iLongest)
				iLongest = iRet;

			pv += (nLen + (int)sizeof(RCDATA_TYPE));
			}
		#endif

		for (i = IDS_EMUNAME_BASE ; i < IDS_EMUNAME_BASE + NBR_EMULATORS; i++)
			{
			nLen = LoadString(glblQueryDllHinst(), (unsigned int)i, achText, sizeof(achText) / sizeof(TCHAR));

			if (nLen == 0)
				return (0);

			if ((iRet = sbrGetSizeInPixels(hwnd, achText)) > iLongest)
				iLongest = iRet;
			}

		}
	return (iLongest);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrTimerRefresh
 *
 * DESCRIPTION:
 *	Refresh the timer display if we are connected.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *	void.
 *
 */
STATIC_FUNC void sbrTimerRefresh(const HWND hwnd, const pSBR pSBRData)
	{
	TCHAR		ach[256], achFormat[256];
	TCHAR		achTime[256];
	int			iRet = -1;
	HCNCT		hCnct = (HCNCT)0;
	SYSTEMTIME	stSystem;

	hCnct = sessQueryCnctHdl(pSBRData->hSession);

	if (hCnct)
		iRet = cnctQueryStatus(hCnct);

	if (iRet == CNCT_STATUS_TRUE)
		{
		pSBRData->iLastCnctStatus = iRet;
		sbrCnctTimeToSystemTime(hwnd, &stSystem, pSBRData);
		achTime[0] = TEXT('\0');

		if (GetTimeFormat(LOCALE_SYSTEM_DEFAULT,	
				TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT,
				&stSystem, NULL, achTime, sizeof(achTime)) == 0)
			{
			DbgShowLastError();
			sbrDrawCnctPart(hwnd, iRet, 0);
			return;
			}

		// Load the "Connected %s" format string...
		// Since this operation is costly, cach the connected format
		// string...
		//
		if (pSBRData->pachCNCT)
			{
			wsprintf(ach, pSBRData->pachCNCT, achTime);
			}

		else
			{
			LoadString(glblQueryDllHinst(), IDS_STATUSBR_CONNECT_FORMAT,
				achFormat, sizeof(achFormat) / sizeof(TCHAR));

			sbrCachString(pSBRData, IDS_STATUSBR_CONNECT_FORMAT, achFormat);
			wsprintf(ach, achFormat, achTime);
			}

		sbrDrawCnctPart(hwnd, -1, (LPTSTR)ach);
		}
	else
		if (iRet != pSBRData->iLastCnctStatus)
			{
			pSBRData->iLastCnctStatus = iRet;
			sbrDrawCnctPart(hwnd, iRet, 0);
			}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrCnctTimeToSystemTime
 *
 * DESCRIPTION:
 *  Get the connection elapsed time and express it in the SYSTEMTIME structure.
 *
 * ARGUMENTS:
 *  hwnd 		- statusbar window handle.
 *	lpSysTime 	- pointer to the SYSTEMTIME structure to fill.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC BOOL sbrCnctTimeToSystemTime(const HWND hwnd,
					LPSYSTEMTIME lpSysTime, const pSBR pSBRData)
	{
	HCNCT		hCnct = (HCNCT)0;
	time_t 		tElapsed_time = (time_t)0;
	WORD		wElapsed;

	hCnct = sessQueryCnctHdl(pSBRData->hSession);

	if (hCnct == (HCNCT)0)
		return FALSE;

	// Get the elapsed time from the connection driver...
	//
	if (cnctQueryElapsedTime(hCnct, &tElapsed_time) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

	// Do the neccessary conversion to get SYSTEMTIME from the elapsed time.
	//
	wElapsed = (WORD)tElapsed_time;
	memset(lpSysTime, 0, sizeof(SYSTEMTIME));

	lpSysTime->wMonth = 1;	// Jan=1 so it can't be zero.
	lpSysTime->wHour = wElapsed/3600;
	lpSysTime->wMinute = (wElapsed%3600)/60;
	lpSysTime->wSecond = (wElapsed%3600)%60;

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrCnctStatus
 *
 * DESCRIPTION:
 *	Refresh the contents of the connection part.
 *	Query the connection status and display appropriate text in the connection
 *	part.
 *
 * ARGUMENTS:
 * 	hwnd - window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrCnctStatus(const HWND hwnd, const pSBR pSBRData)
	{
	int		iRet = -1;
	HCNCT	hCnct = (HCNCT)0;

	hCnct = sessQueryCnctHdl(pSBRData->hSession);

	if (hCnct)
		iRet = cnctQueryStatus(hCnct);

	sbrDrawCnctPart(hwnd, iRet, (LPTSTR)0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrDrawCnctPart
 *
 * DESCRIPTION:
 *	Draw a string in the connection part.
 *
 * ARGUMENTS:
 *  hwnd 		- window handle.
 *	iCnctStatus - connection status.
 *	pszStr		- if this string exists, then display it in the connection part,
 *			  otherwise read the string from the resource file according to
 *			  the value of the iCnctStatus parameter.
 *
 * RETURNS:
 *
 */
STATIC_FUNC void sbrDrawCnctPart(const HWND hwnd, const int iCnctStatus,
									LPTSTR pszStr)
	{
	TCHAR	ach[100];
	UINT	iResId;

	if (pszStr)
		{
		SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_CNCT_PART_NO, (LPARAM)pszStr);
		return;
		}

	switch (iCnctStatus)
		{
	case CNCT_STATUS_TRUE:
		iResId = IDS_STATUSBR_CONNECT;
		break;

	case CNCT_STATUS_CONNECTING:
		iResId = IDS_STATUSBR_CONNECTING;
		break;

	case CNCT_STATUS_DISCONNECTING:
		iResId = IDS_STATUSBR_DISCONNECTING;
		break;

    case CNCT_STATUS_ANSWERING:
        iResId = IDS_STATUSBR_ANSWERING;
        break;

	default:
		iResId = IDS_STATUSBR_DISCONNECT;
		break;
		}

	// For Far-East version nothing would have to be done here since we are
	// just reading a string from the resource and sending it to the common
	// control which should be able to display a string containing DB chars.
	//
	LoadString(glblQueryDllHinst(),
				iResId,
				ach,
				sizeof(ach) / sizeof(TCHAR));

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_CNCT_PART_NO, (LPARAM)(LPCTSTR)ach);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrEmulatorName
 *
 * DESCRIPTION:
 *	Refresh the contents of the emulator part.
 *	Get the emulator name and display it in the emulator part.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrEmulatorName(const HWND hwnd, const pSBR pSBRData)
	{
	TCHAR	ach[100];
	HEMU	hEmu = (HEMU)0;

	hEmu = sessQueryEmuHdl(pSBRData->hSession);
	ach[0] = TEXT('\0');

	if (hEmu)
		emuQueryName(hEmu, ach, sizeof(ach));

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_EMU_PART_NO,
		(LPARAM)(LPCTSTR)ach);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrCom
 *
 * DESCRIPTION:
 *	Handles the Com portion of the status bar
 *
 * ARGUMENTS:
 *	hwnd	 - statusbar window handle
 *	pSBRData - data for this instance
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void sbrCom(const HWND hwnd, const pSBR pSBRData)
	{
	TCHAR ach[100];

	if (cnctGetComSettingsString(sessQueryCnctHdl(pSBRData->hSession), ach,
			sizeof(ach)) == 0)
		{
		SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_COM_PART_NO, (LPARAM)ach);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrScrolLock
 *
 * DESCRIPTION:
 *	Display the status of the scroll lock key in the status bar.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *	void.
 *
 */
STATIC_FUNC void sbrScrolLock(const HWND hwnd, const pSBR pSBRData)
	{
	int		iScrl = 0, nFlag = 0;

	iScrl = (GetKeyState(VK_SCROLL) & 1);
	nFlag = (iScrl == 0) ? SBT_OWNERDRAW : 0;

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_SCRL_PART_NO | nFlag,
		(LPARAM)(LPCTSTR)pSBRData->pachSCRL);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrCapsLock
 *
 * DESCRIPTION:
 *	Refresh the display of the scroll lock key state in the status bar.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrCapsLock(const HWND hwnd, const pSBR pSBRData)
	{
	int		iCap = 0, nFlag = 0;

	iCap = (GetKeyState(VK_CAPITAL) & 1);
	nFlag = (iCap == 0) ? SBT_OWNERDRAW : 0;

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_CAPL_PART_NO | nFlag,
		(LPARAM)(LPCTSTR)pSBRData->pachCAPL);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrNumLock
 *
 * DESCRIPTION:
 *	Display the current status for the Num Lock key on the status bar.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrNumLock(const HWND hwnd, const pSBR pSBRData)
	{
	int		iNum = 0, nFlag = 0;

	iNum  = (GetKeyState(VK_NUMLOCK) & 1);
	nFlag = (iNum == 0) ? SBT_OWNERDRAW : 0;

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_NUML_PART_NO | nFlag,
		(LPARAM)(LPCTSTR)pSBRData->pachNUML);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrCapture
 *
 * DESCRIPTION:
 *	Refresh Caputre part on the status bar.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrCapture(const HWND hwnd, const pSBR pSBRData)
	{
	HCAPTUREFILE	hCapt;
	int		nCapState = 0, nFlag = 0;

	hCapt = sessQueryCaptureFileHdl(pSBRData->hSession);

	if (hCapt != (HCAPTUREFILE)0)
		nCapState = cpfGetCaptureState(hCapt);

	nFlag = (nCapState & CPF_CAPTURE_ON) ? 0 : SBT_OWNERDRAW;

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_CAPT_PART_NO | nFlag,
		(LPARAM)(LPCTSTR)pSBRData->pachCAPT);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbrPrintEcho
 *
 * DESCRIPTION:
 *	Display the status of the print echo in the status bar.
 *
 * ARGUMENTS:
 *  hwnd - window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sbrPrintEcho(const HWND hwnd, const pSBR pSBRData)
	{
	TCHAR	ach[50];
	HEMU	hEmu;
	int		nPrneStatus = 0, nFlag = 0;

	hEmu = sessQueryEmuHdl(pSBRData->hSession);
	ach[0] = TEXT('\0');

	if (hEmu != 0)
		nPrneStatus = printQueryStatus(emuQueryPrintEchoHdl(hEmu));

	nFlag = (nPrneStatus) ? 0 : SBT_OWNERDRAW;

	SendMessage(hwnd, SB_SETTEXT, (WPARAM)SBR_PRNE_PART_NO | nFlag,
		(LPARAM)(LPCTSTR)pSBRData->pachPECHO);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrCreateTimer
 *
 * DESCRIPTION:
 *  Create timer in order to show the connected time on the statusbar.
 *
 * ARGUMENTS:
 *  HWND	hwnd - statusbar window handle.
 *
 * RETURNS:
 *	TRUE - is success, FALSE - if failure
 *
 */
STATIC_FUNC BOOL sbrCreateTimer(const HWND hwnd, const pSBR pSBRData)
	{
	int		nRet;

	nRet = TimerCreate(sessQueryTimerMux(pSBRData->hSession),
					   &pSBRData->hTimer,
					   1000,
					   sbrTimerProc,
					   (void *)pSBRData);

	if (nRet != TIMER_OK)
		return FALSE;

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sbrTimerProc
 *
 * DESCRIPTION:
 *	Timer callback that simply notifies the status line to check its
 *	display components.
 *
 * ARGUMENTS:
 *	DWORD	dwhWnd	-	window handle of status line.
 *  long    uTime	-   not used.
 *
 * RETURNS:
 *	void.
 *
 */
void CALLBACK sbrTimerProc(void *pvData, long uTime)
	{
	pSBR pSBRData = (SBR *)pvData;
	SendMessage((HWND)pSBRData->hwnd, SBR_NTFY_TIMER, 0, 0L);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sbr_WM_DRAWITEM
 *
 * DESCRIPTION:
 *  When SB_SETTEXT message is sent to the statusbar with SBT_OWNERDRAW flag
 *	set a WM_DRAWITEM is posted to the parent window, in our case the session
 *	window.  This function is called from there with the data needed to draw,
 *	i.e., the pointer to DRAWITEMSTRUCT structure.
 *
 * ARGUMENTS:
 *  hwnd  - statusbar window handle.
 *	lpdis - pointer to a DRAWITEMSTRUCT structure filled with useful
 *			information needed to draw the item.
 *
 * RETURNS:
 *	void.
 *
 */
void sbr_WM_DRAWITEM(HWND hwnd, LPDRAWITEMSTRUCT lpdis)
	{
	COLORREF crSave;
	int		 nBkMode;

	// Save and set text color, mode, etc...
	//
	crSave = GetTextColor(lpdis->hDC);
	SetTextColor(lpdis->hDC, GetSysColor(COLOR_3DSHADOW));

	nBkMode = GetBkMode(lpdis->hDC);
	SetBkMode(lpdis->hDC, TRANSPARENT);

	//
	// OK, draw the text...
	//
	TextOut(lpdis->hDC, lpdis->rcItem.left + 2*GetSystemMetrics(SM_CXBORDER),
		lpdis->rcItem.top, (LPTSTR)lpdis->itemData,
			StrCharGetStrLength((LPTSTR)lpdis->itemData));

	SetTextColor(lpdis->hDC, crSave);
	SetBkMode(lpdis->hDC, nBkMode);

	return;
	}
