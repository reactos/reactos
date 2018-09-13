/****************************************************************************
 *
 *  AVICLIP.C
 *
 *  Clipboard support for AVIFile
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
#include <compobj.h>
#include <ole2.h>
#include <dvobj.h>
#include <compman.h>
#include "avifile.h"
#include "avimem.h"
#include "enumfetc.h"
#include "debug.h"

//#define TRYLINKS
#ifdef TRYLINKS
static  SZCODE aszLink[]              = "OwnerLink";
#endif

/* From avifps.h.... */
BOOL FAR TaskHasExistingProxies(void);

#define OWNER_DISPLAY   0

STDMETHODIMP AVIClipQueryInterface(LPDATAOBJECT lpd, REFIID riid, LPVOID FAR* ppvObj);
STDMETHODIMP_(ULONG) AVIClipAddRef(LPDATAOBJECT lpd);
STDMETHODIMP_(ULONG) AVIClipRelease(LPDATAOBJECT lpd);
STDMETHODIMP AVIClipGetData(LPDATAOBJECT lpd, LPFORMATETC pformatetcIn,
			LPSTGMEDIUM pmedium );
STDMETHODIMP AVIClipGetDataHere(LPDATAOBJECT lpd, LPFORMATETC pformatetc,
			LPSTGMEDIUM pmedium );
STDMETHODIMP AVIClipQueryGetData(LPDATAOBJECT lpd, LPFORMATETC pformatetc );
STDMETHODIMP AVIClipGetCanonicalFormatEtc(LPDATAOBJECT lpd, LPFORMATETC pformatetc,
			LPFORMATETC pformatetcOut);
STDMETHODIMP AVIClipSetData(LPDATAOBJECT lpd, LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
			BOOL fRelease);
STDMETHODIMP AVIClipEnumFormatEtc(LPDATAOBJECT lpd, DWORD dwDirection,
			LPENUMFORMATETC FAR* ppenumFormatEtc);
