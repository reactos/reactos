/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * MMIO functions
 *
 * Copyright 1998 Andrew Taylor
 * Copyright 1998 Ove Kåven
 * Copyright 2000,2002 Eric Pouech
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

/* Still to be done:
 * 	+ correct handling of global/local IOProcs (and temporary IOProcs)
 *	+ mode of mmio objects is not used (read vs write vs readwrite)
 *	+ thread safeness
 *      + 32 A <=> W message mapping
 */


#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmio);

LRESULT         (*pFnMmioCallback16)(DWORD,LPMMIOINFO,UINT,LPARAM,LPARAM) /* = NULL */;

/**************************************************************************
 *               	mmioDosIOProc           		[internal]
 */
static LRESULT CALLBACK mmioDosIOProc(LPMMIOINFO lpmmioinfo, UINT uMessage,
				      LPARAM lParam1, LPARAM lParam2)
{
    LRESULT	ret = MMSYSERR_NOERROR;

    TRACE("(%p, %X, 0x%lx, 0x%lx);\n", lpmmioinfo, uMessage, lParam1, lParam2);

    switch (uMessage) {
    case MMIOM_OPEN:
	{
	    /* Parameters:
	     * lParam1 = szFileName parameter from mmioOpen
	     * lParam2 = reserved
	     * Returns: zero on success, error code on error
	     * NOTE: lDiskOffset automatically set to zero
	     */
	    LPCSTR      szFileName = (LPCSTR)lParam1;

	    if (lpmmioinfo->dwFlags & MMIO_GETTEMP) {
		FIXME("MMIO_GETTEMP not implemented\n");
		return MMIOERR_CANNOTOPEN;
	    }

	    /* if filename NULL, assume open file handle in adwInfo[0] */
	    if (szFileName) {
                OFSTRUCT    ofs;
                lpmmioinfo->adwInfo[0] = (DWORD)OpenFile(szFileName, &ofs, lpmmioinfo->dwFlags & 0xFFFF);
            }
	    if (lpmmioinfo->adwInfo[0] == (DWORD)HFILE_ERROR)
		ret = MMIOERR_CANNOTOPEN;
	}
	break;

    case MMIOM_CLOSE:
	/* Parameters:
	 * lParam1 = wFlags parameter from mmioClose
	 * lParam2 = unused
	 * Returns: zero on success, error code on error
	 */
	if (!(lParam1 & MMIO_FHOPEN))
	    _lclose((HFILE)lpmmioinfo->adwInfo[0]);
	break;

    case MMIOM_READ:
	/* Parameters:
	 * lParam1 = huge pointer to read buffer
	 * lParam2 = number of bytes to read
	 * Returns: number of bytes read, 0 for EOF, -1 for error (error code
	 *	   in wErrorRet)
	 */
	ret = _lread((HFILE)lpmmioinfo->adwInfo[0], (HPSTR)lParam1, (LONG)lParam2);
	if (ret != -1)
	    lpmmioinfo->lDiskOffset += ret;

	break;

    case MMIOM_WRITE:
    case MMIOM_WRITEFLUSH:
	/* no internal buffering, so WRITEFLUSH handled same as WRITE */

	/* Parameters:
	 * lParam1 = huge pointer to write buffer
	 * lParam2 = number of bytes to write
	 * Returns: number of bytes written, -1 for error (error code in
	 *		wErrorRet)
	 */
	ret = _hwrite((HFILE)lpmmioinfo->adwInfo[0], (HPSTR)lParam1, (LONG)lParam2);
	if (ret != -1)
	    lpmmioinfo->lDiskOffset += ret;
	break;

    case MMIOM_SEEK:
	/* Parameters:
	 * lParam1 = new position
	 * lParam2 = from whence to seek (SEEK_SET, SEEK_CUR, SEEK_END)
	 * Returns: new file postion, -1 on error
	 */
	ret = _llseek((HFILE)lpmmioinfo->adwInfo[0], (LONG)lParam1, (LONG)lParam2);
	if (ret != -1)
	    lpmmioinfo->lDiskOffset = ret;
	return ret;

    case MMIOM_RENAME:
	/* Parameters:
	 * lParam1 = old name
	 * lParam2 = new name
	 * Returns: zero on success, non-zero on failure
	 */
 	 if (!MoveFileA((const char*)lParam1, (const char*)lParam2))
	     ret = MMIOERR_FILENOTFOUND;
	 break;

    default:
	FIXME("unexpected message %u\n", uMessage);
	return 0;
    }

    return ret;
}

/**************************************************************************
 *               	mmioMemIOProc           		[internal]
 */
static LRESULT CALLBACK mmioMemIOProc(LPMMIOINFO lpmmioinfo, UINT uMessage,
				      LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%p,0x%04x,0x%08lx,0x%08lx)\n", lpmmioinfo, uMessage, lParam1, lParam2);

    switch (uMessage) {

    case MMIOM_OPEN:
	/* Parameters:
	 * lParam1 = filename (must be NULL)
	 * lParam2 = reserved
	 * Returns: zero on success, error code on error
	 * NOTE: lDiskOffset automatically set to zero
	 */
	/* FIXME: io proc shouldn't change it */
	if (!(lpmmioinfo->dwFlags & MMIO_CREATE))
	    lpmmioinfo->pchEndRead = lpmmioinfo->pchEndWrite;
        lpmmioinfo->adwInfo[0] = HFILE_ERROR;
	return 0;

    case MMIOM_CLOSE:
	/* Parameters:
	 * lParam1 = wFlags parameter from mmioClose
	 * lParam2 = unused
	 * Returns: zero on success, error code on error
	 */
	return 0;

    case MMIOM_READ:
	/* Parameters:
	 * lParam1 = huge pointer to read buffer
	 * lParam2 = number of bytes to read
	 * Returns: number of bytes read, 0 for EOF, -1 for error (error code
	 *	   in wErrorRet)
	 * NOTE: lDiskOffset should be updated
	 */
	FIXME("MMIOM_READ on memory files should not occur, buffer may be lost!\n");
	return 0;

    case MMIOM_WRITE:
    case MMIOM_WRITEFLUSH:
	/* no internal buffering, so WRITEFLUSH handled same as WRITE */

	/* Parameters:
	 * lParam1 = huge pointer to write buffer
	 * lParam2 = number of bytes to write
	 * Returns: number of bytes written, -1 for error (error code in
	 *		wErrorRet)
	 * NOTE: lDiskOffset should be updated
	 */
	FIXME("MMIOM_WRITE on memory files should not occur, buffer may be lost!\n");
	return 0;

    case MMIOM_SEEK:
	/* Parameters:
	 * lParam1 = new position
	 * lParam2 = from whence to seek (SEEK_SET, SEEK_CUR, SEEK_END)
	 * Returns: new file postion, -1 on error
	 * NOTE: lDiskOffset should be updated
	 */
	FIXME("MMIOM_SEEK on memory files should not occur, buffer may be lost!\n");
	return -1;

    default:
	FIXME("unexpected message %u\n", uMessage);
	return 0;
    }

    return 0;
}

