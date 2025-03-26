/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Loading/Saving an image file with getting/setting resolution
 * COPYRIGHT:  Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <atlimage.h>

namespace GPDE = Gdiplus::DllExports;

class CImageDx
{
protected:
    HBITMAP m_hBitmap = NULL;

public:
    CImageDx()
    {
        _shared()->AddRef();
    }

    ~CImageDx()
    {
        if (m_hBitmap)
            ::DeleteObject(m_hBitmap);

        _shared()->Release();
    }

    void Attach(HBITMAP hbm)
    {
        if (m_hBitmap)
            ::DeleteObject(m_hBitmap);
        m_hBitmap = hbm;
    }

    HBITMAP Detach()
    {
        HBITMAP hbmOld = m_hBitmap;
        m_hBitmap = NULL;
        return hbmOld;
    }

    BOOL GetResolution(Gdiplus::GpImage *pImage, float *pxDpi, float *pyDpi)
    {
        if (!get_fn(_shared()->m_GetImageHorizontalResolution, "GdipGetImageHorizontalResolution") ||
            !get_fn(_shared()->m_GetImageVerticalResolution, "GdipGetImageVerticalResolution"))
        {
            return FALSE;
        }

        if (pxDpi)
            _shared()->m_GetImageHorizontalResolution(pImage, pxDpi);
        if (pyDpi)
            _shared()->m_GetImageVerticalResolution(pImage, pyDpi);

        return TRUE;
    }

    BOOL SetResolution(Gdiplus::GpBitmap *pBitmap, float xDpi, float yDpi)
    {
        if (!get_fn(_shared()->m_BitmapSetResolution, "GdipBitmapSetResolution"))
            return FALSE;

        _shared()->m_BitmapSetResolution(pBitmap, xDpi, yDpi);
        return TRUE;
    }

    HRESULT LoadDx(LPCWSTR pszFileName, float *pxDpi, float *pyDpi) throw()
    {
        using namespace Gdiplus;

        _shared()->AddRef();

        if (!get_fn(_shared()->m_CreateBitmapFromFile, "GdipCreateBitmapFromFile") ||
            !get_fn(_shared()->m_CreateHBITMAPFromBitmap, "GdipCreateHBITMAPFromBitmap") ||
            !get_fn(_shared()->m_DisposeImage, "GdipDisposeImage"))
        {
            _shared()->Release();
            return E_FAIL;
        }

        // create a GpBitmap object from file
        GpBitmap *pBitmap = NULL;
        if (_shared()->m_CreateBitmapFromFile(pszFileName, &pBitmap) != Ok)
        {
            _shared()->Release();
            return E_FAIL;
        }

        // get an HBITMAP
        HBITMAP hbm = NULL;
        Color color(0xFF, 0xFF, 0xFF);
        Status status = _shared()->m_CreateHBITMAPFromBitmap(pBitmap, &hbm, color.GetValue());

        // get the resolution
        if (pxDpi || pyDpi)
            GetResolution((GpImage*)pBitmap, pxDpi, pyDpi);

        // delete GpBitmap
        _shared()->m_DisposeImage(pBitmap);

        // attach it
        if (status == Ok)
            Attach(hbm);

        _shared()->Release();
        return (status == Ok ? S_OK : E_FAIL);
    }

    HRESULT SaveDx(LPCWSTR pszFileName, REFGUID guidFileType = GUID_NULL,
                   float xDpi = 0, float yDpi = 0) throw()
    {
        using namespace Gdiplus;

        _shared()->AddRef();

        if (!get_fn(_shared()->m_CreateBitmapFromHBITMAP, "GdipCreateBitmapFromHBITMAP") ||
            !get_fn(_shared()->m_SaveImageToFile, "GdipSaveImageToFile") ||
            !get_fn(_shared()->m_DisposeImage, "GdipDisposeImage"))
        {
            _shared()->Release();
            return E_FAIL;
        }

        // create a GpBitmap from HBITMAP
        GpBitmap *pBitmap = NULL;
        _shared()->m_CreateBitmapFromHBITMAP(m_hBitmap, NULL, &pBitmap);

        // set the resolution
        SetResolution(pBitmap, xDpi, yDpi);

        // Get encoders
        UINT cEncoders = 0;
        ImageCodecInfo* pEncoders = GetAllEncoders(cEncoders);

        // if the file type is null, get the file type from extension
        CLSID clsid;
        if (::IsEqualGUID(guidFileType, GUID_NULL))
        {
            CStringW strExt(PathFindExtensionW(pszFileName));
            clsid = FindCodecForExtension(strExt, pEncoders, cEncoders);
        }
        else
        {
            clsid = FindCodecForFileType(guidFileType, pEncoders, cEncoders);
        }

        delete[] pEncoders;

        // save to file
        Status status = _shared()->m_SaveImageToFile(pBitmap, pszFileName, &clsid, NULL);

        // destroy GpBitmap
        _shared()->m_DisposeImage(pBitmap);

        _shared()->Release();

        return (status == Ok ? S_OK : E_FAIL);
    }

