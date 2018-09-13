#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "debug.h"
#include "fileshar.h"

#define MAXTASKS    10

#ifdef WIN32
#define CurrentProcess()    ((HANDLE) GetCurrentProcess())
#else
#define CurrentProcess()    ((HANDLE) GetCurrentPDB())
#endif

typedef struct {
    char	szFile[256];
    DWORD	dwOpenFlags;
    MMIOINFO	mmioinfo;

    HANDLE	htask;
    HMMIO	hmmio;
    int		i;
    LONG	lOffset;
    
    HANDLE	ahtask[MAXTASKS];
    HMMIO	ahmmio[MAXTASKS];
    ULONG	ulRef[MAXTASKS];
} SHFILE, FAR * PSHFILE;


BOOL GetProperTask(PSHFILE psh)
{
    HANDLE	htask = CurrentProcess();
    int		i;

    if (htask == psh->htask)
	return (psh->hmmio != 0) && (psh->hmmio != (HMMIO) -1);

    for (i = 0; i < MAXTASKS; i++) {
	if (psh->ahtask[i] == htask) {
Success:
	    psh->hmmio = psh->ahmmio[i];
	    psh->htask = htask;
	    psh->i = i;
	    mmioSeek(psh->hmmio, psh->lOffset, SEEK_SET);
	    return (psh->hmmio != 0) && (psh->hmmio != (HMMIO) -1);
	}
    }

    for (i = 0; i < MAXTASKS; i++) {
	if (psh->ahtask[i] == 0) {
	    DPF("Re-opening handle %lx in task %x\n", psh, htask);
	    psh->ahmmio[i] = mmioOpen(psh->szFile, NULL, psh->dwOpenFlags);
	    psh->ahtask[i] = htask;
	    if (psh->ahmmio[i] == 0) {
		DPF("mmioOpen failed in GetProperTask!\n");
		return FALSE;
	    }

	    goto Success;
	}
    }

    DPF("File handle open in too many tasks!\n");

    return FALSE;
}


HSHFILE WINAPI shfileOpen(LPSTR szFileName, MMIOINFO FAR* lpmmioinfo,
    DWORD dwOpenFlags)
{
    PSHFILE psh = (PSHFILE) GlobalAllocPtr(GHND | GMEM_SHARE, sizeof(SHFILE));

    if (!psh)
	return NULL;

    lstrcpy(psh->szFile, szFileName);
    psh->dwOpenFlags = dwOpenFlags;

    psh->hmmio = mmioOpen(szFileName, lpmmioinfo, dwOpenFlags);

    DPF("Opening handle %lx ('%s') in task %x, mode = %lx\n", psh, szFileName, CurrentProcess(), psh->dwOpenFlags);
    
    if (psh->hmmio == 0) {
	DPF("mmioOpen failed!\n");
	GlobalFreePtr(psh);
	return NULL;
    }

    psh->ahmmio[0] = psh->hmmio;
    psh->ahtask[0] = psh->htask = CurrentProcess();
    psh->ulRef[0] = 1; // !!! 0?

    return (HSHFILE) GlobalPtrHandle(psh);
}

UINT WINAPI shfileClose(HSHFILE hsh, UINT uFlags)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);
    int i;

    for (i = 0; i < MAXTASKS; i++) {
	if (psh->ahtask[i] && psh->ahmmio[i]) {
	    DPF("Handle %lx closed with ref count %ld in task %x\n", psh, psh->ulRef[i], psh->ahtask[i]);
    
	    mmioClose(psh->ahmmio[i], 0);
	}
    }
    GlobalFreePtr(psh);

    return 0;
}

LONG WINAPI shfileRead(HSHFILE hsh, HPSTR pch, LONG cch)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;
    
    return mmioRead(psh->hmmio, pch, cch);
}

LONG WINAPI shfileWrite(HSHFILE hsh, const char _huge* pch, LONG cch)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

    return mmioWrite(psh->hmmio, pch, cch);
}

LONG WINAPI shfileSeek(HSHFILE hsh, LONG lOffset, int iOrigin)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

    psh->lOffset = mmioSeek(psh->hmmio, lOffset, iOrigin);

    return psh->lOffset;
}

LONG WINAPI shfileFlush(HSHFILE hsh, UINT uFlags)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    return 0;
}

LONG WINAPI shfileAddRef(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;
    
    ++psh->ulRef[psh->i];

    // DPF("Handle %lx in task %x: ref++ == %ld\n", psh, psh->htask, psh->ulRef[psh->i]);
    return 0;
}

LONG WINAPI shfileRelease(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

    if (--psh->ulRef[psh->i] <= 0) {
	DPF("Closing handle %lx in task %x\n", psh, psh->htask);

	psh->ahmmio[psh->i] = 0;
	psh->ahtask[psh->i] = 0;
	psh->ulRef[psh->i] = 0;

	mmioClose(psh->hmmio, 0);
	psh->hmmio = 0;
	psh->htask = 0;
    } else {
	// DPF("Handle %lx in task %x: ref-- == %ld\n", psh, psh->htask, psh->ulRef[psh->i]);
    }


    return 0;
}

static	BYTE bPad;

