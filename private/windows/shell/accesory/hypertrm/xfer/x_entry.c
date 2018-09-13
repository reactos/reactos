/*	File: D:\WACKER\xfer\x_entry.c (Created: 14-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#include <term\res.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\globals.h>
#include <tdll\session.h>
#include <tdll\xfer_msc.h>
#include <tdll\tchar.h>
#include <xfer\xfr_dsp.h>

#include "hpr.h"
#include "krm.h"
#include "mdmx.h"
#include "zmodem.h"

#include "xfer.h"
#include "xfer.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 * This module contains all of the entry points into this DLL.  It also
 * contains a detailed description (as much as necessary) about the service
 * that the entry point offers.
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	xfrGetProtocols
 *
 * DESCRIPTION:
 *	This function builds and returns a list of the available transfer
 *	protocols.  In this version, it just uses internal protocols.  Later
 *	versions may check for additional DLLs by name and query them for
 *	details.
 *
 *	The list that gets returned is a pointer to an array of structures of
 *	type XFR_PROTOCOL.  The end of the list is indicated by a 0 for the
 *	protocol and a 0 length name string.
 *
 * ARGUMENTS:
 *	hSession        -- the session handle
 *	ppList          -- pointer to the list pointer (for returning data)
 *
 * RETURNS:
 *	0 if everything is OK, otherwise an error code.
 *
 */

#define	NUM_PROTOCOLS	9

int WINAPI xfrGetProtocols(const HSESSION hSession,
							const XFR_PROTOCOL **ppList)
	{
	int nIdx;
	XFR_PROTOCOL *pS;

	/*
	 * For the time being, we only return a single protocol.
	 */

	pS = (XFR_PROTOCOL *)malloc(NUM_PROTOCOLS * sizeof(XFR_PROTOCOL));
	if (pS == (XFR_PROTOCOL *)0)
		return XFR_NO_MEMORY;

	nIdx = 0;

#if FALSE
	pS[nIdx].nProtocol = XF_HYPERP;
	StrCharCopy(pS[nIdx].acName, TEXT("HyperProtocol"));
	nIdx += 1;
#endif

	pS[nIdx].nProtocol = XF_XMODEM_1K;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_X_1,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));
	nIdx += 1;

	pS[nIdx].nProtocol = XF_XMODEM;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_X,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));
	nIdx += 1;

	pS[nIdx].nProtocol = XF_YMODEM;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_Y,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));
	nIdx += 1;

	pS[nIdx].nProtocol = XF_YMODEM_G;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_Y_G,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));
	nIdx += 1;

	pS[nIdx].nProtocol = XF_ZMODEM;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_Z,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));
	nIdx += 1;

#if defined(INCL_ZMODEM_CRASH_RECOVERY)
	pS[nIdx].nProtocol = XF_ZMODEM_CR;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_Z_CR,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));

	nIdx += 1;
