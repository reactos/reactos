/****************************************************************************
 *
 *  GETFRAME.CPP
 *
 *  this file contains the GetFrame APIs
 *
 *      AVIStreamGetFrameOpen
 *      AVIStreamGetFrameClose
 *      AVIStreamGetFrame
 *
 *  it also contains the default GetFrame implemenation
 *
 *      GetFrameDef
 *
 ***************************************************************************/

#include <win32.h>
#include <compobj.h>
#include <compman.h>
#include <memory.h>             // for _fmemset

#include "avifile.h"
#include "debug.h"              // for good ol' DPF()

/****************************************************************************
 *
 ***************************************************************************/

//!!! ACK
#define AVISF_VIDEO_PALCHANGES          0x00010000

#define ERR_FAIL   ResultFromScode(E_FAIL)
#define ERR_MEMORY ResultFromScode(E_OUTOFMEMORY)

#define WIDTHBYTES(i)       ((UINT)((i+31)&(~31))/8)
#define DIBWIDTHBYTES(lpbi) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(lpbi)->biBitCount)

/****************************************************************************
 *
 *  class for default IGetFrame
 *
 ***************************************************************************/

class FAR GetFrameDef : public IGetFrame
{
public:
    GetFrameDef(IAVIStream FAR *pavi=NULL);

public:
    // IUnknown stuff

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IGetFrame stuff.

    STDMETHODIMP Begin              (LONG lStart, LONG lEnd, LONG lRate);
    STDMETHODIMP End                ();

    STDMETHODIMP SetFormat          (LPBITMAPINFOHEADER lpbi, LPVOID lpBits, int x, int y, int dx, int dy);

    STDMETHODIMP_(LPVOID) GetFrame  (LONG lPos);

private:
    ~GetFrameDef();
    void FreeStuff();

    // for AddRef
    ULONG   ulRefCount;

    // instance data.
    BOOL                        fBegin;         // inside of Begin/End
    BOOL                        fFmtChanges;    // file has format changes.

    PAVISTREAM			pavi;
    LONG                        lFrame;         // last frame decompressed

    LPVOID                      lpBuffer;       // read buffer.
    LONG                        cbBuffer;       // size of read buffer
    LPVOID                      lpFormat;       // stream format
    LONG                        cbFormat;       // size of format

    LPVOID                      lpFrame;        // the frame (format)
    LPVOID                      lpBits;         // the frame (bits)
    HIC                         hic;            // decompress handle

    BOOL                        fDecompressEx;  // using ICDecompressEx
    int                         x,y,dx,dy;      // where to decompress

    // to watch for the format changing.
    DWORD			dwFormatChangeCount;
    DWORD			dwEditCount;
};

/****************************************************************************

 IUnknown stuff.

 ***************************************************************************/

STDMETHODIMP GetFrameDef::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (riid == IID_IGetFrame ||
        riid == IID_IUnknown) {     //!!! should we do Unknown or pass on?

        *ppv = (LPVOID)this;
        AddRef();
        return ResultFromScode(S_OK);
    }
    else if (pavi) {
        return pavi->QueryInterface(riid, ppv);
    }
    else {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) GetFrameDef::AddRef()
{
    return ulRefCount++;
}

