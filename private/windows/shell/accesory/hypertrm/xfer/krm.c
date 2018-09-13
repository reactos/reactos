/* File: C:\WACKER\xfer\krm.c (Created: 28-Jan-1994)
 * created from HAWIN source file
 * krm.c  --  Functions common to both kermit send and kermit receive
 *		routines.
 *
 *	Copyright 1989,1990,1991,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:22p $
 */
#include <windows.h>
#pragma hdrstop

#include <time.h>
#include <stdlib.h>
#include <sys\types.h>
#include <sys\utime.h>

#include <term\res.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\com.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\load_res.h>
#include <tdll\xfer_msc.h>
#include <tdll\globals.h>
#include <tdll\file_io.h>

#if !defined(BYTE)
#define	BYTE	unsigned char
#endif

#include "cmprs.h"

#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "krm.h"
#include "krm.hh"

// int   krm_dbg;				 /* used for real-time debugging using dbg.c */

// int   k_useattr;			 /* send 'normalized' file names ? */

// int   k_maxl;				 /* maximum packet length we'll take */
// int   k_timeout;			 /* time they should wait for us */
// uchar k_chkt;				 /* check type we want to use */
// int   k_retries;			 /* no. of retries */
// uchar k_markchar;			 /* first char of each packet */
// uchar k_eol;				 /* end of line character for packets */
// int   k_npad;				 /* no. of pad chars. to send us */
// uchar k_padc;				 /* pad char. we want */

// struct s_krm_control FAR *kc;
// void (NEARF *KrmProgress)(HSESSION, bits);

// unsigned ke_msg[] =
	// {
	// TM_NULL,
	// TM_NO_RESP,
	// TM_GOT_RETRY,
	// TM_ERR_DATA,
	// TM_RMT_ERR,
	// TM_BAD_FMT,
	// TM_PCKT_REPT,
	// TM_BAD_SEQ,
	// TM_FAILED,
	// };

/* for mapping Kermit result codes to Transfer Status Codes */
int kresult_code[] =
	{
	TSC_OK, 			   /* KA_OK 		   0 */
	TSC_USER_CANNED,	   /* KA_LABORT1	   1 */
	TSC_RMT_CANNED, 	   /* KA_RABORT1	   2 */
	TSC_USER_CANNED,	   /* KA_LABORTALL	   3 */
	TSC_RMT_CANNED, 	   /* KA_RABORTALL	   4 */
	TSC_USER_CANNED,	   /* KA_IMMEDIATE	   5 */
	TSC_RMT_CANNED, 	   /* KA_RMTERR 	   6 */
	TSC_LOST_CARRIER,	   /* KA_LOST_CARRIER  7 */
	TSC_ERROR_LIMIT,	   /* KA_ERRLIMIT	   8 */
	TSC_OUT_OF_SEQ, 	   /* KA_OUT_OF_SEQ    9 */
	TSC_BAD_FORMAT, 	   /* KA_BAD_FORMAT   10 */
	TSC_TOO_MANY,		   /* KA_TOO_MANY	  11 */
	TSC_CANT_OPEN,		   /* KA_CANT_OPEN	  12 */
	TSC_DISK_FULL,		   /* KA_DISK_FULL	  13 */
	TSC_DISK_ERROR, 	   /* KA_DISK_ERROR   14 */
	TSC_OLDER_FILE, 	   /* KA_OLDER_FILE   15 */
	TSC_NO_FILETIME,	   /* KA_NO_FILETIME  16 */
	TSC_WONT_CANCEL,	   /* KA_WONT_CANCEL  17 */
	TSC_VIRUS_DETECT,	   /* KA_VIRUS_DETECT 18 */
	TSC_REFUSE			   /* KA_USER_REFUSED 19 */
	};


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	krmGetParameters
 *
 * DESCRIPTION:
 *	This function is called to initialize all of the user settable values
 *	that get passed in from the parameters dialog box.
 *
 * ARGUMENTS:
 *	kc -- pointer to the Kermit data block
 *
 * RETURNS:
 *	Nothing.
 *
 */
