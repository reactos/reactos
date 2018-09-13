/*	File: D:\WACKER\tdll\backscrl.c (Created: 10-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>
#include <limits.h>

#include "stdtyp.h"
#include "tdll.h"
#include "mc.h"
#include "assert.h"
#include "session.h"
#include <emu\emu.h>
#include "update.h"
#include "backscrl.h"
#include "backscrl.hh"
#include "sess_ids.h"
#include "tchar.h"
#include "term.h"
#include "sf.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlCreate
 *
 * DESCRIPTION:
 *	Creates a server (now wudge) backsroll handle include the backscroll
 *	region itself.
 *
 * ARGUMENTS:
 *	Size of the backscoll region in bytes.
 *
 * RETURNS:
 *	Handle to a backscroll structure on success, else (HBACKSCRL)0.
 *
 */
HBACKSCRL backscrlCreate(const HSESSION hSession, const int iBytes)
	{
	int 		 i;
	HHBACKSCRL	 hBk;

	assert(hSession);

	hBk = (HHBACKSCRL)malloc(sizeof(struct stBackscrl));

	if (hBk == 0)
		{
		assert(FALSE);
		return 0;
		}

	memset(hBk, 0, sizeof(struct stBackscrl));

	hBk->hSession = hSession;
	hBk->iPages = (iBytes / BACKSCRL_PAGESIZE) + 1;

	if (hBk->iPages > BACKSCRL_MAXPAGES)
		{
		assert(FALSE);
		free(hBk);
		hBk = NULL;
		return 0;
		}

	hBk->hBkPages = (HBKPAGE *)malloc((size_t)hBk->iPages * sizeof(HBKPAGE));

	if (hBk->hBkPages == 0)
		{
		assert(FALSE);
		free(hBk);
		hBk = NULL;
		return (HBACKSCRL)0;
		}

	for (i = 0 ; i < hBk->iPages ; ++i)
		{
		hBk->hBkPages[i] = (HBKPAGE)malloc(sizeof(struct stBackscrlPage));

		if (hBk->hBkPages[i] == (HBKPAGE)0)
			{
			assert(FALSE);
			goto ERROROUT;
			}

		hBk->hBkPages[i]->pachPage =
			(ECHAR *)malloc(BACKSCRL_PAGESIZE * sizeof(ECHAR));

		if (hBk->hBkPages[i]->pachPage == 0)
			{
			assert(FALSE);
			goto ERROROUT;
			}

		ECHAR_Fill(hBk->hBkPages[i]->pachPage, ETEXT(' '), BACKSCRL_PAGESIZE);
		hBk->hBkPages[i]->iLines = 0;
		}

	hBk->iCurrPage = 0;
	hBk->iOffset = 0;
	hBk->iLines = 0;

	// Set this to some default...
	//
	hBk->iUserLines = hBk->iUserLinesSave = BKSCRL_USERLINES_DEFAULT_MAX;

	hBk->hBkPages[hBk->iCurrPage]->iLines = 0;

	return (HBACKSCRL)hBk;

	// Fanstastic error recovery.

	ERROROUT:

	while (--i > 0)
		{
		free(hBk->hBkPages[i]->pachPage);
		hBk->hBkPages[i]->pachPage = NULL;
		free(hBk->hBkPages[i]);
		hBk->hBkPages[i] = NULL;
		}

	free(hBk->hBkPages);
	hBk->hBkPages = NULL;
	free(hBk);
	hBk = NULL;
	return (HBACKSCRL)0; // caller does error message
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlDestroy
 *
 * DESCRIPTION:
 *	Routine to free memory associated with the given backscoll handle.
 *
 * ARGUMENTS:
 *	HBACKSCRL	hBackscrl	- handle to free.
 *
 * RETURNS:
 *	nothing.
 *
 */
VOID backscrlDestroy(const HBACKSCRL hBackscrl)
	{
	int i;
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;

	if (hBk == 0)
		{
		assert(0);
		return;
		}

	if (hBk->hBkPages)
		{
		for (i = 0 ; i < hBk->iPages ; ++i)
			{
			if (hBk->hBkPages[i]->pachPage)
				{
				free(hBk->hBkPages[i]->pachPage);
				hBk->hBkPages[i]->pachPage = NULL;
				}

			free(hBk->hBkPages[i]);
			hBk->hBkPages[i] = NULL;
			}

		free(hBk->hBkPages);
		hBk->hBkPages = NULL;
		}

	free(hBk);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlAdd
 *
 * DESCRIPTION:
 *	Adds a new line to the backscoll handle.  The affect is to scroll the
 *	preceding lines up by one and to add the given string to the bottom
 *	of the backscroll region.
 *
 * ARGUMENTS:
 *	HBACKSCRL	hBackscrl	- as usual
 *	LPTSTR		pachBuf 	- string to add
 *	int 	 usLen		 - length of the string.
 *
 * RETURNS:
 *	TRUE always.
 *
 */
BOOL backscrlAdd(const HBACKSCRL hBackscrl,
				 const ECHAR	 *pachBuf,
				 const int		 iLen
				)
	{
	register int i;
	ECHAR *pachBackscrl;
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;

	if (hBk == 0)
		{
		assert(0);
		return FALSE;
		}

	if (hBk->iUserLines == 0)
		return TRUE;

	// The following test has been removed because the emualtors no
	// longer use a '\0' to delimit the end of a line.	5/16/94 --jcm

	// Never let '\0's into the backscroll buffer!	Avoid this like the
	// plague since they display as wierd characters depending on the
	// font selected on the CLIENT side.  The wierd part is you can't
	// always depend on the emulators putting a '\0' in the the buffer
	// so we need to check for trailing space as well.

	for (i = 0 ; i < iLen ; ++i)
		{
		if (pachBuf[i] == (ECHAR)0)
			break;
		}

	// remove trailing whitespace.

	while (i)
		{
		if (pachBuf[i - 1] != ETEXT(' '))
			break;

		i -= 1;
		}

	DbgOutStr("%d-", i, 0, 0, 0, 0);

	// check to see if there is room on the current page.

	if (hBk->iOffset >= BACKSCRL_PAGESIZE ||
			((int)BACKSCRL_PAGESIZE - hBk->iOffset) <= i)
		{
		// pad rest of page with blanks so we know that this part of
		// the buffer is empty.

		if ((pachBackscrl = hBk->hBkPages[hBk->iCurrPage]->pachPage) == 0)
			{
			assert(0);
			return FALSE;
			}

		ECHAR_Fill(pachBackscrl+hBk->iOffset, ETEXT(' '),
			(size_t)(BACKSCRL_PAGESIZE - hBk->iOffset));

		hBk->iCurrPage += 1;

		if (hBk->iCurrPage >= hBk->iPages)
			hBk->iCurrPage = 0;

		hBk->iOffset = 0;

		// If we have wrapped, subtract the number of lines previously in
		// this page from the total line count.  Since the line count is
		// intialized to 0 (see backscrlCreate()) I can always subract this
		// amount without checking for wrapping since it will only be
		// non-zero if we have wrapped.

		hBk->iLines -= hBk->hBkPages[hBk->iCurrPage]->iLines;
		hBk->hBkPages[hBk->iCurrPage]->iLines = 0;
		}

	// Assign a pointer for speed and clarity

	if ((pachBackscrl = hBk->hBkPages[hBk->iCurrPage]->pachPage) == 0)
		{
		assert(0);
		return FALSE;
		}


        // JYF 26-Mar-1999 limit the size so we don't overrun
        //  the buffer.

        if (i)
            {
            MemCopy (
                    pachBackscrl + hBk->iOffset,
                    pachBuf,
                    (size_t)min(BACKSCRL_PAGESIZE - hBk->iOffset,
                                (int) (i* sizeof(ECHAR)))
                    );
            }

        hBk->iOffset += min(BACKSCRL_PAGESIZE -hBk->iOffset -1,
                                    (int) (i* sizeof(ECHAR)));



	pachBackscrl[hBk->iOffset++] = ETEXT('\n');

	// Here's an interesting problem.  We really can't reference more than
	// a signed-integer's worth of lines, but we may have megabytes of
	// backscroll memory.  The answer is simple in this case.  Never allow
	// the line count to exceed the signed int max.  This has the affect
	// of spilling off the top lines in the buffer. - mrw

	hBk->iLines = min(hBk->iLines+1, INT_MAX);
	hBk->hBkPages[hBk->iCurrPage]->iLines += 1;
	hBk->iChanged = TRUE;
	updateBackscroll(sessQueryUpdateHdl(hBk->hSession), 1);
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlGetBkLines
 *
 * DESCRIPTION:
 *	Retrieves specifed lines from the backscoll.  This function is
 *	complicated by the fact the backscoll memory is paged.	A request
 *	might cross one or more page boundaries.  Thus only a portion of
 *	the request may be satisfied.  The client knows this and makes new
 *	requests based on what it got from the server.
 *
 * ARGUMENTS:
 *	hBackscrl	- the usual
 *	yBeg		- begining line in backscroll to get.
 *	sWant		- number of lines requested.
 *	psGot		- number of lines retrived.
 *	lpststrTxt	- handle to backscrl memory page retrived.
 *	pwOffset	- offset into retrieved page (in TCHAR units).
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL backscrlGetBkLines(const HBACKSCRL hBackscrl,
						const int	yBeg,
						const int	sWant,
						int 	   *psGot,
						ECHAR	   **lptstrTxt,
						int 	   *pwOffset
						)
	{
	register int	 i, j, k;
	ECHAR			 *pachText;
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;

	assert(sWant > 0);

	// Check to see if we are requesting beyond the end of the
	// backscroll buffer.

	if (abs(yBeg) > hBk->iLines)
		return FALSE;

	k = hBk->iCurrPage;
	j = 0;
	i = 0;

	// Find the backscoll page that has the requested text
	//
	for (;;)
		{
		if ((j -= hBk->hBkPages[k]->iLines) <= yBeg)
			break;

		k = (hBk->iPages + k - 1) % hBk->iPages;

		if (++i >= hBk->iPages)
			return FALSE;
		}


	// Found the page.
	//
	*lptstrTxt = hBk->hBkPages[k]->pachPage;

	// Now find offset into page where first line of requested text begins
	//
	for (pachText = hBk->hBkPages[k]->pachPage ; j < yBeg ; ++j)
		{
		while (*pachText != ETEXT('\n'))
			{
			pachText += 1;
			}

		pachText += 1;
		}

	*pwOffset = (DWORD)(pachText - hBk->hBkPages[k]->pachPage);

	// Found offset.  Now grab what we can and return it.
	//
	for (i = 1 ; i <= sWant ; ++i)
		{
		while ((pachText - hBk->hBkPages[k]->pachPage) < BACKSCRL_PAGESIZE
				&& *pachText != ETEXT('\n'))
			{
			pachText += 1;
			}

		if ((pachText - hBk->hBkPages[k]->pachPage) >= BACKSCRL_PAGESIZE)
			break;

		*psGot = i;
		pachText += 1; // blow past newline
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlGetNumLines
 *
 * DESCRIPTION:
 *	Returns the number of lines in the backscrl which is zero if the
 *	backscroll is off, and the maximum is always the user set value.
 *
 * ARGUMENTS:
 *	HBACKSCRL hBackscrl 	- external backscrl handle
 *
 * RETURNS:
 *	Returns the uLines member.
 */
int backscrlGetNumLines(const HBACKSCRL hBackscrl)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	assert(hBk);
	return (hBk->fShowBackscrl) ? min(hBk->iUserLines, hBk->iLines) : 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlUSetNumLines
 *
 * DESCRIPTION:
 *	Returns the iUserLines member.
 *
 * ARGUMENTS:
 *	HBACKSCRL hBackscrl 	- external backscrl handle
 *
 * RETURNS:
 *	void
 */
int backscrlSetUNumLines(const HBACKSCRL hBackscrl, const int iUserLines)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;

	if (hBk == 0)
		{
		assert(0);
		return -1;
		}

	if (iUserLines != hBk->iUserLines)
		{
		backscrlChanged(hBackscrl);
		hBk->iUserLines = iUserLines;

		// If we're setting the number of lines to zero, we're essentially
		// disabling the backscroll.  Flushing clears the screen as well.
		//
		if (iUserLines == 0)
			backscrlFlush(hBackscrl);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlGetUNumLines
 *
 * DESCRIPTION:
 *	Returns the iUserLines member.
 *
 * ARGUMENTS:
 *	HBACKSCRL hBackscrl 	- external backscrl handle
 *
 * RETURNS:
 *	Returns the iUserLines member.
 */
int backscrlGetUNumLines(const HBACKSCRL hBackscrl)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	return hBk->iUserLines;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlRead
 *
 * DESCRIPTION:
 *  Read the number of backscrol lines to keep as entered by the user.
 *	NOTE: This should be put in the backscrlInitializeHdl() when this function
 *	gets written.
 *
 * ARGUMENTS:
 *	HBACKSCRL hBackscrl 	- external backscrl handle
 *
 * RETURNS:
 */
void backscrlRead(const HBACKSCRL hBackscrl)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	unsigned long ulSize;

	ulSize = sizeof(hBk->iUserLines);
	hBk->iUserLines = BKSCRL_USERLINES_DEFAULT_MAX;
	sfGetSessionItem(sessQuerySysFileHdl(hBk->hSession),
						SFID_BKSC_ULINES,
						&ulSize,
						&hBk->iUserLines);
	hBk->iUserLinesSave = hBk->iUserLines;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlSave
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *	HBACKSCRL hBackscrl 	- external backscrl handle
 *
 * RETURNS:
 */
void backscrlSave(const HBACKSCRL hBackscrl)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	unsigned long ulSize;

	if (hBk->iUserLines != hBk->iUserLinesSave)
		{
		ulSize = sizeof(int);
		sfPutSessionItem(sessQuerySysFileHdl(hBk->hSession),
						SFID_BKSC_ULINES,
						ulSize,
						&(hBk->iUserLines));
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlFlush
 *
 * DESCRIPTION:
 *	Empties the backscroll buffer and notifies the terminal so it can
 *	update it's display.
 *
 *	Note: Because this function calls RefreshTermWindow() it should only
 *		  be called from the main thread. - mrw
 *
 * ARGUMENTS:
 *	hBackscrl	- public backscroll handle
 *
 * RETURNS:
 *	void
 *
 */
void backscrlFlush(const HBACKSCRL hBackscrl)
	{
	int i;
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	ECHAR aechBuf[10];

	assert(hBk);

	/* --- Shouldn't need this unless this is called while on line --- */

	emuLock(sessQueryEmuHdl(hBk->hSession));

	/* --- Force the update records to have something in them --- */

	CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), " ", 1);
	backscrlAdd(hBackscrl, aechBuf, 1);

	/* --- Empty all pages --- */

	for (i = 0 ; i < hBk->iPages ; ++i)
		hBk->hBkPages[i]->iLines = 0;

	hBk->iLines = 0;
	hBk->iOffset = 0; //mrw:6/19/95

	emuUnlock(sessQueryEmuHdl(hBk->hSession));

	/* --- Let the terminal update now --- */

	NotifyClient(hBk->hSession, EVENT_TERM_UPDATE, 0);
	RefreshTermWindow(sessQueryHwndTerminal(hBk->hSession));
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlChanged
 *
 * DESCRIPTION:
 *	Returns iChanged member which is set whenever anything is added
 *	to the backscroll buffer.  It can be cleared by calling
 *	backscrlResetChangedFlag().
 *
 * ARGUMENTS:
 *	hBackscrl	- public backscroll handle
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL backscrlChanged(const HBACKSCRL hBackscrl)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	assert(hBk);
	return hBk->iChanged;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlResetChangedFlag
 *
 * DESCRIPTION:
 *	Resets the iChanged member to 0.  Subsequent calls to backscrlAdd()
 *	will set the flag to 1.
 *
 * ARGUMENTS:
 *	hBackscrl	- public backscrl handle
 *
 * RETURNS:
 *	void
 *
 */
void backscrlResetChangedFlag(const HBACKSCRL hBackscrl)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;
	assert(hBk);
	hBk->iChanged = 0;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	backscrlSetShowFlag
 *
 * DESCRIPTION:
 *	The show flag controls whether or not the session will show/display
 *	an antive backscrl.
 *
 * ARGUMENTS:
 *	hBackscrl	- public backscrl handle.
 *	fFlag		- TRUE=show, FALSE=hide
 *
 * RETURNS:
 *	void
 *
 */
void backscrlSetShowFlag(const HBACKSCRL hBackscrl, const int fFlag)
	{
	const HHBACKSCRL hBk = (HHBACKSCRL)hBackscrl;

	assert(hBk);
	hBk->fShowBackscrl = fFlag;
	return;
	}
