/* File: C:\WACKER\xfer\krm_snd.c (Created: 28-Jan-1994)
 * created from HAWIN source code
 * krm_snd.c  --  Routines for handling file transmission using KERMIT
 *				file transfer protocol.
 *
 *	Copyright 1989,1990,1991,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
// #define	DEBUGSTR	1

#include <windows.h>
#pragma hdrstop

#include <time.h>
#include <stdlib.h>
#include <sys\types.h>
#include <sys\utime.h>

#include <term\res.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\load_res.h>
#include <tdll\xfer_msc.h>
#include <tdll\globals.h>
#include <tdll\file_io.h>
#include <tdll\session.h>
#include <tdll\tchar.h>

#if !defined(BYTE)
#define	BYTE	unsigned char
#endif

#include "cmprs.h"
#include "itime.h"
#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "krm.h"
#include "krm.hh"

// unsigned total_retries;
// metachar (NEAR *p_kgetc)(void);
// long kbytes_sent = 0L;
// KPCKT FAR *  this_kpckt;
// KPCKT FAR *  next_kpckt;

/* local funtion prototypes */
void build_attributes(ST_KRM *kc,
					unsigned char *bufr,
					long size,
					unsigned long ul_time);

int ksend_init(ST_KRM *kc);
int ksend_break(ST_KRM *kc);
int ksend_file(ST_KRM *kc, long fsize);

int wldindexx(const char *string,
				const char FAR *substr,
				char wildcard,
				int ic);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krm_snd
 *
 * DESCRIPTION:
 *	send a file or files using kermit protocol
 *
 *	This routine also handles logging of kermit sending operations to the
 *	optional log file.
 *
 * ARGUMENTS:
 *	attended -- TRUE if transfer is run manually, FALSE if run under automation
 *				such as in host or hyperpilot
 *	nfile	 -- number of files to be sent
 *	nbytes	 -- total number of bytes to be sent in all files
 *
 * RETURNS:
 *	Returns TRUE if all files are sent successfully or if all cancellations
 *	were user-requested, 'graceful' ones. Returns FALSE if an error occurs
 *	at either end of transfer or if user forces an immediate exit.
 */
int krm_snd(HSESSION hS, int attended, int nfiles, long nbytes)
	{
	ST_KRM *kc;
	int result;
	long sndsize;
	unsigned long filetime;
	long ttime;

	kc = malloc(sizeof(ST_KRM));
	if (kc == NULL)
		{
		xferMsgClose(hS);
		return TSC_NO_MEM;
		}
	memset(kc, 0, sizeof(ST_KRM));

	kc->hSession = hS;
	kc->hCom = sessQueryComHdl(hS);

	kc->kbytes_sent = 0L;

	kc->this_kpckt = NULL;
	kc->next_kpckt = NULL;

	if (kc != NULL)
		kc->this_kpckt = malloc(sizeof(KPCKT));
	if (kc->this_kpckt != NULL)
		kc->next_kpckt = malloc(sizeof(KPCKT));
	if (kc->next_kpckt == NULL)
		{
		if (kc->this_kpckt != NULL)
			{
			free(kc->this_kpckt);
			kc->this_kpckt = NULL;
			}
		if (kc != NULL)
			{
			free(kc);
			kc = NULL;
			}
		xferMsgClose(hS);
		return TSC_NO_MEM;
		}

	krmGetParameters(kc);

	kc->KrmProgress = ks_progress;

	kc->total_retries = 0;

	xferMsgFilecnt(kc->hSession, nfiles);
	xferMsgTotalsize(kc->hSession, nbytes);

	kc->nbytes = nbytes;
	kc->file_cnt = nfiles;
	kc->files_done = 0;
	kc->its_maxl = 80;
	kc->its_timeout = 15;
	kc->its_npad = 0;
	kc->its_padc = '\0';
	kc->its_eol = kc->k_eol;
	kc->its_chkt = 1;
	kc->its_qctl = K_QCTL;
	kc->its_qbin = '\0';
	kc->its_rept = '\0';
	kc->its_capat = FALSE;

	kc->ksequence = 0;
	kc->packetnum = 1;
	kc->abort_code = KA_OK;
	kc->xfertime = -1L;

	if (!ksend_init(kc))
		{
		int kret;

		kret = kresult_code[kc->abort_code];

		free(kc->this_kpckt);
		kc->this_kpckt = NULL;
		free(kc->next_kpckt);
		kc->next_kpckt = NULL;
		free(kc);
		kc = NULL;

		xferMsgClose(hS);
		return(kret);
		}

	/* don't show init errors once transfer has started */
	kc->total_dsp = kc->total_thru = 0L;  /* new transfer starting */

	while(xfer_nextfile(kc->hSession, kc->our_fname))
		{
		// xfer_idle(kc->hSession, XFER_IDLE_IO);

		if (kc->abort_code == KA_LABORT1)	/* TODO: figure this out */
			kc->abort_code = KA_LABORTALL;

		if (kc->abort_code >= KA_LABORTALL)
			break;

		kc->abort_code = KA_OK;

		result = xfer_opensendfile(kc->hSession,
								&kc->fhdl,
								kc->our_fname,
								&sndsize,
								kc->their_fname,
								&filetime);
		if (result != 0)
			{
			kc->abort_code = KA_CANT_OPEN;
			break;
			}

		if (kc->its_capat)
			build_attributes(kc, kc->next_kpckt->pdata, sndsize, filetime);

		ksend_file(kc, sndsize);

		++kc->files_done;
		if (kc->fhdl)
			{
			fio_close(kc->fhdl);
			kc->fhdl = NULL;
			}

		/* log transfer status here based on kc->abort_code */

		xfer_log_xfer(kc->hSession,
					TRUE,
					kc->our_fname,
					NULL,
					kresult_code[kc->abort_code]);
		}

	if (kc->abort_code < KA_IMMEDIATE)
		ksend_break(kc);
	ks_progress(kc, TRANSFER_DONE);
	ttime = (interval(kc->xfertime) / 10L);

	result = kresult_code[kc->abort_code];

	xferMsgClose(kc->hSession);

	free(kc->this_kpckt);
	kc->this_kpckt = NULL;
	free(kc->next_kpckt);
	kc->next_kpckt = NULL;
	free(kc);
	kc = NULL;

	xferMsgClose(hS);
	return(result);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * build_attributes
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void build_attributes(ST_KRM *kc,
					unsigned char *bufr,
					long size,
					unsigned long ul_time)
	{
	char str[15];
	int sl;
	struct tm *pt;

	/* add file size in K */
	wsprintf((LPSTR)str,
			(LPSTR)"%d",
			(int)(FULL_HUNKS(size, 1024)));

	wsprintf((LPSTR)bufr,
			(LPSTR)"!%c%s",
			tochar(sl = (int)StrCharGetByteCount(str)),
			(LPSTR)str);

	bufr += (sl + 2);

	/* add file size in bytes */
	wsprintf((LPSTR)str,
			(LPSTR)"%ld",
			(ULONG)size);

	wsprintf((LPSTR)bufr,
			(LPSTR)"1%c%s",
			tochar(sl = (int)StrCharGetByteCount(str)),
			(LPSTR)str);

	bufr += (sl + 2);

	/* add file date and time */
	ul_time += itimeGetBasetime();			/* Adjust to C7 and later */
	pt = localtime((time_t*)&ul_time);
	assert(pt);

	if (pt)
		{
		/*
		 * Dimwitted thing sometimes returns 0
		 */
		wsprintf((LPSTR)bufr,
				(LPSTR)"#%c%04d%02d%02d %02d:%02d:%02d",
				tochar(17),
				pt->tm_year + 1900,
				pt->tm_mon + 1,
				pt->tm_mday,
				pt->tm_hour,
				pt->tm_min,
				pt->tm_sec);

		bufr += 19;
		}

	/* system of origin */
	StrCharCat(bufr, ".\"U8");
	bufr += 4;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ksend_init
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int ksend_init(ST_KRM *kc)
	{
	unsigned plen;				/* length of outgoing packet data */
	char rpacket[MAXPCKT];		/* space to receive response packet */
	int rlen, rseq; 			/* length and sequence of response packet */
	int tries = 0;

	/* set init parameters as sender */
	plen = (unsigned)buildparams(kc, TRUE, kc->this_kpckt->pdata);
	xferMsgPacketnumber(kc->hSession, kc->packetnum);

	while (tries < kc->k_retries)
		{
		// xfer_idle(kc->hSession, XFER_IDLE_IO);

		xferMsgPacketErrcnt(kc->hSession, tries);

		ksend_packet(kc, 'S', plen, kc->ksequence, kc->this_kpckt);

		switch (krec_packet(kc, &rlen, &rseq, rpacket))
			{
		case 'Y':
			if (rseq == kc->ksequence)
				{
				kc->xfertime = startinterval();
				getparams(kc, TRUE, rpacket);
				kc->ksequence = (kc->ksequence + 1) % 64;
				++kc->packetnum;
				return(TRUE);
				}

			/* fall through */

		case 'N':
			xferMsgLasterror(kc->hSession, KE_NAK);
			++tries;
			break;

		case 'T':
			xferMsgLasterror(kc->hSession, KE_TIMEOUT);
			++tries;
			break;

		case BAD_PACKET:
			if (xfer_user_interrupt(kc->hSession))
				{
				kc->abort_code = KA_IMMEDIATE;
				return FALSE;
				}

			if (xfer_carrier_lost(kc->hSession))
				{
				kc->abort_code = KA_LOST_CARRIER;
				return FALSE;
				}

			xferMsgLasterror(kc->hSession, KE_BAD_PACKET);
			++tries;
			break;

		case 'E':
			/* received error packet, abort transfer */
			xferMsgLasterror(kc->hSession, KE_RMTERR);
			strncpy(kc->xtra_err, rpacket, (unsigned)65);
			kc->abort_code = KA_RMTERR;
			return(FALSE);
			/*lint -unreachable*/
			break;

		default:
			/* unexpected packet type */
			kc->abort_code = KA_BAD_FORMAT;
			return(FALSE);
			/*lint -unreachable*/
			break;
			}
		}
	/* error count has been exceeded */
	kc->abort_code = KA_ERRLIMIT;
	return(FALSE);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ksend_break
 *
 * DESCRIPTION:
 *	Send 'B' packet to indicate end of transaction
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *
 */
int ksend_break(ST_KRM *kc)
	{
	char rpacket[MAXPCKT];		/* space to receive response packet */
	int rlen, rseq; 			/* length and sequence of response packet */
	int tries = 0;

	while (tries < kc->k_retries)
		{
		// xfer_idle(kc->hSession, XFER_IDLE_IO);

		ksend_packet(kc, 'B', 0, kc->ksequence, kc->this_kpckt);
		switch (krec_packet(kc, &rlen, &rseq, rpacket))
			{
		case 'Y':
			if (rseq == kc->ksequence)
				{
				kc->ksequence = (kc->ksequence + 1) % 64;
				++kc->packetnum;
				return(TRUE);
				}

			/* fall through */

		case 'N':
		case 'T':
		case BAD_PACKET:
			if (xfer_user_interrupt(kc->hSession))
				{
				kc->abort_code = KA_IMMEDIATE;
				return FALSE;
				}

			if (xfer_carrier_lost(kc->hSession))
				{
				kc->abort_code = KA_LOST_CARRIER;
				return FALSE;
				}

			++tries;
			break;

		case 'E':
			/* received error packet, abort transfer */
			StrCharCopy(kc->xtra_err, rpacket);
			kc->abort_code = KA_RMTERR;
			return(FALSE);
			/*lint -unreachable*/
			break;

		default:
			/* unexpected packet type */
			kc->abort_code = KA_BAD_FORMAT;
			return(FALSE);
			/*lint -unreachable*/
			break;
			}
		}
	/* error count has been exceeded */
	kc->abort_code = KA_ERRLIMIT;
	return(FALSE);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ksend_file
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int ksend_file(ST_KRM *kc, long fsize)
	{
	int 		tries = 0;
	int			file_sent = FALSE;
	int			packet_sent = FALSE;
	int 		kbd_abort = KA_OK;
	char		rtype;
	int 		rlen, rseq;
	char		rpacket[MAXPCKT];
	int			sendattr;
	KPCKT 		*tmp;

	DbgOutStr("ksend_file %s\r\n", (LPSTR)kc->our_fname, 0,0,0,0);

	xferMsgNewfile(kc->hSession,
				kc->files_done + 1,
				kc->our_fname,
				kc->our_fname);

	xferMsgFilesize(kc->hSession, fsize);

	xferMsgPacketErrcnt(kc->hSession, 0);

	xferMsgPacketnumber(kc->hSession, 0);

	kc->p_kgetc = ks_getc;
	kc->kbytes_sent = 0;

	/* prepare file-header packet in this_kpckt */
	kc->this_kpckt->ptype = 'F';
	StrCharCopy(kc->this_kpckt->pdata, kc->their_fname);
	kc->this_kpckt->datalen = (int)StrCharGetByteCount(kc->this_kpckt->pdata);
	sendattr = kc->its_capat &&
		(size_t)StrCharGetByteCount(kc->next_kpckt->pdata) <= (size_t)(kc->its_maxl - 5);

	while (!file_sent && kc->abort_code == KA_OK)  /* for each packet */
		{
		// xfer_idle(kc->hSession, XFER_IDLE_IO);

		tries = 0;
		packet_sent = FALSE;
		while (!packet_sent && tries++ < kc->k_retries && kc->abort_code == KA_OK)
			{
			// xfer_idle(kc->hSession, XFER_IDLE_IO);

			if (kbd_abort != KA_OK && tries == 1)
				{
				kc->this_kpckt->ptype = 'Z';
				kc->this_kpckt->datalen = 1;
				StrCharCopy(kc->this_kpckt->pdata, "D");
				}

			DbgOutStr("Calling ksend_packet %d %c (0x%x)",
					tries, kc->this_kpckt->ptype, kc->this_kpckt->ptype, 0,0);

			ksend_packet(kc, kc->this_kpckt->ptype,
						(unsigned)kc->this_kpckt->datalen,
						kc->ksequence, kc->this_kpckt);
			if (tries == 1) 	/* first try for this packet */
				{
				xferMsgPacketnumber(kc->hSession, kc->packetnum);

				if (fsize > 0)
					ks_progress(kc, 0);

				/* get next packet ready while first is being sent */
				if (sendattr)	/* data alreay prepared in next_kpckt */
					{
					kc->next_kpckt->datalen = (int)StrCharGetByteCount(kc->next_kpckt->pdata);
					kc->next_kpckt->ptype = 'A';
					sendattr = FALSE;
					}
				else if ((kc->next_kpckt->datalen =
						kload_packet(kc, kc->next_kpckt->pdata)) == ERROR)
					{
					kc->next_kpckt->ptype = 'E';
					kc->next_kpckt->datalen = (int)StrCharGetByteCount(kc->xtra_err);
					StrCharCopy(kc->next_kpckt->pdata, kc->xtra_err);
					}
				else
					kc->next_kpckt->ptype = (char)(kc->next_kpckt->datalen ? 'D':'Z');

				DbgOutStr(" next packet %c (0x%x)\r\n",
						kc->next_kpckt->ptype, kc->next_kpckt->ptype, 0,0,0);

				} /* end of if (tries == 1) */
			else
				{
				xferMsgPacketErrcnt(kc->hSession, tries - 1);
				xferMsgErrorcnt(kc->hSession, ++kc->total_retries);

				DbgOutStr(" retry\r\n", 0,0,0,0,0);

				}


			rtype = (char)krec_packet(kc, &rlen, &rseq, rpacket);
			if (rtype == 'N' && (--rseq < 0 ? 63 : rseq) == kc->ksequence)
				rtype = 'Y';

			DbgOutStr("called krec_packet %c (0x%x)\r\n", rtype, rtype, 0,0,0);

			switch(rtype)
				{
			case 'Y':
				if (rseq == kc->ksequence)
					{
					packet_sent = TRUE;
					kc->ksequence = (kc->ksequence + 1) % 64;
					++kc->packetnum;
					if (kc->this_kpckt->ptype == 'A')/* response to attr pckt */
						{
						/* If receiver responded to an attribute packet with
						 *	an 'N' in the data field, do not transfer the file.
						 */

						if (rlen > 0 && *rpacket == 'N')
							kbd_abort = KA_RABORT1;
						}
					if (kc->this_kpckt->ptype == 'Z')/* have we sent last one?*/
						{
						file_sent = TRUE;
						kc->abort_code = kbd_abort;
						kbd_abort = KA_OK;
						}
					if (rlen == 1)
						{
						if (*rpacket == 'X')
							kbd_abort = KA_RABORT1;
						else if (*rpacket == 'Z')
							kbd_abort = KA_RABORTALL;
						}
					tmp = kc->this_kpckt;
					kc->this_kpckt = kc->next_kpckt;
					kc->next_kpckt = tmp;
					}
				else
					xferMsgLasterror(kc->hSession, KE_SEQUENCE);
				break;

			case 'N':
				xferMsgLasterror(kc->hSession, KE_NAK);
				break;

			case BAD_PACKET:
				xferMsgLasterror(kc->hSession, KE_BAD_PACKET);
				break;

			case 'T':
				xferMsgLasterror(kc->hSession, KE_TIMEOUT);
				break;

			case 'E':
				xferMsgLasterror(kc->hSession, KE_RMTERR);
				StrCharCopy(kc->xtra_err, rpacket);
				kc->abort_code = KA_RMTERR;
				return(FALSE);
				/*lint -unreachable*/
				break;

			default:
				xferMsgLasterror(kc->hSession, KE_WRONG);
				kc->abort_code = KA_BAD_FORMAT;
				return(FALSE);
				/*lint -unreachable*/
				break;
				}

			if (xfer_user_interrupt(kc->hSession))
				{
				if (kbd_abort == KA_OK) 	/* first time */
					{
					kbd_abort = KA_LABORT1;
					}
				else
					kc->abort_code = KA_IMMEDIATE;
				}

			if (xfer_carrier_lost(kc->hSession))
				kc->abort_code = KA_LOST_CARRIER;
			} /* end while (!packet_sent && etc.) */

		xferMsgPacketErrcnt(kc->hSession, tries = 0);

		if (kc->abort_code == KA_OK && !packet_sent) /* error count exceeded */
			kc->abort_code = KA_ERRLIMIT;
		} /* end while (!file_sent etc.) */

	xferMsgPacketnumber(kc->hSession, kc->packetnum);

	ks_progress(kc, FILE_DONE);
	kc->total_dsp += fsize;
	kc->total_thru += kc->kbytes_sent;
	kc->kbytes_sent = 0;
	return(file_sent);
	} /* end ksend_file() */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ks_progress
 *
 * DESCRIPTION:
 *	Displays transfer progress indicators for Kermit Send
 *
 * ARGUMENTS:
 *	final -- TRUE if final display for a file.
 *
 * RETURNS:
 *	nothing
 */
void ks_progress(ST_KRM *kc, int status)
	{
	long ttime, stime;
	long bytes_sent;
	long cps;
	long krm_stime = -1;
	long krm_ttime = -1;
	long krm_cps = -1;
	long krm_file_so_far = -1;
	long krm_total_so_far = -1;

	if (kc->xfertime == -1L)
		return;
	ttime = interval(kc->xfertime);

	if ((stime = ttime / 10L) != kc->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		/* Display elapsed time */
		krm_stime = stime;

		/* Display amount transferred */
		bytes_sent = kc->total_dsp + kc->kbytes_sent;

		krm_file_so_far = kc->kbytes_sent;
		krm_total_so_far = bytes_sent;

		/* Display throughput and est. time to completion */
		if ((stime > 2 ||
				ttime > 0 && bittest(status, FILE_DONE | TRANSFER_DONE)) &&
				(cps = ((kc->total_thru + kc->kbytes_sent) * 10L) / ttime) > 0)
			{
			krm_cps = cps;

			if ((kc->nbytes > 0))
				{
				ttime = ((kc->nbytes - bytes_sent) / cps) +
						kc->file_cnt - kc->files_done;
				krm_ttime = ttime;
				}
			}
		kc->displayed_time = stime;
		}

	xferMsgProgress(kc->hSession,
					krm_stime,
					krm_ttime,
					krm_cps,
					krm_file_so_far,
					krm_total_so_far);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#define	toupper(x) ((x)-'a'+'A')

int wldindexx(const char *string,
			const char *substr,
			char wildcard,
			int ic)
	/* ic - ignore case */
	{
	short index, limit;
	const char *s;
	const char *ss;

	if (*substr == '\0')
		return(0);
	index = 0;
	limit = (short)StrCharGetByteCount(string) - (short)StrCharGetByteCount(substr);
	while (index <= limit)
		{
		s = &string[index];
		ss = substr;
		while (*ss == wildcard || *s == *ss || (ic && isascii(*s)
					&& isascii(*ss) && toupper(*s) == toupper(*ss)))
			{
			++s;
			if (*++ss == '\0')
				return(index);
			}
		++index;
		}
	return(-1);
	}

/********************* end of krm_snd.c ********************/
