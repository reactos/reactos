/*	File: D:\WACKER\xfer\x_params.c (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\tdll.h>
#include <tdll\globals.h>
#include <tdll\mc.h>
#include "xfer.h"
#include "xfer.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrInitializeParams
 *
 * DESCRIPTION:
 *	This function is called to initialize a transfer parameter block.  It
 *	calls the specific function that is for the specific protocol.
 *
 * ARGUEMENTS:
 *	hSession     -- the session handle
 *	nProtocol    -- indicates which protocol
 *  ppData       -- a pointer to a pointer for the return of the block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int xfrInitializeParams(const HSESSION hSession,
						const int nProtocol,
						VOID **ppData)
	{
	int nRet;

	switch (nProtocol)
		{
		case XF_ZMODEM:
		case XF_ZMODEM_CR:
			nRet = xfrInitializeZmodem(hSession, nProtocol, ppData);
			break;

		case XF_XMODEM:
		case XF_XMODEM_1K:
		case XF_YMODEM:
		case XF_YMODEM_G:
			nRet = xfrInitializeXandYmodem(hSession, ppData);
			break;

#if FALSE
		case XF_HYPERP:
			nRet = xfrInitializeHyperProtocol(hSession, ppData);
			break;
#endif

		case XF_KERMIT:
			nRet = xfrInitializeKermit(hSession, ppData);
			break;

		case XF_CSB:

		default:
			nRet = XFR_BAD_PROTOCOL;
			break;
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrInitializeHyperProtocol
 *
 * DESCRIPTION:
 *	This function is called to initialize a HyperProtocol parameter block. It
 *	will allocate a block if necessary, initialize the block and return.
 *
 * ARGUEMENTS:
 *	hSession     -- the session handle
 *  ppData       -- a pointer to a pointer for the return of the block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
#if FALSE
int xfrInitializeHyperProtocol(const HSESSION hSession, VOID **ppData)
	{
	int nRet;
	XFR_HP_PARAMS *pH;

	nRet = 0;

	pH = (XFR_HP_PARAMS *)*ppData;	/* Get the current parameters */
	if (pH == NULL)
		{
		/* Allocate a new structure */
		pH = (XFR_HP_PARAMS *)malloc(sizeof(XFR_HP_PARAMS));
		if (pH == (XFR_HP_PARAMS *)0)
			nRet = XFR_NO_MEMORY;
		}

	if (nRet == 0)
		{
		/* Remember to set the size */
		pH->nSize           = sizeof(XFR_HP_PARAMS);

		pH->nCheckType      = HP_CT_CHECKSUM;
		pH->nBlockSize      = 2048;
		pH->nResyncTimeout  = 10;

		*ppData = (VOID FAR *)pH;
		}

	return nRet;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrInitializeZmodem
 *
 * DESCRIPTION:
 *	This function is called to initialize a ZMODEM parameter block.  It will
 *	allocate a block if necessary, initialize the block and return.
 *
 * ARGUEMENTS:
 *	hSession     -- the session handle
 *	nProtocol	 --	Zmodem or Zmodem with crash recovery
 *  ppData       -- a pointer to a pointer for the return of the block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int xfrInitializeZmodem(const HSESSION hSession,
						int nProtocol,
						VOID **ppData)
	{
	int nRet;
	XFR_Z_PARAMS *pZ;

	nRet = 0;

	pZ = (XFR_Z_PARAMS *)*ppData;	/* Get the current parameters */
	if (pZ == NULL)
		{
		/* Allocate a new structure */
		pZ = (XFR_Z_PARAMS *)malloc(sizeof(XFR_Z_PARAMS));
		if (pZ == (XFR_Z_PARAMS *)0)
			nRet = XFR_NO_MEMORY;
		}

	if (nRet == 0)
		{
		/* Remember to set the size */
		pZ->nSize = sizeof(XFR_Z_PARAMS);

		pZ->nAutostartOK  = TRUE;
		pZ->nFileExists   = ZP_FE_DLG;
		if (nProtocol == XF_ZMODEM_CR)
			pZ->nCrashRecRecv = ZP_CRR_ALWAYS;
		else
			pZ->nCrashRecRecv = ZP_CRR_NEVER;
		pZ->nOverwriteOpt = ZP_OO_NONE;
		if (nProtocol == XF_ZMODEM_CR)
			pZ->nCrashRecSend = ZP_CRS_ALWAYS;
		else
			pZ->nCrashRecSend = ZP_CRS_NEG;
		pZ->nXferMthd     = ZP_XM_STREAM;
		pZ->nWinSize      = 15;		/* Set in KB */
		pZ->nBlkSize      = 6;		/* A wierd shift value */
		pZ->nCrcType      = ZP_CRC_16;
		pZ->nRetryWait    = 20;
		pZ->nEolConvert   = FALSE;
		pZ->nEscCtrlCodes = FALSE;

		*ppData = (VOID FAR *)pZ;
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrInitializeXandYmodem
 *
 * DESCRIPTION:
 *	This function is called to initialize a XMODEM parameter block.  It will
 *	allocate a block if necessary, initialize the block and return.
 *
 * ARGUEMENTS:
 *	hSession     -- the session handle
 *  ppData       -- a pointer to a pointer for the return of the block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int xfrInitializeXandYmodem(const HSESSION hSession,
						VOID **ppData)
	{
	int nRet;
	XFR_XY_PARAMS *pX;

	nRet = 0;

	pX = (XFR_XY_PARAMS *)*ppData;	/* Get the current parameters */
	if (pX == NULL)
		{
		/* Allocate a new structure */
		pX = (XFR_XY_PARAMS *)malloc(sizeof(XFR_XY_PARAMS));
		if (pX == (XFR_XY_PARAMS *)0)
			nRet = XFR_NO_MEMORY;
		}

	if (nRet == 0)
		{
		/* Remember to set the size */
		pX->nSize = sizeof(XFR_XY_PARAMS);

		pX->nErrCheckType = XP_ECP_AUTOMATIC;
		pX->nPacketWait   = 20;
		pX->nByteWait	  = 5;
		pX->nNumRetries   = 10;

		*ppData = (VOID FAR *)pX;
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrModifyParams
 *
 * DESCRIPTION:
 *	This function is called to bring up a dialog box allowing the user to
 *	change any of the options for a specified protocol.
 *
 * ARGUEMENTS:
 *	hSession      -- a session handle
 *	nProtocol     -- speciofies the protocol
 *	hwnd          -- window to be parent window
 *	pData         -- pointer to the parameters data block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int xfrModifyParams(const HSESSION hSession,
					const int nProtocol,
					const HWND hwnd,
					VOID *pData)
	{
	int nRet;

	if (pData == (VOID *)0)
		return XFR_BAD_POINTER;

	switch (nProtocol)
		{
#if defined(UPPER_FEATURES)
		case XF_ZMODEM:
			nRet = xfrModifyZmodem(hSession, hwnd, pData);
			break;

		case XF_XMODEM:
		case XF_XMODEM_1K:
			nRet = xfrModifyXmodem(hSession, hwnd, pData);
			break;

		case XF_YMODEM:
		case XF_YMODEM_G:
			nRet = xfrModifyYmodem(hSession, hwnd, pData);
			break;

		case XF_HYPERP:
			nRet = xfrModifyHyperProtocol(hSession, hwnd, pData);
			break;

		case XF_KERMIT:
			nRet = xfrModifyKermit(hSession, hwnd, pData);
			break;

		case XF_CSB:

#endif
		default:
			nRet = XFR_BAD_PROTOCOL;
			break;
		}

	return nRet;
	}

#if defined(UPPER_FEATURES)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrModifyXandYmodem
 *
 * DESCRIPTION:
 *	This function brings up the dialog box that allows the user to change
 *	the XMODEM protocol parameters.
 *
 * ARGUEMENTS:
 *	hSession      -- the session handle
 *	hwnd          -- the window to be the parent
 *	pData         -- pointer to the parameter data block
 *
 * RETURNS:
 */
int xfrModifyXmodem(const HSESSION hSession,
					const HWND hwnd,
					VOID *pData)
	{
	int nRet = 0;
	XFR_XY_PARAMS *pX;

	pX = (XFR_XY_PARAMS *)pData;

	DoDialog(glblQueryDllHinst(),
			"XmodemParameters",
			hwnd,				/* parent window */
			XandYmodemParamsDlg,
			(LPARAM)pX);

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrModifyYmodem
 *
 * DESCRIPTION:
 *	This function brings up the dialog box that allows the user to change
 *	the YMODEM protocol parameters.
 *
 * ARGUEMENTS:
 *	hSession      -- the session handle
 *	hwnd          -- the window to be the parent
 *	pData         -- pointer to the parameter data block
 *
 * RETURNS:
 */
int xfrModifyYmodem(const HSESSION hSession,
					const HWND hwnd,
					VOID *pData)
	{
	int nRet = 0;
	XFR_XY_PARAMS *pY;

	pY = (XFR_XY_PARAMS *)pData;

	DoDialog(glblQueryDllHinst(),
			"YmodemParameters",
			hwnd,				/* parent window */
			XandYmodemParamsDlg,
			(LPARAM)pY);

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrModifyZmodem
 *
 * DESCRIPTION:
 *	This function brings up the dialog box that allows the user to change
 *	the ZMODEM protocol parameters.
 *
 * ARGUEMENTS:
 *	hSession      -- the session handle
 *	hwnd          -- the window to be the parent
 *	pData         -- pointer to the parameter data block
 *
 * RETURNS:
 */
int xfrModifyZmodem(const HSESSION hSession,
					const HWND hwnd,
					VOID *pData)
	{
	int nRet = 0;
	XFR_Z_PARAMS *pZ;

	pZ = (XFR_Z_PARAMS *)pData;

	DoDialog(glblQueryDllHinst(),
			"ZmodemParameters",
			hwnd,				/* parent window */
			ZmodemParamsDlg,
			(LPARAM)pZ);

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrModifyHyperProtocol
 *
 * DESCRIPTION:
 *	This function brings up the dialog box that allows the user to change
 *	the HyperProtocol parameters.
 *
 * ARGUEMENTS:
 *	hSession      -- the session handle
 *	hwnd          -- the window to be the parent
 *	pData         -- pointer to the parameter data block
 *
 * RETURNS:
 */
#if FALSE
int xfrModifyHyperProtocol(const HSESSION hSession,
							const HWND hwnd,
							VOID *pData)
	{
	int nRet = 0;
	XFR_HP_PARAMS *pH;

	pH = (XFR_HP_PARAMS *)pData;

	DoDialog(glblQueryDllHinst(),
			"HyperParameters",
			hwnd,				/* parent window */
			HyperProtocolParamsDlg,
			(LPARAM)pH);

	return nRet;
	}
#endif

#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrInitializeKermit
 *
 * DESCRIPTION:
 *	This function is called to initialize a HyperProtocol parameter block. It
 *	will allocate a block if necessary, initialize the block and return.
 *
 * ARGUEMENTS:
 *	hSession     -- the session handle
 *  ppData       -- a pointer to a pointer for the return of the block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int xfrInitializeKermit(const HSESSION hSession, VOID **ppData)
	{
	int nRet;
	XFR_KR_PARAMS *pK;

	nRet = 0;

	pK = (XFR_KR_PARAMS *)*ppData;	/* Get the current parameters */
	if (pK == NULL)
		{
		/* Allocate a new structure */
		pK = (XFR_KR_PARAMS *)malloc(sizeof(XFR_KR_PARAMS));
		if (pK == (XFR_KR_PARAMS *)0)
			nRet = XFR_NO_MEMORY;
		}

	if (nRet == 0)
		{
		/* Remember to set the size */
		pK->nSize              = sizeof(XFR_KR_PARAMS);
		pK->nBytesPerPacket    = 94;
		pK->nSecondsWaitPacket = 5;
		pK->nErrorCheckSize    = 1;
		pK->nRetryCount        = 5;
		pK->nPacketStartChar   = 1;
		pK->nPacketEndChar     = 13;
		pK->nNumberPadChars    = 0;
		pK->nPadChar           = 0;

		*ppData = (VOID FAR *)pK;
		}

	return nRet;
	}

#if defined(UPPER_FEATURES)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
int xfrModifyKermit(const HSESSION hSession,
					const HWND hwnd,
					VOID *pData)
	{
	int nRet = 0;
	XFR_KR_PARAMS *pKR;

	pKR = (XFR_KR_PARAMS *)pData;

	DoDialog(glblQueryDllHinst(),
			"KermitParameters",
			hwnd,				/* parent window */
			KermitParamsDlg,
			(LPARAM)pKR);

	return nRet;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
