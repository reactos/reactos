/* File: C:\WACKER\xfer\hpr_snd0.c (Created: 25-Jan-1994)
 * created from HAWIN source file
 * hpr_snd0.c -- Routines to provide HyperProtocol file send function in
 *			  HyperACCESS.
 *
 *	Copyright 1988,89,1994 by Hilgraeve Inc. -- Monroe, MI
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
#include <tdll\com.h>
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hpr_snd
 *
 * DESCRIPTION:
 *	Handles file sending using the Hyperprotocol transfer method.
 *
 * ARGUMENTS:
 *	attended --  True if the program determines that a user is likely to be
 *				 present at the computer keyboard. FALSE if a user is NOT likely
 *				 to be present (such as host and script modes).
 *	hs_nfiles -- The number of files scheduled to be sent. (Other concurrent
 *				 processes may remove files before we get to them so we may
 *				 not actually send this many files.)
 *	hs_nbytes -- The total size of all files scheduled to be sent. (See note
 *				 in description of hs_nfiles.)
 *
 * RETURNS:
 *	TRUE if the transfer successfully completes. FALSE otherwise.
 */
int hpr_snd(HSESSION hSession, int attended, int hs_nfiles, long hs_nbytes)
	{
	struct s_hc *hc;
	HCOM			  hCom;
	int 			  bailout;
	long			  timer;
	register int cc;

	hCom = sessQueryComHdl(hSession);

	hc = malloc(sizeof(struct s_hc));
	if (hc == NULL)
		return TSC_NO_MEM;

	memset(hc, 0, sizeof(struct s_hc));

	hc->hSession = hSession;

	/* initialize stuff */
	if (!hs_setup(hc, hs_nfiles, hs_nbytes))
		{
		free(hc);
		return TSC_NO_MEM;
		}

	/* force a file break on the next char. fetch */
	hc->sc.hs_ptrgetc = hs_reteof;

	/* initialize variables etc. */
	hc->h_filebytes = 0L;
	// hc->blocksize = 2048;
	// hc->blocksize = xfer_blocksize(hSession);
	hc->blocksize = max(hc->blocksize, H_MINBLOCK);
	hc->current_filen = 0;
	hc->deadmantime = 600;

	hc->total_tries = 0;
	hsdsp_retries(hc, hc->total_tries);

	hc->total_dsp = 0L;
	hc->total_thru = 0L;
	hc->ucancel = FALSE;
	hc->usecrc = TRUE;			/* to start out */
	hc->fhdl = NULL;

	hc->sc.nfiles = hs_nfiles;
	hc->sc.nbytes = hs_nbytes;
	hc->sc.bytes_sent = hc->h_filebytes;
	hc->sc.rmtcancel = FALSE;
	hc->sc.last_response = startinterval();
	hc->sc.lastmsgn = -1;
	hc->sc.rmt_compress = FALSE;		/* until we hear otherwise */
	hc->sc.rmtchkt = H_CRC;			/* until we hear otherwise */
	hc->sc.started = FALSE;
	hc->sc.lasterr_filenum = -1;
	hc->sc.lasterr_offset = -1L;
	hc->sc.sameplace = 0;

	/* setup file table */
	hc->sc.ft_current = 0;
	hc->sc.ft_top = hc->sc.ft_open = 0;

	hc->sc.hs_ftbl[0].filen = 0;
	hc->sc.hs_ftbl[0].cntrl = 0;
	hc->sc.hs_ftbl[0].status = TSC_OK;
	hc->sc.hs_ftbl[0].flength = 0L;
	hc->sc.hs_ftbl[0].thru_bytes = 0L;
	hc->sc.hs_ftbl[0].dsp_bytes = 0L;

	hc->sc.hs_ftbl[0].fname[0] = '\0';
	// strblank(hc->sc.hs_ftbl[0].fname);

	omsg_init(hc, FALSE, TRUE);

	if ((bailout = setjmp(hc->sc.jb_bailout)) == 0)
		{
		/* do normal transfer */

		// RemoteClear();  /* get rid of possible stacked-up starts */
		ComRcvBufrClear(hCom);

		/* restarts and file aborts will branch here */
		if (setjmp(hc->sc.jb_restart) == 0)
			{
			/* first time through, wait for receiver to begin */
			hsdsp_status(hc, HSS_WAITSTART);
			timer = startinterval();
#if defined(NOTIMEOUTS)
			while (TRUE)
#else
			while (interval(timer) < 600)
#endif
				{
				/* prevent deadman timeout */
				hc->xfertimer = hc->sc.last_response = startinterval();
				hs_background(hc);	/* will exit with longjump to restart */
				if (hc->ucancel)
					 longjmp(hc->sc.jb_bailout, TSC_USER_CANNED);

				xfer_idle(hc->hSession);

				}
			hsdsp_event(hc, HSE_NORESP);
			longjmp(hc->sc.jb_bailout, TSC_NO_RESPONSE);
			}

		/* Restarts jump here via longjmp */
		hsdsp_status(hc, HSS_SENDING);
		hsdsp_progress(hc, 0);
		for (;;)/* this loop is only exited by a longjmp to hc->sc.jb_bailout */
			{	/*	  or hc->sc.jb_restart									*/
			hs_background(hc);
			if (hs_datasend(hc))
				{
				/* sent out full data block */
				cc = omsg_setnum(hc, (omsg_number(hc) + 1) % 256);
				hc->h_checksum += (unsigned)cc;
				if (hc->usecrc)
					h_crc_calc(hc, (BYTE)cc);
				HS_XMIT(hc, (BYTE)cc);
				HS_XMIT(hc, (BYTE)((hc->usecrc?hc->h_crc:hc->h_checksum)%256));
				HS_XMIT(hc, (BYTE)((hc->usecrc?hc->h_crc:hc->h_checksum)/256));
				/* update display */
				hsdsp_progress(hc, 0);
				hc->datacnt = 0;
				hc->h_crc = hc->h_checksum = 0;
				}
			else	/* encountered EOF in data */
				{
				if (hc->h_filebytes > 0)	/* EOF probably got counted */
					--hc->h_filebytes;
				hc->sc.bytes_sent = hc->h_filebytes;
				hsdsp_progress(hc, FILE_DONE);
				hs_filebreak(hc, hs_nfiles, hs_nbytes);
				}

			xfer_idle(hc->hSession);

			}
		}
	else	/* a longjmp(hc->sc.jb_bailout, reason) was called */
		{
		if (bailout == TSC_COMPLETE)
			{
			bailout = TSC_OK;
			}
		else
			{
			hc->xfertime = interval(hc->xfertimer);
			if (hc->fhdl)
				{
				hc->sc.hs_ftbl[hc->sc.ft_current].status = (int)bailout;
				fio_close(hc->fhdl);
				hc->fhdl = NULL;
				}
			}
		hsdsp_status(hc, bailout == TSC_OK ? HSS_COMPLETE : HSS_CANCELLED);
		}

	/* cleanup and exit */;
	/* make sure final vu meter is displayed full */
	hc->total_dsp += hc->sc.hs_ftbl[hc->sc.ft_current].flength;
	hc->total_thru += hc->h_filebytes;
	hc->h_filebytes = hc->sc.bytes_sent = 0L;
	hsdsp_progress(hc, TRANSFER_DONE);

	hs_logx(hc, TRUE);
	hs_wrapup(hc, attended, bailout);
	free(hc);
	compress_disable();

	return((USHORT)bailout);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_dorestart
 *
 * DESCRIPTION:
 *	Called when an interrupt message from the receiver requests that
 *	transmission be restarted from a particular location. (The entire
 *	transfer is started by an initial request from the receiver to restart
 *	at file 0, offset 0.
 *	This routine responds to a restart request by interrupting any current
 *	data being sent and sending a special restart message. It then sets up
 *	to resume transmission at a new location
 *
 * ARGUMENTS:
 *	filenum -- The file number of the file to restart.
 *	offset	-- The offset into the file specified by filenum.
 *	msgnum	-- The message number of the restart request. This number is
 *				included in the restart message sent back so the receiver
 *				will know which restart request it corresponds to.
 *	abort	-- True if the restart is the result of a request by the
 *				receiving program to abort transfer of the current file.
 *
 * RETURNS:
 *	nothing (This function always exits via a longjmp to the restart location
 *				or to the bailout location)
 */
void hs_dorestart(struct s_hc *hc, int filenum, long offset, int msgnum, int abort)
	{
	char str[20];
	register struct s_ftbl FAR *ft;
	int cnt;
	long fsize;

	HS_XMIT_CLEAR(hc);	/* clear any pending data waiting for transmission */
	omsg_setnum(hc, -1);/* restart numbering */
	omsg_new(hc, 'R');	/* start a special 'R'estart message */
#if FALSE
	hc->usecrc = (h_chkt == H_CRC || hc->sc.rmtchkt == H_CRC);
#endif

	hc->usecrc = ((hc->h_chkt == H_CRC) || (hc->sc.rmtchkt == H_CRC));

	wsprintf(str, "R%c;T%d", tochar(msgnum), hc->usecrc ? H_CRC : H_CHECKSUM);
	omsg_add(hc, str);

	/* if we're backing up to an earlier point, tell receiver what blocksize
	 *	to use and verify the file number and offset
	 */
	if (!abort && filenum > 0)
		{
		/* since the transfer is not error-free, retreat to min. blocksize */
		hc->blocksize = H_MINBLOCK;
		// StrFmt(str, "B%d;f%d;o%lu", hc->blocksize, filenum, offset);
		wsprintf(str, "B%d;f%d;o%lu", hc->blocksize, filenum, offset);
		omsg_add(hc, str);
		}
	omsg_send(hc, 1, TRUE, FALSE);

	/* find the requested file in the file table */
	while(hc->sc.ft_current >= 0)
		{
		if (hc->sc.hs_ftbl[hc->sc.ft_current].filen == filenum)
			break;
		--hc->sc.ft_current;
		}

	/* if we couldn't find the requested file in the file table it's
		Trouble with a capital 'T' */
	if (hc->sc.ft_current < 0)
		{
		hsdsp_event(hc, HSE_ILLEGAL);
		longjmp(hc->sc.jb_bailout, TSC_BAD_FORMAT);
		}

	ft = &hc->sc.hs_ftbl[hc->sc.ft_current]; /* set local ptr. into table for speed */
	hc->current_filen = filenum;

	/* Check that we're not stuck on an unresolvable problem. Twenty-five
	 *	consecutive requests to restart at the same place means the transfer
	 *	isn't ever likely to succeed. Probably caused by something like
	 *	an intermediate device stripping characters out.
	 */
	if (hc->sc.lasterr_offset == offset && hc->sc.lasterr_filenum == filenum)
		{
		if (++hc->sc.sameplace >= 25)
			{
			ft->status = TSC_ERROR_LIMIT;
			hsdsp_event(hc, HSE_ERRLIMIT);
			longjmp(hc->sc.jb_bailout, TSC_ERROR_LIMIT);
			}
		}
	else
		{
		hc->sc.lasterr_offset = offset;
		hc->sc.lasterr_filenum = filenum;
		hc->sc.sameplace = 0;
		}

	if (abort && ft->status == TSC_OK) /* aborting current file ? */
		ft->status = TSC_RMT_CANNED;

	if (hc->ucancel || hc->sc.rmtcancel)	/* received restart while trying
											to cancel */
		for (cnt = hc->sc.ft_current; cnt <= hc->sc.ft_top; ++cnt)
			{
			hc->sc.hs_ftbl[cnt].status =
					(int)(hc->ucancel ? TSC_USER_CANNED : TSC_RMT_CANNED);
			}

	/* if this isn't a simple backup operation, force the next character
	 *	read to return an EOF which will, in turn, force hs_filebreak to
	 *	be called to handle these more difficult situations
	 */
	if (filenum == 0 || ft->status != TSC_OK)
		hc->sc.hs_ptrgetc = hs_reteof;
	else
		{
		/* reopen file if necessary */
		hc->h_filebytes = hc->sc.bytes_sent = offset;
		hc->total_dsp = ft->dsp_bytes;
		hc->total_thru = ft->thru_bytes;
		if (hc->sc.ft_current != hc->sc.ft_open)
			{
			if (hc->fhdl)
				fio_close(hc->fhdl);
			hc->fhdl = NULL;
			hc->sc.ft_open = hc->sc.ft_current;
			if (xfer_opensendfile(hc->hSession, &hc->fhdl, ft->fname, &fsize,
					NULL, NULL) != 0)
				{
				ft->status = TSC_CANT_OPEN;
				hc->sc.ft_open = -1;
				}
			hsdsp_newfile(hc, filenum, ft->fname, fsize);
			}
		if (ft->status == TSC_OK)
			{
			if (fio_seek(hc->fhdl, offset, FIO_SEEK_SET) == EOF)
				ft->status = TSC_DISK_ERROR;
			else
				{
				hc->sc.hs_ptrgetc = hs_getc;

				/* restart compression if necessary */
				if (bittest(ft->cntrl, FTC_COMPRESSED))
					{
					compress_start(&hc->sc.hs_ptrgetc,
									hc,
									&hc->h_filebytes,
									FALSE);
					hsdsp_compress(hc, ON);
					hsdsp_status(hc, HSS_SENDING);
					}
				else
					hsdsp_compress(hc, OFF);
				}
			}
		}
	hc->datacnt = 0;
	hc->h_checksum = hc->h_crc = 0;
	longjmp(hc->sc.jb_restart, 1);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_background
 *
 * DESCRIPTION:
 *	Called by the main transfer loops to handle asynchronous background tasks.
 *	Specifically, the serial port is scanned for incoming messages from the
 *	receiver. This routine also detects when the receiver has been silent too
 *	long (and thus may be dead).
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void hs_background(struct s_hc *hc)
	{

	hs_rcvmsg(hc);

	if (xfer_user_interrupt(hc->hSession))
		{
		hsdsp_event(hc, HSE_USRCANCEL);
		if (hc->ucancel)		/* this is the second time */
			longjmp(hc->sc.jb_bailout, TSC_USER_CANNED);
		else
			{
			hc->ucancel = TRUE;
			/* TODO: fix this somehow */
			/* errorline(FALSE, strld(TM_WAIT_CONF)); */
			hc->sc.hs_ptrgetc = hs_reteof; /* force sending to break */
			}
		}

	if (xfer_carrier_lost(hc->hSession))
		longjmp(hc->sc.jb_bailout, TSC_LOST_CARRIER);

#if !defined(NOTIMEOUTS)
	if ((long)interval(hc->sc.last_response) > (hc->deadmantime * 2 + 100))
		{
		hsdsp_event(hc, HSE_NORESP);
		longjmp(hc->sc.jb_bailout, TSC_NO_RESPONSE);
		}
#endif

	xfer_idle(hc->hSession);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_rcvmsg
 *
 * DESCRIPTION:
 *	Called by hs_background to collect messages from the receiver piecemeal
 *	and call hs_decode_rmsg when a complete message is assembled.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
#define IDLE			-1
#define AWAITING_TYPE	-2
#define AWAITING_LEN	-3

void hs_rcvmsg(struct s_hc *hc)
	{
	HCOM hCom;
	TCHAR cc;
	unsigned rm_checksum;
	BYTE *sp;

	hCom = sessQueryComHdl(hc->hSession);

	// while ((cc = RemoteGet()) != -1)
	while (mComRcvChar(hCom, &cc) != 0)
		{
		if ((*hc->dptr = (BYTE)cc) == H_MSGCHAR)
			{
			/* start receiving new message, even if last one hadn't finished */
			hc->rmcnt = AWAITING_TYPE + 1;
			hc->dptr = hc->msgdata;
			}
		// TODO: figure the correct way for this - else if (!isprint(*hc->dptr))
		else if ((*hc->dptr < 0x20) || (*hc->dptr > 0x7E))
			/* ignore other non-printing characters */;
		else
			{
			switch(--hc->rmcnt)
				{
			case IDLE:
				++hc->rmcnt;	/* so we'll still be idle next time */
				break;
			case AWAITING_TYPE:
				++hc->dptr; 	/* keep type char. */
				hc->rmcnt = AWAITING_LEN + 1;
				break;
			case AWAITING_LEN:
				if ((hc->rmcnt = unchar(*hc->dptr)) < 3 || hc->rmcnt > 94)
					{
					hc->rmcnt = IDLE + 1;
					hc->dptr = hc->msgdata;
					}
				else
					++hc->dptr;
				break;
			default:
				++hc->dptr; 	/* just keep the char. */
				break;
			case 0: 		/* just received final char. of message */
				hc->rmcnt = IDLE + 1;	/* start over with next char. */
				/* verify the checksum */
				--hc->dptr; 			/* point to first char of checksum */
				rm_checksum = 0;
				for (sp = hc->msgdata; sp < hc->dptr; ++sp)
					rm_checksum += *sp;
				/* sp now points to first check char */
				hc->dptr = hc->msgdata;
				if (*sp == tochar(rm_checksum & 0x3F) &&
						*(sp + 1) == tochar((rm_checksum >> 6) & 0x3F))
					{
					/* received valid message */
					hc->sc.last_response = startinterval();
					if (unchar(hc->msgdata[2]) != (BYTE)hc->sc.lastmsgn)
						{
						/* new message */
						*sp = '\0';
						hc->sc.lastmsgn = unchar(hc->msgdata[2]);
						hs_decode_rmsg(hc, hc->msgdata);
						}
					}
				break;
				}
			}

		xfer_idle(hc->hSession);

		}

	xfer_idle(hc->hSession);

	}

/************************* end of hpr_snd0.c ****************************/
