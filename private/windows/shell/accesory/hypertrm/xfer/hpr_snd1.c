/* File: C:\WACKER\xfer\hpr_snd1.c (Created: 26-Jan-1994)
 * created from HAWIN source file
 * hpr_snd1.c -- Routines to provide HyperProtocol file send function in
 *			  HyperACCESS.
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>
#include <term\res.h>
#include <sys\types.h>
#include <sys\utime.h>

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\load_res.h>
#include <tdll\xfer_msc.h>
#include <tdll\globals.h>
#include <tdll\file_io.h>

#if !defined(BYTE)
#define	BYTE	unsigned char
#endif

#include "itime.h"
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
 * hs_namecheck
 *
 * DESCRIPTION:
 *	This function is called to check the extension on a filename and to guess
 *	if it should be compressed or not.
 *
 * ARGUEMENTS:
 *	ft -- struct s_ftbl *, contains the name and gets the flag value
 *
 * RETURNS:
 *	Nothing, but may set flags in the structure passed as arguement
 */
void hs_namecheck(struct s_ftbl *ft)
	{
	BYTE *ptr;

	if (strlen(ft->fname) < 5)
		return;

	ptr = ft->fname + strlen(ft->fname) - 4;
	if (*ptr++ != '.')
		return;

	/* the extensions we are currently checking for are:
	 *	ARC
	 *	LZH
	 *	PAK
	 *	ZIP
	 *	ZOO
	 */
#if FALSE
	/* TODO: replace with something else ANSI compatible */
	if ((strnicmp(ptr, "ARC", 3) == 0) ||
		(strnicmp(ptr, "LZH", 3) == 0) ||
		(strnicmp(ptr, "PAK", 3) == 0) ||
		(strnicmp(ptr, "ZIP", 3) == 0) ||
		(strnicmp(ptr, "ZOO", 3) == 0))
			bitset(ft->cntrl, FTC_DONT_CMPRS);
#endif

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_filebreak
 *
 * DESCRIPTION:
 *	Handles changes to the currently open file. This routine is called whenever
 *	the data loading routine gets an EOF while attempting to load the next
 *	data character. This can happen when the program first starts or when
 *	a file is exhausted or when a file error is encountered. It can also
 *	occur artificially when, for example, a restart is requested to force
 *	a change in file or file position.
 *
 * ARGUMENTS:
 *	nfiles -- Total number of files expected to be sent during this transfer.
 *				This is only used in the initial (file 0) control message.
 *	nbytes -- Total number of bytes expected to be sent during this transfer.
 *				This is only used in the initial (file 0) control message.
 *
 * RETURNS:
 *	nothing
 */
void hs_filebreak(struct s_hc *hc, int nfiles, long nbytes)
	{
	BYTE str[90];
	BYTE name_to_send[FNAME_LEN];
	struct s_ftbl *ft;

	/* we've either hit a real EOF, or the restart routine
		has backed us up into an aborted file, or we're just
		starting, or we've encountered a file error, or user cancelled.
	*/
	omsg_new(hc, 'C');
	if (hc->datacnt > 0)
		{
		/* put partial block record in message */
		wsprintf(str, "P%u,%u", hc->datacnt,
				(hc->usecrc ? hc->h_crc : hc->h_checksum));
		omsg_add(hc, str);
		hc->datacnt = hc->h_crc = hc->h_checksum = 0;
		}

	if (hc->ucancel || hc->sc.rmtcancel)
		{
		hpr_id_get(hc, str);
		omsg_add(hc, str);
		omsg_add(hc, "X");
		hs_waitack(hc);
		}

	if (hc->current_filen == 0)
		{
		/* just getting started or need to start over */
		/* put 'V', 'B', 'D', 'N', and 'S' records in message */
		hpr_id_get(hc, str);	/* add 'V' field */
		omsg_add(hc, str);
		wsprintf(str, "B%d;D%d;N%u;S%lu", hc->blocksize,
				(int)(hc->deadmantime / 10), nfiles, nbytes);
		omsg_add(hc, str);
		}
	else
		{
#if FALSE
		/* TODO: put this back in later */
		if (fio_error(hc->fhdl))
			{
			/* print error message on the screen */
			hc->sc.hs_ftbl[hc->sc.ft_current].status = TSC_DISK_ERROR;
			}
#endif
		if (hc->sc.hs_ftbl[hc->sc.ft_current].status != TSC_OK)
			{
			strcpy(str, "A");
			omsg_add(hc, str);
			}
		}

	/* scan up the file table looking for first non-cancelled file or
	 *	for top of table
	 */
	while (++hc->sc.ft_current <= hc->sc.ft_top
			&& hc->sc.hs_ftbl[hc->sc.ft_current].status != TSC_OK)
		{
		xfer_name_to_send(hc->hSession,
						hc->sc.hs_ftbl[hc->sc.ft_current].fname,
						name_to_send);
		wsprintf(str, "F%u,%s",
				hc->current_filen = hc->sc.hs_ftbl[hc->sc.ft_current].filen,
				name_to_send);
		omsg_add(hc, str);
		/* put size field in so rcvr can keep display accurate */
		wsprintf(str, "s%lu", hc->sc.hs_ftbl[hc->sc.ft_current].flength);
		omsg_add(hc, str);
		omsg_send(hc, 1, hc->usecrc, FALSE);
		/* now that we've started this file, tell receiver to abort it
		 *	right away since its status is no longer TSC_OK
		 */
		omsg_new(hc, 'C');
		strcpy(str, "A");
		omsg_add(hc, str);
		}

	/* Now we should be ready for next file. We're either pointing at a
	 *	normal file in the table to be restarted or we're at the top
	 *	of the table and its time to open the next file
	 */
	hc->sc.hs_ptrgetc = hs_getc;
	/* set pointer into table for speed */
	ft = &hc->sc.hs_ftbl[hc->sc.ft_current];
	if (hc->sc.ft_current <= hc->sc.ft_top)
		{
		/* try to reopen a file from the table */
		hc->current_filen = ft->filen;
		if (hc->fhdl != NULL && hc->sc.ft_current == hc->sc.ft_open)
			{
			/* file is already open */
			fio_seek(hc->fhdl, 0, FIO_SEEK_SET);
			}
		else
			{
			/* reopen previously opened file */
			if (hc->fhdl)
				{
				fio_close(hc->fhdl);
				}
			hc->fhdl = NULL;
			hc->sc.ft_open = hc->sc.ft_current;
			hc->current_filen = ft->filen;
			hc->h_filebytes = hc->sc.bytes_sent = 0L;
			hc->total_dsp = ft->dsp_bytes;
			hc->total_thru = ft->thru_bytes;
			if (xfer_opensendfile(hc->hSession, &hc->fhdl, ft->fname,
					&ft->flength, NULL, NULL) != 0)
				{
				/* display error */
				ft->status = TSC_CANT_OPEN;
				hc->sc.hs_ptrgetc = hs_reteof;
				hc->sc.ft_open = -1;
				}
			}
		}

	else if (xfer_nextfile(hc->hSession, name_to_send))
		{
		/* There are no more previously started files in the table to be
		 *	restarted, but there is another brand new file to send
		 */
		if (hc->sc.ft_top >= hc->sc.ft_limit)
			{
			hsdsp_event(hc, HSE_FULL);
			hsdsp_status(hc, HSS_WAITACK);
			omsg_add(hc, "I");
			omsg_send(hc, 1, hc->usecrc, FALSE);
			omsg_new(hc, 'C');
			HS_XMIT_FLUSH(hc);
			while(hc->sc.ft_top >= hc->sc.ft_limit)
				{
				hs_background(hc);	/* wait for space in table */
				hs_logx(hc, FALSE);
				}
			hsdsp_event(hc, HSE_GOTACK);
			hsdsp_status(hc, HSS_SENDING);
			/* items may shift in hs_ftbl during hs_background */
			ft = &hc->sc.hs_ftbl[hc->sc.ft_current];
			}

		/* open next file and install it in the table */
		if (hc->fhdl)
			{
			fio_close(hc->fhdl);
			}
		hc->fhdl = NULL;
		strcpy(ft->fname, name_to_send);

		ft->filen = ++hc->current_filen;
		ft->cntrl = 0;
		ft->status = TSC_OK;
		hs_namecheck(ft);
		hc->total_dsp = ft->dsp_bytes =
								hc->sc.hs_ftbl[hc->sc.ft_top].dsp_bytes +
								hc->sc.hs_ftbl[hc->sc.ft_top].flength;
		hc->total_thru = ft->thru_bytes =
				hc->sc.hs_ftbl[hc->sc.ft_top].thru_bytes + hc->h_filebytes;

		hc->sc.ft_top = hc->sc.ft_open = hc->sc.ft_current;

		hc->h_filebytes = hc->sc.bytes_sent = 0;
		if (xfer_opensendfile(hc->hSession, &hc->fhdl, ft->fname, &ft->flength,
				NULL, NULL) != 0)
			{
			/* display error? */
			hc->fhdl = NULL;
			ft->status = TSC_CANT_OPEN;
			hc->sc.ft_open = -1;
			hc->sc.hs_ptrgetc = hs_reteof;
			}
		}
	else	/* no more files in table and no new files to send */
		{
		if (hc->fhdl)
			{
			fio_close(hc->fhdl);
			}
		hc->sc.ft_open = -1;
		hc->fhdl = NULL;
		--hc->sc.ft_current;	/* don't point off top of table */
		hsdsp_event(hc, HSE_DONE);
		omsg_add(hc, "E");
		hs_waitack(hc);
		}

	/* put file name and attribute records in message */
	xfer_name_to_send(hc->hSession, ft->fname, name_to_send);
	wsprintf(str, "F%d,%s", ft->filen, name_to_send);
	omsg_add(hc, str);
	wsprintf(str, "s%lu", ft->flength);
	omsg_add(hc, str);

	if (hc->fhdl)
		{
		unsigned long ftime;
		struct tm *pT;

		ftime = itimeGetFileTime(ft->fname);
		ftime += itimeGetBasetime();		/* fudge for C 7 and later */
		pT = localtime(&ftime);
		wsprintf(str, "t%d,%d,%d,%d,%d,%d",
				pT->tm_year + 1900,
				pT->tm_mon,
				pT->tm_mday,
				pT->tm_hour,
				pT->tm_min,
				pT->tm_sec);
		omsg_add(hc, str);

#if FALSE
		if (hc->sc.rmt_compress && h_trycompress &&
				!bittest(ft->cntrl, FTC_DONT_CMPRS) &&
				ft->flength > CMPRS_MINSIZE &&
				compress_start(&hc->sc.hs_ptrgetc, &h_filebytes, FALSE))
#endif
		if (hc->sc.rmt_compress && hc->h_trycompress &&
				!bittest(ft->cntrl, FTC_DONT_CMPRS) &&
				ft->flength > CMPRS_MINSIZE &&
				compress_start(&hc->sc.hs_ptrgetc, hc, &hc->h_filebytes, FALSE))
			{
			hsdsp_compress(hc, ON);
			/* hsdsp_status(hc, HSS_SENDING); */
			omsg_add(hc, "C");
			bitset(ft->cntrl, FTC_COMPRESSED);
			}
		else
			{
			hsdsp_compress(hc, OFF);
			bitclear(ft->cntrl, FTC_COMPRESSED);
			}
		}
	omsg_send(hc, 1, hc->usecrc, FALSE);

	HS_XMIT_FLUSH(hc);
	hsdsp_newfile(hc, ft->filen, ft->fname, ft->flength);
	hsdsp_progress(hc, 0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_waitack
 *
 * DESCRIPTION:
 *	Called at the end of a transfer to wait for the final acknowldgement from
 *	receiver or until a restart message forces us to back up and retransmit
 *	some of the data. This routine never returns directly to its caller. It
 *	always exits by way of a longjmp either during a hs_background call if
 *	an 'END' or 'RESTART' message is received or at the end of the function
 *	if it times out without receiving either message.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing (see DESCRIPTION)
 */
void hs_waitack(struct s_hc *hc)
	{
	long timer;

	omsg_send(hc, 1, hc->usecrc, FALSE);
	hc->sc.receiver_timedout = FALSE;
	HS_XMIT_FLUSH(hc);
	hsdsp_status(hc, HSS_WAITACK);
	timer = startinterval();


	/* wait for 60 seconds, until longjmp either restarts or ends transfer */
	while (interval(timer) < FINAL_ACK_WAIT)
		{
		hs_background(hc);
		if (hc->sc.receiver_timedout)
			{
			/* receiver must have missed our end of transfer message */
			omsg_send(hc, 1, hc->usecrc, FALSE);
			HS_XMIT_FLUSH(hc);
			hc->sc.receiver_timedout = FALSE;
			}
		}
	/* if we get here, we didn't get response from receiver for 60 seconds */
	hsdsp_event(hc, HSE_NORESP);
	longjmp(hc->sc.jb_bailout, TSC_NO_RESPONSE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_decode_rmsg
 *
 * DESCRIPTION:
 *	Called by hs_rcvmsg when a complete message has been received. This routine
 *	parses and interprets the message either by recording information sent by
 *	the receiver or by altering the course of the transfer in response to an
 *	interruption message. There may be several fields in a message but note
 *	that certain messages transfer control elsewhere via a longjmp and thus
 *	prevent any subsequent fields from being scanned ('E', 'X', and 'R'
 *	messages specifically). All messages from the receiver to the sender are
 *	sent in printable characters.
 *
 * ARGUMENTS:
 *	data -- A string containing the messages to be interpreted. Each message
 *			is in the form of: 'T<data>;'
 *			where T is a single character message identifier and <data> is
 *			optional associated information.
 *
 * RETURNS:
 *	nothing
 *	(exits via a longjmp when certain interruption messages are received)
 */
void hs_decode_rmsg(struct s_hc *hc, BYTE *data)
	{
	BYTE *field;
	int filenum;
	int mnum;
	int tval;

	if (data[0] != 'c') /* this is the only kind we know about now but don't */
		return; 		/*	consider it an error if newer versions send 	 */
						/*	other types */

	mnum = unchar(data[2]);
	field = strtok(&data[3], ";");
	while (field != NULL)
		{
		switch(*field++)
			{
		case 'f':		/* receiver has successfully received file fn */
			hs_fileack(hc, atoi(field));
			break;
		case 'B':		/* receiver is requesting a particular block size */
			if (hc->current_filen == 0)
				{
				tval = atoi(field);
				tval = max(tval, H_MINBLOCK);
				hc->blocksize = min(hc->blocksize, tval);
				}
			break;
		case 'D':		/* receiver is requesting a particular deadman time */
			tval = atoi(field);
			tval = max(tval, H_MINDEADMAN);
			hc->deadmantime = min(hc->deadmantime, tval * 10);
			break;
		case 'T':		/* receiver is requesting check type */
			hc->sc.rmtchkt = atoi(field);
			break;
		case 'C':		/* receiver is agreeing to do compression */
			if (!*field)
				hc->sc.rmt_compress = TRUE;
			break;
		case 'E':		/* receiver is acknowledging end of transfer */
			hc->xfertime = interval(hc->xfertimer);
			if (*field)
				hs_fileack(hc, atoi(field));
			hsdsp_event(hc, HSE_GOTACK);
			if (!hc->ucancel && !hc->sc.rmtcancel)
				longjmp(hc->sc.jb_bailout, TSC_COMPLETE);
			else
				longjmp(hc->sc.jb_bailout,
						hc->ucancel ? TSC_USER_CANNED : TSC_RMT_CANNED);
			break;
		case 'X':		/* receiver is requesting a cancellation of the xfer */
			if (*field)
				hs_fileack(hc, atoi(field));
			hsdsp_event(hc, HSE_RMTCANCEL);
			hc->sc.rmtcancel = TRUE;
			hc->sc.hs_ptrgetc = hs_reteof;
			break;
		case 'A':		/* receiver is aborting a single file */
			hs_fileack(hc, filenum = atoi(field));  /* all earlier files ok */

			/* If we're still sending the file being cancelled, mark it
			 * cancelled and set the character function pointer to hs_reteof
			 * to force a switch to the next file, if any.
			 * If we've already finished sending the file being cancelled,
			 * look it up in the file table and mark if cancelled so that
			 * if we're asked to back up into it, we won't bother resending
			 * it
			 */
			if (hc->current_filen == filenum)
				{
				hc->sc.hs_ftbl[hc->sc.ft_current].status = TSC_RMT_CANNED;
				hc->sc.hs_ptrgetc = hs_reteof; /* force switch to next file */
				}
			else
				{
				/* we're no longer sending file being canned */
				for (tval = hc->sc.ft_current; tval >= 0; --tval)
					if (hc->sc.hs_ftbl[tval].filen == filenum)
						hc->sc.hs_ftbl[tval].status = TSC_RMT_CANNED;
				}
			break;

		case 'V':
			tval = atoi(field);
			field = strchr(field, ',');

			/* if version restrictions were found, stop the transfer */
			if (!hpr_id_check(hc, tval, ++field))
				{
				hsdsp_event(hc, HSE_RMTCANCEL);
				hc->sc.rmtcancel = TRUE;
				hc->sc.hs_ptrgetc = hs_reteof;
				}
			break;

		case 'R':		/* receiver is requesting a restart */
			if (!hc->sc.started)
				{
				hsdsp_event(hc, HSE_GOTSTART);
				hc->sc.started = TRUE;
				}
			else
				{
				hsdsp_event(hc, HSE_GOTRETRY);
				hsdsp_retries(hc, ++hc->total_tries);
				}
			tval = atoi(strtok(field, ","));   /* get file number */
			hs_fileack(hc, tval);
			hs_dorestart(hc, tval, atoi(strtok(NULL, ",")), mnum, FALSE);
			break;

		case 't':		/* receiver stopped receiving data */
			hc->sc.receiver_timedout = TRUE;
			break;
			}
		field = strtok(NULL, ";");
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_fileack
 *
 * DESCRIPTION:
 *	Marks all transmitted files up to a specified file number as acknowledged,
 *	that is, as transmitted completely. This routine merely marks the files
 *	in the file table as complete; completed entries will be removed from the
 *	table later by hs_logx.
 *
 * ARGUMENTS:
 *	n -- The file number that the receiver considers current. All file numbers
 *		 lower than this can be considered complete.
 *
 * RETURNS:
 *	nothing
 */
void hs_fileack(struct s_hc *hc, int n)
	{
	register int ix = 0;

	while (ix <= hc->sc.ft_current)
		{
		if (ix <= hc->sc.ft_limit && hc->sc.hs_ftbl[ix].filen < n)
			bitset(hc->sc.hs_ftbl[ix].cntrl, FTC_CONFIRMED);
		++ix;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_logx
 *
 * DESCRIPTION:
 *	Logs either all acknowledged transfers or all transfers to the log file
 *	and removes their entries from the file table. Note that the values of
 *	hc->sc.ft_top, hc->sc.ft_current, and hc->sc.ft_open can change during this
 *	operation.
 *
 * ARGUMENTS:
 *	all -- if TRUE, log all regardless of confirmation
 *
 * RETURNS:
 *	nothing
 */
void hs_logx(struct s_hc *hc, int all)
	{
	struct s_ftbl *ftptr = &hc->sc.hs_ftbl[0];

	while (hc->sc.ft_top >= 0 && (all ||
			(hc->sc.ft_current > 1 && bittest(hc->sc.hs_ftbl[0].cntrl,FTC_CONFIRMED))))
		{
		if (ftptr->filen > 0)
			{
			if (ftptr->status == TSC_OK &&
					!bittest(ftptr->cntrl,FTC_CONFIRMED))
				ftptr->status = TSC_ERROR_LIMIT;
			xfer_log_xfer(hc->hSession,
						TRUE,
						ftptr->fname,
						NULL,
						ftptr->status);
			}
		MemCopy((char *)&hc->sc.hs_ftbl[0], (char *)&hc->sc.hs_ftbl[1],
				(unsigned)hc->sc.ft_top * sizeof(struct s_ftbl));
		--hc->sc.ft_top;
		--hc->sc.ft_current;
		--hc->sc.ft_open;
		}
	}

/************************** end of hpr_snd1.c ************************/