#endif  // defined(INCL_ZMODEM_CRASH_RECOVERY)

	pS[nIdx].nProtocol = XF_KERMIT;
	LoadString(glblQueryDllHinst(),
				IDS_XD_PROTO_K,
				pS[nIdx].acName,
				sizeof(pS[nIdx].acName) / sizeof(TCHAR));
	nIdx += 1;

	pS[nIdx].nProtocol = 0;		/* Indicates end of list */
	pS[nIdx].acName[0] = '\0';

	*ppList = pS;				/* Return the list */

	return (0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	xfrGetParameters
 *
 * DESCRIPTION:
 *	This function pops up a dialog to get specific transfer protocol
 *	parameters.  You get a different dialog and different parameters for
 *	each protocol.
 *
 * ARGUMENTS:
 *	hSession       -- the session handle
 *	nProtocol      -- the protocol id, returned from xfrGetProtocols
 *	hwnd           -- the parent window handle
 *	ppData         -- pointer to the data pointer (for returning data)
 *
 * NOTES:
 *	The ppData parameter is set up so that if there is a previous block
 *	of data for this protocol, it can be passed in.  If there is no data
 *	for this protocol, a NULL pointer is passed in and one is allocated and
 *	returned.  The returned value should always be used instead of the passed
 *	in value because at some time in the future, a parameter block mey need
 *	to expand and realloc some memory.
 *
 * RETURNS:
 *	0 if everything is OK, otherwise an error code.
 *
 */
int WINAPI xfrGetParameters(const HSESSION hSession,
							const int nProtocol,
							const HWND hwnd,
							VOID **ppData)
	{
	int nRet = 0;
	VOID *pD = *ppData;

	if (pD == (VOID *)0)
		{
		/* Caller did not supply an old parameter block, create one */
		nRet = xfrInitializeParams(hSession, nProtocol, &pD);
		if (nRet != 0)
			return nRet;
		}

	nRet = xfrModifyParams(hSession,
							nProtocol,
							hwnd,
							pD);

	if (nRet != 0)
		{
		/* Clean up on an error */
		free(pD);
		pD = (VOID *)0;
		}

	*ppData = pD;

	return (nRet);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	xfrReceive
 *
 * DESCRIPTION:
 *	This function starts a receive operation for a specific protocol.
 *	Any more details are only related to the sepcific protocol.
 *
 * ARGUMENTS:
 *	hSession      -- the session handle
 *	pXferRec      -- pointer to the receive data block (build by RECEIVE dlg)
 *
 * RETURNS:
 *	0 if everything is OK, otherwise an error code.
 *
 */
int WINAPI xfrReceive(const HSESSION hSession,
						const XFR_RECEIVE *pXferRec)
	{
	int nRet;
	int nOldRecProtocol;
	XFR_PARAMS *pX;

	switch (pXferRec->nProtocol)
		{
#if FALSE
		case XF_HYPERP:
			return hpr_rcv(hSession,
							TRUE,
							FALSE);	/* TODO: Get single_file flag !!!! */
#endif
		case XF_ZMODEM:
		case XF_ZMODEM_CR:
			/* This MIGHT have been an autoload so ... */
			pX = (XFR_PARAMS *)0;
			xfrQueryParameters(sessQueryXferHdl(hSession), (VOID **)&pX);
			nOldRecProtocol = pX->nRecProtocol;
			pX->nRecProtocol = pXferRec->nProtocol;

			nRet =  zmdm_rcv(hSession,
							pXferRec->nProtocol,
							TRUE,
							FALSE);	/* TODO: Get single_file flag !!!! */

			pX->nRecProtocol = nOldRecProtocol;
			return nRet;

		case XF_XMODEM:
		case XF_XMODEM_1K:
			return mdmx_rcv(hSession,
							TRUE,
							pXferRec->nProtocol,
							TRUE);	/* TODO: Get single_file flag !!!! */
		case XF_YMODEM:
		case XF_YMODEM_G:
			return mdmx_rcv(hSession,
							TRUE,
							pXferRec->nProtocol,
							FALSE);	/* TODO: Get single_file flag !!!! */
		case XF_KERMIT:
			return krm_rcv(hSession,
							TRUE,
							FALSE);	/* TODO: Get single_file flag !!!! */
		case XF_CSB:
			break;
		default:
			break;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	xfrSend
 *
 * DESCRIPTION:
 *	This function starts a send operation for a specific protocol.
 *	Any more details are only related to the specific protocols.
 *
 * ARGUMENTS:
 *	hSession      -- the session handle
 *	pXferSend     -- pointer to the send data block (built by SEND dialog)
 *
 * RETURNS:
 *	0 if everything is OK, otherwise an error code.
 *
 */
int WINAPI xfrSend(const HSESSION hSession,
					const XFR_SEND *pXferSend)
	{

	switch (pXferSend->nProtocol)
		{
#if FALSE
		case XF_HYPERP:
			return hpr_snd(hSession,
							TRUE,			/* Attended ??? */
							pXferSend->nCount,
							pXferSend->lSize);
#endif
		case XF_ZMODEM:
        case XF_ZMODEM_CR:
			return zmdm_snd(hSession,
							pXferSend->nProtocol,
							TRUE,			/* Attended ??? */
							pXferSend->nCount,
							pXferSend->lSize);
		case XF_XMODEM:
		case XF_XMODEM_1K:
		case XF_YMODEM:
		case XF_YMODEM_G:
			return mdmx_snd(hSession,
							TRUE,			/* Attended ??? */
							pXferSend->nProtocol,
							pXferSend->nCount,
							pXferSend->lSize);
		case XF_KERMIT:
			return krm_snd(hSession,
							TRUE,			/* Attended ??? */
							pXferSend->nCount,
							pXferSend->lSize);
		case XF_CSB:
			break;
		default:
			break;
		}

	return (0);
	}
