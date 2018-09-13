/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   aviopen.c - open a AVI file

*****************************************************************************/
#include "graphic.h"
#ifdef _WIN32
#include <wchar.h>
#endif

#ifdef USEAVIFILE
    #include <initguid.h>
    #include <scode.h>

    DEFINE_AVIGUID(IID_IAVIFile,            0x00020020, 0, 0);
    DEFINE_AVIGUID(IID_IAVIStream,          0x00020021, 0, 0);
#endif

#define comptypeNONE mmioFOURCC('N','O','N','E')

//
//  special error to force using AVIFile to try and open this file.
//
#define AVIERR_NOT_AVIFILE  4242

//
//  if this is defined we will always use AVIFILE.DLL, except for
//  1:1 interleaved files.
//
#ifdef USEAVIFILE
#define USE_AVIFILE_FOR_NON_INT
#endif

#ifdef _WIN32
BOOL runningInWow = 0; // Assume we are not in WOW
#endif


/***************************************************************************
 * Local function declarations
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL InitStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi);
STATICFN BOOL NEAR PASCAL InitVideoStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi);
STATICFN BOOL NEAR PASCAL InitAudioStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi);
STATICFN BOOL NEAR PASCAL InitOtherStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi);
STATICFN void NEAR PASCAL CloseStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi);


STATICFN BOOL NEAR PASCAL ParseNewHeader(NPMCIGRAPHIC npMCI);

STATICFN BOOL NEAR PASCAL OpenRiffAVIFile(NPMCIGRAPHIC npMCI, BOOL bForce);
STATICFN BOOL NEAR PASCAL OpenWithAVIFile(NPMCIGRAPHIC npMCI);
STATICFN BOOL NEAR PASCAL OpenInterface(NPMCIGRAPHIC npMCI);
#ifdef USEAVIFILE
STATICFN BOOL NEAR PASCAL OpenAVIFile(NPMCIGRAPHIC npMCI, IAVIFile FAR *pf);
STATICFN BOOL NEAR PASCAL OpenAVIStream(NPMCIGRAPHIC npMCI, int stream, IAVIStream FAR *pf);
#endif

INLINE static BOOL NEAR PASCAL IsRectBogus(RECT *prc);
#ifndef _WIN32
static LONG NEAR PASCAL atol(char *sz);
#endif // !_WIN32

#ifdef _WIN32XXXX
    #define GetFileDriveType GetDriveType
#else
    static  UINT NEAR PASCAL GetFileDriveType(LPTSTR szPath);
#endif


#ifndef _WIN32
SZCODE szOLENLSDLL[] = "OLE2NLS.DLL";
SZCODE szOLENLSAPI[] = "GetUserDefaultLangID";
#endif

//
// due to OLE stupidity, all ole calls must be single-threaded and thus on
// the app thread. so we moved the Open call to the app thread



//
// -- called on app thread -------------------------------------------------
//

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | mciaviOpenFile | Open an AVI file.
 *      the filename we are to open is passed to npMCI->szFileName.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

BOOL FAR PASCAL mciaviOpenFile(NPMCIGRAPHIC npMCI)
{
    //
    // mciaviOpenFile should not be called with a file open!
    //
    DPF(("mciaviOpenFile, name = >>%ls<<\n", npMCI->szFilename));
    Assert(npMCI->streams == 0);
    Assert(npMCI->hmmio == NULL);
    Assert(npMCI->hpIndex == NULL);
    Assert(!(npMCI->dwFlags & (
			MCIAVI_NOTINTERLEAVED |
			MCIAVI_ANIMATEPALETTE |
			MCIAVI_CANDRAW |
			MCIAVI_HASINDEX)));
    //
    //  !!!support open new
    //
    if (npMCI->szFilename[0] == '\0') {
    }

    //
    // what media is this file coming from, will be important later
    // when we play.
    //
    if (npMCI->szFilename[0] == TEXT('@')) {
	npMCI->uDriveType = DRIVE_INTERFACE;
	if (IsNTWOW()) {
	    DPF2(("Rejecting open from WOW because of an interface pointer\n"));
	    npMCI->dwTaskError=MCIERR_UNSUPPORTED_FUNCTION;
	    return(FALSE);
	    // Cannot support a 16 bit interface pointer from 32 bit code
	}
    }
    else
	npMCI->uDriveType = GetFileDriveType(npMCI->szFilename);

#ifdef DEBUG
    switch (npMCI->uDriveType) {
	case DRIVE_CDROM:
	    DOUT2("Media is a CD-ROM\n");
	    break;

	case DRIVE_REMOTE:
	    DOUT2("Media is a Network\n");
	    break;

	case DRIVE_FIXED:
	    DOUT2("Media is a Hard disk\n");
	    break;

	case DRIVE_REMOVABLE:
	    DOUT2("Media is a floppy disk\n");
	    break;

	case DRIVE_INTERFACE:
	    DOUT2("Media is OLE COM Interface\n");
	    break;

	default:
	    DPF(("Unknown Media type %d\n", npMCI->uDriveType));
	    break;
    }
#endif

#ifdef USEAVIFILE
    //
    // if the "filename" is of the form: '@########' then we assume we
    // have been pased a interface pointer of some sort.
    //
    if (npMCI->szFilename[0] == TEXT('@') &&
	OpenInterface(npMCI))
	goto DoneOpening;

    // !!! This will open even AVI files this way!
    if ((npMCI->dwOptionFlags & MCIAVIO_USEAVIFILE) &&
	OpenWithAVIFile(npMCI))
	goto DoneOpening;
#endif

    if (!OpenRiffAVIFile(npMCI, FALSE)) {


#ifdef USEAVIFILE
	//
	//  unable to open RIFF file, if it was because it was
	//  not a AVI file, then give AVIFile a try.
	//
	if (npMCI->dwTaskError == AVIERR_NOT_AVIFILE) {

	    npMCI->dwTaskError = 0;

	    if (OpenWithAVIFile(npMCI))
		goto DoneOpening;

	    // if this is a riff file that we could open but chose
	    // to pass to avifile, and avifile is not present, then we need
	    // to pass it back to OpenRiffAVIFile and force it to try
	    // and open it properly.

	    npMCI->dwTaskError = 0;
	    if (OpenRiffAVIFile(npMCI, TRUE))
		goto DoneOpening;

	    if (npMCI->dwTaskError == AVIERR_NOT_AVIFILE) {
		npMCI->dwTaskError = MCIERR_INVALID_FILE;
		// We cannot return an AVI specific error as there is
		// no way for the UI to get access to the error text.
		// Hence we use a generic mci error.
	    }
	}
#endif
	goto error;
    }

DoneOpening:
    npMCI->dwTaskError = 0;
    return TRUE;

error:
    mciaviCloseFile(npMCI);

    if (npMCI->dwTaskError == 0)
	npMCI->dwTaskError = MCIERR_INVALID_FILE;

    return FALSE;
}

#ifdef USEAVIFILE

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | OpenWithAVIFile | Open an file using AVIFile
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL OpenWithAVIFile(NPMCIGRAPHIC npMCI)
{
    IAVIFile FAR *pf = NULL;

    if (!InitAVIFile(npMCI))
	return FALSE;

    AVIFileOpen(&pf, npMCI->szFilename, MMIO_READ, 0);

    if (pf == NULL) {
	npMCI->dwTaskError = MCIERR_INVALID_FILE;
	return FALSE;
    }

    if (!OpenAVIFile(npMCI, pf)) {
	mciaviCloseFile(npMCI);
	pf->lpVtbl->Release(pf);
	return FALSE;
    }

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | OpenInterface | Open an interface pointer
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL OpenInterface(NPMCIGRAPHIC npMCI)
{
    IUnknown FAR *p;
    IAVIFile FAR *pf=NULL;
    IAVIStream FAR *ps=NULL;

    if (!InitAVIFile(npMCI))
	return FALSE;

    if (npMCI->szFilename[0] != TEXT('@')) {
	DPF1(("Failing to open interface for file %ls\n",npMCI->szFilename));
	return FALSE;
    }

#ifdef UNICODE
    p = (IUnknown FAR *)wcstol(npMCI->szFilename+1, NULL, 10);
#else
    p = (IUnknown FAR *)atol(npMCI->szFilename+1);
#endif

#ifdef USE_ISVALIDINTERFACE
    if (!IsValidInterface(p))
	return FALSE;
#endif

#ifndef _WIN32
    //!!!we need to do the PSP stuff? or will the TASK stuff in
    //!!!COMPOBJ screw us up?
    {
    extern void FAR SetPSP(UINT psp);
    SetPSP(npMCI->pspParent);
    }
#endif

    p->lpVtbl->QueryInterface(p, &IID_IAVIFile, (LPVOID FAR *)&pf);

    if (pf != NULL)
    {
	if (OpenAVIFile(npMCI, pf))
	    return TRUE;

	pf->lpVtbl->Release(pf);
	pf = NULL;  // AVIMakeFileFromStreams need not set pf to NULL
    }

    p->lpVtbl->QueryInterface(p, &IID_IAVIStream, (LPVOID FAR *)&ps);

    if (ps != NULL)
    {
	AVIMakeFileFromStreams(&pf, 1, &ps);
	ps->lpVtbl->Release(ps);

	if (pf == NULL)
	    return FALSE;

	if (OpenAVIFile(npMCI, pf))
	    return TRUE;

	pf->lpVtbl->Release(pf);
	//return FALSE;
    }

    return FALSE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | OpenAVIFile | Open an a AVIFile object
 *
 *  NOTE we do not do call AddRef() we assume we dont need to.
 *       neither do we Release().  If we fail, our caller will Release(),
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *        On a successful return npMCI->pf will be loaded with the
 *        input IAVIFILE interface pointer.
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL OpenAVIFile(NPMCIGRAPHIC npMCI, IAVIFile FAR *pf)
{
    AVIFILEINFOW info;
    HRESULT hr;
    IAVIStream FAR *ps;
    STREAMINFO *psi;
    int i;

    Assert(npMCI->pf == NULL);

    _fmemset(&info, 0, sizeof(info));
    hr = pf->lpVtbl->Info(pf, &info, sizeof(info));

    if (FAILED(GetScode(hr))) {
	npMCI->dwTaskError = MCIERR_INVALID_FILE;
	return FALSE;
    }

    DPF1(("OpenAVIFile: %ls\n\t#Streams=%d,  Width=%d    Height=%d\n",
	    info.szFileType, info.dwStreams, info.dwWidth, info.dwHeight));

    //
    // get rid of bad files
    //
    if (info.dwStreams == 0 || info.dwStreams > 255 || info.dwLength == 0) {
	npMCI->dwTaskError = MCIERR_INVALID_FILE;
	return FALSE;
    }

    //
    // save the interface pointer
    // make a copy of the VTable, for later use
    //
    npMCI->pf = pf;
////npMCI->vt = *pf->lpVtbl;

    npMCI->dwFlags |= MCIAVI_HASINDEX;

    npMCI->dwMicroSecPerFrame = muldiv32(info.dwScale, 1000000, info.dwRate);

    npMCI->lFrames  = (LONG)info.dwLength;
    npMCI->dwRate   = info.dwRate;
    npMCI->dwScale  = info.dwScale;

    npMCI->streams  = (int)info.dwStreams;

    npMCI->dwBytesPerSec = info.dwMaxBytesPerSec;
    npMCI->dwSuggestedBufferSize = info.dwSuggestedBufferSize + 2*sizeof(DWORD);

    SetRect(&npMCI->rcMovie,0,0,(int)info.dwWidth,(int)info.dwHeight);

    npMCI->paStreamInfo = (STREAMINFO*)
	LocalAlloc(LPTR,npMCI->streams * sizeof(STREAMINFO));

    if (npMCI->paStreamInfo == NULL) {
	npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	npMCI->pf = NULL;
	return FALSE;
    }

    for (i = 0; i < npMCI->streams; i++) {

	ps = NULL;
	AVIFileGetStream(pf, &ps, 0, i);

	if (ps == NULL) {
	    npMCI->dwTaskError = MCIERR_INVALID_FILE;
	    npMCI->pf = NULL;
	    return FALSE;
	}

	if (!OpenAVIStream(npMCI, i, ps))
	    DPF(("Error opening stream %d!\n", i));

	if (npMCI->dwTaskError) {
	    npMCI->pf = NULL;
	    ps->lpVtbl->Release(ps);
	    return FALSE;
	}
    }

    //
    // compute the key frames every value
    //
    // do this by finding the key frame average over the first few frames.
    //
    #define NFRAMES 250

    if (psi = npMCI->psiVideo) {
	LONG l;
	int nKeyFrames=0;
	int nFrames = (int) min(npMCI->lFrames, NFRAMES);

	for (l=0; l<nFrames; l++) {
	    if (AVIStreamFindSample(psi->ps, psi->sh.dwStart+l, FIND_PREV|FIND_KEY) == l)
		nKeyFrames++;
	}

	if (nKeyFrames > 1)
	    npMCI->dwKeyFrameInfo = (DWORD)((nFrames + nKeyFrames/2)/nKeyFrames);
	else
	    npMCI->dwKeyFrameInfo = 0;
    }

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | OpenAVIStream | Open an a AVIStream object
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL OpenAVIStream(NPMCIGRAPHIC npMCI, int stream, IAVIStream FAR *ps)
{
    STREAMINFO* psi;
    AVISTREAMINFOW info;
    HRESULT hr;

    _fmemset(&info, 0, sizeof(info));
    hr = ps->lpVtbl->Info(ps, &info, sizeof(info));

    if (FAILED(GetScode(hr))) {
	npMCI->dwTaskError = MCIERR_INVALID_FILE;
	return FALSE;
    }

    DPF(("OpenAVIStream(%d) %4.4hs:%4.4hs %ls\n", stream, (LPSTR)&info.fccType, (LPSTR)&info.fccHandler, info.szName));

    //
    // init the STREAMINFO from the IAVIStream
    //
    psi = SI(stream);

    psi->ps = ps;           // save interface
////psi->vt = *ps->lpVtbl;  // save VTable   !!!needed?

    psi->sh.fccType                 = info.fccType;
    psi->sh.fccHandler              = info.fccHandler;
    psi->sh.dwFlags                 = info.dwFlags;
    psi->sh.wPriority               = info.wPriority;
    psi->sh.wLanguage               = info.wLanguage;
    psi->sh.dwInitialFrames         = 0; // info.dwInitialFrames;
    psi->sh.dwScale                 = info.dwScale;
    psi->sh.dwRate                  = info.dwRate;
    psi->sh.dwStart                 = info.dwStart;
    psi->sh.dwLength                = info.dwLength;
    psi->sh.dwSuggestedBufferSize   = info.dwSuggestedBufferSize;
    psi->sh.dwQuality               = info.dwQuality;
    psi->sh.dwSampleSize            = info.dwSampleSize;
    psi->sh.rcFrame                 = info.rcFrame;

    //
    // get the format of the stream.
    //
    AVIStreamFormatSize(ps, 0, &psi->cbFormat);
    psi->lpFormat = GlobalAllocPtr(GMEM_MOVEABLE, psi->cbFormat);

    if (!psi->lpFormat) {
	npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	return FALSE;
    }

    AVIStreamReadFormat(psi->ps, 0, psi->lpFormat, &psi->cbFormat);

    //
    // get the extra data for the stream.
    //
    AVIStreamReadData(psi->ps,ckidSTREAMHANDLERDATA, NULL, &psi->cbData);

    if (psi->cbData > 0) {
	psi->lpData = GlobalAllocPtr(GMEM_MOVEABLE, psi->cbData);

	if (!psi->lpData) {
	    npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	    return FALSE;
	}

	AVIStreamReadData(psi->ps, ckidSTREAMHANDLERDATA, NULL, &psi->cbData);
    }

    return InitStream(npMCI, psi);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | InitAVIFile | called to RTL to AVIFILE.DLL
 *
 * @rdesc TRUE means OK, otherwise error
 *
 ***************************************************************************/

