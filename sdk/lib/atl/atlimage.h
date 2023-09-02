// PROJECT:        ReactOS ATL CImage
// LICENSE:        Public Domain
// PURPOSE:        Provides compatibility to Microsoft ATL
// PROGRAMMERS:    Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)

#ifndef __ATLIMAGE_H__
#define __ATLIMAGE_H__

// !!!!
// TODO: The backend (gdi+) that this class relies on is not yet complete!
//       Before that is finished, this class will not be a perfect replacement.
//       See rostest/apitests/atl/CImage_WIP.txt for test results.
// !!!!

#pragma once

#include <atlcore.h>        // for ATL Core
#include <atlstr.h>         // for CAtlStringMgr
#include <atlsimpstr.h>     // for CSimpleString
#include <atlsimpcoll.h>    // for CSimpleArray

#include <wingdi.h>
#include <cguid.h>          // for GUID_NULL
#include <gdiplus.h>        // GDI+

namespace ATL
{

class CImage
{
public:
    // flags for CImage::Create/CreateEx
    enum
    {
        createAlphaChannel = 1      // enable alpha
    };

    // orientation of DIB
    enum DIBOrientation
    {
        DIBOR_DEFAULT,              // default
        DIBOR_BOTTOMUP,             // bottom-up DIB
        DIBOR_TOPDOWN               // top-down DIB
    };

    CImage() throw()
    {
        m_hBitmap = NULL;
        m_hOldBitmap = NULL;
        m_hDC = NULL;

        m_eOrientation = DIBOR_DEFAULT;
        m_bHasAlphaChannel = false;
        m_bIsDIBSection = false;
        m_clrTransparentColor = CLR_INVALID;
        ZeroMemory(&m_ds, sizeof(m_ds));

        s_gdiplus.IncreaseCImageCount();
    }

    virtual ~CImage() throw()
    {
        Destroy();
        s_gdiplus.DecreaseCImageCount();
    }

    operator HBITMAP() throw()
    {
        return m_hBitmap;
    }

    static void ReleaseGDIPlus()
    {
        s_gdiplus.ReleaseGDIPlus();
    }

    void Attach(HBITMAP hBitmap, DIBOrientation eOrientation = DIBOR_DEFAULT) throw()
    {
        AttachInternal(hBitmap, eOrientation, -1);
    }

    HBITMAP Detach() throw()
    {
        m_eOrientation = DIBOR_DEFAULT;
        m_bHasAlphaChannel = false;
        m_clrTransparentColor = CLR_INVALID;
        ZeroMemory(&m_ds, sizeof(m_ds));

        HBITMAP hBitmap = m_hBitmap;
        m_hBitmap = NULL;
        return hBitmap;
    }

    HDC GetDC() const throw()
    {
        ATLASSERT(m_nDCRefCount >= 0);

        if (::InterlockedIncrement(&m_nDCRefCount) == 1)
        {
            ATLASSERT(m_hDC == NULL);
            ATLASSERT(m_hOldBitmap == NULL);
            m_hDC = ::CreateCompatibleDC(NULL);
            m_hOldBitmap = (HBITMAP)::SelectObject(m_hDC, m_hBitmap);
        }

        return m_hDC;
    }

    void ReleaseDC() const throw()
    {
        ATLASSERT(m_nDCRefCount > 0);

        if (::InterlockedDecrement(&m_nDCRefCount) > 0)
            return;

        ATLASSERT(m_hDC != NULL);
        ATLASSERT(m_hOldBitmap != NULL);

        if (m_hOldBitmap)
        {
            ::SelectObject(m_hDC, m_hOldBitmap);
            m_hOldBitmap = NULL;
        }

        if (m_hDC)
        {
            ::DeleteDC(m_hDC);
            m_hDC = NULL;
        }
    }

    BOOL AlphaBlend(HDC hDestDC,
        int xDest, int yDest, int nDestWidth, int nDestHeight,
        int xSrc, int ySrc, int nSrcWidth, int nSrcHeight,
        BYTE bSrcAlpha = 0xFF, BYTE bBlendOp = AC_SRC_OVER) const
    {
        ATLASSERT(IsTransparencySupported());

        BLENDFUNCTION bf;
        bf.BlendOp = bBlendOp;
        bf.BlendFlags = 0;
        bf.SourceConstantAlpha = bSrcAlpha;
        bf.AlphaFormat = AC_SRC_ALPHA;

        HDC hSrcDC = GetDC();
        BOOL ret = ::AlphaBlend(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                                hSrcDC, xSrc, ySrc, nSrcWidth, nSrcHeight, bf);
        ReleaseDC();
        return ret;
    }
    BOOL AlphaBlend(HDC hDestDC, int xDest, int yDest,
                    BYTE bSrcAlpha = 0xFF, BYTE bBlendOp = AC_SRC_OVER) const
    {
        int width = GetWidth();
        int height = GetHeight();
        return AlphaBlend(hDestDC, xDest, yDest, width, height, 0, 0,
                          width, height, bSrcAlpha, bBlendOp);
    }
    BOOL AlphaBlend(HDC hDestDC, const POINT& pointDest,
                    BYTE bSrcAlpha = 0xFF, BYTE bBlendOp = AC_SRC_OVER) const
    {
        return AlphaBlend(hDestDC, pointDest.x, pointDest.y, bSrcAlpha, bBlendOp);
    }
    BOOL AlphaBlend(HDC hDestDC, const RECT& rectDest, const RECT& rectSrc,
                    BYTE bSrcAlpha = 0xFF, BYTE bBlendOp = AC_SRC_OVER) const
    {
        return AlphaBlend(hDestDC, rectDest.left, rectDest.top,
                          rectDest.right - rectDest.left,
                          rectDest.bottom - rectDest.top,
                          rectSrc.left, rectSrc.top,
                          rectSrc.right - rectSrc.left,
                          rectSrc.bottom - rectSrc.top,
                          bSrcAlpha, bBlendOp);
    }

