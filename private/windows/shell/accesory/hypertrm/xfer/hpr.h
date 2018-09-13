/* File: C:\WACKER\xfer\hpr.h (Created: 24-Jan-1994)
 * created from HAWIN source:
 * hpr.h  --  Exported definitions for HyperProtocol routines.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#if !defined(OFF)
#define	OFF	0
#endif
#if !defined(ON)
#define	ON	1
#endif

#define H_CHECKSUM		1
#define H_CRC			2

/* hpr_rcv display row values */
#define HR_DR_RCV_FILE	 1
#define HR_DR_VIR_SCAN	 1
#define HR_DR_STORING	 2
#define HR_DR_COMPRESS	 3
#define HR_DR_TOTAL_RET  3
#define HR_DR_FILE_SIZE  3
#define HR_DR_LAST_EVENT 4
#define HR_DR_FILES_RCVD 4
#define HR_DR_PSTATUS	 5
#define HR_DR_AMT_RCVD	 5
#define HR_DR_VUF		 8
#define HR_DR_VUT		 12
#define HR_DR_BOTMLINE	 15


/* hpr_snd display row values */
#define HS_DR_SND_FILE	 1
#define HS_DR_COMPRESS	 2
#define HS_DR_TOTAL_RET  2
#define HS_DR_FILE_SIZE  2
#define HS_DR_LAST_EVENT 3
#define HS_DR_FILES_SENT 3
#define HS_DR_PSTATUS	 4
#define HS_DR_AMT_SENT	 4
#define HS_DR_VUF		 7
#define HS_DR_VUT		 11
#define HS_DR_BOTMLINE	 14


/* these four variables are settable by user
 * an external routine can set these values to change the default
 * behavior of HyperProtocol
 */
// extern tbool h_useattr; 	   /* use received attributes when avail */
// extern tbool h_trycompress;    /* try to use compression when possible */
// extern tiny  h_chkt;		   /* 1 == checksum, 2 == CRC */
// extern tbool h_suspenddsk;	   /* TRUE if suspend for disk should be used */


/* the entry points for receiving and sending respectively */
extern int hpr_rcv(HSESSION hSession,
					  int attended,
					  int single_file);

extern int hpr_snd(HSESSION hSession,
					  int attended,
					  int hs_nfiles,
					  long hs_nbytes);


/********************* end of hpr.h ***************************/