STDMETHODIMP AVIClipDAdvise(LPDATAOBJECT lpd, FORMATETC FAR* pFormatetc, DWORD advf,
		LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
STDMETHODIMP AVIClipDUnadvise(LPDATAOBJECT lpd, DWORD dwConnection);
STDMETHODIMP AVIClipEnumDAdvise(LPDATAOBJECT lpd, LPENUMSTATDATA FAR* ppenumAdvise);

HMODULE ghOLE2 = NULL; // handle to OLE2.DLL module

IDataObjectVtbl AVIClipVtbl = {
    AVIClipQueryInterface,
    AVIClipAddRef,
    AVIClipRelease,
    AVIClipGetData,
    AVIClipGetDataHere,
    AVIClipQueryGetData,
    AVIClipGetCanonicalFormatEtc,
    AVIClipSetData,
    AVIClipEnumFormatEtc,
    AVIClipDAdvise,
    AVIClipDUnadvise,
    AVIClipEnumDAdvise
};

#define N_FORMATS   (sizeof(FormatList) / sizeof(FormatList[0]))
FORMATETC FormatList[] = {
    // CF_WAVE must be first, see AVIPutFileOnClipboard
    {CF_WAVE, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    {CF_DIB, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    // CF_PALETTE must be last, see AVIPutFileOnClipboard
    {CF_PALETTE, NULL, DVASPECT_CONTENT, -1, TYMED_GDI}
};

#define AVICLIP_MAGIC   0x42424242

typedef struct {
    IDataObjectVtbl FAR * lpVtbl;
    DWORD               Magic;

    ULONG		ulRefCount;

    PAVIFILE		pf;

    WORD		wFormats;
    LPFORMATETC         lpFormats;

    //!!! what about IDataView
    //!!! what about a IGetFrame

    HWND                hwndMci;
    PGETFRAME           pgf;

} AVICLIP, FAR * LPAVICLIP;

#if OWNER_DISPLAY
static LRESULT CALLBACK _loadds ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LONG lParam);
static WNDPROC OldClipboardWindowProc;
static LPAVICLIP lpcClipboard;
#endif

#ifdef WIN32

static HRESULT STDAPICALLTYPE OleStubP(LPVOID p) {return ResultFromScode(E_FAIL);}
static HRESULT STDAPICALLTYPE OleStubV(void)     {return ResultFromScode(E_FAIL);}

HRESULT (STDAPICALLTYPE *XOleInitialize)(LPMALLOC pMalloc);
void    (STDAPICALLTYPE *XOleUninitialize)(void);
HRESULT (STDAPICALLTYPE *XOleFlushClipboard)(void);
HRESULT (STDAPICALLTYPE *XOleSetClipboard)(LPDATAOBJECT pDataObj);
HRESULT (STDAPICALLTYPE *XOleGetClipboard)(LPDATAOBJECT FAR* ppDataObj);

#define OleInitialize     XOleInitialize
#define OleUninitialize   XOleUninitialize
#define OleGetClipboard   XOleGetClipboard
#define OleSetClipboard   XOleSetClipboard
#define OleFlushClipboard XOleFlushClipboard
#endif

HRESULT NEAR PASCAL InitOle(void)
{
    UINT w;

    if (!ghOLE2) {
	DPF("Loading OLE2.DLL\n");
	w = SetErrorMode(SEM_NOOPENFILEERRORBOX);
#ifdef WIN32
	ghOLE2 = LoadLibrary("OLE2W32.DLL");
#else
	ghOLE2 = LoadLibrary("OLE2.DLL");
#endif
	SetErrorMode(w);
    }

#ifndef WIN32
    if ((UINT)ghOLE2 <= (UINT)HINSTANCE_ERROR)
    	ghOLE2 = NULL;
#endif

#ifdef WIN32
    //
    // dyna link to OLE on NT
    //
    if (ghOLE2)
    {
        (FARPROC)XOleInitialize     = GetProcAddress(ghOLE2, "OleInitialize");
        (FARPROC)XOleUninitialize   = GetProcAddress(ghOLE2, "OleUninitialize");
        (FARPROC)XOleGetClipboard   = GetProcAddress(ghOLE2, "OleGetClipboard");
        (FARPROC)XOleSetClipboard   = GetProcAddress(ghOLE2, "OleSetClipboard");
        (FARPROC)XOleFlushClipboard = GetProcAddress(ghOLE2, "OleFlushClipboard");
    }
    else
    {
        (FARPROC)XOleInitialize     = (FARPROC)OleStubP;
        (FARPROC)XOleGetClipboard   = (FARPROC)OleStubP;
        (FARPROC)XOleSetClipboard   = (FARPROC)OleStubP;
        (FARPROC)XOleUninitialize   = (FARPROC)OleStubV;
        (FARPROC)XOleFlushClipboard = (FARPROC)OleStubV;
    }    	
#endif

    return OleInitialize(NULL);
}

STDAPI AVIGetDataObject(PAVIFILE pf, LPDATAOBJECT FAR *ppDataObj)
{
    LPAVICLIP	lpc;
    PAVISTREAM	ps;

    if (pf == NULL) {
	*ppDataObj = NULL;
	return NOERROR;
    }
    
    AVIFileAddRef(pf);
    
    lpc = (LPAVICLIP) GlobalAllocPtr(GHND | GMEM_SHARE, sizeof(AVICLIP));

    if (!lpc)
	return ResultFromScode(AVIERR_MEMORY);
    
    lpc->lpVtbl = &AVIClipVtbl;
    lpc->ulRefCount = 1;
    lpc->pf = pf;

    lpc->wFormats = N_FORMATS;
    lpc->lpFormats = FormatList;
    lpc->Magic = AVICLIP_MAGIC;

    //
    // if there is no video in the file, dont offer video
    // CF_WAVE must be first.
    //
    if (AVIFileGetStream(pf, &ps, streamtypeVIDEO, 0L) != NOERROR) {
        lpc->wFormats = 1;
    }
    else {
        //
        // if the video format is higher than 8bpp dont offer a palette
        // CF_PALETTE must be last.
        //
	AVISTREAMINFO		strhdr;
	BITMAPINFOHEADER	bi;
	DWORD			dwcbFormat;

	// get the stream header
	AVIStreamInfo(ps, &strhdr, sizeof(strhdr));
	
	// now read the format of this thing
	dwcbFormat = sizeof(bi);
	AVIStreamReadFormat(ps, strhdr.dwStart, (LPVOID)&bi, (LONG FAR *)&dwcbFormat);

	// if it is true color (i.e., > 8bpp) then don't use the palette
        if (bi.biBitCount > 8) {
	    DPF("Turning off CF_PALETTE now\n");
            lpc->wFormats--;	// don't use CF_PALETTE
        }

        ps->lpVtbl->Release(ps);  
    }

    //
    // if there is no audio in the file, dont offer audio
    // CF_WAVE must be first.
    //
    if (AVIFileGetStream(pf, &ps, streamtypeAUDIO, 0L) != NOERROR) {
        lpc->wFormats--;
        lpc->lpFormats++;
    }
    else {
        ps->lpVtbl->Release(ps);
    }

    *ppDataObj = (LPDATAOBJECT) lpc;

    return 0;
}


/**************************************************************************
* @doc EXTERNAL AVIPutFileOnClipboard
*
* @api HRESULT | AVIPutFileOnClipboard | Puts a file described by the passed
*	in PAVIFILE onto the clipboard.
*
* @parm PAVIFILE | pfile | Handle representing the file to put on the clipboard.
*
* @comm 
*
* @rdesc Returns zero on success or an error code.
*
* @xref AVIPutStreamOnClipboard AVIGetFromClipboard
*
*************************************************************************/
STDAPI AVIPutFileOnClipboard(PAVIFILE pf)
{
    LPDATAOBJECT lpd;
    HRESULT	hr;

    hr = AVIGetDataObject(pf, &lpd);

    InitOle();

    hr = OleSetClipboard(lpd);

    if (lpd) {
	lpd->lpVtbl->Release(lpd);

#if OWNER_DISPLAY
	lpcClipboard = lpc;

	//
	// hook the clipboard owner so we can do OWNER_DISPLAY formats
	//
	{
	HWND hwnd = GetClipboardOwner();

	if (OldClipboardWindowProc == NULL) {

	    if (hwnd) {
		OldClipboardWindowProc = (WNDPROC)SetWindowLong(hwnd,
		    GWL_WNDPROC, (LONG)ClipboardWindowProc);
	    }
	}

	if (OpenClipboard(hwnd)) {
	    SetClipboardData(CF_OWNERDISPLAY, NULL);
	    CloseClipboard();
	}
	}
#endif
    }
    
    return hr;
}

/**************************************************************************
* @doc EXTERNAL AVIGetFromClipboard
*
* @api HRESULT | AVIGetFromClipboard | Get a file or stream off of the
*	clipboard.
*
* @parm PAVIFILE FAR * | ppfile | Pointer to a variable that can 
*
* @comm If <p ppfile> is not NULL, the function will first attempt to
*	retrieve a file from the clipboard.  Then, if <p ppstream> is not
*	NULL, it will attempt to retrieve a stream.
*
*	Any file or stream retrieved from the clipboard using this
*	function should eventually be released with <f AVIStreamClose>
*	or <f AVIFileClose>.
*
* @rdesc Returns zero on success or an error code.  If there is no suitable
*	data on the clipboard, no error code will be returned, but
*	the returned variables will be NULL.
*
* @xref AVIPutStreamOnClipboard AVIGetFromClipboard
*
*************************************************************************/
STDAPI AVIGetFromClipboard(PAVIFILE FAR * lppf)
{
    LPDATAOBJECT	lpd = NULL;
    HRESULT		hr = NOERROR;
    FORMATETC		fetc;
    STGMEDIUM		stg;

    if (!lppf)
	return ResultFromScode(E_POINTER);
	
    *lppf = NULL;

    InitOle();

    OleGetClipboard(&lpd);

    if (lpd) {
#ifdef DEBUGXX
	// Print out lots of stuff about what's on the clipboard....
	{
	    LPENUMFORMATETC	lpEnum = NULL;
	    char		achTemp[256];

	    lpd->lpVtbl->EnumFormatEtc(lpd, DATADIR_GET, &lpEnum);

	    if (lpEnum) {
		DPF("Formats available:\n");
		while(lpEnum->lpVtbl->Next(lpEnum, 1,
					   (LPFORMATETC)&fetc,
					   NULL) == NOERROR) {
		    achTemp[0] = '\0';
		    GetClipboardFormatName(fetc.cfFormat, achTemp, sizeof(achTemp));
		    DPF("\t%u\t%lu\t%s\n", fetc.cfFormat, fetc.tymed, (LPSTR) achTemp);

		    if ((fetc.cfFormat == CF_WAVE) ||
			    (fetc.cfFormat == CF_DIB) ||
			    (fetc.cfFormat == CF_RIFF) ||
			    (fetc.cfFormat == CF_METAFILEPICT) ||
			    (fetc.cfFormat == CF_BITMAP) ||
			    (fetc.cfFormat == CF_PENDATA))
			continue;
		    
		    if (fetc.tymed & TYMED_HGLOBAL) {
			fetc.tymed = TYMED_HGLOBAL;
			hr = lpd->lpVtbl->GetData(lpd, &fetc, &stg);
			if (hr == 0) {
			    LPVOID  lp = GlobalLock(stg.hGlobal);
			    DPF("%s\n", (LPSTR) lp);
			    
			    ReleaseStgMedium(&stg);
			}
		    }
		}
	    }
	}
#endif
	
	lpd->lpVtbl->QueryInterface(lpd, &IID_IAVIFile, lppf);

	// Try for IAVIStream here?

#ifdef TRYLINKS
	// See if there's a link to a type of file we can open....
	if (!*lppf) {
	    UINT        cfLink;

	    cfLink      = RegisterClipboardFormat(aszLink);

	    fetc.cfFormat = cfLink;
	    fetc.ptd = 0;
	    fetc.dwAspect = DVASPECT_CONTENT;
	    fetc.lindex = -1;
	    fetc.tymed = TYMED_HGLOBAL;

	    hr = lpd->lpVtbl->GetData(lpd, &fetc, &stg);

	    if (hr == 0) {
		LPSTR lp = GlobalLock(stg.hGlobal);
		LPSTR lpName;

		lpName = lp + lstrlen(lp) + 1;
		DPF("Got CF_LINK (%s/%s) data from clipboard...\n", lp,lpName);
		hr = AVIFileOpen(lppf, lpName, OF_READ | OF_SHARE_DENY_WRITE, NULL);

		if (hr == 0) {
		    DPF("Opened file from link!\n");

		    // !!! If the app name is "MPlayer", we could get
		    // the selection out of the data....
		}

		ReleaseStgMedium(&stg);
	    }
	}
#endif
	
	if (!*lppf) {
	    PAVISTREAM	aps[2];
	    int		cps = 0;
	    
	    fetc.cfFormat = CF_DIB;
	    fetc.ptd = 0;
	    fetc.dwAspect = DVASPECT_CONTENT;
	    fetc.lindex = -1;
	    fetc.tymed = TYMED_HGLOBAL;

	    // CF_BITMAP, CF_PALETTE?
	    
	    hr = lpd->lpVtbl->GetData(lpd, &fetc, &stg);

	    if (hr == 0) {
		DPF("Got CF_DIB data from clipboard...\n");
		hr = AVIMakeStreamFromClipboard(CF_DIB, stg.hGlobal, &aps[cps]);

		if (hr == 0) {
		    cps++;
		}

		ReleaseStgMedium(&stg);
	    }
	    
	    fetc.cfFormat = CF_WAVE;
	    fetc.ptd = 0;
	    fetc.dwAspect = DVASPECT_CONTENT;
	    fetc.lindex = -1;
	    fetc.tymed = TYMED_HGLOBAL;

	    
	    hr = lpd->lpVtbl->GetData(lpd, &fetc, &stg);

	    if (hr == 0) {
		DPF("Got CF_WAVE data from clipboard...\n");
		hr = AVIMakeStreamFromClipboard(CF_WAVE, stg.hGlobal, &aps[cps]);

		if (hr == 0) {
		    cps++;
		}

		ReleaseStgMedium(&stg);
	    }

	    if (cps) {
		hr = AVIMakeFileFromStreams(lppf, cps, aps);

		while (cps-- > 0)
		    AVIStreamClose(aps[cps]);
	    } else    
		hr = ResultFromScode(AVIERR_NODATA);
	}
	
	lpd->lpVtbl->Release(lpd);
    }

    OleUninitialize();

    return hr;
}

/**************************************************************************
* @doc EXTERNAL AVIClearClipboard
*
* @api HRESULT | AVIClearClipboard | Releases any file or stream that
*	has been put on the Clipboard. 
*
* @comm Applications should use this function before exiting if they use
*	     other Clipboard routines.  Do not use this function just to
*       clear the clipboard; it might not return until other
*       applications have finished using the data placed on the Clipboard.
*       Ideally, call this function after hiding your application's windows.
*
* @rdesc Returns zero on success or an error code.
*
* @xref AVIPutStreamOnClipboard AVIGetFromClipboard
*
*************************************************************************/
STDAPI AVIClearClipboard(void)
{
    HRESULT hr;
    
    InitOle();

    hr = OleFlushClipboard();

    while (TaskHasExistingProxies()) {
	MSG msg;

	DPF("AVIClearClipboard: Waiting while streams in use....\n");
	while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }
    
    OleUninitialize();

    return hr;
}

typedef     LPBITMAPINFOHEADER PDIB;

#ifndef BI_BITFIELDS
	#define BI_BITFIELDS 3
#endif  

#ifndef HALFTONE
	#define HALFTONE COLORONCOLOR
#endif


#define DibCompression(lpbi)    (DWORD)(((LPBITMAPINFOHEADER)(lpbi))->biCompression)
#define DibColors(lpbi)         ((RGBQUAD FAR *)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)


/*
 *  CreateBIPalette()
 *
 *  Given a Pointer to a BITMAPINFO struct will create a
 *  a GDI palette object from the color table.
 *
 */
HPALETTE DibCreatePalette(PDIB pdib)
{
    LOGPALETTE         *pPal;
    HPALETTE            hpal = NULL;
    int                 nNumColors;
    int                 i;
    RGBQUAD FAR *       pRgb;

    if (!pdib)
        return NULL;

    nNumColors = DibNumColors(pdib);
    
    if (nNumColors == 3 && DibCompression(pdib) == BI_BITFIELDS)
        nNumColors = 0;

    if (nNumColors > 0)
    {
        pRgb = DibColors(pdib);
        pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));

        if (!pPal)
            goto exit;

        pPal->palNumEntries = nNumColors;
        pPal->palVersion    = 0x300;

        for (i = 0; i < nNumColors; i++)
        {
            pPal->palPalEntry[i].peRed   = pRgb->rgbRed;
            pPal->palPalEntry[i].peGreen = pRgb->rgbGreen;
            pPal->palPalEntry[i].peBlue  = pRgb->rgbBlue;
            pPal->palPalEntry[i].peFlags = (BYTE)0;

            pRgb++;
        }

        hpal = CreatePalette(pPal);
        LocalFree((HLOCAL)pPal);
    }
    else
    {
#ifdef WIN32                 
        HDC hdc = GetDC(NULL);
        hpal = CreateHalftonePalette(hdc);      
        ReleaseDC(NULL, hdc);
#endif          
    }

exit:
    return hpal;
}