    BOOL BitBlt(HDC hDestDC, int xDest, int yDest,
                int nDestWidth, int nDestHeight,
                int xSrc, int ySrc, DWORD dwROP = SRCCOPY) const throw()
    {
        HDC hSrcDC = GetDC();
        BOOL ret = ::BitBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                            hSrcDC, xSrc, ySrc, dwROP);
        ReleaseDC();
        return ret;
    }
    BOOL BitBlt(HDC hDestDC, int xDest, int yDest,
                DWORD dwROP = SRCCOPY) const throw()
    {
        return BitBlt(hDestDC, xDest, yDest,
                      GetWidth(), GetHeight(), 0, 0, dwROP);
    }
    BOOL BitBlt(HDC hDestDC, const POINT& pointDest,
                DWORD dwROP = SRCCOPY) const throw()
    {
        return BitBlt(hDestDC, pointDest.x, pointDest.y, dwROP);
    }
    BOOL BitBlt(HDC hDestDC, const RECT& rectDest, const POINT& pointSrc,
                DWORD dwROP = SRCCOPY) const throw()
    {
        return BitBlt(hDestDC, rectDest.left, rectDest.top,
                      rectDest.right - rectDest.left,
                      rectDest.bottom - rectDest.top,
                      pointSrc.x, pointSrc.y, dwROP);
    }

    BOOL Create(int nWidth, int nHeight, int nBPP, DWORD dwFlags = 0) throw()
    {
        return CreateEx(nWidth, nHeight, nBPP, BI_RGB, NULL, dwFlags);
    }

    BOOL CreateEx(int nWidth, int nHeight, int nBPP, DWORD eCompression,
                  const DWORD* pdwBitmasks = NULL, DWORD dwFlags = 0) throw()
    {
        return CreateInternal(nWidth, nHeight, nBPP, eCompression, pdwBitmasks, dwFlags);
    }

    void Destroy() throw()
    {
        if (m_hBitmap)
        {
            ::DeleteObject(Detach());
        }
    }

