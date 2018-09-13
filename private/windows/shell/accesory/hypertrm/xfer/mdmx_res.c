/* File: C:\WACKER\tdll\mdmx_res.c (Created: 17-Jan-1994)
 * created from HAWIN source file
 * mdmx_res.c -- Routines to handle xmodem sending for HA5G
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>
#pragma hdrstop

#include <setjmp.h>

#define	BYTE	unsigned char

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\xfer_msc.h>
#include <tdll\file_io.h>
#include "xfr_srvc.h"
#include "xfr_todo.h"
#include "xfr_dsp.h"
#include "xfer_tsc.h"
#include "foo.h"

#include "xfer.h"
#include "xfer.hh"

#include "mdmx.h"
#include "mdmx.hh"

/*lint -e502*/				/* lint seems to want the ~ operator applied
							 *	only to unsigned, wer'e using uchar
							 */

// #pragma optimize("a", on)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * load_pckt
 *
 * DESCRIPTION:
 *	Prepares an XMODEM packet for transmission by filling it with data and
 *	initializing other fields such as
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int load_pckt(ST_MDMX *pX,
			 struct s_mdmx_pckt *p,
			 unsigned pcktnum,
			 int kpckt,
			 int chktype)
	{
	BYTE checksum;
	unsigned int crc;
	BYTE *cp;
	int cnt;
	int cc;

	p->pcktnum = (BYTE)(pcktnum % 0x100);
	p->npcktnum = (BYTE)(~p->pcktnum);
	cp = p->bdata;
	checksum = 0;
	p->result = 0;				/* will set TRUE if end of data is reached */

	for (cnt = (kpckt ? LARGE_PACKET : SMALL_PACKET); cnt > 0; --cnt)
		{
		if ((cc = (*pX->p_getc)(pX)) == EOF)
			{
			p->result = 1;			/* so compression display won't check */
#if FALSE
			/* TODO: figure out how to do this */
			if (nb_error(pX->fh))
				return FALSE;
#endif
			break;
			}
		checksum += (*cp++ = (BYTE)cc);
		}

	/* see if we're at end of file */
	if (cnt == (kpckt ? LARGE_PACKET : SMALL_PACKET))
		{
		p->start_char = EOT;
		p->pcktsize = 1;
		return TRUE;
		}

	/* if using large packets but this one is small enough, switch */
	if (kpckt && ((LARGE_PACKET-cnt) <= SMALL_PACKET))
		{
		kpckt = FALSE;						 /* set small packet flag	 */
			/* set count to 128-(1024-cnt)	*/
		cnt = (SMALL_PACKET - (LARGE_PACKET - cnt));
		}

	while (cnt-- > 0)
		{
		*cp++ = CPMEOF;
		checksum += CPMEOF;
		}

	p->start_char = (BYTE)(kpckt ? STX : SOH);
	p->pcktsize = (kpckt ? LARGE_PACKET : SMALL_PACKET) + 4;
	p->byte_count = pX->mdmx_byte_cnt;/* amt. transferred after this packet */
									/*	 is sent						  */
	if (chktype == CHECKSUM)
		{
		*cp = checksum;
		}
	else
		{
		*cp = 0;
		*(cp + 1) = 0;

		crc = calc_crc(pX, (unsigned)0, p->bdata,
				(kpckt ? LARGE_PACKET : SMALL_PACKET) + 2 );

		*cp++ = (BYTE)(crc / 0x100);
		*cp = (BYTE)(crc % 0x100);
		++p->pcktsize;
		}
	return(TRUE);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xm_getc
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int xm_getc(ST_MDMX *pX)
	{

	++pX->mdmx_byte_cnt;
	return(fio_getc(pX->fh));
	// return(nb_getc(pX->fh));
	}


// Receive routines

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
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
int xs_unload(ST_MDMX *pX, BYTE *cp, int size)
	{
	int cnt;

	for (cnt = size + 1; --cnt > 0; )
		{
		if ((*pX->p_putc)(pX, (int)*cp++) == (-1) /* ERROR */)
			return ERROR;
		}
	return 0;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * xm_putc
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int NEAR xm_putc(ST_MDMX *pX, int c)
	{

	pX->mdmx_byte_cnt += 1;
	return ((int)(fio_putc(c, pX->fh)));
	// return ((int)(nb_putc(c, pX->fh)));
	}