#ifdef _WIN32
    SZCODE szAVIFILE[] = TEXT("AVIFIL32.DLL");

#ifdef USE_ISVALIDINTERFACE
    SZCODE szCOMPOBJ[] = TEXT("COMPOB32");
#endif
#else
    SZCODE szAVIFILE[] = "AVIFILE.DLL";
#ifdef USE_ISVALIDINTERFACE
    SZCODE szCOMPOBJ[] = "COMPOBJ";
#endif
#endif

// On NT the entry points will NOT be unicode strings, as there is
// no unicode version of GetProcAddress.  BUT SZCODE generate UNICODE...

// on the other hand, the entry points will all be unicode entrypoints 'W'

#ifdef UNICODE
#define UNICODE_APPEND "W"
#else
#define UNICODE_APPEND
#endif

SZCODEA szAVIFileInit[]      = "AVIFileInit";
SZCODEA szAVIFileExit[]      = "AVIFileExit";

#ifdef USE_ISVALIDINTERFACE
SZCODEA szIsValidInterface[] = "IsValidInterface";
#endif

SZCODEA szAVIMakeFileFromStreams[] = "AVIMakeFileFromStreams";
SZCODEA szAVIStreamBeginStreaming[] = "AVIStreamBeginStreaming";
SZCODEA szAVIStreamEndStreaming[] = "AVIStreamEndStreaming";
SZCODEA szAVIFileOpen[]      = "AVIFileOpen" UNICODE_APPEND;

BOOL FAR InitAVIFile(NPMCIGRAPHIC npMCI)
{
    UINT u;

    if (hdllAVIFILE == (HMODULE)-1)
	// We have already tried to load AVIFILE and failed
	return FALSE;

    if (hdllAVIFILE == NULL) {

	u = SetErrorMode(SEM_NOOPENFILEERRORBOX);
#ifdef _WIN32
	// The load and free of szAVIFILE is safe as it is protected by
	// the npMCI device critical section.  This means that open/close
	// events will be synchronised.
#endif
	hdllAVIFILE = LoadLibrary(szAVIFILE);
	SetErrorMode(u);

#ifndef _WIN32
	if ((UINT)hdllAVIFILE <= (UINT)HINSTANCE_ERROR)
	    hdllAVIFILE = NULL;
#endif

	if (hdllAVIFILE == NULL) {
	    hdllAVIFILE = (HMODULE)-1;
	    DPF(("Failed to load AVIFILE dll - error %d\n", GetLastError()));
	    return FALSE;
	}

#ifdef USE_ISVALIDINTERFACE
	hdllCOMPOBJ = GetModuleHandle(szCOMPOBJ);
	Assert(hdllCOMPOBJ != NULL);

	(FARPROC)XIsValidInterface = GetProcAddress(hdllCOMPOBJ, szIsValidInterface);

	Assert(XIsValidInterface != NULL);
#endif

	(FARPROC)XAVIFileInit = GetProcAddress(hdllAVIFILE, szAVIFileInit);
	(FARPROC)XAVIFileExit = GetProcAddress(hdllAVIFILE, szAVIFileExit);
	(FARPROC)XAVIFileOpen = GetProcAddress(hdllAVIFILE, szAVIFileOpen);
	(FARPROC)XAVIMakeFileFromStreams = GetProcAddress(hdllAVIFILE, szAVIMakeFileFromStreams);
	(FARPROC)XAVIStreamBeginStreaming = GetProcAddress(hdllAVIFILE, szAVIStreamBeginStreaming);
	(FARPROC)XAVIStreamEndStreaming = GetProcAddress(hdllAVIFILE, szAVIStreamEndStreaming);

	Assert(XAVIFileInit != NULL);
	Assert(XAVIFileExit != NULL);
	Assert(XAVIFileOpen != NULL);
	Assert(XAVIMakeFileFromStreams != NULL);
    }

    //
    // we need to call AVIFileInit() and AVIFileExit() for each task that
    // is using AVIFile or things will not work right.
    //
    if (!(npMCI->dwFlags & MCIAVI_USING_AVIFILE)) {

	npMCI->dwFlags |= MCIAVI_USING_AVIFILE;

	AVIFileInit();     // Should we check the return code?
	uAVIFILE++;
    }

    return TRUE;
}

// must be called from app thread too!

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | FreeAVIFile | called to un-RTL to AVIFILE.DLL
 *
 * @rdesc TRUE means OK, otherwise error
 *
 ***************************************************************************/

