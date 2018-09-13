/* File: C:\WACKER\xfer\hpr_rcv0.c (created: 24-Jun-1994)
 * created from HAWIN source file:
 * hpr_rcv0.c -- Routines to implement HyperProtocol receiver.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#include <setjmp.h>
#include <time.h>
#include <term\res.h>
#include <sys\types.h>
#include <sys\utime.h>

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
// #include <tdll\com.h>
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

#include "hpr.h"
#include "hpr.hh"
#include "hpr_sd.hh"

/* 
 *  not all event codes in this table are needed here, but having a complete
 *	table simplifies the lookup code substantially.
 */
int hr_result_codes[] = /* maps HyperProtocol event codes to result codes */
	{
	TSC_OK, 			/* HRE_NONE 	   */
	TSC_ERROR_LIMIT,	/* HRE_DATAERR	   */
	TSC_OUT_OF_SEQ, 	/* HRE_LOSTDATA    */
	TSC_NO_RESPONSE,	/* HRE_NORESP	   */
	TSC_ERROR_LIMIT,	/* HRE_RETRYERR    */
	TSC_BAD_FORMAT, 	/* HRE_ILLEGAL	   */
	TSC_OK, 			/* HRE_ERRFIXED    */
	TSC_RMT_CANNED, 	/* HRE_RMTABORT    */
	TSC_USER_CANNED,	/* HRE_USRCANCEL   */
	TSC_NO_RESPONSE,	/* HRE_TIMEOUT	   */
	TSC_ERROR_LIMIT,	/* HRE_DCMPERR	   */
	TSC_LOST_CARRIER,	/* HRE_LOST_CARR   */
	TSC_TOO_MANY,		/* HRE_TOO_MANY    */
	TSC_DISK_FULL,		/* HRE_DISK_FULL   */
	TSC_CANT_OPEN,		/* HRE_CANT_OPEN   */
	TSC_DISK_ERROR, 	/* HRE_DISK_ERR    */
	TSC_OLDER_FILE, 	/* HRE_OLDER_FILE  */
	TSC_NO_FILETIME,	/* HRE_NO_FILETIME */
	TSC_VIRUS_DETECT,	/* HRE_VIRUS_DET   */
	TSC_USER_SKIP,		/* HRE_USER_SKIP   */
	TSC_REFUSE			/* HRE_REFUSE	   */
	};


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hpr_rcv
 *
 * DESCRIPTION:
 *	Receives files using the Hyperprotocol transfer method.
 *
 * ARGUMENTS:
 *	attended	-- True if the program determines that a user is likely to be
 *					 present at the computer keyboard. FALSE if a user is NOT
 *					 likely to be present (such as host and script modes)
 *	single_file -- TRUE if user specified only a file name to receive the
 *					result of the transfer as opposed to naming a dirctory.
 *
 * RETURNS:
 *	TRUE if the transfer successfully completes. FALSE otherwise.
 */
