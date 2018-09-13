#include "thlibpch.h"

CThumbnailMaker::CThumbnailMaker()
{
    pImH = NULL;

}

CThumbnailMaker::~CThumbnailMaker()
{
    if (pImH)
        delete[] pImH;
}


STDMETHODIMP
CThumbnailMaker::Init( UINT uiDstWidth_,
                       UINT uiDstHeight_,
                       UINT uiSrcWidth_,
                       UINT uiSrcHeight_ )
{
    uiDstWidth = uiDstWidth_;
    uiDstHeight = uiDstHeight_;
    uiSrcWidth = uiSrcWidth_;
    uiSrcHeight = uiSrcHeight_;

    if (uiDstWidth < 1 || uiDstHeight < 1 ||
        uiSrcWidth < 1 || uiSrcHeight < 1)
        return E_INVALIDARG;

    if (pImH)
        delete[] pImH;

    pImH = new BGR3[uiDstWidth * uiSrcHeight];
    if (pImH == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

void
CThumbnailMaker::Scale( BGR3 *pucDst,
                        UINT uiDstWidth,
                        int iDstBytStep,
                        BGR3 *pucSrc,
                        UINT uiSrcWidth,
                        int iSrcBytStep )
{
    UINT uiDstX;
    int frac, mnum, mden;

    mnum = uiSrcWidth;
    mden = uiDstWidth;

    //
    // Scaling up, use a triangle filter.
    //
    if (mden >= mnum)
    {
        BGR3 *pucSrc1;

        frac = 0;

        //
        // Adjust the slope so that we calculate the fraction of the
        // "next" pixel to use (i.e. should be 0 for the first and
        // last dst pixel).
        //
        --mnum;
        if (--mden == 0)
            mden = 0; // avoid div by 0

        pucSrc1 = (BGR3 *)(((UCHAR *)pucSrc) + iSrcBytStep);

        for (uiDstX = 0; uiDstX < uiDstWidth; ++uiDstX)
        {
            if (frac == 0)
            {
                (*pucDst)[0] = (*pucSrc)[0];
                (*pucDst)[1] = (*pucSrc)[1];
                (*pucDst)[2] = (*pucSrc)[2];
            }
            else
            {
                (*pucDst)[0] =
                    ((mden - frac) * (*pucSrc)[0] +
                     frac * (*pucSrc1)[0]) / mden;
                (*pucDst)[1] =
                    ((mden - frac) * (*pucSrc)[1] +
                     frac * (*pucSrc1)[1]) / mden;
                (*pucDst)[2] =
                    ((mden - frac) * (*pucSrc)[2] +
                     frac * (*pucSrc1)[2]) / mden;
            }

            pucDst = (BGR3 *)((UCHAR *)pucDst + iDstBytStep);

            frac += mnum;
            if (frac >= mden)
            {
                frac -= mden;
                pucSrc = (BGR3 *)((UCHAR *)pucSrc +
                          iSrcBytStep);
                pucSrc1 = (BGR3 *)((UCHAR *)pucSrc1 +
                           iSrcBytStep);
            }
        }
    }

    //
    // Scaling down, use a box filter.
    //
    else
    {
        frac = 0;

        for (uiDstX = 0; uiDstX < uiDstWidth; ++uiDstX)
        {
            UINT uiSum[3];
            UINT uiCnt;

            uiSum[0] = uiSum[1] = uiSum[2] = 0;
            uiCnt = 0;

            frac += mnum;
            while (frac >= mden)
            {
                uiSum[0] += (*pucSrc)[0];
                uiSum[1] += (*pucSrc)[1];
                uiSum[2] += (*pucSrc)[2];
                ++uiCnt;

                frac -= mden;
                pucSrc = (BGR3 *)((UCHAR *)pucSrc +
                          iSrcBytStep);
            }

            (*pucDst)[0] = uiSum[0] / uiCnt;
            (*pucDst)[1] = uiSum[1] / uiCnt;
            (*pucDst)[2] = uiSum[2] / uiCnt;

            pucDst = (BGR3 *)((UCHAR *)pucDst + iDstBytStep);
        }
    }
}

//
// For AddScanline, we scale the input horizontally into our temporary
// image buffer.
//
STDMETHODIMP
CThumbnailMaker::AddScanline( UCHAR *pucSrc,
                              UINT uiY )
{
    if (pucSrc == NULL || uiY >= uiSrcHeight)
        return E_INVALIDARG;

    Scale(pImH + uiY * uiDstWidth, uiDstWidth, sizeof(BGR3),
          (BGR3 *)pucSrc, uiSrcWidth, sizeof(BGR3));

    return S_OK;
}

//
// For GetBITMAPINFO, we complete the scaling vertically and return the
// result as a DIB.
//
STDMETHODIMP
CThumbnailMaker::GetBITMAPINFO( BITMAPINFO **ppBMInfo,
                                DWORD *pdwSize )
{
    BITMAPINFO *pBMI;
    BITMAPINFOHEADER *pBMIH;
    DWORD dwBPL, dwTotSize;
    UCHAR *pDst;
    UINT uiDstX;

    if (ppBMInfo == NULL)
        return E_INVALIDARG;

    *ppBMInfo = NULL;

    dwBPL = (((uiDstWidth * 24) + 31) >> 3) & ~3;
    dwTotSize = sizeof(BITMAPINFOHEADER) + dwBPL * uiDstHeight;

    pBMI = (BITMAPINFO *)CoTaskMemAlloc(dwTotSize);
    if (pBMI == NULL)
        return E_OUTOFMEMORY;

    pBMIH = &pBMI->bmiHeader;
    pBMIH->biSize = sizeof(BITMAPINFOHEADER);
    pBMIH->biWidth = uiDstWidth;
    pBMIH->biHeight = uiDstHeight;
    pBMIH->biPlanes = 1;
    pBMIH->biBitCount = 24;
    pBMIH->biCompression = BI_RGB;
    pBMIH->biXPelsPerMeter = 0;
    pBMIH->biYPelsPerMeter = 0;
    pBMIH->biSizeImage = dwBPL * uiDstHeight;
    pBMIH->biClrUsed = 0;
    pBMIH->biClrImportant = 0;

    pDst = (UCHAR *)pBMIH + pBMIH->biSize + (uiDstHeight - 1) * dwBPL;

    for (uiDstX = 0; uiDstX < uiDstWidth; ++uiDstX)
    {
        Scale((BGR3 *)pDst + uiDstX, uiDstHeight, -(int)dwBPL,
              pImH + uiDstX, uiSrcHeight, uiDstWidth * sizeof(BGR3));
    }

    *ppBMInfo = pBMI;

    if (pdwSize)
        *pdwSize = dwTotSize;

    return S_OK;
}

STDMETHODIMP
CThumbnailMaker::GetSharpenedBITMAPINFO( UINT uiSharpPct,
                                         BITMAPINFO **ppBMInfo,
                                         DWORD *pdwSize )
{
    BITMAPINFO *pBMISrc;
    HRESULT hr;
    DWORD dwSize;
    int bpl;
    int x, y, wdiag, wadj, wcent;
#define SCALE 10000
    UCHAR *pucDst, *pucSrc[3];

    if (uiSharpPct > 100)
        return E_INVALIDARG;

    //
    // Get the unsharpened bitmap.
    //
    hr = GetBITMAPINFO(ppBMInfo, &dwSize);
    if (FAILED(hr))
        return hr;

    if (pdwSize)
        *pdwSize = dwSize;

    //
    // Create a duplicate to serve as the original.
    //
    pBMISrc = (BITMAPINFO *)new UCHAR[dwSize];
    if (pBMISrc == NULL)
    {
        delete *ppBMInfo;
        return E_OUTOFMEMORY;
    }
    memcpy(pBMISrc, *ppBMInfo, dwSize);

    bpl = (pBMISrc->bmiHeader.biWidth * 3 + 3) & ~3;

    //
    // Sharpen inside a 1 pixel border
    //
    pucDst = (UCHAR *)*ppBMInfo + sizeof(BITMAPINFOHEADER);
    pucSrc[0] = (UCHAR *)pBMISrc + sizeof(BITMAPINFOHEADER);
    pucSrc[1] = pucSrc[0] + bpl;
    pucSrc[2] = pucSrc[1] + bpl;

    wdiag = (10355 * uiSharpPct) / 100;
    wadj = (14645 * uiSharpPct) / 100;
    wcent = 4 * (wdiag + wadj);

    for (y = 1; y < pBMISrc->bmiHeader.biHeight-1; ++y)
    {
        for (x = 3*(pBMISrc->bmiHeader.biWidth-2); x >= 3; --x)
        {
            int v;

            v = pucDst[x] +
                (pucSrc[1][x] * wcent -
                 ((pucSrc[0][x - 3] +
                   pucSrc[0][x + 3] +
                   pucSrc[2][x - 3] +
                   pucSrc[2][x + 3]) * wdiag +
                  (pucSrc[0][x] +
                   pucSrc[1][x - 3] +
                   pucSrc[1][x + 3] +
                   pucSrc[2][x]) * wadj)) / SCALE;

            pucDst[x] = v < 0 ? 0 : v > 255 ? 255 : v;
        }

        pucDst += bpl;
        pucSrc[0] = pucSrc[1];
        pucSrc[1] = pucSrc[2];
        pucSrc[2] += bpl;
    }

    delete[] pBMISrc;

    return S_OK;
#undef SCALE
}

HRESULT ThumbnailMaker_Create( IThumbnailMaker **ppThumbMaker )
{
    HRESULT hr;
    CThumbnailMaker *pCThumbMaker;

    if (ppThumbMaker == NULL)
        return E_UNEXPECTED;

    hr = S_OK;

    pCThumbMaker = new CComObject<CThumbnailMaker>;
    if (pCThumbMaker == NULL || FAILED(hr))
    {
        if (pCThumbMaker)
            delete pCThumbMaker;
        else
            hr = E_OUTOFMEMORY;
            
        *ppThumbMaker = NULL;

        return hr;
    }

    *ppThumbMaker = (IThumbnailMaker *) pCThumbMaker;
    pCThumbMaker->AddRef();

    return S_OK;
}

STDMETHODIMP CThumbnailMaker::AddDIB(BITMAPINFO *pBMI)
{
    UCHAR * pBits;
    int ncolors;
    
    ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;
        
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    pBits = (UCHAR *)&pBMI->bmiColors[0] + ncolors * sizeof(RGBQUAD);

    return AddDIBSECTION( pBMI, (LPVOID) pBits );
}

STDMETHODIMP CThumbnailMaker::AddDIBSECTION ( BITMAPINFO * pBMI, LPVOID pBits )
{
    RGBQUAD *pRGBQ, *pQ;
    UCHAR *pucBits0, *pucBits, *pB, *pucBits240, *pucBits24, *pB24;
    int bpl;
    int x, y, ncolors;
    ULONG rmask, gmask, bmask;
    int rshift, gshift, bshift;
    HRESULT hr;

    //
    // Make sure that thumbnail maker has been properly initialized.
    //
    if (pBMI == NULL)
        return E_INVALIDARG;

    if (pBMI->bmiHeader.biWidth != (LONG)uiSrcWidth ||
        pBMI->bmiHeader.biHeight != (LONG)uiSrcHeight)
        return E_INVALIDARG;

    //
    // Don't handle RLE.
    //
    if (pBMI->bmiHeader.biCompression != BI_RGB &&
        pBMI->bmiHeader.biCompression != BI_BITFIELDS)
        return E_INVALIDARG;

    pRGBQ = (RGBQUAD *)&pBMI->bmiColors[0];

    ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;

    //
    // Decode 16/32bpp with masks.
    //
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            rmask = ((ULONG *)pRGBQ)[0];
            gmask = ((ULONG *)pRGBQ)[1];
            bmask = ((ULONG *)pRGBQ)[2];
            ncolors = 3;
        }
        else if (pBMI->bmiHeader.biBitCount == 16)
        {
            rmask = 0x7c00;
            gmask = 0x03e0;
            bmask = 0x001f;
        }
        else /* 32 */
        {
            rmask = 0xff0000;
            gmask = 0x00ff00;
            bmask = 0x0000ff;
        }

        for (rshift = 0; (rmask & 1) == 0; rmask >>= 1, ++rshift);
        if (rmask == 0)
            rmask = 1;
        for (gshift = 0; (gmask & 1) == 0; gmask >>= 1, ++gshift);
        if (gmask == 0)
            gmask = 1;
        for (bshift = 0; (bmask & 1) == 0; bmask >>= 1, ++bshift);
        if (bmask == 0)
            bmask = 1;
    }

    bpl = ((pBMI->bmiHeader.biBitCount * uiSrcWidth + 31) >> 3) & ~3;

    pucBits0 = (UCHAR *) pBits;
    pucBits = pucBits0;

    if (pBMI->bmiHeader.biBitCount == 24)
        pucBits240 = pucBits;
    else
    {
        int bpl24 = (uiSrcWidth * 3 + 3) & ~3;

        pucBits240 = new UCHAR[bpl24];
        if (pucBits240 == NULL)
            return E_OUTOFMEMORY;
    }
    pucBits24 = pucBits240;

    hr = S_OK;

    for (y = 0; y < (int)uiSrcHeight; ++y)
    {
        pB = pucBits;
        pB24 = pucBits24;

        switch (pBMI->bmiHeader.biBitCount)
        {
        case 1:
            for (x = uiSrcWidth; x >= 8; x -= 8)
            {
                pQ = &pRGBQ[(*pB >> 7) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 6) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 5) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 4) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 3) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 2) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 1) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB++) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            if (x > 0)
            {
                int shf = 8;

                do
                {
                    pQ = &pRGBQ[(*pB >> --shf) & 1];
                    *pB24++ = pQ->rgbBlue;
                    *pB24++ = pQ->rgbGreen;
                    *pB24++ = pQ->rgbRed;
                }
                while (--x);
            }

            break;

        case 4:
            for (x = uiSrcWidth; x >= 2; x -= 2)
            {
                pQ = &pRGBQ[(*pB >> 4) & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[*pB++ & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            if (x > 0)
            {
                pQ = &pRGBQ[(*pB >> 4) & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                if (x > 1)
                {
                    pQ = &pRGBQ[*pB & 0xf];
                    *pB24++ = pQ->rgbBlue;
                    *pB24++ = pQ->rgbGreen;
                    *pB24++ = pQ->rgbRed;
                }
            }

            break;

        case 8:
            for (x = uiSrcWidth; x--; )
            {
                pQ = &pRGBQ[*pB++];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            break;

        case 16:
        {
            USHORT *pW;

            pW = (USHORT *)pucBits;

            for (x = uiSrcWidth; x--; )
            {
                ULONG w = *pW++;

                *pB24++ = (UCHAR)
                     ((((w >> bshift) & bmask) * 255) / bmask);
                *pB24++ = (UCHAR)
                     ((((w >> gshift) & gmask) * 255) / gmask);
                *pB24++ = (UCHAR)
                     ((((w >> rshift) & rmask) * 255) / rmask);
            }

            break;
        }

        case 24:
            pucBits24 = pucBits;
            break;

        case 32:
        {
            ULONG *pD;

            pD = (ULONG *)pucBits;

            for (x = uiSrcWidth; x--; )
            {
                ULONG d = *pD++;

                *pB24++ = (UCHAR)
                     ((((d >> bshift) & bmask) * 255) / bmask);
                *pB24++ = (UCHAR)
                     ((((d >> gshift) & gmask) * 255) / gmask);
                *pB24++ = (UCHAR)
                     ((((d >> rshift) & rmask) * 255) / rmask);
            }

            break;
        }

        default:
            delete[] pucBits24;
            return E_INVALIDARG;
        }

        hr = AddScanline(pucBits24, (uiSrcHeight-1) - y);
        if (FAILED(hr))
            break;

        pucBits += bpl;
    }

    if (pucBits240 != pucBits0)
        delete[] pucBits240;

    return hr;
}

