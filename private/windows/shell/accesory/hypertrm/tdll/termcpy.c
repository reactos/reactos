/*	File: D:\WACKER\tdll\termcpy.c (Created: 24-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "mc.h"
#include "tdll.h"
#include "tchar.h"
#include "session.h"
#include "backscrl.h"
#include "assert.h"
#include <emu\emu.h>
#include "term.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CopyMarkedTextFromTerminal
 *
 * DESCRIPTION:
 *	Helper function that gets the marked region and passes the call
 *	onto CopyTextFromTerminal().
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *	ppv 		- pointer to a buffer pointer
 *	pdwCnt		- pointer to size variable
 *	fIncludeLF	- TRUE means include linefeeds.
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL CopyMarkedTextFromTerminal(const HSESSION hSession, void **ppv,
								DWORD *pdwCnt, const BOOL fIncludeLF)
	{
	BOOL  fReturn;
	ECHAR *pechBuf;
	TCHAR *pszOutput;
	const HWND	hwndTerm = sessQueryHwndTerminal(hSession);
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwndTerm, GWLP_USERDATA);

	fReturn = CopyTextFromTerminal(hSession, &hhTerm->ptBeg, &hhTerm->ptEnd,
									ppv, pdwCnt, fIncludeLF);

	pechBuf = *ppv;
	// Strip Out Any repeated Characters in the string
	StrCharStripDBCSString(pechBuf, (long)StrCharGetEcharByteCount(pechBuf), pechBuf);

	// hMem currently points to an array of ECHAR's, convert this to
	// TCHARS before giving the results to the caller.
	pszOutput = malloc((ULONG)StrCharGetEcharByteCount(pechBuf) + 1);
	CnvrtECHARtoMBCS(pszOutput, (ULONG)StrCharGetEcharByteCount(pechBuf) + 1,
						pechBuf,StrCharGetEcharByteCount(pechBuf)+1);//mrw:5/17/95
	*(pszOutput + StrCharGetEcharByteCount(pechBuf)) = ETEXT('\0');
	free(pechBuf);
	pechBuf = NULL;
	*ppv = pszOutput;
	*pdwCnt = (ULONG)StrCharGetByteCount(pszOutput);

	return fReturn;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CopyTextFromTerminal
 *
 * DESCRIPTION:
 *	A work-horse function that will use the given points as a marked region
 *	and do the necessary work to make a copy of the marked text into
 *	a new memory region.  The marked range can include the backscroll
 *	region and cross into the terminal region.	The begining and ending
 *	points are in terminal coordinates (backscroll lines are numbered
 *	-1 to -infinity, terminal lines are numbered 1 to 24, line 0 is the
 *	divider line and does not exist for copies but can be included in
 *	the range).  It is *not* a requirement that the begininig point be
 *	less than the ending point.
 *
 * ARGUMENTS:
 *	HSESSION	hSession	- handle to the session.
 *	LPPOINT 	lpptBeg 	- begining point of marked region.
 *	LPPOINT 	lpptEnd 	- end point of mark region.
 *	void	  **pv			- pointer to buffer pointer
 *	DWORD	   *pdwCnt		- pointer to double word for size
 *	BOOL		fIncludeLF	-
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL CopyTextFromTerminal(const HSESSION hSession,
						  const PPOINT pptBeg,
						  const PPOINT pptEnd,
						  void **ppv,
						  DWORD *pdwCnt,
						  const BOOL fIncludeLF)
	{
	int 		iWant,
				iGot,
				yLn,
				xCol,
				iLen,
				iTmp,
				iRows,
				iCols,
				iTopline,
				iOffset,
				iBkLns,
                iSize;
	DWORD		dwSize, dwOldSize;
	ECHAR		*lpachTxt;
	ECHAR		*lpachBuf;
	ECHAR		achTmp[256],
			   *pachTmp;
	ECHAR	   **alpstrTxt;
	long		lBeg, lEnd;
	HCURSOR 	hCursor;
	ECHAR		*hMem,				   // malloc memory
				*hMemTmp = 0;		   // malloc memory
	POINT		ptBeg, ptEnd, ptTmp;
	const HWND	hwndTerm = sessQueryHwndTerminal(hSession);
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwndTerm, GWLP_USERDATA);
	const HBACKSCRL hBackscrl = sessQueryBackscrlHdl(hhTerm->hSession);

	*ppv = 0;
	*pdwCnt = 0;

	hCursor = SetCursor(LoadCursor((HINSTANCE)0, IDC_WAIT));
	dwSize = 1; // By starting at one, we always have room for the '\0'.

	iRows = hhTerm->iRows;
	iCols = hhTerm->iCols;
	iTopline = hhTerm->iTopline;
	alpstrTxt = hhTerm->fplpstrTxt;

	ptBeg = *pptBeg;
	ptEnd = *pptEnd;

	/* --- do some range checking for the line number --- */

	iBkLns = backscrlGetNumLines(hBackscrl);
	iBkLns = -iBkLns;

	if (ptBeg.y > 0 && ptBeg.y > iRows)
		ptBeg.y = iRows;

	if (ptBeg.y < 0 && ptBeg.y < iBkLns)
		ptBeg.y = iBkLns;

	if (ptEnd.y > 0 && ptEnd.y > iRows)
		ptEnd.y = iRows;

	if (ptEnd.y < 0 && ptEnd.y < iBkLns)
		ptEnd.y = iBkLns;

	/* --- and do some range checking for the column number --- */

	if (ptBeg.x < 0)		/* negative column doesn't make much sense */
		ptBeg.x = 0;

	if (ptBeg.x >= iCols)
		ptBeg.x = iCols - 1;

	if (ptEnd.x < 0)		/* see above */
		ptEnd.x = 0;

	if (ptEnd.x >= iCols)
		ptEnd.x = iCols - 1;

	/* --- convert to offsets --- */

	lBeg = (ptBeg.y * iCols) + ptBeg.x;
	lEnd = (ptEnd.y * iCols) + ptEnd.x;

	if (lBeg == lEnd)
		{
		SetCursor(hCursor);
		return FALSE;
		}

	sessSetSuspend(hSession, SUSPEND_TERMINAL_COPY);

	if (lBeg > lEnd)
		{
		ptTmp = ptEnd;
		ptEnd = ptBeg;
		ptBeg = ptTmp;
		}

	if ((hMem = malloc(dwSize * sizeof(ECHAR))) == 0)
		{
		assert(FALSE);
		SetCursor(hCursor);
		sessClearSuspend(hSession, SUSPEND_TERMINAL_COPY);
		return FALSE;
		}

	// Get first line of range

	iGot = 0;
	lpachTxt = 0;
	achTmp[0] = ETEXT('\0');

	if (ptBeg.y < 0)
		{
		xCol  = ptBeg.x;
		yLn   = ptBeg.y;
		iWant = ptEnd.y - ptBeg.y + 1;

		if (iWant > 0)
			{
			while (iWant > 0)
				{
				if (backscrlGetBkLines(hBackscrl, yLn, iWant, &iGot,
						&lpachTxt, &iOffset) == FALSE)
					{
					goto PROCESS_ERROR;
					}

				lpachTxt += iOffset;

				// Lets say the user has highlighted an area of the backscroll
				// region that has no data (ie. a blank line).	Adding xCol to
				// lpachTxt is incorrect.  This for-loop checks for this
				// condition and resets lpachTxt to the beginning of the line.
				// Incidentally, this also sets xCol to 0 which is necessary
				// for subsequent lines to feed in correctly.

				for (lpachBuf = lpachTxt ; xCol > 0 ; --xCol, ++lpachTxt)
					{
					if (*lpachTxt == ETEXT('\n'))
						{
						lpachTxt = lpachBuf;
						xCol = 0;
						break;
						}
					}

				iWant -= iGot;

				while (iGot-- > 0 && yLn < ptEnd.y && yLn < -1)
					{
					for (pachTmp = achTmp, iTmp = 0 ;
						 *lpachTxt != ETEXT('\n') && iTmp < MAX_EMUCOLS ;
						 ++pachTmp, ++iTmp)
						{
						*pachTmp = *lpachTxt++;
						}

					// Fail Safe:  if the text we received from
					// backscrlGetBkLines() is mangled somehow and we
					// don't see a newline, then abort before we seg.

					if (iTmp >= MAX_EMUCOLS)
						goto PROCESS_ERROR;

					lpachTxt += 1;
					*pachTmp++ = ETEXT('\r');

					if (fIncludeLF)
						*pachTmp++ = ETEXT('\n');

					*pachTmp   = ETEXT('\0');

					hMemTmp = hMem;
					dwOldSize = dwSize - 1;
					dwSize += (DWORD)StrCharGetEcharLen(achTmp);
					hMem = realloc(hMemTmp, dwSize *sizeof(ECHAR));

					if (hMem == 0)
						goto PROCESS_ERROR;

					/* remember how a realloc works */
					hMemTmp = (ECHAR *)0;

					lpachBuf = hMem + dwOldSize;
					iSize = StrCharGetEcharByteCount(achTmp);
                    if ( iSize )
                        MemCopy(lpachBuf, achTmp, iSize );
					hMem[dwSize - 1] = ETEXT('\0');

					yLn += 1;
					}

				if (yLn >= ptEnd.y || yLn >= -1)
					break;
				}

			// Another bug bytes the dust.	Wasn't checking to see if iGot
			// was the reason we exited the loop above.  It caused things
			// to seg occassionally.

			if (iGot >= 0)
				{
				// Last line stuff (could be crossing into terminal area but
				// we still need to clean-up backscroll stuff.

				if (ptBeg.y == ptEnd.y)
					xCol = ptEnd.x - ptBeg.x;

				else if (ptEnd.y < 0) /* && ptEnd.x > 0) removed - mrw */
					xCol = ptEnd.x;

				else
					xCol = iCols;

				for (pachTmp = achTmp ; xCol-- > 0 && *lpachTxt != ETEXT('\n'); ++pachTmp)
					*pachTmp = *lpachTxt++;

				if (ptEnd.y >= 0)
					{
					*pachTmp++ = ETEXT('\r');

					if (fIncludeLF)
						*pachTmp++ = ETEXT('\n');
					}

				*pachTmp = ETEXT('\0');
				}

			hMemTmp = hMem;
			dwOldSize = dwSize - 1;
			dwSize += (DWORD)StrCharGetEcharLen(achTmp);

			hMem = realloc(hMemTmp, dwSize * sizeof(ECHAR));

			if (hMem == 0)
				goto PROCESS_ERROR;

			/* remember how a realloc works */
			hMemTmp = (ECHAR *)0;

			lpachBuf = hMem + dwOldSize;
			iSize = StrCharGetEcharByteCount(achTmp);
            if ( iSize )
                MemCopy(lpachBuf, achTmp, iSize);
			hMem[dwSize - 1] = ETEXT('\0');
			} // end if (iWant > 0)

		// terminal area...

		if (ptEnd.y >= 0)
			{
			yLn = 1;

			while (yLn <= ptEnd.y)
				{
				iTmp = ((yLn + iTopline) % MAX_EMUROWS) - 1;

				if (iTmp < 0)
					iTmp = MAX_EMUROWS - 1;

				lpachTxt = alpstrTxt[iTmp];

				iLen = strlentrunc(lpachTxt, iCols);
				xCol = (yLn == ptEnd.y) ? min(ptEnd.x, iLen) : iLen;

				for (pachTmp = achTmp ; xCol > 0 ; --xCol)
					*pachTmp++ = *lpachTxt++;

				if (yLn != ptEnd.y)
					{
					*pachTmp++ = ETEXT('\r');

					if (fIncludeLF)
						*pachTmp++ = ETEXT('\n');
					}

				*pachTmp = ETEXT('\0');

				hMemTmp = hMem;
				dwOldSize = dwSize - 1;
				dwSize += (DWORD)StrCharGetEcharLen(achTmp);
				hMem = realloc(hMemTmp, dwSize * sizeof(ECHAR));

				if (hMem == 0)
					goto PROCESS_ERROR;
	
				/* remember how a realloc works */
				hMemTmp = (ECHAR *)0;

				lpachBuf = hMem + dwOldSize;
			    iSize = StrCharGetEcharByteCount(achTmp);
                if ( iSize )
    				MemCopy(lpachBuf, achTmp, iSize );
				hMem[dwSize - 1] = ETEXT('\0');

				yLn += 1;
				}
			}
		}

	else // terminal only case
		{
		if (ptBeg.y >= 0)
			{
			yLn = ptBeg.y;

			while (yLn <= ptEnd.y)
				{
				iTmp = ((yLn + iTopline) % MAX_EMUROWS) - 1;

				if (iTmp < 0)
					iTmp = MAX_EMUROWS - 1;

				lpachTxt = alpstrTxt[iTmp];

				if (ptBeg.y == ptEnd.y)
					{
					lpachTxt += ptBeg.x;

					xCol = min(strlentrunc(lpachTxt, iCols - ptBeg.x),
						ptEnd.x - ptBeg.x);
					}

				else if (yLn == ptBeg.y)
					{
					lpachTxt += ptBeg.x;
					xCol = strlentrunc(lpachTxt, iCols - ptBeg.x);
					}

				else if (yLn == ptEnd.y)
					{
					xCol = min(ptEnd.x, strlentrunc(lpachTxt, iCols));
					}

				else
					{
					xCol = strlentrunc(lpachTxt, iCols);
					}


				for (pachTmp = achTmp ; xCol > 0 ; --xCol)
					*pachTmp++ = *lpachTxt++;


				if (yLn != ptEnd.y)
					{
					*pachTmp++ = ETEXT('\r');

					if (fIncludeLF)
						*pachTmp++ = ETEXT('\n');
					}

				*pachTmp = ETEXT('\0');

				hMemTmp = hMem;
				dwOldSize = dwSize - 1;
				dwSize += (DWORD)StrCharGetEcharLen(achTmp);
				hMem = realloc(hMemTmp, dwSize * sizeof(ECHAR));

				if (hMem == 0)
					goto PROCESS_ERROR;

				/* remember how a realloc works */
				hMemTmp = (ECHAR *)0;

				lpachBuf = hMem + dwOldSize;
				
			    iSize = StrCharGetEcharByteCount(achTmp);
                if ( iSize )
                    MemCopy(lpachBuf, achTmp, iSize );
				hMem[dwSize - 1] = ETEXT('\0');

				yLn += 1;
				}
			}
		}

	SetCursor(hCursor);

	*ppv = hMem;
	*pdwCnt = (ULONG)StrCharGetEcharByteCount(hMem);
	sessClearSuspend(hSession, SUSPEND_TERMINAL_COPY);
	return TRUE;

	// process error condition here.

	PROCESS_ERROR:

	if (hMemTmp)
		{
		free(hMemTmp);
		hMemTmp = NULL;
		}

	SetCursor(hCursor);
	sessClearSuspend(hSession, SUSPEND_TERMINAL_COPY);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	strlentrunc
 *
 * DESCRIPTION:
 *	Finds the length of a character buffer less the trailing whitespace.
 *
 * ARGUMENTS:
 *	pach	- array of t characters
 *	iLen	- length of buffer
 *
 * RETURNS:
 *	new length
 *
 */
int strlentrunc(const ECHAR *pach, const int iLen)
	{
	int i;

	for (i = iLen-1 ; i >= 0 ; --i)
		{
		switch (pach[i])
			{
		/* Whitespace characters */
		case ETEXT('\0'):
		case ETEXT('\t'):
		case ETEXT('\n'):
		case ETEXT('\v'):
		case ETEXT('\f'):
		case ETEXT('\r'):
		case ETEXT(' '):
			break;

		default:
			return i+1;
			}
		}

	return 0;
	}
