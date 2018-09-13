/* File: C:\WACKER\xfer\mdmx.c (Created: 17-Jan-1994)
 * created from HAWIN source file
 * mdmx.c
 *
 * 	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * mdmx_progress
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
void mdmx_progress(ST_MDMX *pX, int status)
	{
	long ttime;
	long stime = -1;
	long etime = -1;
	long bytes_sent;
	long cps = -1;
	int  k_sent;
	// static long displayed_time = -1L;

	if (pX->xfertimer == -1L)
		return;

	ttime = bittest(status, TRANSFER_DONE) ?
			pX->xfertime : interval(pX->xfertimer);

	if ((stime = ttime / 10L) != pX->displayed_time ||
			bittest(status, FILE_DONE | TRANSFER_DONE))
		{
		bytes_sent = pX->file_bytes + pX->total_bytes;

		if (bittest(status, TRANSFER_DONE))
			k_sent = (int)PART_HUNKS(bytes_sent, 1024);
		else
			k_sent = (int)FULL_HUNKS(bytes_sent, 1024);

		if ((stime > 2 ||
			 ttime > 0 && bittest(status, FILE_DONE | TRANSFER_DONE)) &&
			(cps = (bytes_sent * 10L) / ttime) > 0)
			{
			if (pX->nbytes > 0)
				{
				etime = ((pX->nbytes - bytes_sent) / cps);
				if (pX->nfiles > 0)
					etime += pX->nfiles - pX->filen;
				}
			else if (pX->filesize > 0)
				{
				etime = ((pX->filesize - pX->file_bytes) / cps);
				}
			}
		pX->displayed_time = stime;

		mdmxdspProgress(pX,
						stime,
						etime,
						cps,
						pX->file_bytes,
						bytes_sent);

		}
	}


/* end of mdmx.c */
