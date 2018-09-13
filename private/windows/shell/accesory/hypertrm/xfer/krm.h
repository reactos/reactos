/* File: C:\WACKER\xfer\krm.h (Created: 28-Jan-1994)
 * created from HAWIN source file
 * krm.h  --  Exported definitions for KERMIT file transfer protocol routines.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

/* display rows for receiving */
#define KR_DR_RCV_FILE	 1
#define KR_DR_VIR_SCAN	 1
#define KR_DR_STORING	 2
#define KR_DR_COMPRESS	 3
#define KR_DR_FILE_SIZE  3
#define KR_DR_PACKET	 4
#define KR_DR_RETRIES	 4
#define KR_DR_TOTAL_RET  4
#define KR_DR_FILES_RCVD 4
#define KR_DR_LAST_ERR	 5
#define KR_DR_AMT_RCVD	 5
#define KR_DR_VUF		 8
#define KR_DR_BOTM_LINE  11

/* krm_snd display row values */
#define KS_DR_SND_FILE	 1
#define KS_DR_COMPRESS	 2
#define KS_DR_FILE_SIZE  2
#define KS_DR_PACKET	 3
#define KS_DR_RETRIES	 3
#define KS_DR_TOTAL_RET  3
#define KS_DR_FILES_SENT 3
#define KS_DR_LAST_ERR	 4
#define KS_DR_AMT_SENT	 4
#define KS_DR_VUF		 7
#define KS_DR_VUT		 11
#define KS_DR_BOTM_LINE  14


/* user settable options */

// extern int   k_useattr; 			/* send 'normalized' file names ? */
// extern int	 k_maxl;				/* maximum packet length we'll take */
// extern int	 k_timeout; 			/* time they should wait for us */
// extern uchar k_chkt;				/* check type we want to use */
// extern int	 k_retries; 			/* no. of retries */
// extern uchar k_markchar;			/* first char of each packet */
// extern uchar k_eol; 				/* end of line character for packets */
// extern int   k_npad;				/* no. of pad chars. to send us */
// extern uchar k_padc;				/* pad char. we want */



extern int krm_rcv(HSESSION hS, int attended, int single_file);
extern int krm_snd(HSESSION hS, int attended, int nfiles, long nbytes);

/* from KCALC.ASM */
extern unsigned kcalc_crc(unsigned crc, unsigned char *data, int cnt);

/********************* end of krm.h *************************/