STDMETHODIMP AVIClipQueryInterface(LPDATAOBJECT lpd, REFIID riid, LPVOID FAR* ppvObj)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;
    SCODE scode;

    if (IsEqualIID(riid, &IID_IDataObject) || 
			IsEqualIID(riid, &IID_IUnknown)) {
	
	DPF2("Clip   %lx: Usage++=%lx\n", (DWORD) (LPVOID) lpd, lpc->ulRefCount + 1);
    
        ++lpc->ulRefCount;
        *ppvObj = lpd;
        scode = S_OK;
    }
    else if (lpc->pf && IsEqualIID(riid, &IID_IAVIFile)) {
	AVIFileAddRef(lpc->pf);
	*ppvObj = lpc->pf;
	scode = S_OK;
    }
    else {                 // unsupported interface
        *ppvObj = NULL;
        scode = E_NOINTERFACE;
    }

    return ResultFromScode(scode);
}

STDMETHODIMP_(ULONG) AVIClipAddRef(LPDATAOBJECT lpd) 
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    DPF2("Clip   %lx: Usage++=%lx\n", (DWORD) (LPVOID) lpd, lpc->ulRefCount + 1);
    
    return ++lpc->ulRefCount;    
}

STDMETHODIMP_(ULONG) AVIClipRelease(LPDATAOBJECT lpd)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    DPF2("Clip   %lx: Usage--=%lx\n", (DWORD) (LPVOID) lpd, lpc->ulRefCount - 1);
    
    if (--lpc->ulRefCount)
	return lpc->ulRefCount;
    
    if (lpc->pf)
        AVIFileClose(lpc->pf);

    if (lpc->pgf)
        AVIStreamGetFrameClose(lpc->pgf);

    if (lpc->hwndMci)
        DestroyWindow(lpc->hwndMci);

#if OWNER_DISPLAY
    if (lpc == lpcClipboard)
        lpcClipboard = NULL;
#endif

    GlobalFreePtr(lpc);
    OleUninitialize();
    
    return 0;
}


// *** IDataObject METHODIMPs ***
STDMETHODIMP AVIClipGetData(LPDATAOBJECT lpd, LPFORMATETC pformatetcIn,
			LPSTGMEDIUM pmedium )
{
    LPAVICLIP		lpc = (LPAVICLIP) lpd;
    SCODE		sc = S_OK;

    LPVOID		lp;
    LPBITMAPINFOHEADER	lpbi;
    DWORD		cb;
    PGETFRAME		pgf = NULL;
    PAVISTREAM		ps = NULL;

    if (pformatetcIn->cfFormat == CF_DIB ||
	pformatetcIn->cfFormat == CF_PALETTE) {
	
	AVIFileGetStream(lpc->pf, &ps, streamtypeVIDEO, 0L);

	if (!ps) {
	    sc = E_FAIL;
	    goto error;
	}
	
	pgf = AVIStreamGetFrameOpen(ps, NULL);

	if (!pgf) {
	    DPF("AVIClipGetData: AVIStreamGetFrameOpen failed!\n");
	    sc = E_FAIL;
	    goto error;
	}
	
	lpbi = AVIStreamGetFrame(pgf, 0);
	
        if (! lpbi) {
	    DPF("AVIClipGetData: AVIStreamGetFrame failed!\n");
            sc = E_OUTOFMEMORY;
            goto error;
        }

	if (pformatetcIn->cfFormat == CF_DIB) {
	    DPF("Building CF_DIB data\n");
	    // Verify caller asked for correct medium
	    if (!(pformatetcIn->tymed & TYMED_HGLOBAL)) {
		sc = DATA_E_FORMATETC;
		goto error;
	    }

	    cb = lpbi->biSize +
		 lpbi->biClrUsed * sizeof(RGBQUAD) +
		 lpbi->biSizeImage;
	    pmedium->hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cb);

	    if (!pmedium->hGlobal) {
		sc = E_OUTOFMEMORY;
		goto error;
	    }

	    lp = GlobalLock(pmedium->hGlobal);

	    hmemcpy(lp, lpbi, cb);

	    GlobalUnlock(pmedium->hGlobal);
	    
	    pmedium->tymed = TYMED_HGLOBAL;
	} else /* if (pformatetcIn->cfFormat == CF_PALETTE) */ {
	    HPALETTE	hpal;

	    // Verify caller asked for correct medium
	    if (!(pformatetcIn->tymed & TYMED_GDI)) {
		sc = DATA_E_FORMATETC;
		goto error;
	    }

	    hpal = DibCreatePalette(lpbi);

	    pmedium->hGlobal = hpal;
	    pmedium->tymed = TYMED_GDI;
	    DPF("Building CF_PALETTE data: hpal = %x\n", (UINT) hpal);
	}
    } else if (pformatetcIn->cfFormat == CF_WAVE) {
	LONG		cbFormat;
	AVISTREAMINFO	strhdr;
	DWORD _huge *	hpdw;
#define formtypeWAVE            mmioFOURCC('W', 'A', 'V', 'E')
#define ckidWAVEFORMAT          mmioFOURCC('f', 'm', 't', ' ')
#define ckidWAVEDATA	        mmioFOURCC('d', 'a', 't', 'a')
	
	DPF("Building CF_WAVE data\n");
	AVIFileGetStream(lpc->pf, &ps, streamtypeAUDIO, 0L);

	if (!ps) {
	    sc = E_FAIL;
	    goto error;
	}

	AVIStreamInfo(ps, &strhdr, sizeof(strhdr));

	AVIStreamReadFormat(ps, strhdr.dwStart, NULL, &cbFormat);
	
	cb = strhdr.dwLength * strhdr.dwSampleSize +
	     cbFormat + 5 * sizeof(DWORD) + 2 * sizeof(DWORD);
	
	pmedium->hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cb);

	if (!pmedium->hGlobal) {
	    sc = E_OUTOFMEMORY;
	    goto error;
	}

	lp = GlobalLock(pmedium->hGlobal);

	hpdw = (DWORD _huge *) lp;
	
	*((DWORD _huge *)lp)++ = FOURCC_RIFF;
	*((DWORD _huge *)lp)++ = cb - 2 * sizeof(DWORD);
	*((DWORD _huge *)lp)++ = formtypeWAVE;

	*((DWORD _huge *)lp)++ = ckidWAVEFORMAT;
	*((DWORD _huge *)lp)++ = cbFormat;

	AVIStreamReadFormat(ps, strhdr.dwStart, lp, &cbFormat);

	lp = (BYTE _huge *) lp + cbFormat;

	cb = strhdr.dwLength * strhdr.dwSampleSize;
	*((DWORD _huge *)lp)++ = ckidWAVEDATA;
	*((DWORD _huge *)lp)++ = cb;

	AVIStreamRead(ps, strhdr.dwStart, strhdr.dwLength, lp, cb, NULL, NULL);
	
	GlobalUnlock(pmedium->hGlobal);
	    
	pmedium->tymed = TYMED_HGLOBAL;	
    } else {
        sc = DATA_E_FORMATETC;
	
	goto error;
    }
    