BOOL FAR FreeAVIFile(NPMCIGRAPHIC npMCI)
{
    if (!(npMCI->dwFlags & MCIAVI_USING_AVIFILE))
	return FALSE;

    Assert(hdllAVIFILE != (HMODULE)-1 && hdllAVIFILE != NULL);

    // free this tasks use of AVIFile
    AVIFileExit();

    // if no more people using AVIFile let the DLLs go.
    Assert(uAVIFILE > 0);
    uAVIFILE--;

    if (uAVIFILE == 0) {
	FreeLibrary(hdllAVIFILE);
	hdllAVIFILE = NULL;
#ifdef USE_ISVALIDINTERFACE
	hdllCOMPOBJ = NULL;
#endif
    }

    return TRUE;
}

#endif /* USEAVIFILE */

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | OpenRiffAVIFile | Open an RIFF AVI file
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm BOOL | bForce | if FALSE, return error for files that
 * we think should be opened by AVIFILE instead.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL OpenRiffAVIFile(NPMCIGRAPHIC npMCI, BOOL bForce)
{
    HMMIO               hmmio;
    HANDLE              h = NULL;
    BOOL                fRet = TRUE;
    MMIOINFO            mmioInfo;
    MMCKINFO            ckRIFF;
    MMCKINFO            ckLIST;
    MMCKINFO            ckRECORD;

    _fmemset(&mmioInfo, 0, sizeof(MMIOINFO));
    mmioInfo.htask = (HANDLE)npMCI->hCallingTask;

    hmmio = mmioOpen(npMCI->szFilename, &mmioInfo, MMIO_READ | MMIO_DENYWRITE);

    if (hmmio == NULL)
	hmmio = mmioOpen(npMCI->szFilename, &mmioInfo, MMIO_READ);

    if (!hmmio) {
	switch (mmioInfo.wErrorRet) {
	    case MMIOERR_OUTOFMEMORY:
		npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
		break;
	    case MMIOERR_FILENOTFOUND:
	    case MMIOERR_CANNOTOPEN:
	    default:
		npMCI->dwTaskError = MCIERR_FILE_NOT_FOUND;
		break;
	}
	fRet = FALSE;
	goto exit;
    }

    npMCI->hmmio = hmmio;

    /*
    ** Descend into RIFF file
    */
    if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0) {
	goto ERROR_BADFILE;
    }

    /*
     * check for a 'QuickTime AVI' file, a QuickTime AVI file is a
     * QuickTime public movie with a AVI file in the 'mdat' atom.
     */
    if (ckRIFF.cksize == mmioFOURCC('m','d','a','t'))
    {
	DPF(("File is a QuickTime public movie\n"));

	/*
	 * now the 'mdat' atom better be a RIFF/AVI or we cant handle it.
	 */
	if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0) {
	    goto ERROR_BADFILE;
	}
    }

    /* Make sure it's a RIFF file */
    if (ckRIFF.ckid != FOURCC_RIFF) {
	npMCI->dwTaskError = MCIERR_INVALID_FILE;
	goto ERROR_NOT_AVIFILE;
    }

    /* Otherwise, it should be an AVI file */
    if (ckRIFF.fccType != formtypeAVI) {
	npMCI->dwTaskError = MCIERR_INVALID_FILE;
	goto ERROR_NOT_AVIFILE;
    }

    /*
    ** Descend into header LIST
    */
    ckLIST.fccType = listtypeAVIHEADER;
    if (mmioDescend(hmmio, &ckLIST, &ckRIFF, MMIO_FINDLIST) != 0) {
	goto ERROR_BADFILE;
    }

    /* Leave space at end of buffer for pad word */
    h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, ckLIST.cksize -
				    sizeof(DWORD) +
				    sizeof(DWORD));

    if (!h) {
	npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	return FALSE;
    }

    npMCI->lp = npMCI->lpBuffer = (LPSTR) GlobalLock(h);

    DPF(("Reading header list: %lu bytes.\n", ckLIST.cksize - sizeof(DWORD)));

    if (mmioRead(hmmio, npMCI->lp, ckLIST.cksize - sizeof(DWORD))
			    != (LONG) (ckLIST.cksize - sizeof(DWORD))) {
	npMCI->dwTaskError = MCIERR_FILE_READ;
	goto ERROR_RETURN;
    }

#ifdef USE_AVIFILE_FOR_NON_INT
    //
    //  we check here for AVI RIFF files we dont want to handle with our
    //  built in code, and want to pass on to AVIFILE.DLL
    //
    //  we handle the following files:
    //
    //      interleaved
    //
    //  we pass on the following files to AVIFILE.DLL
    //
    //      non-interleaved
    //
    //  pretty simple right now, just interleaved non-interlaved
    //  but could get as complex as you want.
    //
    // if there is a problem with avifile (eg dll not present) we will
    // be called with bForce TRUE: this means we should open it if at
    // all possible
    //
    if (!bForce) {
	MainAVIHeader FAR * lpHdr;

	lpHdr = (MainAVIHeader FAR *)((BYTE FAR *)npMCI->lp + 8);

	if (!(lpHdr->dwFlags & AVIF_ISINTERLEAVED) ||
	    lpHdr->dwInitialFrames == 0) {

	    DOUT("File is not interleaved, giving it to AVIFILE.DLL\n");
	    goto ERROR_NOT_AVIFILE;
	}

	//
	// ok now we have a 1:1 interleved file.
	//
	// always use our code on a CD-ROM, but on other media...
	//
	switch (npMCI->uDriveType) {
	    case DRIVE_CDROM:
		break;

	    case DRIVE_REMOTE:
	    case DRIVE_FIXED:
	    case DRIVE_REMOVABLE:
		break;

	    default:
		break;
	}
    }
#endif

    if (PEEK_DWORD() == ckidAVIMAINHDR) {
	if (!ParseNewHeader(npMCI))
	    goto ERROR_RETURN;
    } else {
	goto ERROR_BADFILE;
    }

    /* Ascend out of header LIST */
    if (mmioAscend(hmmio, &ckLIST, 0) != 0) {
	npMCI->dwTaskError = MCIERR_FILE_READ;
	goto ERROR_RETURN;
    }

    /* Initially, no frame has been drawn */
    npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;

    /*
    ** Descend into big 'Movie LIST'
    */
    ckLIST.fccType = listtypeAVIMOVIE;
    if (mmioDescend(hmmio, &ckLIST, &ckRIFF, MMIO_FINDLIST) != 0) {
	goto ERROR_BADFILE;
    }

    npMCI->dwMovieListOffset = ckLIST.dwDataOffset;

    /* Calculate end of 'movi' list, in case we need to read the index */
    npMCI->dwBigListEnd = ckLIST.dwDataOffset + ckLIST.cksize +
				(ckLIST.cksize & 1);

    /*
    ** Descend into header of first chunk
    */
    if (mmioDescend(hmmio, &ckRECORD, &ckLIST, 0) != 0) {
	goto ERROR_BADFILE;
    }
    npMCI->dwFirstRecordType = ckRECORD.ckid;
    npMCI->dwFirstRecordSize = ckRECORD.cksize + 2 * sizeof(DWORD);
    npMCI->dwFirstRecordPosition = mmioSeek(hmmio, 0, SEEK_CUR);

    if (mmioAscend(hmmio, &ckRECORD, 0) != 0) {
	npMCI->dwTaskError = MCIERR_FILE_READ;
	goto ERROR_RETURN;
    }

#ifdef DEBUG
    DPF2(("First record (%4.4s) 0x%lx bytes at position 0x%lx.\n",
		(LPSTR)&npMCI->dwFirstRecordType,
		npMCI->dwFirstRecordSize,
		npMCI->dwFirstRecordPosition));

    if (npMCI->dwFirstRecordPosition & 0x7ff) {
	DPF(("!!\n"));
	DPF(("!!  This file is not properly aligned to a 2K boundary.\n"));
	DPF(("!!  It may not play well from CD-ROM.\n"));
	DPF(("!!\n"));
    }
#endif

exit:
    if (!fRet)
	mciaviCloseFile(npMCI);

    if (h) {
	npMCI->lpBuffer = NULL;
	npMCI->dwBufferSize = 0L;
	GlobalUnlock(h);
	GlobalFree(h);
    }

    return fRet;

ERROR_NOT_AVIFILE:
#ifdef USEAVIFILE
    npMCI->dwTaskError = AVIERR_NOT_AVIFILE;  // mark as not a AVI file
    goto ERROR_RETURN;                        // we will be called a second time
#endif

ERROR_BADFILE:
    npMCI->dwTaskError = MCIERR_INVALID_FILE; // something wrong with the file

ERROR_RETURN:
    fRet = FALSE;
    goto exit;
}

