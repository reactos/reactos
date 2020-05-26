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
        GetCommon().CreateBitmapFromFile(pszNameW, &pBitmap);
        ATLASSERT(pBitmap);

        // TODO & FIXME: get parameters (m_rgbTransColor etc.)

        // get bitmap handle
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Gdiplus::Status status;
        status = GetCommon().CreateHBITMAPFromBitmap(
            pBitmap, &hbm, color.GetValue());

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
        GetCommon().CreateBitmapFromStream(pStream, &pBitmap);
        ATLASSERT(pBitmap);

        // TODO & FIXME: get parameters (m_rgbTransColor etc.)

        // get bitmap handle
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Gdiplus::Status status;
        status = GetCommon().CreateHBITMAPFromBitmap(
            pBitmap, &hbm, color.GetValue());

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

        // TODO & FIXME: set parameters (m_rgbTransColor etc.)
        CLSID clsid;
        if (!GetClsidFromFileType(&clsid, guidFileType))
            return E_FAIL;

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

        // if the file type is null, get the file type from extension
        const GUID *FileType = &guidFileType;
        if (IsGuidEqual(guidFileType, GUID_NULL))
        {
            LPCWSTR pszExt = GetFileExtension(pszNameW);
            FileType = FileTypeFromExtension(pszExt);
        }

        // get CLSID from file type
        CLSID clsid;
        if (!GetClsidFromFileType(&clsid, FileType))
            return E_FAIL;

        // create a GpBitmap from HBITMAP
        GpBitmap *pBitmap = NULL;
        GetCommon().CreateBitmapFromHBITMAP(m_hbm, NULL, &pBitmap);

        // save to file
        Status status;
        status = GetCommon().SaveImageToFile(pBitmap, pszNameW, &clsid, NULL);

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

    struct FILTER_DATA {
        DWORD dwExclude;
        const TCHAR *title;
        const TCHAR *extensions;
        const GUID *guid;
    };