/* This array will be the entire list for most apps 
 * Note that temporary ioProcs will be stored with a 0 fourCC code
 */

static struct IOProcList defaultProcs[] = {
    {&defaultProcs[1], FOURCC_DOS, (LPMMIOPROC)mmioDosIOProc, MMIO_PROC_32A, 0},
    {NULL,             FOURCC_MEM, (LPMMIOPROC)mmioMemIOProc, MMIO_PROC_32A, 0},
};

static struct IOProcList*	pIOProcListAnchor = &defaultProcs[0];

/****************************************************************
 *       	MMIO_FindProcNode 			[INTERNAL]
 *
 * Finds the ProcList node associated with a given FOURCC code.
 */
static struct IOProcList*	MMIO_FindProcNode(FOURCC fccIOProc)
{
    struct IOProcList*	pListNode;

    for (pListNode = pIOProcListAnchor; pListNode; pListNode = pListNode->pNext) {
	if (pListNode->fourCC == fccIOProc) {
	    return pListNode;
	}
    }
    return NULL;
}

/****************************************************************
 *       	MMIO_InstallIOProc 			[INTERNAL]
 */
LPMMIOPROC MMIO_InstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc,
                              DWORD dwFlags, enum mmioProcType type)
{
    LPMMIOPROC	        lpProc = NULL;
    struct IOProcList*  pListNode;
    struct IOProcList** ppListNode;

    TRACE("(%08lx, %p, %08lX, %i)\n", fccIOProc, pIOProc, dwFlags, type);

    if (dwFlags & MMIO_GLOBALPROC)
	FIXME("Global procedures not implemented\n");

    /* just handle the known procedures for now */
    switch (dwFlags & (MMIO_INSTALLPROC|MMIO_REMOVEPROC|MMIO_FINDPROC)) {
    case MMIO_INSTALLPROC:
	/* Create new entry for the IOProc list */
	pListNode = HeapAlloc(GetProcessHeap(), 0, sizeof(*pListNode));
	if (pListNode) {
	    /* Fill in this node */
	    pListNode->fourCC = fccIOProc;
	    pListNode->pIOProc = pIOProc;
	    pListNode->type = type;
	    pListNode->count = 0;

	    /* Stick it on the end of the list */
	    pListNode->pNext = pIOProcListAnchor;
	    pIOProcListAnchor = pListNode;

	    /* Return this IOProc - that's how the caller knows we succeeded */
	    lpProc = pIOProc;
	}
	break;

    case MMIO_REMOVEPROC:
	/*
	 * Search for the node that we're trying to remove
         * We search for a matching fourCC code if it's non null, or the proc
         * address otherwise
         * note that this method won't find the first item on the list, but
	 * since the first two items on this list are ones we won't
	 * let the user delete anyway, that's okay
	 */
	ppListNode = &pIOProcListAnchor;
	while ((*ppListNode) && 
               ((fccIOProc != 0) ? 
                (*ppListNode)->fourCC != fccIOProc : 
                (*ppListNode)->pIOProc != pIOProc))
	    ppListNode = &((*ppListNode)->pNext);

	if (*ppListNode) { /* found it */
	    /* FIXME: what should be done if an open mmio object uses this proc ?
	     * shall we return an error, nuke the mmio object ?
	     */
	    if ((*ppListNode)->count) {
		ERR("Cannot remove a mmIOProc while in use\n");
		break;
	    }
	    /* remove it, but only if it isn't builtin */
	    if ((*ppListNode) >= defaultProcs &&
		(*ppListNode) < defaultProcs + sizeof(defaultProcs)) {
		WARN("Tried to remove built-in mmio proc. Skipping\n");
	    } else {
		/* Okay, nuke it */
		struct IOProcList*  ptmpNode = *ppListNode;
		lpProc = (*ppListNode)->pIOProc;
		*ppListNode = (*ppListNode)->pNext;
		HeapFree(GetProcessHeap(), 0, ptmpNode);
	    }
	}
	break;

    case MMIO_FINDPROC:
	if ((pListNode = MMIO_FindProcNode(fccIOProc))) {
	    lpProc = pListNode->pIOProc;
	}
	break;
    }

    return lpProc;
}

/****************************************************************
 *       	send_message    			[INTERNAL]
 */
static LRESULT	send_message(struct IOProcList* ioProc, LPMMIOINFO mmioinfo,
                             DWORD wMsg, LPARAM lParam1,
                             LPARAM lParam2, enum mmioProcType type)
{
    LRESULT 		result = MMSYSERR_ERROR;
    LPARAM		lp1 = lParam1, lp2 = lParam2;

    if (!ioProc) {
	ERR("brrr\n");
	result = MMSYSERR_INVALPARAM;
    }

    switch (ioProc->type) {
    case MMIO_PROC_16:
        if (pFnMmioCallback16)
            result = pFnMmioCallback16((DWORD)ioProc->pIOProc,
                                       mmioinfo, wMsg, lp1, lp2);
        break;
    case MMIO_PROC_32A:
    case MMIO_PROC_32W:
	if (ioProc->type != type) {
	    /* map (lParam1, lParam2) into (lp1, lp2) 32 A<=>W */
	    FIXME("NIY 32 A<=>W mapping\n");
	}
	result = (ioProc->pIOProc)((LPSTR)mmioinfo, wMsg, lp1, lp2);

#if 0
	if (ioProc->type != type) {
	    /* unmap (lParam1, lParam2) into (lp1, lp2) 32 A<=>W */
	}
#endif
	break;
    default:
	FIXME("Internal error\n");
    }

    return result;
}

/**************************************************************************
 *      			MMIO_ParseExtA 		        [internal]
 *
 * Parses a filename for the extension.
 *
 * RETURNS
 *  The FOURCC code for the extension if found, else 0.
 */
