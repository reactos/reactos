/*
 * File Compression Interface
 *
 * Copyright 2002 Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "fci.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

/***********************************************************************
 *		FCICreate (CABINET.10)
 */
HFCI __cdecl FCICreate(
	PERF perf,
	PFNFCIFILEPLACED   pfnfcifp,
	PFNFCIALLOC        pfna,
	PFNFCIFREE         pfnf,
	PFNFCIOPEN         pfnopen,
	PFNFCIREAD         pfnread,
	PFNFCIWRITE        pfnwrite,
	PFNFCICLOSE        pfnclose,
	PFNFCISEEK         pfnseek,
	PFNFCIDELETE       pfndelete,
	PFNFCIGETTEMPFILE  pfnfcigtf,
	PCCAB              pccab,
	void *pv)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p): stub\n",
	  perf, pfnfcifp, pfna, pfnf, pfnopen, pfnread, pfnwrite, pfnclose,
	  pfnseek, pfndelete, pfnfcigtf, pccab, pv);

    perf->erfOper = FCIERR_NONE;
    perf->erfType = 0;
    perf->fError = TRUE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		FCIAddFile (CABINET.11)
 */
BOOL __cdecl FCIAddFile(
	HFCI                  hfci,
	char                 *pszSourceFile,
	char                 *pszFileName,
	BOOL                  fExecute,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis,
	PFNFCIGETOPENINFO     pfnfcigoi,
	TCOMP                 typeCompress)
{
    FIXME("(%p, %p, %p, %d, %p, %p, %p, %hu): stub\n", hfci, pszSourceFile,
	  pszFileName, fExecute, pfnfcignc, pfnfcis, pfnfcigoi, typeCompress);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		FCIFlushCabinet (CABINET.13)
 */
BOOL __cdecl FCIFlushCabinet(
	HFCI                  hfci,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
    FIXME("(%p, %d, %p, %p): stub\n", hfci, fGetNextCab, pfnfcignc, pfnfcis);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		FCIFlushFolder (CABINET.12)
 */
BOOL __cdecl FCIFlushFolder(
	HFCI                  hfci,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
    FIXME("(%p, %p, %p): stub\n", hfci, pfnfcignc, pfnfcis);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		FCIDestroy (CABINET.14)
 */
BOOL __cdecl FCIDestroy(HFCI hfci)
{
    FIXME("(%p): stub\n", hfci);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}
