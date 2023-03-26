/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Loading/Saving an image file with getting/setting resolution
 * COPYRIGHT:  Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CImageDx : public CImage
{
public:
    CImageDx() : CImage()
    {
        GetImageHorizontalResolution = NULL;
        GetImageVerticalResolution = NULL;
        BitmapSetResolution = NULL;
    }

    BOOL GetResolution(Gdiplus::GpImage *pImage, float *pxDpi, float *pyDpi)
    {
        *pxDpi = 96;
        *pyDpi = 96;

        if (GetImageHorizontalResolution == NULL || GetImageVerticalResolution == NULL)
        {
            GetImageHorizontalResolution =
                AddrOf<GETIMAGEHORIZONTALRESOLUTION>("GdipGetImageHorizontalResolution");
            GetImageVerticalResolution =
                AddrOf<GETIMAGEVERTICALRESOLUTION>("GdipGetImageVerticalResolution");
        }

        if (GetImageHorizontalResolution == NULL || GetImageVerticalResolution == NULL)
            return FALSE;

        GetImageHorizontalResolution(pImage, pxDpi);
        GetImageVerticalResolution(pImage, pyDpi);
        return TRUE;
    }

    BOOL SetResolution(Gdiplus::GpBitmap *pBitmap, float xDpi, float yDpi) const
    {
        if (BitmapSetResolution == NULL)
            BitmapSetResolution = AddrOf<BITMAPSETRESOLUTION>("GdipBitmapSetResolution");

        if (BitmapSetResolution == NULL)
            return FALSE;

        BitmapSetResolution(pBitmap, xDpi, yDpi);
        return TRUE;
    }

    HRESULT LoadDx(LPCTSTR pszFileName, float *pxDpi, float *pyDpi) throw()
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
        Gdiplus::Status status;
        status = GetCommon().CreateHBITMAPFromBitmap(pBitmap, &hbm, color.GetValue());

        // get the resolution
        GetResolution((Gdiplus::GpImage*)pBitmap, pxDpi, pyDpi);

        // delete GpBitmap
        GetCommon().DisposeImage(pBitmap);

        // attach it
        if (status == Ok)
            Attach(hbm);
        return (status == Ok ? S_OK : E_FAIL);
    }

    HRESULT SaveDx(LPCTSTR pszFileName, REFGUID guidFileType = GUID_NULL,
                   float xDpi = 0, float yDpi = 0) const throw()
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

        // set the resolution
        SetResolution(pBitmap, xDpi, yDpi);

        // save to file
        Status status;
        status = GetCommon().SaveImageToFile(pBitmap, pszNameW, &clsid, NULL);

        // destroy GpBitmap
        GetCommon().DisposeImage(pBitmap);

        return (status == Ok ? S_OK : E_FAIL);
    }

protected:
    // get procedure address of the DLL
    template <typename TYPE>
    TYPE AddrOf(const char *name) const
    {
        FARPROC proc = ::GetProcAddress(GetCommon().hinstGdiPlus, name);
        return reinterpret_cast<TYPE>(proc);
    }

    typedef St (WINGDIPAPI *GETIMAGEHORIZONTALRESOLUTION)(Im *, float*);
    typedef St (WINGDIPAPI *GETIMAGEVERTICALRESOLUTION)(Im *, float*);
    typedef St (WINGDIPAPI *BITMAPSETRESOLUTION)(Bm *, float, float);

    GETIMAGEHORIZONTALRESOLUTION    GetImageHorizontalResolution;
    GETIMAGEVERTICALRESOLUTION      GetImageVerticalResolution;
    mutable BITMAPSETRESOLUTION     BitmapSetResolution;
};
