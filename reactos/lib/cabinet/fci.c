/*
 * File Compression Interface
 *
 * Copyright 2002 Patrik Stridvall
 * Copyright 2005 Gerold Jens Wucherpfennig
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
#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

/***********************************************************************
 *		FCICreate (CABINET.10)
 *
 * Provided with several callbacks,
 * returns a handle which can be used to perform operations
 * on cabinet files.
 *
 * PARAMS
 *   perf       [IO]  A pointer to an ERF structure.  When FCICreate
 *                    returns an error condition, error information may
 *                    be found here as well as from GetLastError.
 *   pfnfiledest [I]  A pointer to a function which is called when a file
 *                    is placed. Only useful for subsequent cabinet files.
 *   pfnalloc    [I]  A pointer to a function which allocates ram.  Uses
 *                    the same interface as malloc.
 *   pfnfree     [I]  A pointer to a function which frees ram.  Uses the
 *                    same interface as free.
 *   pfnopen     [I]  A pointer to a function which opens a file.  Uses
 *                    the same interface as _open.
 *   pfnread     [I]  A pointer to a function which reads from a file into
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _read
 *   pfnwrite    [I]  A pointer to a function which writes to a file from
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _write.
 *   pfnclose    [I]  A pointer to a function which closes a file handle.
 *                    Uses the same interface as _close.
 *   pfnseek     [I]  A pointer to a function which seeks in a file.
 *                    Uses the same interface as _lseek.
 *   pfndelete   [I]  A pointer to a function which deletes a file.
 *   pfnfcigtf   [I]  A pointer to a function which gets the name of a
 *                    temporary file; ignored in wine
 *   pccab       [I]  A pointer to an initialized CCAB structure
 *   pv          [I]  A pointer to an application-defined notification
 *                    function which will be passed to other FCI functions
 *                    as a parameter.
 *
 * RETURNS
 *   On success, returns an FCI handle of type HFCI.
 *   On failure, the NULL file handle is returned. Error
 *   info can be retrieved from perf.
 *
 * INCLUDES
 *   fci.h
 *
 */
HFCI __cdecl FCICreate(
	PERF perf,
	PFNFCIFILEPLACED   pfnfiledest,
	PFNFCIALLOC        pfnalloc,
	PFNFCIFREE         pfnfree,
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
  HFCI rv;

  if ((!pfnalloc) || (!pfnfree)) {
    perf->erfOper = FCIERR_NONE;
    perf->erfType = ERROR_BAD_ARGUMENTS;
    perf->fError = TRUE;

    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }

  if (!(rv = (HFCI) (*pfnalloc)(sizeof(FCI_Int)))) {
    perf->erfOper = FCIERR_ALLOC_FAIL;
    perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
    perf->fError = TRUE;

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }

  PFCI_INT(rv)->FCI_Intmagic = FCI_INT_MAGIC;
  PFCI_INT(rv)->perf = perf;
  PFCI_INT(rv)->pfnfiledest = pfnfiledest;
  PFCI_INT(rv)->pfnalloc = pfnalloc;
  PFCI_INT(rv)->pfnfree = pfnfree;
  PFCI_INT(rv)->pfnopen = pfnopen;
  PFCI_INT(rv)->pfnread = pfnread;
  PFCI_INT(rv)->pfnwrite = pfnwrite;
  PFCI_INT(rv)->pfnclose = pfnclose;
  PFCI_INT(rv)->pfnseek = pfnseek;
  PFCI_INT(rv)->pfndelete = pfndelete;
  PFCI_INT(rv)->pfnfcigtf = pfnfcigtf;
  PFCI_INT(rv)->pccab = pccab;
  PFCI_INT(rv)->pv = pv;

  /* Still mark as incomplete, because of other missing FCI* APIs */

  PFCI_INT(rv)->FCI_Intmagic = 0;
  PFDI_FREE(rv, rv);
  FIXME("(%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p): stub\n",
    perf, pfnfiledest, pfnalloc, pfnfree, pfnopen, pfnread, pfnwrite, pfnclose,
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
 *
 * Frees a handle created by FCICreate.
 * Only reason for failure would be an invalid handle.
 *
 * PARAMS
 *   hfci [I] The HFCI to free
 *
 * RETURNS
 *   TRUE for success
 *   FALSE for failure
 */
BOOL __cdecl FCIDestroy(HFCI hfci)
{
  if (REALLY_IS_FCI(hfci)) {
    PFCI_INT(hfci)->FCI_Intmagic = 0;
    PFDI_FREE(hfci, hfci);
    /*return TRUE; */
  } else {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  /* Still mark as incomplete, because of other missing FCI* APIs */
  FIXME("(%p): stub\n", hfci);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
