/* File: C:\WACKER\xfer\hpr_res.c (Created: 25-Jan-1995)
 * created from HAWIN source file
 * hpr_res.c -- Routines to implement HyperProtocol. These	are the routines
 *			that make character-by-character calls and must be fast
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
// #define	DEBUGSTR	1

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
 * hr_collect_data
 *
 * DESCRIPTION:
 *	 Collects and stores bytes for HyperProtocol
 *		save routine.  This routine exits when one of several conditions
 *		is detected and returns a code to indicate which condition caused
 *		it to exit. Exit conditions are decribed below.
 *	In any case, the checksum, crc, and character count variables have
 *	valid values up to the point where collection stopped.
 *
 * ARGUMENTS:
 *	charcount -- number of characters to collect
 *	docheck   -- TRUE if collected data should be subject to error checking
 *	timeout   -- idle time (in tenths of seconds) after which we should give
 *				  up and return HR_TIMEOUT
 *
 * RETURNS:
 *	A status code is returned which may be one of:
 *
 *		HR_COMPLETE -- The predetermined maximum received character count
 *					was reached.
 *		HR_BADCHECK -- All characters were received, but a checksum error
 *					was detected.
 *		HR_MESSAGE	-- The message character, ASCII 01, was detected followed
 *					by a character other than another ASCII 01. The 2nd
 *					character of the sequence is NOT removed from the buffer.
 *		HR_TIMEOUT	-- There was a period of time during which no characters
 *					were received that exceeded the predetermined limit.
 *		HR_KBDINT  -- The user typed an ESC key at the keyboard. This will only
 *					be detected by the hr_collect_data routine if there is a
 *					break in the received data.
 *		HR_FILEERR -- An error occured while storing a byte of the received
 *					data.
 *		HR_LOST_CARR -- Carrier was lost while waiting for data
 *
 */
int hr_collect_data(struct s_hc *hc, int *charcount, int docheck, long timeout)
	{
	HCOM hCom;
	register int ourcount = *charcount;
	int rcvd_msgnum = -1;
	int chkbytes_needed = 2;
	int iret;
	TCHAR cc;
	int got1 = FALSE;
	long timer;
	int result = HR_UNDECIDED;
	unsigned rcvd_checksum = 0;

	hCom = sessQueryComHdl(hc->hSession);

	for ( ; ; )
		{
		if (mComRcvChar(hCom, &cc) == 0)
			{
			timer = startinterval();
			while (mComRcvChar(hCom, &cc) == 0) /* while no chars */
				{
				xfer_idle(hc->hSession);
				iret = xfer_user_interrupt(hc->hSession);
				if (iret == XFER_ABORT)
					{
					result = HR_KBDINT;
					break;
					}
				else if (iret == XFER_SKIP)
					{
					hr_reject_file(hc, HRE_USER_SKIP);
					}

				if (xfer_carrier_lost(hc->hSession))
					{
					result = HR_LOST_CARR;
					break;
					}

#if !defined(NOTIMEOUTS)
				if ((long)interval(timer) > timeout)
					{
					hrdsp_event(hc, HRE_TIMEOUT);
					result = HR_TIMEOUT;
					break;
					}
#endif
				}
			if (result != HR_UNDECIDED)
				break;
			}

		if ((!got1 && cc != H_MSGCHAR) || (got1 && cc == H_MSGCHAR))
			{
			got1 = FALSE;
			if (hc->usecrc)
				h_crc_calc(hc, (BYTE)cc);
			if (ourcount > 0)
				{
				hc->h_checksum += (unsigned)cc;
				if ((*hc->rc.hr_ptr_putc)(hc, cc) ==  -1 /* ERROR */)
					{
					result = decompress_error() ? HR_DCMPERR : HR_FILEERR;
					break;
					}
				else if (--ourcount <= 0 && !docheck)
					{
					result = HR_COMPLETE;
					break;
					}
				}
			else if (rcvd_msgnum == -1)
				{
				rcvd_msgnum = cc;
				hc->h_checksum += (unsigned)cc;
				}
			else
				{
				rcvd_checksum +=
						((unsigned)cc * (chkbytes_needed == 2 ? 1 : 256));
				if (--chkbytes_needed <= 0)
					{
					result = HR_COMPLETE;
					break;
					}
				}
			}
		else if (got1)	/* got1 && cc != H_MSGCHAR */
			{
			result = HR_MESSAGE;	/* leave char. in buffer */
			mComRcvBufrPutback(hCom, cc);
			break;
			}
		else			/* !got1 && cc == H_MSGCHAR */
			got1 = TRUE;
		}
	if (result == HR_COMPLETE && docheck)
		{
		if (hc->usecrc)
			result = (hc->h_crc == 0 ? HR_COMPLETE : HR_BADCHECK);
		else if (hc->h_checksum != rcvd_checksum)
			result = HR_BADCHECK;
		if (result == HR_COMPLETE)
			if (rcvd_msgnum == hc->rc.expected_msg)
				hc->rc.expected_msg = ++hc->rc.expected_msg % 256;
			else
				result = HR_LOSTDATA;
		}
	*charcount = ourcount;

	if (hc->rc.virus_detected)
		result = HR_VIRUS_FOUND;

	return(result);
	}