    BOOL Draw(HDC hDestDC, int xDest, int yDest, int nDestWidth, int nDestHeight,
              int xSrc, int ySrc, int nSrcWidth, int nSrcHeight) const throw()
    {
        ATLASSERT(IsTransparencySupported());
        if (m_bHasAlphaChannel)
        {
            return AlphaBlend(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                              xSrc, ySrc, nSrcWidth, nSrcHeight);
        }
        else if (m_clrTransparentColor != CLR_INVALID)
        {
            COLORREF rgb;
            if ((m_clrTransparentColor & 0xFF000000) == 0x01000000)
                rgb = RGBFromPaletteIndex(m_clrTransparentColor & 0xFF);
            else
                rgb = m_clrTransparentColor;
            return TransparentBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                                  xSrc, ySrc, nSrcWidth, nSrcHeight, rgb);
        }
        else
        {
            return StretchBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                              xSrc, ySrc, nSrcWidth, nSrcHeight);
        }
    }
    BOOL Draw(HDC hDestDC, const RECT& rectDest, const RECT& rectSrc) const throw()
    {
        return Draw(hDestDC, rectDest.left, rectDest.top,
                    rectDest.right - rectDest.left,
                    rectDest.bottom - rectDest.top,
                    rectSrc.left, rectSrc.top,
                    rectSrc.right - rectSrc.left,
                    rectSrc.bottom - rectSrc.top);
    }
    BOOL Draw(HDC hDestDC, int xDest, int yDest) const throw()
    {
        return Draw(hDestDC, xDest, yDest, GetWidth(), GetHeight());
    }
    BOOL Draw(HDC hDestDC, const POINT& pointDest) const throw()
    {
        return Draw(hDestDC, pointDest.x, pointDest.y);
    }
    BOOL Draw(HDC hDestDC, int xDest, int yDest,
              int nDestWidth, int nDestHeight) const throw()
    {
        return Draw(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                    0, 0, GetWidth(), GetHeight());
    }
    BOOL Draw(HDC hDestDC, const RECT& rectDest) const throw()
    {
        return Draw(hDestDC, rectDest.left, rectDest.top,
                    rectDest.right - rectDest.left,
                    rectDest.bottom - rectDest.top);
    }

    void *GetBits() throw()
    {
        ATLASSERT(IsDIBSection());
        BYTE *pb = (BYTE *)m_bm.bmBits;
        if (m_eOrientation == DIBOR_BOTTOMUP)
        {
            pb += m_bm.bmWidthBytes * (m_bm.bmHeight - 1);
        }
        return pb;
    }

    int GetBPP() const throw()
    {
        ATLASSERT(m_hBitmap);
        return m_bm.bmBitsPixel;
    }

    void GetColorTable(UINT iFirstColor, UINT nColors,
                       RGBQUAD* prgbColors) const throw()
    {
        ATLASSERT(IsDIBSection());
        HDC hSrcDC = GetDC();
        ::GetDIBColorTable(hSrcDC, iFirstColor, nColors, prgbColors);
        ReleaseDC();
    }

    int GetHeight() const throw()
    {
        ATLASSERT(m_hBitmap);
        return m_bm.bmHeight;
    }

    int GetMaxColorTableEntries() const throw()
    {
        ATLASSERT(IsDIBSection());
        if (m_ds.dsBmih.biClrUsed && m_ds.dsBmih.biBitCount < 16)
            return m_ds.dsBmih.biClrUsed;
        switch (m_bm.bmBitsPixel)
        {
            case 1:     return 2;
            case 4:     return 16;
            case 8:     return 256;
            case 16: case 32:
                if (m_ds.dsBmih.biCompression == BI_BITFIELDS)
                    return 3;
                return 0;
            case 24:
            default:
                return 0;
        }
    }

    int GetPitch() const throw()
    {
        ATLASSERT(IsDIBSection());
        if (m_eOrientation == DIBOR_BOTTOMUP)
            return -m_bm.bmWidthBytes;
        else
            return m_bm.bmWidthBytes;
    }

    COLORREF GetPixel(int x, int y) const throw()
    {
        HDC hSrcDC = GetDC();
        COLORREF ret = ::GetPixel(hSrcDC, x, y);
        ReleaseDC();
        return ret;
    }

    void* GetPixelAddress(int x, int y) throw()
    {
        ATLASSERT(IsDIBSection());
        BYTE *pb = (BYTE *)GetBits();
        pb += GetPitch() * y;
        pb += (GetBPP() * x) / 8;
        return pb;
    }

    COLORREF GetTransparentColor() const throw()
    {
        return m_clrTransparentColor;
    }

    int GetWidth() const throw()
    {
        ATLASSERT(m_hBitmap);
        return m_bm.bmWidth;
    }

    bool IsDIBSection() const throw()
    {
        ATLASSERT(m_hBitmap);
        return m_bIsDIBSection;
    }

    bool IsIndexed() const throw()
    {
        ATLASSERT(IsDIBSection());
        return GetBPP() <= 8;
    }

    bool IsNull() const throw()
    {
        return m_hBitmap == NULL;
    }

    HRESULT Load(LPCTSTR pszFileName) throw()
    {
        if (!InitGDIPlus())
            return E_FAIL;

        // convert the file name string into Unicode
        CStringW pszNameW(pszFileName);

        // create a GpBitmap object from file
        using namespace Gdiplus;
        GpBitmap *pBitmap = NULL;
        if (s_gdiplus.CreateBitmapFromFile(pszNameW, &pBitmap) != Ok)
        {
            return E_FAIL;
        }

        // get bitmap handle
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Status status = s_gdiplus.CreateHBITMAPFromBitmap(pBitmap, &hbm, color.GetValue());

        // delete GpBitmap
        s_gdiplus.DisposeImage(pBitmap);

        // attach it
        if (status == Ok)
            Attach(hbm);
        return (status == Ok ? S_OK : E_FAIL);
    }
    HRESULT Load(IStream* pStream) throw()
    {
        if (!InitGDIPlus())
            return E_FAIL;

        // create GpBitmap from stream
        using namespace Gdiplus;
        GpBitmap *pBitmap = NULL;
        if (s_gdiplus.CreateBitmapFromStream(pStream, &pBitmap) != Ok)
        {
            return E_FAIL;
        }

        // get bitmap handle
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Status status = s_gdiplus.CreateHBITMAPFromBitmap(pBitmap, &hbm, color.GetValue());

        // delete Bitmap
        s_gdiplus.DisposeImage(pBitmap);

        // attach it
        if (status == Ok)
            Attach(hbm);
        return (status == Ok ? S_OK : E_FAIL);
    }

    // NOTE: LoadFromResource loads BITMAP resource only
    void LoadFromResource(HINSTANCE hInstance, LPCTSTR pszResourceName) throw()
    {
        HANDLE hHandle = ::LoadImage(hInstance, pszResourceName,
                                     IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        Attach(reinterpret_cast<HBITMAP>(hHandle));
    }
    void LoadFromResource(HINSTANCE hInstance, UINT nIDResource) throw()
    {
        LoadFromResource(hInstance, MAKEINTRESOURCE(nIDResource));
    }

    BOOL MaskBlt(HDC hDestDC, int xDest, int yDest,
                 int nDestWidth, int nDestHeight, int xSrc, int ySrc,
                 HBITMAP hbmMask, int xMask, int yMask,
                 DWORD dwROP = SRCCOPY) const throw()
    {
        ATLASSERT(IsTransparencySupported());
        HDC hSrcDC = GetDC();
        BOOL ret = ::MaskBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                             hSrcDC, xSrc, ySrc,
                             hbmMask, xMask, yMask, dwROP);
        ReleaseDC();
        return ret;
    }
    BOOL MaskBlt(HDC hDestDC, const RECT& rectDest, const POINT& pointSrc,
                 HBITMAP hbmMask, const POINT& pointMask,
                 DWORD dwROP = SRCCOPY) const throw()
    {
        return MaskBlt(hDestDC, rectDest.left, rectDest.top,
            rectDest.right - rectDest.left, rectDest.bottom - rectDest.top,
            pointSrc.x, pointSrc.y, hbmMask, pointMask.x, pointMask.y, dwROP);
    }
    BOOL MaskBlt(HDC hDestDC, int xDest, int yDest,
                 HBITMAP hbmMask, DWORD dwROP = SRCCOPY) const throw()
    {
        return MaskBlt(hDestDC, xDest, yDest, GetWidth(), GetHeight(),
                       0, 0, hbmMask, 0, 0, dwROP);
    }
    BOOL MaskBlt(HDC hDestDC, const POINT& pointDest,
                 HBITMAP hbmMask, DWORD dwROP = SRCCOPY) const throw()
    {
        return MaskBlt(hDestDC, pointDest.x, pointDest.y, hbmMask, dwROP);
    }

    BOOL PlgBlt(HDC hDestDC, const POINT* pPoints,
                int xSrc, int ySrc, int nSrcWidth, int nSrcHeight,
                HBITMAP hbmMask = NULL,
                int xMask = 0, int yMask = 0) const throw()
    {
        ATLASSERT(IsTransparencySupported());
        HDC hSrcDC = GetDC();
        BOOL ret = ::PlgBlt(hDestDC, pPoints, hSrcDC,
                            xSrc, ySrc, nSrcWidth, nSrcHeight,
                            hbmMask, xMask, yMask);
        ReleaseDC();
        return ret;
    }
    BOOL PlgBlt(HDC hDestDC, const POINT* pPoints,
                HBITMAP hbmMask = NULL) const throw()
    {
        return PlgBlt(hDestDC, pPoints, 0, 0, GetWidth(), GetHeight(),
                      hbmMask);
    }
    BOOL PlgBlt(HDC hDestDC, const POINT* pPoints, const RECT& rectSrc,
                HBITMAP hbmMask, const POINT& pointMask) const throw()
    {
        return PlgBlt(hDestDC, pPoints, rectSrc.left, rectSrc.top,
            rectSrc.right - rectSrc.left, rectSrc.bottom - rectSrc.top,
            hbmMask, pointMask.x, pointMask.y);
    }
    BOOL PlgBlt(HDC hDestDC, const POINT* pPoints, const RECT& rectSrc,
                HBITMAP hbmMask = NULL) const throw()
    {
        POINT pointMask = {0, 0};
        return PlgBlt(hDestDC, pPoints, rectSrc, hbmMask, pointMask);
    }

    HRESULT Save(IStream* pStream, GUID *guidFileType) const throw()
    {
        if (!InitGDIPlus())
            return E_FAIL;

        using namespace Gdiplus;
        ATLASSERT(m_hBitmap);

        // Get encoders
        UINT cEncoders = 0;
        ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);

        // Get Codec
        CLSID clsid = FindCodecForFileType(*guidFileType, pEncoders, cEncoders);
        delete[] pEncoders;

        // create a GpBitmap from HBITMAP
        GpBitmap *pBitmap = NULL;
        s_gdiplus.CreateBitmapFromHBITMAP(m_hBitmap, NULL, &pBitmap);

        // save to stream
        Status status;
        status = s_gdiplus.SaveImageToStream(pBitmap, pStream, &clsid, NULL);

        // destroy GpBitmap
        s_gdiplus.DisposeImage(pBitmap);

        return (status == Ok ? S_OK : E_FAIL);
    }

    HRESULT Save(LPCTSTR pszFileName,
                 REFGUID guidFileType = GUID_NULL) const throw()
    {
        if (!InitGDIPlus())
            return E_FAIL;

        using namespace Gdiplus;
        ATLASSERT(m_hBitmap);

        // convert the file name string into Unicode
        CStringW pszNameW(pszFileName);

        // Get encoders
        UINT cEncoders = 0;
        ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);

        // if the file type is null, get the file type from extension
        CLSID clsid;
        if (::IsEqualGUID(guidFileType, GUID_NULL))
        {
            CString strExt(GetFileExtension(pszNameW));
            clsid = FindCodecForExtension(strExt, pEncoders, cEncoders);
        }
        else
        {
            clsid = FindCodecForFileType(guidFileType, pEncoders, cEncoders);
        }
        delete[] pEncoders;

        // create a GpBitmap from HBITMAP
        GpBitmap *pBitmap = NULL;
        s_gdiplus.CreateBitmapFromHBITMAP(m_hBitmap, NULL, &pBitmap);

        // save to file
        Status status = s_gdiplus.SaveImageToFile(pBitmap, pszNameW, &clsid, NULL);

        // destroy GpBitmap
        s_gdiplus.DisposeImage(pBitmap);

        return (status == Ok ? S_OK : E_FAIL);
    }

    void SetColorTable(UINT iFirstColor, UINT nColors,
                       const RGBQUAD* prgbColors) throw()
    {
        ATLASSERT(IsDIBSection());
        HDC hSrcDC = GetDC();
        ::SetDIBColorTable(hSrcDC, iFirstColor, nColors, prgbColors);
        ReleaseDC();
    }

    void SetPixel(int x, int y, COLORREF color) throw()
    {
        HDC hSrcDC = GetDC();
        ::SetPixelV(hSrcDC, x, y, color);
        ReleaseDC();
    }

    void SetPixelIndexed(int x, int y, int iIndex) throw()
    {
        ATLASSERT(IsIndexed());
        HDC hSrcDC = GetDC();
        ::SetPixelV(hSrcDC, x, y, PALETTEINDEX(iIndex));
        ReleaseDC();
    }

    void SetPixelRGB(int x, int y, BYTE r, BYTE g, BYTE b) throw()
    {
        SetPixel(x, y, RGB(r, g, b));
    }

    COLORREF SetTransparentColor(COLORREF rgbTransparent) throw()
    {
        ATLASSERT(m_hBitmap);
        COLORREF rgbOldColor = m_clrTransparentColor;
        m_clrTransparentColor = rgbTransparent;
        return rgbOldColor;
    }

    BOOL StretchBlt(HDC hDestDC, int xDest, int yDest,
                    int nDestWidth, int nDestHeight,
                    int xSrc, int ySrc, int nSrcWidth, int nSrcHeight,
                    DWORD dwROP = SRCCOPY) const throw()
    {
        HDC hSrcDC = GetDC();
        BOOL ret = ::StretchBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                                hSrcDC, xSrc, ySrc, nSrcWidth, nSrcHeight, dwROP);
        ReleaseDC();
        return ret;
    }
    BOOL StretchBlt(HDC hDestDC, int xDest, int yDest,
                    int nDestWidth, int nDestHeight,
                    DWORD dwROP = SRCCOPY) const throw()
    {
        return StretchBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                          0, 0, GetWidth(), GetHeight(), dwROP);
    }
    BOOL StretchBlt(HDC hDestDC, const RECT& rectDest,
                    DWORD dwROP = SRCCOPY) const throw()
    {
        return StretchBlt(hDestDC, rectDest.left, rectDest.top,
                          rectDest.right - rectDest.left,
                          rectDest.bottom - rectDest.top, dwROP);
    }
    BOOL StretchBlt(HDC hDestDC, const RECT& rectDest,
                    const RECT& rectSrc, DWORD dwROP = SRCCOPY) const throw()
    {
        return StretchBlt(hDestDC, rectDest.left, rectDest.top,
                          rectDest.right - rectDest.left,
                          rectDest.bottom - rectDest.top,
                          rectSrc.left, rectSrc.top,
                          rectSrc.right - rectSrc.left,
                          rectSrc.bottom - rectSrc.top, dwROP);
    }

    BOOL TransparentBlt(HDC hDestDC, int xDest, int yDest,
                        int nDestWidth, int nDestHeight,
                        int xSrc, int ySrc, int nSrcWidth, int nSrcHeight,
                        UINT crTransparent = CLR_INVALID) const throw()
    {
        ATLASSERT(IsTransparencySupported());
        HDC hSrcDC = GetDC();
        BOOL ret = ::TransparentBlt(hDestDC, xDest, yDest,
                                    nDestWidth, nDestHeight,
                                    hSrcDC, xSrc, ySrc,
                                    nSrcWidth, nSrcHeight, crTransparent);
        ReleaseDC();
        return ret;
    }
    BOOL TransparentBlt(HDC hDestDC, int xDest, int yDest,
                        int nDestWidth, int nDestHeight,
                        UINT crTransparent = CLR_INVALID) const throw()
    {
        return TransparentBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                              0, 0, GetWidth(), GetHeight(), crTransparent);
    }
    BOOL TransparentBlt(HDC hDestDC, const RECT& rectDest,
                        UINT crTransparent = CLR_INVALID) const throw()
    {
        return TransparentBlt(hDestDC, rectDest.left, rectDest.top,
                              rectDest.right - rectDest.left,
                              rectDest.bottom - rectDest.top, crTransparent);
    }
    BOOL TransparentBlt(
       HDC hDestDC, const RECT& rectDest,
       const RECT& rectSrc, UINT crTransparent = CLR_INVALID) const throw()
    {
        return TransparentBlt(hDestDC, rectDest.left, rectDest.top,
            rectDest.right - rectDest.left, rectDest.bottom - rectDest.left,
            rectSrc.left, rectSrc.top, rectSrc.right - rectSrc.left,
            rectSrc.bottom - rectSrc.top, crTransparent);
    }

    static BOOL IsTransparencySupported() throw()
    {
        return TRUE;
    }

    enum ExcludeFlags
    {
        excludeGIF          = 0x01,
        excludeBMP          = 0x02,
        excludeEMF          = 0x04,
        excludeWMF          = 0x08,
        excludeJPEG         = 0x10,
        excludePNG          = 0x20,
        excludeTIFF         = 0x40,
        excludeIcon         = 0x80,
        excludeOther        = 0x80000000,
        excludeDefaultLoad  = 0,
        excludeDefaultSave  = excludeIcon | excludeEMF | excludeWMF
    };