static FOURCC MMIO_ParseExtA(LPCSTR szFileName)
{
    /* Filenames are of the form file.ext{+ABC}
       For now, we take the last '+' if present */

    FOURCC ret = 0;

    /* Note that ext{Start,End} point to the . and + respectively */
    LPSTR extEnd;
    LPSTR extStart;

    TRACE("(%s)\n", debugstr_a(szFileName));

    if (!szFileName)
	return ret;

    /* Find the last '.' */
    extStart = strrchr(szFileName,'.');

    if (!extStart) {
         ERR("No . in szFileName: %s\n", debugstr_a(szFileName));
    } else {
        CHAR ext[5];

        /* Find the '+' afterwards */
        extEnd = strchr(extStart,'+');
        if (extEnd) {

            if (extEnd - extStart - 1 > 4)
                WARN("Extension length > 4\n");
            lstrcpynA(ext, extStart + 1, min(extEnd-extStart,5));

        } else {
            /* No + so just an extension */
            if (strlen(extStart) > 4) {
                WARN("Extension length > 4\n");
            }
            lstrcpynA(ext, extStart + 1, 5);
        }
        TRACE("Got extension: %s\n", debugstr_a(ext));

        /* FOURCC codes identifying file-extensions must be uppercase */
        ret = mmioStringToFOURCCA(ext, MMIO_TOUPPER);
    }
    return ret;
}

/**************************************************************************
 *				MMIO_Get			[internal]
 *
 * Retrieves the mmio object from current process
 */
LPWINE_MMIO	MMIO_Get(HMMIO h)
{
    LPWINE_MMIO		wm = NULL;

    EnterCriticalSection(&WINMM_IData->cs);
    for (wm = WINMM_IData->lpMMIO; wm; wm = wm->lpNext) {
	if (wm->info.hmmio == h)
	    break;
    }
    LeaveCriticalSection(&WINMM_IData->cs);
    return wm;
}

/**************************************************************************
 *				MMIO_Create			[internal]
 *
 * Creates an internal representation for a mmio instance
 */
static	LPWINE_MMIO		MMIO_Create(void)
{
    static	WORD	MMIO_counter = 0;
    LPWINE_MMIO		wm;

    wm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MMIO));
    if (wm) {
	EnterCriticalSection(&WINMM_IData->cs);
        /* lookup next unallocated WORD handle, with a non NULL value */
	while (++MMIO_counter == 0 || MMIO_Get((HMMIO)(ULONG_PTR)MMIO_counter));
	wm->info.hmmio = (HMMIO)(ULONG_PTR)MMIO_counter;
	wm->lpNext = WINMM_IData->lpMMIO;
	WINMM_IData->lpMMIO = wm;
	LeaveCriticalSection(&WINMM_IData->cs);
    }
    return wm;
}

/**************************************************************************
 *				MMIO_Destroy			[internal]
 *-
 * Destroys an internal representation for a mmio instance
 */
static	BOOL		MMIO_Destroy(LPWINE_MMIO wm)
{
    LPWINE_MMIO*	m;

    EnterCriticalSection(&WINMM_IData->cs);
    /* search for the matching one... */
    m = &(WINMM_IData->lpMMIO);
    while (*m && *m != wm) m = &(*m)->lpNext;
    /* ...and destroy */
    if (*m) {
	*m = (*m)->lpNext;
	HeapFree(GetProcessHeap(), 0, wm);
	wm = NULL;
    }
    LeaveCriticalSection(&WINMM_IData->cs);
    return wm ? FALSE : TRUE;
}

/****************************************************************
 *       		MMIO_Flush 			[INTERNAL]
 */
static	MMRESULT MMIO_Flush(WINE_MMIO* wm, UINT uFlags)
{
    if (wm->info.cchBuffer && (wm->info.fccIOProc != FOURCC_MEM)) {
	/* not quite sure what to do here, but I'll guess */
	if (wm->info.dwFlags & MMIO_DIRTY) {
            /* FIXME: error handling */
	    send_message(wm->ioProc, &wm->info, MMIOM_SEEK, 
                         wm->info.lBufOffset, SEEK_SET, MMIO_PROC_32A);
	    send_message(wm->ioProc, &wm->info, MMIOM_WRITE, 
                         (LPARAM)wm->info.pchBuffer,
                         wm->info.pchNext - wm->info.pchBuffer, MMIO_PROC_32A);
	}
	if (uFlags & MMIO_EMPTYBUF)
	    wm->info.pchNext = wm->info.pchEndRead = wm->info.pchBuffer;
    }
    wm->info.dwFlags &= ~MMIO_DIRTY;

    return MMSYSERR_NOERROR;
}

/***************************************************************************
 *       		MMIO_GrabNextBuffer			[INTERNAL]
 */
static LONG	MMIO_GrabNextBuffer(LPWINE_MMIO wm, int for_read)
{
    LONG	size = wm->info.cchBuffer;

    TRACE("bo=%lx do=%lx of=%lx\n",
	  wm->info.lBufOffset, wm->info.lDiskOffset,
	  send_message(wm->ioProc, &wm->info, MMIOM_SEEK, 0, SEEK_CUR, MMIO_PROC_32A));

    wm->info.lBufOffset = wm->info.lDiskOffset;
    wm->info.pchNext = wm->info.pchBuffer;
    wm->info.pchEndRead = wm->info.pchBuffer;
    wm->info.pchEndWrite = wm->info.pchBuffer + wm->info.cchBuffer;

    wm->bBufferLoaded = TRUE;
    if (for_read) {
	size = send_message(wm->ioProc, &wm->info, MMIOM_READ, 
                            (LPARAM)wm->info.pchBuffer, size, MMIO_PROC_32A);
	if (size > 0)
	    wm->info.pchEndRead += size;
        else
            wm->bBufferLoaded = FALSE;
    }

    return size;
}

/***************************************************************************
 *       		MMIO_SetBuffer 				[INTERNAL]
 */