error:

    if (pgf)
	AVIStreamGetFrameClose(pgf);
    if (ps)
	AVIStreamClose(ps);

    DPF2("GetData returns %lx\n", (DWORD) sc);
    return ResultFromScode(sc);
}

STDMETHODIMP AVIClipGetDataHere(LPDATAOBJECT lpd, LPFORMATETC pformatetc,
			LPSTGMEDIUM pmedium )
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    return ResultFromScode(DATA_E_FORMATETC);
}

STDMETHODIMP AVIClipQueryGetData(LPDATAOBJECT lpd, LPFORMATETC pformatetc )
{
    LPAVICLIP		lpc = (LPAVICLIP) lpd;
    PAVISTREAM		ps = NULL;

    if (pformatetc->cfFormat == CF_DIB) {
	AVIFileGetStream(lpc->pf, &ps, streamtypeVIDEO, 0L);

	if (ps) {
	    ps->lpVtbl->Release(ps);
	    if (pformatetc->tymed & TYMED_HGLOBAL) {
		return NOERROR;
	    } else {
		return ResultFromScode(DATA_E_FORMATETC);
	    }
	}	
    } else if (pformatetc->cfFormat == CF_PALETTE) {
	AVIFileGetStream(lpc->pf, &ps, streamtypeVIDEO, 0L);

	if (ps) {
	    ps->lpVtbl->Release(ps);
	    if (pformatetc->tymed & TYMED_GDI) {
		return NOERROR;
	    } else {
		return ResultFromScode(DATA_E_FORMATETC);
	    }
	}
    } else if (pformatetc->cfFormat == CF_WAVE) {
	AVIFileGetStream(lpc->pf, &ps, streamtypeAUDIO, 0L);

	if (ps) {
	    ps->lpVtbl->Release(ps);
	    if (pformatetc->tymed & TYMED_HGLOBAL) {
		return NOERROR;
	    } else {
		return ResultFromScode(DATA_E_FORMATETC);
	    }
	}
    } 

    return ResultFromScode(DATA_E_FORMATETC);    
}

