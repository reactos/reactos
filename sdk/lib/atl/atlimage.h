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

// TODO: CImage::Load, CImage::Save
// TODO: make CImage thread-safe

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
        m_hbm = NULL;
        m_hbmOld = NULL;
        m_hDC = NULL;

        m_eOrientation = DIBOR_DEFAULT;
        m_bHasAlphaCh = false;
        m_bIsDIBSec = false;
        m_rgbTransColor = CLR_INVALID;
        ZeroMemory(&m_ds, sizeof(m_ds));

        if (GetCommon().AddRef() == 1)
        {
            GetCommon().LoadLib();
        }
    }

    ~CImage()
    {
        Destroy();
        ReleaseGDIPlus();
    }

    operator HBITMAP()
    {
        return m_hbm;
    }

public:
    void Attach(HBITMAP hBitmap, DIBOrientation eOrientation = DIBOR_DEFAULT)
    {
        AttachInternal(hBitmap, eOrientation, -1);
    }

    HBITMAP Detach() throw()
    {
        m_eOrientation = DIBOR_DEFAULT;
        m_bHasAlphaCh = false;
        m_rgbTransColor = CLR_INVALID;
        ZeroMemory(&m_ds, sizeof(m_ds));

        HBITMAP hBitmap = m_hbm;
        m_hbm = NULL;
        return hBitmap;
    }

    HDC GetDC() const throw()
    {
        if (m_hDC)
            return m_hDC;

        m_hDC = ::CreateCompatibleDC(NULL);
        m_hbmOld = ::SelectObject(m_hDC, m_hbm);
        return m_hDC;
    }

    void ReleaseDC() const throw()
    {
        ATLASSERT(m_hDC);

        if (m_hDC == NULL)
            return;

        if (m_hbmOld)
        {
            ::SelectObject(m_hDC, m_hbmOld);
            m_hbmOld = NULL;
        }
        ::DeleteDC(m_hDC);
        m_hDC = NULL;
    }