static MMRESULT MMIO_SetBuffer(WINE_MMIO* wm, void* pchBuffer, LONG cchBuffer,
			       UINT uFlags)
{
    TRACE("(%p %p %ld %u)\n", wm, pchBuffer, cchBuffer, uFlags);

    if (uFlags)			return MMSYSERR_INVALPARAM;
    if (cchBuffer > 0xFFFF)
	WARN("Untested handling of huge mmio buffers (%ld >= 64k)\n", cchBuffer);

    if (MMIO_Flush(wm, 0) != MMSYSERR_NOERROR)
	return MMIOERR_CANNOTWRITE;

    /* free previous buffer if allocated */
    if (wm->info.dwFlags & MMIO_ALLOCBUF) {
        HeapFree(GetProcessHeap(), 0, wm->info.pchBuffer);
        wm->info.pchBuffer = NULL;
	wm->info.dwFlags &= ~MMIO_ALLOCBUF;
    }

    if (pchBuffer) {
        wm->info.pchBuffer = pchBuffer;
    } else if (cchBuffer) {
	if (!(wm->info.pchBuffer = HeapAlloc(GetProcessHeap(), 0, cchBuffer)))
	    return MMIOERR_OUTOFMEMORY;
	wm->info.dwFlags |= MMIO_ALLOCBUF;
    } else {
	wm->info.pchBuffer = NULL;
    }

    wm->info.cchBuffer = cchBuffer;
    wm->info.pchNext = wm->info.pchBuffer;
    wm->info.pchEndRead = wm->info.pchBuffer;
    wm->info.pchEndWrite = wm->info.pchBuffer + cchBuffer;
    wm->info.lBufOffset = 0;
    wm->bBufferLoaded = FALSE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			MMIO_Open       			[internal]
 */
HMMIO MMIO_Open(LPSTR szFileName, MMIOINFO* refmminfo, DWORD dwOpenFlags, 
                enum mmioProcType type)
{
    LPWINE_MMIO		wm;
    MMIOINFO    	mmioinfo;

    TRACE("('%s', %p, %08lX, %d);\n", szFileName, refmminfo, dwOpenFlags, type);

    if (!refmminfo) {
        refmminfo = &mmioinfo;

	mmioinfo.fccIOProc = 0;
	mmioinfo.pIOProc = NULL;
	mmioinfo.pchBuffer = NULL;
	mmioinfo.cchBuffer = 0;
        type = MMIO_PROC_32A;
    }

    if (dwOpenFlags & (MMIO_PARSE|MMIO_EXIST)) {
	char	buffer[MAX_PATH];

	if (GetFullPathNameA(szFileName, sizeof(buffer), buffer, NULL) >= sizeof(buffer))
	    return (HMMIO)FALSE;
	if ((dwOpenFlags & MMIO_EXIST) && (GetFileAttributesA(buffer) == INVALID_FILE_ATTRIBUTES))
	    return (HMMIO)FALSE;
	strcpy(szFileName, buffer);
	return (HMMIO)TRUE;
    }

    if ((wm = MMIO_Create()) == NULL)
	return 0;

    /* If both params are NULL, then parse the file name if available */
    if (refmminfo->fccIOProc == 0 && refmminfo->pIOProc == NULL) {
	wm->info.fccIOProc = MMIO_ParseExtA(szFileName);
	/* Handle any unhandled/error case. Assume DOS file */
	if (wm->info.fccIOProc == 0)
	    wm->info.fccIOProc = FOURCC_DOS;
	if (!(wm->ioProc = MMIO_FindProcNode(wm->info.fccIOProc))) {
	    /* If not found, retry with FOURCC_DOS */
	    wm->info.fccIOProc = FOURCC_DOS;
	    if (!(wm->ioProc = MMIO_FindProcNode(wm->info.fccIOProc)))
		goto error2;
	}
	wm->bTmpIOProc = FALSE;
    }
    /* if just the four character code is present, look up IO proc */
    else if (refmminfo->pIOProc == NULL) {
	wm->info.fccIOProc = refmminfo->fccIOProc;
	if (!(wm->ioProc = MMIO_FindProcNode(wm->info.fccIOProc))) goto error2;
	wm->bTmpIOProc = FALSE;
    }
    /* if IO proc specified, use it and specified four character code */
    else {
	wm->info.fccIOProc = refmminfo->fccIOProc;
	MMIO_InstallIOProc(wm->info.fccIOProc, refmminfo->pIOProc,
                           MMIO_INSTALLPROC, type);
	if (!(wm->ioProc = MMIO_FindProcNode(wm->info.fccIOProc))) goto error2;
	assert(wm->ioProc->pIOProc == refmminfo->pIOProc);
	wm->bTmpIOProc = TRUE;
    }

    wm->bBufferLoaded = FALSE;
    wm->ioProc->count++;

    if (dwOpenFlags & MMIO_ALLOCBUF) {
	if ((refmminfo->wErrorRet = MMIO_SetBuffer(wm, NULL, MMIO_DEFAULTBUFFER, 0)))
	    goto error1;
    } else if (wm->info.fccIOProc == FOURCC_MEM) {
        refmminfo->wErrorRet = MMIO_SetBuffer(wm, refmminfo->pchBuffer, refmminfo->cchBuffer, 0);
	if (refmminfo->wErrorRet != MMSYSERR_NOERROR)
	    goto error1;
	wm->bBufferLoaded = TRUE;
    } /* else => unbuffered, wm->info.pchBuffer == NULL */

    /* see mmioDosIOProc for that one */
    wm->info.adwInfo[0] = refmminfo->adwInfo[0];
    wm->info.dwFlags = dwOpenFlags;

    /* call IO proc to actually open file */
    refmminfo->wErrorRet = send_message(wm->ioProc, &wm->info, MMIOM_OPEN, 
                                        (LPARAM)szFileName, 0, MMIO_PROC_32A);

    /* grab file size, when possible */
    wm->dwFileSize = GetFileSize((HANDLE)wm->info.adwInfo[0], NULL);

    if (refmminfo->wErrorRet == 0)
	return wm->info.hmmio;
 error1:
    if (wm->ioProc) wm->ioProc->count--;
 error2:
    MMIO_Destroy(wm);
    return 0;
}

/**************************************************************************
 * 				mmioOpenW       		[WINMM.@]
 */
HMMIO WINAPI mmioOpenW(LPWSTR szFileName, MMIOINFO* lpmmioinfo,
		       DWORD dwOpenFlags)
{
    HMMIO 	ret;
    LPSTR	szFn = NULL;

    if (szFileName)
    {
        INT     len = WideCharToMultiByte( CP_ACP, 0, szFileName, -1, NULL, 0, NULL, NULL );
        szFn = HeapAlloc( GetProcessHeap(), 0, len );
        if (!szFn) return (HMMIO)NULL;
        WideCharToMultiByte( CP_ACP, 0, szFileName, -1, szFn, len, NULL, NULL );
    }

    ret = MMIO_Open(szFn, lpmmioinfo, dwOpenFlags, MMIO_PROC_32W);

    HeapFree(GetProcessHeap(), 0, szFn);
    return ret;
}

/**************************************************************************
 * 				mmioOpenA       		[WINMM.@]
 */
HMMIO WINAPI mmioOpenA(LPSTR szFileName, MMIOINFO* lpmmioinfo,
		       DWORD dwOpenFlags)
{
    return  MMIO_Open(szFileName, lpmmioinfo, dwOpenFlags, MMIO_PROC_32A);
}

/**************************************************************************
 * 				mmioClose      		[WINMM.@]
 */
MMRESULT WINAPI mmioClose(HMMIO hmmio, UINT uFlags)
{
    LPWINE_MMIO	wm;
    MMRESULT 	result;

    TRACE("(%p, %04X);\n", hmmio, uFlags);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMIO_Flush(wm, 0)) != MMSYSERR_NOERROR)
	return result;

    result = send_message(wm->ioProc, &wm->info, MMIOM_CLOSE, 
                          uFlags, 0, MMIO_PROC_32A);

    MMIO_SetBuffer(wm, NULL, 0, 0);

    wm->ioProc->count--;

    if (wm->bTmpIOProc)
	MMIO_InstallIOProc(wm->info.fccIOProc, wm->ioProc->pIOProc,
                           MMIO_REMOVEPROC, wm->ioProc->type);

    MMIO_Destroy(wm);

    return result;
}

