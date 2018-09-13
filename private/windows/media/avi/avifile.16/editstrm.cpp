/****************************************************************************
 *
 *  EDITSTRM.C
 *
 *  routines for reading Standard AVI files
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <compman.h>
#include "avifile.h"
#include <storage.h>
#include "editstrm.h"

#ifdef DEBUG
    static void CDECL dprintf(LPSTR, ...);
    #define DPF dprintf
#else
    #define DPF ; / ## /
#endif

/*
 * memcopy.asm
 */
#ifdef WIN32
#define MemCopy(dst, src, cnt) memmove(dst,src,cnt)
#else
EXTERN_C LONG FAR PASCAL MemCopy(HPSTR, HPSTR, DWORD);
#endif



STDAPI EditStreamCut(PAVISTREAM pavi, LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult)
{
    PAVIEDITSTREAM  pedit = NULL;
    HRESULT	    hr;

    pavi->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &pedit);
    if (!pedit)
	return ResultFromScode(E_NOINTERFACE);

    hr = pedit->Cut(plStart, plLength, ppResult);

    pedit->Release();

    return hr;
}

STDAPI EditStreamCopy(PAVISTREAM pavi, LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult)
{
    PAVIEDITSTREAM  pedit = NULL;
    HRESULT	    hr;

    pavi->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &pedit);
    if (!pedit)
	return ResultFromScode(E_NOINTERFACE);

    hr = pedit->Copy(plStart, plLength, ppResult);

    pedit->Release();

    return hr;
}

STDAPI EditStreamPaste(PAVISTREAM pavi, LONG FAR *plPos, LONG FAR *plLength, PAVISTREAM pstream, LONG lStart, LONG lLength)
{
    PAVIEDITSTREAM  pedit = NULL;
    HRESULT	    hr;

    pavi->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &pedit);
    if (!pedit)
	return ResultFromScode(E_NOINTERFACE);

    hr = pedit->Paste(plPos, plLength, pstream, lStart, lLength);

    pedit->Release();

    return hr;
}

STDAPI EditStreamClone(PAVISTREAM pavi, PAVISTREAM FAR *ppResult)
{
    PAVIEDITSTREAM  pedit = NULL;
    HRESULT	    hr;

    pavi->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &pedit);
    if (!pedit)
	return ResultFromScode(E_NOINTERFACE);

    hr = pedit->Clone(ppResult);

    pedit->Release();

    return hr;
}

STDAPI EditStreamSetInfo(PAVISTREAM pavi, AVISTREAMINFO FAR *lpInfo, LONG cbInfo)
{
    PAVIEDITSTREAM  pedit = NULL;
    HRESULT	    hr;

    pavi->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &pedit);
    if (!pedit)
	return ResultFromScode(E_NOINTERFACE);

    hr = pedit->SetInfo(lpInfo, cbInfo);

    pedit->Release();

    return hr;
}

STDAPI EditStreamSetName(PAVISTREAM pavi, LPCSTR lpszName)
{
    PAVIEDITSTREAM  pedit = NULL;
    HRESULT	    hr;
    AVISTREAMINFO   info;

    pavi->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &pedit);
    if (!pedit)
	return ResultFromScode(E_NOINTERFACE);

    pavi->Info(&info, sizeof(info));
    _fstrncpy(info.szName, lpszName, sizeof(info.szName));
    hr = pedit->SetInfo(&info, sizeof(info));

    pedit->Release();

    return hr;
}

// #define EDITCHECK

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#define USUAL_ALLOC	16
#define EXTRA_ALLOC	8
HRESULT CEditStream::AllocEditSpace(LONG l, LONG cNew)
{
    LPEDIT  p;
    LONG    size;

    if (cedits + cNew > maxedits) {
	size = maxedits + max(cNew + EXTRA_ALLOC, USUAL_ALLOC);

	p = (LPEDIT) GlobalReAllocPtr(edits, size * sizeof(EDIT), GHND | GMEM_SHARE);

	if (!p)
	    return ResultFromScode(AVIERR_MEMORY);

	edits = p;
	maxedits = size;
    }

    if (l < cedits)
	MemCopy((HPSTR) &edits[l + cNew],
		(HPSTR) &edits[l],
		(cedits - l) * sizeof(EDIT));

    cedits += cNew;

    return AVIERR_OK;
}

HRESULT CEditStream::PossiblyRemoveEdit(LONG l)
{
    if (edits[l].lLength > 0)
	return AVIERR_OK;

    if (edits[l].pavi)
	AVIStreamRelease(edits[l].pavi);

    --cedits;

    if (l < cedits)
	MemCopy((HPSTR) &edits[l],
		(HPSTR) &edits[l + 1],
		(cedits - l) * sizeof(EDIT));

    return AVIERR_OK;
}

CEditStream FAR * CEditStream::NewEditStream(PAVISTREAM psSource)
{
    CEditStream FAR * pedit;

    pedit = new CEditStream;

    if (pedit)
	(pedit->Create)((LONG) psSource, 0);
    // !!! error check

    return pedit;
}