STDMETHODIMP_(ULONG) GetFrameDef::Release()
{
    if (--ulRefCount == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/****************************************************************************
 ***************************************************************************/

GetFrameDef::GetFrameDef(IAVIStream FAR *pavi)
{
    this->pavi = pavi;

    ulRefCount = 1;

    fBegin = FALSE;
    fFmtChanges = FALSE;
    fDecompressEx = FALSE;

    lFrame = -4242;

    lpBuffer = NULL;
    lpFormat = NULL;
    cbBuffer = 0;
    cbFormat = 0;

    lpFrame = NULL;
    lpBits  = NULL;
    hic     = NULL;

    if (this->pavi == NULL)
        return;

    pavi->AddRef();
}

/****************************************************************************
 ***************************************************************************/

GetFrameDef::~GetFrameDef()
{
    FreeStuff();

    if (pavi)
        pavi->Release();
}

/****************************************************************************
 ***************************************************************************/

void GetFrameDef::FreeStuff()
{
    if (this->lpFrame && this->lpFrame != this->lpFormat) {
        GlobalFreePtr(this->lpFrame);
        this->lpFrame = 0;
    }

    if (this->lpFormat) {
        GlobalFreePtr(this->lpFormat);
        this->lpFormat = 0;
    }

    if (this->hic) {

        if (this->fDecompressEx)
            ICDecompressExEnd(this->hic);
        else
            ICDecompressEnd(this->hic);

        ICClose(this->hic);
        this->hic = 0;
    }
}

/****************************************************************************
 ***************************************************************************/

STDMETHODIMP GetFrameDef::SetFormat(LPBITMAPINFOHEADER lpbi, LPVOID lpBits, int x, int y, int dx, int dy)
{
    LPBITMAPINFOHEADER	lpbiC;
    LPBITMAPINFOHEADER	lpbiU;
    DWORD		dw;
    DWORD		fccHandler;
    AVISTREAMINFO       info;
    BOOL                fScreen;

    //
    // lpbi == 1 means choose a format for the screen.
    //
    // !!!we need a flag or something to do this.
    //
    if (fScreen = (lpbi == (LPBITMAPINFOHEADER)1))
        lpbi = NULL;

    //
    // get the vital stats
    //
    _fmemset(&info, 0, sizeof(info));
    pavi->Info(&info, sizeof(info));

    //
    //  is this a video stream?
    //
    if (info.fccType != streamtypeVIDEO)
        return ERR_FAIL;

    this->fBegin = FALSE;
    this->fFmtChanges = (info.dwFlags & AVISF_VIDEO_PALCHANGES) != 0;

    this->dwEditCount = info.dwEditCount;
    this->dwFormatChangeCount = info.dwFormatChangeCount;

    //
    // get the stream format
    //
    if (this->lpFormat == NULL) {

        //
        // alocate a read buffer.
        //
        this->cbBuffer = (LONG)info.dwSuggestedBufferSize;

	if (this->cbBuffer == 0)
	    this->cbBuffer = 1024;

        AVIStreamFormatSize(this->pavi,
			    AVIStreamStart(this->pavi),
                            &this->cbFormat);

        this->lpFormat = GlobalAllocPtr(GHND,this->cbFormat + this->cbBuffer);

	if (this->lpFormat == NULL)
	    goto error;

	AVIStreamReadFormat(this->pavi, AVIStreamStart(this->pavi),
			    this->lpFormat, &this->cbFormat);

	this->lpBuffer = (LPBYTE)this->lpFormat+this->cbFormat;
    }

    lpbiC = (LPBITMAPINFOHEADER)this->lpFormat;

    //
    // do standard BITMAPINFO header cleanup!
    //
    if (lpbiC->biClrUsed == 0 && lpbiC->biBitCount <= 8)
	lpbiC->biClrUsed = (1 << (int)lpbiC->biBitCount);

    if (lpbiC->biSizeImage == 0 && lpbiC->biCompression == BI_RGB)
	lpbiC->biSizeImage = DIBWIDTHBYTES(lpbiC) * lpbiC->biHeight;

    //
    // if the stream is uncompressed, we dont need a decompress buffer
    // make sure the caller hs not suggested a format first.
    //
    if (lpbiC->biCompression == 0 && lpBits == NULL) {

	if (lpbi == NULL ||
	   (lpbi->biCompression == lpbiC->biCompression &&
	    lpbi->biWidth	== lpbiC->biWidth &&
	    lpbi->biHeight	== lpbiC->biHeight &&
	    lpbi->biBitCount	== lpbi->biBitCount)) {


	    this->lpBits = (LPBYTE)lpbiC + (int)lpbiC->biSize +
		(int)lpbiC->biClrUsed * sizeof(RGBQUAD);

	    goto done;
	}
    }

    //
    // alocate the decompress buffer.
    //
    if (this->lpFrame == NULL) {

        this->lpFrame = GlobalAllocPtr(GHND,
            sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD));

        if (this->lpFrame == NULL) {
	    DPF("GetFrameInit: Can't allocate frame buffer!\n");
	    goto error;
        }
    }

    lpbiC = (LPBITMAPINFOHEADER)this->lpFormat;
    lpbiU = (LPBITMAPINFOHEADER)this->lpFrame;

    if (this->hic == NULL) {

        if (lpbiC->biCompression == 0)
            fccHandler = mmioFOURCC('D','I','B',' ');
        else if (lpbiC->biCompression == BI_RLE8)
            fccHandler = mmioFOURCC('R','L','E',' ');
        else
            fccHandler = info.fccHandler;

        if (lpbi) {
            if (lpbi->biWidth == 0)
                lpbi->biWidth = lpbiC->biWidth;

            if (lpbi->biHeight == 0)
                lpbi->biHeight = lpbiC->biHeight;
        }

        this->hic = ICDecompressOpen(ICTYPE_VIDEO, /*info.fccType,*/
                                   fccHandler,lpbiC,lpbi);

        if (this->hic == NULL) {
	    DPF("GetFrameInit: Can't find decompressor!\n");
	    goto error;
        }
    }

    if (lpbi) {
        if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
            lpbi->biClrUsed = (1 << (int)lpbi->biBitCount);
	
        hmemcpy(lpbiU,lpbi,lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD));

        if (lpbi->biBitCount <= 8) {
            ICDecompressGetPalette(this->hic,lpbiC,lpbiU);
        }
    } else if (fScreen) {

        ICGetDisplayFormat(this->hic, lpbiC, lpbiU, 0, dx, dy);

    } else {
        dw = ICDecompressGetFormat(this->hic,lpbiC,lpbiU);

	if ((LONG)dw < ICERR_OK)
	    goto error;
    }

    //
    // do standard BITMAPINFO header cleanup!
    //
    if (lpbiU->biClrUsed == 0 && lpbiU->biBitCount <= 8)
        lpbiU->biClrUsed = (1 << (int)lpbiU->biBitCount);

    if (lpbiU->biSizeImage == 0 && lpbiU->biCompression == BI_RGB)
        lpbiU->biSizeImage = DIBWIDTHBYTES(lpbiU) * lpbiU->biHeight;

    //
    // if we were passed a bits pointer, use it else re-alloc lpFrame
    // to contain the bits too.
    //
    if (lpBits) {
        this->lpBits = lpBits;
    }
    else {
        this->lpFrame = GlobalReAllocPtr(this->lpFrame,lpbiU->biSize +
            lpbiU->biSizeImage +
            lpbiU->biClrUsed * sizeof(RGBQUAD), GMEM_MOVEABLE);

        if (this->lpFrame == NULL) {
            DPF("GetFrameInit: Can't resize frame buffer!\n");
	    goto error;
        }

        lpbiU = (LPBITMAPINFOHEADER)this->lpFrame;

        this->lpBits = (LPBYTE)lpbiU + (int)lpbiU->biSize +
                (int)lpbiU->biClrUsed * sizeof(RGBQUAD);
    }

    //
    // use ICDecompressEx if we need to.  we need DecompressEx if
    // we are decompressing into a smaller area of the DIB, not the
    // whole surface.
    //
    if (dx == -1)
        dx = (int)lpbiU->biWidth;

    if (dy == -1)
        dy = (int)lpbiU->biHeight;

    this->fDecompressEx = (x != 0 || y != 0 ||
        dy != (int)lpbiU->biHeight || dx != (int)lpbiU->biWidth);

    if (this->fDecompressEx) {

        this->x = x;
        this->y = y;
        this->dx = dx;
        this->dy = dy;

        dw = ICDecompressExBegin(this->hic, 0,
            lpbiC, NULL, 0, 0, lpbiC->biWidth, lpbiC->biHeight,
            lpbiU, NULL, x, y, dx, dy);
    }
    else {
        dw = ICDecompressBegin(this->hic,lpbiC,lpbiU);
    }

    if (dw != ICERR_OK) {
        DPF("GetFrameSetFormat: ICDecompressBegin failed!\n");
	goto error;
    }

done:
    this->lFrame = -4224;   // bogus value
    return AVIERR_OK;

error:
    FreeStuff();
    return ERR_FAIL;
}

