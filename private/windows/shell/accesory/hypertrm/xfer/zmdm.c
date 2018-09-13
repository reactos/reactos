/* zmdm.c -- Routines to handle zmodem for HyperACCESS
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:22p $
 */
/*
 *   Z M . C
 *    ZMODEM protocol primitives
 *    05-09-88  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr, eflag) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 */

#include <windows.h>
#pragma hdrstop

#include <setjmp.h>

#define	BYTE	unsigned char

#include <tdll\stdtyp.h>
#include <tdll\com.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\file_io.h>
#include "xfr_srvc.h"
#include "xfr_todo.h"
#include "xfr_dsp.h"
#include "xfer_tsc.h"
#include "foo.h"


#include "zmodem.hh"
#include "zmodem.h"

#if defined(DEBUG_DUMPPACKET)
#include <stdio.h>
extern FILE* fpPacket;
void DbgDumpPacket(ZC* zc, BYTE* buf, int nLength);
#endif  // defined(DEBUG_DUMPPACKET)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * readline - added to get all this stuff to match up
 *
 */

/*----------------------------------------------------------------------+
 | zmdm_rl
 +----------------------------------------------------------------------*/
int zmdm_rl (ZC *zc, int timeout)
	{
	TCHAR c;
	int x;
	int n;
	long elapsed_time;

	elapsed_time = startinterval();

	// if ((c = rdget(zc)) == (-1))
	if ((n = mComRcvChar(zc->hCom, &c)) == 0)
		{
		xfer_idle(zc->hSession, XFER_IDLE_IO);

		while (((long)interval(elapsed_time) <= timeout) && (n == 0))
			{
			x = xfer_user_interrupt(zc->hSession);

			switch (x)
				{
				case XFER_ABORT:
					zmdmr_update(zc, ZCAN);
					longjmp(zc->flagkey_buf, 1);
					break;

				case XFER_SKIP:
					/* This MUST only happen while receiving */
					stohdr(zc, zc->filesize);
					zshhdr(zc, ZRPOS, zc->Txhdr);
					zc->nSkip = TRUE;
					zc->file_bytes = zc->filesize;
					break;

				default:
					break;
				}

			xfer_idle(zc->hSession, XFER_IDLE_IO);
			// c = rdget(zc);
			n = mComRcvChar(zc->hCom, &c);
			}
		if (n == 0)
			return (TIMEOUT);
		}

	return (c);
	}

/*----------------------------------------------------------------------+
 | zsbhdr - Send ZMODEM binary header hdr of type type.
 +----------------------------------------------------------------------*/
void zsbhdr(ZC *zc, int type, BYTE *hdr)
    {
	register int n;
	register unsigned short crc;

	xsendline(zc, &zc->stP, ZPAD);
	xsendline(zc, &zc->stP, ZDLE);

	if (zc->Crc32t = zc->Txfcs32)
		zsbh32(zc, hdr, type);
	else
		{
		xsendline(zc, &zc->stP, ZBIN);
		zsendline(zc, type);
		crc = updcrc(zc, type, 0);

		for (n=4; --n >= 0; ++hdr)
			{
			zsendline(zc, *hdr);
			crc = updcrc(zc, (0377& *hdr), crc);
			}
		crc = updcrc(zc, 0,crc);
		crc = updcrc(zc, 0,crc);
		zsendline(zc, crc>>8);
		zsendline(zc, crc);
		}
	if (type != ZDATA)
		flushmo(zc, &zc->stP);

	// xfer_idle(zc->hSession, XFER_IDLE_IO);
	}

/*----------------------------------------------------------------------+
 | zsbh32 - Send ZMODEM binary header hdr of type type.
 +----------------------------------------------------------------------*/
void zsbh32(ZC *zc, BYTE *hdr, int type)
	{
	register int n;
	register unsigned long crc;

	xsendline(zc, &zc->stP, ZBIN32);
	zsendline(zc, type);
	crc = 0xFFFFFFFFL; crc = UPDC32(zc, type, crc);

	for (n=4; --n >= 0; ++hdr)
		{
		crc = UPDC32(zc, (0377 & *hdr), crc);
		zsendline(zc, *hdr);
		}
	crc = ~crc;
	for (n=4; --n >= 0;)
		{
		zsendline(zc, (int)crc);
		crc >>= 8;
		}

	flushmo(zc, &zc->stP);

	// xfer_idle(zc->hSession, XFER_IDLE_IO);

	}

/*----------------------------------------------------------------------+
 | zshhdr - Send ZMODEM HEX header hdr of type type.
 +----------------------------------------------------------------------*/
void zshhdr(ZC *zc, int type, BYTE *hdr)
	{
	register int n;
	register unsigned short crc;

	sendline(zc, &zc->stP, ZPAD);
	sendline(zc, &zc->stP, ZPAD);
	sendline(zc, &zc->stP, ZDLE);
	sendline(zc, &zc->stP, ZHEX);

	zputhex(zc, type);
	zc->Crc32t = 0;

	crc = updcrc(zc, type, 0);
	for (n=4; --n >= 0; ++hdr)
		{
		zputhex(zc, *hdr);
		crc = updcrc(zc, (0377 & *hdr), crc);
		}
	crc = updcrc(zc, 0, crc);
	crc = updcrc(zc, 0,crc);
	zputhex(zc, crc>>8);
	zputhex(zc, crc);

	/* Make it printable on remote machine */
	sendline(zc, &zc->stP, 015);
	sendline(zc, &zc->stP, 0212);
	/*
	 * Uncork the remote in case a fake XOFF has stopped data flow
	 */
	if (type != ZFIN && type != ZACK)
		sendline(zc, &zc->stP, 021);
	flushmo(zc, &zc->stP);

	// xfer_idle(zc->hSession, XFER_IDLE_IO);
	}

/*----------------------------------------------------------------------+
 | zsdata - Send binary array buf of lengthl length, with ending ZDLE
 |			sequence frameend.
 +----------------------------------------------------------------------*/
void zsdata(ZC *zc, BYTE *buf, int length, int frameend)
	{
	register unsigned short crc;

	// xfer_idle(zc->hSession, XFER_IDLE_IO);

	if (zc->Crc32t)
		zsda32(zc, buf, length, frameend);
	else
		{
		crc = 0;
		for (;--length >= 0; ++buf)
			{
			zsendline(zc, *buf); crc = updcrc(zc, (0377 & *buf), crc);
			}
		xsendline(zc, &zc->stP, ZDLE);
		xsendline(zc, &zc->stP, (UCHAR)frameend);
		crc = updcrc(zc, frameend, crc);

		crc = updcrc(zc, 0,crc);
		crc = updcrc(zc, 0,crc);
		zsendline(zc, crc>>8); zsendline(zc, crc);
		}
	if (frameend == ZCRCW)
		{
		xsendline(zc, &zc->stP, XON);
		flushmo(zc, &zc->stP);
		}

	// xfer_idle(zc->hSession, XFER_IDLE_IO);
	}

/*----------------------------------------------------------------------+
 | zsda32
 +----------------------------------------------------------------------*/
void zsda32(ZC *zc, BYTE *buf, int length, int frameend)
	{
	register int c;
	register unsigned long crc;

	crc = 0xFFFFFFFFL;
	for (;--length >= 0; ++buf)
		{
		c = *buf & 0377;
		if (c & 0140)
			{
			zc->lastsent = c;
			xsendline(zc, &zc->stP, ((UCHAR)c));
			}
		else
			zsendline(zc, c);
		crc = UPDC32(zc, c, crc);
		}
	xsendline(zc, &zc->stP, ZDLE);
	xsendline(zc, &zc->stP, (UCHAR)frameend);
	crc = UPDC32(zc, frameend, crc);

	crc = ~crc;
	for (length=4; --length >= 0;)
		{
		zsendline(zc, (int)crc);  crc >>= 8;
		}
	}

/*----------------------------------------------------------------------+
 | zrdata - Receive array buf of max length with ending ZDLE sequence and
 |			CRC.  Returns the ending character or error code.  NB: On
 |			errors may store length+1 bytes!
 +----------------------------------------------------------------------*/
