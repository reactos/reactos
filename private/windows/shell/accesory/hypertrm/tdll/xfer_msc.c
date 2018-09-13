/*	File: D:\WACKER\tdll\xfer_msc.c (Created: 28-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 5/25/99 8:56a $
 */

#include <windows.h>
#pragma hdrstop

#include <term\res.h>
#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\mc.h>
#include <tdll\tdll.h>
#include <tdll\tchar.h>
#include <tdll\cloop.h>
#include <tdll\assert.h>
#include <tdll\globals.h>
#include <tdll\errorbox.h>
#include <tdll\file_msc.h>
#include <tdll\xfdspdlg.h>
#include <tdll\sf.h>

#include "sess_ids.h"

#include <xfer\xfer.h>
#include <xfer\xfer.hh>

#include "xfer_msc.h"
#include "xfer_msc.hh"

static void xfrInitDspStruct(HXFER hXfer);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *
 *                          X F E R _ M S C . C
 *
 *	This module contains various functions that are used in this dll in order
 *	to implement transfers.  While most of the code exists in the dialog procs
 *	for the Transfer Send and Transfer Receive dialogs, some is here to make
 *	things a little bit easier.
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#define	LIST_CHUNK	2

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CreateXferHdl
 *
 * DESCRIPTION:
 *	This function creates an "empty" Xfer handle.  It has stuff in it.  It
 *	just didn't come from the user.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	A blinded pointer to the Xfer handle.
 *
 */
HXFER CreateXferHdl(const HSESSION hSession)
	{
	int nRet;
	XD_TYPE *pX;

	pX = (XD_TYPE *)malloc(sizeof(XD_TYPE));
	assert(pX);
	if (pX)
		{
		memset(pX, 0, sizeof(XD_TYPE));

		nRet = InitializeXferHdl(hSession, (HXFER)pX);
		if (nRet)
			goto CXHexit;
		}

	pX->nSendListCount = 0;
	pX->acSendNames = NULL;

	return (HXFER)pX;
CXHexit:
	if (pX)
		{
		if (pX->xfer_params)
			{
			free(pX->xfer_params);
			pX->xfer_params = NULL;
			}
		if (pX->xfer_old_params)
			{
			free(pX->xfer_old_params);
			pX->xfer_old_params = NULL;
			}
		free(pX);
		pX = NULL;
		}
	return (HXFER)0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	InitializeXferHdl
 *
 * DESCRIPTION:
 *	This function initializes the Xfer handle to a known state.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	hXfer    -- the Xfer handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
INT InitializeXferHdl(const HSESSION hSession, HXFER hXfer)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)hXfer;

	assert(pX);

	if (pX)
		{
		memset(pX, 0, sizeof(XD_TYPE));

		pX->hSession = hSession;

		pX->nBps = 0;
		pX->nOldBps = pX->nBps;

		xfrQueryParameters(sessQueryXferHdl(hSession), &pX->xfer_params);
		if (pX->xfer_params == (SZ_TYPE *)0)
			return -1;

		pX->xfer_old_params = malloc(pX->xfer_params->nSize);
		if (pX->xfer_old_params == (SZ_TYPE *)0)
			{
			free(pX->xfer_params);
			pX->xfer_params = (SZ_TYPE *)0;
			return -1;
			}
		MemCopy(pX->xfer_old_params, pX->xfer_params, pX->xfer_params->nSize);
		}

	pX->nSendListCount = 0;
	pX->acSendNames = NULL;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	LoadXferHdl
 *
 * DESCRIPTION:
 *	This function loads data from the session file into the Xfer handle.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