STDAPI CreateEditableStream(
		PAVISTREAM FAR *	    ppsEditable,
		PAVISTREAM		    psSource)
{
    // First, check if the stream is already editable....

    if (psSource) {
	PAVIEDITSTREAM	paviedit = NULL;

	psSource->QueryInterface(IID_IAVIEditStream, (LPVOID FAR *) &paviedit);

	if (paviedit) {
	    paviedit->Clone(ppsEditable);
	    paviedit->Release();
	    return AVIERR_OK;
	}
    }

    *ppsEditable = (PAVISTREAM) CEditStream::NewEditStream(psSource);

    if (!*ppsEditable)
	return ResultFromScode(AVIERR_MEMORY);

    return AVIERR_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//  EditStreamOpen()
//
//  open a single stream of a particular type from a AVI file.
//
//  params:
//      szFile      - AVI file name
//      fccType     - stream type 0 for any type
//      iStream     - zero based stream number
//
//  returns:
//      a PAVISTREAM for the specifed stream or NULL.
//
//  example:
//
//      EditStreamOpen(pavi, "Foo.avi", 0, 0)
//
//          will open stream 0 (the first stream)
//
//      EditStreamOpen(pavi, "Foo.avi", 1)
//
//          will open stream 1
//
//      EditStreamOpenStream(pavi, "Foo.avi", 'vids', 0)
//
//          will open the first video stream
//
//      AVIOpenStream(pavi, "Foo.avi", 'auds', 0)
//
//          will open the first audio stream
//
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CEditStream::Create(LONG lParam1, LONG lParam2)
{


    this->edits = (LPEDIT) GlobalAllocPtr(GHND | GMEM_SHARE, USUAL_ALLOC * sizeof(EDIT));
    if (this->edits == NULL)
	return ResultFromScode(AVIERR_MEMORY);

    this->maxedits = USUAL_ALLOC;
    this->ulRefCount = 1;

    this->pgf = NULL;
    this->psgf = NULL;
    this->lpbiLast = NULL;
    this->fFullFrames = FALSE;
    this->edits[0].pavi = (PAVISTREAM)lParam1;
    _fmemset(&this->sinfo, 0, sizeof(this->sinfo));
    this->cedits = 1;
    if (this->edits[0].pavi) {
	AVIStreamAddRef(this->edits[0].pavi);
	this->edits[0].lStart = AVIStreamStart(this->edits[0].pavi);
	this->edits[0].lLength = AVIStreamLength(this->edits[0].pavi);

	AVIStreamInfo(this->edits[0].pavi, &this->sinfo, sizeof(this->sinfo));
	
	CheckEditList();
    } else {
	this->edits[0].lStart = 0;
	this->edits[0].lLength = 0;
    }

    DPF("Edit   %08lx: Usage++=%lx\n", (DWORD) (LPVOID) this, 1L);

    //
    // all done return success.
    //
    return 0; // success
}

///////////////////////////////////////////////////////////////////////////
//
//  EditStreamAddRef()
//
//      increase the reference count of the stream
//
///////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CEditStream::AddRef()
{
    DPF("Edit   %08lx: Usage++=%lx\n", (DWORD) (LPVOID) this, this->ulRefCount + 1);

    return ++this->ulRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
//  EditStreamRelease()
//
//      close a EditStream stream
//
///////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CEditStream::Release()
{
    LONG	l;

    DPF("Edit   %08lx: Usage--=%lx\n", (DWORD) (LPVOID) this, this->ulRefCount - 1);
    if (--this->ulRefCount)
	return this->ulRefCount;

    // free edits....
    for (l = 0; l < this->cedits; l++) {
	if (this->edits[l].pavi)
	    AVIStreamRelease(this->edits[l].pavi);
    }

    GlobalFreePtr(this->edits);

    this->edits = 0;

    if (this->pgf)
	AVIStreamGetFrameClose(this->pgf);

    delete this;

    return 0;
}

LPBITMAPINFOHEADER NEAR PASCAL CEditStream::CallGetFrame(
						      PAVISTREAM p,
						      LONG l)
{
    if (psgf != p) {
	PGETFRAME   pgfNew;
	
	pgfNew = AVIStreamGetFrameOpen(p, NULL);
	
	if (!pgfNew)
	    return NULL;

	if (pgf) {
#ifdef DEBUG
	    DPF("Trying to SetFormat %dx%dx%d '%4.4s'\n",
	    	    (int)lpbiLast->biWidth,
		    (int)lpbiLast->biHeight,
		    (int)lpbiLast->biBitCount,
		    (lpbiLast->biCompression == BI_RGB  ? (LPSTR)"None" :
		    lpbiLast->biCompression == BI_RLE8 ? (LPSTR)"Rle8" :
			lpbiLast->biCompression == BI_RLE4 ? (LPSTR)"Rle4" :
			    (LPSTR)&lpbiLast->biCompression));
#endif	

            if (pgfNew->SetFormat(lpbiLast, NULL, 0, 0, -1, -1) != AVIERR_OK) {
		DPF("Couldn't AVIStreamGetFrameSetFormat!\n");
		AVIStreamGetFrameClose(pgfNew);
		return NULL;
	    }
		
	    AVIStreamGetFrameClose(pgf);
	}

	pgf = pgfNew;
	psgf = p;

    }

    lpbiLast = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pgf, l);

    if (lpbiLast)
	sinfo.dwSuggestedBufferSize = lpbiLast->biSizeImage;

    return lpbiLast;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CEditStream::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    PAVISTREAM	p;
    LONG	l;
    HRESULT	hr;

    if ((lPos < (LONG) sinfo.dwStart) ||
		(lPos >= (LONG) (sinfo.dwStart + sinfo.dwLength))) {
	return ResultFromScode(AVIERR_BADPARAM);
    }
	
    hr = ResolveEdits(lPos, &p, &l, NULL, FALSE);

    if (hr != 0) {
	DPF("ReadFormat: ResolveEdits failed!\n");
	return hr;
    }

    if (fFullFrames) {
	LPBITMAPINFOHEADER  lpbi;
	LONG		    lSize;

	// This isn't really right: we really need to make the formats
	// agree.  Should we just get the format from the first frame?
	
	lpbi = CallGetFrame(p, l);

	if (!lpbi) {
	    DPF("ReadFormat: GetFrame failed!\n");
	    return ResultFromScode(E_FAIL);
	}

	lSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

	if (lpFormat)
	    hmemcpy(lpFormat, lpbi, min(*lpcbFormat, lSize));

	*lpcbFormat = lSize;
	return 0;
    } else {
	return AVIStreamReadFormat(p, l, lpFormat, lpcbFormat);
    }
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CEditStream::Info(AVISTREAMINFO FAR * psi, LONG lSize)
{

    if (psi)
	hmemcpy(psi, &sinfo, min(lSize, sizeof(sinfo)));
    return 0; // !!! sizeof(pavi->sinfo);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

LONG STDMETHODCALLTYPE CEditStream::FindSample(LONG lPos, LONG lFlags)
{
    PAVISTREAM	p;
    LONG	l;
    LONG	edit;
    LONG	lRet;
    HRESULT	hr;

    if ((lPos < (LONG) sinfo.dwStart) ||
		(lPos >= (LONG) (sinfo.dwStart + sinfo.dwLength))) {
	return -1;
    }
	
    hr = ResolveEdits(lPos, &p, &l, &edit, TRUE);

    if (hr != 0) {
	DPF("FindSample: error from ResolveEdits!\n");
	return -1;
    }

    if (lFlags & FIND_FORMAT) {
	// !!!!  This isn't right, obviously.
	if (lFlags & FIND_PREV)
	    return 0;
	else
	    return -1;
    }

    if (this->fFullFrames) {
	return lPos;
    }

    // !!! This won't really work, especially for searching forward.
    lRet = AVIStreamFindSample(p, l, lFlags);

#ifdef DEBUG
    if (lRet < edits[edit].lStart) {
	DPF("We were about to return a key frame before a segment: returning %ld instead of %ld.\n", edits[edit].lStart, lRet);
    }
#endif

    // DPF("FindSample: lPos = %ld, Key = %ld\n", lPos, lPos - (l - lRet));
    return lPos - (l - lRet);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CEditStream::Read(
                 LONG       lStart,
                 LONG       lSamples,
                 LPVOID     lpBuffer,
                 LONG       cbBuffer,
                 LONG FAR * plBytes,
                 LONG FAR * plSamples)
{
    PAVISTREAM	p;
    LONG	l;
    LONG	edit;
    LONG	lSamplesRead;
    LONG	lBytesRead;
    LONG	lSamplesThisTime;
    HRESULT	hr;

    if (plBytes)
	*plBytes = 0;
    if (plSamples)
	*plSamples = 0;

#ifdef TOOMUCHDEBUG
    if (lpBuffer) {
	DPF("Read %08lx: Start = %ld Length = %ld\n", (DWORD) (LPVOID) this, lStart, lSamples);
    }
#endif

    if ((lStart < (LONG) sinfo.dwStart) ||
		(lStart >= (LONG) (sinfo.dwStart + sinfo.dwLength))) {
	DPF("Read at position %ld, start = %lu, len = %lu\n", lStart, sinfo.dwStart, sinfo.dwStart + sinfo.dwLength);
	
	return ResultFromScode(AVIERR_BADPARAM);
    }
	
    while (lSamples) {
	hr = ResolveEdits(lStart, &p, &l, &edit, FALSE);

	if (hr != 0) {
	    DPF("Read: ResolveEdits failed!\n");
	    return ResultFromScode(E_FAIL);
	}
	
	// Don't read past the end of this edit.
	if ((l - this->edits[edit].lStart) + lSamples > this->edits[edit].lLength)
	    lSamplesThisTime = this->edits[edit].lLength - (l - this->edits[edit].lStart);
	else
	    lSamplesThisTime = lSamples;


	if (this->fFullFrames) {
	    LPBITMAPINFOHEADER  lpbi;
	    LPVOID		    lp;

	    lpbi = CallGetFrame(p, l);

	    if (!lpbi) {
		DPF("Read: GetFrame failed!\n");
		return ResultFromScode(E_FAIL);
	    }

	    //
	    // a NULL buffer means return the size buffer needed to read
	    // the given sample.
	    //
	    if (lpBuffer == NULL)
		goto exit;

	    lp = (LPBYTE) lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

	    if (cbBuffer >= (LONG) lpbi->biSizeImage) {
		hmemcpy(lpBuffer, lp, lpbi->biSizeImage);
    exit:
		if (plBytes)
		    *plBytes = lpbi->biSizeImage;

		if (plSamples)
		    *plSamples = 1;

		return 0;
	    }
	    if (plBytes)
		*plBytes = 0;

	    if (plSamples)
		*plSamples = 0;

	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	} else {
	    hr = AVIStreamRead(p, l, lSamplesThisTime, lpBuffer, cbBuffer,
			       &lBytesRead, &lSamplesRead);

	    if (hr != NOERROR)
		return hr;
	
	    if (plBytes)
		*plBytes += lBytesRead;
	    if (plSamples)
		*plSamples += lSamplesRead;

	    lpBuffer = (BYTE _huge *) lpBuffer + lBytesRead;
	    lStart += lSamplesThisTime;
	    lSamples -= lSamplesThisTime;
	    cbBuffer -= lBytesRead;

	    // If we've read up to the end of the file,
	    // stop now, rather than return an error....
	    if (lStart >= (LONG) (this->sinfo.dwLength + this->sinfo.dwStart))
		break;
	}
    }

#ifdef TOOMUCHDEBUG
    if (lpBuffer && plBytes) {
	DPF("Read %08lx:  Bytes Read = %ld\n", (DWORD) (LPVOID) this, *plBytes);
    }
#endif

    return 0;
}

void CEditStream::CheckEditList()
{
#ifdef EDITCHECK
    LONG    lTotal = 0;
    LONG    l;

    DPF("Edit list %08lx: %s\n", (DWORD) this, fFullFrames ? (LPSTR) " (Using full frames)" : (LPSTR) "");

    for (l = 0; l < cedits; l++) {
	DPF("\t\t%ld:\t%08lx\t%ld\t%ld\n", l, (DWORD) (LPVOID) edits[l].pavi, edits[l].lStart, edits[l].lLength);
	lTotal += edits[l].lLength;
    }

    if (lTotal != (LONG) sinfo.dwLength) {
	* (LPSTR) 0 = 0;
    }
#endif
}

HRESULT CEditStream::ResolveEdits(LONG lPos, PAVISTREAM FAR *ppavi,
		  LONG FAR *plPos, LONG FAR *pl, BOOL fAllowEnd)
{
    LONG    edit;

    //
    // Search edit list, get position...
    //

    if (lPos < (LONG) this->sinfo.dwStart) {
	DPF("ResolveEdits: Read at %ld, before start at %ld\n", lPos, this->sinfo.dwStart);
	return ResultFromScode(AVIERR_BADPARAM);
    }

    lPos -= (LONG) this->sinfo.dwStart;

    for (edit = 0; edit < this->cedits; edit++) {
	if (lPos < this->edits[edit].lLength) {
	    *ppavi = this->edits[edit].pavi;
	    *plPos = lPos + this->edits[edit].lStart;
	    if (pl)
		*pl = edit;
	    return 0;
	}

	lPos -= this->edits[edit].lLength;
    }

    // Normally, we don't return a position at the end of an edit--we instead
    // go to the next thing.
    if (lPos == 0 && fAllowEnd) {
	edit--;
	*ppavi = this->edits[edit].pavi;
	*plPos = this->edits[edit].lStart + this->edits[edit].lLength;
	if (pl)
	    *pl = edit;
	return 0;
    }

    *ppavi = 0;
    *plPos = 0;
    if (pl)
	*pl = 0;

    return ResultFromScode(AVIERR_BADPARAM);
}

//
// Cut:
//
// Takes start, length to cut out
//
// returns actual start, length cut, along with new stream
//
STDMETHODIMP CEditStream::Cut(LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult)
{
    HRESULT	hr = AVIERR_OK;
    PAVISTREAM	p;
    LONG	l;
    LONG		edit;
    LONG	lStart, lLength;

    l = AVIStreamLength(this);

    if (ppResult)
	*ppResult = 0;

    if (!plStart || !plLength) {
	return ResultFromScode(AVIERR_BADPARAM);
    }

    if (*plStart < 0) {
	return ResultFromScode(AVIERR_BADPARAM);
    }

    if (*plLength < 0 || *plStart + *plLength > l) {
	if (*plStart >= l)
	    return ResultFromScode(AVIERR_BADPARAM);
	*plLength = l - *plStart;
    }

#ifdef KEYALWAYS
    // Make cut end at key frame
    for (l = *plStart + *plLength; l < AVIStreamLength(this); l++) {
	if (AVIStreamFindSample(this, l, 0) == l)
	    break;
    }
    *plLength = l - *plStart;
#else
    // we cut whatever they ask us to....
#endif

    // Make a copy of the section being cut out
    if (ppResult) {
	// This will make cut start at key frame if it needs to
	hr = this->Copy(plStart, plLength, ppResult);

	if (hr != AVIERR_OK)
	    return hr;
    }

    lLength = *plLength;
    lStart = *plStart;

#ifndef KEYALWAYS
    if (!this->fFullFrames &&
	lStart + lLength < AVIStreamLength(this) &&
	AVIStreamFindSample(this, lStart + lLength, 0) != lStart + lLength) {
	DPF("Cut: Converting stream to full frames\n");
	this->fFullFrames = TRUE;
	this->sinfo.dwFormatChangeCount++;
	this->sinfo.fccHandler = 0;
    }
#endif

    // Now do the actual cut
    hr = ResolveEdits(lStart, &p, &l, &edit, FALSE);

    if (hr != NOERROR)
	return hr;

    if (this->edits[edit].lStart + this->edits[edit].lLength > l + lLength) {
	// The part cut out is entirely within this edit.
	if (this->edits[edit].lStart == l) {
	    // The part cut out is the start of this edit
	    this->edits[edit].lStart = l + lLength;
	    this->edits[edit].lLength -= lLength;
	} else {
	    hr = AllocEditSpace(edit, 1);

	    if (hr == AVIERR_OK) {
		this->edits[edit] = this->edits[edit+1];
		if (this->edits[edit].pavi)
		    AVIStreamAddRef(this->edits[edit].pavi);
		this->edits[edit].lStart = this->edits[edit+1].lStart;
		this->edits[edit].lLength = l - this->edits[edit].lStart;
		this->edits[edit+1].lStart = l + lLength;
		this->edits[edit+1].lLength -= lLength +
					       this->edits[edit].lLength;
	    }
	}
    } else if (this->edits[edit].lStart + this->edits[edit].lLength == l + lLength) {
	// The part cut out is the end of this edit
	this->edits[edit].lLength = l - this->edits[edit].lStart;
    } else {
	LONG lTemp = lLength;
	
	// We're cutting out more than this one edit.
	// First, cut out the rest of this edit.
	lTemp -= this->edits[edit].lStart + this->edits[edit].lLength - l;
	this->edits[edit].lLength = l - this->edits[edit].lStart;

	edit++;

	// As long as subsequent edits are still shorter than the cut,
	// kill them..
	while (edit < this->cedits &&
	       this->edits[edit].lLength < lTemp) {
	    lTemp -= this->edits[edit].lLength;
	    this->edits[edit].lLength = 0;
	    edit++;
	}

	if (edit < this->cedits) {
	    this->edits[edit].lStart += lTemp;
	    this->edits[edit].lLength -= lTemp;
	}
    }

    if (hr == AVIERR_OK) {
	this->sinfo.dwLength -= lLength;
	this->sinfo.dwEditCount++;

	CheckEditList();
    } else {
	if (ppResult)
	    AVIStreamRelease(*ppResult);
    }
    return hr;
}

//
// Copy:
//
// Takes start, length to cut out
//
// returns actual start, length cut, along with new stream
//
//

STDMETHODIMP CEditStream::Copy(LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult)
{
    PAVISTREAM	p1;
    LONG	l1;
    LONG	edit1;
    PAVISTREAM	p2;
    LONG	l2;
    LONG	edit2;
    LONG	l;
    CEditStream FAR *p;
    HRESULT	hr;
    LONG	lStart, lLength;

    l1 = AVIStreamLength(this);

    // If start, length < 0, pick some defaults
    if (*plStart < 0)
	*plStart = 0;

    if (*plLength < 0)
	*plLength = l1 - *plStart;

    // Make sure the start position is within range
    if (*plStart > l1) {
	if (ppResult)
	    *ppResult = 0;
	return ResultFromScode(AVIERR_BADPARAM);
    }

    // Make sure the length is within range
    if (*plStart + *plLength > l1)
	*plLength = l1 - *plStart;

#ifdef KEYALWAYS
    // Make copy start at key frame
    lStart = AVIStreamFindSample(this, *plStart, 0);
    *plLength += *plStart - lStart;
    *plStart = lStart;
#endif

    lLength = *plLength;
    lStart = *plStart;

    p = NewEditStream(NULL);
    *ppResult = (PAVISTREAM) p;
    if (!p)
	return ResultFromScode(AVIERR_MEMORY);

    hmemcpy(&p->sinfo, &this->sinfo, sizeof(p->sinfo));

    if (lLength <= 0)
	lLength = (LONG) (p->sinfo.dwLength + p->sinfo.dwStart) - lStart;

    hr = ResolveEdits(lStart, &p1, &l1, &edit1, FALSE);
    hr = ResolveEdits(lStart + lLength, &p2, &l2, &edit2, TRUE);

    if (edit1 == edit2) {
	p->edits[0].pavi = p1;
	if (p1)
	    AVIStreamAddRef(p1);
	p->edits[0].lStart = l1;
	p->edits[0].lLength = lLength;
    } else {
	hr = p->AllocEditSpace(1, edit2 - edit1);

	for (l = 0; l <= edit2 - edit1; l++) {
	    if (l == 0) {
		p->edits[l].pavi = p1;
		if (p1)
		    AVIStreamAddRef(p1);
		p->edits[l].lStart = l1;
		p->edits[l].lLength = this->edits[edit1].lStart +
				      this->edits[edit1].lLength - l1;
	    } else if (l < edit2 - edit1) {
		p->edits[l] = this->edits[l+edit1];
		if (p->edits[l].pavi)
		    AVIStreamAddRef(p->edits[l].pavi);
	    } else {
		p->edits[l] = this->edits[edit2];
		if (p->edits[l].pavi)
		    AVIStreamAddRef(p->edits[l].pavi);
		p->edits[l].lLength = l2 - p->edits[l].lStart;
	    }
	}
	
	p->PossiblyRemoveEdit(edit2 - edit1);	
	p->PossiblyRemoveEdit(0);
    }

#ifndef KEYALWAYS
    l1 = AVIStreamFindSample(p->edits[0].pavi, p->edits[0].lStart, 0);
    DPF("edit starts at %ld, key at %ld\n", p->edits[0].lStart, l1);
    if (l1 != p->edits[0].lStart) {
	p->fFullFrames = TRUE;
	DPF("Copy: Converting new stream to full frames\n");
    }
#endif

    AVIStreamInfo(this->edits[0].pavi, &p->sinfo, sizeof(p->sinfo));
    p->sinfo.dwStart = 0;
    p->sinfo.dwLength = (DWORD) lLength;
    p->sinfo.dwEditCount = 0;
    p->sinfo.dwFormatChangeCount = 0;
    if (p->fFullFrames)
	p->sinfo.fccHandler = 0;

    p->CheckEditList();
    CheckEditList();

    return AVIERR_OK;
}

/**************************************************************************
* @doc  INTERNAL DRAWDIB
*
* @api BOOL | DibEq | This function compares two dibs.
*
* @parm LPBITMAPINFOHEADER lpbi1 | Pointer to one bitmap.
*       this DIB is assumed to have the colors after the BITMAPINFOHEADER
*
* @parm LPBITMAPINFOHEADER | lpbi2 | Pointer to second bitmap.
*       this DIB is assumed to have the colors after biSize bytes.
*
* @rdesc Returns TRUE if bitmaps are identical, FALSE otherwise.
*
**************************************************************************/
inline BOOL DibEq(LPBITMAPINFOHEADER lpbi1, LPBITMAPINFOHEADER lpbi2)
{
    return
        lpbi1->biCompression == lpbi2->biCompression   &&
        lpbi1->biSize        == lpbi2->biSize          &&
        lpbi1->biWidth       == lpbi2->biWidth         &&
        lpbi1->biHeight      == lpbi2->biHeight        &&
        lpbi1->biBitCount    == lpbi2->biBitCount;
}

BOOL AreVideoStreamsCompatible(PAVISTREAM ps1, PAVISTREAM ps2)
{
    LONG	cb1, cb2;
    BITMAPINFOHEADER	bih1, bih2;
	
    AVIStreamReadFormat(ps1, AVIStreamStart(ps1), NULL, &cb1);
    AVIStreamReadFormat(ps2, AVIStreamStart(ps2), NULL, &cb2);

    if (cb1 != cb2)
	return FALSE;

    cb1 = sizeof(bih1);
    cb2 = sizeof(bih2);
    AVIStreamReadFormat(ps1, AVIStreamStart(ps1), &bih1, &cb1);
    AVIStreamReadFormat(ps2, AVIStreamStart(ps2), &bih2, &cb2);

    if (DibEq(&bih1, &bih2))
	return TRUE;

    return FALSE;
}

BOOL AreAudioStreamsCompatible(PAVISTREAM ps1, PAVISTREAM ps2)
{
    LONG	cb1, cb2;
    LPVOID	lpf;
    BOOL	f;

    AVIStreamReadFormat(ps1, AVIStreamStart(ps1), NULL, &cb1);
    AVIStreamReadFormat(ps2, AVIStreamStart(ps2), NULL, &cb2);

    if (cb1 != cb2)
	return FALSE;

    lpf = GlobalAllocPtr(GHND, cb1 + cb2);

    if (!lpf)
	return FALSE; // !!!

    AVIStreamReadFormat(ps1, AVIStreamStart(ps1), lpf, &cb1);
    AVIStreamReadFormat(ps2, AVIStreamStart(ps2), (BYTE FAR *)lpf + cb1, &cb2);

    f = !_fmemcmp(lpf, (BYTE FAR *)lpf + cb1, (UINT) cb1);

    GlobalFreePtr(lpf);

    return f;
}

//
// Paste:
//
//     Takes stream to paste, along with start and length within that stream,
//	and also target stream and position within the stream to do the paste.
//
//	Returns position and length pasted.
//
STDMETHODIMP CEditStream::Paste(LONG FAR *plPos, LONG FAR *plLength, PAVISTREAM pstream, LONG lStart, LONG lLength)
{
    PAVISTREAM	p;
    LONG	l;
    LONG	edit;
    HRESULT	hr;
    LONG	lPos;
    CEditStream	FAR *pnew;
    AVISTREAMINFO   strinfo;

    AVIStreamInfo(pstream, &strinfo, sizeof(strinfo));

    if (this->sinfo.fccType == 0) {
	AVIStreamInfo(pstream, &this->sinfo, sizeof(this->sinfo));
	this->sinfo.dwLength = 0;
	this->sinfo.dwStart = *plPos;
    }

    if (*plPos > (LONG) (sinfo.dwLength + sinfo.dwStart)) {
	// !!! We should handle this case....
	return ResultFromScode(AVIERR_BADPARAM);
    }


#ifdef KEYALWAYS
    // Make paste go before a key frame...
    *plPos = AVIStreamFindSample(this, *plPos, 0);
#endif
    lPos = *plPos;

    if (strinfo.fccType != this->sinfo.fccType) {
	DPF("Paste: Incompatible stream types!\n");
	return ResultFromScode(AVIERR_UNSUPPORTED);
    }

    if (lLength <= 0 || ((lStart + lLength) >
			 (LONG) (strinfo.dwStart + strinfo.dwLength))) {
	if (lStart >= (LONG) (strinfo.dwLength + strinfo.dwStart))
	    return ResultFromScode(AVIERR_BADPARAM);

	lLength = (LONG) (strinfo.dwLength + strinfo.dwStart) - lStart;
    }

    if ((DWORD) lPos + (DWORD) lLength > 0x80000000) {
	DPF("Paste result would be more than 2 billion frames!\n");
	return ResultFromScode(AVIERR_MEMORY);
    }

    // !!! What if the frame rates don't match?

#define SIZEMISMATCH(rc1, rc2) \
    (((rc1.right - rc1.left) != (rc2.right - rc2.left)) || \
     ((rc1.bottom - rc1.top) != (rc2.bottom - rc2.top)))

    if (strinfo.fccType == streamtypeVIDEO &&
		SIZEMISMATCH(strinfo.rcFrame, this->sinfo.rcFrame)) {
	// !!! It would be nice if this worked.
	DPF("Paste: Video streams are different sizes!\n");
	return ResultFromScode(AVIERR_UNSUPPORTED);
    }

    if (this->sinfo.fccType == streamtypeAUDIO) {
	if (!AreAudioStreamsCompatible((PAVISTREAM) this, pstream)) {
	    DPF("Paste: Audio streams are different formats!\n");
	    return ResultFromScode(AVIERR_UNSUPPORTED);
	}
    }

    pnew = NULL;
    pstream->QueryInterface(CLSID_EditStream, (LPVOID FAR *) &pnew);

#ifndef KEYALWAYS
    if (this->sinfo.fccType == streamtypeVIDEO) {
	if (!this->fFullFrames) {
	    if ((!AVIStreamIsKeyFrame(pstream, lStart) ||
		 (pnew && pnew->fFullFrames)) ||
		((lPos < (LONG) (sinfo.dwLength + sinfo.dwStart)) &&
		 !AVIStreamIsKeyFrame((PAVISTREAM) this, lPos)) ||
		!AreVideoStreamsCompatible((PAVISTREAM) this, pstream)) {

		// !!! What if we're pasting, say, an 8-bit and a 32-bit
		// movie together?  Do we have to pick a common format
		// to convert to?
		CallGetFrame(this->edits[0].pavi, this->edits[0].lStart);
		if (CallGetFrame(pstream, lStart) == NULL) {
		    DPF("Paste: Can't make a common format!\n");
		    if (pnew)
			pnew->Release();
		
		    return ResultFromScode(AVIERR_BADFORMAT);
		}

		this->fFullFrames = TRUE;
		DPF("Paste: Converting stream to full frames\n");
		this->sinfo.dwFormatChangeCount++;

		// ??? !!! Call get frame once, just so it's been done....
	    }
	} else {
	    if (CallGetFrame(pstream, lStart) == NULL) {
		DPF("Paste: Can't make a common format!\n");
		if (pnew)
		    pnew->Release();
		
		return ResultFromScode(AVIERR_BADFORMAT);
	    }
	}
    }
#endif

    // Find where to do the paste...
    hr = ResolveEdits(lPos, &p, &l, &edit, TRUE);

    // Report back the size of what we pasted...
    if (plLength)
	*plLength = lLength;

    if (pnew) {
	LONG	lNew;

	// The inserted stream is itself an edit list; take advantage
	// of this fact.
	hr = AllocEditSpace(edit, 1 + pnew->cedits);

	this->edits[edit].pavi = this->edits[edit + 1 + pnew->cedits].pavi;
	if (this->edits[edit].pavi)
	    AVIStreamAddRef(this->edits[edit].pavi);
	this->edits[edit].lStart = this->edits[edit + 1 + pnew->cedits].lStart;
	this->edits[edit].lLength = l - this->edits[edit].lStart;

	// !!! We're ignoring lStart and lLength!
	for (lNew = 0; lNew < pnew->cedits; lNew++) {
	    this->edits[edit + 1 + lNew] = pnew->edits[lNew];
	    AVIStreamAddRef(pnew->edits[lNew].pavi);
	}

	this->edits[edit + pnew->cedits + 1].lStart = l;
	this->edits[edit + pnew->cedits + 1].lLength -= this->edits[edit].lLength;

	// Get rid of zero-length edits....
	PossiblyRemoveEdit(edit + pnew->cedits + 1);
	
	PossiblyRemoveEdit(edit);
	
	this->sinfo.dwLength += lLength;
	pnew->CheckEditList();

	pnew->Release();
    } else {
	// Just insert the stream as a blob.
	hr = AllocEditSpace(edit, 2);

	this->edits[edit].pavi = this->edits[edit+2].pavi;
	if (this->edits[edit].pavi)
	    AVIStreamAddRef(this->edits[edit].pavi);
	this->edits[edit].lStart = this->edits[edit+2].lStart;
	this->edits[edit].lLength = l - this->edits[edit+2].lStart;

	this->edits[edit+ 1].pavi = pstream;
	if (pstream)
	    AVIStreamAddRef(pstream);
	this->edits[edit + 1].lStart = lStart;
	this->edits[edit + 1].lLength = lLength;

	this->edits[edit + 2].lStart = l;
	this->edits[edit + 2].lLength -= this->edits[edit].lLength;
	// No addref here, since the edit we're splitting had a ref already
	
	this->sinfo.dwLength += lLength;

	// Get rid of zero-length edits....
	PossiblyRemoveEdit(edit + 2);
	
	PossiblyRemoveEdit(edit);
    }

    CheckEditList();
    this->sinfo.dwEditCount++;

    return AVIERR_OK;
}

STDMETHODIMP CEditStream::Clone(PAVISTREAM FAR *ppResult)
{
    CEditStream FAR *	pnew;
    HRESULT		hr;
    LONG		l;

    pnew = NewEditStream(NULL);
    *ppResult = (PAVISTREAM) pnew;
    if (!pnew)
	return ResultFromScode(AVIERR_MEMORY);

    if (this->cedits > 1) {
	hr = pnew->AllocEditSpace(1, this->cedits - 1);
	if (hr != NOERROR) {
	    // !!! Clean things up
	    return hr;
	}
    }

    for (l = 0; l < this->cedits; l++) {
	pnew->edits[l] = this->edits[l];
	if (pnew->edits[l].pavi)
	    AVIStreamAddRef(pnew->edits[l].pavi);
    }

    pnew->sinfo = this->sinfo;
    pnew->fFullFrames = this->fFullFrames;

    pnew->CheckEditList();

    return AVIERR_OK;
}

STDMETHODIMP CEditStream::SetInfo(AVISTREAMINFO FAR * lpInfo, LONG cbInfo)
{
    if ((cbInfo < sizeof(AVISTREAMINFO)) ||
	(IsBadReadPtr(lpInfo, sizeof(AVISTREAMINFO))))
	return ResultFromScode(AVIERR_BADPARAM);

    // Things we don't copy:
    // fccType
    // fccHandler
    // dwFlags
    // dwCaps
    // dwLength
    // dwInitialFrames
    // dwSuggestedBufferSize
    // dwSampleSize
    // dwEditCount
    // dwFormatChangeCount

    this->sinfo.wPriority = lpInfo->wPriority;
    this->sinfo.wLanguage = lpInfo->wLanguage;
    this->sinfo.dwScale   = lpInfo->dwScale;
    this->sinfo.dwRate    = lpInfo->dwRate;
    this->sinfo.dwStart   = lpInfo->dwStart;  // !!! ???
    this->sinfo.dwQuality = lpInfo->dwQuality;
    this->sinfo.rcFrame   = lpInfo->rcFrame;

    if (lpInfo->szName[0])
	_fmemcpy(this->sinfo.szName, lpInfo->szName, sizeof(this->sinfo.szName));

    // The stream has been changed....
    ++this->sinfo.dwEditCount;

    return NOERROR;
}



//
//
//   Extra unimplemented functions.....
//
//
//
HRESULT STDMETHODCALLTYPE CEditStream::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (riid == IID_IUnknown)
	*ppvObj = ((IUnknown FAR *) (IAVIStream FAR *) this);
    else if (riid == IID_IAVIStream)
	*ppvObj = ((IAVIStream FAR *) this);
    else if (riid == IID_IAVIEditStream)
	*ppvObj = ((IAVIEditStream FAR *) this);
#if 0
    else if ((riid == IID_IMarshal) && CanMarshalSimply()) // !!!! Remove once fixed!
	*ppvObj = ((IMarshal FAR *) this);
#endif
    else if (riid == CLSID_EditStream)
	*ppvObj = this;
    else {                 // unsupported interface
        *ppvObj = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    AddRef();

    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CEditStream::ReadData     (DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::SetFormat    (LONG lPos, LPVOID lpFormat, LONG cbFormat)
{
    // !!! We could set the whole format of the stream here, and do mapping....

    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::WriteData    (DWORD fcc, LPVOID lp, LONG cb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::Write        (LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::Delete       (LONG lStart, LONG lSamples)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT CEditStream::NewInstance(IUnknown FAR* pUnknownOuter,
			       REFIID riid,
			       LPVOID FAR* ppv)
{
    CEditStream FAR *	pedit;
    HRESULT		hr;

    pedit = new CEditStream;

    if (pedit)
	(pedit->Create)(NULL, 0);
    // !!! error check

    hr = pedit->QueryInterface(riid, ppv);

    if (FAILED(GetScode(hr)))
	delete pedit;

    return hr;
}

BOOL CEditStream::CanMarshalSimply()
{
    LONG	l;
    LPUNKNOWN	punk;

    for (l = 0; l < this->cedits; l++) {
	punk = NULL;

	this->edits[l].pavi->QueryInterface(CLSID_AVISimpleUnMarshal,
					    (LPVOID FAR *) &punk);

	if (!punk)
	    return FALSE;
	
	punk->Release();
    }

    return TRUE;
}

STDMETHODIMP CEditStream::GetUnmarshalClass (REFIID riid, LPVOID pv,
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPCLSID pCid)
{
    if (CanMarshalSimply()) {
	DPF("UnMarshalClass called (simple)\n");

	*pCid = CLSID_AVISimpleUnMarshal;
    } else {
	DPF("UnMarshalClass called (not simple)\n");

	*pCid = CLSID_EditStream;
    }
    return NOERROR;
}

STDMETHODIMP CEditStream::GetMarshalSizeMax (REFIID riid, LPVOID pv,
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPDWORD pSize)
{
    if (CanMarshalSimply())
	*pSize = 4;
    else
	*pSize = 0;

    return NOERROR;
}

STDMETHODIMP CEditStream::MarshalInterface (LPSTREAM pStm, REFIID riid,
		    LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags)
{
    HRESULT	hr;

    if ((riid != IID_IAVIStream && riid != IID_IAVIFile && riid != IID_IUnknown))
        return ResultFromScode(E_INVALIDARG);

    if (CanMarshalSimply()) {
	LPUNKNOWN pUnk = (LPUNKNOWN) (PAVISTREAM) this;
	
	DPF("MarshalInterface called (simple): Marshalling %lx\n", (DWORD) pUnk);
	if ((hr = pStm->Write(&pUnk, sizeof(pUnk), NULL)) == NOERROR)
	    AddRef();
    } else {

	DPF("MarshalInterface called (not simple)\n");
	hr = ResultFromScode(AVIERR_UNSUPPORTED);

    }

    DPF("Returns %lx\n", (DWORD) (LPVOID) hr);

    return hr;
}

STDMETHODIMP CEditStream::UnmarshalInterface (LPSTREAM pStm, REFIID riid,
		    LPVOID FAR* ppv)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CEditStream::ReleaseMarshalData (LPSTREAM pStm)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CEditStream::DisconnectObject (DWORD dwReserved)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}



HRESULT STDMETHODCALLTYPE CEditStream::Reserved1()
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::Reserved2()
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::Reserved3()
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::Reserved4()
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE CEditStream::Reserved5()
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}



/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * ICSAMPLE=1
 *
 ****************************************************************************/

#ifdef DEBUG

#define MODNAME "EditStrm"
static BOOL  fDebug = -1;

static void cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];

#ifdef WIN32
    va_list va;
    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug",MODNAME, FALSE);

    if (!fDebug)
        return;

    va_start(va, szFormat);
    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)va);
    va_end(va);
//  lstrcatA(ach, "\r\r\n");

    OutputDebugStringA(ach);
#else
    if (fDebug == -1)
        fDebug = GetProfileInt("Debug",MODNAME, FALSE);

    if (!fDebug)
        return;

    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpy(ach, MODNAME ": ");

    wvsprintf(ach+lstrlen(ach),szFormat,(LPSTR)(&szFormat+1));
//  lstrcat(ach, "\r\r\n");

    OutputDebugString(ach);
#endif
}

#endif