int hpr_rcv(HSESSION hSession, int attended, int single_file)
	{
	struct s_hc *hc;
	HCOM	  hCom;
	int	      usRetVal;
	int		  iret;
	int 	  status;
	int 	  result;
	int 	  timeout_cnt = 0;
	int       mtype;
	BYTE     *mdata;
	char	  str[20];
	long	  timer;
	struct st_rcv_open stRcv;
	BYTE	  tmp_name[FNAME_LEN];

	hCom = sessQueryComHdl(hSession);

	hc = malloc(sizeof(struct s_hc));
	if (hc == NULL)
		return TSC_NO_MEM;

	memset(hc, 0, sizeof(struct s_hc));

	hc->hSession = hSession;

	/* initialize stuff */
	if (!hr_setup(hc))
		{
		free(hc);
		return TSC_NO_MEM;
		}

	/* initialize control variables */

	/* blocksize depends on the speed of the connection. Larger block sizes
	 * can be used for faster connections. If the blocksize is too large,
	 * error detection will be slow. If too small, there is unnecessary overhead
	 */
	hc->blocksize = 2048;
	// hc->blocksize = xfer_blocksize(hSession);

	hc->current_filen = 0;
	hc->datacnt = hc->blocksize;
	hc->deadmantime = 600;
	hc->total_tries = 0;
	hc->total_thru = 0L;
	hc->total_dsp = 0L;
	hc->ucancel = FALSE;
	hc->usecrc = TRUE;		/* first messages in will use CRC */
	hc->fhdl = NULL;

	hc->rc.checkpoint = 0L;
	hc->rc.files_expected = 0;
	hc->rc.bytes_expected = -1L;
	hc->rc.filesize = -1L;
	hc->rc.expected_msg = 0;
	hc->rc.cancel_reason = HRE_NONE;
	hc->rc.using_compression = FALSE;
	hc->rc.virus_detected = FALSE;

	hc->rc.hr_ptr_putc = hr_toss;
	hc->h_crc = hc->h_checksum = 0;

	omsg_init(hc, TRUE, FALSE);
	hc->rc.single_file = single_file; 	/*	 to the sender					 */


	/* Receiver begins the transfer by transmitting a starting message
		repeatedly until the sender begins */

	/* prepare the initial message */
	omsg_new(hc, 'c');

	/* tell sender who we are */
	hpr_id_get(hc, str);
	omsg_add(hc, str);

	/* we can express our opinion about what checktype and blocksize to use
	 *	but it will be up to the sender to make the final choice
	 */
	wsprintf(str, "T%d", hc->h_chkt == H_CRC ? H_CRC : H_CHECKSUM);
	omsg_add(hc, str);

	wsprintf(str, "B%d", hc->blocksize);
	omsg_add(hc, str);

	/* let sender know whether we can handle compression */
	if (hc->h_trycompress & compress_enable());
		{
		omsg_add(hc, "C");
		}

	/* A restart 0,0 request causes sender to start */
	omsg_add(hc, "R0,0");

	/* send first response packet at intervals until first H_MSGCHAR
	 *	 is received
	 */
	status = H_OK;
	hrdsp_status(hc, HRS_REQSTART);
	timer = startinterval();

	stRcv.pszSuggestedName = "junk.jnk";
	stRcv.pszActualName = tmp_name;
	// stRcv.pstFtCompare = NULL;
	stRcv.lFileTime = 0;
	// stRcv.pfnVscanOutput = NULL;
	// stRcv.ssmchVscanHdl = (SSHDLMCH)0;

	// hc->rc.pfVirusCheck = MakeProcInstance((FARPROC)hr_virus_detect,
	//								hSession->hInstance);

	// stRcv.pfnVscanOutput = (VOID (FAR *)(void *, int))hc->rc.pfVirusCheck;

	// transfer_build_rcv_name(&stRcv);
	xfer_build_rcv_name(hSession, &stRcv);

	// hc->rc.ssmchVscan = stRcv.ssmchVscanHdl;

	hc->xfertimer = -1;

#if FALSE
	/* if we are the host, don't send an immediate start request because
	 * the user probably had to start us first and then set himself up. If
	 * we are the attended machine, though, the other end has probably already
	 * been started.
	 */
	// sendnext = (attended ? 0 : 40);

	// Changed to always try to start immediately since we may be responding
	// to auto-start in which case sender is already waiting
	sendnext = 0;

	repeat
		{
		hc->xfertimer = startinterval();
		if (mComRcvBufrPeek(hCom, &rcode) != 0)
			{
			if (rcode == H_MSGCHAR)
				{
				if (!hr_resynch(hSession, HRE_NONE))
					status = H_NOSTART;
				break;
				}
			// RemoteGet(); 	   /* wrong character, remove it from buffer */
			mComRcvChar(hCom, &rcode);

			/* Other end can send us an ESC to cancel the transfer before
			 *	it ever gets started.
			 */
			if ((rcode == ESC) || (rcode == CAN))
				{
				status = H_RMTABORT;
				hrdsp_event(hSession, hc->rc.cancel_reason = HRE_RMTABORT);
				break;
				}
			}

		/* We can't wait forever to get started. If we haven't seen a start
		 *	character in H_START_WAIT seconds, give up.
		 */
		if ((time = interval(timer)) > H_START_WAIT * 10)
			{
			status = H_NOSTART;
			hc->rc.cancel_reason = HRE_NORESP;
			break;
			}

		/* see if it's time to send another startup request */
		else if (time > sendnext || (rcode & 0x7F) == '\r')
			{
			sendnext = time + 40;	/* send again in 4 seconds */
			omsg_send(hc, 1, FALSE, TRUE);
			}

		/* finally, see if someone at keyboard want's us to stop trying */
		iret = xfer_user_interrupt(hSession);
		if (iret == XFER_ABORT)
			{
			status = H_USERABORT;
			hrdsp_event(hSession, hc->rc.cancel_reason = HRE_USRCANCEL);
			break;
			}
		else if (iret == XFER_SKIP)
			{
			hr_reject_file(hSession, HRE_USER_SKIP);
			}

		hpr_idle(hSession);

		}

#endif

	omsg_send(hc, 1, FALSE, TRUE);
	if (!hr_resynch(hc, HRE_NONE))
		status = H_NOSTART;

	/* If status is still H_OK, it means we've synched with sender.
	 * We'll stay in this loop now until the transfer is finished.
	 */
	while (status == H_OK)
		{
		hr_still_alive(hc, FALSE, FALSE);   /* check whether deadman msg
											is in order */
		hrdsp_progress(hc, 0);		  /* keep user notified */

		/* Collect blocks of data, which may be interrupted by messages
		 * from the sender.
		 */
		result = hr_collect_data(hc, &hc->datacnt, TRUE, H_CHARTIME);
		if (result != HR_TIMEOUT)
			timeout_cnt = 0;
		switch(result)
			{
		case HR_VIRUS_FOUND:
			goto virus_found;

		case HR_COMPLETE:
			/* got all chars. we asked for, setup to receive another
			 * full block
			 */
			hc->rc.checkpoint = hc->h_filebytes;
			hc->h_checksum = hc->h_crc = 0;
			hc->datacnt = hc->blocksize;
			break;

		case HR_DCMPERR :
			/* data error caused decompression algorithm to fail */
			if (!hr_restart(hc, HRE_DCMPERR))
				status = H_NORESYNCH;
			break;

		case HR_BADCHECK :
			/* got complete block but checksum or CRC didn't match */
			if (!hr_restart(hc, HRE_DATAERR))
				status = H_NORESYNCH;
			break;

		case HR_LOSTDATA :
			/* received block n+1 before block n */
			if (!hr_restart(hc, HRE_LOSTDATA))
				status = H_NORESYNCH;
			break;

		case HR_MESSAGE:
			/* Block of data was interrupted by a message. All that's
			 * actually been detected is a message character in the data,
			 * we must now extract and analyze the message
			 */
			switch(result = hr_collect_msg(hc, &mtype, &mdata, H_CHARTIME))
				{
			case HR_KBDINT:
				/* local user interrupted us while receiving the message
				 * if user had interrupted us once and is doing it again
				 * while we are attempting to tell the other end what we're
				 * doing, drop out immediately and leave the sender to fend
				 * for himself.
				 */
				if (hc->ucancel)
					status = H_USERABORT;
				else
					{
					hr_kbdint(hc);
					/* try to let sender know what we're doing */
					if (!hr_cancel(hc, HRE_USRCANCEL))
						status = H_USERABORT;
					}
				break;

			case HR_TIMEOUT:
			case HR_BADMSG:
			case HR_BADCHECK:
				/* message was scrambled, try to resynch */
				if (!hr_restart(hc, HRE_DATAERR))
					status = H_NORESYNCH;
				break;

			case HR_LOSTDATA:
				/* message was recevied, but it was the wrong one */
				if (!hr_restart(hc, HRE_LOSTDATA))
					status = H_NORESYNCH;
				break;

			case HR_COMPLETE:
				/* message received ok, figure out what sender wants */
				status = hr_decode_msg(hc, mdata);
				break;
				}
			break;

		case HR_TIMEOUT:
			/* sender stopped sending to us, try to prod him into restartting */
			if (timeout_cnt++ < TIMEOUT_LIMIT)
				{
				/* TODO: generalize this
				if (cnfg.save_xprot)
					RemoteSendChar(cnfg.save_xon);
				*/
				hr_still_alive(hc, TRUE, TRUE); /* send file ack and timeout msg */
				}
			else
				{
				status = H_TIMEOUT;
				hc->rc.cancel_reason = HRE_TIMEOUT;
				}
			break;

		case HR_KBDINT:
			/* user is trying to interrupt the transfer */
			if (hc->ucancel)
				status = H_USERABORT;
			else
				{
				hr_kbdint(hc);
				/* try to inform sender about what we are doing */
				if (!hr_cancel(hc, HRE_USRCANCEL))
					status = H_USERABORT;
				}
			break;

		case HR_LOST_CARR:
			/* we lost carrier while trying to transfer */
			if (!hr_cancel(hc, HRE_LOST_CARR))
				status = H_TIMEOUT;
			break;

		case HR_FILEERR:
			/* A file error occurred while trying to save incoming data */
			if (!hr_cancel(hc, HRE_DISK_ERR))
				status = H_FILEERR;
			break;
			}

		/* during full-bore transfers, the data collection routines won't
		 *	waste time checking the keyboard for an interrupt request from
		 *	the user or carrier loss so we'll check here at least once per
		 *	data block
		 */
		iret = xfer_user_interrupt(hSession);
		if (iret == XFER_ABORT)
			{
			if (hc->ucancel)
				status = H_USERABORT;
			else
				{
				hr_kbdint(hc);
				if (!hr_cancel(hc, HRE_USRCANCEL))
					status = H_USERABORT;
				}
			}
		else if (iret == XFER_SKIP)
			{
			hr_reject_file(hc, HRE_USER_SKIP);
			}

		if (xfer_carrier_lost(hSession))
			if (!hr_cancel(hc, HRE_LOST_CARR))
					status = H_TIMEOUT;

		/* Actual virus detection occurs deep in the bowels of a transfer.
		 * Therefore, the detecting routine merely sets a flag and begins
		 * tossing data. We actually shut down here
		 */
virus_found:
		if (hc->rc.virus_detected)
			{
			hc->rc.virus_detected = FALSE;	/* don't come in here again */
			if (!hr_cancel(hc, HRE_VIRUS_DET))
				status = H_USERABORT;
			}
		}

	/* Transfer is all done, 'status' indicates the final result. */
	hrdsp_progress(hc, TRANSFER_DONE);
	compress_disable();

	// if (stRcv.ssmchVscanHdl != (SSHDLMCH)0)
	//	StrSrchStopSrch(stRcv.ssmchVscanHdl);

	// if (hc->rc.pfVirusCheck != NULL)
	//	{
	//	FreeProcInstance(hc->rc.pfVirusCheck);
	//	hc->rc.pfVirusCheck = NULL;
	//	}

	usRetVal = (int)hr_result_codes[hc->rc.cancel_reason];

	/* clear display, free memory, etc. */
	status = hr_wrapup(hc, attended, status);
	free(hc);

	return usRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_collect_msg
 *
 * DESCRIPTION:
 *	Called when a message has been detected within a block of data. Messages
 *	start with H_MSGCHAR (0x01). If an H_MSGCHAR occurs as part of the data
 *	being sent, it will be doubled. When it is encountered alone, this routine
 *	is called to extract the following message from the stream of data.
 *
 * ARGUMENTS:
 *	mtype	-- pointer to a variable to be updated with the message type
 *	mdata	-- pointer to a variable to be updated with the address of the
 *				message data
 *	timeout -- amount of time (in tenths of seconds) to wait for the data
 *				to complete the message.
 *
 * RETURNS:
 *	Returns a status code:
 *	  HR_COMPLETE -- message successfully received
 *	  HR_BADCHECK -- crc or checksum error on message data
 *	  HR_TIMEOUT  -- time out exceeded while waiting for data
 *	  HR_KBDINT   -- user interrupted from keyboard
 *	  HR_BADMSG   -- message data was not in recognized format
 *	  HR_LOSTDATA -- message was complete but message number was not the
 *						expected one.
 *	  HR_LOST_CARR --Lost carrier while collecting message
 */
// char FAR *storageptr;	 /* place to put data as we receive it */

int hr_collect_msg(struct s_hc *hc,
					int *mtype,
					BYTE **mdata,
					long timeout)
	{
	unsigned hold_checksum;
	unsigned hold_crc;
	int gotlen = FALSE;
	int count;
	int result = HR_UNDECIDED;
	int (*holdptr)(void *, int);
	int msgn;

	/* since a message is embedded within a data block, we need to preserve
	 * a few values for the interrupted data collection routine.
	 */
	holdptr = hc->rc.hr_ptr_putc;
	/* set collection routine to store data for us */
	hc->rc.hr_ptr_putc = hr_storedata;
	hold_checksum = hc->h_checksum;
	hold_crc = hc->h_crc;
	hc->h_checksum = 0; 			/* messages have their own check bytes */
	hc->h_crc = 0;

	/* We will retrieve the message in two parts, first we'll get the type and
	 *	length fields, then, based on those, we can collect the rest of the
	 *	message.
	 */
	hc->storageptr = hc->rc.rmsg_bufr;
	count = 2;
	while (result == HR_UNDECIDED)
		{
		switch (result = hr_collect_data(hc, &count, FALSE, timeout))
			{
		case HR_COMPLETE:
			if (!gotlen)
				{
				/* got first part, set up to get rest of message */
				result = HR_UNDECIDED;
				*mtype = hc->rc.rmsg_bufr[0];
				count = hc->rc.rmsg_bufr[1];
				if (count < 3)
					result = HR_BADMSG;
				gotlen = TRUE;
				}
			else
				{
				/* got everything, check for valid message */
				msgn = hc->rc.rmsg_bufr[2];
				count = hc->rc.rmsg_bufr[1];
				if (hc->usecrc)
					{
					if (hc->h_crc != 0)
						result = HR_BADCHECK;
					}
				else
					{
					hc->h_checksum -= hc->rc.rmsg_bufr[count];
					hc->h_checksum -= hc->rc.rmsg_bufr[count + 1];
					if (hc->rc.rmsg_bufr[count] != (BYTE)(hc->h_checksum % 256) ||
							hc->rc.rmsg_bufr[count + 1] !=
							(BYTE)(hc->h_checksum / 256))
						result = HR_BADCHECK;
					}
				hc->rc.rmsg_bufr[count] = '\0';
				}
			break;

		case HR_LOST_CARR:
		case HR_TIMEOUT:
		case HR_KBDINT:
			/* return same result */
			break;

		case HR_MESSAGE:
			/* we encountered what looked like a message within a message
			 *	but that is illegal
			 */
			result = HR_BADMSG;
			break;
			}
		}

	/* we're done, restore details for overlying data collection routine */
	hc->rc.hr_ptr_putc = holdptr;
	hc->h_checksum = hold_checksum;
	hc->h_crc = hold_crc;
	*mdata = &hc->rc.rmsg_bufr[3];
	if (result == HR_COMPLETE)
		{
		if (msgn != hc->rc.expected_msg)
			result = HR_LOSTDATA;
		else
			hc->rc.expected_msg = ++hc->rc.expected_msg % 256;
		}
	return(result);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_still_alive
 *
 * DESCRIPTION:
 *	This routine is called periodically during receiving. It determines
 *	whether it is time to send the sender a 'deadman' message. Since there
 *	is no regular response from the receiver to the sender unless errors
 *	occur, the deadman message prevents the sender from sending into a void
 *	for long periods of time. If the sender doesn't receive ANYTHING from
 *	the receiver for the negotiated deadman time, it can assume the receiver
 *	is no longer active.
 *
 * ARGUMENTS:
 *	force	  -- TRUE if a deadman notification should be sent whether it is
 *					officially time for one or not.
 *	timed_out -- TRUE if receiver has timeout and we want sender to know that.
 *
 * RETURNS:
 *	nothing
 */
void hr_still_alive(struct s_hc *hc, int force, int timed_out)
	{
	char msg[20];

	if (force || (long)interval(omsg_last(hc)) >= hc->deadmantime)
		{
		omsg_new(hc, 'c');
		if (timed_out)
			omsg_add(hc, "t");

		/* While we're talking to the sender, we'll let him know how much
		 *	we've actually received. This lets him clear the table of
		 *	unacknowledged files that he keeps.
		 */
		// StrFmt(msg, "f%d,%lu", hc->current_filen, hc->rc.checkpoint);
		wsprintf(msg, "f%d,%lu", hc->current_filen, hc->rc.checkpoint);
		omsg_add(hc, msg);
		omsg_send(hc, BURSTSIZE, FALSE, FALSE);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_kbdint
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
void hr_kbdint(struct s_hc *hc)
	{
	/* TODO: fix this somehow
	if (!hc->ucancel)
		errorline(FALSE, strld(TM_WAIT_CONF));
	*/
	hc->ucancel = TRUE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_suspend_input
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
void hr_suspend_input(void *hS, int suspend)
	{
#if FALSE
	if (suspend)
		suspendinput(FLG_DISK_ACTIVE, 5);
	else
		allowinput(FLG_DISK_ACTIVE);
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_check_input
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *
 */
void	hr_check_input(void *hS, int suspend)
	{
	}



/********************** end of hpr_rcv0.c ***************************/