/***************************************************************************
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | ParseNewHeader | 'nuf said
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL ParseNewHeader(NPMCIGRAPHIC npMCI)
{
    DWORD               dwHeaderSize;
    MainAVIHeader FAR * lpHdr;
    int                 stream;

    if (GET_DWORD() != ckidAVIMAINHDR) {
	goto FileError;
    }

    dwHeaderSize = GET_DWORD(); /* Skip size */

    /* Now, we're pointing at the actual header */
    lpHdr = (MainAVIHeader FAR *) npMCI->lp;

    npMCI->lFrames = (LONG)lpHdr->dwTotalFrames;
    npMCI->dwMicroSecPerFrame = lpHdr->dwMicroSecPerFrame;
    npMCI->dwRate = 1000000;
    npMCI->dwScale = npMCI->dwMicroSecPerFrame;

    /* Reject some bad values */
    if (!lpHdr->dwStreams || lpHdr->dwStreams > 255 || !npMCI->lFrames) {
	goto FileError;
    }

    npMCI->streams = (int) lpHdr->dwStreams;
    npMCI->dwBytesPerSec = lpHdr->dwMaxBytesPerSec;
    npMCI->wEarlyRecords = (UINT) lpHdr->dwInitialFrames;
    npMCI->dwSuggestedBufferSize = lpHdr->dwSuggestedBufferSize;

    SetRect(&npMCI->rcMovie,0,0,(int)lpHdr->dwWidth,(int)lpHdr->dwHeight);

    npMCI->dwFlags |= MCIAVI_HASINDEX;

    if (!(lpHdr->dwFlags & AVIF_ISINTERLEAVED)) {
	DPF(("File is not interleaved.\n"));
	npMCI->dwFlags |= MCIAVI_NOTINTERLEAVED;
    }

    SKIP_BYTES(dwHeaderSize);   /* Skip rest of chunk */

    npMCI->paStreamInfo = (STREAMINFO NEAR *)
		    LocalAlloc(LPTR, npMCI->streams * sizeof(STREAMINFO));
    // !!! error check

    for (stream = 0; stream < npMCI->streams; stream++) {
	AVIStreamHeader FAR *   lpStream;
	HPSTR                   hpNextChunk;
	STREAMINFO *            psi = &npMCI->paStreamInfo[stream];

	if (GET_DWORD() != FOURCC_LIST) {
	    goto FileError;
	}

	dwHeaderSize = GET_DWORD(); /* Skip size */

	hpNextChunk = npMCI->lp + (dwHeaderSize + (dwHeaderSize & 1));

	if (GET_DWORD() != listtypeSTREAMHEADER) {
	    goto FileError;
	}

	/* Now, we're at the begging of the stream's header chunks. */

	if (GET_DWORD() != ckidSTREAMHEADER) {
	    goto FileError;
	}

	dwHeaderSize = GET_DWORD(); /* Skip size */

	/* Now, we're pointing at the stream header */
	lpStream = (AVIStreamHeader FAR *) npMCI->lp;
	hmemcpy(&psi->sh, lpStream, min(dwHeaderSize, sizeof(psi->sh)));

	//
	// reject files with more than one video stream.
	//
	if (psi->sh.fccType == streamtypeVIDEO &&
	    npMCI->nVideoStreams >= 1) {
	    DPF(("File has multiple video streams, giving it to AVIFILE.DLL\n"));
	    goto DontHandleThisFile;
	}

	SKIP_BYTES(dwHeaderSize);

	/* Read the  format */
	if (GET_DWORD() != ckidSTREAMFORMAT) {
	    goto FileError;
	}

	dwHeaderSize = GET_DWORD(); /* Skip size */

	if (dwHeaderSize > 16384L) {
	    goto FileError;
	}

	psi->cbFormat = dwHeaderSize;
	psi->lpFormat = GlobalAllocPtr(GHND,dwHeaderSize);
	if (!psi->lpFormat) {
	    npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	    return FALSE;
	}

	hmemcpy(psi->lpFormat, npMCI->lp, dwHeaderSize);

	SKIP_BYTES(dwHeaderSize);

	if (PEEK_DWORD() == ckidSTREAMHANDLERDATA) {
	    GET_DWORD();
	    dwHeaderSize = GET_DWORD(); /* Skip size */

	    psi->cbData = dwHeaderSize;
	    psi->lpData = GlobalAllocPtr(GHND,dwHeaderSize);

	    if (!psi->lpData) {
		npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
		return FALSE;
	    }

	    hmemcpy(psi->lpData, npMCI->lp, dwHeaderSize);

	    /* Skip to end of Data chunk */
	    SKIP_BYTES(dwHeaderSize);
	} else {
	    psi->cbData = 0;
	    psi->lpData = NULL;
	}

	InitStream(npMCI, psi);

	npMCI->lp = hpNextChunk;
    }

    if (npMCI->dwTaskError) {
	return FALSE;
    }
    return TRUE;

FileError:
    npMCI->dwTaskError = MCIERR_INVALID_FILE;
    return FALSE;

DontHandleThisFile:
    npMCI->dwTaskError = AVIERR_NOT_AVIFILE;
    return FALSE;
}

//
// --- called on worker thread ---------------------------------------------
//

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | OpenFileInit | called after a file is opened to init things
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

BOOL NEAR PASCAL OpenFileInit(NPMCIGRAPHIC npMCI)
{
    int i;
    RECT rc;

    //
    //  lets make sure we opened something.
    //
    if (npMCI->streams == 0)
	return FALSE;

    if (npMCI->nVideoStreams + npMCI->nAudioStreams + npMCI->nOtherStreams == 0)
	return FALSE;

    if (npMCI->nVideoStreams == 0)
	npMCI->dwFlags &= ~MCIAVI_SHOWVIDEO;

    if (npMCI->nAudioStreams == 0)
	npMCI->dwFlags &= ~MCIAVI_PLAYAUDIO; // No audio streams means silence

    if (IsNTWOW()) {
	if (!npMCI->pbiFormat) {
	// Although there are valid video streams we do not appear to have
	// access to a decompressor that knows what to do with them.  Hence
	// if we are in NT/WOW we fail the open, the call will return to
	// 16 bit land and there might be a codec there that can cope
	    return(FALSE);
	}
    }


    if (npMCI->nAudioStreams > 1) {
	UINT        wLang;
	int         stream;

#ifndef _WIN32
	UINT (WINAPI * GetUserDefaultLangID)(void);
	UINT        u;
	HANDLE      hdll;

	u = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	hdll = LoadLibrary(szOLENLSDLL);
	SetErrorMode(u);

	wLang = 0;
	if ((UINT)hdll > (UINT)HINSTANCE_ERROR)
	{
	    if ((FARPROC) GetUserDefaultLangID = GetProcAddress(hdll, szOLENLSAPI)) {
		wLang = GetUserDefaultLangID();
	    }
	    FreeLibrary(hdll);
	}
#else
	wLang = GetUserDefaultLangID();
#endif
	DPF(("Current language: %x\n", wLang));

	if (wLang > 0) {
	    for (stream = 0; stream < npMCI->streams; stream++) {
		if (SH(stream).fccType == streamtypeAUDIO) {

		    if (!(SH(stream).dwFlags & STREAM_ENABLED))
			continue;

		    if (SH(stream).wLanguage == wLang) {
			npMCI->nAudioStream = stream;
			npMCI->psiAudio = SI(stream);
			break;
		    }
		}
	    }
	}
    }

    if (npMCI->dwFlags & MCIAVI_NOTINTERLEAVED) {
	npMCI->wEarlyRecords = npMCI->wEarlyVideo;
    }
    else {
	npMCI->wEarlyRecords = max(npMCI->wEarlyVideo, npMCI->wEarlyAudio);
    }

    if (npMCI->wEarlyRecords == 0 &&
	    !(npMCI->dwFlags & MCIAVI_NOTINTERLEAVED)) {
	DPF(("Interleaved file with no audio skew?\n"));
	npMCI->dwFlags |= MCIAVI_NOTINTERLEAVED;
    }

    if (npMCI->dwFlags & MCIAVI_ANIMATEPALETTE) {
	DPF(("This AVI file has palette changes.\n"));

	if (npMCI->nVideoStreams > 1) {
	    npMCI->dwFlags &= ~MCIAVI_ANIMATEPALETTE;
	    DPF(("...But we are going to ignore them?\n"));
	}
    }

    //
    // this must be set
    //
    if (npMCI->dwSuggestedBufferSize == 0) {
	for (i=0; i<npMCI->streams; i++)
	    npMCI->dwSuggestedBufferSize =
		max(SH(i).dwSuggestedBufferSize,npMCI->dwSuggestedBufferSize);
    }

    //
    // check all fields in the main header
    //
    if (npMCI->dwScale == 0 ||
	npMCI->dwRate == 0) {
    }

////will be set when header parsed
////npMCI->dwMicroSecPerFrame      = muldiv32(npMCI->dwScale, 1000000, npMCI->dwRate);
    npMCI->dwPlayMicroSecPerFrame  = npMCI->dwMicroSecPerFrame;

#define COMMON_SCALE    10000
    //
    // convert the rate/scale into something that is normalized to 1000
    //
    npMCI->dwRate = muldiv32(npMCI->dwRate, COMMON_SCALE, npMCI->dwScale);
    npMCI->dwScale = COMMON_SCALE;

    //
    // walk all streams and fix them up.
    //
    for (i=0; i<npMCI->streams; i++) {
	STREAMINFO *psi = SI(i);
	LONG lStart;
	LONG lEnd;

	//
	// convert the rate/scale into something that is normalized to 1000
	//
	psi->sh.dwRate = muldiv32(psi->sh.dwRate, COMMON_SCALE, psi->sh.dwScale);
	psi->sh.dwScale = COMMON_SCALE;

	//
	// trim any streams that hang over the movie.
	//
	lStart = MovieToStream(psi, 0);
	lEnd   = MovieToStream(psi, npMCI->lFrames);

	if ((LONG)(psi->sh.dwStart + psi->sh.dwLength) > lEnd) {

	    DPF(("Stream #%d is too long, was %ld now %ld\n", i,
		psi->sh.dwLength, lEnd - psi->sh.dwStart));

	    psi->sh.dwLength = lEnd - psi->sh.dwStart;
	}
    }

    //
    // If this is WOW we had better check that there is an audio codec
    // available.  If not, we let the 16 bit side open the video.
    //
    if (IsNTWOW()) {
	if (SetUpAudio(npMCI, FALSE)) return FALSE;
    }

    //
    // fix up the movie rect
    //
    if (IsRectEmpty(&npMCI->rcMovie)) {
	DPF2(("Movie rect is empty\n"));

	SetRectEmpty(&rc);

	for (i=0; i<npMCI->streams; i++) {
	    UnionRect(&rc,&rc,&SH(i).rcFrame);
	}

	npMCI->rcMovie = rc;
	DPF2(("Movie rect was empty, now [%d, %d, %d, %d]\n", rc));
    } else {
	rc = npMCI->rcMovie;
    }


    // check we can access the interface ptr - the main reason for this
    // is that if we have pasted this interface ptr, we cannot access it on
    // the worker thread (which is where we are now). Clean up the error handling
    // by actually detecting this case.

    if (npMCI->pf) {
	IAVIStream * ps;
	long l;
	if ((l = AVIFileGetStream(npMCI->pf, &ps, 0, 0)) != 0) {
	    DPF(("avifile err %d", l));
	    npMCI->dwTaskError = l;
	    return FALSE;
	} else {
	    if (ps->lpVtbl->FindSample(ps, 0, FIND_FROM_START|FIND_NEXT|FIND_ANY) < 0) {
		npMCI->dwTaskError = MCIERR_DEVICE_NOT_READY;
		ps->lpVtbl->Release(ps);
		return FALSE;
	    }
	    ps->lpVtbl->Release(ps);
	}
    }


    //
    // always read the index, so we can skip frames even on CD!
    //
    ReadIndex(npMCI);

    DPF(("Key frames are every (on average): %ld frames (%ld ms)\n",npMCI->dwKeyFrameInfo, MovieToTime(npMCI->dwKeyFrameInfo)));

    // force things to happen, in case we're re-loading
    SetRectEmpty(&npMCI->rcSource);
    SetRectEmpty(&npMCI->rcDest);

    /* this will call DrawDibBegin() ... */
    if (TryPutRect(npMCI, MCI_DGV_PUT_SOURCE, &rc, &npMCI->dwTaskError)) {
	return FALSE;
    }

    /*
     * also set the dest rect. This should be done
     * by the WM_SIZE message sent during SetWindowToDefaultSize.
     * On NT, the WM_SIZE message is not sent synchronously since it
     * is an inter-thread sendmessage (the winproc is on the original thread
     * whereas we are currently running on the avi thread). The winproc
     * thread may well not get the WM_SIZE message until much too late, so
     * set the dest rect here. Note: don't use ResetDestRect since that
     * also relies on the window size, which is not set yet.
     */

    /* double frame size of destination if zoom by 2 */

    if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2)
	SetRect(&rc, 0, 0, rc.right*2, rc.bottom*2);

    if (TryPutRect(npMCI, MCI_DGV_PUT_DESTINATION, &rc, &npMCI->dwTaskError)) {
	return FALSE;
    }
    if (npMCI->dwTaskError) {
	return FALSE;
    }

    //
    // size the window and things.
    //
    SetWindowToDefaultSize(npMCI, FALSE);

    DrawBegin(npMCI, NULL);
    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | mciaviCloseFile | Close an AVI file.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc TRUE means OK, otherwise mci error in dwTaskError
 *
 ***************************************************************************/

