/****************************************************************************
 *
 *  AVIBALL.C
 *
 *  Sample AVIStream handler for a bouncing ball.  This code demonstrates
 *  how to write a custom stream handler so an application can deal with
 *  your custom file/data/whatever by using the standard AVIStream functions.
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

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <vfw.h>
#include <coguid.h>

///////////////////////////////////////////////////////////////////////////
//
// silly default parameters
//
///////////////////////////////////////////////////////////////////////////

#define DEFAULT_WIDTH   240
#define DEFAULT_HEIGHT  120
#define DEFAULT_LENGTH  100
#define DEFAULT_SIZE    6
#define DEFAULT_COLOR   RGB(255,0,0)
#define XSPEED		7
#define YSPEED		5

///////////////////////////////////////////////////////////////////////////
//
// useful macros
//
///////////////////////////////////////////////////////////////////////////

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)
#define DIBPTR(lpbi) ((LPBYTE)(lpbi) + \
	    (int)(lpbi)->biSize + \
	    (int)(lpbi)->biClrUsed * sizeof(RGBQUAD) )

///////////////////////////////////////////////////////////////////////////
//
// custom video stream instance structure
//
///////////////////////////////////////////////////////////////////////////

typedef struct {

    //
    // The Vtbl must come first
    //
    IAVIStreamVtbl FAR * lpvtbl;

    //
    //  private ball instance data
    //
    ULONG	ulRefCount;

    DWORD       fccType;        // is this audio/video

    int         width;          // size in pixels of each frame
    int         height;
    int         length;         // length in frames of the pretend AVI movie
    int         size;
    COLORREF    color;          // ball color

} AVIBALL, FAR * PAVIBALL;

///////////////////////////////////////////////////////////////////////////
//
// custom stream methods
//
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE AVIBallQueryInterface(PAVISTREAM ps, REFIID riid, LPVOID FAR* ppvObj);
HRESULT STDMETHODCALLTYPE AVIBallCreate       (PAVISTREAM ps, LONG lParam1, LONG lParam2);
ULONG   STDMETHODCALLTYPE AVIBallAddRef       (PAVISTREAM ps);
ULONG   STDMETHODCALLTYPE AVIBallRelease      (PAVISTREAM ps);
HRESULT STDMETHODCALLTYPE AVIBallInfo         (PAVISTREAM ps, AVISTREAMINFO FAR * psi, LONG lSize);
LONG    STDMETHODCALLTYPE AVIBallFindSample (PAVISTREAM ps, LONG lPos, LONG lFlags);
HRESULT STDMETHODCALLTYPE AVIBallReadFormat   (PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat);
HRESULT STDMETHODCALLTYPE AVIBallSetFormat    (PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG cbFormat);
HRESULT STDMETHODCALLTYPE AVIBallRead         (PAVISTREAM ps, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes,LONG FAR * plSamples);
HRESULT STDMETHODCALLTYPE AVIBallWrite        (PAVISTREAM ps, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten);
HRESULT STDMETHODCALLTYPE AVIBallDelete       (PAVISTREAM ps, LONG lStart, LONG lSamples);
HRESULT STDMETHODCALLTYPE AVIBallReadData     (PAVISTREAM ps, DWORD fcc, LPVOID lp,LONG FAR *lpcb);
HRESULT STDMETHODCALLTYPE AVIBallWriteData    (PAVISTREAM ps, DWORD fcc, LPVOID lp,LONG cb);

IAVIStreamVtbl AVIBallHandler = {
    AVIBallQueryInterface,
    AVIBallAddRef,
    AVIBallRelease,
    AVIBallCreate,
    AVIBallInfo,
    AVIBallFindSample,
    AVIBallReadFormat,
    AVIBallSetFormat,
    AVIBallRead,
    AVIBallWrite,
    AVIBallDelete,
    AVIBallReadData,
    AVIBallWriteData
};


//
// This is the function an application would call to create a PAVISTREAM to
// reference the ball.  Then the standard AVIStream function calls can be
// used to work with this stream.
//
PAVISTREAM FAR PASCAL NewBall(void)
{
    PAVIBALL pball;

    //
    // Create a pointer to our private structure which will act as our
    // PAVISTREAM
    //
    pball = (PAVIBALL) GlobalAllocPtr(GHND, sizeof(AVIBALL));

    if (!pball)
	return 0;

    //
    // Fill the function table
    //
    pball->lpvtbl = &AVIBallHandler;

    //
    // Call our own create code to create a new instance (calls AVIBallCreate)
    // For now, don't use any lParams.
    //
    pball->lpvtbl->Create((PAVISTREAM) pball, 0, 0);

    return (PAVISTREAM) pball;
}

///////////////////////////////////////////////////////////////////////////
//
// This function is called to initialize an instance of the bouncing ball.
//
// When called, we look at the information possibly passed in <lParam1>,
// if any, and use it to determine the length of movie they want. (Not
// supported by NewBall right now, but it could be).
//
///////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE AVIBallCreate(PAVISTREAM ps, LONG lParam1, LONG lParam2)
{
    PAVIBALL pball = (PAVIBALL) ps;

    //
    // what type of data are we? (audio/video/other stream)
    //
    pball->fccType = streamtypeVIDEO;

    //
    // We define lParam1 as being the length of movie they want us to pretend
    // to be.
    //
    if (lParam1)
	pball->length = (int) lParam1;
    else
	pball->length = DEFAULT_LENGTH;

    switch (pball->fccType) {

	case streamtypeVIDEO:
	    pball->color  = DEFAULT_COLOR;
	    pball->width  = DEFAULT_WIDTH;
	    pball->height = DEFAULT_HEIGHT;
	    pball->size   = DEFAULT_SIZE;
	    pball->ulRefCount = 1;	// note that we are opened once
	    return AVIERR_OK;           // success

	case streamtypeAUDIO:
	    return ResultFromScode(AVIERR_UNSUPPORTED); // we don't do audio

	default:
	    return ResultFromScode(AVIERR_UNSUPPORTED); // or anything else
    }
}


//
// Increment our reference count
//
ULONG STDMETHODCALLTYPE AVIBallAddRef(PAVISTREAM ps)
{
    PAVIBALL pball = (PAVIBALL) ps;
    return (++pball->ulRefCount);
}


//
// Decrement our reference count
//
ULONG STDMETHODCALLTYPE AVIBallRelease(PAVISTREAM ps)
{
    PAVIBALL pball = (PAVIBALL) ps;
    if (--pball->ulRefCount)
	return pball->ulRefCount;

    // Free any data we're keeping around - like our private structure
    GlobalFreePtr(pball);

    return 0;
}


//
// Fills an AVISTREAMINFO structure
//
HRESULT STDMETHODCALLTYPE AVIBallInfo(PAVISTREAM ps, AVISTREAMINFO FAR * psi, LONG lSize)
{
    PAVIBALL pball = (PAVIBALL) ps;

    if (lSize < sizeof(AVISTREAMINFO))
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);

    _fmemset(psi, 0, lSize);

    // Fill out a stream header with information about us.
    psi->fccType                = pball->fccType;
    psi->fccHandler             = mmioFOURCC('B','a','l','l');
    psi->dwScale                = 1;
    psi->dwRate                 = 15;
    psi->dwLength               = pball->length;
    psi->dwSuggestedBufferSize  = pball->height * ALIGNULONG(pball->width);
    psi->rcFrame.right          = pball->width;
    psi->rcFrame.bottom         = pball->height;
    lstrcpy(psi->szName, TEXT("Bouncing ball video"));

    return AVIERR_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// AVIBallReadFormat: needs to return the format of our data.
//
///////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE AVIBallReadFormat   (PAVISTREAM ps, LONG lPos,LPVOID lpFormat,LONG FAR *lpcbFormat)
{
    PAVIBALL pball = (PAVIBALL) ps;
    LPBITMAPINFO    lpbi = (LPBITMAPINFO) lpFormat;

    if (lpFormat == NULL || *lpcbFormat == 0) {
	*lpcbFormat = sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD);
	return AVIERR_OK;
    }

    if (*lpcbFormat < sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD))
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);

    // This is a relatively silly example: we build up our
    // format from scratch every time.

    lpbi->bmiHeader.biSize              = sizeof(BITMAPINFOHEADER);
    lpbi->bmiHeader.biCompression       = BI_RGB;
    lpbi->bmiHeader.biWidth             = pball->width;
    lpbi->bmiHeader.biHeight            = pball->height;
    lpbi->bmiHeader.biBitCount          = 8;
    lpbi->bmiHeader.biPlanes            = 1;
    lpbi->bmiHeader.biClrUsed           = 2;
    lpbi->bmiHeader.biSizeImage         = pball->height * DIBWIDTHBYTES(lpbi->bmiHeader);

    lpbi->bmiColors[0].rgbRed           = 0;
    lpbi->bmiColors[0].rgbGreen         = 0;
    lpbi->bmiColors[0].rgbBlue          = 0;
    lpbi->bmiColors[1].rgbRed           = GetRValue(pball->color);
    lpbi->bmiColors[1].rgbGreen         = GetGValue(pball->color);
    lpbi->bmiColors[1].rgbBlue          = GetBValue(pball->color);

    *lpcbFormat = sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD);

    return AVIERR_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// AVIBallRead: needs to return the data for a particular frame.
//
///////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE AVIBallRead (PAVISTREAM ps, LONG lStart,LONG lSamples,LPVOID lpBuffer,LONG cbBuffer,LONG FAR * plBytes,LONG FAR * plSamples)
{
    PAVIBALL pball = (PAVIBALL) ps;
    LONG   lSize = pball->height * ALIGNULONG(pball->width); // size of frame
							     // in bytes
    int x, y;
    BYTE _huge *hp = lpBuffer;
    int xPos, yPos;

    // Reject out of range values
    if (lStart < 0 || lStart >= pball->length)
	return ResultFromScode(AVIERR_BADPARAM);

    // Did they just want to know the size of our data?
    if (lpBuffer == NULL || cbBuffer == 0)
	goto exit;

    // Will our frame fit in the buffer passed?
    if (lSize > cbBuffer)
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);

    // Figure out the position of the ball.
    // It just bounces back and forth.

    xPos = 5 + XSPEED * (int) lStart;			    // x = x0 + vt
    xPos = xPos % ((pball->width - pball->size) * 2);	    // limit to 2xwidth
    if (xPos > (pball->width - pball->size))		    // reflect if
	xPos = 2 * (pball->width - pball->size) - xPos;	    //   needed

    yPos = 5 + YSPEED * (int) lStart;
    yPos = yPos % ((pball->height - pball->size) * 2);
    if (yPos > (pball->height - pball->size))
	yPos = 2 * (pball->height - pball->size) - yPos;

    //
    // Build a DIB from scratch by writing in 1's where the ball is, 0's
    // where it isn't.
    //
    // Notice that we just build it in the buffer we've been passed.
    //
    // This is pretty ugly, I have to admit.
    //
    for (y = 0; y < pball->height; y++)
	{
	if (y >= yPos && y < yPos + pball->size)
	{
	    for (x = 0; x < pball->width; x++)
	    {
		*hp++ = (BYTE) ((x >= xPos && x < xPos + pball->size) ? 1 : 0);
	    }
	}
	else
	{
	    for (x = 0; x < pball->width; x++)
	    {
		*hp++ = 0;
	    }
	}
	
	hp += pball->width - ALIGNULONG(pball->width);
    }

exit:
    // We always return exactly one frame
    if (plSamples)
	*plSamples = 1;

    // Return the size of our frame
    if (plBytes)
	*plBytes = lSize;

    return AVIERR_OK;
}


HRESULT STDMETHODCALLTYPE AVIBallQueryInterface(PAVISTREAM ps, REFIID riid, LPVOID FAR* ppvObj)
{
    PAVIBALL pball = (PAVIBALL) ps;

    // We support the Unknown interface (everybody does) and our Stream
    // interface.

    if (_fmemcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0)
        *ppvObj = (LPVOID)pball;

    else if (_fmemcmp(riid, &IID_IAVIStream, sizeof(GUID)) == 0)
        *ppvObj = (LPVOID)pball;

    else {
        *ppvObj = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    AVIBallAddRef(ps);

    return AVIERR_OK;
}

LONG    STDMETHODCALLTYPE AVIBallFindSample (PAVISTREAM ps, LONG lPos, LONG lFlags)
{
    // The only format change is frame 0
    if ((lFlags & FIND_TYPE) == FIND_FORMAT) {
	if ((lFlags & FIND_DIR) == FIND_NEXT && lPos > 0)
	    return -1;	// no more format changes
	else
	    return 0;

    // FIND_KEY and FIND_ANY always return the same position because
    // every frame is non-empty and a key frame
    } else
        return lPos;
}

HRESULT STDMETHODCALLTYPE AVIBallReadData     (PAVISTREAM ps, DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE AVIBallSetFormat    (PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG cbFormat)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE AVIBallWriteData    (PAVISTREAM ps, DWORD fcc, LPVOID lp, LONG cb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE AVIBallWrite        (PAVISTREAM ps, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE AVIBallDelete       (PAVISTREAM ps, LONG lStart, LONG lSamples)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