/**************************************************************************
 * 				mmioRead	       	[WINMM.@]
 */
LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
    LPWINE_MMIO	wm;
    LONG 	count;

    TRACE("(%p, %p, %ld);\n", hmmio, pch, cch);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return -1;

    /* unbuffered case first */
    if (!wm->info.pchBuffer)
	return send_message(wm->ioProc, &wm->info, MMIOM_READ, 
                            (LPARAM)pch, cch, MMIO_PROC_32A);

    /* first try from current buffer */
    if (wm->info.pchNext != wm->info.pchEndRead) {
	count = wm->info.pchEndRead - wm->info.pchNext;
	if (count > cch || count < 0) count = cch;
	memcpy(pch, wm->info.pchNext, count);
	wm->info.pchNext += count;
	pch += count;
	cch -= count;
    } else
	count = 0;

    if (cch && (wm->info.fccIOProc != FOURCC_MEM)) {
	assert(wm->info.cchBuffer);

	while (cch) {
	    LONG size;

	    size = MMIO_GrabNextBuffer(wm, TRUE);
	    if (size <= 0) break;
	    if (size > cch) size = cch;
	    memcpy(pch, wm->info.pchBuffer, size);
	    wm->info.pchNext += size;
	    pch += size;
	    cch -= size;
	    count += size;
	}
    }

    TRACE("count=%ld\n", count);
    return count;
}

/**************************************************************************
 * 				mmioWrite      		[WINMM.@]
 */
LONG WINAPI mmioWrite(HMMIO hmmio, HPCSTR pch, LONG cch)
{
    LPWINE_MMIO	wm;
    LONG	count;

    TRACE("(%p, %p, %ld);\n", hmmio, pch, cch);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return -1;

    if (wm->info.cchBuffer) {
	LONG	bytesW = 0;

        count = 0;
        while (cch) {
            if (wm->info.pchNext != wm->info.pchEndWrite) {
                count = wm->info.pchEndWrite - wm->info.pchNext;
                if (count > cch || count < 0) count = cch;
                memcpy(wm->info.pchNext, pch, count);
                wm->info.pchNext += count;
                pch += count;
                cch -= count;
                bytesW += count;
                wm->info.dwFlags |= MMIO_DIRTY;
	    } else {
                if (wm->info.fccIOProc == FOURCC_MEM) {
                    if (wm->info.adwInfo[0]) {
                        /* from where would we get the memory handle? */
                        FIXME("memory file expansion not implemented!\n");
                        break;
		    } else break;
                }
            }

            if (wm->info.pchNext == wm->info.pchEndWrite)
            {
                MMIO_Flush(wm, MMIO_EMPTYBUF);
                MMIO_GrabNextBuffer(wm, FALSE);
            }
            else break;
        }
	count = bytesW;
    } else {
	count = send_message(wm->ioProc, &wm->info, MMIOM_WRITE, 
                             (LPARAM)pch, cch, MMIO_PROC_32A);
	wm->info.lBufOffset = wm->info.lDiskOffset;
    }

    TRACE("bytes written=%ld\n", count);
    return count;
}

/**************************************************************************
 * 				mmioSeek		[WINMM.@]
 */
LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, INT iOrigin)
{
    LPWINE_MMIO	wm;
    LONG 	offset;

    TRACE("(%p, %08lX, %d);\n", hmmio, lOffset, iOrigin);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    /* not buffered, direct seek on file */
    if (!wm->info.pchBuffer)
	return send_message(wm->ioProc, &wm->info, MMIOM_SEEK, 
                            lOffset, iOrigin, MMIO_PROC_32A);

    switch (iOrigin) {
    case SEEK_SET:
	offset = lOffset;
	break;
    case SEEK_CUR:
	offset = wm->info.lBufOffset + (wm->info.pchNext - wm->info.pchBuffer) + lOffset;
	break;
    case SEEK_END:
	offset = ((wm->info.fccIOProc == FOURCC_MEM)? wm->info.cchBuffer : wm->dwFileSize) - lOffset;
	break;
    default:
	return -1;
    }

    if (offset && offset >= wm->dwFileSize && wm->info.fccIOProc != FOURCC_MEM) {
        /* should check that write mode exists */
        if (MMIO_Flush(wm, 0) != MMSYSERR_NOERROR)
            return -1;
        wm->info.lBufOffset = offset;
        wm->info.pchEndRead = wm->info.pchBuffer;
        wm->info.pchEndWrite = wm->info.pchBuffer + wm->info.cchBuffer;
        if ((wm->info.dwFlags & MMIO_RWMODE) == MMIO_READ) {
            wm->info.lDiskOffset = wm->dwFileSize;
        }
    } else if ((wm->info.cchBuffer > 0) &&
	((offset < wm->info.lBufOffset) ||
	 (offset >= wm->info.lBufOffset + wm->info.cchBuffer) ||
	 !wm->bBufferLoaded)) {
        /* stay in same buffer ? */
        /* some memory mapped buffers are defined with -1 as a size */

	/* condition to change buffer */
	if ((wm->info.fccIOProc == FOURCC_MEM) ||
	    MMIO_Flush(wm, 0) != MMSYSERR_NOERROR ||
	    /* this also sets the wm->info.lDiskOffset field */
	    send_message(wm->ioProc, &wm->info, MMIOM_SEEK,
                         (offset / wm->info.cchBuffer) * wm->info.cchBuffer,
                         SEEK_SET, MMIO_PROC_32A) == -1)
	    return -1;
	MMIO_GrabNextBuffer(wm, TRUE);
    }

    wm->info.pchNext = wm->info.pchBuffer + (offset - wm->info.lBufOffset);

    TRACE("=> %ld\n", offset);
    return offset;
}