/****************************************************************************
 ***************************************************************************/

STDMETHODIMP GetFrameDef::Begin(LONG lStart, LONG lEnd, LONG lRate)
{
    fBegin = TRUE;
    GetFrame(lStart);

    return AVIERR_OK;
}

/****************************************************************************
 ***************************************************************************/

STDMETHODIMP GetFrameDef::End()
{
    fBegin = FALSE;
    return AVIERR_OK;
}

/****************************************************************************
 ***************************************************************************/

STDMETHODIMP_(LPVOID) GetFrameDef::GetFrame(LONG lPos)
{
    LPBITMAPINFOHEADER	    lpbiC;
    LPBITMAPINFOHEADER	    lpbiU;
    LONG                    l;
    LONG                    lKey;
    LONG		    lBytes;
    LONG		    lSize;
    LONG		    lRead;
    LONG                    err;
    AVISTREAMINFO           info;
    HRESULT		    hr;

    if (!this->pavi) {
	DPF("AVIStreamGetFrame: bad pointer\n");
	return NULL;
    }

    if (this->lpFormat == NULL) {
        return NULL;
    }

    //
    // if we are not in a Begin/End pair check for the format changing etc.
    //
    if (!this->fBegin) {

        _fmemset(&info, 0, sizeof(info));
        this->pavi->Info(&info, sizeof(info));

        if (info.dwFormatChangeCount != dwFormatChangeCount) {

            DPF("AVIStreamGetFrame: format has changed\n");

            BITMAPINFOHEADER bi = *((LPBITMAPINFOHEADER)this->lpFrame);

            FreeStuff();    // nuke it all.

            if (SetFormat(&bi, NULL, 0, 0, -1, -1) != 0 &&
                SetFormat(NULL, NULL, 0, 0, -1, -1) != 0)

                return NULL;
        }

        if (info.dwEditCount != dwEditCount) {
            DPF("AVIStreamGetFrame: stream has been edited (%lu)\n", info.dwEditCount);
            dwEditCount = info.dwEditCount;
            this->lFrame = -4224;     // Invalidate the cached frame
        }
    }

    //
    // quick check for the last frame.
    //
    if (this->lFrame == lPos)
        return this->hic ? this->lpFrame : this->lpFormat;

    //
    // locate the nearest key frame.
    //
    lKey = AVIStreamFindSample(this->pavi, lPos, FIND_KEY|FIND_PREV);

    //
    // either lPos was out of range or some internal error!
    //
    if (lKey == -1) {
	DPF("AVIStreamGetFrame: Couldn't find key frame!\n");
	return NULL;
    }

    //
    // we need to go back to the specifed key frame
    // or our current frame witch ever is closer
    //
    if (this->lFrame < lPos && this->lFrame >= lKey)
        lKey = this->lFrame + 1;

    lpbiC = (LPBITMAPINFOHEADER)this->lpFormat;
    lpbiU = (LPBITMAPINFOHEADER)this->lpFrame;

    //
    // decompress frame data from key frame to current frame.
    //
    for (l=lKey; l<=lPos; l++) {

        //
	// go read the format and call ICDecompressGetPalette() so
	// if the palette changes things will work.
	//
        if (this->fFmtChanges) {

            AVIStreamReadFormat(this->pavi, l, lpbiC, &this->cbFormat);
	
	    if (lpbiU && lpbiU->biBitCount <= 8) {
                ICDecompressGetPalette(this->hic,lpbiC,lpbiU);
	    }
	}

try_read_again:
        hr = AVIStreamRead(this->pavi, l, 1,
            this->lpBuffer, this->cbBuffer, &lBytes, &lRead);

        //
        // the read failed, mabey our buffer was too small
        // or it was a real error.
        //
        if (hr != NOERROR) {

            DPF("AVIStreamGetFrame: AVIStreamRead returns %lx\n", (DWORD) hr);

            lSize = 0;
            hr = AVIStreamSampleSize(this->pavi, l, &lSize);

            if (lSize > this->cbBuffer) {
                LPVOID lp;

                DPF("AVIStreamGetFrame: re-sizing read buffer from %ld to %ld\n", this->cbBuffer, lSize);

		lp = GlobalReAllocPtr(this->lpFormat,this->cbFormat+lSize,0);

                if (lp == NULL) {
                    DPF("AVIStreamGetFrame: Couldn't resize buffer\n");
                    return NULL;
                }

		this->lpFormat = lp;
		lpbiC = (LPBITMAPINFOHEADER)this->lpFormat;
		this->lpBuffer = (LPBYTE)lp + this->cbFormat;
                this->cbBuffer = lSize;

                goto try_read_again;
            }
	}

	if (lRead != 1) {
	    DPF("AVIStreamGetFrame: AVIStreamRead failed!\n");
	    return NULL;
	}

	if (lBytes == 0)
	    continue;

	lpbiC->biSizeImage = lBytes;

	if (this->hic == NULL) {
	    this->lFrame = lPos;
	    return this->lpFormat;
	}
	else if (this->fDecompressEx) {
            err = ICDecompressEx(this->hic,0,
                lpbiC,this->lpBuffer,
                0,0,(int)lpbiC->biWidth,(int)lpbiC->biHeight,
                lpbiU,this->lpBits,
                this->x,this->y,this->dx,this->dy);
        }
        else {
            err = ICDecompress(this->hic,0,
                lpbiC,this->lpBuffer,lpbiU,this->lpBits);
        }

        // !!! Error check?

        if (err < 0)
            ;
    }

    this->lFrame = lPos;
    return this->hic ? this->lpFrame : this->lpFormat;
}

