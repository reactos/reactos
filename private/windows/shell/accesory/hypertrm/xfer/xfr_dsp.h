/* xfr_dsp.h -- a file of transfer display routines
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

/* Transfer display functions */
extern void xferMsgProgress(HSESSION hSession,
							long stime,
							long ttime,
							long cps,
							long file_so_far,
							long total_so_far);

extern void xferMsgNewfile(HSESSION hSession,
						   int filen,
						   BYTE *theirname,
						   TCHAR *ourname);

extern void xferMsgFilesize(HSESSION hSession, long fsize);

extern void xferMsgStatus(HSESSION hSession, int status);

extern void xferMsgEvent(HSESSION hSession, int event);

extern void xferMsgErrorcnt(HSESSION hSession, int cnt);

extern void xferMsgFilecnt(HSESSION hSession, int cnt);

extern void xferMsgTotalsize(HSESSION hSession, long bytes);

extern void xferMsgClose(HSESSION hSession);

extern void xferMsgVirusScan(HSESSION hSession, int cnt);

extern void xferMsgChecktype(HSESSION hSession, int event);

extern void xferMsgPacketnumber(HSESSION hSession, long number);

extern void xferMsgLasterror(HSESSION hSession, int event);

extern void xferMsgPacketErrcnt(HSESSION hSession, int event);

extern void xferMsgProtocol(HSESSION hSession, int nProtocol);

extern void xferMsgMessage(HSESSION hSession, BYTE *pszMsg);