void krmGetParameters(ST_KRM *kc)
	{
	XFR_PARAMS *pX;
	XFR_KR_PARAMS *pK;

	pX = (XFR_PARAMS *)0;
	xfrQueryParameters(sessQueryXferHdl(kc->hSession), (VOID **)&pX);
	assert(pX);
	if (pX != (XFR_PARAMS *)0)
		kc->k_useattr = pX->fUseDateTime;

	pK = (XFR_KR_PARAMS *)xfer_get_params(kc->hSession, XF_KERMIT);
	assert(pK);
	if (pK)
		{
		kc->k_maxl        = pK->nBytesPerPacket;
		kc->k_timeout     = pK->nSecondsWaitPacket;
		kc->k_chkt        = (BYTE)pK->nErrorCheckSize;
		kc->k_retries     = pK->nRetryCount;
		kc->k_markchar    = (BYTE)pK->nPacketStartChar;
		kc->k_eol         = (BYTE)pK->nPacketEndChar;
		kc->k_npad        = pK->nNumberPadChars;
		kc->k_padc        = (BYTE)pK->nPadChar;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ksend_packet
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void ksend_packet(ST_KRM *kc,
					unsigned char type,
					unsigned dlength,
					int seq,
					KPCKT FAR *pckt)
	{
	unsigned csum;
	unsigned crc;
	int i;
	char *cp;

	if (type == 'N' || type == 'E') /* wait for input to clear */
		ComRcvBufrClear(kc->hCom);

	/* send any padding necessary */
	for (i = kc->its_npad + 1; --i > 0; )
		ComSendCharNow(kc->hCom, kc->its_padc);

	/* when received, only packet data is valid, we fill in remainder */
	pckt->pmark = kc->k_markchar;
	pckt->plen = (int)tochar(dlength + 3);
	if (kc->its_chkt == K_CHK2)
		pckt->plen += 1;
	else if (kc->its_chkt == K_CHK3)
		pckt->plen += 2;
	pckt->pseq = (int)tochar(seq);
	pckt->ptype = type;

	/* now figure check bytes */
	if (kc->its_chkt == K_CHK3)
		{
		crc = kcalc_crc((unsigned)0,
						(unsigned char *)&pckt->plen,
						(int)dlength + 3);
		cp = pckt->pdata + dlength;
		*cp++ = (char)tochar((crc >> 12) & 0x0F);
		*cp++ = (char)tochar((crc >> 6) & 0x3F);
		*cp++ = (char)tochar(crc & 0x3F);
		}
	else
		{
		csum = 0;
		cp = (char *)&pckt->plen;
		for (i = dlength + 4; --i > 0; )
			csum += *cp++;
		/* cp is left pointing to first byte past data */
		if (kc->its_chkt == K_CHK2)
			{
			*cp++ = (char)tochar((csum >> 6) & 0x3F);
			*cp++ = (char)tochar(csum & 0x3F);
			}
		else
			*cp++ = (char)tochar((((csum & 0xC0) >> 6) + csum) & 0x3F);
		}
	*cp = kc->its_eol;

	/* send off all chars in buffer */
	// (VOID)mComSndBufr(comhdl, (char *)pckt,
	//		(uchar)(unchar(pckt->plen) + 3), /* include mark, len & eol */
	//		100, kc->flagkey_hdl);

	ComSndBufrSend(kc->hCom,
					(void *)pckt,
					(int)(unchar(pckt->plen) + 3),
					100);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krec_packet
 *
 * DESCRIPTION:
 *	Receive a kermit packet, check it for validity and return either the
 *	type of the received packet or an error code.
 *
 * ARGUMENTS:
 *	len
 *	seq
 *	data
 *
 * RETURNS:
 *
 */
int krec_packet(ST_KRM *kc,
				int *len,
				int *seq,
				unsigned char *data)
	{
	TCHAR c = 0;
	char *bp;
	char hdr[3];
	int done, got_hdr;
	int cnt, i;
	long j;
	unsigned chksum;
	unsigned rchk;
	long stime;
	long timelimit = kc->its_timeout * 10;

	   /* wait until any packet is transmitted */
	// (VOID)mComSndBufrWait(comhdl, 100, kc->flagkey_hdl);
	ComSndBufrWait(kc->hCom, 100);

	stime = startinterval();
	while (c != (int)kc->k_markchar)
		{
		if (j = xfer_user_interrupt(kc->hSession))
			{
			/* Yes, this is needed */
			// XferAbort(kc->hSession, (LPVOID)((LPSTR)j));
			xfer_user_abort(kc->hSession, j);
			return(BAD_PACKET);
			}

		if (xfer_carrier_lost(kc->hSession))
			{
			return(BAD_PACKET);
			}

		(*kc->KrmProgress)(kc, 0);

		// if ((c = mComRcvChar(comhdl)) == -1)
		if (mComRcvChar(kc->hCom, &c) == 0)
			{
			if ((long)interval(stime) > timelimit)
				{
				return('T');
				}
			xfer_idle(kc->hSession, XFER_IDLE_IO);
			}
		else if (c != (int)kc->k_markchar)
			{
			}
		else
			;	/* for lint */
		}
	getpacket:
	chksum = 0;
	done = got_hdr = FALSE;
	bp = &hdr[0];
	cnt = 3;
	while (!done)
		{
		for (i = cnt + 1; --i > 0; )
			{
			// while ((c = mComRcvChar(comhdl)) == -1)
			while (mComRcvChar(kc->hCom, &c) == 0)
				{
				if (j = xfer_user_interrupt(kc->hSession))
					{
					// XferAbort(kc->hSession, (LPVOID)((LPSTR)j));
					xfer_user_abort(kc->hSession, j);
					return(BAD_PACKET);
					}

				if (xfer_carrier_lost(kc->hSession))
					{
					return(BAD_PACKET);
					}

				(*kc->KrmProgress)(kc, 0);

				if ((long)interval(stime) > timelimit)
					{
					return('T');
					}
				xfer_idle(kc->hSession, XFER_IDLE_IO);
				}
			*bp = (char)c;
			if ((unsigned char)*bp == kc->k_markchar)
				{
				goto getpacket;
				}
			chksum += *bp++;
			}
		if (!got_hdr)
			{
			got_hdr = TRUE;
			*seq = unchar(hdr[1]);
			cnt = unchar(hdr[0]) - 2;	/* we've already got seq & len chars */
			if (cnt < 0 || cnt > 92)
				{
				return(BAD_PACKET);
				}
			bp = data;
			}
		else
			done = TRUE;
		}
	bp -= kc->its_chkt;   /* move pointer back to beginning of check field */
	switch(kc->its_chkt)
		{
	case 1:
		*len = cnt - 1;
		chksum -= bp[0];
		chksum = (((chksum & 0xC0) >> 6) + chksum) & 0x3F;
		rchk = (unsigned char)unchar(bp[0]);
		break;
	case 2:
		*len = cnt - 2;
		chksum = (chksum - (bp[0] + bp[1])) & 0x0FFF;
		rchk = ((unsigned char)unchar(bp[0]) << 6) + unchar(bp[1]);
		break;
	case 3:
		*len = cnt - 3;
		rchk = ((unsigned char)unchar(bp[0]) << 12) +
				((unsigned char)unchar(bp[1]) << 6) +
				unchar(bp[2]);
		chksum = kcalc_crc((unsigned)0, hdr, 3);
		if (*len > 0)
			chksum = kcalc_crc(chksum, data, *len);
		break;

	default:
		break;
		}
	*bp = '\0';
	if (*len < 0 || chksum != rchk)
		{
		return(BAD_PACKET);
		}
	else
		{
		return(hdr[2]);
		}
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * buildparams
 *
 * DESCRIPTION:
 *	Build a packet containing our initializing parameters. Return length of
 *	data in packet.
 *
 * ARGUMENTS:
 *	initiating	-- TRUE if we're initiating the transfer, FALSE if we're ACKing
 *					its initializing packet
 *	bufr		-- a place to put the results
 * RETURNS:
 *
 */
int buildparams(ST_KRM *kc, int initiating, unsigned char *bufr)
	{
	unsigned char *bp = bufr;

	if (initiating) 	/* just tell them what we want to do */
		{
		*bp++ = (unsigned char)tochar(kc->k_maxl);	   /* MAXL */
		*bp++ = (unsigned char)tochar(kc->k_timeout);  /* TIME */
		*bp++ = (unsigned char)tochar(kc->k_npad);	   /* NPAD */
		*bp++ = (unsigned char)ctl(kc->k_padc); 	   /* PADC */
		*bp++ = (unsigned char)tochar(kc->k_eol);	   /* EOL */
		*bp++ = K_QCTL; 				   /* QCTL */
		*bp++ = 'Y';					   /* QBIN */
		*bp++ = (unsigned char)(kc->k_chkt + '0');	   /* CHKT */
		*bp++ = K_REPT; 				   /* REPT */
		*bp++ = (unsigned char)tochar(CAPMASK_ATTR);/* CAPAS */
		}
	else				/* we're responding to them */
		{
		/* MAXL */
		*bp++ = (char)tochar(kc->k_maxl);
		/* TIME */
		*bp++ = (char)tochar((abs(kc->k_timeout - kc->its_timeout) <= 2) ?
				kc->k_timeout + 2 : kc->k_timeout);
		/* NPAD */
		*bp++ = (unsigned char)tochar(kc->k_npad);
		/* PADC */
		*bp++ = (unsigned char)ctl(kc->k_padc);
		/* EOL */
		*bp++ = (unsigned char)tochar(kc->k_eol);
		/* QCTL */
		*bp++ = K_QCTL;
		/* QBIN */
		if (kc->its_qbin == 'Y')
			kc->its_qbin = (char)(cnfgBitsPerChar(kc->hSession) == 8 ? 'N' : K_QBIN);

		if (IN_RANGE(kc->its_qbin, 33, 62) || IN_RANGE(kc->its_qbin, 96, 126))
			*bp++ = kc->its_qbin;
		else
			*bp++ = 'N', kc->its_qbin = '\0';
		/* CHKT */
		if (!IN_RANGE(kc->its_chkt, 1, 3))
			kc->its_chkt = 1;
		*bp++ = (unsigned char)(kc->its_chkt + '0');
		/* REPT */
		if (IN_RANGE(kc->its_rept, 33, 62) || IN_RANGE(kc->its_rept, 96, 126))
			*bp++ = kc->its_rept;
		else
			*bp++ = ' ', kc->its_rept = '\0';

		if (kc->its_capat)	  /* if sender can handle A packets, we can too */
			*bp++ = tochar(CAPMASK_ATTR);
		}

	return (int)(bp - bufr);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * getparams
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	nothing
 */
void getparams(ST_KRM *kc, int initiating, unsigned char *bufr)
	{
	if (!*bufr)
		return;
	kc->its_maxl = (*bufr == ' ' ? 80 : unchar(*bufr));
	/* if user has shortened packet length, he must know something about
		intervening transmission system that other end may not know about */
	if (kc->its_maxl > kc->k_maxl)
		kc->its_maxl = kc->k_maxl;
	if (!*++bufr)
		return;
	kc->its_timeout = (*bufr == ' ' ? 10 : unchar(*bufr));
	if (!initiating && abs(kc->k_timeout - kc->its_timeout) <= 2)
		kc->its_timeout = kc->k_timeout + 2;

	if (!*++bufr)
		return;
	kc->its_npad = unchar(*bufr);

	if (!*++bufr)
		return;
	kc->its_padc = (char)(*bufr == ' ' ? '\0' : ctl(*bufr));

	if (!*++bufr)
		return;
	kc->its_eol = (char)(*bufr == ' ' ? '\r' : unchar(*bufr));

	if (!*++bufr)
		return;
	kc->its_qctl = (char)(*bufr == ' ' ? K_QCTL : *bufr);

	if (!*++bufr)
		return;
	kc->its_qbin = *bufr;
	if (initiating &&
			!(IN_RANGE(kc->its_qbin, 33, 62) || IN_RANGE(kc->its_qbin, 96, 126)))
		kc->its_qbin = '\0';

	if (!*++bufr)
		return;
	kc->its_chkt = (unsigned char)(*bufr - '0');
	if (initiating && kc->its_chkt != kc->k_chkt)
		kc->its_chkt = 1;

	if (!*++bufr)
		return;
	kc->its_rept = *bufr;
	if (!(IN_RANGE(kc->its_rept, 33, 62) || IN_RANGE(kc->its_rept, 96, 126)))
		kc->its_rept = '\0';
	if (initiating && kc->its_rept != K_REPT)
		kc->its_rept = '\0';
	if (!*++bufr)
		return;
	if (unchar(*bufr) & CAPMASK_ATTR)
		kc->its_capat = TRUE;

	return;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * kcalc_crc
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
// #if FALSE		/* implemented in machine code for speed */
unsigned kcalc_crc(unsigned crc, unsigned char *data, int cnt)
	{
	unsigned int c;
	unsigned q;

	while (cnt--)
		{
		c = *data++;
		q = (crc ^ c) & 017;
		crc = (crc >> 4) ^ (q * 010201);
		q = (crc ^ (c >> 4)) & 017;
		crc = (crc >> 4) ^ (q * 010201);
		}
	return(crc);
	}
// #endif

/* end of krm.c */
