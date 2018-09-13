/* zmdm_rcv.c -- ZMODEM compatible file receiving routines for HyperACCESS
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>
#pragma hdrstop

//#define DEBUGSTR

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
#include <tdll\file_msc.h>
#include <tdll\tchar.h>

#define	BYTE	unsigned char

#include "itime.h"
#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "foo.h"

#include "zmodem.hh"
#include "zmodem.h"

/* * * * * * * * * * * * *
 *	Function Prototypes  *
 * * * * * * * * * * * * */

#if defined(DEBUG_DUMPPACKET)
#include <stdio.h>
FILE*   fpPacket;
#endif  // defined(DEBUG_DUMPPACKET)

int    procheader (ZC *zc, TCHAR *name);
int    putsec     (ZC *zc, BYTE *buf, int n);

int    isvalid    (TCHAR c, int base);
TCHAR *stoi       (TCHAR *ptr, int *val, int base);
TCHAR *stol       (TCHAR *ptr, long *val, int base);
int    IsAnyLower (TCHAR *s);
long   getfree    (void);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * zmdm_rcv
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
USHORT zmdm_rcv(HSESSION h, int method, int attended, int single_file)
	{
	ZC *zc;
	int still_trying = TRUE, override = FALSE;
	int nJmpVal = 0;
	int	xstatus;
	unsigned int uiOldOptions;
	int	tvar;
	TCHAR ch;
	XFR_Z_PARAMS *pZ;

#if defined(DEBUG_DUMPPACKET)
    fpPacket = fopen("zpacket.dbg", "a");
    assert(fpPacket);
    fputs("----- Starting Zmodem rcv -----\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)

	(void)&single_file;

	zc		  = NULL;

	xstatus = TSC_NO_MEM;

	zc = malloc(sizeof(struct z_mdm_cntrl));
	if (zc == NULL)
		goto done;
	memset(zc, 0, sizeof(struct z_mdm_cntrl));

	zc->nMethod = method;	// Zmodem, or Zmodem crash recovery
	// This makes it easy to override some settings when
	// Zmodem crash recovery is selected.
	if (method == XF_ZMODEM_CR)
		{
		zc->fSavePartial = TRUE;			// Leave partial files around
		zc->ulOverride   = XFR_RO_ALWAYS;	// Always overwrite existing file
		}
	else
		{
		// Use default settings otherwise.
		//
		zc->fSavePartial = xfer_save_partial(h);
		zc->ulOverride = 0;
		}

	zc->hSession  = h;
	zc->hCom      = sessQueryComHdl(h);
	zc->secbuf	  = NULL;
	zc->fname	  = NULL;
	zc->our_fname = NULL;
	zc->z_crctab  = NULL;
	zc->z_cr3tab  = NULL;

	/* allocate space for large packets since we don't necessarily know what
	 *	we'll be getting.
	 */
	zc->secbuf = malloc(1025);
	if (zc->secbuf == NULL)
		goto done;

	zc->fname = malloc(FNAME_LEN * sizeof(TCHAR));
	if (zc->fname == NULL)
		goto done;

	zc->our_fname = malloc(FNAME_LEN * sizeof(TCHAR));
	if (zc->our_fname == NULL)
		goto done;

	resLoadDataBlock(glblQueryDllHinst(),
					IDT_CSB_CRC_TABLE,
					&zc->z_crctab,
					(int *)0);
	if (zc->z_crctab == NULL)
		goto done;

	resLoadDataBlock(glblQueryDllHinst(),
					IDT_CRC_32_TAB,
					&zc->z_cr3tab,
					(int *)0);
	if (zc->z_cr3tab == NULL)
		goto done;

	xstatus = TSC_OK;

	if (xfer_set_comport(h, FALSE, &uiOldOptions) != TRUE)
		goto done;
	else
		override = TRUE;

	zmdm_retval (zc, TRUE, ZACK);

	zc->file_bytes = 0L;
	zc->real_bytes = 0L;
	zc->total_bytes = 0L;
	zc->actual_bytes = 0L;
	zc->nSkip = FALSE;
	zc->fh = NULL;
	zc->pstatus = zc->last_event = -4;
	zc->xfertimer = -1L;
	zc->nfiles = 0;
	zc->filen = 0;
	zc->filesize = -1L;
	zc->nbytes = -1L;
	zc->errors = 0;

	// Capture the current execution environment into the flagkey_buf buffer.
	// The Trow() function will later use it to restore the execution
	// environment (i.e., the state of all system registers and the instruction
	// counter.
	//
	nJmpVal = setjmp(zc->flagkey_buf);
	if (nJmpVal != 0)
		{
		if (nJmpVal == 4)
			{
			xstatus = TSC_DISK_FULL;
			zmdm_retval(zc, TRUE, ZFERR);
			}
		else
			{
			xstatus = TSC_USER_CANNED;
			zmdm_retval(zc, TRUE, ZABORT);
			}
		zc->xfertime = interval(zc->xfertimer);
		stohdr(zc, 0L);
		zshhdr(zc, ZCAN, zc->Txhdr);
		canit(zc);
		if (zc->fh)
			xfer_close_rcv_file(h,
								zc->fh,
								xstatus,
								zc->fname,
								zc->our_fname,
								zc->fSavePartial,
								zc->basesize + zc->filesize,
								0);
		zc->fh = NULL;
		// zmdm_retval(zc, TRUE, ZABORT);  jkh, 2/12/95   see above
		goto done;
		}

	pZ = (XFR_Z_PARAMS *)xfer_get_params(zc->hSession, zc->nMethod);
	zc->Zctlesc = pZ->nEscCtrlCodes;
	zc->Rxtimeout = pZ->nRetryWait;
	if (zc->Rxtimeout == 0) zc->Rxtimeout = 10;
	zc->Rxtimeout *= 10;

	zc->do_init = TRUE; // Always start right up since we may be autostarted

	zc->tryzhdrtype = ZRINIT;

	if (tryz(zc) == ZFILE)
		{
		// tzset();

		if (zc->xfertimer == (-1L))
			zc->xfertimer = startinterval();

		// Receive files with Z-modem protocol.
		//
		switch (xstatus = rzfiles(zc))
			{
			case OK:
				xstatus = TSC_OK;
				break;
			case ZABORT:
				xstatus = TSC_USER_CANNED;
				do {
					// purgeline(zc);
					ComRcvBufrClear(zc->hCom);
					Sleep(100);
				} while (mComRcvBufrPeek(zc->hCom, &ch) != 0);
				// } while (rdchk(h) != ERROR);

				/* we try to eat any characters until the other end quits */
				break;
			case RCDO:			/* Not too sure about this one */
				xstatus = TSC_LOST_CARRIER;
				break;
			case ERROR:
				canit(zc);
				/* fall through */
			case ZMDM_VIRUS:
				do {
					// purgeline(zc);
					ComRcvBufrClear(zc->hCom);
					Sleep(100);
				} while (mComRcvBufrPeek(zc->hCom, &ch) != 0);
				// } while (rdchk(zc) != ERROR);

				/* fall thru to the default case */
			default:
				xstatus = zmdm_error(zc, xstatus);
				break;
			}
		}
done:

	zmdmr_progress(zc, TRANSFER_DONE);

	if (override)
		{
		xfer_restore_comport(h, uiOldOptions);
		}

	// Fool around with the return code to get a useful status return
	if ((tvar = zmdm_retval(zc, FALSE, 0)) != ZACK)
		// Retrieve last error message
		{
		if (tvar == ZMDM_VIRUS)
			{
			do {
				// purgeline(zc);
				ComRcvBufrClear(zc->hCom);
				Sleep(100);
			} while (mComRcvBufrPeek(zc->hCom, &ch) != 0);
			// } while (rdchk(zc) != ERROR);
			}
		xstatus = zmdm_error(zc, tvar);
		}

	if (xstatus != TSC_OK)
		{
		if (zc->fh)
			{
			xfer_close_rcv_file(h,
								zc->fh,
								xstatus,
								zc->fname,
								zc->our_fname,
								zc->fSavePartial,
								zc->basesize + zc->filesize,
								0);
			}
		}

	if (zc->secbuf != NULL)
		{
		free(zc->secbuf);
		zc->secbuf = NULL;
		}
	if (zc->fname != NULL)
		{
		free(zc->fname);
		zc->fname = NULL;
		}
	if (zc->our_fname != NULL)
		{
		free(zc->our_fname);
		zc->our_fname = NULL;
		}
	if (zc->z_crctab != NULL)
		resFreeDataBlock(h, zc->z_crctab);
	if (zc->z_cr3tab)
		resFreeDataBlock(h, zc->z_crctab);
	if (zc != NULL)
		{
		free(zc);
		zc = NULL;
		}

	xferMsgClose(h);

#if defined(DEBUG_DUMPPACKET)
    fputs("------- Ending Zmodem rcv -----\n", fpPacket);
    fclose(fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)

	return((USHORT)xstatus);
	}

/*----------------------------------------------------------------------+
 | getfree - Calculates the free bytes on the current file system.
 |			 ~0 means many free bytes (unknown).
 +----------------------------------------------------------------------*/
long getfree(void)
{
	return(~0L);	/* many free bytes ... */
}

/*----------------------------------------------------------------------+
 | tryz - Initialize for ZMODEM receive attempt, try to activate ZMODEM
 |		  sender.  Handles ZSINIT frame.  Returns ZFILE if ZMODEM filename
 |		  received, -1 on error, ZCOMPL if transaction finished, else 0.
 +----------------------------------------------------------------------*/
int tryz(ZC *zc)
{
	register c, n;
	int x;
	int retrys;

	for ( n = 10; --n >= 0; )
		{
		/* Set buffer length (0) and capability flags */
		stohdr(zc, 0L);

		/* Do we need an option to set the block size ? */

		zc->Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO /* |CANBRK */ ;
		if (zc->Zctlesc)
			zc->Txhdr[ZF0] |= TESCCTL;
		if (n <= 8)
			zc->do_init = TRUE;
		if (zc->do_init)
			zshhdr(zc, zc->tryzhdrtype, zc->Txhdr);
		if (zc->tryzhdrtype == ZSKIP)	/* Don't skip too far */
			zc->tryzhdrtype = ZRINIT;	/* CAF 8-21-87 */

		retrys = 25;

again:

		switch (x = xfer_user_interrupt(zc->hSession))
			{
			case XFER_ABORT:
				zmdmr_update(zc, ZCAN);
				longjmp(zc->flagkey_buf, 1);
				break;

			case XFER_SKIP:
				/* This MUST only happen while receiving */
				stohdr(zc, zc->filesize);
#if defined(DEBUG_DUMPPACKET)
                fputs("tryz: User skipped. ZRPOS\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
				zshhdr(zc, ZRPOS, zc->Txhdr);
				zc->nSkip = TRUE;
				zc->file_bytes = zc->filesize;
				break;

			default:
				break;
			}

		if (xfer_carrier_lost(zc->hSession))
			{
			zmdm_retval(zc, TRUE, ZCARRIER_LOST);
			return ZCARRIER_LOST;
			}

		switch (zgethdr(zc, zc->Rxhdr, 'R'))
			{
			case ZRQINIT:
				zc->do_init = TRUE;
				continue;
			case ZEOF:
				zc->do_init = TRUE;
				continue;
			case TIMEOUT:
				continue;
			case ZFILE:
				zc->zconv = zc->Rxhdr[ZF0];
				zc->zmanag = zc->Rxhdr[ZF1];
				zc->ztrans = zc->Rxhdr[ZF2];
				zc->tryzhdrtype = ZRINIT;
				c = zrdata(zc, zc->secbuf, 1024);
				/* mode(3);  TODO: figure out what this was supposed to do */
				if (c == GOTCRCW)
					return ZFILE;
				if (--retrys <= 0)
					{
					zmdm_retval(zc, TRUE, ZNAK);
					return ZNAK;
					}
				zshhdr(zc, ZNAK, zc->Txhdr);
				goto again;
			case ZSINIT:
				zc->Zctlesc = TESCCTL & zc->Rxhdr[ZF0];
				if (zrdata(zc, zc->Attn, ZATTNLEN) == GOTCRCW)
					{
					stohdr(zc, 1L);
					zshhdr(zc, ZACK, zc->Txhdr);
					goto again;
					}
				if (--retrys <= 0)
					{
					zmdm_retval(zc, TRUE, ZNAK);
					return ZNAK;
					}
				zshhdr(zc, ZNAK, zc->Txhdr);
				goto again;
			case ZFREECNT:
				stohdr(zc, getfree());
				zshhdr(zc, ZACK, zc->Txhdr);
				goto again;
			case ZCOMMAND:
				zmdm_retval(zc, TRUE, ZCOMMAND);
				return ZCOMMAND;
			case ZCOMPL:
				goto again;
			default:
				zc->do_init = TRUE;
				continue;
			case ZFIN:
				zc->xfertime = interval(zc->xfertimer);
				ackbibi(zc); return ZCOMPL;
			case ZCAN:
				zmdm_retval(zc, TRUE, ZCAN);
				return ZCAN;
			}
		}
	zmdm_retval(zc, TRUE, TIMEOUT);
	return TIMEOUT;
	}

/*----------------------------------------------------------------------+
 | rzfiles - Receive 1 or more files with ZMODEM protocol.
 +----------------------------------------------------------------------*/
int rzfiles(ZC *zc)
	{
	register c, d;

	for (;;)
		{
		switch (c = rzfile(zc))
			{
			case ZEOF:
			case ZSKIP:
				switch (d = tryz(zc))
					{
					case ZCOMPL:
						return OK;
					default:
						return d;
					case ZFILE:
						break;
					}
				continue;
			default:
				return c;
			case ERROR:
				return ERROR;
			}
		}
	}

/*----------------------------------------------------------------------+
 | rzfile - Receive single file with ZMODEM protocol.
 |			NOTE: Assumes file name frame is in secbuf.
 +----------------------------------------------------------------------*/
int rzfile(ZC *zc)
	{
	register c, n;
	int x;
	int		fBlocking = FALSE;
	unsigned long	ulBlockStart = 0;

	zc->Eofseen = FALSE;

	zc->file_bytes = 0L;
	zc->real_bytes = 0L;

	if (procheader(zc, (TCHAR *)zc->secbuf) == ERROR)
		{
		return (zc->tryzhdrtype = ZSKIP);
		}

	n = 20;

	for (;;)
		{
		// If we're blocking and we've timed out, reset for another ZRPOS
		if( fBlocking && interval( ulBlockStart ) > 100 )
			fBlocking = FALSE;

		// If we're not blocking already, set up for ZRPOS
		if( ! fBlocking )
			{
			DbgOutStr( "Sending ZRPOS to %ld", zc->file_bytes, 0, 0, 0, 0 );
			stohdr(zc, zc->file_bytes);
#if defined(DEBUG_DUMPPACKET)
            fprintf(fpPacket, "rzfile: ZRPOS to %ld\n", zc->file_bytes);
#endif  // defined(DEBUG_DUMPPACKET)
			zshhdr(zc, ZRPOS, zc->Txhdr);

			fBlocking = 1;
			ulBlockStart = startinterval( );
			DbgOutStr("Now blocked at t=%lu",ulBlockStart,0,0,0,0);
			}
nxthdr:
		switch (x = xfer_user_interrupt(zc->hSession))
			{
			case XFER_ABORT:
				zmdmr_update(zc, ZCAN);
				longjmp(zc->flagkey_buf, 1);
				break;

			case XFER_SKIP:
				/* This MUST only happen while receiving */
				stohdr(zc, zc->filesize);
#if defined(DEBUG_DUMPPACKET)
                fputs("rzfile: User skipped (1). ZRPOS\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
				zshhdr(zc, ZRPOS, zc->Txhdr);
				zc->nSkip = TRUE;
				zc->file_bytes = zc->filesize;
				break;

			default:
				break;
			}

		if (xfer_carrier_lost(zc->hSession))
			{
			zmdm_retval(zc, TRUE, ZCARRIER_LOST);
			return ZCARRIER_LOST;
			}

		switch (c = zgethdr(zc, zc->Rxhdr, 'R'))
			{
			default:
				if (--n < 0)		/* A little fix from Delrina */
					{
					/* return ERROR; */
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					zmdm_retval(zc, TRUE, c);
					return c;
					}
				continue;
			case ZNAK:
			case TIMEOUT:
				if ( --n < 0)
					{
					/* return ERROR; */
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					zmdm_retval(zc, TRUE, TIMEOUT);
					return c;
					}
				continue;			/* Another fix from Delrina */
			case ZFILE:
				if( fBlocking )
                    {
					DbgOutStr( "rzfile: ZFILE && fBlocking!\n", 0, 0, 0, 0, 0 );
                    }
				else
					zrdata(zc, zc->secbuf, 1024);
				continue;
			case ZEOF:
				if (rclhdr(zc->Rxhdr) !=  zc->file_bytes)
					{
					/*
					 * Ignore eof if it's at wrong place - force
					 *  a timeout because the eof might have gone
					 *  out before we sent our zrpos.
					 */
					goto nxthdr;
					}
				if (closeit(zc))
					{
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					zc->tryzhdrtype = ZFERR;
					/* return ERROR; */
					return ZEOF;
					}
				zmdmr_progress(zc, FILE_DONE);

				return c;
			case ERROR:	/* Too much garbage in header search error */
				if ( --n < 0)
					{
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					zmdm_retval(zc, TRUE, ERROR);
					return ERROR;
					}
				if( ! fBlocking )
					zmputs(zc, zc->Attn);
				continue;
			case ZSKIP:
				closeit(zc);
				zmdmr_progress(zc, FILE_DONE);

				return c;
			case ZDATA:
				if( ! fBlocking )
					{
					if( rclhdr(zc->Rxhdr) != zc->file_bytes)
						{
						// DbgOutStr( "rzfile: ZDATA: n=%d\n", n, 0, 0, 0, 0 );
						if ( --n < 0)
							{
							DbgOutStr("ZMODEM error %s %d", TEXT(__FILE__), __LINE__,0,0,0);
#if defined(DEBUG_DUMPPACKET)
                            fprintf(fpPacket, "rzfile: ZDATA pos = 0x%08lX vs. 0x%08lX\n",
                                rclhdr(zc->Rxhdr), zc->file_bytes);
#endif  // defined(DEBUG_DUMPPACKET)
							zmdm_retval(zc, TRUE, ZBADFMT);
							return ERROR;
							}
						zmputs(zc, zc->Attn);  continue;
						}
					}
				else
					{
					// Did sender finally respond to our ZRPOS?
					if( rclhdr(zc->Rxhdr) == zc->file_bytes )
						{
						// DbgOutStr("Now unblocked after %lu t-sec\n",interval(ulBlockStart),0,0,0,0);
						fBlocking = FALSE;
						}
					else
						{
						// Read the buffer and toss it
						c = zrdata(zc, zc->secbuf, 1024);
						continue;
						}
					}
moredata:
				zmdmr_update(zc, ZDATA);

				switch (x = xfer_user_interrupt(zc->hSession))
					{
					case XFER_ABORT:
						zmdmr_update(zc, ZCAN);
						longjmp(zc->flagkey_buf, 1);
						break;

					case XFER_SKIP:
						/* This MUST only happen while receiving */
						stohdr(zc, zc->filesize);
#if defined(DEBUG_DUMPPACKET)
                        fputs("rzfile: User skipped (2). ZRPOS\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
						zshhdr(zc, ZRPOS, zc->Txhdr);
						zc->nSkip = TRUE;
						zc->file_bytes = zc->filesize;
						break;

					default:
						break;
					}

				if (xfer_carrier_lost(zc->hSession))
					{
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					zmdm_retval(zc, TRUE, ZCARRIER_LOST);
					return ZCARRIER_LOST;
					}

				switch (c = zrdata(zc, zc->secbuf, 1024))
					{
					case ZCAN:
						zmdm_retval(zc, TRUE, c);
						return c;
					case ERROR:	/* CRC error */
						if ( --n < 0)
							{
							DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
							zmdm_retval(zc, TRUE, ERROR);
							return ERROR;
							}
						zmputs(zc, zc->Attn);
						continue;
					case TIMEOUT:
						if ( --n < 0)
							{
							/* return ERROR; */
							DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
							zmdm_retval(zc, TRUE, c);
							return c;
							}
						continue;
					case GOTCRCW:
						n = 20;
						putsec(zc, zc->secbuf, zc->Rxcount);
						zc->file_bytes += zc->Rxcount;
						zc->real_bytes += zc->Rxcount;
						zmdmr_update(zc, ZMDM_ACKED);
						zmdmr_progress(zc, 0);
						stohdr(zc, zc->file_bytes);
						zshhdr(zc, ZACK, zc->Txhdr);
						sendline(zc, &zc->stP, XON);
						goto nxthdr;
					case GOTCRCQ:
						n = 20;
						putsec(zc, zc->secbuf, zc->Rxcount);
						zc->file_bytes += zc->Rxcount;
						zc->real_bytes += zc->Rxcount;
						zmdmr_update(zc, ZMDM_ACKED);
						zmdmr_progress(zc, 0);
						stohdr(zc, zc->file_bytes);
						zshhdr(zc, ZACK, zc->Txhdr);
						goto moredata;
					case GOTCRCG:
						n = 20;
						putsec(zc, zc->secbuf, zc->Rxcount);
						zc->file_bytes += zc->Rxcount;
						zc->real_bytes += zc->Rxcount;
						zmdmr_progress(zc, 0);
						goto moredata;
					case GOTCRCE:
						n = 20;
						putsec(zc, zc->secbuf, zc->Rxcount);
						zc->file_bytes += zc->Rxcount;
						zc->real_bytes += zc->Rxcount;
						zmdmr_progress(zc, 0);
						goto nxthdr;
					}
			} /* switch */
		} /* end for */
	}

/*----------------------------------------------------------------------+
 | zmputs - Send a string to the modem, processing for \336 (sleep 1 sec)
 |			and \335 (break signal).
 +----------------------------------------------------------------------*/
void zmputs(ZC *zc, char *s)
	{
	register c;

	while (*s)
		{
		switch (c = *s++)
			{
			case '\336':
				Sleep(1000);
				continue;
			case '\335':
				/* TODO: put in a call to sendbreak */
				// sendbreak(h);
				continue;
			default:
				sendline(zc, &zc->stP, (UCHAR)c);
			}
		}
	}


/*----------------------------------------------------------------------+
 | IsAnyLower - Returns TRUE if string s has lower case letters.
 +----------------------------------------------------------------------*/
int IsAnyLower(char *s)
	{
	for ( ; *s; ++s)
		// Don't use this stuff in a Chicago DLL
		// if (islower(*s))
		if ((*s >= 'a') && (*s <= 'z'))
			return TRUE;
	return FALSE;
	}

/*----------------------------------------------------------------------+
 | closeit - Close the receive dataset, return OK or ERROR
 +----------------------------------------------------------------------*/
int closeit(ZC *zc)
{
	// struct utimbuf timep;
	int reason;
	XFR_PARAMS *pX;

	reason = TSC_COMPLETE;		/* TODO: Get the real reason */
	if (zc->nSkip)
		{
		reason = TSC_USER_SKIP;
		}

	if (xfer_close_rcv_file(zc->hSession,
							zc->fh,
							reason,
							zc->fname,
							zc->our_fname,
							zc->fSavePartial,
							0,
							0) == ERROR)
		{
		return ERROR;
		}
	zc->fh = NULL;

	zc->actual_bytes += zc->real_bytes;
	zc->real_bytes = 0L;
	zc->total_bytes += zc->file_bytes;
	zc->file_bytes = 0L;
	zc->filesize = 0L;

	zc->nSkip = FALSE;

	pX = (XFR_PARAMS *)0;
	xfrQueryParameters(sessQueryXferHdl(zc->hSession), (VOID **)&pX);
	if ((pX != (XFR_PARAMS *)0) && (pX->fUseDateTime))
		{
		if (zc->Modtime)
			{
			// BYTE acName[FNAME_LEN];
			// zc->Modtime += timezone; /* Convert from GMT to local timezone */
			// timep.actime = time(NULL);
			// timep.modtime = zc->Modtime;
			// CharToOem(zc->our_fname, acName);
			// utime(acName, (void FAR *)&timep);
			itimeSetFileTime(zc->our_fname, zc->Modtime);
			}
		}

	// Disable this, it needs conversion and we only ever got complaints
	// about it anyway
	//if ((zc->Filemode & S_IFMT) == S_IFREG)
	//	far_chmod(zc->our_fname, (07777 & zc->Filemode));

	return OK;
	}

/*----------------------------------------------------------------------+
 | ackbibi - Ack a ZFIN packet, let byegones be byegones
 +----------------------------------------------------------------------*/
void ackbibi(ZC *zc)
	{
	register n;

	stohdr(zc, 0L);
	for (n = 3; --n >= 0; )
		{
		// purgeline(zc);
		ComRcvBufrClear(zc->hCom);
		zshhdr(zc, ZFIN, zc->Txhdr);
		// switch (readline(h, 100))
		switch (readline(zc, zc->Rxtimeout))		// Mobidem
			{
			case 'O':
				// readline(h, 1);    /* Discard 2nd 'O' */
				readline(zc, zc->Rxtimeout); /* Discard 2nd 'O' */	// Mobidem
				return;
			case RCDO:
				return;
			case TIMEOUT:
			default:
				break;
			}
		}
	}

/*----------------------------------------------------------------------+
 | isvalid
 +----------------------------------------------------------------------*/
int isvalid(char c, int base)
	{
	if (c < '0')
		return FALSE;
	switch (base)
	{
	case 8:
		if (c > '7')
			return FALSE;
		break;
	case 10:
		if (c > '9')
			return FALSE;
		break;
	case 16:
		if (c <= '9')
			return TRUE;
		// Don't use this stuff in a Chicago DLL
		// if (toupper(c) < 'A')
		//	return FALSE;
		//if (toupper(c) > 'F')
		//	return FALSE;
		if ((c >= 'a') && (c <= 'f'))
			break;
		if ((c >= 'A') && (c <= 'F'))
			break;
		return FALSE;
	}
	return TRUE;
}

/*----------------------------------------------------------------------+
 | ourspace -- replacement for isspace
 +----------------------------------------------------------------------*/
int ourspace(char c)
	{
	if (c == 0x20)
		return TRUE;
	if ((c >= 0x9) && (c <= 0xD))
		return TRUE;
	return FALSE;
	}

/*----------------------------------------------------------------------+
 | stoi - string to integer.
 +----------------------------------------------------------------------*/
char *stoi(char *ptr, int *val, int base)
	{
	int cnt;

	if (ptr == NULL)
		return NULL;
	// Don't do this in a Chicago DLL
	// while ((*ptr) && (isspace(*ptr)))
	while ((*ptr) && ourspace(*ptr))
		ptr++;
	cnt = 0;
	while ((*ptr) && (isvalid(*ptr, base)))
		{
		cnt *= base;
		cnt += (*ptr++ - '0');
		}
	*val = cnt;
	return ptr;
}

/*----------------------------------------------------------------------+
 | stol - string to long.
 +----------------------------------------------------------------------*/
char *stol(char *ptr, long *val, int base)
	{
	long cnt;

	if (ptr == NULL)
		return NULL;
	// Don't do this in a Chicago DLL
	// while ((*ptr) && (isspace(*ptr)))
	while ((*ptr) && (ourspace(*ptr)))
		ptr++;
	cnt = 0;
	while ((*ptr) && (isvalid(*ptr, base)))
		{
		cnt *= base;
		cnt += (*ptr++ - '0');
		}
	*val = cnt;
	return ptr;
}

/*----------------------------------------------------------------------+
 | procheader - Process incoming file information header.
 +----------------------------------------------------------------------*/
int procheader(ZC *zc, TCHAR *name)
	{
	int zRecover = FALSE;
	int lconv;
	int file_err;
	register char *p;
	int serial_number;
	int files_remaining;
	long bytes_remaining;
	long our_size;
	LONG lOptions = 0;
	XFR_Z_PARAMS *pZ;
	struct st_rcv_open stRcv;
	TCHAR loc_fname[FNAME_LEN];

	StrCharCopy(zc->fname, name);
	CharUpper(zc->fname);
	zc->Thisbinary = FALSE;

	zc->filesize = 0L;
	zc->Filemode = 0;
	zc->Modtime = 0L;
	serial_number = 0;
	files_remaining = -1;
	bytes_remaining = -1L;

	p = name + 1 + StrCharGetByteCount(name);
	if (*p)
		{	/* file coming from Unix or DOS system */
		if (*p)
			p = stol(p, &zc->filesize, 10);
		if (*p)
			p = stol(p, &zc->Modtime, 8);
		if (*p)
			p = stoi(p, &zc->Filemode, 8);
		if (*p)
			p = stoi(p, &serial_number, 10);
		if (*p)
			p = stoi(p, &files_remaining, 10);
		if (*p)
			p = stol(p, &bytes_remaining, 10);

		if ((zc->nfiles == 0) && (files_remaining != -1))
			{
			zc->nfiles = files_remaining;
			zmdmr_filecnt (zc, zc->nfiles);
			}

		if ((zc->nbytes == (-1L)) && (bytes_remaining != (-1L)))
			{
			zc->nbytes = bytes_remaining;
			zmdmr_totalsize (zc, zc->nbytes);
			}

		}
	else
		{	   /* File coming from CP/M system */
		for (p = zc->fname; *p; ++p)	   /* change / to _ */
			if ( *p == '/')
				*p = '_';

		if ( *--p == '.')		/* zap trailing period */
			*p = 0;
		}

	StrCharCopy(zc->our_fname, zc->fname);
	StrCharCopy(loc_fname, zc->fname);

	stRcv.pszSuggestedName = loc_fname;
	stRcv.pszActualName = zc->our_fname;
	stRcv.lFileTime = zc->Modtime;

	xfer_build_rcv_name(zc->hSession, &stRcv);

	//zc->ssMch = stRcv.ssmchVscanHdl;

	lconv = 0;

	pZ = (XFR_Z_PARAMS *)xfer_get_params(zc->hSession, zc->nMethod);
	if (pZ)
		{
		switch (pZ->nCrashRecRecv)
			{
			case ZP_CRR_ALWAYS:
				lconv = ZCRECOV;
				break;
			case ZP_CRR_NEVER:
				if ((lconv = zc->zconv) == ZCRECOV)
					lconv = 0;
				break;
			case ZP_CRR_NEG:
			default:
				lconv = zc->zconv;
				break;
			}
		}

	switch (lconv)
	{
	case ZCNL:
		zc->Thisbinary = FALSE;
		break;
	case ZCRECOV:
		/*
		 * This is a little complicated.  To do recovery, we need to check
		 * the following:
		 * 1. Does the file exist on OUR side.
		 * 2. Has the sender sent over a file size.
		 * 3. Is the size sent greater than the size of OUR file.
		 * If so, we fudge around a little with the file and let things rip.
		 */
		zRecover = TRUE;

		our_size = 0L;
		if (zRecover)
			{
			if (GetFileSizeFromName(stRcv.pszActualName, &our_size))
				zRecover = TRUE;
			else
				zRecover = FALSE;
			}

		if (zRecover)
			{
			/* Has the sender sent over a file size ? */
			if (zc->filesize <= 0)
				zRecover = FALSE;
			}

		if (zRecover)
			{
			/* This gets set up above after checking for existance */
			if (our_size != 0L)
				{
				if (our_size < zc->filesize)
					{
					/*
					 * We do this in the vain hope of avoiding problems with
					 * files terminated by ^Z and padded last blocks
					 *
					 * Given that we don't know if it is necessary, it might be
					 * possible to eliminate it
					 */
					our_size = (our_size - 1) & ~255L;
					}
				else
					{
                    return ERROR;
					//zRecover = FALSE;
					}
				}
			else
				{
				zRecover = FALSE;
				}
			}

		if (zRecover)
			{
			zc->file_bytes = our_size;
			}

		/* FALL THROUGH */
	case ZCBIN:
	default:
		zc->Thisbinary = TRUE;
		break;
	}

	if (zRecover)
		{
		lOptions = XFR_RO_APPEND;
		}
	else if (pZ->nFileExists == ZP_FE_SENDER)
		{
		switch (zc->zmanag & ZMMASK)
			{
			case ZMNEWL:
				/* TODO: complete this option */
				lOptions = 0;
				break;
			case ZMCRC:
				/* TODO: complete this option */
				lOptions = 0;
				break;
			case ZMAPND:
				lOptions = XFR_RO_APPEND;
				break;
			case ZMCLOB:
				lOptions = XFR_RO_ALWAYS;
				break;
			case ZMNEW:
				lOptions = XFR_RO_NEWER;
				break;
			case ZMDIFF:
				/* TODO: complete this option */
				lOptions = 0;
				break;
			case ZMPROT:
				lOptions = XFR_RO_NEVER;
				break;
			default:
				break;
			}
		}
	else
		{
		lOptions = zc->ulOverride;
		}

	StrCharCopy(zc->our_fname, zc->fname);
	StrCharCopy(loc_fname, zc->fname);

	stRcv.pszSuggestedName = loc_fname;
	stRcv.pszActualName = zc->our_fname;
	stRcv.lFileTime = zc->Modtime;

	/* TODO: pick up override options as necessary, like above */
	file_err = xfer_open_rcv_file(zc->hSession, &stRcv, lOptions);

	if (file_err != 0)
		{
		DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
		switch (file_err)
			{
        case -8:
            zmdm_retval(zc, TRUE, ZMDM_INUSE);
            break;
		case -6:            // File was rejected unconditionally
			zmdm_retval(zc, TRUE, ZMDM_REFUSE);
			break;
		case -5:			// Were unable to create needed directories
			zmdm_retval(zc, TRUE, ZFERR);
			break;
		case -4:  			// No date, time supplied when required
			zmdm_retval(zc, TRUE, ZFERR);
			break;
		case -3:			// File could not be saved
			zmdm_retval(zc, TRUE, ZFERR);
			break;
		case -2:  			// File was rejected due to date
			zmdm_retval(zc, TRUE, ZMDM_OLDER);
			break;
		case -1:			// Read/Write error occured
		default:
			zmdm_retval(zc, TRUE, ZFERR);
			break;
			}

		zc->total_bytes += zc->filesize;
		zc->filen += 1;
		zmdmr_newfile (zc, zc->filen, zc->fname, stRcv.pszActualName);
		return ERROR;
		}
	else
		{
		zmdm_retval(zc, TRUE, ZACK);
		}

	zc->fh = stRcv.bfHdl;
	zc->basesize = stRcv.lInitialSize;

	if (zRecover)
		{
        //jmh 04-02-96 Sure we opened it in "append" mode (which really
        // opens the file for write and seeks to the end), but we
        // might actually seek to a spot just before that due to the
        // possible padding of the file with ^Z's. 
        //
		fio_seek(zc->fh, zc->file_bytes, FIO_SEEK_SET);
		}

	zc->filen += 1;

	zmdmr_newfile (zc, zc->filen, zc->fname, stRcv.pszActualName);
	zmdmr_filesize(zc, zc->filesize);

	return OK;
	}

/*----------------------------------------------------------------------+
 | putsec - Putsec writes the n characters of buf to receive file.
 |			If not in binary mode, carriage returns, and all characters
 |			starting with CPMEOF are discarded.
 +----------------------------------------------------------------------*/
int putsec(ZC *zc, BYTE *buf, int n)
	{
	register BYTE *p;
	register int ii;

	if (n == 0)
		return OK;

	if (zc->Thisbinary)
		{
		// jkh, 2/11 Added error check
		if (fio_write(buf, 1, n, zc->fh) != n)
			longjmp(zc->flagkey_buf, 4);
		}
	else
		{
		if (zc->Eofseen)
			return OK;
		ii = FALSE;
		for (p=buf; --n>=0; ++p )
			{
			if ( *p == '\n')
				{
				/*
				 * If we get a <NL> that wasn't preceeded by a <CR>
				 */
				if (ii == FALSE)
					fio_putc('\r', zc->fh);
				}

			ii = (*p == '\r');

			if (*p == CPMEOF)
				{
				zc->Eofseen=TRUE; return OK;
				}
			fio_putc(*p, zc->fh);
			}
		}
	return OK;
	}

/* *********** end of zmdm_rcv.c *********** */
