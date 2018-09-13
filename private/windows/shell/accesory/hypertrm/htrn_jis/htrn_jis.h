/*	File: D:\WACKER\htrn_jis\htrn_jis.h (Created: 24-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:06p $
 */

/*
 * Return codes
 */

#define	TRANS_OK			(0)

#define	TRANS_NO_SPACE		(-1)


/*
 * Function prototypes
 */

VOID *transCreateHandle(HSESSION hSession);

int transInitHandle(VOID *pHdl);

int transLoadHandle(VOID *pHdl);

int transSaveHandle(VOID *pHdl);

int transDestroyHandle(VOID *pHdl);

int transDoDialog(HWND hDLg, VOID *pHdl);

/*
 * These two functtions work about the same.  The caller stuffs character
 * after character into them and eventually gets some characters back.
 */
int transCharIn(VOID *pHdl,
				TCHAR cIn,
				int *nReady,
				int nSize,
				TCHAR *cReady);

int transCharOut(VOID *pHdl,
				TCHAR cOut,
				int *nReady,
				int nSize,
				TCHAR *cReady);