/**************************************************************************
 * 				mmioGetInfo	       	[WINMM.@]
 */
MMRESULT WINAPI mmioGetInfo(HMMIO hmmio, MMIOINFO* lpmmioinfo, UINT uFlags)
{
    LPWINE_MMIO		wm;

    TRACE("(%p,%p,0x%08x)\n",hmmio,lpmmioinfo,uFlags);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    memcpy(lpmmioinfo, &wm->info, sizeof(MMIOINFO));
    /* don't expose 16 bit ioproc:s */
    if (wm->ioProc->type != MMIO_PROC_16)
        lpmmioinfo->pIOProc = wm->ioProc->pIOProc;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioSetInfo    		[WINMM.@]
 */
MMRESULT WINAPI mmioSetInfo(HMMIO hmmio, const MMIOINFO* lpmmioinfo, UINT uFlags)
{
    LPWINE_MMIO		wm;

    TRACE("(%p,%p,0x%08x)\n",hmmio,lpmmioinfo,uFlags);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    /* check pointers coherence */
    if (lpmmioinfo->pchNext < wm->info.pchBuffer ||
	lpmmioinfo->pchNext > wm->info.pchBuffer + wm->info.cchBuffer ||
	lpmmioinfo->pchEndRead < wm->info.pchBuffer ||
	lpmmioinfo->pchEndRead > wm->info.pchBuffer + wm->info.cchBuffer ||
	lpmmioinfo->pchEndWrite < wm->info.pchBuffer ||
	lpmmioinfo->pchEndWrite > wm->info.pchBuffer + wm->info.cchBuffer)
	return MMSYSERR_INVALPARAM;

    wm->info.pchNext = lpmmioinfo->pchNext;
    wm->info.pchEndRead = lpmmioinfo->pchEndRead;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				mmioSetBuffer		[WINMM.@]
*/
MMRESULT WINAPI mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer, UINT uFlags)
{
    LPWINE_MMIO		wm;

    TRACE("(hmmio=%p, pchBuf=%p, cchBuf=%ld, uFlags=%#08x)\n",
	  hmmio, pchBuffer, cchBuffer, uFlags);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMIO_SetBuffer(wm, pchBuffer, cchBuffer, uFlags);
}

/**************************************************************************
 * 				mmioFlush      		[WINMM.@]
 */
MMRESULT WINAPI mmioFlush(HMMIO hmmio, UINT uFlags)
{
    LPWINE_MMIO		wm;

    TRACE("(%p, %04X)\n", hmmio, uFlags);

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMIO_Flush(wm, uFlags);
}

/**************************************************************************
 * 				mmioAdvance      	[WINMM.@]
 */
MMRESULT WINAPI mmioAdvance(HMMIO hmmio, MMIOINFO* lpmmioinfo, UINT uFlags)
{
    LPWINE_MMIO		wm;

    TRACE("hmmio=%p, lpmmioinfo=%p, uFlags=%04X\n", hmmio, lpmmioinfo, uFlags);

    /* NOTE: mmioAdvance16 heavily relies on parameters from lpmmioinfo we're using
     * here. be sure if you change something here to check mmioAdvance16 as well
     */
    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if (!wm->info.cchBuffer)
	return MMIOERR_UNBUFFERED;

    if (uFlags != MMIO_READ && uFlags != MMIO_WRITE)
	return MMSYSERR_INVALPARAM;

    if (uFlags == MMIO_WRITE && (lpmmioinfo->dwFlags & MMIO_DIRTY))
    {
        send_message(wm->ioProc, &wm->info, MMIOM_SEEK, 
                     lpmmioinfo->lBufOffset, SEEK_SET, MMIO_PROC_32A);
        send_message(wm->ioProc, &wm->info, MMIOM_WRITE, 
                     (LPARAM)lpmmioinfo->pchBuffer,
                     lpmmioinfo->pchNext - lpmmioinfo->pchBuffer, MMIO_PROC_32A);
        lpmmioinfo->dwFlags &= ~MMIO_DIRTY;
    }
    if (MMIO_Flush(wm, 0) != MMSYSERR_NOERROR)
	return MMIOERR_CANNOTWRITE;

    if (lpmmioinfo) {
	wm->dwFileSize = max(wm->dwFileSize, lpmmioinfo->lBufOffset + 
                             (lpmmioinfo->pchNext - lpmmioinfo->pchBuffer));
    }
    MMIO_GrabNextBuffer(wm, uFlags == MMIO_READ);

    if (lpmmioinfo) {
	lpmmioinfo->pchNext = lpmmioinfo->pchBuffer;
	lpmmioinfo->pchEndRead  = lpmmioinfo->pchBuffer +
	    (wm->info.pchEndRead - wm->info.pchBuffer);
	lpmmioinfo->pchEndWrite = lpmmioinfo->pchBuffer +
	    (wm->info.pchEndWrite - wm->info.pchBuffer);
	lpmmioinfo->lDiskOffset = wm->info.lDiskOffset;
	lpmmioinfo->lBufOffset = wm->info.lBufOffset;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioStringToFOURCCA	[WINMM.@]
 */
FOURCC WINAPI mmioStringToFOURCCA(LPCSTR sz, UINT uFlags)
{
    CHAR cc[4];
    int i = 0;

    for (i = 0; i < 4 && sz[i]; i++) {
	if (uFlags & MMIO_TOUPPER) {
	    cc[i] = toupper(sz[i]);
	} else {
	    cc[i] = sz[i];
	}
    }

    /* Pad with spaces */
    while (i < 4) cc[i++] = ' ';

    TRACE("Got '%.4s'\n",cc);
    return mmioFOURCC(cc[0],cc[1],cc[2],cc[3]);
}

/**************************************************************************
 * 				mmioStringToFOURCCW	[WINMM.@]
 */
FOURCC WINAPI mmioStringToFOURCCW(LPCWSTR sz, UINT uFlags)
{
    char	szA[4];

    WideCharToMultiByte( CP_ACP, 0, sz, 4, szA, sizeof(szA), NULL, NULL );
    return mmioStringToFOURCCA(szA,uFlags);
}

/**************************************************************************
 * 				mmioInstallIOProcA	   [WINMM.@]
 */
LPMMIOPROC WINAPI mmioInstallIOProcA(FOURCC fccIOProc,
				     LPMMIOPROC pIOProc, DWORD dwFlags)
{
    return MMIO_InstallIOProc(fccIOProc, pIOProc, dwFlags, MMIO_PROC_32A);
}

/**************************************************************************
 * 				mmioInstallIOProcW	   [WINMM.@]
 */
LPMMIOPROC WINAPI mmioInstallIOProcW(FOURCC fccIOProc,
				     LPMMIOPROC pIOProc, DWORD dwFlags)
{
    return MMIO_InstallIOProc(fccIOProc, pIOProc, dwFlags, MMIO_PROC_32W);
}

/******************************************************************
 *		MMIO_SendMessage
 *
 *
 */
LRESULT         MMIO_SendMessage(HMMIO hmmio, UINT uMessage, LPARAM lParam1, 
                                 LPARAM lParam2, enum mmioProcType type)
{
    LPWINE_MMIO		wm;

    TRACE("(%p, %u, %ld, %ld, %d)\n", hmmio, uMessage, lParam1, lParam2, type);

    if (uMessage < MMIOM_USER)
	return MMSYSERR_INVALPARAM;

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return send_message(wm->ioProc, &wm->info, uMessage, lParam1, lParam2, type);
}

/**************************************************************************
 * 				mmioSendMessage		[WINMM.@]
 */
LRESULT WINAPI mmioSendMessage(HMMIO hmmio, UINT uMessage,
			       LPARAM lParam1, LPARAM lParam2)
{
    return MMIO_SendMessage(hmmio, uMessage, lParam1, lParam2, MMIO_PROC_32A);
}

/**************************************************************************
 * 				mmioDescend         	[WINMM.@]
 */
MMRESULT WINAPI mmioDescend(HMMIO hmmio, LPMMCKINFO lpck,
                            const MMCKINFO* lpckParent, UINT uFlags)
{
    DWORD		dwOldPos;
    FOURCC		srchCkId;
    FOURCC		srchType;


    TRACE("(%p, %p, %p, %04X);\n", hmmio, lpck, lpckParent, uFlags);

    if (lpck == NULL)
	return MMSYSERR_INVALPARAM;

    dwOldPos = mmioSeek(hmmio, 0, SEEK_CUR);
    TRACE("dwOldPos=%ld\n", dwOldPos);

    if (lpckParent != NULL) {
	TRACE("seek inside parent at %ld !\n", lpckParent->dwDataOffset);
	/* EPP: was dwOldPos = mmioSeek(hmmio,lpckParent->dwDataOffset,SEEK_SET); */
	if (dwOldPos < lpckParent->dwDataOffset ||
	    dwOldPos >= lpckParent->dwDataOffset + lpckParent->cksize) {
	    WARN("outside parent chunk\n");
	    return MMIOERR_CHUNKNOTFOUND;
	}
    }

    /* The SDK docu says 'ckid' is used for all cases. Real World
     * examples disagree -Marcus,990216.
     */

    srchType = 0;
    /* find_chunk looks for 'ckid' */
    if (uFlags & MMIO_FINDCHUNK)
	srchCkId = lpck->ckid;
    /* find_riff and find_list look for 'fccType' */
    if (uFlags & MMIO_FINDLIST) {
	srchCkId = FOURCC_LIST;
	srchType = lpck->fccType;
    }
    if (uFlags & MMIO_FINDRIFF) {
	srchCkId = FOURCC_RIFF;
	srchType = lpck->fccType;
    }

    if (uFlags & (MMIO_FINDCHUNK|MMIO_FINDLIST|MMIO_FINDRIFF)) {
	TRACE("searching for %.4s.%.4s\n",
	      (LPSTR)&srchCkId,
	      srchType?(LPSTR)&srchType:"any ");

	while (TRUE) {
	    LONG ix;

	    ix = mmioRead(hmmio, (LPSTR)lpck, 3 * sizeof(DWORD));
	    if (ix < 2*sizeof(DWORD)) {
		mmioSeek(hmmio, dwOldPos, SEEK_SET);
		WARN("return ChunkNotFound\n");
		return MMIOERR_CHUNKNOTFOUND;
	    }
	    lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
	    if (ix < lpck->dwDataOffset - dwOldPos) {
		mmioSeek(hmmio, dwOldPos, SEEK_SET);
		WARN("return ChunkNotFound\n");
		return MMIOERR_CHUNKNOTFOUND;
	    }
	    TRACE("ckid=%.4s fcc=%.4s cksize=%08lX !\n",
		  (LPSTR)&lpck->ckid,
		  srchType?(LPSTR)&lpck->fccType:"<na>",
		  lpck->cksize);
	    if ((srchCkId == lpck->ckid) &&
		(!srchType || (srchType == lpck->fccType))
		)
		break;

	    dwOldPos = lpck->dwDataOffset + ((lpck->cksize + 1) & ~1);
	    mmioSeek(hmmio, dwOldPos, SEEK_SET);
	}
    } else {
	/* FIXME: unverified, does it do this? */
	/* NB: This part is used by WAVE_mciOpen, among others */
	if (mmioRead(hmmio, (LPSTR)lpck, 3 * sizeof(DWORD)) < 3 * sizeof(DWORD)) {
	    mmioSeek(hmmio, dwOldPos, SEEK_SET);
	    WARN("return ChunkNotFound 2nd\n");
	    return MMIOERR_CHUNKNOTFOUND;
	}
	lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
    }
    lpck->dwFlags = 0;
    /* If we were looking for RIFF/LIST chunks, the final file position
     * is after the chunkid. If we were just looking for the chunk
     * it is after the cksize. So add 4 in RIFF/LIST case.
     */
    if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST)
	mmioSeek(hmmio, lpck->dwDataOffset + sizeof(DWORD), SEEK_SET);
    else
	mmioSeek(hmmio, lpck->dwDataOffset, SEEK_SET);
    TRACE("lpck: ckid=%.4s, cksize=%ld, dwDataOffset=%ld fccType=%08lX (%.4s)!\n",
	  (LPSTR)&lpck->ckid, lpck->cksize, lpck->dwDataOffset,
	  lpck->fccType, srchType?(LPSTR)&lpck->fccType:"");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioAscend     		[WINMM.@]
 */
MMRESULT WINAPI mmioAscend(HMMIO hmmio, LPMMCKINFO lpck, UINT uFlags)
{
    TRACE("(%p, %p, %04X);\n", hmmio, lpck, uFlags);

    if (lpck->dwFlags & MMIO_DIRTY) {
	DWORD	dwOldPos, dwNewSize;

	TRACE("Chunk is dirty, checking if chunk's size is correct\n");
	dwOldPos = mmioSeek(hmmio, 0, SEEK_CUR);
	TRACE("dwOldPos=%ld lpck->dwDataOffset = %ld\n", dwOldPos, lpck->dwDataOffset);
	dwNewSize = dwOldPos - lpck->dwDataOffset;
	if (dwNewSize != lpck->cksize) {
	    TRACE("Nope: lpck->cksize=%ld dwNewSize=%ld\n", lpck->cksize, dwNewSize);
	    lpck->cksize = dwNewSize;

	    /* pad odd size with 0 */
	    if (dwNewSize & 1) {
		char ch = 0;
		mmioWrite(hmmio, &ch, 1);
	    }
	    mmioSeek(hmmio, lpck->dwDataOffset - sizeof(DWORD), SEEK_SET);
	    mmioWrite(hmmio, (LPSTR)&dwNewSize, sizeof(DWORD));
	}
	lpck->dwFlags = 0;
    }

    mmioSeek(hmmio, lpck->dwDataOffset + ((lpck->cksize + 1) & ~1), SEEK_SET);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			mmioCreateChunk				[WINMM.@]
 */
MMRESULT WINAPI mmioCreateChunk(HMMIO hmmio, MMCKINFO* lpck, UINT uFlags)
{
    DWORD	dwOldPos;
    LONG 	size;
    LONG 	ix;

    TRACE("(%p, %p, %04X);\n", hmmio, lpck, uFlags);

    dwOldPos = mmioSeek(hmmio, 0, SEEK_CUR);
    TRACE("dwOldPos=%ld\n", dwOldPos);

    if (uFlags == MMIO_CREATELIST)
	lpck->ckid = FOURCC_LIST;
    else if (uFlags == MMIO_CREATERIFF)
	lpck->ckid = FOURCC_RIFF;

    TRACE("ckid=%.4s\n", (LPSTR)&lpck->ckid);

    size = 2 * sizeof(DWORD);
    lpck->dwDataOffset = dwOldPos + size;

    if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST)
	size += sizeof(DWORD);
    lpck->dwFlags = MMIO_DIRTY;

    ix = mmioWrite(hmmio, (LPSTR)lpck, size);
    TRACE("after mmioWrite ix = %ld req = %ld, errno = %d\n",ix, size, errno);
    if (ix < size) {
	mmioSeek(hmmio, dwOldPos, SEEK_SET);
	WARN("return CannotWrite\n");
	return MMIOERR_CANNOTWRITE;
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioRenameA    			[WINMM.@]
 */
MMRESULT WINAPI mmioRenameA(LPCSTR szFileName, LPCSTR szNewFileName,
                            const MMIOINFO* lpmmioinfo, DWORD dwFlags)
{
    struct IOProcList*  ioProc = NULL;
    struct IOProcList   tmp;
    FOURCC              fcc;

    TRACE("('%s', '%s', %p, %08lX);\n",
	  debugstr_a(szFileName), debugstr_a(szNewFileName), lpmmioinfo, dwFlags);

    /* If both params are NULL, then parse the file name */
    if (lpmmioinfo && lpmmioinfo->fccIOProc == 0 && lpmmioinfo->pIOProc == NULL)
    {
	fcc = MMIO_ParseExtA(szFileName);
        if (fcc) ioProc = MMIO_FindProcNode(fcc);
    }

    /* Handle any unhandled/error case from above. Assume DOS file */
    if (!lpmmioinfo || (lpmmioinfo->fccIOProc == 0 && lpmmioinfo->pIOProc == NULL && ioProc == NULL))
	ioProc = MMIO_FindProcNode(FOURCC_DOS);
    /* if just the four character code is present, look up IO proc */
    else if (lpmmioinfo->pIOProc == NULL)
        ioProc = MMIO_FindProcNode(lpmmioinfo->fccIOProc);
    else /* use relevant ioProc */
    {
        ioProc = &tmp;
        tmp.fourCC = lpmmioinfo->fccIOProc;
        tmp.pIOProc = lpmmioinfo->pIOProc;
        tmp.type = MMIO_PROC_32A;
        tmp.count = 1;
    }

    /* FIXME: should we actually pass lpmmioinfo down the drain ???
     * or make a copy of it because it's const ???
     */
    return send_message(ioProc, (LPMMIOINFO)lpmmioinfo, MMIOM_RENAME,
                        (LPARAM)szFileName, (LPARAM)szNewFileName, MMIO_PROC_32A);
}

/**************************************************************************
 * 				mmioRenameW    			[WINMM.@]
 */
MMRESULT WINAPI mmioRenameW(LPCWSTR szFileName, LPCWSTR szNewFileName,
                            const MMIOINFO* lpmmioinfo, DWORD dwFlags)
{
    LPSTR	szFn = NULL;
    LPSTR	sznFn = NULL;
    UINT	ret;
    INT         len;

    if (szFileName)
    {
        len = WideCharToMultiByte( CP_ACP, 0, szFileName, -1, NULL, 0, NULL, NULL );
        szFn = HeapAlloc( GetProcessHeap(), 0, len );
        if (!szFn) return MMSYSERR_NOMEM;
        WideCharToMultiByte( CP_ACP, 0, szFileName, -1, szFn, len, NULL, NULL );
    }
    if (szNewFileName)
    {
        len = WideCharToMultiByte( CP_ACP, 0, szNewFileName, -1, NULL, 0, NULL, NULL );
        sznFn = HeapAlloc( GetProcessHeap(), 0, len );
        if (!sznFn) return MMSYSERR_NOMEM;
        WideCharToMultiByte( CP_ACP, 0, szNewFileName, -1, sznFn, len, NULL, NULL );
    }

    ret = mmioRenameA(szFn, sznFn, lpmmioinfo, dwFlags);

    HeapFree(GetProcessHeap(),0,szFn);
    HeapFree(GetProcessHeap(),0,sznFn);
    return ret;
}