private:
    static bool ShouldExcludeFormat(REFGUID guidFileType, DWORD dwExclude)
    {
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatGIF))
            return !!(dwExclude & excludeGIF);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatBMP))
            return !!(dwExclude & excludeBMP);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatEMF))
            return !!(dwExclude & excludeEMF);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatWMF))
            return !!(dwExclude & excludeWMF);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatJPEG))
            return !!(dwExclude & excludeJPEG);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatPNG))
            return !!(dwExclude & excludePNG);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatTIFF))
            return !!(dwExclude & excludeTIFF);
        if (::IsEqualGUID(guidFileType, Gdiplus::ImageFormatIcon))
            return !!(dwExclude & excludeIcon);
        return ((dwExclude & excludeOther) == excludeOther);
    }

    static HRESULT BuildCodecFilterString(
        const Gdiplus::ImageCodecInfo* pCodecs,
        UINT cCodecs,
        CSimpleString& strFilter,
        CSimpleArray<GUID>& aguidFileTypes,
        LPCTSTR pszAllFilesDescription,
        DWORD dwExclude,
        TCHAR chSeparator)
    {
        if (!pCodecs || !cCodecs)
        {
            strFilter += chSeparator;
            return E_FAIL;
        }

        if (pszAllFilesDescription)
        {
            strFilter += pszAllFilesDescription;

            BOOL bFirst = TRUE;
            CString extensions;
            for (UINT i = 0; i < cCodecs; ++i)
            {
                if (ShouldExcludeFormat(pCodecs[i].FormatID, dwExclude))
                    continue;

                if (bFirst)
                    bFirst = FALSE;
                else
                    extensions += TEXT(';');

                CString ext(pCodecs[i].FilenameExtension);
                extensions += ext;
            }

            strFilter += chSeparator;
            strFilter += extensions;
            strFilter += chSeparator;

            aguidFileTypes.Add(GUID_NULL);
        }

        for (UINT i = 0; i < cCodecs; ++i)
        {
            if (ShouldExcludeFormat(pCodecs[i].FormatID, dwExclude))
                continue;

            CString extensions(pCodecs[i].FilenameExtension);

            CString desc(pCodecs[i].FormatDescription);
            strFilter += desc;
            strFilter += TEXT(" (");
            strFilter += extensions;
            strFilter += TEXT(")");
            strFilter += chSeparator;
            strFilter += extensions;
            strFilter += chSeparator;

            aguidFileTypes.Add(pCodecs[i].FormatID);
        }

        strFilter += chSeparator;
        return S_OK;
    }

