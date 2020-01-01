/*
 * GdiPlusMetaFile.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSMETAFILE_H
#define _GDIPLUSMETAFILE_H

class Metafile : public Image
{
  public:
    Metafile(HDC referenceHdc, EmfType type = EmfTypeEmfPlusDual, const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus =
            DllExports::GdipRecordMetafile(referenceHdc, type, NULL, MetafileFrameUnitGdi, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(const WCHAR *filename)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipCreateMetafileFromFile(filename, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        HDC referenceHdc,
        const RectF &frameRect,
        MetafileFrameUnit frameUnit = MetafileFrameUnitGdi,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafile(referenceHdc, type, &frameRect, frameUnit, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(HMETAFILE hWmf, const WmfPlaceableFileHeader *wmfPlaceableFileHeader, BOOL deleteWmf = FALSE)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipCreateMetafileFromWmf(hWmf, deleteWmf, wmfPlaceableFileHeader, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        const WCHAR *fileName,
        HDC referenceHdc,
        const Rect &frameRect,
        MetafileFrameUnit frameUnit = MetafileFrameUnitGdi,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafileFileNameI(
            fileName, referenceHdc, type, &frameRect, frameUnit, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        IStream *stream,
        HDC referenceHdc,
        const RectF &frameRect,
        MetafileFrameUnit frameUnit = MetafileFrameUnitGdi,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafile(referenceHdc, type, &frameRect, frameUnit, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(IStream *stream, HDC referenceHdc, EmfType type = EmfTypeEmfPlusDual, const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafileStream(
            stream, referenceHdc, type, NULL, MetafileFrameUnitGdi, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        IStream *stream,
        HDC referenceHdc,
        const Rect &frameRect,
        MetafileFrameUnit frameUnit = MetafileFrameUnitGdi,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafileI(referenceHdc, type, &frameRect, frameUnit, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        const WCHAR *fileName,
        HDC referenceHdc,
        const RectF &frameRect,
        MetafileFrameUnit frameUnit = MetafileFrameUnitGdi,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafileFileName(
            fileName, referenceHdc, type, &frameRect, frameUnit, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        const WCHAR *fileName,
        HDC referenceHdc,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafileFileName(
            fileName, referenceHdc, type, NULL, MetafileFrameUnitGdi, description, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(IStream *stream)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipCreateMetafileFromStream(stream, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(HENHMETAFILE hEmf, BOOL deleteEmf = FALSE)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipCreateMetafileFromEmf(hEmf, deleteEmf, &metafile);
        SetNativeImage(metafile);
    }

    Metafile(
        HDC referenceHdc,
        const Rect &frameRect,
        MetafileFrameUnit frameUnit = MetafileFrameUnitGdi,
        EmfType type = EmfTypeEmfPlusDual,
        const WCHAR *description = NULL)
    {
        GpMetafile *metafile = NULL;
        lastStatus = DllExports::GdipRecordMetafileI(referenceHdc, type, &frameRect, frameUnit, description, &metafile);
        SetNativeImage(metafile);
    }

    static UINT
    EmfToWmfBits(
        HENHMETAFILE hemf,
        UINT cbData16,
        LPBYTE pData16,
        INT iMapMode = MM_ANISOTROPIC,
        EmfToWmfBitsFlags eFlags = EmfToWmfBitsFlagsDefault)
    {
        return DllExports::GdipEmfToWmfBits(hemf, cbData16, pData16, iMapMode, eFlags);
    }

    UINT
    GetDownLevelRasterizationLimit() const
    {
#if 1
        return 0; // FIXME
#else
        UINT metafileRasterizationLimitDpi = 0;
        SetStatus(DllExports::GdipGetMetafileDownLevelRasterizationLimit(
            GetNativeMetafile(), &metafileRasterizationLimitDpi));
        return metafileRasterizationLimitDpi;
#endif
    }

    HENHMETAFILE
    GetHENHMETAFILE() const
    {
        HENHMETAFILE hEmf;
        SetStatus(DllExports::GdipGetHemfFromMetafile(GetNativeMetafile(), &hEmf));
        return hEmf;
    }

    static Status
    GetMetafileHeader(const WCHAR *filename, MetafileHeader *header)
    {
        return DllExports::GdipGetMetafileHeaderFromFile(filename, header);
    }

    static Status
    GetMetafileHeader(HENHMETAFILE hEmf, MetafileHeader *header)
    {
        return DllExports::GdipGetMetafileHeaderFromEmf(hEmf, header);
    }

    static Status
    GetMetafileHeader(HMETAFILE hWmf, const WmfPlaceableFileHeader *wmfPlaceableFileHeader, MetafileHeader *header)
    {
        return DllExports::GdipGetMetafileHeaderFromWmf(hWmf, wmfPlaceableFileHeader, header);
    }

    Status
    GetMetafileHeader(MetafileHeader *header) const
    {
        return SetStatus(DllExports::GdipGetMetafileHeaderFromMetafile(GetNativeMetafile(), header));
    }

    static Status
    GetMetafileHeader(IStream *stream, MetafileHeader *header)
    {
        return DllExports::GdipGetMetafileHeaderFromStream(stream, header);
    }

    Status
    PlayRecord(EmfPlusRecordType recordType, UINT flags, UINT dataSize, const BYTE *data)
    {
        return SetStatus(DllExports::GdipPlayMetafileRecord(GetNativeMetafile(), recordType, flags, dataSize, data));
    }

    Status
    SetDownLevelRasterizationLimit(UINT metafileRasterizationLimitDpi)
    {
        return SetStatus(
            DllExports::GdipSetMetafileDownLevelRasterizationLimit(GetNativeMetafile(), metafileRasterizationLimitDpi));
    }

  protected:
    GpMetafile *
    GetNativeMetafile() const
    {
        return static_cast<GpMetafile *>(nativeImage);
    }

    // get native
    friend inline GpMetafile *&
    getNat(const Metafile *metafile)
    {
        return reinterpret_cast<GpMetafile *&>(const_cast<Metafile *>(metafile)->nativeImage);
    }
};

#endif /* _GDIPLUSMETAFILE_H */
