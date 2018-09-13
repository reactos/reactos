/* xfr_dsp.c  -- Transfer display functions
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#define BYTE	char

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\xfdspdlg.h>
#include <tdll\xfer_msc.hh>
#include <tdll\tchar.h>
#include <tdll\assert.h>
#include <xfer\xfr_dsp.h>
#include <xfer\xfr_srvc.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgProgress
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession     -- the session handle
 *	stime        -- elapsed time (maybe)
 *	ttime        -- remaining time (maybe)
 *	cps          -- speed of transfer
 *	file_so_far  -- what it says
 *	total_so_far -- what it says
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgProgress(HSESSION hSession,
							long stime,
							long ttime,
							long cps,
							long file_so_far,
							long total_so_far)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		if (stime != -1)
			{
			pX->lElapsedTime = stime;
			pX->bElapsedTime = 1;
			}

		if (ttime != -1)
			{
			pX->lRemainingTime = ttime;
			pX->bRemainingTime = 1;
			}

		if (cps != -1)
			{
			pX->lThroughput = cps;
			pX->bThroughput = 1;
			}

		if (file_so_far != -1)
			{
			pX->lFileSoFar = file_so_far;
			pX->bFileSoFar = 1;
			}

		if (total_so_far != -1)
			{
			pX->lTotalSoFar = total_so_far;
			pX->bTotalSoFar = 1;
			}
		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgNewfile
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	filen     -- the file number
 *	theirname -- an ASCII copy of their name (TODO: convert to UNICODE)
 *	ourname   -- a copy of the filename as we used it
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgNewfile(HSESSION hSession,
						   int filen,
						   BYTE *theirname,
						   TCHAR *ourname)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);

	assert(pX);

	if (pX)
		{
		pX->wFileCnt = (WORD)filen;
		pX->bFileCnt = 1;

		if (theirname != NULL)
			{
			StrCharCopy(pX->acTheirName, theirname);
			pX->bTheirName = 1;
			}

		//assert(pX->bTheirName == 1);

		if (ourname != NULL)
			{
			StrCharCopy(pX->acOurName,   ourname);
			pX->bOurName = 1;
			}

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgFilesize
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	fsize    -- the size of the current file
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgFilesize(HSESSION hSession, long fsize)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->lFileSize = (LONG)fsize;
		pX->bFileSize = 1;

		pX->lFileSoFar = 0;
		pX->bFileSoFar = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgStatus
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	status   -- a protocol specific status code
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgStatus(HSESSION hSession, int status)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wStatus = (WORD)status;
		pX->bStatus = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgEvent
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	event    -- a protocol specific event code
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgEvent(HSESSION hSession, int event)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wEvent = (WORD)event;
		pX->bEvent = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgErrorcnt
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	cnt      -- the new number of errors to display
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgErrorcnt(HSESSION hSession, int cnt)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wErrorCnt = (WORD)cnt;
		pX->bErrorCnt = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgFilecnt
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	cnt      -- the total number of files in the transfer
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgFilecnt(HSESSION hSession, int cnt)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wTotalCnt = (WORD)cnt;
		pX->bTotalCnt = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			SendMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgTotalsize
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	bytes    -- the total size of all the files in the transfer operation
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgTotalsize(HSESSION hSession, long bytes)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->lTotalSize = (LONG)bytes;
		pX->bTotalSize = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgClose
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.  This function is actually called to indicate
 *	that a transfer is finished.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgClose(HSESSION hSession)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->nClose = TRUE;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgChecktype
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	ctype    -- indicates the current checksum type
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgChecktype(HSESSION hSession, int ctype)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wChecktype = (WORD)ctype;
		pX->bChecktype = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgPacketnumber
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	number   -- the current packet number
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgPacketnumber(HSESSION hSession, long number)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->lPacketNumber = number;
		pX->bPacketNumber = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrMsgLasterror
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	event    -- indicates the last error type for a specific protocol
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgLasterror(HSESSION hSession, int event)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wLastErrtype = (WORD)event;
		pX->bLastErrtype = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgPacketErrcnt
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	ecount   -- indicates the number of errors in the current packet
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgPacketErrcnt(HSESSION hSession, int ecount)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->wPcktErrCnt = (WORD)ecount;
		pX->bPcktErrCnt = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgProtocol
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	nProtocol -- which CSB protocol is being used
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgProtocol(HSESSION hSession, int nProtocol)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		switch (nProtocol)
			{
			default:
				pX->uProtocol = 0;
				break;
			case 0:
				pX->uProtocol = 1;
				break;
			case 1:
				pX->uProtocol = 2;
				break;
			}
		pX->bProtocol = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xferMsgMessage
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pszMsg   -- pointer to the CSP ASCII message string (TODO: convert)
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsgMessage(HSESSION hSession, BYTE *pszMsg)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		// _fmemset(pW->acMessage, 0, sizeof(pW->acMessage));
		// _fstrncpy(pW->acMessage, pszMsg, sizeof(pW->acMessage) - 1);
		StrCharCopy(pX->acMessage, pszMsg);
		pX->bMessage = 1;

		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}

#if FALSE

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to update various parts
 *	of the transfer display.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	Nothing.
 */
void xferMsg(HSESSION hSession)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		if (IsWindow(pX->hwndXfrDisplay))
			{
			PostMessage(pX->hwndXfrDisplay,
						WM_DLG_TO_DISPLAY,
						XFR_UPDATE_DLG, 0);
			}
		xfer_idle(hSession, XFER_IDLE_DISPLAY);
		}
	}
#endif
