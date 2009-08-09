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
  Metafile(HDC referenceHdc, EmfType type, const WCHAR *description)
  {
  }

  Metafile(const WCHAR *filename)
  {
  }

  Metafile(HDC referenceHdc, const RectF &frameRect, MetafileFrameUnit frameUnit, EmfType type, const WCHAR *description)
  {
  }

  Metafile(HMETAFILE hWmf, const WmfPlaceableFileHeader *wmfPlaceableFileHeader, BOOL deleteWmf)
  {
  }

  Metafile(const WCHAR *fileName, HDC referenceHdc, const Rect &frameRect, MetafileFrameUnit frameUnit, EmfType type, const WCHAR *description)
  {
  }

  Metafile(IStream *stream, HDC referenceHdc, const RectF &frameRect, MetafileFrameUnit frameUnit, EmfType type, const WCHAR *description)
  {
  }

  Metafile(IStream *stream, HDC referenceHdc, EmfType type, const WCHAR *description)
  {
  }

  Metafile(IStream *stream, HDC referenceHdc, const Rect &frameRect, MetafileFrameUnit frameUnit, EmfType type, const WCHAR *description)
  {
  }

  Metafile(const WCHAR *fileName, HDC referenceHdc, const RectF &frameRect, MetafileFrameUnit frameUnit, EmfType type, const WCHAR *description)
  {
  }

  Metafile(const WCHAR *fileName, HDC referenceHdc, EmfType type, const WCHAR *description)
  {
  }

  Metafile(IStream *stream)
  {
  }

  Metafile(HENHMETAFILE hEmf, BOOL deleteEmf)
  {
  }

  Metafile(HDC referenceHdc, const Rect &frameRect, MetafileFrameUnit frameUnit, EmfType type, const WCHAR *description)
  {
  }

  static UINT EmfToWmfBits(HENHMETAFILE hemf, UINT cbData16, LPBYTE pData16, INT iMapMode, EmfToWmfBitsFlags eFlags)
  {
    return 0;
  }

  UINT GetDownLevelRasterizationLimit(VOID)
  {
    return 0;
  }

  HENHMETAFILE GetHENHMETAFILE(VOID)
  {
    return NULL;
  }

  static Status GetMetafileHeader(const WCHAR *filename, MetafileHeader *header)
  {
    return NotImplemented;
  }

  static Status GetMetafileHeader(HENHMETAFILE *hEmf, MetafileHeader *header)
  {
    return NotImplemented;
  }

  static Status GetMetafileHeader(HMETAFILE hWmf, const WmfPlaceableFileHeader *wmfPlaceableFileHeader, MetafileHeader *header)
  {
    return NotImplemented;
  }

  Status GetMetafileHeader(MetafileHeader *header) const
  {
    return NotImplemented;
  }

  static Status GetMetafileHeader(IStream *stream, MetafileHeader *header)
  {
    return NotImplemented;
  }

  Status PlayRecord(EmfPlusRecordType recordType, UINT flags, UINT dataSize, const BYTE *data)
  {
    return NotImplemented;
  }

  Status SetDownLevelRasterizationLimit(UINT metafileRasterizationLimitDpi)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSMETAFILE_H */