STDMETHODIMP AVIClipGetCanonicalFormatEtc(LPDATAOBJECT lpd, LPFORMATETC pformatetc,
			LPFORMATETC pformatetcOut)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    return ResultFromScode(E_NOTIMPL);    
}

STDMETHODIMP AVIClipSetData(LPDATAOBJECT lpd, LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
			BOOL fRelease)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    return ResultFromScode(E_FAIL);
}

STDMETHODIMP AVIClipEnumFormatEtc(LPDATAOBJECT lpd, DWORD dwDirection,
			LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    SCODE sc = S_OK;
    if (dwDirection == DATADIR_GET) {
	// Build an enumerator....
        *ppenumFormatEtc = OleStdEnumFmtEtc_Create(
				lpc->wFormats, lpc->lpFormats);
	
        if (*ppenumFormatEtc == NULL)
            sc = E_OUTOFMEMORY;
    } else if (dwDirection == DATADIR_SET) {
        /* OLE2NOTE: a document that is used to transfer data
        **    (either via the clipboard or drag/drop does NOT
        **    accept SetData on ANY format! 
        */
        sc = E_NOTIMPL;
        goto error;
    } else {
        sc = E_INVALIDARG;
        goto error;
    }
    
error:
    return ResultFromScode(sc);    
}


