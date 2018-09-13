/* File: C:\WACKER\xfer\hpr_rcv1.c (Created: 24-Jan-1994)
 * created from HAWIN source file
 * hpr_rcv1.c  --  Routines to implement HyperProtocol receiver.
 *
 *	Copyright 1989,1993,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
// #define DEBUGSTR

#include <windows.h>
#include <setjmp.h>
#include <stdlib.h>
#include <time.h>
#include <term\res.h>
#include <sys\types.h>
#include <sys\utime.h>

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
#include "itime.h"

#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "hpr.h"
#include "hpr.hh"
#include "hpr_sd.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_decode_msg
 *
 * DESCRIPTION:
 *	Called when a complete message has been received to decode the fields
 *	of the message. Messages are composed of printable characters in one
 *	or more fields separated by semicolons. Each field within a message
 *	is considered to be independent of any others but fields are guaranteed
 *	to be examined left to right with a message. Fields containing badly
 *	formatted information generate an error but fields containing unrecognized
 *	data are ignored.
 *	Note also that certain message fields may prevent subsequent fields from
 *	being decoded. For instance, if a partial data block 'P' field indicates
 *	that the previous data block was no good, a restart will be requested
 *	immediately and any undecoded fields will be ignored.
 *
 * ARGUMENTS:
 *	mdata -- A pointer to the printable string that contains the message
 *				fields.
 *
 * RETURNS:
 *	A status code which can be one of:
 *		H_OK		if all fields were decoded without incident
 *		H_BADMSGFMT if a field is of a recognized type but contains badly
 *						formatted data.
 *		H_NORESYNCH if a restart was needed as the result of a field but
 *						could not be completed.
 *		H_FILEERR	if a file error occurred while processing a field
 *		H_COMPLETE	if the end of transfer field was encountered.
 */