public:
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

        GetDC();
        BOOL ret = ::AlphaBlend(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                                m_hDC, xSrc, ySrc, nSrcWidth, nSrcHeight, bf);
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
        GetDC();
        BOOL ret = ::BitBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                            m_hDC, xSrc, ySrc, dwROP);
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
        if (m_hbm)
        {
            ::DeleteObject(Detach());
        }
    }

    BOOL Draw(HDC hDestDC, int xDest, int yDest, int nDestWidth, int nDestHeight,
              int xSrc, int ySrc, int nSrcWidth, int nSrcHeight) const throw()
    {
        ATLASSERT(IsTransparencySupported());
        if (m_bHasAlphaCh)
        {
            return AlphaBlend(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                              xSrc, ySrc, nSrcWidth, nSrcHeight);
        }
        else if (m_rgbTransColor != CLR_INVALID)
        {
            COLORREF rgb;
            if ((m_rgbTransColor & 0xFF000000) == 0x01000000)
                rgb = RGBFromPaletteIndex(m_rgbTransColor & 0xFF);
            else
                rgb = m_rgbTransColor;
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
        ATLASSERT(m_hbm);
        return m_bm.bmBitsPixel;
    }

    void GetColorTable(UINT iFirstColor, UINT nColors,
                       RGBQUAD* prgbColors) const throw()
    {
        ATLASSERT(IsDIBSection());
        GetDC();
        ::GetDIBColorTable(m_hDC, iFirstColor, nColors, prgbColors);
        ReleaseDC();
    }

    int GetHeight() const throw()
    {
        ATLASSERT(m_hbm);
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
        GetDC();
        COLORREF ret = ::GetPixel(m_hDC, x, y);
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
        return m_rgbTransColor;
    }

    int GetWidth() const throw()
    {
        ATLASSERT(m_hbm);
        return m_bm.bmWidth;
    }

    bool IsDIBSection() const throw()
    {
        ATLASSERT(m_hbm);
        return m_bIsDIBSec;
    }

    bool IsIndexed() const throw()
    {
        ATLASSERT(IsDIBSection());
        return GetBPP() <= 8;
    }

    bool IsNull() const throw()
    {
        return m_hbm == NULL;
    }

    HRESULT Load(LPCTSTR pszFileName) throw()
    {
        // convert the file name string into Unicode
        CStringW pszNameW(pszFileName);

        // create a GpBitmap object from file
        using namespace Gdiplus;
        GpBitmap *pBitmap = NULL;
        if (GetCommon().CreateBitmapFromFile(pszNameW, &pBitmap) != Ok)
        {
            return E_FAIL;
        }

        // get bitmap handle
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Status status = GetCommon().CreateHBITMAPFromBitmap(pBitmap, &hbm, color.GetValue());

        // delete GpBitmap
        GetCommon().DisposeImage(pBitmap);

        // attach it
        if (status == Ok)
            Attach(hbm);
        return (status == Ok ? S_OK : E_FAIL);
    }
    HRESULT Load(IStream* pStream) throw()
    {
        // create GpBitmap from stream
        using namespace Gdiplus;
        GpBitmap *pBitmap = NULL;
        if (GetCommon().CreateBitmapFromStream(pStream, &pBitmap) != Ok)
        {
            return E_FAIL;
        }

        // get bitmap handle
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Status status = GetCommon().CreateHBITMAPFromBitmap(pBitmap, &hbm, color.GetValue());

        // delete Bitmap
        GetCommon().DisposeImage(pBitmap);

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
        GetDC();
        BOOL ret = ::MaskBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                             m_hDC, xSrc, ySrc,
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
        GetDC();
        BOOL ret = ::PlgBlt(hDestDC, pPoints, m_hDC,
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

    void ReleaseGDIPlus() throw()
    {
        COMMON*& pCommon = GetCommonPtr();
        if (pCommon && pCommon->Release() == 0)
        {
            delete pCommon;
            pCommon = NULL;
        }
    }

    HRESULT Save(IStream* pStream, GUID *guidFileType) const throw()
    {
        using namespace Gdiplus;
        ATLASSERT(m_hbm);

        // Get encoders
        UINT cEncoders = 0;
        ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);

        // Get Codec
        CLSID clsid = FindCodecForFileType(*guidFileType, pEncoders, cEncoders);
        _free_mem(pEncoders);

        // create a GpBitmap from HBITMAP
        GpBitmap *pBitmap = NULL;
        GetCommon().CreateBitmapFromHBITMAP(m_hbm, NULL, &pBitmap);

        // save to stream
        Status status;
        status = GetCommon().SaveImageToStream(pBitmap, pStream, &clsid, NULL);

        // destroy GpBitmap
        GetCommon().DisposeImage(pBitmap);

        return (status == Ok ? S_OK : E_FAIL);
    }

    HRESULT Save(LPCTSTR pszFileName,
                 REFGUID guidFileType = GUID_NULL) const throw()
    {
        using namespace Gdiplus;
        ATLASSERT(m_hbm);

        // TODO & FIXME: set parameters (m_rgbTransColor etc.)

        // convert the file name string into Unicode
        CStringW pszNameW(pszFileName);

        // Get encoders
        UINT cEncoders = 0;
        ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);

        // if the file type is null, get the file type from extension
        CLSID clsid;
        if (::IsEqualGUID(guidFileType, GUID_NULL))
        {
            CString strExt = GetFileExtension(pszNameW);
            clsid = FindCodecForExtension(strExt, pEncoders, cEncoders);
        }
        else
        {
            clsid = FindCodecForFileType(guidFileType, pEncoders, cEncoders);
        }
        _free_mem(pEncoders);

        // create a GpBitmap from HBITMAP
        GpBitmap *pBitmap = NULL;
        GetCommon().CreateBitmapFromHBITMAP(m_hbm, NULL, &pBitmap);

        // save to file
        Status status = GetCommon().SaveImageToFile(pBitmap, pszNameW, &clsid, NULL);

        // destroy GpBitmap
        GetCommon().DisposeImage(pBitmap);

        return (status == Ok ? S_OK : E_FAIL);
    }

    void SetColorTable(UINT iFirstColor, UINT nColors,
                       const RGBQUAD* prgbColors) throw()
    {
        ATLASSERT(IsDIBSection());
        GetDC();
        ::SetDIBColorTable(m_hDC, iFirstColor, nColors, prgbColors);
        ReleaseDC();
    }

    void SetPixel(int x, int y, COLORREF color) throw()
    {
        GetDC();
        ::SetPixelV(m_hDC, x, y, color);
        ReleaseDC();
    }

    void SetPixelIndexed(int x, int y, int iIndex) throw()
    {
        ATLASSERT(IsIndexed());
        GetDC();
        ::SetPixelV(m_hDC, x, y, PALETTEINDEX(iIndex));
        ReleaseDC();
    }

    void SetPixelRGB(int x, int y, BYTE r, BYTE g, BYTE b) throw()
    {
        SetPixel(x, y, RGB(r, g, b));
    }

    COLORREF SetTransparentColor(COLORREF rgbTransparent) throw()
    {
        ATLASSERT(m_hbm);
        COLORREF rgbOldColor = m_rgbTransColor;
        m_rgbTransColor = rgbTransparent;
        return rgbOldColor;
    }

    BOOL StretchBlt(HDC hDestDC, int xDest, int yDest,
                    int nDestWidth, int nDestHeight,
                    int xSrc, int ySrc, int nSrcWidth, int nSrcHeight,
                    DWORD dwROP = SRCCOPY) const throw()
    {
        GetDC();
        BOOL ret = ::StretchBlt(hDestDC, xDest, yDest, nDestWidth, nDestHeight,
                                m_hDC, xSrc, ySrc, nSrcWidth, nSrcHeight, dwROP);
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
        GetDC();
        BOOL ret = ::TransparentBlt(hDestDC, xDest, yDest,
                                    nDestWidth, nDestHeight,
                                    m_hDC, xSrc, ySrc,
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

public:
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

protected:
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
            extensions.MakeLower();

            strFilter += TEXT(" (");
            strFilter += extensions;
            strFilter += TEXT(")");
            strFilter += chSeparator;

            strFilter += extensions;
            strFilter += chSeparator;

            aguidFileTypes.Add(GUID_NULL);
        }

        for (UINT i = 0; i < cCodecs; ++i)
        {
            if (ShouldExcludeFormat(pCodecs[i].FormatID, dwExclude))
                continue;

            CString extensions = pCodecs[i].FilenameExtension;
            extensions.MakeLower();

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
        UINT cDecoders = 0;
        Gdiplus::ImageCodecInfo* pDecoders = _getAllDecoders(cDecoders);
        HRESULT hr = BuildCodecFilterString(pDecoders,
                                            cDecoders,
                                            strImporters,
                                            aguidFileTypes,
                                            pszAllFilesDescription,
                                            dwExclude,
                                            chSeparator);
        _free_mem(pDecoders);
        return hr;
    }

    static HRESULT GetExporterFilterString(
        CSimpleString& strExporters,
        CSimpleArray<GUID>& aguidFileTypes,
        LPCTSTR pszAllFilesDescription = NULL,
        DWORD dwExclude = excludeDefaultSave,
        TCHAR chSeparator = TEXT('|'))
    {
        UINT cEncoders = 0;
        Gdiplus::ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);
        HRESULT hr = BuildCodecFilterString(pEncoders,
                                            cEncoders,
                                            strExporters,
                                            aguidFileTypes,
                                            pszAllFilesDescription,
                                            dwExclude,
                                            chSeparator);
        _free_mem(pEncoders);
        return hr;
    }

protected:
    // an extension of BITMAPINFO
    struct MYBITMAPINFOEX : BITMAPINFO
    {
        RGBQUAD bmiColorsExtra[256 - 1];
    };

    // abbreviations of GDI+ basic types. FIXME: Delete us
    typedef Gdiplus::GpStatus St;
    typedef Gdiplus::GpBitmap Bm;
    typedef Gdiplus::GpImage Im;

    // The common data of atlimage
    struct COMMON
    {
        // GDI+ function types
        using FUN_Startup = decltype(&Gdiplus::GdiplusStartup);
        using FUN_Shutdown = decltype(&Gdiplus::GdiplusShutdown);
        using FUN_GetImageEncoderSize = decltype(&Gdiplus::DllExports::GdipGetImageEncodersSize);
        using FUN_GetImageEncoder = decltype(&Gdiplus::DllExports::GdipGetImageEncoders);
        using FUN_GetImageDecoderSize = decltype(&Gdiplus::DllExports::GdipGetImageDecodersSize);
        using FUN_GetImageDecoder = decltype(&Gdiplus::DllExports::GdipGetImageDecoders);
        using FUN_CreateBitmapFromFile = decltype(&Gdiplus::DllExports::GdipCreateBitmapFromFile);
        using FUN_CreateHBITMAPFromBitmap = decltype(&Gdiplus::DllExports::GdipCreateHBITMAPFromBitmap);
        using FUN_CreateBitmapFromStream = decltype(&Gdiplus::DllExports::GdipCreateBitmapFromStream);
        using FUN_CreateBitmapFromHBITMAP = decltype(&Gdiplus::DllExports::GdipCreateBitmapFromHBITMAP);
        using FUN_SaveImageToStream = decltype(&Gdiplus::DllExports::GdipSaveImageToStream);
        using FUN_SaveImageToFile = decltype(&Gdiplus::DllExports::GdipSaveImageToFile);
        using FUN_DisposeImage = decltype(&Gdiplus::DllExports::GdipDisposeImage);

        // members
        int                     count = 0;
        HINSTANCE               hinstGdiPlus = NULL;
        ULONG_PTR               gdiplusToken = 0;

        // GDI+ functions
        FUN_Startup                 Startup                     = NULL;
        FUN_Shutdown                Shutdown                    = NULL;
        FUN_GetImageEncoderSize     GetImageEncodersSize        = NULL;
        FUN_GetImageEncoder         GetImageEncoders            = NULL;
        FUN_GetImageDecoderSize     GetImageDecodersSize        = NULL;
        FUN_GetImageDecoder         GetImageDecoders            = NULL;
        FUN_CreateBitmapFromFile    CreateBitmapFromFile        = NULL;
        FUN_CreateHBITMAPFromBitmap CreateHBITMAPFromBitmap     = NULL;
        FUN_CreateBitmapFromStream  CreateBitmapFromStream      = NULL;
        FUN_CreateBitmapFromHBITMAP CreateBitmapFromHBITMAP     = NULL;
        FUN_SaveImageToStream       SaveImageToStream           = NULL;
        FUN_SaveImageToFile         SaveImageToFile             = NULL;
        FUN_DisposeImage            DisposeImage                = NULL;

        COMMON()
        {
        }

        ~COMMON()
        {
            FreeLib();
        }

        ULONG AddRef()
        {
            return ++count;
        }
        ULONG Release()
        {
            return --count;
        }

        // get procedure address of the DLL
        template <typename FUN_T>
        bool _getFUN(FUN_T& fun, const char *name)
        {
            if (fun)
                return true;
            fun = reinterpret_cast<FUN_T>(::GetProcAddress(hinstGdiPlus, name));
            return fun != NULL;
        }

        HINSTANCE LoadLib()
        {
            if (hinstGdiPlus)
                return hinstGdiPlus;

            hinstGdiPlus = ::LoadLibraryA("gdiplus.dll");

            _getFUN(Startup, "GdiplusStartup");
            _getFUN(Shutdown, "GdiplusShutdown");
            _getFUN(GetImageEncodersSize, "GdipGetImageEncodersSize");
            _getFUN(GetImageEncoders, "GdipGetImageEncoders");
            _getFUN(GetImageDecodersSize, "GdipGetImageDecodersSize");
            _getFUN(GetImageDecoders, "GdipGetImageDecoders");
            _getFUN(CreateBitmapFromFile, "GdipCreateBitmapFromFile");
            _getFUN(CreateHBITMAPFromBitmap, "GdipCreateHBITMAPFromBitmap");
            _getFUN(CreateBitmapFromStream, "GdipCreateBitmapFromStream");
            _getFUN(CreateBitmapFromHBITMAP, "GdipCreateBitmapFromHBITMAP");
            _getFUN(SaveImageToStream, "GdipSaveImageToStream");
            _getFUN(SaveImageToFile, "GdipSaveImageToFile");
            _getFUN(DisposeImage, "GdipDisposeImage");

            if (hinstGdiPlus && Startup)
            {
                Gdiplus::GdiplusStartupInput gdiplusStartupInput;
                Startup(&gdiplusToken, &gdiplusStartupInput, NULL);
            }

            return hinstGdiPlus;
        }
        void FreeLib()
        {
            if (hinstGdiPlus)
            {
                Shutdown(gdiplusToken);

                Startup = NULL;
                Shutdown = NULL;
                GetImageEncodersSize = NULL;
                GetImageEncoders = NULL;
                GetImageDecodersSize = NULL;
                GetImageDecoders = NULL;
                CreateBitmapFromFile = NULL;
                CreateHBITMAPFromBitmap = NULL;
                CreateBitmapFromStream = NULL;
                CreateBitmapFromHBITMAP = NULL;
                SaveImageToStream = NULL;
                SaveImageToFile = NULL;
                DisposeImage = NULL;
                ::FreeLibrary(hinstGdiPlus);
                hinstGdiPlus = NULL;
            }
        }
    }; // struct COMMON

    static COMMON*& GetCommonPtr()
    {
        static COMMON *s_pCommon = NULL;
        return s_pCommon;
    }

    static COMMON& GetCommon()
    {
        COMMON*& pCommon = GetCommonPtr();
        if (pCommon == NULL)
            pCommon = new COMMON;
        return *pCommon;
    }

protected:
    HBITMAP             m_hbm;
    mutable HGDIOBJ     m_hbmOld;
    mutable HDC         m_hDC;
    DIBOrientation      m_eOrientation;
    bool                m_bHasAlphaCh;
    bool                m_bIsDIBSec;
    COLORREF            m_rgbTransColor;
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
            CString strSpecs = pCodecs[i].FilenameExtension;
            int i0 = 0, i1;
            for (;;)
            {
                i1 = strSpecs.Find(L';', i0);

                CString strSpec;
                if (i1 < 0)
                    strSpec = strSpecs.Mid(i0);
                else
                    strSpec = strSpecs.Mid(i0, i1 - i0);

                int ichDot = strSpec.ReverseFind(TEXT('.'));
                if (ichDot >= 0)
                    strSpec = strSpec.Mid(ichDot);

                if (!dotext || strSpec.CompareNoCase(dotext) == 0)
                    return pCodecs[i].Clsid;

                if (i1 < 0)
                    break;

                i0 = i1 + 1;
            }
        }
        return CLSID_NULL;
    }

    // Deprecated. Don't use this
    static const GUID *FileTypeFromExtension(LPCTSTR dotext)
    {
        UINT cEncoders = 0;
        Gdiplus::ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);

        for (UINT i = 0; i < cEncoders; ++i)
        {
            CString strSpecs = pEncoders[i].FilenameExtension;
            int i0 = 0, i1;
            for (;;)
            {
                i1 = strSpecs.Find(L';', i0);

                CString strSpec;
                if (i1 < 0)
                    strSpec = strSpecs.Mid(i0);
                else
                    strSpec = strSpecs.Mid(i0, i1 - i0);

                int ichDot = strSpec.ReverseFind(TEXT('.'));
                if (ichDot >= 0)
                    strSpec = strSpec.Mid(ichDot);

                if (!dotext || strSpec.CompareNoCase(dotext) == 0)
                {
                    static GUID s_guid;
                    s_guid = pEncoders[i].FormatID;
                    _free_mem(pEncoders);
                    return &s_guid;
                }

                if (i1 < 0)
                    break;

                i0 = i1 + 1;
            }
        }

        _free_mem(pEncoders);
        return NULL;
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

    // Deprecated. Don't use this
    static bool GetClsidFromFileType(CLSID *clsid, const GUID *guid)
    {
        UINT cEncoders = 0;
        Gdiplus::ImageCodecInfo* pEncoders = _getAllEncoders(cEncoders);
        *clsid = FindCodecForFileType(*guid, pEncoders, cEncoders);
        _free_mem(pEncoders);
        return true;
    }

    static Gdiplus::ImageCodecInfo* _getAllEncoders(UINT& cEncoders)
    {
        CImage image; // Initialize common

        UINT total_size = 0;
        GetCommon().GetImageEncodersSize(&cEncoders, &total_size);
        if (total_size == 0)
            return NULL;  // failure

        Gdiplus::ImageCodecInfo *ret;
        ret = reinterpret_cast<Gdiplus::ImageCodecInfo*>(_alloc_mem(total_size));
        if (ret == NULL)
        {
            cEncoders = 0;
            return NULL;  // failure
        }

        GetCommon().GetImageEncoders(cEncoders, total_size, ret);
        return ret; // needs _free_mem()
    }

    static Gdiplus::ImageCodecInfo* _getAllDecoders(UINT& cDecoders)
    {
        CImage image; // Initialize common

        UINT total_size = 0;
        GetCommon().GetImageDecodersSize(&cDecoders, &total_size);
        if (total_size == 0)
            return NULL;  // failure

        Gdiplus::ImageCodecInfo *ret;
        ret = reinterpret_cast<Gdiplus::ImageCodecInfo*>(_alloc_mem(total_size));
        if (ret == NULL)
        {
            cDecoders = 0;
            return NULL;  // failure
        }

        GetCommon().GetImageDecoders(cDecoders, total_size, ret);
        return ret; // needs _free_mem()
    }

    void AttachInternal(HBITMAP hBitmap, DIBOrientation eOrientation,
                        LONG iTransColor)
    {
        Destroy();

        const int size = sizeof(DIBSECTION);
        m_bIsDIBSec = (::GetObject(hBitmap, size, &m_ds) == size);

        bool bOK = (::GetObject(hBitmap, sizeof(BITMAP), &m_bm) != 0);

        if (bOK)
        {
            m_hbm = hBitmap;
            m_eOrientation = eOrientation;
            m_bHasAlphaCh = (m_bm.bmBitsPixel == 32);
            m_rgbTransColor = CLR_INVALID;
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
        m_bHasAlphaCh = bHasAlphaCh;

        return hbm != NULL;
    }

    static void *_alloc_mem(size_t size)
    {
        return ::HeapAlloc(::GetProcessHeap(), 0, size);
    }

    static void _free_mem(void *ptr)
    {
        ::HeapFree(::GetProcessHeap(), 0, ptr);
    }

private:
    // NOTE: CImage is not copyable
    CImage(const CImage&);
    CImage& operator=(const CImage&);
};

}

#endif

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE
