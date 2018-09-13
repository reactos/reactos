/* File: C:\WACKER\xfer\krm_res.c (Created: 28-Jan-1994)
 * created from HAWIN source file
 * krm_res.c  --  Routines for handling file transmission using KERMIT
 *				file transfer protocol.
 *
 *	Copyright 1989,1991,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:22p $
 */

#include <windows.h>
#pragma hdrstop

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

#include "cmprs.h"

#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "krm.h"
#include "krm.hh"

// Resident routines for kermit receiving

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * kunload_packet
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
int kunload_packet(ST_KRM *kc, int len, unsigned char *bufr)
	{
	int sethbit = FALSE;
	unsigned char c;
	unsigned char *bp;
	unsigned char c7;
	unsigned char *limit;
	int reptcount;
	int i;

	xfer_idle(kc->hSession, XFER_IDLE_IO);

	bp = bufr;
	limit = bufr + len;
	while (bp < limit)
		{
		c = *bp++;
		if (kc->its_rept && c == kc->its_rept)
			{
			reptcount = unchar(*bp++);
			c = *bp++;
			}
		else
			reptcount = 1;
		if (kc->its_qbin && c == kc->its_qbin)
			{
			sethbit = TRUE;
			c = *bp++;
			}
		if (c == kc->its_qctl)
			{
			c = *bp++;
			c7 = (unsigned char)(c & 0x7F);
			if (c7 != kc->its_qctl && c7 != kc->its_qbin && c7 != kc->its_rept)
				c = (unsigned char)ctl(c);
			}
		if (sethbit)
			{
			c |= 0x80;
			sethbit = FALSE;
			}
		for (i = reptcount + 1; --i > 0; )
			if ((*kc->p_kputc)(kc, c) == ERROR)
				return(ERROR);
		}	/* while (bp < limit) */

	/* pointer should stop right at limit, if not, somethings wrong */
	if (bp > limit)
		return(ERROR);
	else
		return(0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * kr_putc
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
int kr_putc(ST_KRM *kc, int c)
	{

	++kc->kbytes_received;
	return ((int)(fio_putc(c, kc->fhdl)));
	}

// Resident routines for kermit sending


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * kload_packet
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int kload_packet(ST_KRM *kc, unsigned char *bufr)
	{
	char *bp = bufr;
	char *rp = bufr;
	char *limit;
	int rptcount = 1;
	int c, c7;
	int lastc = -1;
	int no8thbit = 0 /* (cnfg.bits_per_char != 8) */;

	xfer_idle(kc->hSession, XFER_IDLE_IO);

	limit = bufr + (kc->its_maxl - kc->its_chkt - 6);
	while (bp < limit)
		{
		if ((c = (*kc->p_kgetc)(kc)) == EOF)
			break;
		if (kc->its_rept)	  /* check for repeat characters */
			{
			if (c != lastc)
				{
				rptcount = 1;	/* start new check for repeat chars */
				rp = bp;
				lastc = c;
				}
			else if (++rptcount == 4)
				{
				bp = rp;		/* back up in buffer to where repeat started */
				while ((c = (*kc->p_kgetc)(kc)) == lastc && ++rptcount < 94)
					;
				if (c != lastc)
					{
					if (c != EOF)			/* note: putting the extra char. */
						{					/*	directly back into the send  */
						fio_ungetc(c, kc->fhdl);/* file buffer will not work */
						--kc->kbytes_sent;	/*	if p_kgetc is not pointing	  */
						}					/*	directly to ks_getc().		 */
					c = lastc;
					}
				lastc = -1;
				*bp++ = kc->its_rept;
				*bp++ = (char)tochar(rptcount);
				}
			}

		/* check for binary (8th bit) quoting */
		if (c & 0x80)
			{
			if (kc->its_qbin)	  /* do 8th-bit quoting */
				{
				*bp++ = kc->its_qbin;
				c &= 0x7F;
				}
			else if (no8thbit)
				{
				/* error! */
				// strcpy(kc->xtra_err, strld(TM_NOT_CNFGD));
				/* TODO: figure this error out */
				return(ERROR);
				}
			}

		/* check for characters needing control-quoting */
		c7 = (c & 0x7F);
		if (c7 < ' ' || c7 == DEL)
			{
			*bp++ = kc->its_qctl;
			c = ctl(c);
			}
		else if (c7 == (int)kc->its_qctl ||
				c7 == (int)kc->its_qbin ||
				c7 == (int)kc->its_rept)
			*bp++ = kc->its_qctl;
		*bp++ = (char)c;
		}
	if (fio_ferror(kc->fhdl))
		return(ERROR);

	return (int)(bp - bufr);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ks_getc
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
#if 1
int ks_getc(ST_KRM *kc)
	{
	++kc->kbytes_sent;
	return(fio_getc(kc->fhdl));
	}
#else
int ks_getc(ST_KRM *kc)
	{
	int c;

	++kc->kbytes_sent;
	c = fio_getc(kc->fhdl);

	DbgOutStr("%c", c, 0,0,0,0);

	return c;
	}
#endif