MMRESULT WINAPI
shfileDescend(HSHFILE hshfile, LPMMCKINFO lpck, const LPMMCKINFO lpckParent, UINT wFlags)
{
	FOURCC		ckidFind;	// chunk ID to find (or NULL)
	FOURCC		fccTypeFind;	// form/list type to find (or NULL)

	/* figure out what chunk id and form/list type to search for */
	if (wFlags & MMIO_FINDCHUNK)
		ckidFind = lpck->ckid, fccTypeFind = NULL;
	else
	if (wFlags & MMIO_FINDRIFF)
		ckidFind = FOURCC_RIFF, fccTypeFind = lpck->fccType;
	else
	if (wFlags & MMIO_FINDLIST)
		ckidFind = FOURCC_LIST, fccTypeFind = lpck->fccType;
	else
		ckidFind = fccTypeFind = NULL;
	
	lpck->dwFlags = 0L;

	while (TRUE)
	{
		UINT		w;

		/* read the chunk header */
		if (shfileRead(hshfile, (HPSTR) lpck, 2 * sizeof(DWORD)) !=
		    2 * sizeof(DWORD))
			return MMIOERR_CHUNKNOTFOUND;

		/* store the offset of the data part of the chunk */
		if ((lpck->dwDataOffset = shfileSeek(hshfile, 0L, SEEK_CUR)) == -1)
			return MMIOERR_CANNOTSEEK;
		
		/* check for unreasonable chunk size */
		/* see if the chunk is within the parent chunk (if given) */
		if ((lpckParent != NULL) && ((	lpck->dwDataOffset - 8L) >=
		     (lpckParent->dwDataOffset + lpckParent->cksize)))
			return MMIOERR_CHUNKNOTFOUND;

		/* if the chunk if a 'RIFF' or 'LIST' chunk, read the
		 * form type or list type
		 */
		if ((lpck->ckid == FOURCC_RIFF) || (lpck->ckid == FOURCC_LIST))
		{
			if (shfileRead(hshfile, (HPSTR) &lpck->fccType,
				     sizeof(DWORD)) != sizeof(DWORD))
				return MMIOERR_CHUNKNOTFOUND;
		}
		else
			lpck->fccType = NULL;

		/* if this is the chunk we're looking for, stop looking */
		if ( ((ckidFind == NULL) || (ckidFind == lpck->ckid)) &&
		     ((fccTypeFind == NULL) || (fccTypeFind == lpck->fccType)) )
			break;
		
		/* ascend out of the chunk and try again */
		if ((w = shfileAscend(hshfile, lpck, 0)) != 0)
			return w;
	}

	return 0;
}

MMRESULT WINAPI
shfileAscend(HSHFILE hshfile, LPMMCKINFO lpck, UINT wFlags)
{
	if (lpck->dwFlags & MMIO_DIRTY)
	{
		/* <lpck> refers to a chunk created by shfileCreateChunk();
		 * check that the chunk size that was written when
		 * shfileCreateChunk() was called is the real chunk size;
		 * if not, fix it
		 */
		LONG		lOffset;	// current offset in file
		LONG		lActualSize;	// actual size of chunk data

		if ((lOffset = shfileSeek(hshfile, 0L, SEEK_CUR)) == -1)
			return MMIOERR_CANNOTSEEK;
		if ((lActualSize = lOffset - lpck->dwDataOffset) < 0)
			return MMIOERR_CANNOTWRITE;

		if (LOWORD(lActualSize) & 1)
		{
			/* chunk size is odd -- write a null pad byte */
			if (shfileWrite(hshfile, (HPSTR) &bPad, sizeof(bPad))
					!= sizeof(bPad))
				return MMIOERR_CANNOTWRITE;
			
		}

		if (lpck->cksize == (DWORD)lActualSize)
			return 0;

		/* fix the chunk header */
		lpck->cksize = lActualSize;
		if (shfileSeek(hshfile, lpck->dwDataOffset
				- sizeof(DWORD), SEEK_SET) == -1)
			return MMIOERR_CANNOTSEEK;
		if (shfileWrite(hshfile, (HPSTR) &lpck->cksize,
				sizeof(DWORD)) != sizeof(DWORD))
			return MMIOERR_CANNOTWRITE;
	}

	/* seek to the end of the chunk, past the null pad byte
	 * (which is only there if chunk size is odd)
	 */
	if (shfileSeek(hshfile, lpck->dwDataOffset + lpck->cksize
		+ (lpck->cksize & 1L), SEEK_SET) == -1)
		return MMIOERR_CANNOTSEEK;

	return 0;
}

MMRESULT WINAPI
shfileCreateChunk(HSHFILE hshfile, LPMMCKINFO lpck, UINT wFlags)
{
	int		iBytes;			// bytes to write
	LONG		lOffset;	// current offset in file

	/* store the offset of the data part of the chunk */
	if ((lOffset = shfileSeek(hshfile, 0L, SEEK_CUR)) == -1)
		return MMIOERR_CANNOTSEEK;
	lpck->dwDataOffset = lOffset + 2 * sizeof(DWORD);

	/* figure out if a form/list type needs to be written */
	if (wFlags & MMIO_CREATERIFF)
		lpck->ckid = FOURCC_RIFF, iBytes = 3 * sizeof(DWORD);
	else
	if (wFlags & MMIO_CREATELIST)
		lpck->ckid = FOURCC_LIST, iBytes = 3 * sizeof(DWORD);
	else
		iBytes = 2 * sizeof(DWORD);

	/* write the chunk header */
	if (shfileWrite(hshfile, (HPSTR) lpck, (LONG) iBytes) != (LONG) iBytes)
		return MMIOERR_CANNOTWRITE;

	lpck->dwFlags = MMIO_DIRTY;

	return 0;
}

