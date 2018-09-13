/*	File: D:\WACKER\tdll\xfer_msc.hh (Created: 30-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

/*
 * All the structures in this structure are defined in other modules.  They
 * all conform to the single rule that the first element of the structure is
 * an integer that contains the size of the structure.
 */

struct stSizeType
	{
	int nSize;
	};

typedef struct stSizeType SZ_TYPE;

struct stXferData
	{

	HSESSION hSession;

	/* This holds the generic transfer parameters */
	SZ_TYPE		*xfer_old_params;	/* for conditional save */
	SZ_TYPE		*xfer_params;		/* the parameters currently in use */

	/* TODO: add stuff for conditional save */

	/* This holds the protocol specific parameters */
	SZ_TYPE		*xfer_proto_params[16];

#define	XFER_SEND		1
#define	XFER_RECV		2
	int nDirection;

#define	XFER_ABORT		1
#define	XFER_SKIP		2
	int nUserCancel;				/* User canceled the transfer */

	int nCarrierLost;				/* Carrier has been lost */


	/* This holds the stuff for the send list */
	int nSendListCount;				/* How many files so far */
	TCHAR **acSendNames;			/* Pointer to list block */

	VOID *pXferStuff;				/* Set by the other side for storage */

	/*
	 * This block of stuff is used by the transfer displays.
	 */
	HWND hwndXfrDisplay;			/* handle of the display window */

	int nLgSingleTemplate;			/* template ID for single file transfer */
	int nLgMultiTemplate;			/* template ID for multiple file xfer */

#if FALSE
	/* Removed and switched to integers */
	LPCSTR pszLgSingleTemplate;		/* template for single file transfer */
	LPCSTR pszLgMultiTemplate;		/* template for multiple file transfer */
#if FALSE
	/* Size change is not supported in Lower Wacker */
	LPTSTR pszSmSingleTemplate;
	LPTSTR pszSmMultiTemplate;
#endif
#endif

	int nStatusBase;				/* start of status ID list */
	int nEventBase;					/* start of event ID list */

	int nOldBps;					/* Saved copy of the following */
	int nBps;						/* TRUE to display as BPS vs CPS */

	int nExpanded;					/* Set if we have already expanded */

	int nCancel;					/* the ever popular cancel option */
	int nSkip;						/* TRUE if we want to skip this file */

	int	nPerCent;					/* Percent done, if we know it */

	int nClose;						/* The transfer finished */
	int nCloseStatus;				/* The closing status */
	/*
	 * First, we have the bit flags indicating which fields have changed
	 */
	int		bChecktype		: 1;
	int		bErrorCnt		: 1;
	int		bPcktErrCnt		: 1;
	int		bLastErrtype	: 1;
	int		bTotalSize		: 1;
	int		bTotalSoFar		: 1;
	int		bFileSize		: 1;
	int		bFileSoFar		: 1;
	int		bPacketNumber	: 1;
	int		bTotalCnt		: 1;
	int		bFileCnt		: 1;
	int		bEvent			: 1;
	int		bStatus			: 1;
	int		bElapsedTime	: 1;
	int		bRemainingTime	: 1;
	int		bThroughput		: 1;
	int		bProtocol		: 1;
	int		bMessage		: 1;
	int		bOurName		: 1;
	int		bTheirName		: 1;

	/*
	 * Then, we have the data fields themselves
	 */
	int		wChecktype; 				/* Added for XMODEM */
	int		wErrorCnt;
	int		wPcktErrCnt;				/* Added for XMODEM */
	int		wLastErrtype;				/* Added for XMODEM */
	long	lTotalSize;
	long	lTotalSoFar;
	long	lFileSize;
	long	lFileSoFar;
	long	lPacketNumber;				/* Added for XMODEM */
	int		wTotalCnt;
	int		wFileCnt;
	int		wEvent;
	int		wStatus;
	long	lElapsedTime;
	long	lRemainingTime;
	long	lThroughput;
	int		uProtocol;
	TCHAR	acMessage[80];
	TCHAR	acOurName[256];
	TCHAR	acTheirName[256];
	/*
	 * End of the transfer display data !!
	 */

	};

typedef struct stXferData XD_TYPE;