int hr_decode_msg(struct s_hc *hc, BYTE *mdata)
	{
	HCOM hCom;
	BYTE *field;
	BYTE mstr[20];
	int result = H_OK, retval, HRE_code;
	int i;
	BYTE typecode;
	struct st_rcv_open stRcv;

	hCom = sessQueryComHdl(hc->hSession);

	// field = _fstrtok(mdata, FstrScolon());
	field = strtok(mdata, ";");
	while (field != NULL)
		{
		switch(typecode = *field++)
			{
		case 'N' : /* number of files to expect */
			if (!hc->rc.files_expected)
				{
				hc->rc.files_expected = (int)atol(field);
				if (hc->rc.files_expected > 1 && hc->rc.single_file &&
						!hr_cancel(hc, HRE_TOO_MANY))
					return(H_BADMSGFMT);	/* this may need a new status */
				hrdsp_filecnt(hc, hc->rc.files_expected);
				}
			break;

		case 'S' :	/* size in bytes of all files */
			if (hc->rc.bytes_expected == -1L)
				{
				if ((hc->rc.bytes_expected = atol(field)) <= 0)
					hc->rc.bytes_expected = -1L;
				else
					hrdsp_totalsize(hc, hc->rc.bytes_expected);
				}
			break;

		case 'A' :	/* abort current file */
			hrdsp_event(hc, HRE_RMTABORT);
			if (hc->fhdl)
				hr_closefile(hc, HRE_RMTABORT);
			break;

		case 'P' :	/* check partial block */
			if ((int)atol(field) != (hc->blocksize - hc->datacnt)
					|| (field = strchr(field, ',')) == NULL
					|| (unsigned)atol(field + 1)
					!= (hc->usecrc ? hc->h_crc : hc->h_checksum))
				if (!hr_restart(hc, HRE_DATAERR))
					return(H_NORESYNCH);
			hc->rc.checkpoint = hc->h_filebytes;
			hc->datacnt = hc->blocksize;
			hc->h_checksum = hc->h_crc = 0;
			break;

		case 'F' :	/* new file */
			if (hc->current_filen && hc->fhdl && !hr_closefile(hc, HRE_NONE))
				return(H_FILEERR);
			/* set up for new file */
			hc->h_filebytes = hc->rc.checkpoint = 0L;
			hc->rc.filesize = -1L;
			hc->rc.ul_lstw = 0;				/* don't know date/time */
			if ((int)atol(field) != ++hc->current_filen ||
					(field = strchr(field, ',')) == NULL)
				return(H_BADMSGFMT);
			strcpy(hc->rc.rmtfname, ++field);
			hc->rc.filesize = -1L;


			/* set up argument structure and let transfer open the file
			 */
			stRcv.pszSuggestedName = hc->rc.rmtfname;
			stRcv.pszActualName = hc->rc.ourfname;
			/* TODO: figure out how this time stuff works */
			stRcv.lFileTime = hc->rc.ul_cmp;

			// stRcv.pfnVscanOutput = hr_virus_detect;
			// stRcv.pfnVscanOutput = (VOID (FAR *)(VOID FAR *, USHORT))hc->rc.pfVirusCheck;
			// stRcv.ssmchVscanHdl = hc->rc.ssmchVscan;

			if ((retval = xfer_open_rcv_file(hc->hSession, &stRcv, 0L)) != 0)
				{
				switch (retval)
				{
				case -6:
					HRE_code = HRE_USER_REFUSED;
					break;
				case -5:
					HRE_code = HRE_DISK_ERR;
					break;
				case -4:
					HRE_code = HRE_NO_FILETIME;
					break;
				case -3:
					HRE_code = HRE_CANT_OPEN;
					break;
				case -2:
					HRE_code = HRE_OLDER_FILE;
					break;
				case -1:
				default:
					HRE_code = HRE_CANT_OPEN;
					break;
				}

				if (!hr_cancel(hc, HRE_code))
					return(H_FILEERR);
				}
			hc->fhdl = stRcv.bfHdl;
			hc->rc.basesize = stRcv.lInitialSize;

			// hc->rc.ssmchVscan = stRcv.ssmchVscanHdl;
			// xferMsgVirusScan(hSession, (hc->rc.ssmchVscan == (SSHDLMCH)0) ? 0:1);
			// hc->rc.virus_detected = FALSE;

			hrdsp_compress(hc, FALSE);
			hrdsp_newfile(hc, hc->current_filen, hc->rc.rmtfname, hc->rc.ourfname);
			hc->datacnt = hc->blocksize;
			hc->h_checksum = hc->h_crc = 0;

			/* If there is no valid compare time, then the /N option is not
			 *	being used or there is no existing file and we can set up to
			 *	store received data. If there is a compare time, keep tossing
			 *	characters until we've verified that the incoming file is
			 *	newer than what we have.
			 */
			if (hc->rc.ul_cmp == (unsigned long)(-1))
				hc->rc.hr_ptr_putc = hr_toss;
			//else if (hc->rc.ssmchVscan == (SSHDLMCH)0)/* is virscan active? */
			//	hc->rc.hr_ptr_putc = hr_putc;
			// else
			//hc->rc.hr_ptr_putc = hr_putc_vir;
			else
				hc->rc.hr_ptr_putc = hr_putc;

			break;

		case 'V' :	/* Version identifier from sender */
			i = (int)atol(field);
			if ((field = strchr(field, ',')) == NULL)
				return(H_BADMSGFMT);

			/* if version restrictions were found, stop the transfer */
			if (hpr_id_check(hc, i, ++field))
				break;
			/* else fall through */

		case 'X' :
			if (hc->rc.cancel_reason == HRE_NONE)
				hc->rc.cancel_reason = HRE_RMTABORT;
			hrdsp_event(hc, hc->rc.cancel_reason);
			/* fall through */

		case 'E' :	/* end of transfer */
			omsg_new(hc, 'c');
			// StrFmt(mstr, "E%d", hc->current_filen + 1);
			wsprintf(mstr, "E%d", hc->current_filen + 1);
			omsg_add(hc, mstr);
			// RemoteClear();  /* get rid of any prod messages */
			ComRcvBufrClear(hCom);

			/* Sending the next message as a burst causes the excess characters
			 *	to be sent to the remote system after the transfer has ended,
			 *	perhaps causing them to dither on the display. It is tempting
			 *	to send a single message rather that a burst, but the receiver
			 *	is going to assume the transfer is done and exit after sending
			 *	this message so there is no chance for an error recovery if
			 *	the message is hit.
			 */
			omsg_send(hc, BURSTSIZE, FALSE, TRUE);
			hc->xfertime = interval(hc->xfertimer);
			hrdsp_status(hc, typecode == 'E' ? HRS_COMPLETE : HRS_CANCELLED);
			if (hc->current_filen > 0 && hc->fhdl)
				{
				if (typecode == 'E')
					{
					if (!hr_closefile(hc, HRE_NONE))
						return(H_FILEERR);
					}
				else
					hr_closefile(hc, hc->rc.cancel_reason);
				}
			return(H_COMPLETE);

		case 'C' :
			if (strlen(field) == 0 &&
				!decompress_start(&hc->rc.hr_ptr_putc, hc, FALSE))
				{
				result = H_FILEERR;
				}
			else
				{
				hrdsp_compress(hc, TRUE);
				hc->rc.using_compression = TRUE;
				hrdsp_status(hc, HRS_REC);
				}
			break;

		case 's' :
			if ((hc->rc.filesize = atol(field)) > 0)
				hrdsp_filesize(hc, hc->rc.filesize);
			break;

		case 't' :	/* file time/date */
			{
			unsigned long ulTime;
			struct tm stT;

			memset(&stT, 0, sizeof(struct tm));
			stT.tm_year = atoi(field);		/* ?? adjust */
			if ((field = strchr(field, ',')) != NULL)
				stT.tm_mon = atoi(++field);
			if (field != NULL && (field = strchr(field, ',')) != NULL)
				stT.tm_mday = atoi(++field);
			if (field != NULL && (field = strchr(field, ',')) != NULL)
				stT.tm_hour = atoi(++field);
			if (field != NULL && (field = strchr(field, ',')) != NULL)
				stT.tm_min = atoi(++field);
			if (field != NULL && (field = strchr(field, ',')) != NULL)
				stT.tm_sec = atoi(++field);

			ulTime = mktime(&stT);
			if ((long)ulTime == (-1))
				ulTime = 0;
			else
				ulTime -= itimeGetBasetime();

			hc->rc.ul_lstw = ulTime;

			/*
			 * if hc->rc.ul_cmp contains a valid date/time, it means that
			 * the /N options was used and the file being received already
			 * exists.  Compare the two times and reject any file that is
			 * not newer than what we already have.
			 */
			if (hc->rc.ul_cmp != 0)
				{
				if (hc->rc.ul_cmp >= hc->rc.ul_lstw)
					{
					/* reject incoming file */
					hr_reject_file(hc, HRE_OLDER_FILE);
					hc->rc.ul_cmp = (unsigned long)(-1);
					}
				else
					{
					/* everything is OK */
					// if (hc->rc.ssmchVscan == (SSHDLMCH)0)
						hc->rc.hr_ptr_putc = hr_putc;	/* no virus checking */
					// else   
					// 	hc->rc.hr_ptr_putc = hr_putc_vir;	/* use vir check */
					hc->rc.ul_cmp = 0;
					}
				}
			}
			break;

		case 'B' :
			hc->blocksize = (int)atol(field);
			break;

		case 'D' :
			hc->deadmantime = atol(field) * 10;
			break;

		case 'I' :
			hr_still_alive(hc, TRUE, FALSE);
			break;

		case 'T' :
			if ((i = (int)atol(field)) == 1)
				hc->usecrc = FALSE;
			else if (i == 2)
				hc->usecrc = TRUE;
			else
				result = H_BADMSGFMT;
			break;

		default:
			/* ignore, might be option supported in later version */
			break;
			}
		// field = _fstrtok(NULL, FstrScolon());
		field = strtok(NULL, ";");
		}
	return(result);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_reject_file
 *
 * DESCRIPTION:
 *	Called when we are trying to reject a file that we are receiving.
 *
 * ARGUEMENTS:
 *	reason	-- why we are rejecting the file
 *
 * RETURNS:
 *	Always TRUE
 */
int	hr_reject_file(struct s_hc *hc, int reason)
	{
	BYTE mstr[20];

	/* reject incoming file */
	omsg_new(hc, 'c');
	wsprintf(mstr, "A%d", hc->current_filen);
	omsg_add(hc, mstr);
	omsg_send(hc, BURSTSIZE, FALSE, TRUE);
	hrdsp_event(hc, reason);
	hr_closefile(hc, reason);

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_closefile
 *
 * DESCRIPTION:
 *	Called when the receiver is through saving to a particular file prior
 *	to switching to another file or finishing the transfer.
 *
 * ARGUMENTS:
 *	reason -- an event code indicating why this file is being closed
 *
 * RETURNS:
 *	TRUE if file is closed without problem.
 *	FALSE if an error occurred while closing.
 */
int hr_closefile(struct s_hc *hc, int reason)
	{
	int retcode;

	hrdsp_progress(hc, FILE_DONE);
	hc->total_thru += hc->h_filebytes;
	hc->total_dsp += ((hc->rc.filesize == -1) ? hc->h_filebytes : hc->rc.filesize);
	if (hc->fhdl != NULL)
		{
		// retcode = transfer_close_rcv_file(hc->fhdl, hr_result_codes[reason],
		retcode = xfer_close_rcv_file(hc->hSession,
									hc->fhdl,
									hr_result_codes[reason],
									hc->rc.rmtfname,
									hc->rc.ourfname,
									xfer_save_partial(hc->hSession),
									hc->h_filebytes + hc->rc.basesize,
									hc->h_useattr ? hc->rc.ul_lstw : 0L);
		}

	hc->fhdl = NULL;
	hc->h_filebytes = 0;
	hc->rc.using_compression = FALSE;
	hc->rc.hr_ptr_putc = hr_toss;
	return retcode;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_cancel
 *
 * DESCRIPTION:
 *	This routine is called when a problem or interruption during receiving
 *	forces a cancellation of the transfer. This routine attempts to inform
 *	the sender of the cancellation so that a graceful shutdown can be
 *	accomplished.
 *	It is up to the sender to actually shut down the transfer. We just send
 *	an interruption message to the sender asking him to do so.
 *
 * ARGUMENTS:
 *	reason -- A code providing the reason we must cancel. The reason codes
 *			are the same as the receiver event codes defined in the header
 *			file as HRE_*****
 *
 * RETURNS:
 *	TRUE if the sender responded to our request within a reasonable time.
 *	FALSE if the sender did not respond.
 *
 */
int hr_cancel(struct s_hc *hc, int reason)
	{
	HCOM hCom;
	BYTE str[40];

	hCom = sessQueryComHdl(hc->hSession);

	if (hc->rc.cancel_reason == HRE_NONE)
		hc->rc.cancel_reason = reason;

	if (reason == HRE_LOST_CARR)
		return FALSE;

	omsg_new(hc, 'c');
	// StrFmt(str, "X;R%d,%lu", hc->current_filen, hc->rc.checkpoint);
	wsprintf(str, "X;R%d,%lu", hc->current_filen, hc->rc.checkpoint);

	DbgOutStr("%s\r\n", (LPSTR)str, 0,0,0,0);

	omsg_add(hc, str);
	hc->rc.hr_ptr_putc = hr_toss;
	// RemoteClear();
	ComRcvBufrClear(hCom);
	hc->h_filebytes = hc->rc.checkpoint;
	omsg_send(hc, BURSTSIZE, FALSE, FALSE);
	return(hr_resynch(hc, reason));
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_restart
 *
 * DESCRIPTION:
 *	This routine is called when an error in the received data forces us
 *	to restart the transfer at the last known good location. This routine
 *	formats the interruption request message to the sender, sends the first
 *	message, then passes control to hr_resynch to figure out when and if
 *	the sender responds.
 *
 * ARGUMENTS:
 *	reason -- A code indicating the reason for the restart. The codes used
 *			are the receiver event codes itemized in the header file as
 *			HRE_*****
 *
 * RETURNS:
 *	TRUE if sender responds to the restart request.
 *	FALSE if the sender does not respond within a reasonable time.
 */
int hr_restart(struct s_hc *hc, int reason)
	{
	HCOM hCom;
	BYTE str[15];

	hCom = sessQueryComHdl(hc->hSession);

	omsg_new(hc, 'c');
	// StrFmt(str, "R%d,%lu", hc->current_filen, hc->rc.checkpoint);
	wsprintf(str, "R%d,%lu", hc->current_filen, hc->rc.checkpoint);
	omsg_add(hc, str);
	// RemoteClear();
	ComRcvBufrClear(hCom);

	/* Don't start accepting characters if file is being rejected due to
	 *	/N option and there is a file open to receive them.
	 */
	if ((hc->rc.ul_cmp == (unsigned long)(-1)) && (hc->fhdl != NULL))
		// hc->rc.hr_ptr_putc = (hc->rc.ssmchVscan == (SSHDLMCH)0) ? hr_putc : hr_putc_vir;
		hc->rc.hr_ptr_putc = hr_putc;
	else
		hc->rc.hr_ptr_putc = hr_toss;

	if (hc->rc.using_compression)
		decompress_start(&hc->rc.hr_ptr_putc, hc, FALSE);
	hc->h_filebytes = hc->rc.checkpoint;
	omsg_send(hc, BURSTSIZE, FALSE, FALSE);
	if (hc->fhdl)
		{
		fio_seek(hc->fhdl,
				(long)hc->rc.checkpoint + hc->rc.basesize,
				FIO_SEEK_SET);
		}
	return(hr_resynch(hc, reason));
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_resynch
 *
 * DESCRIPTION:
 *	Attempts to resynchronize the receiver with the data stream coming
 *	from the sender.  This routine is called when the receiver starts or
 *	after any interruption where the receiver asked the sender to restart
 *	its sending from a new location. The calling routine will have already
 *	sent the first copy of the restart message to the sender. This routine
 *	will monitor all messages arriving from the sender, looking for a
 *	restart message that matches the last request sent out. This routine
 *	will resend the restart request message at intervals to account for
 *	possible data loss or error in either the receiver message or the
 *	response from the sender.
 *
 * ARGUMENTS:
 *	hSession -- The session handle
 *	reason	 -- A reason code describing why the resynch must be performed.
 *				This will be HRE_NONE only in the case of initial startup.
 *
 * RETURNS:
 *	TRUE if resynch was successful.
 *	FALSE if resynch failed
 */
int hr_resynch(struct s_hc *hc, int reason)
	{
	HCOM hCom;
	int fStarted = FALSE;
	int tries = 0;
	int try_limit = 0;
	int request_again = FALSE; /* first request already made */
	int result, i, j;
	int iret;
	// long l;
	long timer, time_limit;
	TCHAR rcode;
	int mtype;
	BYTE *msgdata;
	BYTE *field;
	int ok;
	struct
		{
		int msgn;
		long time;
		} request_tbl[MAX_START_TRIES];
	/* this variable is not accessed if NOTIMEOUTS is on */
	long tmptime;				/*lint -esym(550,tmptime)*/

	hCom = sessQueryComHdl(hc->hSession);

	// reason will be HRE_NONE only if this is the initial resynch call
	// so we will set fStarted appropriately and use it to control behavior
	// later on
	if (reason == HRE_NONE)
		{
		fStarted = FALSE;
		try_limit = MAX_START_TRIES;
		}
	else
		{
		fStarted = TRUE;
		try_limit = MAXRETRIES;
		hrdsp_event(hc, reason);
		hrdsp_status(hc, HRS_REQRESYNCH);
		++hc->total_tries;
		hrdsp_errorcnt(hc, hc->total_tries);
		}
	// time_limit = RESYNCH_TIME;		// MOBIDEM
	j = hc->h_resynctime;
	j *= 10;
	if (j == 0)
		j = RESYNCH_TIME;
	time_limit = j;

	request_tbl[0].msgn = omsg_number(hc);
	timer = request_tbl[0].time = startinterval();
	DbgOutStr("Initial:%d, %ld, %ld\r\n",
			request_tbl[0].msgn, timer, time_limit, 0, 0);
	for ( ; ; )
		{
		if (request_again)
			{
			if (++tries >= try_limit)
				{
				hrdsp_status(hc, HRS_CANCELLED);
				if (hc->rc.cancel_reason == HRE_NONE)
					hc->rc.cancel_reason = HRE_RETRYERR;
				return(FALSE);
				}
			if (fStarted)
				omsg_send(hc, BURSTSIZE, FALSE, FALSE);
			else
				omsg_send(hc, 1, FALSE, TRUE);

			// Record this attempt in the table so we can match up responses
			request_tbl[tries].msgn = omsg_number(hc);
			timer = request_tbl[tries].time = startinterval();
			++hc->total_tries;
			hrdsp_errorcnt(hc, hc->total_tries);
			request_again = FALSE;
			DbgOutStr("Restart:%d, %0ld, %0ld\r\n",
					request_tbl[tries].msgn, timer, time_limit, 0, 0);
			}

		iret = xfer_user_interrupt(hc->hSession);
		if (iret == XFER_ABORT)
			{
			if (!fStarted || hc->ucancel)
				return(FALSE);
			else
				{
				return(hr_cancel(hc, HRE_USRCANCEL));
				}
			}
		else if (iret == XFER_SKIP)
			{
			hr_reject_file(hc, HRE_USER_SKIP);
			}

		if (xfer_carrier_lost(hc->hSession))
			return hr_cancel(hc, HRE_LOST_CARR);

#if !defined(NOTIMEOUTS)
		if ((tmptime = interval(timer)) > time_limit)
			{
			request_again = TRUE;
			if (fStarted)
				// time_limit += RESYNCH_INC;		// MOBIDEM
				{
				j = hc->h_resynctime;
				j *= 10;
				if (j == 0)
					j = RESYNCH_INC;
				time_limit += j;
				}
			hrdsp_event(hc, HRE_NORESP);
			}
		else
#endif
		if (mComRcvChar(hCom, &rcode) == 0)
			xfer_idle(hc->hSession);
		else if (rcode == H_MSGCHAR)
			{
			/* restart messages always start with #0 & use CRC */
			hc->rc.expected_msg = 0;
			hc->usecrc = TRUE;
			mtype = ' ';
			result = hr_collect_msg(hc, &mtype, &msgdata, H_CHARTIME);
			if (result == HR_KBDINT)
				{
				if (hc->ucancel)
					return(FALSE);
				else
					{
					hr_kbdint(hc);
					return(hr_cancel(hc, HRE_USRCANCEL));
					}
				}

			else if (result == HR_LOST_CARR)
				return hr_cancel(hc, HRE_LOST_CARR);

			else if (result != HR_COMPLETE) /* HR_TIMEOUT or HR_BAD??? */
				{
				DbgOutStr("Bad message\r\n", 0,0,0,0,0);
				if (mtype == 'R')	/* probably our resynch, try again */
					{
					DbgOutStr("Bad message was type R\r\n", 0,0,0,0,0);
					hrdsp_event(hc, HRE_RETRYERR);
					request_again = TRUE;
					}
				}
			else if (mtype == 'R')	/* good Resynch message */
				{
				if (!fStarted)
					hc->xfertimer = startinterval();
				fStarted = TRUE;
				field = strtok(msgdata, ";");
				if (field == NULL || *field++ != 'R')
					{
					DbgOutStr("Bad format\r\n", 0,0,0,0,0);
					request_again = TRUE;
					hrdsp_event(hc, HRE_ILLEGAL);
					}
				else
					{
					if (unchar(*field) != (BYTE)request_tbl[tries].msgn)
						{
						for (i = tries - 1; i >= 0; --i)
							if (unchar(*field) == (BYTE)request_tbl[i].msgn)
								{
								// We got a response to an earlier restart request.
								// Adjust the amount of time we wait for responses
								// based on the actual time of the message just
								// received.

								// time_limit = interval(request_tbl[i].time) +
								//		RESYNCH_INC;		// MOBIDEM
								j = hc->h_resynctime;
								j *= 10;
								if (j == 0)
									j = RESYNCH_INC;
								time_limit = interval(request_tbl[i].time) + j;

								DbgOutStr("Got %d, new time_limit=%ld\r\n",
										request_tbl[i].msgn, time_limit, 0,0,0);
								break;	/* don't ask for retry */
								}
						if (i < 0)
							{
							hrdsp_event(hc, HRE_RETRYERR);
							request_again = TRUE;
							DbgOutStr("Not in table\r\n", 0,0,0,0,0);
							}
						}
					else
						{
						ok = TRUE;
						// field = _fstrtok(NULL, FstrScolon());
						field = strtok(NULL, ";");
						while (field != NULL)
							{
							switch (*field++)
								{
							case 'f':
								if ((int)atol(field) != hc->current_filen)
									ok = FALSE;
								break;
							case 'o':
								if (atol(field) != hc->rc.checkpoint)
									ok = FALSE;
								break;
							case 'B':
								hc->blocksize = (int)atol(field);
								break;
							case 'T' :
								if ((i = (int)atol(field)) == 1)
									hc->usecrc = FALSE;
								else if (i == 2)
									hc->usecrc = TRUE;
								break;
							default:
								break;
								}
							// field = _fstrtok(NULL, FstrScolon());
							field = strtok(NULL, ";");
							}
						if (ok)
							{
							hc->h_checksum = hc->h_crc = 0;
							hc->datacnt = hc->blocksize;
							if (reason != HRE_NONE || tries > 0)
								hrdsp_event(hc, HRE_ERRFIXED);
							hrdsp_status(hc, HRS_REC);
							DbgOutStr("Resynch successful\r\n", 0,0,0,0,0);
							return(TRUE);
							}
						else
							{
							hrdsp_event(hc, HRE_RETRYERR);
							request_again = TRUE;
							DbgOutStr("Not OK\r\n", 0,0,0,0,0);
							}
						}
					}
				}
			}
		}
	/*lint -unreachable*/
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_virus_detect
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
#if FALSE
STATICF VOID _export PASCAL hr_virus_detect(VOID FAR *h, USHORT usMatchId)
	{

	hc->rc.virus_detected = TRUE; /* force cancel of transfer */
	}
#endif


/********************** end of hpr_rcv1.c ************************/