/********************************************************************
* @doc EXTERNAL AVIStreamGetFrameOpen
*
* @api PGETFRAME | AVIStreamGetFrameOpen | This functions prepares 
*      to decompress video frames from the stream specified.
*
* @parm PAVISTREAM | pavi | Specifies a pointer to the 
*       stream used as the video source.
*
* @parm LPBITMAPINFOHEADER | lpbiWanted | Specifies a pointer to 
*       a structure defining the desired  video format.  If this is NULL,
*       a default format is used.
*
* @rdesc Returns a GetFrame object, which can be used with
*	<f AVIStreamGetFrame>.
*
*	If the system can't find decompressor that can decompress the stream
*	to the format given, or to any RGB format, the function returns NULL.
*
* @comm The <p pavi> parameter must specify a video stream.
*
*	This is essentially just a helper function to handle a simple form
*	of decompression.
*
* @xref <f AVIStreamGetFrame> <f AVIStreamGetFrameClose>
**********************************************************************/
STDAPI_(PGETFRAME) AVIStreamGetFrameOpen(PAVISTREAM pavi, LPBITMAPINFOHEADER lpbiWanted)
{
    PGETFRAME pgf=NULL;

    //
    // first ask the IAVIStream object if it can handle IGetFrame and
    // if it can let it do it.
    //
    pavi->QueryInterface(IID_IGetFrame, (LPVOID FAR *)&pgf);

    if (pgf == NULL) {
        //
        // the stream can't do it, make our own object.
        //
        pgf = new GetFrameDef(pavi);
    }

    //
    // set the format the caller wants
    //
    if (pgf->SetFormat(lpbiWanted, NULL, 0, 0, -1, -1)) {
        DPF("AVIStreamGetFrameOpen: unable to set format\n");
        pgf->Release();
        return NULL;
    }

    return pgf;
}

