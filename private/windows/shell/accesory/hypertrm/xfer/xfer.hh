/*	File: D:\WACKER\xfer\xfer.hh (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#define FULL_HUNKS(n, s) ((n) / (s))
#define PART_HUNKS(n, s) ((long)(n) == 0L ? 0L : ((long)(n) - 1L) / (long)(s) + 1L)

/* XMODEM and YMODEM specific functions and data structures */

struct stXandYmodemParams
	{								/* nSize MUST BE THE FIRST ITEM */
	int			nSize;				/* the size of this data block */

#define	XP_ECP_AUTOMATIC	1
#define	XP_ECP_CRC			2
#define	XP_ECP_CHECKSUM		3
	int			nErrCheckType;		/* XMODEM - what type of error check */

	int			nPacketWait;		/* 1 to 60 seconds */

	int			nByteWait;			/* 1 to 60 seconds */

	int			nNumRetries;		/* 1 - 25 retries */
	};

typedef	struct stXandYmodemParams XFR_XY_PARAMS;

extern int xfrInitializeXandYmodem(const HSESSION hSession,
								VOID **ppData);

extern BOOL CALLBACK XandYmodemParamsDlg(HWND hDlg,
									UINT wMsg,
									WPARAM wPar,
									LPARAM lPar);

extern int xfrModifyXmodem(const HSESSION hSession,
							const HWND hwnd,
							VOID *pData);

extern int xfrModifyYmodem(const HSESSION hSession,
							const HWND hwnd,
							VOID *pData);

/* ZMODEM specific functions and data structures */

struct stZmodemParams
	{								/* nSize MUST BE THE FIRST ITEM */
	int			nSize;				/* the size of this data block */

	int			nAutostartOK;		/* TRUE if we allow autostarts */

	int			nFileExists;		/* defines follow */
#define	ZP_FE_SENDER	1			/* Follow sender A/O options */
#define	ZP_FE_DLG		2			/* Follow dialog box options */

	int			nCrashRecRecv;		/* defines follow */
#define ZP_CRR_NEG		1			/* Negotiate */
#define ZP_CRR_NEVER	2			/* Never recover */
#define	ZP_CRR_ALWAYS	3			/* Always recover */

	int			nOverwriteOpt;		/* defines follow */
#define ZP_OO_NONE		1			/* None */
#define	ZP_OO_N_L		2			/* Newer or longer */
#define	ZP_OO_CRC		3			/* CRC differs */
#define ZP_OO_APPEND	4			/* Append to file */
#define ZP_OO_ALWAYS	5			/* Overwrite always */
#define ZP_OO_NEWER		6			/* Newer */
#define	ZP_OO_L_D		7			/* Length or date differ */
#define ZP_OO_NEVER		8			/* Never overwrite */

	int			nCrashRecSend;		/* defines follow */
#define ZP_CRS_NEG		1			/* Negotiate */
#define	ZP_CRS_ONCE		2			/* One time */
#define	ZP_CRS_ALWAYS	3			/* Always */

	int			nXferMthd;			/* defines follow */
#define ZP_XM_STREAM	1			/* Streaming mode */
#define	ZP_XM_WINDOW	2			/* Windowed mode */

	int			nWinSize;			/* Window size 1K to 32K */
									/* TODO: check and document the format */

	int			nBlkSize;			/* Block size 32 - 1024 bytes */
									/* TODO: check and document the format */

	int			nCrcType;			/* defines follow */
#define	ZP_CRC_16		1
#define	ZP_CRC_32		2

	int			nRetryWait;			/* integer between 5 and 60 seconds */

	int			nEolConvert;		/* EOL conversion TRUE or FALSE */

	int			nEscCtrlCodes;		/* escape control codes, T or F */
	};

typedef	struct stZmodemParams XFR_Z_PARAMS;

extern int xfrInitializeZmodem(const HSESSION hSession,
								int nProtocol,
								VOID **ppData);

extern BOOL CALLBACK ZmodemParamsDlg(HWND hDlg,
									UINT wMsg,
									WPARAM wPar,
									LPARAM lPar);

extern int xfrModifyZmodem(const HSESSION hSession,
							const HWND hwnd,
							VOID *pData);

/* HyperProtocol specific functions and data structures */

struct stHyperProtocolParams
	{								/* nSize MUST BE THE FIRST ITEM */
	int			nSize;				/* the size of this data block */

#define	HP_CT_CHECKSUM		1
#define	HP_CT_CRC			2
	int			nCheckType;			/* the check type */

	int			nBlockSize;			/* 128 - 16384 bytes */

	int			nResyncTimeout;		/* 3-60 seconds */
	};

typedef	struct stHyperProtocolParams XFR_HP_PARAMS;

extern int xfrInitializeHyperProtocol(const HSESSION hSession,
								VOID **ppData);

extern BOOL CALLBACK HyperProtocolParamsDlg(HWND hDlg,
									UINT wMsg,
									WPARAM wPar,
									LPARAM lPar);

extern int xfrModifyHyperProtocol(const HSESSION hSession,
							const HWND hwnd,
							VOID *pData);

/* Kermit specific functions and data structures */

struct stKermitParams
	{								/* nSize MUST BE THE FIRST ITEM */
	int			nSize;				/* the size of this data block */

	int			nBytesPerPacket;	/* you can guess */
	int			nSecondsWaitPacket;
	int			nErrorCheckSize;
	int			nRetryCount;
	int			nPacketStartChar;
	int			nPacketEndChar;
	int			nNumberPadChars;
	int			nPadChar;
	};

typedef struct stKermitParams XFR_KR_PARAMS;

extern int xfrInitializeKermit(const HSESSION hSession,
								VOID **ppData);

extern BOOL CALLBACK KermitParamsDlg(HWND hDlg,
									UINT wMsg,
									WPARAM wPar,
									LPARAM lPar);

extern int xfrModifyKermit(const HSESSION hSession,
							const HWND hwnd,
							VOID *pData);

/* Generic functions */

extern int xfrInitializeParams(const HSESSION hSession,
								const int nProtocol,
								VOID **ppData);

extern int xfrModifyParams(const HSESSION hSession,
							const int nProtocol,
							const HWND hwnd,
							VOID *pData);