STDMETHODIMP AVIClipDAdvise(LPDATAOBJECT lpd, FORMATETC FAR* pFormatetc, DWORD advf, 
		LPADVISESINK pAdvSink, DWORD FAR* pdwConnection)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP AVIClipDUnadvise(LPDATAOBJECT lpd, DWORD dwConnection)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP AVIClipEnumDAdvise(LPDATAOBJECT lpd, LPENUMSTATDATA FAR* ppenumAdvise)
{
    LPAVICLIP	lpc = (LPAVICLIP) lpd;

    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

#if OWNER_DISPLAY

/**************************************************************************
* @doc INTERNAL AVIFILE
*
* @api ClipboardWindowProc
*
*************************************************************************/
static LRESULT CALLBACK _loadds ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LONG lParam)
{
    WNDPROC x;
    HWND hwndViewer;
    PAINTSTRUCT ps;
    RECT rc;
    LPAVICLIP lpc;

    switch (msg) {
        case WM_DESTROY:
        case WM_DESTROYCLIPBOARD:
            DPF("WM_DESTROYCLIPBOARD\n");

            x = OldClipboardWindowProc;
            SetWindowLong(hwnd, GWL_WNDPROC, (LONG)x);
            OldClipboardWindowProc = NULL;
            return (*x)(hwnd, msg, wParam, lParam);

        case WM_RENDERFORMAT:
            DPF("WM_RENDERFORMAT cf=%d\n", (int)wParam);
            break;

        case WM_PAINTCLIPBOARD:
            DPF("WM_PAINTCLIPBOARD\n");

            hwndViewer = (HWND)wParam;

            if (!lParam)
                break;

            lpc = lpcClipboard;

            if (lpc == NULL)
                break;

            ps = *(LPPAINTSTRUCT)GlobalLock((HGLOBAL)lParam);

            FillRect(ps.hdc, &ps.rcPaint, GetStockObject(DKGRAY_BRUSH));
            return 0;
            break;

        case WM_SIZECLIPBOARD:
            DPF("WM_SIZECLIPBOARD\n");

            hwndViewer = (HWND)wParam;

            lpc = lpcClipboard;

            if (lpc == NULL)
                break;

            if (lParam)
                rc = *(LPRECT)GlobalLock((HGLOBAL)lParam);
            else
                SetRectEmpty(&rc);

            if (IsRectEmpty(&rc)) {
            }
            else {
            }
            break;

        case WM_VSCROLLCLIPBOARD:
        case WM_HSCROLLCLIPBOARD:
            DPF("WM_VHSCROLLCLIPBOARD\n");
            hwndViewer = (HWND)wParam;
            break;

        case WM_ASKCBFORMATNAME:
            DPF("WM_ASKCBFORMATNAME\n");
            break;
    }

    return OldClipboardWindowProc(hwnd, msg, wParam, lParam);
}

#endif
