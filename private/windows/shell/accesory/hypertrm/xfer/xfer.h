/*	File: D:\WACKER\xfer\xfer.h (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:22p $
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *	This module contains all the function prototypes and associated data
 *	types that are needed to start transfers.
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/* Error codes from transfer routines */

#define	XFR_ERR_BASE			0x100

#define	XFR_NO_MEMORY			XFR_ERR_BASE+1
#define	XFR_BAD_PROTOCOL		XFR_ERR_BASE+2
#define	XFR_BAD_POINTER			XFR_ERR_BASE+3
#define	XFR_BAD_PARAMETER		XFR_ERR_BASE+4

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*
 * This structure contains the "generic" transfer parameters.
 * The values are set in the Transfer Send and Transfer Receive
 * dialogs and then passed to the transfer routines in the
 * session handle.
 */

struct xfer_parameters
	{
	int nSize;					/* Set to the size of this structure */

	/*
	 * This section is for receiving parameters
	 */
	int nRecProtocol;			/* default receiving protocol, see below */
	int fUseFilenames;			/* TRUE to use received filenames */
	int fUseDateTime;			/* TRUE to use received date and time */
	int fUseDirectory;			/* TRUE to use received directory */
	int fSavePartial;			/* TRUE to save partial files */

#define	XFR_RO_ALWAYS			0x1
#define	XFR_RO_NEVER			0x2
#define	XFR_RO_APPEND			0x3
#define	XFR_RO_NEWER			0x4
#define	XFR_RO_REN_DATE			0x5
#define	XFR_RO_REN_SEQ			0x6
	int nRecOverwrite;			/* default overwrite options */

	/*
	 * This section is for sending parameters
	 */
	int nSndProtocol;			/* default sending protocol, see below */
	int fChkSubdirs;			/* TRUE to check subdirs on search op */
	int fIncPaths;				/* TRUE to send paths to receiver */
	};

typedef struct xfer_parameters XFR_PARAMS;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/* Protocols supported */

#define	XF_HYPERP		1
#define	XF_ZMODEM		2
#define	XF_XMODEM		3
#define	XF_XMODEM_1K	4
#define	XF_YMODEM		5
#define	XF_YMODEM_G		6
#define	XF_KERMIT		7
#define	XF_CSB			8
#define	XF_ZMODEM_CR	9

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

struct xfer_protocol
	{
	int nProtocol;
        TCHAR acName[40];                       /* that should be big enough. JPN needs 32bytes at least*/
	};

typedef struct xfer_protocol XFR_PROTOCOL;

extern int WINAPI xfrGetProtocols(const HSESSION hSession,
								const XFR_PROTOCOL **ppList);


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

extern int WINAPI xfrGetParameters(const HSESSION hSession,
								const int nProtocol,
								const HWND hwnd,
								VOID **ppData); /* protocol parameters */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

struct xfer_receive
	{
	int nProtocol;				/* what protocol to use */
	XFR_PARAMS *pParams;		/* general transfer parameters */
	VOID *pProParams;			/* protocol specific parameters */
	LPTSTR pszDir;				/* prototype directory string */
	LPTSTR pszName;				/* prototype filename string */
	};

typedef struct xfer_receive XFR_RECEIVE;

extern int WINAPI xfrReceive(const HSESSION hSession,
								const XFR_RECEIVE *pXferRec);


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

struct xfer_send_list
	{
	long lSize;
	LPTSTR pszName;
	};

typedef struct xfer_send_list XFR_LIST;

struct xfer_send
	{
	int nProtocol;				/* what protocol to use */
	XFR_PARAMS *pParams;		/* general transfer parameters */
	VOID *pProParams;			/* protocol specific parameters */
	int nCount;					/* number of files to send */
	int nIndex;					/* current index into the list */
	long lSize;					/* total size of files in list */
	XFR_LIST *pList;			/* pointer to the list */
	};

typedef struct xfer_send XFR_SEND;

extern int WINAPI xfrSend(const HSESSION hSession,
							const XFR_SEND *pXferSend);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#define XF_DUPNAME_MASK 		0x00000F00L
#define XF_DUPNAME_APPEND		0x00000100L
#define XF_DUPNAME_OVERWRT		0x00000200L
#define XF_DUPNAME_REFUSE		0x00000300L
#define XF_DUPNAME_NEWER		0x00000400L
#define XF_DUPNAME_DATE 		0x00000500L
#define XF_DUPNAME_SEQ			0x00000600L

#define XF_CHECK_VIRUS			0x00001000L

#define XF_USE_FILENAME 		0x00002000L

#define XF_USE_DIRECTORY		0x00004000L

#define	XF_SAVE_PARTIAL			0x00008000L

#define	XF_USE_DATETIME			0x00010000L

#define XF_INCLUDE_SUBDIRS		0x00020000L

#define XF_INCLUDE_PATHS		0x00040000L

struct st_rcv_open
	{
	HANDLE bfHdl;
	TCHAR *pszSuggestedName;
	TCHAR *pszActualName;
	LONG  lInitialSize;
	// struct s_filetime FAR *pstFtCompare;
	LONG  lFileTime;
	// SSHDLMCH ssmchVscanHdl;
	VOID (FAR *pfnVscanOutput)(VOID FAR *hSession, USHORT usID);
	};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

int xfer_makepaths(HSESSION hSession, LPTSTR pszPath);
