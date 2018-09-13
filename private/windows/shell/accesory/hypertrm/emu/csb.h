/*	File: D:\WACKER\emu\csb.h (Created: 27-Dec-1993)
 *
 *	Copyright 1989 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:27p $
 */


/* Display rows for receive screen */

#define CR_DR_RCV_FILE	1
#define CR_DR_VIR_SCAN	1
#define CR_DR_STORING	2
#define CR_DR_ERR_CHK	3
#define CR_DR_PACKET	4
#define CR_DR_RETRIES	4
#define CR_DR_TOTAL_RET 4
#define CR_DR_LAST_ERR	5
#define CR_DR_AMT_RCVD	5
#define CR_DR_BOTM_LINE 7

/* Display rows for send screen */
#define CS_DR_SND_FILE	1
#define CS_DR_ERR_CHK	2
#define CS_DR_PACKET	3
#define CS_DR_RETRIES	3
#define CS_DR_TOTAL_RET 3
#define CS_DR_LAST_ERR	4
#define CS_DR_AMT_RCVD	4
#define CS_DR_VUF		7
#define CS_DR_BOTM_LINE 10

extern USHORT csb_rcv(BOOL attended, BOOL single_file);
extern USHORT csb_snd(BOOL attended, unsigned nfiles, long nbytes);

/* for export to emulator */
extern VOID   CsbENQ(VOID);
extern VOID   CsbAdvanceSetup(VOID);
extern VOID   CsbInterrogate(VOID);