    static BOOL IsExtensionSupported(PWCHAR pchDotExt)
    {
        _shared()->AddRef();

        UINT cEncoders;
        Gdiplus::ImageCodecInfo* pEncoders = GetAllEncoders(cEncoders);

        CLSID clsid = FindCodecForExtension(pchDotExt, pEncoders, cEncoders);
        BOOL ret = !::IsEqualGUID(clsid, CLSID_NULL);
        delete[] pEncoders;

        _shared()->Release();
        return ret;
    }

protected:
    using FN_Startup = decltype(&Gdiplus::GdiplusStartup);
    using FN_Shutdown = decltype(&Gdiplus::GdiplusShutdown);
    using FN_GetImageHorizontalResolution = decltype(&GPDE::GdipGetImageHorizontalResolution);
    using FN_GetImageVerticalResolution = decltype(&GPDE::GdipGetImageVerticalResolution);
    using FN_BitmapSetResolution = decltype(&GPDE::GdipBitmapSetResolution);
    using FN_CreateBitmapFromHBITMAP = decltype(&GPDE::GdipCreateBitmapFromHBITMAP);
    using FN_CreateBitmapFromFile = decltype(&GPDE::GdipCreateBitmapFromFile);
    using FN_CreateHBITMAPFromBitmap = decltype(&GPDE::GdipCreateHBITMAPFromBitmap);
    using FN_SaveImageToFile = decltype(&GPDE::GdipSaveImageToFile);
    using FN_DisposeImage = decltype(&GPDE::GdipDisposeImage);
    using FN_GetImageEncodersSize = decltype(&GPDE::GdipGetImageEncodersSize);
    using FN_GetImageEncoders = decltype(&GPDE::GdipGetImageEncoders);

    struct SHARED
    {
        HINSTANCE                       m_hGdiPlus                      = NULL;
        LONG                            m_cRefs                         = 0;
        ULONG_PTR                       m_dwToken                       = 0;
        FN_Shutdown                     m_Shutdown                      = NULL;
        FN_GetImageHorizontalResolution m_GetImageHorizontalResolution  = NULL;
        FN_GetImageVerticalResolution   m_GetImageVerticalResolution    = NULL;
        FN_BitmapSetResolution          m_BitmapSetResolution           = NULL;
        FN_CreateBitmapFromHBITMAP      m_CreateBitmapFromHBITMAP       = NULL;
        FN_CreateBitmapFromFile         m_CreateBitmapFromFile          = NULL;
        FN_CreateHBITMAPFromBitmap      m_CreateHBITMAPFromBitmap       = NULL;
        FN_SaveImageToFile              m_SaveImageToFile               = NULL;
        FN_DisposeImage                 m_DisposeImage                  = NULL;
        FN_GetImageEncodersSize         m_GetImageEncodersSize          = NULL;
        FN_GetImageEncoders             m_GetImageEncoders              = NULL;

        HINSTANCE Init()
        {
            if (m_hGdiPlus)
                return m_hGdiPlus;

            m_hGdiPlus = ::LoadLibraryW(L"gdiplus.dll");
            if (!m_hGdiPlus)
                return NULL;

            FN_Startup Startup = (FN_Startup)GetProcAddress(m_hGdiPlus, "GdiplusStartup");
            m_Shutdown = (FN_Shutdown)GetProcAddress(m_hGdiPlus, "GdiplusShutdown");
            if (!Startup || !m_Shutdown)
            {
                ::FreeLibrary(m_hGdiPlus);
                m_hGdiPlus = NULL;
                return NULL;
            }

            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Startup(&m_dwToken, &gdiplusStartupInput, NULL);

            return m_hGdiPlus;
        }

        void Free()
        {
            ::FreeLibrary(m_hGdiPlus);
            ZeroMemory(this, sizeof(*this));
        }

        LONG AddRef()
        {
            return ++m_cRefs;
        }

        LONG Release()
        {
            if (--m_cRefs == 0)
            {
                Free();
                return 0;
            }
            return m_cRefs;
        }
    };

    static SHARED* _shared()
    {
        static SHARED s_shared;
        return &s_shared;
    }

    static Gdiplus::ImageCodecInfo* GetAllEncoders(UINT& cEncoders)
    {
        Gdiplus::ImageCodecInfo *ret = NULL;
        UINT total_size;

        if (!get_fn(_shared()->m_GetImageEncodersSize, "GdipGetImageEncodersSize") ||
            !get_fn(_shared()->m_GetImageEncoders, "GdipGetImageEncoders"))
        {
            cEncoders = 0;
            return NULL;
        }

        _shared()->m_GetImageEncodersSize(&cEncoders, &total_size);
        if (total_size == 0)
            return NULL;

        ret = new Gdiplus::ImageCodecInfo[total_size / sizeof(ret[0])];
        if (ret == NULL)
        {
            cEncoders = 0;
            return NULL;
        }

        _shared()->m_GetImageEncoders(cEncoders, total_size, ret);

        return ret; // needs delete[]
    }

    template <typename FN_T>
    static bool get_fn(FN_T& fn, const char *name)
    {
        if (fn)
            return true;
        HINSTANCE hGdiPlus = _shared()->Init();
        fn = reinterpret_cast<FN_T>(::GetProcAddress(hGdiPlus, name));
        return fn != NULL;
    }

    // CImage::FindCodecForExtension is private. We have to duplicate it at here...
    static CLSID
    FindCodecForExtension(LPCWSTR dotext, const Gdiplus::ImageCodecInfo *pCodecs, UINT nCodecs)
    {
        for (UINT i = 0; i < nCodecs; ++i)
        {
            CStringW strSpecs(pCodecs[i].FilenameExtension);
            int ichOld = 0, ichSep;
            for (;;)
            {
                ichSep = strSpecs.Find(L';', ichOld);

                CStringW strSpec;
                if (ichSep < 0)
                    strSpec = strSpecs.Mid(ichOld);
                else
                    strSpec = strSpecs.Mid(ichOld, ichSep - ichOld);

                int ichDot = strSpec.ReverseFind(L'.');
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

    // CImage::FindCodecForFileType is private. We have to duplicate it at here...
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
};
