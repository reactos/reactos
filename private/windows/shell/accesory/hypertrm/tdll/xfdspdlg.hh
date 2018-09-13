/* C:\WACKER\TDLL\XFDSPDLG.HH (Created: 10-Jan-1994)
 * Created from:
 * s_r_dlg.h - various stuff used in sends and recieves
 *
 *	Copyright 1994 by Hilgraeve, Inc -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

/*
 * This data is getting moved into the structure in xfer_msc.hh.  It is here
 * only as a transitional device.
 */

struct stReceiveDisplayRecord
	{
	/*
	 * First, we have the bit flags indicating which fields have changed
	 */
	UINT	bChecktype		: 1;
	UINT	bErrorCnt		: 1;
	UINT	bPcktErrCnt		: 1;
	UINT	bLastErrtype	: 1;
	UINT	bVirScan		: 1;
	UINT	bTotalSize		: 1;
	UINT	bTotalSoFar		: 1;
	UINT	bFileSize		: 1;
	UINT	bFileSoFar		: 1;
	UINT	bPacketNumber	: 1;
	UINT	bTotalCnt		: 1;
	UINT	bFileCnt		: 1;
	UINT	bEvent			: 1;
	UINT	bStatus			: 1;
	UINT	bElapsedTime	: 1;
	UINT	bRemainingTime	: 1;
	UINT	bThroughput		: 1;
	UINT	bProtocol		: 1;
	UINT	bMessage		: 1;
	UINT	bOurName		: 1;
	UINT	bTheirName		: 1;

	/*
	 * Then, we have the data fields themselves
	 */
	WORD	wChecktype; 				/* Added for XMODEM */
	WORD	wErrorCnt;
	WORD	wPcktErrCnt;				/* Added for XMODEM */
	WORD	wLastErrtype;				/* Added for XMODEM */
	WORD	wVirScan;
	LONG	lTotalSize;
	LONG	lTotalSoFar;
	LONG	lFileSize;
	LONG	lFileSoFar;
	LONG	lPacketNumber;				/* Added for XMODEM */
	WORD	wTotalCnt;
	WORD	wFileCnt;
	WORD	wEvent;
	WORD	wStatus;
	LONG	lElapsedTime;
	LONG	lRemainingTime;
	LONG	lThroughput;
	UINT	uProtocol;
	TCHAR	acMessage[80];
	TCHAR	acOurName[256];
	TCHAR	acTheirName[256];
	};

typedef struct stReceiveDisplayRecord	sRD;
typedef sRD FAR *psRD;


