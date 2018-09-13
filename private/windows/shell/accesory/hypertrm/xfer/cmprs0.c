/* File: C:\WACKER\xfer\cmprs0.c
 * created from HAWIN sources
 * cmprs0.c -- Functions common to compression and decompression
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>

#include <tdll\stdtyp.h>
#include <tdll\mc.h>

#if !defined(BYTE)
#define BYTE unsigned char
#endif

#include "cmprs.h"
#include "cmprs.hh"

// debug_init(__FILE__)


/* Routines to handle overall compression enable and disable. */

void *compress_tblspace;

/* These variables are shared by the compression and decompression routines */

unsigned long  ulHoldReg;
int            sBitsLeft;
int            sCodeBits;
unsigned int   usMaxCode;
unsigned int   usFreeCode;
unsigned int   usxCmprsStatus = COMPRESS_IDLE;
int            fxLastBuildGood = FALSE;
int            fFlushable = FALSE;		 // True if compression stream can
										 //  flushed and resumed


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * compress_enable
 *
 * DESCRIPTION:
 *	Called to determine whether compression is feasible, and if it is to
 *	allocate the necessary memory to do so.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	If compression was already enabled, returns TRUE.
 *	If compression was not previously enabled, or if it was disabled,
 *	returns TRUE if memory is available for compression, FALSE otherwise.
 */
int compress_enable(void)
	{
#if defined(DOS_HOST)
	static struct s_cmprs_node tbl[MAXNODES+2];

	compress_tblspace = (void *)&tbl;
	return TRUE;

#else

	if (compress_tblspace != (void *)0)
		return(TRUE);
	else
		{
		compress_tblspace = malloc(sizeof(struct s_cmprs_node)*(MAXNODES+2));

		return(compress_tblspace != (void *)0);
		}
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * compress_disable
 *
 * DESCRIPTION
 *	Disables file compression and released memory used for compression tables.
 *	Has no effect if compression was not enabled.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void compress_disable(void)
	{
#if !defined(DOS_HOST)
	if (compress_tblspace != (void *)0)
		free(compress_tblspace);
	compress_tblspace = (void *)0;
#endif

	usxCmprsStatus = COMPRESS_IDLE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * compress_status
 *
 * DESCRIPTION:
 *	Returns current status of compression -- whether idle, active, or shutdown.
 *	Applies to both compression and decompression.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	COMPRESS_IDLE	  if compression has not been activated or has been stopped
 *						 normally
 *	COMPRESS_ACTIVE   if compression is currently active.
 *	COMPRESS_SHUTDOWN if compression has been activated but shut itself down
 *						 upon determining that the compression is not effective
 *						 on the current file.
 */
unsigned int compress_status(void)
	{
	return usxCmprsStatus;
	}

/* end of cmprs0.c */