BOOL FAR PASCAL mciaviCloseFile (NPMCIGRAPHIC npMCI)
{
    if (!npMCI)
	return FALSE;

    if (npMCI->lpMMIOBuffer) {
	GlobalFreePtr(npMCI->lpMMIOBuffer);
	npMCI->lpMMIOBuffer = NULL;
    }

    npMCI->hicDraw = NULL;

    if (npMCI->hicDrawDefault) {
	if (npMCI->hicDrawDefault != (HIC) -1)
	    ICClose(npMCI->hicDrawDefault);
	npMCI->hicDrawDefault = NULL;
    }

    if (npMCI->hicDrawFull) {
	if (npMCI->hicDrawFull != (HIC) -1)
	    ICClose(npMCI->hicDrawFull);
	npMCI->hicDrawFull = NULL;
    }

    if (npMCI->hicDecompress) {
	// !!! What if we never began it?
	ICDecompressEnd(npMCI->hicDecompress);

	ICClose(npMCI->hicDecompress);
	npMCI->hicDecompress = NULL;
    }

    if (npMCI->hicInternal) {
	ICClose(npMCI->hicInternal);
	npMCI->hicInternal = NULL;
    }

    if (npMCI->hicInternalFull) {
	ICClose(npMCI->hicInternalFull);
	npMCI->hicInternalFull = NULL;
    }

    if (npMCI->hmmio) {
	mmioClose(npMCI->hmmio, 0);
	npMCI->hmmio = NULL;
    }

    if (npMCI->hmmioAudio) {
	mmioClose(npMCI->hmmioAudio, 0);
	npMCI->hmmioAudio = NULL;
    }

    if (npMCI->pWF) {
	LocalFree((HANDLE) npMCI->pWF);
	npMCI->pWF = NULL;
    }

    if (npMCI->pbiFormat) {
	GlobalFreePtr(npMCI->pbiFormat);
	npMCI->pbiFormat = NULL;
    }

//  if (npMCI->hpal) {
//      DeleteObject(npMCI->hpal);
//      npMCI->hpal = NULL;
//  }

    if (npMCI->hpDecompress) {
	GlobalFreePtr(npMCI->hpDecompress);
	npMCI->hpDecompress = NULL;
    }

    if (npMCI->hpIndex) {
	GlobalFreePtr(npMCI->hpIndex);
	npMCI->hpIndex = NULL;
    }

    if (npMCI->hpFrameIndex) {
	GlobalUnlock(npMCI->hgFrameIndex);
	GlobalFree(npMCI->hgFrameIndex);

	npMCI->hpFrameIndex = NULL;
	npMCI->hgFrameIndex = 0;
    }

    if (npMCI->pVolumeTable) {
	LocalFree((HLOCAL)npMCI->pVolumeTable);
	npMCI->pVolumeTable = NULL;
    }

#ifdef USEAVIFILE
    if (npMCI->pf) {
	AVIFileClose(npMCI->pf);
	npMCI->pf = NULL;
    }
#endif

    if (npMCI->paStreamInfo) {
	int i;

	for (i = 0; i < npMCI->streams; i++) {
	    CloseStream(npMCI, &npMCI->paStreamInfo[i]);
	}

	LocalFree((HLOCAL)npMCI->paStreamInfo);
	npMCI->paStreamInfo = NULL;
    }

    npMCI->streams = 0;
    npMCI->nAudioStreams = 0;
    npMCI->nVideoStreams = 0;
    npMCI->nErrorStreams = 0;
    npMCI->nOtherStreams = 0;

    npMCI->wEarlyVideo = 0;
    npMCI->wEarlyAudio = 0;
    npMCI->wEarlyRecords = 0;

    //!!! I bet we need to clear more
    npMCI->dwFlags &= ~(MCIAVI_NOTINTERLEAVED |
			MCIAVI_ANIMATEPALETTE |
			MCIAVI_CANDRAW |
			MCIAVI_HASINDEX);

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | CloseStream | Close an StreamAVI file.
 *
 ***************************************************************************/

STATICFN void NEAR PASCAL CloseStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi)
{
    psi->dwFlags &= ~STREAM_ENABLED;
////psi->sh.fccType = 0;
////psi->sh.fccHandler = 0;

    if (psi->lpFormat) {
	GlobalFreePtr(psi->lpFormat);
	psi->lpFormat = NULL;
    }

    if (psi->lpData) {
	GlobalFreePtr(psi->lpData);
	psi->lpData = NULL;
    }

    if (psi->hicDraw) {
	ICClose(psi->hicDraw);
	psi->hicDraw = NULL;
    }

#ifdef USEAVIFILE
    if (psi->ps) {
	AVIStreamClose(psi->ps);
	psi->ps = NULL;
    }
#endif
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | InitStream | initialize a stream
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL InitStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi)
{
    BOOL f;

    //
    // set flags
    //
    if (psi->sh.dwFlags & AVISF_VIDEO_PALCHANGES)
	psi->dwFlags |= STREAM_PALCHANGES;

    psi->lStart = (LONG)psi->sh.dwStart;
    psi->lEnd   = (LONG)psi->sh.dwStart + psi->sh.dwLength;

    if (psi->sh.fccType == streamtypeVIDEO &&
	!(npMCI->dwFlags & MCIAVI_NOTINTERLEAVED))
	psi->lStart -= (LONG)psi->sh.dwInitialFrames;

    switch(psi->sh.fccType) {
	case streamtypeVIDEO:
	    f = InitVideoStream(npMCI, psi);
	    break;

	case streamtypeAUDIO:
	    f = InitAudioStream(npMCI, psi);
	    break;

	default:
	    f = InitOtherStream(npMCI, psi);
	    break;
	    }

    if (!f)  {
	psi->dwFlags |= STREAM_ERROR;
	npMCI->nErrorStreams++;
	CloseStream(npMCI, psi);
    }

    //
    // disable the stream if the file header says to
    //
    if (psi->sh.dwFlags & AVISF_DISABLED) {
	psi->dwFlags &= ~STREAM_ENABLED;
    }

    return f;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | InitVideoStream | initialize a video stream
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL InitVideoStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi)
{
    LPBITMAPINFOHEADER lpbi;
    int stream = (int) (INT_PTR) (psi - npMCI->paStreamInfo);

    npMCI->wEarlyVideo = (UINT)psi->sh.dwInitialFrames;

    if (psi->sh.dwFlags & AVISF_VIDEO_PALCHANGES) {
	//!!! is this right.
	npMCI->dwFlags |= MCIAVI_ANIMATEPALETTE;
    }

    if (IsRectBogus(&psi->sh.rcFrame)) {
	DPF(("BOGUS Stream rectangle [%d %d %d %d]\n", psi->sh.rcFrame));
	SetRectEmpty(&psi->sh.rcFrame);
    }

    // In case the rectangle is totally wrong, chop it down to size....
    // !!! What if the user _wants_ a zero-size RECT?
    IntersectRect(&psi->sh.rcFrame, &psi->sh.rcFrame, &npMCI->rcMovie);
    if (IsRectEmpty(&psi->sh.rcFrame)) {
	DPF2(("Video stream rect is empty, correcting\n"));
	SetRect(&psi->sh.rcFrame, 0, 0,
	    (int)((LPBITMAPINFOHEADER)psi->lpFormat)->biWidth,
	    (int)((LPBITMAPINFOHEADER)psi->lpFormat)->biHeight);
    }

    //
    // make sure the biCompression is right for RLE files.
    //
    lpbi = (LPBITMAPINFOHEADER)psi->lpFormat;

    if (psi->sh.fccHandler == 0) {

	if (lpbi->biCompression == 0)
	    psi->sh.fccHandler = comptypeDIB;

	if (lpbi->biCompression == BI_RLE8 && lpbi->biBitCount == 8)
	    psi->sh.fccHandler = comptypeRLE;

	if (lpbi->biCompression > 256)
	    psi->sh.fccHandler = lpbi->biCompression;
    }

    if (lpbi->biCompression <= BI_RLE8 && lpbi->biBitCount == 8) {

	if (psi->sh.fccHandler == comptypeRLE0 ||
	    psi->sh.fccHandler == comptypeRLE)
	    lpbi->biCompression = BI_RLE8;

// Assuming a DIB handler has RGB data will blow up files that have RLE data.
// Unfortunately, VidEdit writes out stupid files like this.
//        if (psi->sh.fccHandler == comptypeDIB)
//            lpbi->biCompression = BI_RGB;
    }

    //
    // make sure the color table is set to the right size
    //
    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
	lpbi->biClrUsed = (1 << (int)lpbi->biBitCount);

    //
    // try to open draw handler
    //
    if (psi->sh.fccHandler) {
	psi->hicDraw = ICDrawOpen(psi->sh.fccType,psi->sh.fccHandler,psi->lpFormat);

	if (psi->hicDraw)
	    DPF(("Opened draw handler %4.4hs:%4.4hs (Stream %d)\n", (LPSTR)&psi->sh.fccType,(LPSTR)&psi->sh.fccHandler, stream));
	else
	    DPF(("Failed to open draw handler for %4.4hs:%4.4hs (Stream %d)\n", (LPSTR)&psi->sh.fccType,(LPSTR)&psi->sh.fccHandler, stream));
    }

    //
    // one video stream is the master, he controls the palette etc
    // for lack of a better default the first valid (i.e. one we can
    // handle) video stream will
    // become the master.
    //
    if (npMCI->pbiFormat == NULL) {

	npMCI->nVideoStream = stream;
	npMCI->psiVideo = psi;

	npMCI->pbiFormat = (LPBITMAPINFOHEADER)
		    GlobalAllocPtr(GMEM_MOVEABLE, psi->cbFormat);

	if (!npMCI->pbiFormat) {
	    npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	    return FALSE;
	}

	//
	// copy the entire format over
	//
	hmemcpy(npMCI->pbiFormat,psi->lpFormat,psi->cbFormat);

	npMCI->bih = *npMCI->pbiFormat;
	npMCI->bih.biSize = sizeof(BITMAPINFOHEADER);
	npMCI->bih.biCompression = BI_RGB;

	if (npMCI->bih.biClrUsed) {
	    /* save the original colors. */
	    hmemcpy(npMCI->argb, (LPBYTE)npMCI->pbiFormat + npMCI->pbiFormat->biSize,
			    (int)npMCI->bih.biClrUsed * sizeof(RGBQUAD));
	    hmemcpy(npMCI->argbOriginal, (LPSTR) npMCI->pbiFormat + npMCI->pbiFormat->biSize,
			    (int)npMCI->bih.biClrUsed * sizeof(RGBQUAD));
	}

	//
	// now open the decompressor, try fastdecompress if it will do it.
	//
	npMCI->hicDecompress = ICLocate(ICTYPE_VIDEO,psi->sh.fccHandler,
		    psi->lpFormat,NULL,ICMODE_FASTDECOMPRESS);

	// fast decompress may not be supported
	if (npMCI->hicDecompress == NULL) {
	    npMCI->hicDecompress = ICDecompressOpen(ICTYPE_VIDEO,
			psi->sh.fccHandler,psi->lpFormat,NULL);
	}

	//
	// set any state data.
	//
	if (npMCI->hicDecompress && psi->cbData) {
	    ICSetState(npMCI->hicDecompress, psi->lpData, psi->cbData);
	}

	if (psi->hicDraw == NULL && npMCI->hicDecompress == NULL &&
	    psi->sh.fccHandler != comptypeRLE0 &&
	    psi->sh.fccHandler != comptypeNONE &&
	    psi->sh.fccHandler != comptypeDIB &&
	    psi->sh.fccHandler != comptypeRLE &&
	    psi->sh.fccHandler != 0) {

	    DPF(("Unable to open compressor '%4.4hs'!!!\n", (LPSTR) &psi->sh.fccHandler));

	    npMCI->nVideoStream = -1;
	    npMCI->psiVideo = NULL;

	    GlobalFreePtr(npMCI->pbiFormat);
	    npMCI->pbiFormat = NULL;

	    //
	    // we would like to return a custom error but by the time the
	    // user (application) comes to retrieve the error text MCI will
	    // have unloaded us (because the open failed) and therefore
	    // will not be able to get our specific error text.
	    // Hence we return a bogus generic error.
	    //
#ifdef _WIN32
	    SetLastError(MCIERR_AVI_NOCOMPRESSOR);
#endif
	    if (npMCI->streams == 1)    // this is the only stream
		npMCI->dwTaskError = MMSYSERR_NODRIVER; // MCIERR_AVI_NOCOMPRESSOR;

	    return FALSE;   // cant load this video stream
	}
    }
    else {
	//
	// this is not the default video stream find a draw handler that
	// can deal with the stream.
	//

	//
	// try VIDS.DRAW
	//
	// if that fails open a draw handler not-specific to the format
	//
	if (psi->hicDraw == NULL) {

	    psi->hicDraw = ICOpen(psi->sh.fccType,FOURCC_AVIDraw,ICMODE_DRAW);

	    if (psi->hicDraw)
		DOUT("Opened draw handler VIDS.DRAW\n");

	    if (psi->hicDraw && ICDrawQuery(psi->hicDraw,psi->lpFormat) != ICERR_OK) {
		DOUT("Closing VIDS.DRAW because it cant handle this format");
		ICClose(psi->hicDraw);
		psi->hicDraw = NULL;
	    }
	}

	//
	// if that fails open our internal handler.
	//
	if (psi->hicDraw == NULL) {

	    psi->hicDraw = ICOpenFunction(psi->sh.fccType,
		FOURCC_AVIDraw,ICMODE_DRAW,(FARPROC)ICAVIDrawProc);

	    if (psi->hicDraw)
		DOUT("Opened Internal draw handler\n");
	}
    }

    npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;

    psi->dwFlags |= STREAM_VIDEO;       // is a video stream
    psi->dwFlags |= STREAM_ENABLED;
    npMCI->nVideoStreams++;

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | InitAudioStream | initialize a audio stream
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL InitAudioStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi)
{
    int stream = (int) (INT_PTR) (psi - npMCI->paStreamInfo);
    LPWAVEFORMAT pwf;

    npMCI->wEarlyAudio = (UINT)psi->sh.dwInitialFrames;

    pwf = (LPWAVEFORMAT)psi->lpFormat;

    if (pwf->nChannels == 0 || pwf->nSamplesPerSec == 0) {
	return FALSE;
    }

    if (pwf->wFormatTag == WAVE_FORMAT_PCM) {
	pwf->nBlockAlign = pwf->nChannels *
	    ((((LPPCMWAVEFORMAT)pwf)->wBitsPerSample + 7) / 8);

	pwf->nAvgBytesPerSec = pwf->nBlockAlign * pwf->nSamplesPerSec;
    }

    psi->sh.dwSampleSize = pwf->nBlockAlign;

    psi->dwFlags |= STREAM_AUDIO;       // audio stream
    psi->dwFlags |= STREAM_ENABLED;     // enabled by default.

    //
    //  make sure dwRate and dwScale are right
    //  dwRate/dwScale should be blocks/sec
    //
    Assert(muldiv32(pwf->nAvgBytesPerSec,1000,pwf->nBlockAlign) ==
	   muldiv32(psi->sh.dwRate, 1000, psi->sh.dwScale));

    //
    //  just to be safe set these ourself to the right value.
    //
    psi->sh.dwRate  = pwf->nAvgBytesPerSec;
    psi->sh.dwScale = pwf->nBlockAlign;

    //
    // only one audio stream can be active at once
    // for lack of a better default the first audio stream will
    // become the active one.
    //
    if (npMCI->nAudioStreams == 0) {
	npMCI->nAudioStream = stream;
	npMCI->psiAudio = psi;
    }

    npMCI->nAudioStreams++;
    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | InitOtherStream | initialize a random stream
 *
 ***************************************************************************/

STATICFN BOOL NEAR PASCAL InitOtherStream(NPMCIGRAPHIC npMCI, STREAMINFO *psi)
{
    int stream = (int) (INT_PTR) (psi - npMCI->paStreamInfo);

    /* Open the specified video compressor */
    psi->hicDraw = ICDrawOpen(psi->sh.fccType,psi->sh.fccHandler,psi->lpFormat);

    if (psi->hicDraw == NULL) {
	DPF(("Unable to play stream!\n"));
	return FALSE;
    }

    if (psi->cbData > 0) {
	ICSetState(psi->hicDraw, psi->lpData, psi->cbData);
    }

    psi->dwFlags |= STREAM_ENABLED;
////psi->dwFlags |= STREAM_OTHER;
    npMCI->nOtherStreams++;
    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | CleanIndex | This function cleans up the index loaded by
 *  ReadIndex() it does the following when cleaning up the index:
 *
 *      converts all offsets to be absolute
 *
 *      converts "alpha" format index's into new format.
 *
 *      computes the max buffer size needed to read this file.
 *
 ***************************************************************************/

INLINE static BOOL NEAR CleanIndex(NPMCIGRAPHIC npMCI)
{
    LONG        lScan;
    AVIINDEXENTRY FAR * px;
    AVIINDEXENTRY FAR * pxRec=NULL;
    DWORD       lIndexAdjust;

    Assert(npMCI->hpIndex != NULL);

    px = (AVIINDEXENTRY FAR *)npMCI->hpIndex;

#ifdef ALPHAFILES
    if (npMCI->dwFlags & MCIAVI_USINGALPHAFORMAT)
	lIndexAdjust = 0;
    else
#endif
    if (// (avihdr.dwFlags & AVIF_MUSTUSEINDEX) ||
		(px->dwChunkOffset < 100))
	lIndexAdjust = npMCI->dwMovieListOffset;
    else
	lIndexAdjust = (npMCI->dwMovieListOffset + sizeof(DWORD)) -
			    px->dwChunkOffset;

//!!! only compute this for the video stream! (or interleaved...)
    npMCI->dwSuggestedBufferSize = 0; // lets get this exactly right

    DPF(("Adjusting index by %ld bytes....\n", lIndexAdjust));

    /* Can we do anything to see if the index is valid? */
    for (lScan = 0; lScan < (LONG)npMCI->macIndex;
		lScan++, ++((AVIINDEXENTRY _huge *)px)) {
	DWORD   ckid;

	//
	// adjust the offset to be absolute
	//
	px->dwChunkOffset += lIndexAdjust;

	// get ckid
	ckid = px->ckid;

	//
	// make sure the buffer size is right, ignore audio chunks because
	// they are either in a 'rec' or we will be reading them into
	// internal buffers not the main buffer.365
	//
	if (((npMCI->dwFlags & MCIAVI_NOTINTERLEAVED) ||
	    ckid == listtypeAVIRECORD) &&
	    TWOCCFromFOURCC(ckid) != cktypeWAVEbytes) {

	    if (px->dwChunkLength + 8 > npMCI->dwSuggestedBufferSize)
		npMCI->dwSuggestedBufferSize = px->dwChunkLength + 12;
	}

#ifdef ALPHAFILES
	//
	// convert a "old" index to a new index
	//
	if (npMCI->dwFlags & MCIAVI_USINGALPHAFORMAT) {
	    switch(ckid) {
		case ckidDIBbits:
		    px->dwFlags |= AVIIF_KEYFRAME;
		    px->ckid = MAKEAVICKID(cktypeDIBbits, 0);
		    break;

		case ckidDIBcompressed:
		    px->ckid = MAKEAVICKID(cktypeDIBcompressed, 0);
		    break;

		case ckidDIBhalfframe:
		    px->ckid = MAKEAVICKID(cktypeDIBhalf, 0);
		    break;

		case ckidPALchange:
		    px->ckid = MAKEAVICKID(cktypePALchange, 0);
		    break;

		case ckidWAVEbytes:
		    px->ckid = MAKEAVICKID(cktypeWAVEbytes, 1);
		    break;

		case ckidWAVEsilence:
		    px->ckid = MAKEAVICKID(cktypeWAVEsilence, 1);
		    break;

		case ckidAVIPADDING:
		case ckidOLDPADDING:
		    px->ckid = ckidAVIPADDING;
		    break;
	    }

	    ckid = px->ckid;
	}
#endif

	//
	// do special things with the video stream.
	//

	if (StreamFromFOURCC(ckid) == (UINT)npMCI->nVideoStream) {

	    //
	    // fix up bogus index's by adding any missing AVIIF_KEYFRAME
	    // bits. ie this only applies for silly RLE files.
	    //
	    if (TWOCCFromFOURCC(ckid) == cktypeDIBbits &&
		VIDFMT(npMCI->nVideoStream)->biCompression <= BI_RLE8)

		px->dwFlags |= AVIIF_KEYFRAME;

	    //
	    // for video streams, make sure the palette changes are marked
	    // as a no time chunk
	    //
	    if (TWOCCFromFOURCC(ckid) == cktypePALchange)
		px->dwFlags |= AVIIF_NOTIME/*|AVIIF_PALCHANGE*/;

	    //
	    //  make sure the 'REC ' list has the right flags.
	    //
	    if (pxRec) {
		if ((px->dwFlags & AVIIF_KEYFRAME) !=
		    (pxRec->dwFlags & AVIIF_KEYFRAME)) {

		    // Record list does not have correct flags

		    pxRec->dwFlags &= ~AVIIF_KEYFRAME;
		    pxRec->dwFlags |= (px->dwFlags & AVIIF_KEYFRAME);
		}
	    }
	}

	if (ckid == listtypeAVIRECORD) {

	    pxRec = px;

	    if (npMCI->dwFlags & MCIAVI_NOTINTERLEAVED) {
		DPF(("Non interleaved file with a 'REC ' in it?\n"));
		npMCI->wEarlyRecords = max(npMCI->wEarlyVideo, npMCI->wEarlyAudio);

		if (npMCI->wEarlyRecords > 0) {
		    DPF(("Interlaved file with bad header\n"));
		    npMCI->dwFlags &= ~MCIAVI_NOTINTERLEAVED;
		}
	    }
	}
    }

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | MakeFrameIndex | makes the frame index
 *
 *      the frame index is a array of AVIFRAMEINDEX entries, one for each
 *      frame in the movie.  using the frame index we can easily find
 *      a given frame, along with it's keyframe and palette.
 *
 ***************************************************************************/

static BOOL NEAR MakeFrameIndex(NPMCIGRAPHIC npMCI)
{
    LONG        nFrames;
    LONG        iFrameStart;
    LONG        iFrame;
    LONG        iKeyFrame;
    LONG        nKeyFrames;
    LONG        iScan;
    LONG        iNewIndex;
    LONG        iPalette;
    BOOL        fInterleaved;
    DWORD       ckid;
    STREAMINFO *psi;

    AVIINDEXENTRY _huge * pNewIndex;
    AVIINDEXENTRY _huge * pIndexEntry;
    AVIFRAMEINDEX _huge * pFrameIndex;

    if (npMCI->nVideoStreams == 0)
	return TRUE;

    if (npMCI->hpFrameIndex != NULL)
	return TRUE;

    psi = npMCI->psiVideo;
    Assert(psi != NULL);

    fInterleaved = !(npMCI->dwFlags & MCIAVI_NOTINTERLEAVED);

    if (fInterleaved &&
	muldiv32(npMCI->dwRate, 1000, npMCI->dwScale) !=
	muldiv32(psi->sh.dwRate, 1000, psi->sh.dwScale)) {
	//
	//  master video stream should match the movie rate!
	//
	AssertSz(FALSE, "Video stream differnet rate than movie");
	npMCI->dwRate  = psi->sh.dwRate;
	npMCI->dwScale = psi->sh.dwScale;
    }

    if (fInterleaved)
	iFrameStart = -(LONG)npMCI->wEarlyRecords;
    else
	iFrameStart = -(LONG)npMCI->wEarlyVideo;

    nFrames = npMCI->lFrames - iFrameStart;

    // hpFrameIndex is modified to point partway into the block.
    // therefore we need to remember the handle of the block so we
    // can free it correctly (esp on NT)
    if ( (npMCI->hgFrameIndex = GlobalAlloc(GMEM_SHARE|GHND, (DWORD)(nFrames+1) * sizeof(AVIFRAMEINDEX)))
      && (npMCI->hpFrameIndex = GlobalLock(npMCI->hgFrameIndex))) {
    } else {
	// We failed to allocate, or we failed to lock down.  Clean up
	// and return an error.
	if (npMCI->hgFrameIndex) {
	    GlobalFree(npMCI->hgFrameIndex);
	    npMCI->hgFrameIndex = 0;
	}
	DPF(("Couldn't allocate memory for frame index!\n"));
	return FALSE;
    }

    //
    //  do this so we can just index the array with the frame number
    //  (positive or neg)
    //
    npMCI->hpFrameIndex += (-iFrameStart);

    pFrameIndex = npMCI->hpFrameIndex;

    iFrame    = iFrameStart;
    iKeyFrame = -(LONG)npMCI->wEarlyVideo; // iFrameStart;
    iNewIndex = 0;
    iPalette  = -1; // first palette
    nKeyFrames= 0;

#ifdef USEAVIFILE
    if (npMCI->pf) {
	PAVISTREAM ps = SI(npMCI->nVideoStream)->ps;

	for (iFrame = 0; iFrame < npMCI->lFrames; iFrame++) {

	    LONG iKey;

	    iKey      = AVIStreamFindSample(ps,iFrame,FIND_PREV|FIND_KEY);
	    iPalette  = AVIStreamFindSample(ps,iFrame,FIND_PREV|FIND_FORMAT);

	    if (iKey != -1)
		iKeyFrame = iKey;

	    if (iPalette == -1)
		iPalette = 0;

	    pFrameIndex[iFrame].iPrevKey = (UINT)(iFrame - iKeyFrame);
	    pFrameIndex[iFrame].iNextKey = 0;
	    pFrameIndex[iFrame].iPalette = (WORD)iPalette;
	    pFrameIndex[iFrame].dwOffset = 0;
	    pFrameIndex[iFrame].dwLength = 0;

	    Assert(iPalette <= 0xFFFF);

	    if (iFrame - iKeyFrame > 0xFFFF) {
		//!!! we need to set a flag!
		//!!! we need to throw out the index!
		AssertSz(FALSE, "File has too few key frames");
		pFrameIndex[iFrame].iPrevKey = 0;
	    }
	}

	goto ack;
    }
#endif

    Assert(npMCI->hpIndex != NULL);
    Assert(npMCI->macIndex != 0L);
    pNewIndex   = npMCI->hpIndex;
    pIndexEntry = npMCI->hpIndex;

    for (iScan = 0; iScan < (LONG)npMCI->macIndex; iScan++, pIndexEntry++) {

	ckid = pIndexEntry->ckid;

	//
	// check for palette changes.
	//
	if (StreamFromFOURCC(ckid) == (UINT)npMCI->nVideoStream &&
	    TWOCCFromFOURCC(ckid) == cktypePALchange) {

	    iPalette = iNewIndex;

	    pNewIndex[iNewIndex++] = *pIndexEntry;

	    if (fInterleaved)
		pFrameIndex[iFrame-1].iPalette = (WORD)iPalette;
	}

	//
	// remove the video stream from the master index
	//
	if ((ckid != listtypeAVIRECORD) &&
	    (StreamFromFOURCC(ckid) != (UINT)npMCI->nVideoStream)) {
	    pNewIndex[iNewIndex++] = *pIndexEntry;
	}

	//
	//  in interleaved files a "frame" happens every list record
	//
	//  in non-interleaved files a "frame" happens every piece of
	//  data in the video stream (except no time chunks)
	//
	if (fInterleaved) {

	    if (ckid != listtypeAVIRECORD)
		continue;

	} else {

	    if ((StreamFromFOURCC(ckid) != (UINT)npMCI->nVideoStream) ||
		(pIndexEntry->dwFlags & AVIIF_NOTIME))

		continue;
	}

	AssertSz(iFrame < npMCI->lFrames,"Too many frames in index!");

	if (iFrame >= npMCI->lFrames) {
	    break;
	}

	if (pIndexEntry->dwFlags & AVIIF_KEYFRAME) {
	    iKeyFrame = iFrame;
	    nKeyFrames++;
	}

	pFrameIndex[iFrame].iPrevKey = (UINT)(iFrame - iKeyFrame);
	pFrameIndex[iFrame].iNextKey = 0;
	pFrameIndex[iFrame].iPalette = (WORD)iPalette;
	pFrameIndex[iFrame].dwOffset = pIndexEntry->dwChunkOffset;
	pFrameIndex[iFrame].dwLength = pIndexEntry->dwChunkLength;

	if (fInterleaved)
	    pFrameIndex[iFrame].dwOffset += 3 * sizeof(DWORD);

	Assert(iPalette <= 0xFFFF);

	if (iFrame - iKeyFrame > 0xFFFF) {
	    //!!! we need to set a flag!
	    //!!! we need to throw out the index!
	    AssertSz(FALSE, "File has too few key frames");
	    pFrameIndex[iFrame].iPrevKey = 0;
	}

	iFrame++;
    }
ack:
    //
    //  iFrame better equal npMCI->lFrames
    //
    Assert(iFrame == npMCI->lFrames);

    if (iFrame < npMCI->lFrames)
	npMCI->lFrames = iFrame;

    //
    // make a "dummy" last frame
    //
    pFrameIndex[iFrame].iPrevKey = (UINT)(iFrame - iKeyFrame);
    pFrameIndex[iFrame].iNextKey = 0;
    pFrameIndex[iFrame].iPalette = (WORD)iPalette;
    pFrameIndex[iFrame].dwOffset = 0;
    pFrameIndex[iFrame].dwLength = 0;

    //
    // compute the key frames every value
    //
    if (nKeyFrames) {

	if (nKeyFrames > 1)
	    npMCI->dwKeyFrameInfo = (DWORD)((nFrames + nKeyFrames/2)/nKeyFrames);
	else
	    npMCI->dwKeyFrameInfo = 0;
    }

    //
    //  now go through the index, and fix all the iNextKey fields
    //
    pFrameIndex = npMCI->hpFrameIndex;
////iKeyFrame = npMCI->lFrames; //!!! what should this be set to? zero?

    for (iFrame = npMCI->lFrames; iFrame>=iFrameStart; iFrame--)
    {
	if (pFrameIndex[iFrame].iPrevKey == 0)
	    iKeyFrame = iFrame;

	if (iKeyFrame >= iFrame)
	    pFrameIndex[iFrame].iNextKey = (UINT)(iKeyFrame - iFrame);
	else
	    pFrameIndex[iFrame].iNextKey = 0xFFFF;      // way far away

	if (iKeyFrame - iFrame > 0xFFFF) {
	    //!!! we need to set a flag!
	    //!!! we need to throw out the index!
	    AssertSz(FALSE, "File has too few key frames");
	    pFrameIndex[iFrame].iNextKey = 0;
	}
    }

    //
    // we dont need the index, if we are using AVIFile or
    // we have a interleaved file.  when the file is interleaved
    // we never do random access (except for palette changes)
    //
    // !!!this is not true, we need the index iff we have a audio only
    // file or we play a interleaved file real slow.
    //
    if (npMCI->pf /* ||
	(fInterleaved && !(npMCI->dwFlags & MCIAVI_ANIMATEPALETTE))*/ ) {
	DOUT("The Master index must go!\n");
	iNewIndex = 0;
    }

    //
    // now re-alloc the master index down to size.
    //
    // !!! do we even need the master index anymore, for interleaved files?
    //
    DPF(("Master index was %ld entries now %ld\n",npMCI->macIndex, iNewIndex));

    npMCI->macIndex = iNewIndex;

    if (iNewIndex > 0) {
	npMCI->hpIndex = (AVIINDEXENTRY _huge *)
		GlobalReAllocPtr(npMCI->hpIndex,
				 (LONG)iNewIndex * sizeof(AVIINDEXENTRY),
				 GMEM_MOVEABLE | GMEM_SHARE);

	Assert(npMCI->hpIndex != NULL);
    }
    else {
	if (npMCI->hpIndex)
	    GlobalFreePtr(npMCI->hpIndex);
	npMCI->hpIndex = NULL;
    }

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | ReadIndex | Read the index into npMCI->hpIndex.  Should
 *      only be called if the HASINDEX flag is set.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data
 *
 * @rdesc TRUE means no errors, false means unable to read index.
 *
 ***************************************************************************/
BOOL FAR PASCAL ReadIndex(NPMCIGRAPHIC npMCI)
{
    MMCKINFO    ck;
    DWORD       dwOldPos;

    if (npMCI->hpIndex || npMCI->hpFrameIndex)
	return TRUE;

    if (!(npMCI->dwFlags & MCIAVI_HASINDEX))
	return FALSE;

    if (npMCI->pf) {
	MakeFrameIndex(npMCI);
	return TRUE;
    }

#if 0
    if (GetCurrentTask() != npMCI->hTask) {

	/* this function is called (from GraphicStatus) when
	 * possibly playing - so we have to suspend play while we read
	 * the index.
	 */
    // won't work with the current multi-thread design
    //   TEMPORARYSTATE  ts;

    //   if (StopTemporarily(npMCI, &ts) == 0) {
    //       mciaviTaskMessage(npMCI, TASKREADINDEX);
    //        RestartAgain(npMCI, &ts);
    //        return (npMCI->hpIndex != NULL);
    //    }
	return(FALSE);
    }
#else
    if (GetCurrentTask() != npMCI->hTask)
	return FALSE;
#endif

    dwOldPos = mmioSeek(npMCI->hmmio, 0, SEEK_CUR);

    DPF(("Reading index: starting from %lx\n", npMCI->dwBigListEnd));

    if (mmioSeek(npMCI->hmmio, npMCI->dwBigListEnd, SEEK_SET) == -1) {
IndexReadError:
	DPF(("Error reading index!\n"));
	npMCI->dwFlags &= ~(MCIAVI_HASINDEX);
	mmioSeek(npMCI->hmmio, dwOldPos, SEEK_SET);
	return FALSE;
    }

    ck.ckid = ckidAVINEWINDEX;
    if (mmioDescend(npMCI->hmmio, &ck, NULL, MMIO_FINDCHUNK) != 0) {
	goto IndexReadError;
    }

    /* A zero-size index isn't much good. */
    if (ck.cksize == 0)
	goto IndexReadError;

    npMCI->macIndex = ck.cksize / sizeof(AVIINDEXENTRY);
    npMCI->hpIndex = (AVIINDEXENTRY _huge *)
		     GlobalAllocPtr(GMEM_SHARE | GMEM_MOVEABLE, ck.cksize);

    if (!npMCI->hpIndex) {
	DPF(("Insufficient memory to read index.\n"));
	goto IndexReadError;
    }

#ifndef _WIN32
    Assert(OFFSETOF(npMCI->hpIndex) == 0);
#endif

    if (mmioRead(npMCI->hmmio, (HPSTR) npMCI->hpIndex, ck.cksize) != (LONG) ck.cksize) {
	Assert(0);
	goto IndexReadError;
    }

    CleanIndex(npMCI);
    MakeFrameIndex(npMCI);

////should we do this for audio? remove video data?
////MakeStreamIndex(npMCI, ???);

    mmioSeek(npMCI->hmmio, dwOldPos, SEEK_SET);
    return TRUE;
}

/***************************************************************************
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | IsRectBogus | 'nuf said
 *
 ***************************************************************************/

INLINE static BOOL NEAR PASCAL IsRectBogus(RECT* prc)
{
    if (prc->right  - prc->left <= 0 ||
	prc->bottom - prc->top <= 0 ||
	prc->bottom <= 0 ||
	prc->right <= 0)

	return TRUE;
    else
	return FALSE;
}

#ifndef _WIN32  // _WIN32 has one!
/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | atol | local version of atol
 *
 ***************************************************************************/

static LONG NEAR PASCAL atol(char *sz)
{
    LONG l = 0;

    while (*sz && *sz >= '0' && *sz <= '9')
	l = l*10 + *sz++ - '0';

    return l;
}
#endif

#ifndef _WIN32

/*--------------------------------------------------------------------------
 *
 *  IsCDROMDrive() -
 *
 * Purpose:  Return non-zero if a RAM drive
 *
 *  wDrive   drive index (0=A, 1=B, ...)
 *
 *  return   TRUE/FALSE
 *-------------------------------------------------------------------------*/

#pragma optimize("", off)
static BOOL NEAR PASCAL IsCDROMDrive(UINT wDrive)
{
    BOOL f;

    _asm {
	mov ax, 1500h     /* first test for presence of MSCDEX */
	xor bx, bx
	int 2fh
	mov ax, bx        /* MSCDEX is not there if bx is still zero */
	or  ax, ax        /* ...so return FALSE from this function */
	jz  no_mscdex
	mov ax, 150bh     /* MSCDEX driver check API */
	mov cx, wDrive    /* ...cx is drive index */
	int 2fh
no_mscdex:
	mov f,ax
    }
    return f;
}
#pragma optimize("", on)

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | IsNetFile | is the passed file on a network drive?
 *
 ***************************************************************************/

static BOOL NEAR PASCAL IsNetFile(LPTSTR szFile)
{
    OFSTRUCT            of;

    if (OpenFile(szFile, &of, OF_PARSE) == -1)
	return FALSE;

    AnsiUpper(of.szPathName);

    if (of.szPathName[0] == '\\' && of.szPathName[1] == '\\')
	return TRUE;

    if (of.szPathName[0] == '/' && of.szPathName[1] == '/')
	return TRUE;

    if (of.szPathName[1] == ':' &&
	GetDriveType(of.szPathName[0] - 'A') == DRIVE_REMOTE &&
	!IsCDROMDrive(of.szPathName[0] - 'A'))

	return TRUE;

    return FALSE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | IsCDROMFile | is the passed file on a CD-ROM drive?
 *
 ***************************************************************************/

static BOOL NEAR PASCAL IsCDROMFile(LPTSTR szFile)
{
    OFSTRUCT of;

    if (OpenFile(szFile, &of, OF_PARSE) == -1)
	return FALSE;

    AnsiUpper(of.szPathName);

    if (of.szPathName[0] == '\\' && of.szPathName[1] == '\\')
	return FALSE;

    if (of.szPathName[0] == '/' && of.szPathName[1] == '/')
	return FALSE;

    if (of.szPathName[1] == ':' &&
	GetDriveType(of.szPathName[0] - 'A') == DRIVE_REMOTE &&
	IsCDROMDrive(of.szPathName[0] - 'A'))

	return TRUE;

    return FALSE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api UINT | GetFileDriveType | return drive type given a file
 *
 *      DRIVE_CDROM
 *      DRIVE_REMOTE
 *      DRIVE_FIXED
 *      DRIVE_REMOVABLE
 *
 ***************************************************************************/

static UINT NEAR PASCAL GetFileDriveType(LPSTR szPath)
{
    if (IsCDROMFile(szPath))
	return DRIVE_CDROM;

    if (IsNetFile(szPath))
	return DRIVE_REMOTE;

    if (szPath[1] == ':')
	return GetDriveType(szPath[0] - 'A');

    return 0;
}
#else // _WIN32
/*
 * We cannot pass the call directly to GetDriveType as we have a full
 * pathname - and that gets rejected by GetFileDriveType.  Using a
 * static variable is ok - the open call is serialised.  The assumption
 * is that this routine is ONLY called with complete path information.
 */

TCHAR DriveType[4] = TEXT(".:\\");
static UINT NEAR PASCAL GetFileDriveType(LPTSTR szPath)
{

    Assert(szPath != NULL);
    if (*szPath == TEXT('\\')) {
	Assert(szPath[1] == TEXT('\\'));
	return(DRIVE_REMOTE);
    }

    if (szPath[1] == TEXT(':')) {
	DriveType[0] = szPath[0];
	return GetDriveType(DriveType);
    }

    return 0;
}

#endif
