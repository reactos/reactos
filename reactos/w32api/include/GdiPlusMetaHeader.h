/*
 * GdiPlusMetaHeader.h
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

#ifndef _GDIPLUSMETAHEADER_H
#define _GDIPLUSMETAHEADER_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

typedef struct {
  DWORD iType;
  DWORD nSize;
  RECTL rclBounds;
  RECTL rclFrame;
  DWORD dSignature;
  DWORD nVersion;
  DWORD nBytes;
  DWORD nRecords;
  WORD nHandles;
  WORD sReserved;
  DWORD nDescription;
  DWORD offDescription;
  DWORD nPalEntries;
  SIZEL szlDevice;
  SIZEL szlMillimeters;
} ENHMETAHEADER3;

typedef struct {
  INT16 Left;
  INT16 Top;
  INT16 Right;
  INT16 Bottom;
} PWMFRect16;

typedef struct {
  UINT32 Key;
  INT16 Hmf;
  PWMFRect16 BoundingBox;
  INT16 Inch;
  UINT32 Reserved;
  INT16 Checksum;
} WmfPlaceableFileHeader;


class MetafileHeader
{
public:
  VOID GetBounds(Rect *rect)
  {
  }

  REAL GetDpiX(VOID)
  {
    return 0;
  }

  REAL GetDpiY(VOID)
  {
    return 0;
  }

  const ENHMETAHEADER3 *GetEmfHeader(VOID) const
  {
    return NULL;
  }

  UINT GetEmPlusFlags(VOID)
  {
    return 0;
  }

  UINT GetMetafileSize(VOID)
  {
    return 0;
  }

  MetafileType GetType(VOID)
  {
    return MetafileTypeInvalid;
  }

  UINT GetVersion(VOID)
  {
    return 0;
  }

  const METAHEADER *GetWmfHeader(VOID) const
  {
    return NULL;
  }

  BOOL IsDisplay(VOID) const
  {
    return FALSE;
  }

  BOOL IsEmf(VOID) const
  {
    return FALSE;
  }

  BOOL IsEmfOrEmfPlus(VOID) const
  {
    return FALSE;
  }

  BOOL IsEmfPlus(VOID) const
  {
    return FALSE;
  }

  BOOL IsEmfPlusDual(VOID) const
  {
    return FALSE;
  }

  BOOL IsEmfPlusOnly(VOID) const
  {
    return FALSE;
  }

  BOOL IsWmf(VOID)
  {
    return FALSE;
  }

  BOOL IsWmfPlaceable(VOID) const
  {
    return FALSE;
  }

  REAL DpiX;
  REAL DpiY;
  UINT EmfPlusFlags;
  INT EmfPlusHeaderSize;
  INT Height;
  INT LogicalDpiX;
  INT LogicalDpiY;
  MetafileType Type;
  UINT Version;
  INT Width;
  INT X;
  INT Y;

  union
  {
    METAHEADER WmfHeader;
    ENHMETAHEADER3 EmfHeader;
  };
};

#endif /* _GDIPLUSMETAHEADER_H */