int zrdata(ZC *zc, BYTE *buf, int length)
	{
	register int c;
	register unsigned short crc;
	register BYTE *end;
	register int d;

	if (zc->Rxframeind == ZBIN32)
		return zrdat32(zc, buf, length);

	crc = zc->Rxcount = 0;  end = buf + length;
	while (buf <= end)
		{
		if ((c = zdlread(zc)) & ~0377)
			{
crcfoo:
			switch (c)
				{
				case GOTCRCE:
				case GOTCRCG:
				case GOTCRCQ:
				case GOTCRCW:
					crc = updcrc(zc, (d=c)&0377, crc);
					if ((c = zdlread(zc)) & ~0377)
						goto crcfoo;
					crc = updcrc(zc, c, crc);
					if ((c = zdlread(zc)) & ~0377)
						goto crcfoo;
					crc = updcrc(zc, c, crc);
					if (crc & 0xFFFF)
						{
						/* zperr(badcrc); */
#if defined(DEBUG_DUMPPACKET)
                        fprintf(fpPacket, "zrdata: Bad CRC 0x%04X\n", crc);
                        DbgDumpPacket(zc, end - length, length - (end - buf));
#endif  // defined(DEBUG_DUMPPACKET)
						zmdmr_update (zc, ERROR);
						return ERROR;
						}
					zc->Rxcount = (int)(length - (end - buf));
					return d;
				case GOTCAN:
					/* zperr("Sender Canceled"); */
					zmdmr_update (zc, ZCAN);
					return ZCAN;
				case TIMEOUT:
					/* zperr("TIMEOUT"); */
#if defined(DEBUG_DUMPPACKET)
                    fputs("zrdata: Timed-out\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
					zmdmr_update(zc, TIMEOUT);
					return c;
				default:
					/* zperr("Bad data subpacket"); */
#if defined(DEBUG_DUMPPACKET)
                    fprintf(fpPacket, "zrdata: Bad data subpacket c=%d\n", c);
#endif  // defined(DEBUG_DUMPPACKET)
					zmdmr_update(zc, ZBADFMT);
					return c;
				}
			}
		*buf++ = (char)c;
		crc = updcrc(zc, c, crc);
		}
	/* zperr("Data subpacket too long"); */
	DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
#if defined(DEBUG_DUMPPACKET)
    fputs("zrdata: Data subpacket too long\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
	zmdmr_update(zc, ZBADFMT);
	return ERROR;
	}

/*----------------------------------------------------------------------+
 | zrdat32
 +----------------------------------------------------------------------*/
int zrdat32(ZC *zc, BYTE *buf, int length)
	{
	register int c;
	register unsigned long crc;
	register BYTE *end;
	register int d;

	crc = 0xFFFFFFFFL;  zc->Rxcount = 0;  end = buf + length;
	while (buf <= end)
		{
		if ((c = zdlread(zc)) & ~0377)
			{
crcfoo:
			switch (c)
				{
				case GOTCRCE:
				case GOTCRCG:
				case GOTCRCQ:
				case GOTCRCW:
					d = c;  c &= 0377;
					crc = UPDC32(zc, c, crc);
					if ((c = zdlread(zc)) & ~0377)
						goto crcfoo;
					crc = UPDC32(zc, c, crc);
					if ((c = zdlread(zc)) & ~0377)
						goto crcfoo;
					crc = UPDC32(zc, c, crc);
					if ((c = zdlread(zc)) & ~0377)
						goto crcfoo;
					crc = UPDC32(zc, c, crc);
					if ((c = zdlread(zc)) & ~0377)
						goto crcfoo;
					crc = UPDC32(zc, c, crc);
					if (crc != 0xDEBB20E3)
						{
						/* zperr(badcrc); */
						DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
#if defined(DEBUG_DUMPPACKET)
                        fprintf(fpPacket, "zrdat32: Bad 32-bit CRC 0x%08lX\n", crc);
                        DbgDumpPacket(zc, end - length, length - (end - buf));
#endif  // defined(DEBUG_DUMPPACKET)
						zmdmr_update (zc, ERROR);
						return ERROR;
						}
					zc->Rxcount = (int)(length - (end - buf));
					return d;
				case GOTCAN:
					/* zperr("Sender Canceled"); */
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					zmdmr_update (zc, ZCAN);
					return ZCAN;
				case TIMEOUT:
					/* zperr("TIMEOUT"); */
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
#if defined(DEBUG_DUMPPACKET)
                    fputs("zrdata: Timed-out\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
					zmdmr_update (zc, TIMEOUT);
					return c;
				default:
					/* zperr("Bad data subpacket"); */
					DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
#ifdef  DEBUG_DUMPPACKET
                    fputs("zrdat32: Bad data subpacket\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
					zmdmr_update (zc, ZBADFMT);
					return c;
				}
			}
		*buf++ = (char)c;
		crc = UPDC32(zc, c, crc);
		}
	/* zperr("Data subpacket too long"); */
	DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
#ifdef  DEBUG_DUMPPACKET
    fputs("zrdat32: Data subpacket too long\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
	zmdmr_update (zc, ZBADFMT);
	return ERROR;
	}

#if defined(DEBUG_DUMPPACKET)
void DbgDumpPacket(ZC *zc, BYTE *buf, int length)
    {
    int     nCount;
    int     jj;

    fputs("Here's the offending packet:\n", fpPacket);

    for (nCount = 0; nCount < length; nCount += 16)
        {
        for (jj = nCount; jj < nCount + 16 && jj < length; jj += 1)
            {
            fprintf(fpPacket, "%02X ", (unsigned int) buf[jj]);
            }
        fputs("\n", fpPacket);
        }
    }
#endif  // defined(DEBUG_DUMPPACKET)

/*----------------------------------------------------------------------+
 | zgethdr - Read a ZMODEM header to hdr, either binary or HEX.
 |			 On success, set Rxpos and return type of header.
 |			 Otherwise return negative on error.
 |			 Return ERROR instantly if ZCRCW sequence, for fast error recovery.
 +----------------------------------------------------------------------*/
int zgethdr(ZC *zc, BYTE *hdr, int eflag)
	{
	register int c, n, cancount;


	/* Max bytes before start of frame */
	// n = zc->Zrwindow + (int)cnfg.bit_rate;
	n = zc->Zrwindow + cnfgBitRate();

	zc->Rxframeind = zc->Rxtype = 0;

startover:
	cancount = 5;
again:
	/* Return immediate ERROR if ZCRCW sequence seen */
	switch (c = readline(zc, zc->Rxtimeout))
		{
		case RCDO:
		case TIMEOUT:
			goto fifi;
		case CAN:
gotcan:
			if (--cancount <= 0)
				{
				c = ZCAN; goto fifi;
				}
			// switch (c = readline(h, 1))
			switch (c = readline(zc, zc->Rxtimeout))		// Mobidem
				{
				case TIMEOUT:
					goto again;
				case ZCRCW:
#if defined(DEBUG_DUMPPACKET)
                    fputs("zgethdr: ZCRCW ret from readline\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
					c = ERROR;
					/* **** FALL THRU TO **** */
				case RCDO:
					goto fifi;
				default:
					break;
				case CAN:
					if (--cancount <= 0)
						{
						c = ZCAN; goto fifi;
						}
					goto again;
				}
			/* **** FALL THRU TO **** */
		default:
agn2:
			if ( --n == 0)
				{
				/* zperr("Garbage count exceeded"); */
				DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
#if defined(DEBUG_DUMPPACKET)
                fputs("zgethdr: Garbage count exceeded\n", fpPacket);
#endif  // defined(DEBUG_DUMPPACKET)
				return(ERROR);
				}
			goto startover;
		case ZPAD|0200:		/* This is what we want. */
			zc->Not8bit = c;
		case ZPAD:		/* This is what we want. */
			break;
		}
	cancount = 5;
splat:
	switch (c = noxrd7(zc))
		{
		case ZPAD:
			goto splat;
		case RCDO:
		case TIMEOUT:
			goto fifi;
		default:
			goto agn2;
		case ZDLE:		/* This is what we want. */
			break;
		}

	switch (c = noxrd7(zc))
		{
		case RCDO:
		case TIMEOUT:
			goto fifi;
		case ZBIN:
			zc->Rxframeind = ZBIN;  zc->Crc32 = FALSE;
			c =  zrbhdr(zc, hdr, eflag);
			break;
		case ZBIN32:
			zc->Crc32 = zc->Rxframeind = ZBIN32;
			c =  zrbhdr32(zc, hdr, eflag);
			break;
		case ZHEX:
			zc->Rxframeind = ZHEX;  zc->Crc32 = FALSE;
			c =  zrhhdr(zc, hdr, eflag);
			break;
		case CAN:
			goto gotcan;
		default:
			goto agn2;
		}
	zc->Rxpos = hdr[ZP3] & 0377;
	zc->Rxpos = (zc->Rxpos<<8) + (hdr[ZP2] & 0377);
	zc->Rxpos = (zc->Rxpos<<8) + (hdr[ZP1] & 0377);
	zc->Rxpos = (zc->Rxpos<<8) + (hdr[ZP0] & 0377);
fifi:
	switch (c)
		{
		case GOTCAN:
			c = ZCAN;
		/* **** FALL THRU TO **** */
		case ZNAK:
		case ZCAN:
		case ERROR:
		case TIMEOUT:
		case RCDO:
			/* **** FALL THRU TO **** */
		default:
			break;
		}
	if (eflag == 'T')
		{
		zmdms_update(zc, c);
		}
	else if (eflag == 'R')
		{
		zmdmr_update(zc, c);
		}
	return c;
	}

/*----------------------------------------------------------------------+
 | zrbhdr - Receive a binary style header (type and position).
 +----------------------------------------------------------------------*/
int zrbhdr(ZC *zc, BYTE *hdr, int eflag)
	{
	register int c, n;
	register unsigned short crc;

	if ((c = zdlread(zc)) & ~0377)
		return c;
	zc->Rxtype = c;
	crc = updcrc(zc, c, 0);

	for (n=4; --n >= 0; ++hdr)
		{
		if ((c = zdlread(zc)) & ~0377)
			return c;
		crc = updcrc(zc, c, crc);
		*hdr = (char)c;
		}
	if ((c = zdlread(zc)) & ~0377)
		return c;
	crc = updcrc(zc, c, crc);
	if ((c = zdlread(zc)) & ~0377)
		return c;
	crc = updcrc(zc, c, crc);
	if (crc & 0xFFFF)
		{
		/* zperr(badcrc); */
		DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
		if (eflag == 'T')
			zmdms_update(zc, ERROR);
		else if (eflag == 'R')
            {
#if defined(DEBUG_DUMPPACKET)
            fprintf(fpPacket, "zrbhdr: Bad CRC 0x%04X\n", crc);
#endif  // defined(DEBUG_DUMPPACKET)
			zmdmr_update(zc, ERROR);
            }
		return ERROR;
		}
	return zc->Rxtype;
	}

/*----------------------------------------------------------------------+
 | zrbhdr32 - Receive a binary style header (type and position) with
 |			  32 bit FCS.
 +----------------------------------------------------------------------*/
int zrbhdr32(ZC *zc, BYTE *hdr, int eflag)
	{
	register int c, n;
	register unsigned long crc;

	if ((c = zdlread(zc)) & ~0377)
		return c;
	zc->Rxtype = c;
	crc = 0xFFFFFFFFL;
	crc = UPDC32(zc, c, crc);

	for (n=4; --n >= 0; ++hdr)
		{
		if ((c = zdlread(zc)) & ~0377)
			return c;
		crc = UPDC32(zc, c, crc);
		*hdr = (char)c;
		}
	for (n=4; --n >= 0;)
		{
		if ((c = zdlread(zc)) & ~0377)
			return c;
		crc = UPDC32(zc, c, crc);
		}
	if (crc != 0xDEBB20E3)
		{
		/* zperr(badcrc); */
		DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
		if (eflag == 'T')
			zmdms_update(zc, ERROR);
		else if (eflag == 'R')
            {
#if defined(DEBUG_DUMPPACKET)
            fprintf(fpPacket, "zrbhdr32: Bad CRC 0x%08lX\n", crc);
#endif  // defined(DEBUG_DUMPPACKET)
			zmdmr_update(zc, ERROR);
            }
		return ERROR;
		}
	return zc->Rxtype;
	}

/*----------------------------------------------------------------------+
 | zrhhdr - Receive a hex style header (type and postion).
 +----------------------------------------------------------------------*/
int zrhhdr(ZC *zc, BYTE *hdr, int eflag)
	{
	register int c;
	register unsigned short crc;
	register int n;

	if ((c = zgethex(zc)) < 0)
		return c;
	zc->Rxtype = c;
	crc = updcrc(zc, c, 0);

	for (n=4; --n >= 0; ++hdr)
		{
		if ((c = zgethex(zc)) < 0)
			return c;
		crc = updcrc(zc, c, crc);
		*hdr = (char)c;
		}
	if ((c = zgethex(zc)) < 0)
		return c;
	crc = updcrc(zc, c, crc);
	if ((c = zgethex(zc)) < 0)
		return c;
	crc = updcrc(zc, c, crc);
	if (crc & 0xFFFF)
		{
		/* zperr(badcrc); */
		DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
		if (eflag == 'T')
			zmdms_update(zc, ERROR);
		else if (eflag == 'R')
            {
#if defined(DEBUG_DUMPPACKET)
            fprintf(fpPacket, "zrhhdr: Bad CRC 0x%04X\n", crc);
#endif  // defined(DEBUG_DUMPPACKET)
			zmdmr_update(zc, ERROR);
            }
		return ERROR;
		}
	// switch ( c = readline(h, 1))
	switch ( c = readline(zc, zc->Rxtimeout))		// Mobidem
		{
		case 0215:
			zc->Not8bit = c;
			/* **** FALL THRU TO **** */
		case 015:
		 	/* Throw away possible cr/lf */
			// switch (c = readline(h, 1))
			switch (c = readline(zc, zc->Rxtimeout))		// Mobidem
				{
				case 012:
					zc->Not8bit |= c;
				}
		}
	return zc->Rxtype;
	}

/*----------------------------------------------------------------------+
 | zputhex - Send a byte as two hex digits.
 +----------------------------------------------------------------------*/
void zputhex(ZC *zc, int c)
	{
	static BYTE	digits[]	= "0123456789abcdef";

	sendline(zc, &zc->stP, digits[(c&0xF0)>>4]);
	sendline(zc, &zc->stP, digits[(c)&0xF]);
	}

/*----------------------------------------------------------------------+
 | zsendline - Send character c with ZMODEM escape sequence encoding.
 |			   Escape XON, XOFF.  Escape CR following @ (Telnet net escape).
 +----------------------------------------------------------------------*/
void zsendline(ZC *zc, int c)
	{

	/* Quick check for non control characters */
	if (c & 0140)
		{
		zc->lastsent = c;
		xsendline(zc, &zc->stP, ((UCHAR)c));
		}
	else
		{
		switch (c &= 0377)
			{
			case ZDLE:
				xsendline(zc, &zc->stP, ZDLE);
				xsendline(zc, &zc->stP, (UCHAR)(zc->lastsent = (c ^= 0100)));
				break;
			case 015:
			case 0215:
				if (!zc->Zctlesc && (zc->lastsent & 0177) != '@')
					goto sendit;
				/* **** FALL THRU TO **** */
			case 020:
			case 021:
			case 023:
			case 0220:
			case 0221:
			case 0223:
				xsendline(zc, &zc->stP, ZDLE);
				c ^= 0100;
	sendit:
				xsendline(zc, &zc->stP, (UCHAR)(zc->lastsent = c));
				break;
			default:
				if (zc->Zctlesc && ! (c & 0140))
					{
					xsendline(zc, &zc->stP, ZDLE);
					c ^= 0100;
					}
				xsendline(zc, &zc->stP, (UCHAR)(zc->lastsent = c));
			}
		}
	}

/*----------------------------------------------------------------------+
 | zgethex - Decode two lower case hex digits into an 8 bit byte value.
 +----------------------------------------------------------------------*/
int zgethex(ZC *zc)
	{
	register int c;

	c = zgeth1(zc);
	return c;
	}

/*----------------------------------------------------------------------+
 | zgeth1
 +----------------------------------------------------------------------*/
int zgeth1(ZC *zc)
	{
	register int c, n;

	if ((c = noxrd7(zc)) < 0)
		return c;
	n = c - '0';
	if (n > 9)
		n -= ('a' - ':');
	if (n & ~0xF)
        {
#if defined(DEBUG_DUMPPACKET)
        fprintf(fpPacket, "zgeth1: n = 0x%02X\n", n);
#endif  // defined(DEBUG_DUMPPACKET)
		return ERROR;
        }
	if ((c = noxrd7(zc)) < 0)
		return c;
	c -= '0';
	if (c > 9)
		c -= ('a' - ':');
	if (c & ~0xF)
        {
#if defined(DEBUG_DUMPPACKET)
        fprintf(fpPacket, "zgeth1: c = 0x%02X\n", c);
#endif  // defined(DEBUG_DUMPPACKET)
		return ERROR;
        }
	c += (n<<4);
	return c;
	}

/*----------------------------------------------------------------------+
 | zdlread - Read a byte, checking for ZMODEM escape encoding including
 |			 CAN*5 which represents a quick abort.
 +----------------------------------------------------------------------*/
int zdlread(ZC *zc)
	{
	register int c;

again:
	/* Quick check for non control characters */
	if ((c = readline(zc, zc->Rxtimeout)) & 0140)
		return c;
	switch (c)
		{
		case ZDLE:
			break;
		case 023:
		case 0223:
		case 021:
		case 0221:
			goto again;
		default:
			if (zc->Zctlesc && !(c & 0140))
				{
				goto again;
				}
			return c;
		}
again2:
	if ((c = readline(zc, zc->Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(zc, zc->Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(zc, zc->Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(zc, zc->Rxtimeout)) < 0)
		return c;
	switch (c)
		{
		case CAN:
			return GOTCAN;
		case ZCRCE:
		case ZCRCG:
		case ZCRCQ:
		case ZCRCW:
			return (c | GOTOR);
		case ZRUB0:
			return 0177;
		case ZRUB1:
			return 0377;
		case 023:
		case 0223:
		case 021:
		case 0221:
			goto again2;
		default:
			if (zc->Zctlesc && ! (c & 0140))
				{
				goto again2;
				}
			if ((c & 0140) ==  0100)
				return (c ^ 0100);
			break;
		}
	return ERROR;
	}
/*----------------------------------------------------------------------+
 | noxrd7 - Read a character from the modem line with timeout.
 |			Eat parity, XON and XOFF characters.
 +----------------------------------------------------------------------*/
int noxrd7(ZC *zc)
	{
	register int c;

	for (;;)
		{
		if ((c = readline(zc, zc->Rxtimeout)) < 0)
			return c;
		switch (c &= 0177)
			{
			case XON:
			case XOFF:
				continue;
			default:
				if (zc->Zctlesc && !(c & 0140))
					continue;
			case '\r':
			case '\n':
			case ZDLE:
				return c;
			}
		}
	}

/*----------------------------------------------------------------------+
 | stohdr - Store long integer pos in Txhdr.
 +----------------------------------------------------------------------*/
void stohdr(ZC *zc, long pos)
	{
	zc->Txhdr[ZP0] = (char)(pos & 0377);
	zc->Txhdr[ZP1] = (char)((pos>>8) & 0377);
	zc->Txhdr[ZP2] = (char)((pos>>16) & 0377);
	zc->Txhdr[ZP3] = (char)((pos>>24) & 0377);
	}

/*----------------------------------------------------------------------+
 | rclhdr - Recover a long integer from a header.
 +----------------------------------------------------------------------*/
long rclhdr(BYTE *hdr)
	{
	register long l;

	l = (hdr[ZP3] & 0377);
	l = (l << 8) | (hdr[ZP2] & 0377);
	l = (l << 8) | (hdr[ZP1] & 0377);
	l = (l << 8) | (hdr[ZP0] & 0377);
	return l;
	}

static unsigned int z_errors [] = {
	TSC_DISK_FULL,			/* -4 */
	TSC_LOST_CARRIER,		/* -3 */
	TSC_NO_RESPONSE,		/* -2 */
	TSC_BAD_FORMAT,			/* -1 */
#define ERROFFSET	4
	TSC_GEN_FAILURE,		/* ZRQINIT (0)	   */
	TSC_GEN_FAILURE,		/* ZRINIT (1)	   */
	TSC_GEN_FAILURE,		/* ZSINIT (2)	   */
	TSC_OK,					/* ZACK (3) 	   */
	TSC_GEN_FAILURE,		/* ZFILE (4)	   */
	TSC_OK, 				/* ZSKIP (5)	   */
	TSC_GEN_FAILURE,		/* ZNAK (6) 	   */
	TSC_RMT_CANNED,			/* ZABORT (7)	   */
	TSC_COMPLETE,			/* ZFIN (8) 	   */
	TSC_GEN_FAILURE,		/* ZRPOS (9)	   */
	TSC_GEN_FAILURE,		/* ZDATA (10)	   */
	TSC_OK,					/* ZEOF (11)	   */
	TSC_DISK_ERROR,			/* ZFERR (12)	   */
	TSC_GEN_FAILURE,		/* ZCRC (13)	   */
	TSC_OK,					/* ZCHALLENGE (14) */
	TSC_COMPLETE,			/* ZCOMPL (15)	   */
	TSC_USER_CANNED,		/* ZCAN (16)	   */
	TSC_OK, 				/* ZFREECNT (17)   */
	TSC_CANT_START,			/* ZCOMMAND (18)   */
	TSC_CANT_START,			/* ZSTDERR (19)    */
	TSC_BAD_FORMAT,			/* added for HA/5  */
	TSC_OK, 				/* sent ack (21)   */
	TSC_VIRUS_DETECT,		/* ZMDM_VIRUS (22) */
	TSC_REFUSE,	 			/* ZMDM_REFUSE (23)*/
	TSC_OLDER_FILE,			/* ZMDM_OLDER (24) */
    TSC_FILEINUSE           /* ZMDM_INUSE (25) */
};
#define ERRSIZE 29

/*----------------------------------------------------------------------+
 | zmdm_error
 +----------------------------------------------------------------------*/
unsigned int zmdm_error(ZC *zc, int error)
	{

	error += ERROFFSET;
	if ((error > ERRSIZE) || (error < 0))
		return TSC_GEN_FAILURE;
	else
		return z_errors[error];
	}

/*----------------------------------------------------------------------+
 | zmdm_retval
 +----------------------------------------------------------------------*/
int zmdm_retval(ZC *zc, int flag, int error)
	{

	if (flag == TRUE)
		zc->s_error = error;

	return(zc->s_error);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * zmdms_progress
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
void zmdms_progress(ZC *zc, int status)
	{
	long ttime, stime;
	long bytes_sent;
	long cps;
	// int	k_sent;
	long new_stime	  = -1;
	long new_ttime	  = -1;
	long new_cps	  = -1;
	long file_so_far  = -1;
	long total_so_far = -1;

	if (zc->xfertimer == -1L)
		return;

	ttime = bittest(status, TRANSFER_DONE) ?
			zc->xfertime : interval(zc->xfertimer);
	if ((stime = ttime / 10L) != zc->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		new_stime = stime;

		/* Display amount transferred */
		bytes_sent = zc->file_bytes + zc->total_bytes;

		file_so_far  = zc->file_bytes;
		total_so_far = bytes_sent;

		/* Display throughput and time remaining */
		if ((stime > 2 ||
			 ttime > 0 && bittest(status, FILE_DONE | TRANSFER_DONE)) &&
			(cps = ((zc->real_bytes + zc->actual_bytes) * 10L) / ttime) > 0)
			{
			new_cps = cps;

			if (bittest(status, TRANSFER_DONE))
				ttime = 0;
			else
				ttime = ((zc->nbytes - bytes_sent) / cps) +
						  zc->nfiles - zc->filen;

			new_ttime = ttime;
			}
		zc->displayed_time = stime;
		}

		xferMsgProgress(zc->hSession,
						new_stime,
						new_ttime,
						new_cps,
						file_so_far,
						total_so_far);

	}

/*----------------------------------------------------------------------+
 | zmdms_newfile
 +----------------------------------------------------------------------*/
void zmdms_newfile(ZC *zc, int filen, TCHAR *fname, long flength)
	{
	xferMsgNewfile(zc->hSession,
				   filen,
				   NULL,
				   fname);

	xferMsgFilesize(zc->hSession, flength);

	}

/*----------------------------------------------------------------------+
 | zmsma_update
 +----------------------------------------------------------------------*/
void zmdms_update(ZC *zc, int state)
	{

	if ((state == ZACK) || (state == ZMDM_ACKED))
		return;

	zc->last_event = zc->pstatus;
	zc->pstatus = state;

	xferMsgStatus(zc->hSession, zc->pstatus + 4);

	xferMsgEvent(zc->hSession, zc->last_event + 4);

	if (state == ERROR)
		{
		zc->errors += 1;
		xferMsgErrorcnt(zc->hSession, zc->errors);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * zmdmr_progress
 *
 * DESCRIPTION:
 *	Displays the progress of a filetransfer on screen by showing the number of
 *	bytes transferred and updating the vu meters if they have been initialized.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void zmdmr_progress(ZC *zc, int status)
	{
	long ttime, stime;
	long bytes_rcvd;
	long bytes_diff;
	// long k_rcvd;
	long cps;
	long new_stime	  = -1;
	long new_ttime	  = -1;
	long new_cps	  = -1;
	long file_so_far  = -1;
	long total_so_far = -1;

	if (zc->xfertimer == -1L)
		return;
	ttime = bittest(status, TRANSFER_DONE) ?
			zc->xfertime : interval(zc->xfertimer);

	if ((stime = ttime / 10L) != zc->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		/* Display elapsed time */
		new_stime = stime;

		bytes_rcvd = zc->total_bytes + zc->file_bytes;
		if (bittest(status, FILE_DONE))
			if (zc->filesize != 0)
				if (zc->file_bytes != zc->filesize)
					{
					zmdmr_filesize(zc, zc->file_bytes);
					bytes_diff = zc->filesize- zc->file_bytes;
					zc->filesize -= bytes_diff;
					zc->nbytes -= bytes_diff;
					zmdmr_totalsize(zc, zc->nbytes);
					}

		/* Display amount received */
		file_so_far = zc->file_bytes;

		total_so_far = bytes_rcvd;

		/* Display throughput and time remaining */
		if (stime > 0 &&
				(cps = ((zc->actual_bytes + zc->real_bytes)*10L) / ttime) > 0)
			{
			new_cps = cps;

			/* calculate time to completion */
			if (zc->nbytes > 0L)
				{
				ttime = (zc->nbytes - bytes_rcvd) / cps;
				if (zc->nfiles > 1)
					ttime += (zc->nfiles - zc->filen);
				new_ttime = ttime;
				}
			else if (zc->filesize > 0L)
				{
				ttime = (zc->filesize - zc->file_bytes) / cps;
				new_ttime = ttime;
				}
			}

		xferMsgProgress(zc->hSession,
						new_stime,
						new_ttime,
						new_cps,
						file_so_far,
						total_so_far);
		}
	}

/*----------------------------------------------------------------------+
 | zmdmr_update
 +----------------------------------------------------------------------*/
void zmdmr_update(ZC *zc, int status)
	{

	if ((status == ZACK) || (status == ZMDM_ACKED))
		return;

	zc->last_event = zc->pstatus;
	zc->pstatus = status;

	xferMsgStatus(zc->hSession, zc->pstatus + 4);

	xferMsgEvent(zc->hSession, zc->last_event + 4);

	if (status == ERROR)
		{
		zc->errors += 1;
		xferMsgErrorcnt(zc->hSession, zc->errors);
		}
	}

/*----------------------------------------------------------------------+
 | zmdmr_filecnt
 +----------------------------------------------------------------------*/
void zmdmr_filecnt(ZC *zc, int cnt)
	{
	xferMsgFilecnt(zc->hSession, cnt);
	}

/*----------------------------------------------------------------------+
 | zmdmr_totalsize
 +----------------------------------------------------------------------*/
void zmdmr_totalsize(ZC *zc, long bytes)
	{
	if (zc->nfiles >= 1)
		{
		xferMsgTotalsize(zc->hSession, bytes);
		}
	}

/*----------------------------------------------------------------------+
 | zmdmr_newfile
 +----------------------------------------------------------------------*/
void zmdmr_newfile(ZC *zc, int filen, BYTE *theirname, TCHAR *ourname)
	{
	xferMsgNewfile(zc->hSession,
				   filen,
				   theirname,
				   ourname);

	}

/*----------------------------------------------------------------------+
 | zmdmr_filesize
 +----------------------------------------------------------------------*/
void zmdmr_filesize(ZC *zc, long fsize)
	{

	if (fsize <= 0L)
		return;

	xferMsgFilesize(zc->hSession, fsize);
	}

/* End of zmodem.c */
