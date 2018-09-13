/*	File: D:\WACKER\tdll\update.c (Created: 09-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:36p $
 */

#include <windows.h>
#pragma hdrstop

#include <math.h>

#include "stdtyp.h"
#include "mc.h"
#include "assert.h"
#include "session.h"
#include <emu\emu.h>

#include "update.h"
#include "update.hh"

static HHUPDATE VerifyUpdateHdl(const HUPDATE hUpdate);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateCreate
 *
 * DESCRIPTION:
 *	Creates and initializes an update record.  Make sure to call
 *	UpdateDestroy when before killing the session or pay dear and dire
 *	consequences in lost memory blocks - a Windows no-no.
 *
 * ARGUMENTS:
 *	None, zippo, nil, nada, zilch, nothing.
 *
 * RETURNS:
 *	If you're lucky, and it's Tuesday, and I can program (highly unlikely),
 *	a handle (read pointer) to the update record.  Otherwize, a NULL pointer
 *	indicating that you're a memory piggy.
 *
 */
HUPDATE updateCreate(const HSESSION hSession)
	{
	HHUPDATE hUpd;

	// Get some space for the update record.

	hUpd = (HHUPDATE)malloc(sizeof(struct stUpdate));

	if (hUpd == (HHUPDATE)0)		  // release version returns NULL
		{
		assert(FALSE);
		return 0;
		}

	memset(hUpd, 0, sizeof(struct stUpdate));
	hUpd->hSession = hSession;
	updateReset(hUpd);

	return (HUPDATE)hUpd;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateDestory
 *
 * DESCRIPTION:
 *	Releases memory allocated for Update record.
 *
 * ARGUMENTS:
 *	HUPDATE 	hUpdate -- handle of update record to nuke.
 *
 * RETURNS:
 *	void
 */
void updateDestroy(const HUPDATE hUpdate)
	{
	HHUPDATE hUpd = (HHUPDATE)hUpdate;

	if (hUpd == (HHUPDATE)0)
		return;

	free(hUpd);
	hUpd = NULL;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	VerifyUpdateHdl
 *
 * DESCRIPTION:
 *	Verifies that we have a valid update handle.
 *
 * ARGUMENTS:
 *	hUpdate - external update handle
 *
 * RETURNS:
 *	Internal update handle or zero on error.
 *
 */
static HHUPDATE VerifyUpdateHdl(const HUPDATE hUpdate)
	{
	const HHUPDATE hUpd = (HHUPDATE)hUpdate;

	if (hUpd == 0)
		{
		assert(0);
		ExitProcess(1);
		}

	return hUpd;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateReset
 *
 * DESCRIPTION:
 *	Resets the update record to its initial state.
 *
 * ARGUMENTS:
 *	HUPDATE 	hUpdate -- handle of update record to reset.
 *
 * RETURNS:
 *	nothing
 *
 */
void updateReset(const HHUPDATE hUpd)
	{
	// Something tricky going on here.	I purposely don't reset the
	// sTopline, sRow, sCol, and usCType values so that they persist.
	// This avoids some problems on Client side of trying to figure
	// out what has changed.

	hUpd->bUpdateType = UPD_LINE;
	hUpd->fUpdateLock = FALSE;
	hUpd->stLine.iLine = -1;
	hUpd->fRealloc = FALSE;

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateScroll
 *
 * DESCRIPTION:
 *	Modifies update record to reflect the specified scrolling operation.
 *
 * ARGUMENTS:
 *	Never argues.  Just does what it's told to do.
 *
 *	HUPDATE 	hUpdate 	-- the update record of course.
 *	int 		yBeg		-- begining line # of scroll region (inclusive)
 *	int 		yEnd		-- ending line # of scroll region (inclusive)
 *	int 		sScrlInc	-- Amount to scroll (negative or positive)
 *	int 		sTopline	-- line in image buf of screen row 0 (emu_imgtop)
 *	BOOL		fSave		-- wheather to save line to backscroll buffer.
 *
 * RETURNS:
 *	Nothing
 *
 */
void updateScroll(const HUPDATE hUpdate,
				  const int yBeg,
				  const int yEnd,
				  const int iScrlInc,
				  const int iTopLine,
				  const BOOL  fSave)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);
	struct stScrlMode *pstScrl;
	int  i, j;

	pstScrl = &hUpd->stScrl;

	DbgOutStr("iScrollInc=%d, yBeg=%d, yEnd=%d, iTopLine=%d\r\n",
		iScrlInc, yBeg, yEnd, iTopLine, 0);

	if (hUpd->bUpdateType != UPD_SCROLL)
		{
		DbgOutStr("Trans to Scroll Mode\r\n", 0, 0, 0, 0, 0);
		i = hUpd->stLine.iLine; 	// save this value for test below.

		// Setup initial scroll mode paramaters.

		hUpd->bUpdateType = UPD_SCROLL;
		pstScrl->iFirstLine = 0;
		pstScrl->iScrlInc = 0;
		pstScrl->iRgnScrlInc = 0;
		pstScrl->iBksScrlInc = 0;
		pstScrl->yBeg = 0;
		pstScrl->yEnd = 0;

		memset(pstScrl->auchLines, 0, UPD_MAXLINES * sizeof(BYTE));

		// If we were updating in line mode, make sure to mark that line
		// in scroll mode.

		if (i != -1)
			{
			pstScrl->auchLines[i] = (UCHAR)1;
			DbgOutStr("Trans -> %d\r\n", i, 0, 0, 0, 0);
			}
		}

	hUpd->iTopline = iTopLine;
	pstScrl->fSave = fSave;

	emuQueryRowsCols(sessQueryEmuHdl(hUpd->hSession), &j, &i);

	// Adjust for zero offset used by emulators...
	//
	j -= 1;

	/* -------------- Full screen scroll-up case ------------- */

	if (yBeg == 0 && yEnd == j && iScrlInc > 0)
		{
		pstScrl->yBeg = yBeg;
		pstScrl->yEnd = yEnd;

		pstScrl->iScrlInc += iScrlInc;

		pstScrl->iFirstLine =
			min(pstScrl->iFirstLine+iScrlInc, (UPD_MAXLINES-1)/2);
		}

	/* -------------- Region scroll and scroll-down ------------- */

	else
		{
		if (iScrlInc > 0)
			pstScrl->iRgnScrlInc += iScrlInc;

		memset(pstScrl->auchLines + pstScrl->iFirstLine + yBeg, 1,
			(abs(yEnd-yBeg)+1) * sizeof(UCHAR));
		}

	if ((pstScrl->iScrlInc + pstScrl->iRgnScrlInc) >= hUpd->iScrlMax)
		hUpd->fUpdateLock = TRUE;

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateBackscroll
 *
 * DESCRIPTION:
 *	This function updates the number of lines scrolled in the backscroll
 *	buffer which may be different than the number of lines scrolled in
 *	the terminal.  I had to decouple the scrolling of these two regions
 *	to handle the goofy way internet systems clear the screen.	This
 *	function is only called by backscrlAdd().
 *
 * ARGUMENTS:
 *	HUPDATE 	hUpdate 	- the update record of course.
 *	int 		iLInes		- number of lines to scroll backscroll buffer.
 *
 * RETURNS:
 *	void
 *
 */
void updateBackscroll(const HUPDATE hUpdate, const int iLines)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);
	struct stScrlMode *pstScrl;
	int  i;

	pstScrl = &hUpd->stScrl;

	if (hUpd->bUpdateType != UPD_SCROLL)
		{
		DbgOutStr("Trans to Scroll Mode (BS)\r\n", 0, 0, 0, 0, 0);
		i = hUpd->stLine.iLine; 	// save this value for test below.

		// Setup initial scroll mode paramaters.

		hUpd->bUpdateType = UPD_SCROLL;
		pstScrl->iFirstLine = 0;
		pstScrl->iScrlInc = 0;
		pstScrl->iRgnScrlInc = 0;
		pstScrl->iBksScrlInc = 0;
		pstScrl->yBeg = 0;
		pstScrl->yEnd = 0;

		memset(pstScrl->auchLines, 0, UPD_MAXLINES * sizeof(BYTE));

		// If we were updating in line mode, make sure to mark that line
		// in scroll mode.

		if (i != -1)
			{
			pstScrl->auchLines[i] = (UCHAR)1;
			DbgOutStr("Trans -> %d\r\n", i, 0, 0, 0, 0);
			}
		}

	pstScrl->iBksScrlInc += iLines;
	DbgOutStr("iBksScrlInc = %d\r\n", pstScrl->iBksScrlInc, 0, 0, 0, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateLine
 *
 * DESCRIPTION:
 *	Modifies the update record line array for the given line range.
 *
 * ARGUMENTS:
 *	HUPDATE 	hUpdate 	-- the update record of course.
 *	int 		yBeg		-- begining line # (inclusive)
 *	int 		yEnd		-- ending line # (inclusive)
 *
 * RETURNS:
 *	void
 *
 */
void updateLine(const HUPDATE hUpdate, const int yBeg, const int yEnd)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);
	BYTE *pauchLines;
	struct stScrlMode *pstScrl;

	assert(hUpd != (HHUPDATE)0);

	pstScrl = &hUpd->stScrl;			 // for speed...

	if (hUpd->bUpdateType != UPD_SCROLL)
		{
		DbgOutStr("Trans to Line Mode\r\n", 0, 0, 0, 0, 0);

		memset(pstScrl->auchLines, 0, UPD_MAXLINES);

		// 10/20/92 - This if statement fixes a bug that caused the
		// update records to miss a line that was being updated in
		// character mode and then jumping to another line - mrw.

		if (hUpd->stLine.iLine != -1)
			{
			pauchLines = pstScrl->auchLines + hUpd->stLine.iLine;
			*pauchLines = (UCHAR)1;
			DbgOutStr("Trans -> %d\r\n", hUpd->stLine.iLine, 0, 0, 0, 0);
			}

		hUpd->bUpdateType = UPD_SCROLL;

		pstScrl->yBeg	  = 0;
		pstScrl->yEnd	  = 0;
		pstScrl->iScrlInc = 0;
		pstScrl->iRgnScrlInc = 0;
		pstScrl->iBksScrlInc = 0;
		pstScrl->fSave	  = FALSE;
		pstScrl->iFirstLine = 0;
		}

	memset(pstScrl->auchLines+pstScrl->iFirstLine+yBeg, 1, yEnd-yBeg+1);
	DbgOutStr("Line -> %d - %d\r\n", yBeg, yEnd, 0, 0, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateChar
 *
 * DESCRIPTION:
 *	Modifies the update record when it is in character mode.  This is a
 *	complex function but it handles a character a quickly as possible.
 *	Usually, only one or two checks are made.  Also, if this routine
 *	is called in line mode, it calls the appropriate line mode routine.
 *
 * ARGUMENTS:
 *	HUPDATE 	hUpdate 	-- handle to update buffer
 *	int 		yPos		-- line to modify
 *	int 		xPos		-- character position within line.
 *
 * RETURNS:
 *	TRUE on success
 *
 */
void updateChar(const HUPDATE hUpdate,
				const int yPos,
				const int xBegPos,
				const int xEndPos)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);
	struct stLineMode *pstLine;

	assert(hUpd != (HHUPDATE)0);
	assert(xBegPos <= xEndPos);

	// First check to see if we are in line mode.  If not, check if this
	// line is included in the UPD_SCROLL parameters of hUpd already.
	// The check is made to avoid the overhead of a function call if the
	// line is already set in the UPD_SCROLL parameters.

	if (hUpd->bUpdateType == UPD_SCROLL)
		{
		struct stScrlMode *pstScrl = &hUpd->stScrl;

		if (pstScrl->auchLines[yPos + pstScrl->iFirstLine] == 0)
			updateLine((HUPDATE)hUpd, yPos, yPos);

		return;
		}

	// Ok, it is just a line.  Update the UPD_LINE parameters.
	// Check to see that we have not jumped to a different line however.
	// If we have, we are no longer in UPD_LINE mode.

	pstLine = &hUpd->stLine;		 // for speed...

	if (yPos != pstLine->iLine)
		{
		// Check to see if sLine == -1.  This means the update buffer was
		// just flushed and reset and that this is the first change to come
		// in since that time.	Otherwise, we have more than one line in
		// our update region and have to jump to UPD_SCROLL mode.

		if (pstLine->iLine != -1)
			{
			updateLine((HUPDATE)hUpd, pstLine->iLine, pstLine->iLine);
			updateLine((HUPDATE)hUpd, yPos, yPos);
			}

		else
			{
			pstLine->iLine = yPos;
			pstLine->xBeg = xBegPos;
			pstLine->xEnd = xEndPos;

			DbgOutStr("Char -> iLine=%d, xBeg=%d, xEnd=%d\r\n",
			   yPos, xBegPos, xEndPos, 0, 0);
			}

		return;
		}

	// Ok, we've eliminated any conflicts.  Go ahead and update.

	if (xBegPos < pstLine->xBeg)
		pstLine->xBeg = xBegPos;

	if (xEndPos > pstLine->xEnd)
		pstLine->xEnd = xEndPos;

	DbgOutStr("Char -> iLine=%d, xBeg=%d, xEnd=%d\r\n",
	   yPos, xBegPos, xEndPos, 0, 0);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateCursorPos
 *
 * DESCRIPTION:
 *	Sets the row and col values of the host cursor in the given update handle
 *
 * ARGUMENTS:
 *	HUPDATE 	hUpdate - need I say!
 *	int 		iRow	- host cursor row
 *	int 		iCol	- host cursor col
 *
 * RETURNS:
 *	nothing
 *
 */
void updateCursorPos(const HUPDATE hUpdate,
					 const int iRow,
					 const int iCol)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);

	hUpd->iRow = iRow;
	hUpd->iCol = iCol;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateSetReallocFlag
 *
 * DESCRIPTION:
 *	Sets the realloc flag in the update records.  This flag can only be
 *	cleared by a client update request.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int updateSetReallocFlag(const HUPDATE hUpdate, const BOOL fState)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);

	hUpd->fRealloc = fState ? TRUE : FALSE;
	NotifyClient(hUpd->hSession, EVENT_TERM_UPDATE, 0);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	updateSetScrlMax
 *
 * DESCRIPTION:
 *	Sets the upper limit on the number of lines that can be scrolled
 *	before the update records stop accepting input.
 *
 * ARGUMENTS:
 *	hUpdate 	- external update handle
 *	iScrlmax	- max limit
 *
 * RETURNS:
 *	0
 *
 */
int updateSetScrlMax(const HUPDATE hUpdate, const int iScrlMax)
	{
	const HHUPDATE hUpd = VerifyUpdateHdl(hUpdate);

	hUpd->iScrlMax = iScrlMax;
	return 0;
	}