/********************************************************************
* @doc EXTERNAL AVIStreamGetFrameClose
*
* @api LONG | AVIStreamGetFrameClose | This function releases resources
*	used to decompress video frames.
*
* @parm PGETFRAME | pget | Specifies a handle returned from <f AVIStreamGetFrameOpen>.
*	After calling this function, the handle is invalid.
*
* @rdesc Returns an error code.
*
* @xref <f AVIStreamGetFrameOpen> <f AVIStreamGetFrame>
**********************************************************************/
STDAPI AVIStreamGetFrameClose(PGETFRAME pgf)
{
    if (pgf)
        pgf->Release();

    return AVIERR_OK;
}

/********************************************************************
* @doc EXTERNAL AVIStreamGetFrame
*
* @api LPVOID | AVIStreamGetFrame | This function returns a pointer to
*	a decompressed frame of video.
*
* @parm PGETFRAME | pgf | Specifies a pointer to a GetFrame object.
*
* @parm LONG | lPos | Specifies the position of desired frame in samples.
*
* @rdesc Returns NULL on error; otherwise it returns a far pointer 
*        to the frame data.  The returned data is a packed DIB.
*
* @comm The returned frame is valid only until the next call
*	to <f AVIStreamGetFrame> or <f AVIStreamGetFrameClose>.
*
* @xref <f AVIStreamGetFrameOpen>
**********************************************************************/
STDAPI_(LPVOID) AVIStreamGetFrame(PGETFRAME pgf, LONG lPos)
{
    if (pgf == NULL)
        return NULL;

    return pgf->GetFrame(lPos);
}

// !!! Do we need an AVIStreamGetFrameSetFormat?