// extern char FAR *storageptr;	/* place to put data as we receive it */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_storedata
 *
 * DESCRIPTION:
 *	This is a little routine used by hr_collect_msg to collect the data
 *	within a message. Data is normally written directly to the receive file.
 *	This routine, however, collects it into memory.
 *
 * ARGUMENTS:
 *	c -- a character to be stored
 *
 * RETURNS:
 *	Returns the character stored.
 */
int hr_storedata(struct s_hc *hc, int c)
	{
	*hc->storageptr++ = (char)c;
	return(c);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_putc
 *
 * DESCRIPTION:
 *	This is the normal function for dispatching received characters. It
 *	is normally called through a pointer to a function. When decompression
 *	is active, the pointer be redirected to point at the decompression
 *	routine and the decompression code may then be calling this function.
 *	In either case, this function writes one character to the output file
 *	and counts it.
 *
 * ARGUMENTS:
 *	c -- the character to be written
 *
 * RETURNS:
 *	The argument.
 */
int hr_putc(struct s_hc *hc, int c)
	{
	return (fio_putc(c, hc->fhdl));
	// return (FilePutc(c, hc->fhdl));
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_putc_vir
 *
 * DESCRIPTION:
 *	This is the normal function for dispatching received characters. It
 *	is normally called through a pointer to a function. When decompression
 *	is active, the pointer be redirected to point at the decompression
 *	routine and the decompression code may then be calling this function.
 *	In either case, this function writes one character to the output file
 *	and counts it.
 *	This version also performs virus checking. If a virus is detected, the
 *	StrSrchNextChar routine will call hr_virus_detect().
 *
 * ARGUMENTS:
 *	c -- the character to be written
 *
 * RETURNS:
 *	The argument.
 */
int hr_putc_vir(struct s_hc *hc, int c)
	{
	// ++hc->h_filebytes;
	// StrSrchNextChar(hc->rc.ssmchVscan, (VOID FAR *)NULL, (UCHAR)c);
	return (fio_putc(c, hc->fhdl));
	// return (FilePutc(c, hc->fhdl));
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hr_toss
 *
 * DESCRIPTION:
 *	This function is installed in place of hr_putc during periods between when
 *	a data error has been detected and the transfer successfully resynchs so
 *	that bogus data will not be stored in the output file. It merely tosses
 *	the character.
 *
 * ARGUMENTS:
 *	c -- the received character.
 *
 * RETURNS:
 *	the argument
 */
int hr_toss(struct s_hc *hc, int c)
	{
	/* throw character away without counting it */
	return(c);
	}

// Resident routines for sending


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_datasend
 *
 * DESCRIPTION:
 *	Attempts to send enough data to complete the current data block (as
 *	defined by (hc->blocksize).
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	TRUE if all bytes were sent.
 *	FALSE if an EOF is encountered prior to the end of the block.
 */
int hs_datasend(struct s_hc *hc)
	{
	register int cc;
	register int count = hc->blocksize - hc->datacnt;

	for ( ; ; )
		{
		if ((cc = (*hc->sc.hs_ptrgetc)(hc)) == EOF)
			{
			hc->datacnt = hc->blocksize - count;
			return(FALSE);
			}
		hc->h_checksum += (unsigned)cc;
		if (hc->usecrc)
			h_crc_calc(hc, (BYTE)cc);
		HS_XMIT(hc, (BYTE)cc);
		if (--count == 0)
			{
			hc->datacnt = hc->blocksize - count;

			/* Display current compression status */
			hsdsp_compress(hc, compress_status() == COMPRESS_ACTIVE);
			return(TRUE);
			}
		}
	/*lint -unreachable*/
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_reteof
 *
 * DESCRIPTION:
 *	This function merely returns an EOF code. Since all requests for file
 *	characters are made through a pointer to a function, that pointer can
 *	be set to this function to force the next request to return an EOF.
 *	It is normally used to interrupt the transmission of data and force a
 *	call to hs_filebreak to set a new location.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	Always returns EOF
 */
int hs_reteof(struct s_hc *hc)
	{
	return(EOF);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * hs_getc
 *
 * DESCRIPTION:
 *	Fetches one character from the input file and counts it in h_filebytes.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	The character fetched.
 */
int hs_getc(struct s_hc *hc)
	{
	return(fio_getc(hc->fhdl));
	// return(FileGetc(hc->fhdl));
	}
