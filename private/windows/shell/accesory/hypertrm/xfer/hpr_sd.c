/* File: C:\WACKER\xfer\hpr_sd.c (Created: 25-Jan-1994)
 * created from HAWIN source file
 * hpr_sd.c -- System dependent routines to support HyperProtocol
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
// #define DEBUGSTR


#include <windows.h>
#include <setjmp.h>
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

#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "hpr.h"
#include "hpr.hh"
#include "hpr_sd.hh"

// struct s_hprsd FAR *hsd;

/* RECEIVING */

#define DRR_RCV_FILE 1
#define DRR_STORING  2

// int suspend_for_disk = 0;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_setup
 *
 * DESCRIPTION:
 *	Called as a HyperProtocol receive session is beginning. This routine
 *	must allocate memory for control structures hc and hrc. It may also
 *	initiate a screen display to use during the transfer and do any other
 *	necessary setup.
 *	Values for the user-settable globals, h_useattr, h_trycompress, h_chkt
 *	and h_suspenddsk, should also be set here if they have not already been
 *	set through the use of program options or menu settings.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	TRUE if transfer can continue.
 *	FALSE if a memory allocation or other type of error occurred.
 */
int hr_setup(struct s_hc *hc)
	{
	unsigned int uiOldOptions;
	XFR_PARAMS *pX;
	XFR_HP_PARAMS *pH;

	pX = (XFR_PARAMS *)0;
	xfrQueryParameters(hc->hSession, (VOID **)&pX);
	if (pX != (XFR_PARAMS *)0)
		hc->h_useattr = pX->fUseDateTime;

	pH = (XFR_HP_PARAMS *)xfer_get_params(hc->hSession, XF_HYPERP);

	hc->blocksize     = pH->nBlockSize;
	hc->h_chkt        = (pH->nCheckType == HP_CT_CRC) ? H_CRC : H_CHECKSUM;
	hc->h_resynctime  = pH->nResyncTimeout;
	hc->h_trycompress = pH->nTryCompression;

	if (xfer_set_comport(hc->hSession, FALSE, &uiOldOptions) != TRUE)
		{
		/* TODO: put in an error message of some sort */
		return FALSE;
		}

	hc->sd.hld_options = uiOldOptions;

	hrdsp_compress(hc, compress_status() == COMPRESS_ACTIVE);

	hc->sd.k_received = 0;

	return TRUE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_wrapup
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
int hr_wrapup(struct s_hc *hc, int attended, int status)
	{

 	xfer_restore_comport(hc->hSession, hc->sd.hld_options);
	/* TODO: decide if anything can be done if we get an error */

	if (hc->fhdl || status == H_FILEERR)   /* abnormal exit */
		{
		xfer_log_xfer(hc->hSession, FALSE,
					hc->rc.rmtfname, NULL,
					hr_result_codes[hc->rc.cancel_reason]);

		fio_close(hc->fhdl);
		hc->fhdl = NULL;
		DeleteFile(hc->rc.ourfname);
		}
	if (hc->fhdl)
		{
		fio_close(hc->fhdl);
		hc->fhdl = NULL;
		}
	if (attended)
		{
#if FALSE
		menu_bottom_line(BL_ESC, 0L);
#if defined(OS2)
		if (os2_cfg_popup)
			popup_replybox(-1, ENTER_RESP, T_POPUP_WAIT, TM_POPUP_COMPLETE);
#endif
		DosBeep(beepfreq, beeplen);
		menu_replybox(hc->sd.msgrow, ENTER_RESP, 0,
				(int)transfer_status_msg((USHORT)hr_result_codes[hc->rc.cancel_reason]));
#endif
		}

	xferMsgClose(hc->hSession);

	return status;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_progress
 *
 * DESCRIPTION:
 *	Displays the progress of a filetransfer on screen by showing the number of
 *	bytes transferred and updating the vu meters if they have been initialized.
 *
 * ARGUMENTS:
 *	final -- If TRUE, indicates that the transfer is finished and the final
 *			 progress display is needed. Durning a transfer the number of
 *			 bytes is rounded off to the next lower 'k'. After a transfer
 *			 is completed, the final display is rounded UP.
 *
 * RETURNS:
 *
 */
void hrdsp_progress(struct s_hc *hc, int status)
	{
	long ttime, stime;
	long bytes_rcvd;
	long cps;

	long new_elapsed = -1;
	long new_remaining = -1;
	long new_cps = -1;
	long new_file = -1;
	long new_total = -1;

	if (hc->xfertimer == -1L)
		return;
	ttime = bittest(status, TRANSFER_DONE) ?
			hc->xfertime : interval(hc->xfertimer);

	if ((stime = ttime / 10L) != hc->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		/* Display elapsed time */
		new_elapsed = (hc->displayed_time = stime);

		/* Display amount received */
		bytes_rcvd = hc->total_dsp + hc->h_filebytes;

		/* if an error occurs at the end of an interrupt data block, it
		 *	can temporarily appear that we've received more data than is
		 *	actually available -- make sure we don't show more received than
		 *	is coming.
		 */
		if (hc->rc.bytes_expected > 0L && bytes_rcvd > hc->rc.bytes_expected)
			bytes_rcvd = hc->rc.bytes_expected;

		new_file = hc->h_filebytes;
		new_total = bytes_rcvd;

		/* Display current compression status */
		if (!bittest(status, FILE_DONE | TRANSFER_DONE))
			hrdsp_compress(hc, compress_status() == COMPRESS_ACTIVE);

		/* Display throughput and time remaining */
		if (stime > 0 &&
				(cps = ((hc->total_thru + hc->h_filebytes) * 10L) / ttime) > 0)
			{
			new_cps = cps;

			/* calculate time to completion */
			if (hc->rc.bytes_expected > 0L)
				{
				ttime = (hc->rc.bytes_expected - bytes_rcvd) / cps;
				if (hc->rc.files_expected > 1)
					ttime += (hc->rc.files_expected - hc->current_filen);
				new_remaining = ttime;
				}
			else if (hc->rc.filesize > 0L)
				{
				ttime = (hc->rc.filesize - hc->h_filebytes) / cps;
				new_remaining = ttime;
				}
			}
		}

	DbgOutStr("elapsed=%ld, remaining=%ld, cps=%ld\r\n",
			new_elapsed, new_remaining, new_cps, 0, 0);

	xferMsgProgress(hc->hSession,
					new_elapsed,
					new_remaining,
					new_cps,
					new_file,
					new_total);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_status
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
void hrdsp_status(struct s_hc *hc, int status)
	{

	xferMsgStatus(hc->hSession, status);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_event
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
void hrdsp_event(struct s_hc *hc, int event)
	{

	xferMsgEvent(hc->hSession, event);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_filecnt
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
void hrdsp_filecnt(struct s_hc *hc, int cnt)
	{

	xferMsgFilecnt(hc->hSession, cnt);
	}

void hrdsp_errorcnt(struct s_hc *hc, int cnt)
	{

	xferMsgErrorcnt(hc->hSession, cnt);
	}

void hrdsp_compress(struct s_hc *hc, int cnt)
	{

	xferMsgCompression(hc->hSession, cnt);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_totalsize
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
void hrdsp_totalsize(struct s_hc *hc, long bytes)
	{

	xferMsgTotalsize(hc->hSession, bytes);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_newfile
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
void hrdsp_newfile(struct s_hc *hc,
					int filen,
					BYTE *theirname,
					TCHAR *ourname)
	{

	xferMsgNewfile(hc->hSession, filen, theirname, ourname);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hrdsp_filesize
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
void hrdsp_filesize(struct s_hc *hc, long fsize)
	{

	xferMsgFilesize(hc->hSession, fsize);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
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
void NEARF hpr_idle(HSESSION hS)
	{
	/* update elapsed time */
	// task_exec();

	DoYield(mGetCLoopHdl(hS));

	}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *								 SENDING								 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_setup
 *
 * DESCRIPTION:
 *	Handles transfer initialization and intial screen display.
 *
 * ARGUMENTS:
 *	nfiles -- The number of files that will be sent
 *	nbytes -- The total number of bytes to be sent.
 *
 * RETURNS:
 *	TRUE if no errors were encountered and its ok for transfer to proceed.
 *	FALSE if lack of memory prevents start of transfer.
 */
int hs_setup(struct s_hc *hc, int nfiles, long nbytes)
	{
	unsigned int uiOldOptions;
	int enough_memory = TRUE;
	XFR_PARAMS *pX;
	XFR_HP_PARAMS *pH;

	pX = (XFR_PARAMS *)0;
	xfrQueryParameters(hc->hSession, (VOID **)&pX);
	if (pX != (XFR_PARAMS *)0)
		hc->h_useattr = pX->fUseDateTime;

	pH = (XFR_HP_PARAMS *)xfer_get_params(hc->hSession, XF_HYPERP);

	hc->blocksize     = pH->nBlockSize;
	hc->h_chkt        = (pH->nCheckType == HP_CT_CRC) ? H_CRC : H_CHECKSUM;
	hc->h_resynctime  = pH->nResyncTimeout;
	hc->h_trycompress = pH->nTryCompression;

	hc->dptr = hc->msgdata;
	hc->rmcnt = 0;

	if (xfer_set_comport(hc->hSession, TRUE, &uiOldOptions) != TRUE)
		{
		/* TODO: decide of we need an error message or something */
		return FALSE;
		}

	hc->hsxb.bufrsize = HSXB_SIZE;
	hc->hsxb.cnt = HSXB_CYCLE;
	hc->hsxb.total = hc->hsxb.bufrsize - hc->hsxb.cnt;
	hc->hsxb.bptr = hc->hsxb.curbufr;

	hc->hsxb.curbufr = NULL;
	hc->hsxb.altbufr = NULL;

	if (enough_memory)
		hc->hsxb.curbufr = malloc(hc->hsxb.bufrsize);
	if (hc->hsxb.curbufr == NULL)
		enough_memory = FALSE;

	if (enough_memory)
		hc->hsxb.altbufr = malloc(hc->hsxb.bufrsize);
	if (hc->hsxb.altbufr == NULL)
		enough_memory = FALSE;

	/* allocate as much memory as is available for the file table */
	if (enough_memory)
		{
		hc->sc.ft_limit = 32; 			/* just for now */

		hc->sc.hs_ftbl = malloc((size_t)(hc->sc.ft_limit + 1) * sizeof(struct s_ftbl));
		if (hc->sc.hs_ftbl == NULL)
			enough_memory = FALSE;
		}

	/* if we couldn't get enough memory, report it and leave */
	if (!enough_memory)
		{
		if (hc->hsxb.curbufr != NULL)
			free(hc->hsxb.curbufr);
		if (hc->hsxb.altbufr != NULL)
			free(hc->hsxb.altbufr);
		if (hc->sc.hs_ftbl != NULL)
			free(hc->sc.hs_ftbl);

		/* TODO: add reperror call, or something like it */
		assert(enough_memory);

		return(FALSE);
		}

	hc->sd.hld_options = uiOldOptions;

	hsdsp_compress(hc, compress_status() == COMPRESS_ACTIVE);

	xferMsgFilecnt(hc->hSession, nfiles);

	xferMsgTotalsize(hc->hSession, nbytes);

	return TRUE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_wrapup
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
void hs_wrapup(struct s_hc *hc, int attended, int bailout_status)
	{
	// bxmit_clear();	/* make sure no dregs are left in xmitter buffer */
	ComSndBufrClear(sessQueryComHdl(hc->hSession));

	xfer_restore_comport(hc->hSession, hc->sd.hld_options);
	/* TODO: decide if it is useful to actually check for an error */

	if (bailout_status == TSC_OK)
		{
		// hp_report_xtime((unsigned)hc->xfertime);
		}


	/* free all the memory we used */

	xferMsgClose(hc->hSession);

	free(hc->hsxb.curbufr);
	free(hc->hsxb.altbufr);
	free(hc->sc.hs_ftbl);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_fxmit
 *
 * DESCRIPTION:
 *	Sends a single character using HyperProtocol's dual buffering system.
 *	This functions does exactly what the macro hs_xmit_ does, but is in
 *	function form so it can be passed as a pointer.
 *
 * ARGUMENTS:
 *	c -- The character to be transmitted.
 *
 * RETURNS:
 *	nothing
 */
void hs_fxmit(struct s_hc *hc, BYTE c)
	{
	hs_xmit_(hc, c);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_xmit_switch
 *
 * DESCRIPTION:
 *	Sends a character using HyperProtocol's dual buffering system and checks
 *	whether it is time to switch buffers. Normally, one buffer will be in the
 *	process of being transmitted while the other is being filled. If the
 *	transmission is completed before the next buffer has been filled, some
 *	idle time could be wasted waiting for the second buffer to fill. This
 *	routine checks whether the transmitter is idle. If it is, it sets up the
 *	current buffer to start transmitting and begins filling the other buffer.
 *	If the transmitter is still busy, it continues on filling the current
 *	buffer.
 *
 * ARGUMENTS:
 *	c -- The character to be transmitted.
 *
 * RETURNS:
 *	Returns the argument as a convenience.
 */
BYTE hs_xmit_switch(struct s_hc *hc, BYTE c)
	{
	*hc->hsxb.bptr++ = c;	/* place the character in the current buffer */

	if (!ComSndBufrBusy(sessQueryComHdl(hc->hSession)) || (hc->hsxb.total == 0))
		{
		/* start xmitting this buffer and filling the other */
		hs_xbswitch(hc);
		}
	else
		{
		/* keep going with same buffer by starting a new fill cycle */
#if defined(FINETUNE)
		hc->hsxb.cnt = min(usr_hsxb_cycle, hc->hsxb.total);
#else
		hc->hsxb.cnt = min(HSXB_CYCLE, hc->hsxb.total);
#endif
		hc->hsxb.total -= hc->hsxb.cnt;
		}

	xfer_idle(hc->hSession);

	return(c);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_xbswitch
 *
 * DESCRIPTION:
 *	Switches the currently filling buffer with the transmitting buffer.
 *	This function performs the actual switch described in the header for the
 *	function hs_xmit_switch(). The current buffer is set up be transmitted
 *	and the other buffer is setup to be filled. If the transmitter is busy
 *	This function will wait for a defined period of time for it to complete.
 *	If the function has to wait longer than a certain minimum time, the
 *	event and status messages will be updated to inform the user of the delay.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void hs_xbswitch(struct s_hc *hc)
	{
	HCOM hCom;
	register int displayed = FALSE;
	long timer;
	long time;
	unsigned bsize;

	hCom = sessQueryComHdl(hc->hSession);

	if (hc->hsxb.bptr > hc->hsxb.curbufr)/* if there is anything to transmit */
		{
		bsize = (unsigned)(hc->hsxb.bptr - hc->hsxb.curbufr);
		if (ComSndBufrSend(hCom, hc->hsxb.curbufr, bsize, 10))
			{
			timer = startinterval();
			while (ComSndBufrSend(hCom, hc->hsxb.curbufr, bsize, 10))
				{
				// hs_background(hSession);
				/* keep elapsed time display accurate */
				if ((time = (interval(hc->xfertimer) / 10L)) != hc->displayed_time)
					xferMsgProgress(hc->hSession,
									(hc->displayed_time = time),
									-1, -1, -1, -1);


				if (!displayed &&  interval(timer) > 40L)
					{
					hsdsp_event(hc, HSE_FULL);
					hsdsp_status(hc, HSS_WAITRESUME);
					displayed = TRUE;
					}
				}
			if (displayed)
				{
				hsdsp_event(hc, HSE_GOTRESUME);
				hsdsp_status(hc, HSS_SENDING);
				}
			}
		/* hc->sc.bytes_sent is used in the throughput and time remaining
		 * calculations. It is always ahead of the number of characters
		 * actually transmitted because of buffering. To keep it from
		 * being too far ahead, we add in only half of the buffer about
		 * to be queued for transmission. Since a buffer-full may span
		 * more than one file, don't let value go negative
		 */
		if ((hc->sc.bytes_sent = hc->h_filebytes - (bsize / 2)) < 0)
			hc->sc.bytes_sent = 0;
		hc->hsxb.bptr = hc->hsxb.altbufr;
		hc->hsxb.altbufr = hc->hsxb.curbufr;
		hc->hsxb.curbufr = hc->hsxb.bptr;
#if defined(FINETUNE)
		hc->hsxb.cnt = usr_hsxb_cycle;
#else
		hc->hsxb.cnt = HSXB_CYCLE;
#endif
		hc->hsxb.total = hc->hsxb.bufrsize - hc->hsxb.cnt;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_xbclear
 *
 * DESCRIPTION:
 *	Clears the dual buffering system used in HyperProtocol. Any buffer being
 *	transitted is cut off and the current fill buffer is emptied.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void hs_xbclear(struct s_hc *hc)
	{
	hc->hsxb.bptr = hc->hsxb.curbufr;
#if defined(FINETUNE)
	hc->hsxb.cnt = usr_hsxb_cycle;
#else
	hc->hsxb.cnt = HSXB_CYCLE;
#endif
	hc->hsxb.total = hc->hsxb.bufrsize - hc->hsxb.cnt;

	// xmit_count = 0;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hsdsp_progress
 *
 * DESCRIPTION:
 *	Updates display fields on screen to indicate the progress of the transfer.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void hsdsp_progress(struct s_hc *hc, int status)
	{
	long new_stime = -1;
	long new_ttime = -1;
	long new_cps = -1;
	long file_so_far = -1;
	long total_so_far = -1;

	long ttime, stime;
	long bytes_sent;
	long cps;
	// int	k_sent;

	if (hc->xfertimer == -1L)
		return;

	ttime = bittest(status, TRANSFER_DONE) ?
			hc->xfertime : interval(hc->xfertimer);
	if ((stime = ttime / 10L) != hc->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		new_stime = stime;

		/* Display amount transferred */
		bytes_sent = hc->total_dsp + hc->sc.bytes_sent;

		if (!bittest(status, TRANSFER_DONE))
			file_so_far = hc->sc.bytes_sent;
		total_so_far = bytes_sent;

		/* Display throughput and time remaining */
		if ((stime > 2 ||
			 ttime > 0 && bittest(status, FILE_DONE | TRANSFER_DONE)) &&
			(cps = ((hc->total_thru + hc->sc.bytes_sent) * 10L) / ttime) > 0)
			{
			new_cps = cps;

			ttime = ((hc->sc.nbytes - bytes_sent) / cps) +
						hc->sc.nfiles - hc->current_filen;
			new_ttime = ttime;
			}
		hc->displayed_time = stime;
		}

	xferMsgProgress(hc->hSession,
					new_stime,
					new_ttime,
					new_cps,
					file_so_far,
					total_so_far);

	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hsdsp_newfile
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
void hsdsp_newfile(struct s_hc *hc, int filen, TCHAR *fname, long flength)
	{

	xferMsgNewfile(hc->hSession,
				   filen,
				   NULL,
				   fname);

	xferMsgFilesize(hc->hSession, flength);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hsdsp_compress
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

void hsdsp_compress(struct s_hc *hc, int tf)
	{

	xferMsgCompression(hc->hSession, tf);
	}

 /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hsdsp_retries
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

void hsdsp_retries(struct s_hc *hc, int t)
	{

	xferMsgErrorcnt(hc->hSession, t);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hsdsp_status
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

void hsdsp_status(struct s_hc *hc, int s)
	{

	xferMsgStatus(hc->hSession, s);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hsdsp_event
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

void hsdsp_event(struct s_hc *hc, int e)
	{

	xferMsgEvent(hc->hSession, e);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hpr_id_get
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
void hpr_id_get(struct s_hc *hc, BYTE *dst)
	{
	wsprintf(dst, "V%u,%s", 100, (BYTE *)"HyperTerm by Hilgraeve, Inc.");
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hpr_id_check
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
int hpr_id_check(struct s_hc *hc, int rev, BYTE *name)
	{
	/* no restrictions on who we'll talk to */
	rev = rev;			  /* keep compiler and lint from complaining */
	name = name;

	return TRUE;
	}