INT LoadXferHdl(HXFER hXfer)
	{
	long lSize;
	XD_TYPE *pX;
	SZ_TYPE *pZ;

	pX = (XD_TYPE *)hXfer;

	if (pX)
		{

		InitializeXferHdl(pX->hSession, hXfer);

		/*
		 * Try and load up the general parameters first
		 */
		pZ = (SZ_TYPE *)0;
#if FALSE
		/* removed as per MRW request */
		sfdGetDataBlock(pX->hSession,
						SFID_XFER_PARAMS,
						(void **)&pZ);
#endif
		pZ = NULL;
		lSize = 0;
		sfGetSessionItem(sessQuerySysFileHdl(pX->hSession),
						SFID_XFER_PARAMS,
						&lSize,
						NULL);
		if (lSize > 0)
			{
			pZ = malloc(lSize);
			if (pZ)
				{
				sfGetSessionItem(sessQuerySysFileHdl(pX->hSession),
								SFID_XFER_PARAMS,
								&lSize,
								pZ);
				}
			}
		if (pZ)
			{
			if (pX->xfer_params)
				{
				free(pX->xfer_params);
				pX->xfer_params = NULL;
				}
			pX->xfer_params = pZ;
			if (pX->xfer_old_params)
				{
				free(pX->xfer_old_params);
				pX->xfer_old_params = NULL;
				}
			pX->xfer_old_params = malloc(lSize);
			MemCopy(pX->xfer_old_params, pX->xfer_params, lSize);
			}

		/*
		 * Try and get the Bps/Cps flag
		 */
		lSize = sizeof(pX->nBps);
		sfGetSessionItem(sessQuerySysFileHdl(pX->hSession),
						SFID_XFR_USE_BPS,
						&lSize,
						&pX->nBps);
		pX->nOldBps = pX->nBps;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SaveXferHdl
 *
 * DESCRIPTION:
 *	This function is called to save all the settings in the Xfer handle out
 *	to the session file.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
INT SaveXferHdl(HXFER hXfer)
	{
	int nSize;
	XD_TYPE *pX;
	SZ_TYPE *pZ;

	pX = (XD_TYPE *)hXfer;

	if (pX)
		{
		/*
		 * Save the generic transfer stuff
		 */
		pZ = pX->xfer_params;
		nSize = pZ->nSize;
#if FALSE
		/* removed as per MRW request */
		sfdPutDataBlock(pX->hSession,
						SFID_XFER_PARAMS,
						pX->xfer_params);
#endif
		if (memcmp(pX->xfer_old_params, pX->xfer_params, nSize) != 0)
			{
			sfPutSessionItem(sessQuerySysFileHdl(pX->hSession),
							SFID_XFER_PARAMS,
							nSize,
							pZ);
			}

		/*
		 * Save the Bps/Cps flag
		 */
		if (pX->nBps != pX->nOldBps)
			{
			sfPutSessionItem(sessQuerySysFileHdl(pX->hSession),
							SFID_XFR_USE_BPS,
							sizeof(pX->nBps),
							&pX->nBps);
			pX->nOldBps = pX->nBps;
			}

		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
INT DestroyXferHdl(HXFER hXfer)
	{
	int nLimit;
	int nIndex;
	XD_TYPE *pX;

	pX = (XD_TYPE *)hXfer;

	if (pX)
		{
		if (pX->xfer_params)
			{
			free(pX->xfer_params);
			pX->xfer_params = NULL;
			}

		if (pX->xfer_old_params)
			{
			free(pX->xfer_old_params);
			pX->xfer_old_params = NULL;
			}

		/*
		 * Loop through the protocol specific stuff
		 */
		nLimit = SFID_PROTO_PARAMS_END - SFID_PROTO_PARAMS;
		for (nIndex = 0; nIndex < nLimit; nIndex += 1)
			{
			if (pX->xfer_proto_params[nIndex])
				{
				free(pX->xfer_proto_params[nIndex]);
				pX->xfer_proto_params[nIndex] = NULL;
				}
			}
		/*
		 * Free up stuff as necessary
		 */
		for (nIndex = 1; nIndex < pX->nSendListCount; nIndex += 1)
			{
			free(pX->acSendNames[nIndex]);
			pX->acSendNames[nIndex] = NULL;
			}
		if (pX->acSendNames)
			{
			free(pX->acSendNames);
			pX->acSendNames = NULL;
			}

		pX->nSendListCount = 0;
		pX->acSendNames = NULL;

		free(pX);
		pX = NULL;

		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrSetDataPointer
 *
 * DESCRIPTION:
 *	This function is called from the transfer routines in the transfer dll
 *	to save the pointer to the parameter block that was passed to them.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	pData     -- the pointer to be saved
 *
 * RETURNS:
 *	Nothing.
 *
 */
VOID WINAPI xfrSetDataPointer(HXFER hXfer, VOID *pData)
	{
	XD_TYPE *pH;

	// pH = (XD_TYPE *)sessQueryXferHdl(hSession);
	pH = (XD_TYPE *)hXfer;
	assert(pH);
	if (pH)
		{
		pH->pXferStuff = pData;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrQueryDataPointer
 *
 * DESCRIPTION:
 *	This function is called from the transfer routines in the transfer dll to
 *	recover the saved data pointer to the parameter block that was passed to
 *	them.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	ppData    -- pointer to where to put the pointer
 *
 * RETURNS:
 *	Nothing.
 *
 */
VOID WINAPI xfrQueryDataPointer(HXFER hXfer, VOID **ppData)
	{
	XD_TYPE *pH;

	// pH = (XD_TYPE *)sessQueryXferHdl(hSession);
	pH = (XD_TYPE *)hXfer;
	assert(pH);
	if (pH)
		{
		*ppData = pH->pXferStuff;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrQueryParameters
 *
 * DESCRIPTION:
 *	This function returns a pointer to the default transfer parameters.  It
 *	gets the pointer to the block from the session handle and passed the
 *	the pointer to the caller.
 *
 * PARAMETERS:
 *	hSession     -- the session handle
 *	ppData       -- pointer to where the pointer should be saved
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code
 *
 */

INT WINAPI xfrQueryParameters(HXFER hXfer, VOID **ppData)
	{
	INT nRet = 0;
	XD_TYPE *pH;
	XFR_PARAMS *pX;

	pX = (XFR_PARAMS *)0;
	// pH = (XD_TYPE *)sessQueryXferHdl(hSession);
	pH = (XD_TYPE *)hXfer;
	if (pH)
		{
		pX = (XFR_PARAMS *)pH->xfer_params;
		}

	if (nRet == 0)
		{
		if (pX == (XFR_PARAMS *)0)
			{
			/* Build a default one */
			pX = (XFR_PARAMS *)malloc(sizeof(XFR_PARAMS));
			assert(pX);
			if (pX == (XFR_PARAMS *)0)
				nRet = XFR_NO_MEMORY;

			if (nRet == 0)
				{
				pX->nSize             = sizeof(XFR_PARAMS);

				/* Initialize the defaults */
#if defined(INCL_ZMODEM_CRASH_RECOVERY)
				pX->nRecProtocol      = XF_ZMODEM_CR;
				pX->fSavePartial      = TRUE;
#else   // defined(INCL_ZMODEM_CRASH_RECOVERY)
                pX->nRecProtocol      = XF_ZMODEM;
				pX->fSavePartial      = FALSE;
#endif  // defined(INCL_ZMODEM_CRASH_RECOVERY)
				pX->fUseFilenames     = TRUE;
				pX->fUseDateTime      = TRUE;
				pX->fUseDirectory     = FALSE;
				pX->nRecOverwrite     = XFR_RO_REN_SEQ;

				pX->nSndProtocol      = XF_ZMODEM_CR;
				pX->fChkSubdirs       = FALSE;
				pX->fIncPaths         = FALSE;
				}

			if (nRet == 0)
				{
				xfrSetParameters(hXfer, (VOID *)pX);
				}
			}
#if defined(INCL_ZMODEM_CRASH_RECOVERY)
        else
            {
            // This should always be true
            //
            pX->fSavePartial = TRUE;
            }
#endif  // defined(INCL_ZMODEM_CRASH_RECOVERY)
		}

	if (nRet == 0)
		*ppData = (VOID *)pX;

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrSetParameters
 *
 * DESCRIPTION:
 *	This function is called to change the default transfer parameters.  If
 *	the parameter block returned is different from the default block, the
 *	settings are copied.  If the block is the same, then they don't need to
 *	be.  Please note that the previous function is exported and can be called
 *	to get the parameters, but this function is not exported and cannot be
 *	accessed externally.
 *
 * PARAMETERS:
 *	hSession     -- the session handle
 *	pData        -- pointer to the new parameter block
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfrSetParameters(HXFER hXfer, VOID *pData)
	{
	XD_TYPE *pX;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;

	if (pX)
		{
		/* TODO: check that we really need to change it */
		if (pX->xfer_params)
			{
			if (pX->xfer_params != (SZ_TYPE *)pData)
				{
				free(pX->xfer_params);
				pX->xfer_params = NULL;
				}
			}

		pX->xfer_params = (SZ_TYPE *)pData;
		}
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
int WINAPI xfrQueryProtoParams(HXFER hXfer, int nId, VOID **ppData)
	{
	int nRet = 0;
	int nLimit;
	XD_TYPE *pX;

	nLimit = SFID_PROTO_PARAMS_END - SFID_PROTO_PARAMS;

	if ((nId < 0) || (nId > nLimit))
		nRet = XFR_BAD_PARAMETER;

	if (nRet == 0)
		{
		// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
		pX = (XD_TYPE *)hXfer;
		assert(pX);
		if (pX)
			{
			*ppData = (VOID *)pX->xfer_proto_params[nId];
			}
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
void WINAPI xfrSetProtoParams(HXFER hXfer, int nId, VOID *pData)
	{
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrSendAddToList
 *
 * DESCRIPTION:
 *	This function is called to add a file to the list of files that are being
 *	ququed up to send to whatever system we are connected to.
 *
 * PARAMETERS:
 *	hSession   -- the session handle
 *	pszFile    -- the file name, see note below
 *
 * NOTE:
 *	If the second parameter, "pszFile" is NULL, this function acts as a
 *	empty or clear list function.
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
int xfrSendAddToList(HXFER hXfer, LPCTSTR pszFile)
	{
	int nRet = 0;
	XD_TYPE *pX;
	LPTSTR pszName;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		if (pszFile == NULL)
			{
			int nIndex;

			/* Clear list */
			for (nIndex = 0; nIndex < pX->nSendListCount; nIndex += 1)
				{
				free(pX->acSendNames[nIndex]);
				pX->acSendNames[nIndex] = NULL;
				}
			if (pX->acSendNames)
				{
				free(pX->acSendNames);
				pX->acSendNames = NULL;
				}

			pX->nSendListCount = 0;
			pX->acSendNames = NULL;
			}
		else
			{
			/* Do we have enough space on the list ? */
			if (pX->nSendListCount == 0)
				{
				/* Allocate the initial chunk */
				pX->acSendNames = malloc(sizeof(TCHAR *) * LIST_CHUNK);
				if (pX->acSendNames == NULL)
					{
					nRet = XFR_NO_MEMORY;
					goto SATLexit;
					}
				}
			else if (((pX->nSendListCount + 1) % LIST_CHUNK) == 0)
				{
				/* Need a bigger chunk */
				pX->acSendNames = realloc(pX->acSendNames,
										sizeof(TCHAR *) * (pX->nSendListCount + 1 + LIST_CHUNK));
				if (pX->acSendNames == NULL)
					{
					nRet = XFR_NO_MEMORY;
					pX->nSendListCount = 0;
					goto SATLexit;
					}
				}
			/* Add item to list */
			pszName = malloc(StrCharGetByteCount(pszFile) + 1);
			if (pszName == NULL)
				{
				nRet = XFR_NO_MEMORY;
				goto SATLexit;
				}
			StrCharCopy(pszName, pszFile);
			pX->acSendNames[pX->nSendListCount] = pszName;
			pX->nSendListCount += 1;
			}
		}
SATLexit:
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrSendListSend
 *
 * DESCRIPTION:
 *	This function is called to send the files that were previously placed on
 *	the send list.  The list is cleared after the operation.
 *
 * PARAMETERS:
 *	hSession    -- the session handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
int xfrSendListSend(HXFER hXfer)
	{
	HSESSION hSession;
	int nRet = 0;
	int nIdx;
	long lSize;
	long lTmp;
	LPTSTR pszName;
	XD_TYPE *pX;
	XFR_SEND *pSend;
	HCLOOP hCL;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		hSession = pX->hSession;
		pSend = malloc(sizeof(XFR_SEND));
		assert(pSend);
		if (pSend == NULL)
			{
			nRet = XFR_NO_MEMORY;
			goto SLSexit;
			}
		memset(pSend, 0, sizeof(XFR_SEND));

		/*
		 * Fill in the single stuff
		 */
		pSend->pParams = (XFR_PARAMS *)pX->xfer_params;
		pSend->nProtocol = pSend->pParams->nSndProtocol;
		pSend->pProParams = (VOID *)pX->xfer_proto_params[pSend->nProtocol];
		pSend->nCount = pX->nSendListCount;
		pSend->nIndex = 0;

		/* TODO: initialize stuff like the templates and status/event bases */

		/*
		 * Do the file specific stuff
		 */
		pSend->pList = malloc(sizeof(XFR_LIST) * pSend->nCount);
		assert(pSend->pList);
		if (pSend->pList == NULL)
			{
			nRet = XFR_NO_MEMORY;
			goto SLSexit;
			}
		for (lSize = 0, nIdx = 0; nIdx < pSend->nCount; nIdx += 1)
			{
			pszName = pX->acSendNames[nIdx];
			lTmp = 0;
			GetFileSizeFromName(pszName, &lTmp);
			pSend->pList[nIdx].lSize = lTmp;
			lSize += lTmp;
			pSend->pList[nIdx].pszName = pszName;
			pX->acSendNames[nIdx] = NULL;
			}
		/* These no longer belong to this side */
		free(pX->acSendNames);
		pX->acSendNames = NULL;
		pX->nSendListCount = 0;

		pX->nDirection = XFER_SEND;

		pSend->lSize = lSize;

		xfrInitDspStruct(hXfer);

		switch (pSend->nProtocol)
			{
#if FALSE
			case XF_HYPERP:
				pX->nLgSingleTemplate = IDD_XFERHPRSNDSTANDARDSINGLE;
				pX->nLgMultiTemplate = IDD_XFERHPRSNDSTANDARDDOUBLE;
				pX->nStatusBase = IDS_TM_SS_ZERO;
				pX->nEventBase = IDS_TM_SE_ZERO;
				break;
#endif
			case XF_KERMIT:
				pX->nLgSingleTemplate = IDD_XFERKRMSNDSTANDARDSINGLE;
				pX->nLgMultiTemplate = IDD_XFERKRMSNDSTANDARDDOUBLE;
				pX->nStatusBase = pX->nEventBase = IDS_TM_K_ZERO;
				break;
			case XF_CSB:
			default:
				assert(FALSE);
			case XF_ZMODEM:
				pX->nLgSingleTemplate = IDD_XFERZMDMSNDSTANDARDSINGLE;
				pX->nLgMultiTemplate = IDD_XFERZMDMSNDSTANDARDDOUBLE;
				pX->nStatusBase = IDS_TM_SZ_ZERO;
				pX->nEventBase = IDS_TM_SZ_ZERO;
				break;
			case XF_XMODEM:
			case XF_XMODEM_1K:
				pX->nLgSingleTemplate = IDD_XFERXMDMSNDSTANDARDDISPLAY;
				pX->nLgMultiTemplate = pX->nLgSingleTemplate;
				pX->nStatusBase = pX->nEventBase = IDS_TM_RX_ZERO;
				break;
			case XF_YMODEM:
			case XF_YMODEM_G:
				pX->nLgSingleTemplate = IDD_XFERYMDMSNDSTANDARDSINGLE;
				pX->nLgMultiTemplate = IDD_XFERYMDMSNDSTANDARDDOUBLE;
				pX->nStatusBase = pX->nEventBase = IDS_TM_RX_ZERO;
				break;
			}

		pX->pXferStuff = (VOID *)pSend;

		pX->nExpanded = FALSE;

		pX->hwndXfrDisplay = DoModelessDialog(glblQueryDllHinst(),
										MAKEINTRESOURCE(pX->nLgSingleTemplate),
										sessQueryHwnd(hSession),
										XfrDisplayDlg,
										(LPARAM)hSession);

		/*
		 * Now get it going
		 */
		hCL = sessQueryCLoopHdl(hSession);
		if (hCL)
			{
			// DbgOutStr("Tell CLoop TRANSFER_READY\r\n", 0,0,0,0,0);

			CLoopControl(hCL, CLOOP_SET, CLOOP_TRANSFER_READY);
			}
		}

SLSexit:
	if (nRet != 0)
		{
		/* Clean up before we leave */
		if (pSend != NULL)
			{
			if (pSend->pList != NULL)
				{
				free(pSend->pList);
				pSend->pList = NULL;
				}
			free(pSend);
			pSend = NULL;
			}
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrRecvStart
 *
 * DESCRIPTION:
 *	This function is called when we think we have everything that we need
 *	in order to start a Transfer Receive operation.
 *
 * PARAMETERS:
 *	hSession    -- the session handle
 *	pszDir      -- a string with a directory in it
 *	pszName     -- a string with a file name in it (maybe)
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
int xfrRecvStart(HXFER hXfer, LPCTSTR pszDir, LPCTSTR pszName)
	{
	HSESSION hSession;
	int nRet = 0;
	XD_TYPE *pX;
	XFR_RECEIVE *pRec;
	HCLOOP hCL;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		hSession = pX->hSession;
		pRec = (XFR_RECEIVE *)malloc(sizeof(XFR_RECEIVE));
		assert(pRec);
		if (pRec == (XFR_RECEIVE *)0)
			{
			nRet = XFR_NO_MEMORY;
			goto RSexit;
			}
		memset(pRec, 0, sizeof(XFR_RECEIVE));

		pRec->pParams = (XFR_PARAMS *)pX->xfer_params;
		pRec->nProtocol = pRec->pParams->nRecProtocol;
		pRec->pProParams = (VOID *)pX->xfer_proto_params[pRec->nProtocol];
		pRec->pszDir = malloc(StrCharGetByteCount(pszDir) + 1);
		if (pRec->pszDir == (LPTSTR)0)
			{
			nRet = XFR_NO_MEMORY;
			goto RSexit;
			}
		StrCharCopy(pRec->pszDir, pszDir);
		pRec->pszName = malloc(StrCharGetByteCount(pszName) + 1);
		if (pRec->pszName == (LPTSTR)0)
			{
			nRet = XFR_NO_MEMORY;
			goto RSexit;
			}
		StrCharCopy(pRec->pszName, pszName);

		xfrInitDspStruct(hXfer);

		switch (pRec->nProtocol)
			{
#if FALSE
			case XF_HYPERP:
				pX->nLgSingleTemplate = IDD_XFERHPRRECSTANDARDSINGLE;
				pX->nLgMultiTemplate = IDD_XFERHPRRECSTANDARDDOUBLE;
				pX->nStatusBase = IDS_TM_RS_ZERO;
				pX->nEventBase = IDS_TM_RE_ZERO;
				break;
#endif
			case XF_KERMIT:
				pX->nLgSingleTemplate = IDD_XFERKRMRECSTANDARDDISPLAY;
				pX->nLgMultiTemplate = pX->nLgSingleTemplate;
				pX->nStatusBase = pX->nEventBase = IDS_TM_K_ZERO;
				break;
			case XF_CSB:
			default:
				assert(FALSE);
			case XF_ZMODEM:
            case XF_ZMODEM_CR:
				pX->nLgSingleTemplate = IDD_XFERZMDMRECSTANDARDSINGLE;
				pX->nLgMultiTemplate = IDD_XFERZMDMRECSTANDARDDOUBLE;
				pX->nStatusBase = IDS_TM_SZ_ZERO;
				pX->nEventBase = IDS_TM_SZ_ZERO;
				break;
			case XF_XMODEM:
			case XF_XMODEM_1K:
				pX->nLgSingleTemplate = IDD_XFERXMDMRECSTANDARDDISPLAY;
				pX->nLgMultiTemplate = pX->nLgSingleTemplate;
				pX->nStatusBase = pX->nEventBase = IDS_TM_RX_ZERO;
				break;
			case XF_YMODEM:
			case XF_YMODEM_G:
				pX->nLgSingleTemplate = IDD_XFERYMDMRECSTANDARDDISPLAY;
				pX->nLgMultiTemplate = pX->nLgSingleTemplate;
				pX->nStatusBase = pX->nEventBase = IDS_TM_RX_ZERO;
				break;
			}

		pX->nDirection = XFER_RECV;

		pX->pXferStuff = (VOID *)pRec;

		pX->nExpanded = FALSE;

		pX->hwndXfrDisplay = DoModelessDialog(glblQueryDllHinst(),
										MAKEINTRESOURCE(pX->nLgSingleTemplate),
										sessQueryHwnd(hSession),
										XfrDisplayDlg,
										(LPARAM)hSession);

		hCL = sessQueryCLoopHdl(hSession);
		if (hCL)
			{
			CLoopControl(hCL, CLOOP_SET, CLOOP_TRANSFER_READY);
			}
		}
RSexit:
	if (nRet != 0)
		{
		/*
		 * If we failed, clean up the mess.
		 */
		if (pRec != (XFR_RECEIVE *)0)
			{
			if (pRec->pszDir != (LPTSTR)0)
				{
				free(pRec->pszDir);
				pRec->pszDir = NULL;
				}
			if (pRec->pszName != (LPTSTR)0)
				{
				free(pRec->pszName);
				pRec->pszName = NULL;
				}
			free(pRec);
			pRec = NULL;
			}
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrGetEventBase
 *
 * DESCRIPTION:
 *	This function is called by the transfer display.  It is used to get the
 *	starting number (of the the resource strings) of a list of events that
 *	can be displayed for some of the transfer protocols.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	A resource ID that should be passed on to LoadString.
 *
 */
int xfrGetEventBase(HXFER hXfer)
	{
	XD_TYPE *pX;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		return pX->nEventBase;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrGetStatusBase
 *
 * DESCRIPTION:
 *	This function is called bye the transfer display.  It is used to get the
 *	starting number (of the resource strings) of a list of status messages that
 *	can be displayed for some of the transfer protocols.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	A resource ID that should be passed on to LoadString.
 *
 */
int xfrGetStatusBase(HXFER hXfer)
	{
	XD_TYPE *pX;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		return pX->nStatusBase;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrGetXferDspBps
 *
 * DESCRIPTION:
 *	This function is called to get the current value of the BPS/CPS flag.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	The current value of the BPS flag.
 *
 */
int xfrGetXferDspBps(HXFER hXfer)
	{
	XD_TYPE *pX;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		return pX->nBps;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrSetXferDspBps
 *
 * DESCRIPTION:
 *	This function is called to set the BPS/CPS flag in the tranfer handle.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	nBps     -- the new BPS flag
 *
 * RETURNS:
 *	The old value of the BPS flag.
 *
 */
int xfrSetXferDspBps(HXFER hXfer, int nBps)
	{

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrInitDspStruct
 *
 * DESCRIPTION:
 *	This function is called before a transfer is started to make sure that
 *	the display variables in the transfer structure are all set to a known
 *	initial value.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	Nothing.
 */
static void xfrInitDspStruct(HXFER hXfer)
	{
	XD_TYPE *pX;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);
	if (pX)
		{
		pX->nClose         = 0;
		pX->nCloseStatus   = 0;

		pX->bChecktype     = 0;
		pX->bErrorCnt      = 0;
		pX->bPcktErrCnt    = 0;
		pX->bLastErrtype   = 0;
		pX->bTotalSoFar    = 0;
		pX->bFileSize      = 0;
		pX->bFileSoFar     = 0;
		pX->bPacketNumber  = 0;
		pX->bTotalCnt      = 0;
		pX->bTotalSize     = 0;
		pX->bFileCnt       = 0;
		pX->bEvent         = 0;
		pX->bStatus        = 0;
		pX->bElapsedTime   = 0;
		pX->bRemainingTime = 0;
		pX->bThroughput    = 0;
		pX->bProtocol      = 0;
		pX->bMessage       = 0;
		pX->bOurName       = 0;
		pX->bTheirName     = 0;
		pX->wChecktype     = 0;
		pX->wErrorCnt      = 0;
		pX->wPcktErrCnt    = 0;
		pX->wLastErrtype   = 0;
		pX->lTotalSize     = 0;
		pX->lTotalSoFar    = 0;
		pX->lFileSize      = 0;
		pX->lFileSoFar     = 0;
		pX->lPacketNumber  = 0;
		pX->wTotalCnt      = 0;
		pX->wFileCnt       = 0;
		pX->wEvent         = 0;
		pX->wStatus        = 0;
		pX->lElapsedTime   = 0;
		pX->lRemainingTime = 0;
		pX->lThroughput    = 0;
		pX->uProtocol      = 0;

		TCHAR_Fill(pX->acMessage, TEXT('\0'),
					sizeof(pX->acMessage) / sizeof(TCHAR));
		TCHAR_Fill(pX->acOurName, TEXT('\0'),
					sizeof(pX->acOurName) / sizeof(TCHAR));
		TCHAR_Fill(pX->acTheirName, TEXT('\0'),
					sizeof(pX->acTheirName) / sizeof(TCHAR));
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrCleanUpReceive
 *
 * DESCRIPTION:
 *	This function is called from xfrDoTransfer after a transfer in order to
 *	clean up the stuff that was allocated in order to do the transfer.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *	Nothing.
 */
void xfrCleanUpReceive(HSESSION hSession)
	{
	XD_TYPE *pX;
	XFR_RECEIVE *pR;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	assert(pX);
	if (pX)
		{
		pR = (XFR_RECEIVE *)pX->pXferStuff;
		assert(pR);
		if (pR)
			{
			if (pR->pszDir != NULL)
				{
				free(pR->pszDir);
				pR->pszDir = NULL;
				}
			if (pR->pszName != NULL)
				{
				free(pR->pszName);
				pR->pszName = NULL;
				}
			free(pR);
			pR = NULL;
			}
		pX->pXferStuff = (void *)0;
		pX->nExpanded = FALSE;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrCleanUpSend
 *
 * DESCRIPTION:
 *	This function is called from xfrDoTransfer after a transfer in order to
 *	clean up the stuff that was allocated in order to do the transfer.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *	Nothing.
 */
void xfrCleanUpSend(HSESSION hSession)
	{
	XD_TYPE *pX;
	XFR_SEND *pS;

	/* TODO: finish this thing up */

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	assert(pX);
	if (pX)
		{
		pS = (XFR_SEND *)pX->pXferStuff;
		assert(pS);
		if (pS)
			{
			int n;

			if (pS->pList)
				{
				for (n = 0; n < pS->nCount; n += 1)
					{
					if (pS->pList[n].pszName)
						{
						free(pS->pList[n].pszName);
						pS->pList[n].pszName = NULL;
						}
					}
				free(pS->pList);
				pS->pList = NULL;
				}
			free(pS);
			pS = NULL;
			}
		pX->pXferStuff = (void *)0;
		pX->nExpanded = FALSE;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrDoTransfer
 *
 * DESCRIPTION:
 *	Yes, folks, this is the one you have been waiting for.  It runs in the
 *	CLOOP thread.  It calls the XFER DLL (if there is one).  It slices.  It
 *	dices.  It actually does the transfer.
 *
 * PARAMETERS:
 *	hSession -- the font of all knowledge
 *
 * RETURNS:
 *	Nothing.  What can be said after a transfer is done?
 *
 */
void xfrDoTransfer(HXFER hXfer)
	{
	XD_TYPE *pX;
	HSESSION hSession = (HSESSION)0;
	int nRet = 0;
	int nTitle;
	TCHAR acTitle[64];
	TCHAR acMessage[255];

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	if (pX)
		{
		hSession = pX->hSession;
		switch (pX->nDirection)
			{
			case XFER_SEND:
				nTitle = IDS_XD_SEND;
				nRet = xfrSend(hSession, (XFR_SEND *)pX->pXferStuff);
				xfrCleanUpSend(hSession);
				break;
			case XFER_RECV:
				nTitle = IDS_XD_RECEIVE;
				nRet = xfrReceive(hSession, (XFR_RECEIVE *)pX->pXferStuff);
				xfrCleanUpReceive(hSession);
				break;
			default:
				assert(FALSE);
				break;
			}
		}

	if (sessQuerySound(hSession))
		MessageBeep(MB_OK);

	switch (nRet)
		{
		case 0:
		case 14:
		case 15:
			break;
		default:
			LoadString(glblQueryDllHinst(),
					nTitle,
					acTitle,
					sizeof(acTitle) / sizeof(TCHAR));
			LoadString(glblQueryDllHinst(),
					nRet + IDS_TM_XFER_ZERO,
					acMessage,
					sizeof(acMessage) / sizeof(TCHAR));
			if (StrCharGetStrLength(acMessage) > 0)
				{
				TimedMessageBox(sessQueryHwnd(hSession),
								acMessage,
								acTitle,
								MB_OK | MB_ICONINFORMATION,
								sessQueryTimeout(hSession));
				}
			break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrSetPercentDone
 *
 * DESCRIPTION:
 *	This function is called to set the percent done value for a transfer.
 *	This value is only of real use when the program is shown as an icon.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	nPerCent -- the percent done (0 to 100)
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfrSetPercentDone(HXFER hXfer, int nPerCent)
	{
	XD_TYPE *pX;
	HWND hwnd;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;
	assert(pX);

	if (pX)
		{
		// DbgOutStr("Set percent %d", nPerCent, 0,0,0,0);
		pX->nPerCent = nPerCent;
		hwnd = sessQueryHwnd(pX->hSession);
		if (IsIconic(hwnd))
			{
			// DbgOutStr(" !!!", 0,0,0,0,0);
			InvalidateRect(hwnd, 0, TRUE);
			}
		// DbgOutStr("\r\n", 0,0,0,0,0);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrGetPercentDone
 *
 * DESCRIPTION:
 *	This function is called to get the stored percentage value for a transfer.
 *	This is usually done only to display the value as an icon.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	The percentage (0 to 100).
 *
 */
int  xfrGetPercentDone(HXFER hXfer)
	{
	XD_TYPE *pX;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;

	if (pX)
		{
		return pX->nPerCent;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrGetDisplayWindow
 *
 * DESCRIPTION:
 *	Returns the window handle of the transfer display window, if any.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	A window handle (HWND) or NULL.
 *
 */
HWND xfrGetDisplayWindow(HXFER hXfer)
	{
	HWND hRet;
	XD_TYPE *pX;

	hRet = (HWND)0;

	// pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	pX = (XD_TYPE *)hXfer;

	if (pX)
		{
		hRet = pX->hwndXfrDisplay;		/* handle of the display window */
		}

	return hRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrDoAutostart
 *
 * DESCRIPTION:
 *	This function is called whenever the session proc gets an XFER_REQ event.
 *	This usually indicates that somebody wants to do a ZMODEM receive.  In
 *	UPPER WACKER other protocols will be added, notably CSB and HyperP.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	lProtocol -- which protocol is requested
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfrDoAutostart(HXFER hXfer, long lProtocol)
	{
	XD_TYPE *pX;
	HSESSION hSession;

	pX = (XD_TYPE *)hXfer;
	assert(pX);
	hSession = pX->hSession;

	switch (lProtocol)
		{
		case XF_ZMODEM:
			{
			int nOldProtocol;
			LPCTSTR  pszDir;
			XFR_PARAMS *pP;
			XFR_Z_PARAMS *pZ;

			pP = (XFR_PARAMS *)0;
			xfrQueryParameters(hXfer, (VOID **)&pP);
			assert(pP);

#if	defined(INCL_ZMODEM_CRASH_RECOVERY)
			// For Zmodem autostarts, check to see if the receiver's
			// protocol is set for plain Zmodem. Otherwise use
            // crash recovery.
			//
            if (pP->nRecProtocol == XF_ZMODEM)
                {
                lProtocol = XF_ZMODEM;
                }
            else
                {
                lProtocol = XF_ZMODEM_CR;
                }
#endif	// defined(INCL_ZMODEM_CRASH_RECOVERY)

			pZ = (XFR_Z_PARAMS *)0;
			xfrQueryProtoParams(hXfer,
								(int)lProtocol,
								(void **)&pZ);
			if (pZ)
				{
				if (!pZ->nAutostartOK)
					break;					/* Not allowed ! */
				}

			nOldProtocol = pP->nRecProtocol;
			pP->nRecProtocol = (int)lProtocol;

			/* Try and start up the transfer */
			pszDir = filesQueryRecvDirectory(sessQueryFilesDirsHdl(hSession));

			xfrRecvStart(hXfer, pszDir, "");

			/* Restore what we changed up above */
			pP->nRecProtocol = nOldProtocol;
			}
			break;

		default:
			assert(FALSE);
			break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