protected:
    static HRESULT GetCommonFilterString(
        CSimpleString& strFilter,
        CSimpleArray<GUID>& aguidFileTypes,
        LPCTSTR pszAllFilesDescription,
        DWORD dwExclude,
        TCHAR chSeparator)
    {
        static const FILTER_DATA table[] =
        {
            {excludeBMP, TEXT("BMP"), TEXT("*.BMP;*.DIB;*.RLE"), &Gdiplus::ImageFormatBMP},
            {excludeJPEG, TEXT("JPEG"), TEXT("*.JPG;*.JPEG;*.JPE;*.JFIF"), &Gdiplus::ImageFormatJPEG},
            {excludeGIF, TEXT("GIF"), TEXT("*.GIF"), &Gdiplus::ImageFormatGIF},
            {excludeEMF, TEXT("EMF"), TEXT("*.EMF"), &Gdiplus::ImageFormatEMF},
            {excludeWMF, TEXT("WMF"), TEXT("*.WMF"), &Gdiplus::ImageFormatWMF},
            {excludeTIFF, TEXT("TIFF"), TEXT("*.TIF;*.TIFF"), &Gdiplus::ImageFormatTIFF},
            {excludePNG, TEXT("PNG"), TEXT("*.PNG"), &Gdiplus::ImageFormatPNG},
            {excludeIcon, TEXT("ICO"), TEXT("*.ICO"), &Gdiplus::ImageFormatIcon}
        };

        if (pszAllFilesDescription)
        {
            strFilter += pszAllFilesDescription;
            strFilter += chSeparator;

            BOOL bFirst = TRUE;
            for (size_t i = 0; i < _countof(table); ++i)
            {
                if ((dwExclude & table[i].dwExclude) != 0)
                    continue;

                if (bFirst)
                    bFirst = FALSE;
                else
                    strFilter += TEXT(';');

                strFilter += table[i].extensions;
            }
            strFilter += chSeparator;

            aguidFileTypes.Add(GUID_NULL);
        }

        for (size_t i = 0; i < _countof(table); ++i)
        {
            if ((dwExclude & table[i].dwExclude) != 0)
                continue;
            strFilter += table[i].title;
            strFilter += TEXT(" (");
            strFilter += table[i].extensions;
            strFilter += TEXT(")");
            strFilter += chSeparator;
            strFilter += table[i].extensions;
            strFilter += chSeparator;

            aguidFileTypes.Add(*table[i].guid);
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
        return GetCommonFilterString(strImporters,
                                     aguidFileTypes,
                                     pszAllFilesDescription,
                                     dwExclude,
                                     chSeparator);
    }

    static HRESULT GetExporterFilterString(
        CSimpleString& strExporters,
        CSimpleArray<GUID>& aguidFileTypes,
        LPCTSTR pszAllFilesDescription = NULL,
        DWORD dwExclude = excludeDefaultSave,
        TCHAR chSeparator = TEXT('|'))
    {
        return GetCommonFilterString(strExporters,
                                     aguidFileTypes,
                                     pszAllFilesDescription,
                                     dwExclude,
                                     chSeparator);
    }

protected:
    // an extension of BITMAPINFO
    struct MYBITMAPINFOEX
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[256];
        BITMAPINFO *get()
        {
            return reinterpret_cast<BITMAPINFO *>(this);
        }
        const BITMAPINFO *get() const
        {
            return reinterpret_cast<const BITMAPINFO *>(this);
        }
    };

    // The common data of atlimage
    struct COMMON
    {
        // abbreviations of GDI+ basic types
        typedef Gdiplus::GpStatus St;
        typedef Gdiplus::ImageCodecInfo ICI;
        typedef Gdiplus::GpBitmap Bm;
        typedef Gdiplus::EncoderParameters EncParams;
        typedef Gdiplus::GpImage Im;
        typedef Gdiplus::ARGB ARGB;
        typedef HBITMAP HBM;
        typedef Gdiplus::GdiplusStartupInput GSI;
        typedef Gdiplus::GdiplusStartupOutput GSO;

        // GDI+ function types
#undef API
#undef CST
#define API WINGDIPAPI
#define CST GDIPCONST
        typedef St (WINAPI *STARTUP)(ULONG_PTR *, const GSI *, GSO *);
        typedef void (WINAPI *SHUTDOWN)(ULONG_PTR);
        typedef St (API *GETIMAGEENCODERSSIZE)(UINT *, UINT *);
        typedef St (API *GETIMAGEENCODERS)(UINT, UINT, ICI *);
        typedef St (API *CREATEBITMAPFROMFILE)(CST WCHAR*, Bm **);
        typedef St (API *CREATEHBITMAPFROMBITMAP)(Bm *, HBM *, ARGB);
        typedef St (API *CREATEBITMAPFROMSTREAM)(IStream *, Bm **);
        typedef St (API *CREATEBITMAPFROMHBITMAP)(HBM, HPALETTE, Bm **);
        typedef St (API *SAVEIMAGETOSTREAM)(Im *, IStream *, CST CLSID *,
                                            CST EncParams *);
        typedef St (API *SAVEIMAGETOFILE)(Im *, CST WCHAR *, CST CLSID *,
                                          CST EncParams *);
        typedef St (API *DISPOSEIMAGE)(Im*);
#undef API
#undef CST

        // members
        int                     count;
        HINSTANCE               hinstGdiPlus;
        ULONG_PTR               gdiplusToken;

        // GDI+ functions
        STARTUP                 Startup;
        SHUTDOWN                Shutdown;
        GETIMAGEENCODERSSIZE    GetImageEncodersSize;
        GETIMAGEENCODERS        GetImageEncoders;
        CREATEBITMAPFROMFILE    CreateBitmapFromFile;
        CREATEHBITMAPFROMBITMAP CreateHBITMAPFromBitmap;
        CREATEBITMAPFROMSTREAM  CreateBitmapFromStream;
        CREATEBITMAPFROMHBITMAP CreateBitmapFromHBITMAP;
        SAVEIMAGETOSTREAM       SaveImageToStream;
        SAVEIMAGETOFILE         SaveImageToFile;
        DISPOSEIMAGE            DisposeImage;

        COMMON()
        {
            count = 0;
            hinstGdiPlus = NULL;
            Startup = NULL;
            Shutdown = NULL;
            GetImageEncodersSize = NULL;
            GetImageEncoders = NULL;
            CreateBitmapFromFile = NULL;
            CreateHBITMAPFromBitmap = NULL;
            CreateBitmapFromStream = NULL;
            CreateBitmapFromHBITMAP = NULL;
            SaveImageToStream = NULL;
            SaveImageToFile = NULL;
            DisposeImage = NULL;
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
        template <typename TYPE>
        TYPE AddrOf(const char *name)
        {
            FARPROC proc = ::GetProcAddress(hinstGdiPlus, name);
            return reinterpret_cast<TYPE>(proc);
        }

        HINSTANCE LoadLib()
        {
            if (hinstGdiPlus)
                return hinstGdiPlus;

            hinstGdiPlus = ::LoadLibraryA("gdiplus.dll");

            // get procedure addresses from the DLL
            Startup = AddrOf<STARTUP>("GdiplusStartup");
            Shutdown = AddrOf<SHUTDOWN>("GdiplusShutdown");
            GetImageEncodersSize =
                AddrOf<GETIMAGEENCODERSSIZE>("GdipGetImageEncodersSize");
            GetImageEncoders = AddrOf<GETIMAGEENCODERS>("GdipGetImageEncoders");
            CreateBitmapFromFile =
                AddrOf<CREATEBITMAPFROMFILE>("GdipCreateBitmapFromFile");
            CreateHBITMAPFromBitmap =
                AddrOf<CREATEHBITMAPFROMBITMAP>("GdipCreateHBITMAPFromBitmap");
            CreateBitmapFromStream =
                AddrOf<CREATEBITMAPFROMSTREAM>("GdipCreateBitmapFromStream");
            CreateBitmapFromHBITMAP =
                AddrOf<CREATEBITMAPFROMHBITMAP>("GdipCreateBitmapFromHBITMAP");
            SaveImageToStream =
                AddrOf<SAVEIMAGETOSTREAM>("GdipSaveImageToStream");
            SaveImageToFile = AddrOf<SAVEIMAGETOFILE>("GdipSaveImageToFile");
            DisposeImage = AddrOf<DISPOSEIMAGE>("GdipDisposeImage");

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

    struct EXTENSION_ENTRY
    {
        LPCWSTR pszExt;
        GUID guid;
    };

    const GUID *FileTypeFromExtension(LPCWSTR pszExt) const
    {
        static const EXTENSION_ENTRY table[] =
        {
            {L".jpg", Gdiplus::ImageFormatJPEG},
            {L".png", Gdiplus::ImageFormatPNG},
            {L".bmp", Gdiplus::ImageFormatBMP},
            {L".gif", Gdiplus::ImageFormatGIF},
            {L".tif", Gdiplus::ImageFormatTIFF},
            {L".jpeg", Gdiplus::ImageFormatJPEG},
            {L".jpe", Gdiplus::ImageFormatJPEG},
            {L".jfif", Gdiplus::ImageFormatJPEG},
            {L".dib", Gdiplus::ImageFormatBMP},
            {L".rle", Gdiplus::ImageFormatBMP},
            {L".tiff", Gdiplus::ImageFormatTIFF}
        };
        const size_t count = _countof(table);
        for (size_t i = 0; i < count; ++i)
        {
            if (::lstrcmpiW(table[i].pszExt, pszExt) == 0)
                return &table[i].guid;
        }
        return NULL;
    }

    struct FORMAT_ENTRY
    {
        GUID guid;
        LPCWSTR mime;
    };

    bool GetClsidFromFileType(CLSID *clsid, const GUID *guid) const
    {
        static const FORMAT_ENTRY table[] =
        {
            {Gdiplus::ImageFormatJPEG, L"image/jpeg"},
            {Gdiplus::ImageFormatPNG, L"image/png"},
            {Gdiplus::ImageFormatBMP, L"image/bmp"},
            {Gdiplus::ImageFormatGIF, L"image/gif"},
            {Gdiplus::ImageFormatTIFF, L"image/tiff"}
        };
        const size_t count = _countof(table);
        for (size_t i = 0; i < count; ++i)
        {
            if (IsGuidEqual(table[i].guid, *guid))
            {
                int num = GetEncoderClsid(table[i].mime, clsid);
                if (num >= 0)
                {
                    return true;
                }
            }
        }
        return false;
    }

    int GetEncoderClsid(LPCWSTR mime, CLSID *clsid) const
    {
        UINT count = 0, total_size = 0;
        GetCommon().GetImageEncodersSize(&count, &total_size);
        if (total_size == 0)
            return -1;  // failure

        Gdiplus::ImageCodecInfo *pInfo;
        BYTE *pb = new BYTE[total_size];
        ATLASSERT(pb);
        pInfo = reinterpret_cast<Gdiplus::ImageCodecInfo *>(pb);
        if (pInfo == NULL)
            return -1;  // failure

        GetCommon().GetImageEncoders(count, total_size, pInfo);

        for (UINT iInfo = 0; iInfo < count; ++iInfo)
        {
            if (::lstrcmpiW(pInfo[iInfo].MimeType, mime) == 0)
            {
                *clsid = pInfo[iInfo].Clsid;
                delete[] pb;
                return iInfo;  // success
            }
        }

        delete[] pb;
        return -1;  // failure
    }

    bool IsGuidEqual(const GUID& guid1, const GUID& guid2) const
    {
        RPC_STATUS status;
        if (::UuidEqual(const_cast<GUID *>(&guid1),
                        const_cast<GUID *>(&guid2), &status))
        {
            if (status == RPC_S_OK)
                return true;
        }
        return false;
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
        LPVOID pvBits;
        HBITMAP hbm = ::CreateDIBSection(hDC, bi.get(), DIB_RGB_COLORS,
                                         &pvBits, NULL, 0);
        ATLASSERT(hbm);
        ::DeleteDC(hDC);

        // attach it
        AttachInternal(hbm, eOrientation, -1);
        m_bHasAlphaCh = bHasAlphaCh;

        return hbm != NULL;
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