public:
    static HRESULT GetImporterFilterString(
        CSimpleString& strImporters,
        CSimpleArray<GUID>& aguidFileTypes,
        LPCTSTR pszAllFilesDescription = NULL,
        DWORD dwExclude = excludeDefaultLoad,
        TCHAR chSeparator = TEXT('|'))
    {
        if (!InitGDIPlus())
            return E_FAIL;

        UINT cDecoders = 0;
        Gdiplus::ImageCodecInfo* pDecoders = _getAllDecoders(cDecoders);
        HRESULT hr = BuildCodecFilterString(pDecoders,
                                            cDecoders,
                                            strImporters,
                                            aguidFileTypes,
                                            pszAllFilesDescription,
                                            dwExclude,
                                            chSeparator);
        delete[] pDecoders;
        return hr;
    }

    static HRESULT GetExporterFilterString(
        CSimpleString& strExporters,
        CSimpleArray<GUID>& aguidFileTypes,
        LPCTSTR pszAllFilesDescription = NULL,
        DWORD dwExclude = excludeDefaultSave,
        TCHAR chSeparator = TEXT('|'))
    {
        if (!InitGDIPlus())
            return E_FAIL;

        UINT cEncoders = 0;
        Gdiplus::ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);
        HRESULT hr = BuildCodecFilterString(pEncoders,
                                            cEncoders,
                                            strExporters,
                                            aguidFileTypes,
                                            pszAllFilesDescription,
                                            dwExclude,
                                            chSeparator);
        delete[] pEncoders;
        return hr;
    }

private:
    // an extension of BITMAPINFO
    struct MYBITMAPINFOEX : BITMAPINFO
    {
        RGBQUAD bmiColorsExtra[256 - 1];
    };

    class CInitGDIPlus
    {
    private:
        // GDI+ function types
        using FUN_Startup = decltype(&Gdiplus::GdiplusStartup);
        using FUN_Shutdown = decltype(&Gdiplus::GdiplusShutdown);
        using FUN_CreateBitmapFromFile = decltype(&Gdiplus::DllExports::GdipCreateBitmapFromFile);
        using FUN_CreateBitmapFromHBITMAP = decltype(&Gdiplus::DllExports::GdipCreateBitmapFromHBITMAP);
        using FUN_CreateBitmapFromStream = decltype(&Gdiplus::DllExports::GdipCreateBitmapFromStream);
        using FUN_CreateHBITMAPFromBitmap = decltype(&Gdiplus::DllExports::GdipCreateHBITMAPFromBitmap);
        using FUN_DisposeImage = decltype(&Gdiplus::DllExports::GdipDisposeImage);
        using FUN_GetImageDecoder = decltype(&Gdiplus::DllExports::GdipGetImageDecoders);
        using FUN_GetImageDecoderSize = decltype(&Gdiplus::DllExports::GdipGetImageDecodersSize);
        using FUN_GetImageEncoder = decltype(&Gdiplus::DllExports::GdipGetImageEncoders);
        using FUN_GetImageEncoderSize = decltype(&Gdiplus::DllExports::GdipGetImageEncodersSize);
        using FUN_SaveImageToFile = decltype(&Gdiplus::DllExports::GdipSaveImageToFile);
        using FUN_SaveImageToStream = decltype(&Gdiplus::DllExports::GdipSaveImageToStream);

        // members
        HINSTANCE m_hInst;
        ULONG_PTR m_dwToken;
        CRITICAL_SECTION m_sect;
        LONG m_nCImageObjects;
        DWORD m_dwLastError;

        void _clear_funs() throw()
        {
            Startup = NULL;
            Shutdown = NULL;
            CreateBitmapFromFile = NULL;
            CreateBitmapFromHBITMAP = NULL;
            CreateBitmapFromStream = NULL;
            CreateHBITMAPFromBitmap = NULL;
            DisposeImage = NULL;
            GetImageDecoders = NULL;
            GetImageDecodersSize = NULL;
            GetImageEncoders = NULL;
            GetImageEncodersSize = NULL;
            SaveImageToFile = NULL;
            SaveImageToStream = NULL;
        }

        template <typename T_FUN>
        T_FUN _get_fun(T_FUN& fun, LPCSTR name) throw()
        {
            if (!fun)
                fun = reinterpret_cast<T_FUN>(::GetProcAddress(m_hInst, name));
            return fun;
        }

    public:
        // GDI+ functions
        FUN_Startup                 Startup;
        FUN_Shutdown                Shutdown;
        FUN_CreateBitmapFromFile    CreateBitmapFromFile;
        FUN_CreateBitmapFromHBITMAP CreateBitmapFromHBITMAP;
        FUN_CreateBitmapFromStream  CreateBitmapFromStream;
        FUN_CreateHBITMAPFromBitmap CreateHBITMAPFromBitmap;
        FUN_DisposeImage            DisposeImage;
        FUN_GetImageDecoder         GetImageDecoders;
        FUN_GetImageDecoderSize     GetImageDecodersSize;
        FUN_GetImageEncoder         GetImageEncoders;
        FUN_GetImageEncoderSize     GetImageEncodersSize;
        FUN_SaveImageToFile         SaveImageToFile;
        FUN_SaveImageToStream       SaveImageToStream;

        CInitGDIPlus() throw()
            : m_hInst(NULL)
            , m_dwToken(0)
            , m_nCImageObjects(0)
            , m_dwLastError(ERROR_SUCCESS)
        {
            _clear_funs();
            ::InitializeCriticalSection(&m_sect);
        }

        ~CInitGDIPlus() throw()
        {
            ReleaseGDIPlus();
            ::DeleteCriticalSection(&m_sect);
        }

        bool Init() throw()
        {
            ::EnterCriticalSection(&m_sect);

            if (m_dwToken == 0)
            {
                if (!m_hInst)
                    m_hInst = ::LoadLibrary(TEXT("gdiplus.dll"));

                if (m_hInst &&
                    _get_fun(Startup, "GdiplusStartup") &&
                    _get_fun(Shutdown, "GdiplusShutdown") &&
                    _get_fun(CreateBitmapFromFile, "GdipCreateBitmapFromFile") &&
                    _get_fun(CreateBitmapFromHBITMAP, "GdipCreateBitmapFromHBITMAP") &&
                    _get_fun(CreateBitmapFromStream, "GdipCreateBitmapFromStream") &&
                    _get_fun(CreateHBITMAPFromBitmap, "GdipCreateHBITMAPFromBitmap") &&
                    _get_fun(DisposeImage, "GdipDisposeImage") &&
                    _get_fun(GetImageDecoders, "GdipGetImageDecoders") &&
                    _get_fun(GetImageDecodersSize, "GdipGetImageDecodersSize") &&
                    _get_fun(GetImageEncoders, "GdipGetImageEncoders") &&
                    _get_fun(GetImageEncodersSize, "GdipGetImageEncodersSize") &&
                    _get_fun(SaveImageToFile, "GdipSaveImageToFile") &&
                    _get_fun(SaveImageToStream, "GdipSaveImageToStream"))
                {
                    using namespace Gdiplus;
                    GdiplusStartupInput input;
                    GdiplusStartupOutput output;
                    Startup(&m_dwToken, &input, &output);
                }
            }

            bool ret = (m_dwToken != 0);
            ::LeaveCriticalSection(&m_sect);

            return ret;
        }

        void ReleaseGDIPlus() throw()
        {
            ::EnterCriticalSection(&m_sect);
            if (m_dwToken)
            {
                if (Shutdown)
                    Shutdown(m_dwToken);

                m_dwToken = 0;
            }
            if (m_hInst)
            {
                ::FreeLibrary(m_hInst);
                m_hInst = NULL;
            }
            _clear_funs();
            ::LeaveCriticalSection(&m_sect);
        }

        void IncreaseCImageCount() throw()
        {
            ::EnterCriticalSection(&m_sect);
            ++m_nCImageObjects;
            ::LeaveCriticalSection(&m_sect);
        }

        void DecreaseCImageCount() throw()
        {
            ::EnterCriticalSection(&m_sect);
            if (--m_nCImageObjects == 0)
            {
                ReleaseGDIPlus();
            }
            ::LeaveCriticalSection(&m_sect);
        }
    };

    static CInitGDIPlus s_gdiplus;

    static bool InitGDIPlus() throw()
    {
        return s_gdiplus.Init();
    }

