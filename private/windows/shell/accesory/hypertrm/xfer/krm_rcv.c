/* File: C:\WACKER\xfer\krm_rcv.c (Created: 28-Jan-1994)
 * created from HAWIN source file
 * krm_rcv.c  --  Routines for handling file transmission using KERMIT
 *				file transfer protocol.
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

// struct s_krm_rcv_control FAR *krc;
// metachar (NEAR *p_kputc)(metachar);
// long kbytes_received;

// #define	DO_DATE(hS)		((xfer_flags(hS)&XF_USE_DATETIME)!=0L)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krm_rcv
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int krm_rcv(HSESSION hS, int attended, int single_file)
	{
	ST_KRM *kc;
	unsigned total_retries;
	unsigned uabort_seq = 0;
	int 	 kr_state;
	int		 iret;

	kc = NULL;

	kc = malloc(sizeof(*kc));
	if (kc == NULL)
		{
		xferMsgClose(hS);
		return TSC_NO_MEM;
		}

	kc->hSession = hS;
	kc->hCom = sessQueryComHdl(hS);

	kc->kbytes_received = 0;

	krmGetParameters(kc);

	kc->KrmProgress = kr_progress;

	kc->fhdl = NULL;
	kc->total_thru = 0L;
	kc->its_maxl = 80;
	kc->its_timeout = 10;
	kc->its_npad = 0;
	kc->its_padc = '\0';
	kc->its_eol = '\r';
	kc->its_chkt = 1;
	kc->its_qctl = K_QCTL;
	kc->its_qbin = '\0';
	kc->its_rept = '\0';
	kc->its_capat = FALSE;
	kc->files_done = 0;

	kc->xfertime = -1L;
	kc->kr.files_received = kc->kr.files_aborted = 0;
	kr_state = KREC_INIT;
	kc->ksequence = 0;
	kc->packetnum = 1;
	kc->tries = total_retries = 0;
	kc->kr.lasterr = KE_NOERROR;
	kc->abort_code = KA_OK;
	kc->kr.uabort_code = '\0';
	kc->kr.data_packet_rcvd = FALSE;
	kc->kr.dsptries = 0;
	kc->kr.store_later = FALSE;

	/* normally, the sender initiates a transfer, but if we're the terminal
	 *	end of the transfer, we may be able to get the sender to start up
	 *	without waiting for a timeout by sending a NAK packet as soon as we
	 *	start up.
	 */
	ksend_packet(kc, 'N', 0, kc->ksequence, &kc->kr.resp_pckt);

	for ( ; ; )
		{
		xfer_idle(kc->hSession, XFER_IDLE_IO);

		switch(kr_state)
			{
		case KREC_INIT:
			kr_state = krec_init(kc);
			if (kr_state == KREC_FILE)
				{
				/* clear init errors */

				xferMsgErrorcnt(kc->hSession, total_retries = 0);
				xferMsgLasterror(kc->hSession, 0);
				}
			break;

		case KREC_FILE:
			kc->kr.data_packet_rcvd = FALSE;
			kc->kr.next_rtype = '\0';   /* init for krec_data routine */
			kr_state = krec_file(kc);
			if (!kc->tries && kr_state == KREC_DATA)
				{
				xferMsgNewfile(kc->hSession,
							   ++kc->files_done,
							   kc->their_fname,
							   kc->our_fname);
				}
			break;

		case KREC_DATA:
			kr_state = krec_data(kc);
			if ((kc->kr.uabort_code == 'Z') &&
				(uabort_seq > (unsigned)(kc->packetnum + 3)))
				kr_state = KREC_ABORT;
			break;

		case KREC_COMPLETE:
			{
			int kret;
			kret = kresult_code[kc->abort_code];
			free(kc);
			kc = NULL;
			xferMsgClose(hS);
			return(kret);
			}
			/*lint -unreachable*/
			break;

		case KREC_ABORT:
			{
			int kret;

			xferMsgLasterror(kc->hSession, kc->kr.lasterr);

			if (kc->fhdl != NULL)
				xfer_close_rcv_file(hS,
									kc->fhdl,
									kresult_code[kc->abort_code],
									kc->their_fname,
									kc->our_fname,
									FALSE,
									0L,
									0);

			kret = kresult_code[kc->abort_code];
			free(kc);
			kc = NULL;
			xferMsgClose(hS);
			return(kret);
			}
			/*lint -unreachable*/
			break;

		default:
			assert(FALSE);
			break;
			}

		xferMsgPacketErrcnt(kc->hSession,
							kc->kr.dsptries ? kc->kr.dsptries : kc->tries);

		if (kc->tries || kc->kr.dsptries)
		 	{
			xferMsgErrorcnt(kc->hSession, ++kc->total_retries);
			xferMsgLasterror(kc->hSession, kc->kr.lasterr);

			kc->kr.dsptries = 0;
			}
		else
			xferMsgPacketnumber(kc->hSession, kc->packetnum);

		/* check for keyboard abort */
		if (iret = xfer_user_interrupt(kc->hSession))
			{
			if (iret == XFER_ABORT)
				{
				if (kc->kr.uabort_code)	 /* not the first time */
					{
					kc->abort_code = KA_IMMEDIATE;
					kr_state = KREC_ABORT;
					}
				else				/* start user abort process */
					{
					if (single_file)
						kc->kr.uabort_code = 'Z';
					else
						kc->kr.uabort_code = 'X';

					/* force it */
					kc->kr.uabort_code = 'Z';

					uabort_seq = kc->packetnum;
					kc->abort_code = (kc->kr.uabort_code == 'X' ?
							KA_LABORT1 : KA_LABORTALL);
					}
				}
			else
				{
				if (kc->kr.uabort_code == 0)
					{
					kc->kr.uabort_code = 'X';
					uabort_seq = kc->packetnum;
					kc->abort_code = KA_LABORT1;
					}
				}
			}

		if (xfer_carrier_lost(kc->hSession))
			{
			kc->abort_code = KA_LOST_CARRIER;
			kr_state = KREC_ABORT;
			}
		}

	/*lint -unreachable*/
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krec_init
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int krec_init(ST_KRM *kc)
	{
	int rtype;
	int plen, rseq;
	unsigned slen;
	unsigned char packet[MAXPCKT];
	unsigned char tchkt;

	if (kc->tries++ > kc->k_retries)
		{
		kc->abort_code = KA_ERRLIMIT ;
		return(KREC_ABORT);
		}
	switch (rtype = krec_packet(kc, &plen, &rseq, packet))
		{
	case 'S':
		kc->xfertime = startinterval();
		getparams(kc, FALSE, packet);
		slen = (unsigned)buildparams(kc, FALSE, kc->kr.resp_pckt.pdata);
		tchkt = kc->its_chkt;
		kc->its_chkt = 1;	  /* response must use checktype 1 */
		ksend_packet(kc, 'Y', slen, kc->ksequence, &kc->kr.resp_pckt);
		kc->its_chkt = tchkt;
		kc->kr.oldtries = kc->tries;
		kc->tries = 0;
		kc->ksequence = (kc->ksequence + 1) % 64;
		++kc->packetnum;
		return(KREC_FILE);
		/*lint -unreachable*/
		break;

	case 'T':
	case BAD_PACKET:
		kc->kr.lasterr = (rtype == 'T' ? KE_TIMEOUT : KE_BAD_PACKET);
		ksend_packet(kc, 'N', 0, kc->ksequence, &kc->kr.resp_pckt);
		return(KREC_INIT);	/* try again */
		/*lint -unreachable*/
		break;

	case 'E':
		kc->kr.lasterr = KE_RMTERR;
		StrCharCopy(kc->xtra_err, packet);
		kc->abort_code = KA_RMTERR;
		return(KREC_ABORT);
		/*lint -unreachable*/
		break;

	default:
		kc->kr.lasterr = KE_WRONG;
		kc->abort_code = KA_BAD_FORMAT;
		return(KREC_ABORT);
		/*lint -unreachable*/
		break;
		}
	/*lint -unreachable*/
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krec_file
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int krec_file(ST_KRM *kc)
	{
	int rtype;
	int plen, rseq;
	unsigned slen;
	int result;
	unsigned char packet[MAXPCKT];
	struct st_rcv_open stRcv;
	// struct fn_parts fns;
	// int disk;

	if (kc->tries++ > kc->k_retries)
		{
		kc->abort_code = KA_ERRLIMIT ;
		return(KREC_ABORT);
		}
	rtype = krec_packet(kc, &plen, &rseq, packet);
	if (kc->kr.store_later && (rtype != 'B' || rseq != kc->ksequence))
		{
		/* if kc->kr.store_later is TRUE, it means a file has been received
		 *	 but it hasn't been closed yet to allow us to come here and
		 *	 see if we should stop the transfer timer before closing and
		 *	 logging the file. If the transfer isn't over, take care of
		 *	 the last file.
		 */
		// bf_setcheck(NULL);

		xfer_close_rcv_file(kc->hSession,
							kc->fhdl,
							TSC_OK,
							kc->their_fname,
							kc->our_fname,
							FALSE,
							kc->basesize + kc->kbytes_received,
							kc->k_useattr ? kc->kr.ul_filetime : 0);

		kc->kr.store_later = FALSE;
		kc->total_thru += kc->kbytes_received;
		kc->kbytes_received = 0;
		}

	switch (rtype)
		{
	case 'F':
		if (rseq != kc->ksequence)
			{
			kc->kr.lasterr = KE_SEQUENCE;
			kc->abort_code = KA_OUT_OF_SEQ;
			return(KREC_ABORT);
			}
		StrCharCopy(kc->their_fname, packet);

		stRcv.pszSuggestedName = kc->their_fname;
		stRcv.pszActualName = kc->our_fname;

		kc->kr.ul_compare_time = 0;

		xfer_build_rcv_name(kc->hSession, &stRcv);

		result = xfer_open_rcv_file(kc->hSession, &stRcv, 0L);

		if (result != 0)
			{
			unsigned char buffer[64];

			LoadString(glblQueryDllHinst(),
						IDS_TM_KRM_CANT_OPEN,
						buffer, sizeof(buffer) / sizeof(TCHAR));
			StrCharCopy(kc->kr.resp_pckt.pdata, buffer);
			ksend_packet(kc, 'E', StrCharGetByteCount(kc->kr.resp_pckt.pdata),
					kc->ksequence,
					&kc->kr.resp_pckt);
			kc->kr.lasterr = KE_FATAL;
			switch (result)
			{
			case -6:
				kc->abort_code = KA_USER_REFUSED;
				break;
			case -5:
				kc->abort_code = KA_CANT_OPEN;
				break;
			case -4:
				kc->abort_code = KA_NO_FILETIME;
				break;
			case -3:
				kc->abort_code = KA_TOO_MANY;
				break;
			case -2:
				kc->abort_code = KA_OLDER_FILE;
				break;
			case -1:
			default:
				kc->abort_code = KA_DISK_ERROR;
				break;
			}
			return(KREC_ABORT);
			}
		kc->fhdl = stRcv.bfHdl;
		kc->basesize = stRcv.lInitialSize;

		/* return our file name in Y packet */
		StrCharCopy(kc->kr.resp_pckt.pdata, kc->our_fname);
		ksend_packet(kc, 'Y', StrCharGetByteCount(kc->our_fname), kc->ksequence,
				&kc->kr.resp_pckt);

		kc->kr.oldtries = kc->tries;
		kc->tries = 0;
		kc->ksequence = (kc->ksequence + 1) % 64;
		++kc->packetnum;
		kc->kr.ul_filetime = 0;			/* no date received yet */
		kc->kr.size_known = FALSE;

		kc->kbytes_received = 0L;
		kc->p_kputc = kr_putc;
		return(KREC_DATA);
		/*lint -unreachable*/
		break;

	case 'B':	/* end of batch */
		if (rseq != kc->ksequence)
			{
			kc->kr.lasterr = KE_SEQUENCE;
			kc->abort_code = KA_OUT_OF_SEQ;
			return(KREC_ABORT);
			}
		ksend_packet(kc, 'Y', 0, kc->ksequence, &kc->kr.resp_pckt);
		kc->xfertime = interval(kc->xfertime);
		kr_progress(kc, TRANSFER_DONE);
		//hp_report_xtime((unsigned)(kc->xfertime / 10L));
		if (kc->kr.store_later)
			{
			/* This stuff is here to allow us to stop the transfer timer
			   before spending time closing the last file and logging the
			   transfer.
			*/
			//bf_setcheck(NULL);

			xfer_close_rcv_file(kc->hSession,
								kc->fhdl,
								TSC_OK,
								kc->their_fname,
								kc->our_fname,
								FALSE,
								kc->basesize + kc->kbytes_received,
								kc->k_useattr ? kc->kr.ul_filetime : 0);

			kc->kr.store_later = FALSE;
			kc->total_thru += kc->kbytes_received;
			kc->kbytes_received = 0;
			}
		kc->tries = 0;
		return(KREC_COMPLETE);
		/*lint -unreachable*/
		break;

	case 'S':	/* received another send init packet, maybe they missed ACK */
		if (kc->kr.oldtries++ > kc->k_retries)
			{
			kc->abort_code = KA_ERRLIMIT ;
			return(KREC_ABORT);
			}
		if (rseq == ((kc->ksequence == 0) ? 63 : kc->ksequence - 1))
			{
			slen = (unsigned)buildparams(kc, FALSE, kc->kr.resp_pckt.pdata);
			ksend_packet(kc, 'Y', slen, rseq, &kc->kr.resp_pckt);
			kc->tries = 0;
			kc->kr.dsptries = kc->kr.oldtries;
			kc->kr.lasterr = KE_REPEAT;
			return(KREC_FILE);
			}
		else
			{
			kc->kr.lasterr = KE_WRONG;
			kc->abort_code = KA_BAD_FORMAT;
			return(KREC_ABORT);
			}
		/*lint -unreachable*/
		break;

	case 'Z':
		if (kc->kr.oldtries++ > kc->k_retries)
			{
			kc->abort_code = KA_ERRLIMIT ;
			return(KREC_ABORT);
			}
		if (rseq == ((kc->ksequence == 0) ? 63 : kc->ksequence - 1))
			{
			ksend_packet(kc, 'Y', 0, rseq, &kc->kr.resp_pckt);
			kc->tries = 0;
			kc->kr.dsptries = kc->kr.oldtries;
			kc->kr.lasterr = KE_REPEAT;
			return(KREC_FILE);
			}
		else
			{
			kc->abort_code = KA_BAD_FORMAT;
			kc->kr.lasterr = KE_WRONG;
			return(KREC_ABORT);
			}
		/*lint -unreachable*/
		break;

	case 'T':
	case BAD_PACKET:
		kc->kr.lasterr = (rtype == 'T' ? KE_TIMEOUT : KE_BAD_PACKET);
		ksend_packet(kc, 'N', 0, kc->ksequence, &kc->kr.resp_pckt);
		return(KREC_FILE);	/* try again */
		/*lint -unreachable*/
		break;

	case 'E':
		kc->kr.lasterr = KE_RMTERR;
		StrCharCopy(kc->xtra_err, packet);
		kc->abort_code = KA_RMTERR;
		return(KREC_ABORT);
		/*lint -unreachable*/
		break;

	default:
		kc->kr.lasterr = KE_WRONG;
		kc->abort_code = KA_BAD_FORMAT;
		return(KREC_ABORT);
		/*lint -unreachable*/
		break;
		}
	/*lint -unreachable*/
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krec_data
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int krec_data(ST_KRM *kc)
	{
	int rtype;
	int kplen, krseq;
	unsigned char packet[MAXPCKT];

	if (kc->tries++ > kc->k_retries)
		{
		kc->abort_code = KA_ERRLIMIT;
		return(KREC_ABORT);
		}
	if (kc->kr.next_rtype == '\0')
		rtype = krec_packet(kc, &kplen, &krseq, packet);
	else
		{
		rtype = kc->kr.next_rtype;
		kplen = kc->kr.next_plen;
		krseq = kc->kr.next_rseq;
		MemCopy(packet, kc->kr.next_packet, (unsigned)MAXPCKT);
		kc->kr.next_rtype = '\0';
		}
	switch (rtype)
		{
	case 'A':					/* attribute packet */
	case 'D':					/* data packet		*/
		if (krseq != kc->ksequence)
			{
			if (kc->kr.oldtries++ > kc->k_retries)
				{
				kc->abort_code = KA_ERRLIMIT;
				return(KREC_ABORT);
				}
			if (krseq == ((kc->ksequence == 0) ? 63 : kc->ksequence - 1))
				{
				ksend_packet(kc, 'Y', 0, krseq, &kc->kr.resp_pckt);
				kc->tries = 0;
				kc->kr.dsptries = kc->kr.oldtries;
				kc->kr.lasterr = KE_REPEAT;
				return(KREC_DATA);
				}
			else
				{
				kc->abort_code = KA_OUT_OF_SEQ;
				return(KREC_ABORT);
				}
			}
		if (rtype == 'D')
			{
			/* got good data */

			/* If the /N option is being used, and an older file is being
			 *	received, a request was made to reject the file in the
			 *	response to the attribute packet. If a data packet comes
			 *	in anyway it means the sender failed to respond appropriately
			 *	and we must abort the transfer.
			 */
			if (kc->kr.uabort_code == 'N')
				{
				kc->abort_code = KA_WONT_CANCEL;
				return(KREC_ABORT);
				}

			/* If there is a compare_time still specified, it means the /N
			 *	option was specified but the sender failed to include an
			 *	attribute packet with a filetime. We must abort the transfer
			 */
			if (kc->kr.ul_compare_time != 0)
				{
				kc->abort_code = KA_NO_FILETIME;
				return(KREC_ABORT);
				}

			/* check for pending kbd abort and inlcude 'Z' or 'X' in packet */
			kc->kr.data_packet_rcvd = TRUE;
			kc->kr.resp_pckt.pdata[0] = kc->kr.uabort_code;
			ksend_packet(kc, 'Y', kc->kr.uabort_code ? 1 : 0, kc->ksequence,
					&kc->kr.resp_pckt);
			kr_progress(kc, 0);
			if (kunload_packet(kc, kplen, packet) == ERROR)
				{	/* storage file error */
				kc->kr.lasterr = KE_FATAL;
				kc->abort_code = KA_DISK_ERROR;
				LoadString(glblQueryDllHinst(),
							IDS_TM_KRM_CANT_WRITE,
							kc->xtra_err, sizeof(kc->xtra_err) / sizeof(TCHAR));
				/* we're already sent response to this packet, wait & send
					error packet with next packet number */
				Sleep((DWORD)1000);
				StrCharCopy(kc->kr.resp_pckt.pdata, kc->xtra_err);
				ksend_packet(kc, 'E',
						StrCharGetByteCount(kc->kr.resp_pckt.pdata),
						(kc->ksequence + 1) % 64, &kc->kr.resp_pckt);
				return(KREC_ABORT);
				}

			}
		else if (rtype == 'A')
			{
			if (kc->kr.data_packet_rcvd) /* all 'A' packets must precede 'D' packets */
				{
				kc->kr.lasterr = KE_WRONG;
				kc->abort_code = KA_BAD_FORMAT;
				return(KREC_ABORT);
				}
			// strblank(kc->kr.resp_pckt.pdata);
			kc->kr.resp_pckt.pdata[0] = '\0';
			kunload_attributes(kc, packet, &kc->kr.resp_pckt);
			ksend_packet(kc, 'Y',
						StrCharGetByteCount(kc->kr.resp_pckt.pdata),
						kc->ksequence,
						&kc->kr.resp_pckt);
			}
		kc->kr.oldtries = kc->tries;
		kc->tries = 0;
		kc->ksequence = (kc->ksequence + 1) % 64;
		++kc->packetnum;
		return(KREC_DATA);
		/*lint -unreachable*/
		break;

	case 'Z':	/* end of file */
		if (krseq != kc->ksequence)
			{
			kc->kr.lasterr = KE_WRONG;
			kc->abort_code = KA_OUT_OF_SEQ;
			return(KREC_ABORT);
			}
		if (strcmp(packet, "D") == 0) /* discard file? */
			{
			if (!kc->kr.uabort_code)
				kc->abort_code = KA_RABORT1;

			xfer_close_rcv_file(kc->hSession,
								kc->fhdl,
								kresult_code[kc->abort_code],
								kc->their_fname,
								kc->our_fname,
								FALSE,
								0L,
								kc->k_useattr ? kc->kr.ul_filetime : 0);

			kc->total_thru += kc->kbytes_received;
			kc->kbytes_received = 0;
			++kc->kr.files_aborted;
			}
		else
			{
			/* file has been received */
			kr_progress(kc, FILE_DONE);
			++kc->kr.files_received;

			/* if all is well, hold off on closing file and logging tranfer
			 * until after the next packet is in. This way we can stop the
			 * transfer timer earlier
			 */
			if (kc->abort_code == KA_OK)
				kc->kr.store_later = TRUE;
			else
				{
				xfer_close_rcv_file(kc->hSession,
									kc->fhdl,
									kresult_code[kc->abort_code],
									kc->their_fname,
									kc->our_fname,
									FALSE,
									kc->basesize + kc->kbytes_received,
									kc->k_useattr ? kc->kr.ul_filetime : 0);

				kc->total_thru += kc->kbytes_received;
				kc->kbytes_received = 0;
				}
			}

		ksend_packet(kc, 'Y', 0, kc->ksequence, &kc->kr.resp_pckt);
		if (kc->kr.uabort_code == 'X' || kc->kr.uabort_code == 'N')
			kc->kr.uabort_code = '\0', kc->abort_code = KA_OK;
		kc->kr.oldtries = kc->tries;
		kc->tries = 0;
		kc->ksequence = (kc->ksequence + 1) % 64;
		++kc->packetnum;
		return(KREC_FILE);
		/*lint -unreachable*/
		break;
	case 'F':		/* receiving file name again? */
		if (kc->kr.oldtries++ > kc->k_retries)
			{
			kc->abort_code = KA_ERRLIMIT;
			return(KREC_ABORT);
			}
		if (krseq == ((kc->ksequence == 0) ? 63 : kc->ksequence - 1))
			{
			StrCharCopy(kc->kr.resp_pckt.pdata, kc->our_fname);
			ksend_packet(kc, 'Y',
						StrCharGetByteCount(kc->our_fname),
						krseq,
						&kc->kr.resp_pckt);
			kc->tries = 0;
			kc->kr.lasterr = KE_REPEAT;
			kc->kr.dsptries = kc->kr.oldtries;
			return(KREC_DATA);
			}
		else
			{
			kc->kr.lasterr = KE_WRONG;
			kc->abort_code = KA_BAD_FORMAT;
			return(KREC_ABORT);
			}
		/*lint -unreachable*/
		break;

/* account for repeated 'X' packet here */
	case 'T':
	case BAD_PACKET:
		kc->kr.lasterr = (rtype == 'T' ? KE_TIMEOUT : KE_BAD_PACKET);
		ksend_packet(kc, 'N', 0, kc->ksequence, &kc->kr.resp_pckt);
		return(KREC_DATA);	/* try again */
		/*lint -unreachable*/
		break;

	case 'E':
		kc->kr.lasterr = KE_RMTERR;
		StrCharCopy(kc->xtra_err, packet);
		kc->abort_code = KA_RMTERR;
		return(KREC_ABORT);
		/*lint -unreachable*/
		break;

	default:
		kc->kr.lasterr = KE_WRONG;
		kc->abort_code = KA_BAD_FORMAT;
		return(KREC_ABORT);
		/*lint -unreachable*/
		break;
		}
	/*lint -unreachable*/
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * kr_progress
 *
 * DESCRIPTION:
 *	Displays transfer progress indicators for Kermit receive
 *
 * ARGUMENTS:
 *	final -- TRUE if final display for a file.
 *
 * RETURNS:
 *	nothing
 */
void kr_progress(ST_KRM *kc, int status)
	{
	long ttime, stime;
	long bytes_rcvd;
	long cps;
	int  k_rcvd;
	long krm_stime = -1;
	long krm_ttime = -1;
	long krm_cps = -1;
	long krm_file_so_far = -1;
	long krm_total_so_far = -1;

	if (kc->xfertime == -1L)
		return;
	ttime = bittest(status, TRANSFER_DONE) ?
			kc->xfertime : interval(kc->xfertime);
	if ((stime = ttime / 10L) != kc->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		krm_stime = stime;

		bytes_rcvd = kc->total_thru + kc->kbytes_received;
		if (bittest(status, FILE_DONE | TRANSFER_DONE))
			k_rcvd = (int)PART_HUNKS(bytes_rcvd, 1024);
		else
			k_rcvd = (int)FULL_HUNKS(bytes_rcvd, 1024);

		krm_total_so_far = k_rcvd;

		krm_file_so_far = kc->kbytes_received;

		if (stime > 0 && (cps = (bytes_rcvd * 10L) / ttime) > 0)
			{
			krm_cps = cps;

			if ((kc->kr.bytes_expected > 0))
				{
				ttime = (kc->kr.bytes_expected - kc->kbytes_received) / cps;

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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krm_rcheck
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
#if 0
void krm_rcheck(HSESSION hS, bool before)
	{
	if (before)
		{
		/* wait till next packet is in before writing to disk */
		Dbg(krm_dbg, D_KRM_RCHECK);
		kc->kr.next_rtype = krec_packet(hS, &kc->kr.next_plen, &kc->kr.next_rseq,
				kc->kr.next_packet);
		}
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krm_check_input
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *
 */
#if 0
void krm_check_input(bool suspend)
	{
	int disk;
	struct fn_parts fns;
	static USHORT old_ovr;
		   USHORT new_ovr;

	if (suspend)
		{
		(VOID)mComGetErrors(comhdl, FALSE, NULL, &old_ovr, NULL, NULL);
		}
	else
		{
		(VOID)mComGetErrors(comhdl, FALSE, NULL, &new_ovr, NULL, NULL);
		if (new_ovr > old_ovr)
			{
			/*
			 * Got an error.  Make sure things are taken care of
			 * so that we don't get any more errors.
			 */
			if (kc->fhdl != NULL)
				{
				/* only do this if it might have been our file I/O */
				bf_setcheck(NULL);
				bf_setcheck(krm_rcheck);
				fl_dissect_fn(bf_name(kc->fhdl), &fns);
				disk = (fns.fn_drv[0] - 'A') & 0x1F;
				transfer_setspeed(disk, cnfg.bit_rate);
				}
			}
		}
	}
#endif


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * kunload_attributes
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void kunload_attributes(ST_KRM *kc, unsigned char *data, KPCKT *rsp_pckt)
	{
	unsigned char *limit = data + StrCharGetByteCount(data);
	unsigned len;
	unsigned char attrfield[20];

	while (data <= limit - 2)
		{
		len = (unsigned char)unchar(data[1]);
		if ((data + len + 1) >= limit)
			break;

		if (len <= sizeof(attrfield) - 1)
			{
			strncpy(attrfield, data + 2, len);
			attrfield[len] = '\0';
			switch (*data)
				{
			case '0':
				break;

			case '1':
				/* This attribute gives an exact byte count of a file as it
				 *	it was stored on the senders system. This is what we need
				 *	to display a vu_meter etc. as the file is received
				 *	(There is also a '!' field available that contains the
				 *	filesize expressed in K. We don't currently use that
				 *	field.
				 */
				if ((kc->kr.bytes_expected = atol(attrfield)) > 0)
					{
					kc->kr.size_known = TRUE;

					xferMsgFilesize(kc->hSession, kc->kr.bytes_expected);
					}
				break;

			case '#':
				/* This field specifies the creation date of the file on the
				 * senders system, expressed as "[yy]yymmdd[ hh:mm[:ss]]".
				 * We use this for two things: If the user has asked us to use
				 * received attributes, we set the time/date of the new file
				 * based on this field. If the user specified the /N receive
				 * option, we compare this received file time with the filetime
				 * of any existing file of the same name and reject the file
				 * unless the incoming file is newer.
				 */

				/* extract date/time from the packet data field */
				krm_settime(attrfield, &kc->kr.ul_filetime);

				/* if kc->kr.compare_time contains a valid date/time, it means
				 *	that the /N option was used and the file being received
				 *	already exists. Compare the two times and reject any file
				 *	that is not newer than what we already have.
				 */
				if (kc->kr.ul_compare_time != 0)
					{
					/* if incoming file (kc->kr.filetime) <= existing file
					 * (kc->kr.compare_time), reject the file
					 */
					if (kc->kr.ul_filetime <= kc->kr.ul_compare_time)
						{
						/* reject incoming file */
						StrCharCopy(rsp_pckt->pdata, "N#");
						kc->kr.uabort_code = 'N';
						kc->abort_code = KA_OLDER_FILE;
						}
					else
						{
						/* clear compare_time or transfer would fail when
						 * first data packet is received
						 */
						kc->kr.ul_compare_time = 0;
						}
					}

				/* user may opt not to use received file time/date */
				if (!kc->k_useattr)
					kc->kr.ul_filetime = 0;
				break;

			default :
				/* ignore */
				break;
				}
			}
		data += (len + 2);
		}
	}

#define atoc(c) ((c) - (CHAR)'0')
#define	isadigit(x)	((x >= '0') && (x <= '9'))

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * krm_settime
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void krm_settime(unsigned char *data, unsigned long *ptime)
	{
	unsigned long ltime;
	struct tm sT;
	unsigned char *datestr = data;
	unsigned char *timestr = NULL;
	char ch;
	int s;
	unsigned i;
	unsigned sl;

	sT.tm_year = sT.tm_hour = -1;

	if ((sl = StrCharGetByteCount(data)) > 7 && data[6] == ' ')
		{
		data[6] = '\0';
		timestr = &data[7];
		}
	else if (sl > 9 && data[8] == ' ')
		{
		data[8] = '\0';
		timestr = &data[9];
		}
	/* try to get date */
	if ((sl = StrCharGetByteCount(datestr)) == 6 || sl == 8)
		{
		for (i = 0; i < sl; ++i)
			if (!isadigit(datestr[i]))
				break;
		if (i == sl)
			{
			if (sl == 8)
				{
				ch = atoc(datestr[0]);
				s = (int)ch;
				sT.tm_year = s * 1000;
				sT.tm_year += ((int)atoc(datestr[1]) * 100);
				datestr += 2;
				}
			else
				{
				sT.tm_year = 1900;
				}
			sT.tm_year += (atoc(datestr[0]) * 10 + atoc(datestr[1]));
			sT.tm_mon = atoc(datestr[2]) * 10 + atoc(datestr[3]);
			sT.tm_mday = atoc(datestr[4]) * 10 + atoc(datestr[5]);
			}
		if (sT.tm_mon > 12 || sT.tm_mday > 31)
			sT.tm_year = -1;
		}

	/* try to get a time */
	if (timestr)
		{
		if (((sl = StrCharGetByteCount(timestr)) == 5 || (sl == 8 && timestr[5] == ':'))
				&& timestr[2] == ':')
			{
			sT.tm_hour = atoc(timestr[0]) * 10 + atoc(timestr[1]);
			sT.tm_min = atoc(timestr[3]) * 10 + atoc(timestr[4]);
			if (sl == 8)
				sT.tm_sec = atoc(timestr[6]) * 10 + atoc(timestr[7]);
			else
				sT.tm_sec = 0;
			}
		if (sT.tm_hour > 24 || sT.tm_min > 59 || sT.tm_sec > 59)
			sT.tm_hour = -1;
		}

	if (sT.tm_year == -1 || sT.tm_hour == -1)
		return;

	sT.tm_year -= 1900;
	ltime = (unsigned long)mktime(&sT);
	ltime += itimeGetBasetime();
	*ptime = ltime;
	}

/*********************** end of krm_rcv.c ****************************/