private:
    HBITMAP             m_hBitmap;
    mutable HBITMAP     m_hOldBitmap;
    mutable HDC         m_hDC;
    mutable LONG        m_nDCRefCount = 0;
    DIBOrientation      m_eOrientation;
    bool                m_bHasAlphaChannel;
    bool                m_bIsDIBSection;
    COLORREF            m_clrTransparentColor;
    union
    {
        BITMAP          m_bm;
        DIBSECTION      m_ds;
    };

    LPCWSTR GetFileExtension(LPCWSTR pszFileName) const
    {
        LPCWSTR pch = wcsrchr(pszFileName, L'\\');
        if (pch == NULL)
            pch = wcsrchr(pszFileName, L'/');
        pch = (pch ? wcsrchr(pch, L'.') : wcsrchr(pszFileName, L'.'));
        return (pch ? pch : (pszFileName + ::lstrlenW(pszFileName)));
    }

    COLORREF RGBFromPaletteIndex(int iIndex) const
    {
        RGBQUAD table[256];
        GetColorTable(0, 256, table);
        RGBQUAD& quad =  table[iIndex];
        return RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
    }

    static CLSID
    FindCodecForExtension(LPCTSTR dotext, const Gdiplus::ImageCodecInfo *pCodecs, UINT nCodecs)
    {
        for (UINT i = 0; i < nCodecs; ++i)
        {
            CString strSpecs(pCodecs[i].FilenameExtension);
            int ichOld = 0, ichSep;
            for (;;)
            {
                ichSep = strSpecs.Find(TEXT(';'), ichOld);

                CString strSpec;
                if (ichSep < 0)
                    strSpec = strSpecs.Mid(ichOld);
                else
                    strSpec = strSpecs.Mid(ichOld, ichSep - ichOld);

                int ichDot = strSpec.ReverseFind(TEXT('.'));
                if (ichDot >= 0)
                    strSpec = strSpec.Mid(ichDot);

                if (!dotext || strSpec.CompareNoCase(dotext) == 0)
                    return pCodecs[i].Clsid;

                if (ichSep < 0)
                    break;

                ichOld = ichSep + 1;
            }
        }
        return CLSID_NULL;
    }

    static CLSID
    FindCodecForFileType(REFGUID guidFileType, const Gdiplus::ImageCodecInfo *pCodecs, UINT nCodecs)
    {
        for (UINT iInfo = 0; iInfo < nCodecs; ++iInfo)
        {
            if (::IsEqualGUID(pCodecs[iInfo].FormatID, guidFileType))
                return pCodecs[iInfo].Clsid;
        }
        return CLSID_NULL;
    }

    static Gdiplus::ImageCodecInfo* _getAllEncoders(UINT& cEncoders)
    {
        UINT total_size = 0;
        s_gdiplus.GetImageEncodersSize(&cEncoders, &total_size);
        if (total_size == 0)
            return NULL;  // failure

        Gdiplus::ImageCodecInfo *ret;
        ret = new Gdiplus::ImageCodecInfo[total_size / sizeof(ret[0])];
        if (ret == NULL)
        {
            cEncoders = 0;
            return NULL;  // failure
        }

        s_gdiplus.GetImageEncoders(cEncoders, total_size, ret);
        return ret; // needs delete[]
    }

    static Gdiplus::ImageCodecInfo* _getAllDecoders(UINT& cDecoders)
    {
        UINT total_size = 0;
        s_gdiplus.GetImageDecodersSize(&cDecoders, &total_size);
        if (total_size == 0)
            return NULL;  // failure

        Gdiplus::ImageCodecInfo *ret;
        ret = new Gdiplus::ImageCodecInfo[total_size / sizeof(ret[0])];
        if (ret == NULL)
        {
            cDecoders = 0;
            return NULL;  // failure
        }

        s_gdiplus.GetImageDecoders(cDecoders, total_size, ret);
        return ret; // needs delete[]
    }

    void AttachInternal(HBITMAP hBitmap, DIBOrientation eOrientation,
                        LONG iTransColor) throw()
    {
        Destroy();

        const int size = sizeof(DIBSECTION);
        m_bIsDIBSection = (::GetObject(hBitmap, size, &m_ds) == size);

        bool bOK = (::GetObject(hBitmap, sizeof(BITMAP), &m_bm) != 0);

        if (bOK)
        {
            m_hBitmap = hBitmap;
            m_eOrientation = eOrientation;
            m_bHasAlphaChannel = (m_bm.bmBitsPixel == 32);
            m_clrTransparentColor = CLR_INVALID;
        }
    }

    BOOL CreateInternal(int nWidth, int nHeight, int nBPP,
                        DWORD eCompression, const DWORD* pdwBitmasks = NULL,
                        DWORD dwFlags = 0) throw()
    {
        ATLASSERT(nWidth != 0);
        ATLASSERT(nHeight != 0);

        // initialize BITMAPINFO extension
        MYBITMAPINFOEX bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = nWidth;
        bi.bmiHeader.biHeight = nHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = nBPP;
        bi.bmiHeader.biCompression = eCompression;

        // is there alpha channel?
        bool bHasAlphaCh;
        bHasAlphaCh = (nBPP == 32 && (dwFlags & createAlphaChannel));

        // get orientation
        DIBOrientation eOrientation;
        eOrientation = ((nHeight > 0) ? DIBOR_BOTTOMUP : DIBOR_TOPDOWN);

        // does it have bit fields?
        if (eCompression == BI_BITFIELDS)
        {
            if (nBPP == 16 || nBPP == 32)
            {
                // store the mask data
                LPDWORD pdwMask = reinterpret_cast<LPDWORD>(bi.bmiColors);
                pdwMask[0] = pdwBitmasks[0];
                pdwMask[1] = pdwBitmasks[1];
                pdwMask[2] = pdwBitmasks[2];
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            ATLASSERT(pdwBitmasks == NULL);
            if (pdwBitmasks)
                return FALSE;
        }

        // create a DIB section
        HDC hDC = ::CreateCompatibleDC(NULL);
        ATLASSERT(hDC);
        HBITMAP hbm = ::CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, NULL, NULL, 0);
        ATLASSERT(hbm);
        ::DeleteDC(hDC);

        // attach it
        AttachInternal(hbm, eOrientation, -1);
        m_bHasAlphaChannel = bHasAlphaCh;

        return hbm != NULL;
    }

private:
    CImage(const CImage&) = delete;
    CImage& operator=(const CImage&) = delete;
};

DECLSPEC_SELECTANY CImage::CInitGDIPlus CImage::s_gdiplus;

class CImageDC
{
private:
    const CImage& m_image;
    HDC m_hDC;

public:
    CImageDC(const CImage& image)
        : m_image(image)
        , m_hDC(image.GetDC())
    {
    }

    virtual ~CImageDC() throw()
    {
        m_image.ReleaseDC();
        m_hDC = NULL;
    }

    operator HDC() const throw()
    {
        return m_hDC;
    }

private:
    CImageDC(const CImageDC&) = delete;
    CImageDC& operator=(const CImageDC&) = delete;
};

} // namespace ATL

#endif

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE
