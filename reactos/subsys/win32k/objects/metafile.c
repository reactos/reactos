

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/metafile.h>

// #define NDEBUG
#include <internal/debug.h>

HENHMETAFILE  W32kCloseEnhMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
}

HMETAFILE  W32kCloseMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
}

HENHMETAFILE  W32kCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File)
{
  UNIMPLEMENTED;
}

HMETAFILE  W32kCopyMetaFile(HMETAFILE  Src,
                            LPCWSTR  File)
{
  UNIMPLEMENTED;
}

HDC  W32kCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
  UNIMPLEMENTED;
}

HDC  W32kCreateMetaFile(LPCWSTR  File)
{
  UNIMPLEMENTED;
}

BOOL  W32kDeleteEnhMetaFile(HENHMETAFILE  emf)
{
  UNIMPLEMENTED;
}

BOOL  W32kDeleteMetaFile(HMETAFILE  mf)
{
  UNIMPLEMENTED;
}

BOOL  W32kEnumEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  emf,
                          ENHMFENUMPROC  EnhMetaFunc,
                          LPVOID  Data,
                          CONST LPRECT  Rect)
{
  UNIMPLEMENTED;
}

BOOL  W32kEnumMetaFile(HDC  hDC,
                       HMETAFILE  mf,
                       MFENUMPROC  MetaFunc,
                       LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL  W32kGdiComment(HDC  hDC,
                     UINT  Size,
                     CONST LPBYTE  Data)
{
  UNIMPLEMENTED;
}

HENHMETAFILE  W32kGetEnhMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
}

UINT  W32kGetEnhMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer)
{
  UNIMPLEMENTED;
}

UINT  W32kGetEnhMetaFileDescription(HENHMETAFILE  hemf,
                                    UINT  BufSize,
                                    LPWSTR  Description)
{
  UNIMPLEMENTED;
}

UINT  W32kGetEnhMetaFileHeader(HENHMETAFILE  hemf,
                               UINT  BufSize,
                               LPENHMETAHEADER  emh)
{
  UNIMPLEMENTED;
}

UINT  W32kGetEnhMetaFilePaletteEntries(HENHMETAFILE  hemf,
                                       UINT  Entries,
                                       LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

HMETAFILE  W32kGetMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
}

UINT  W32kGetMetaFileBitsEx(HMETAFILE  hmf,
                            UINT  Size,
                            LPVOID  Data)
{
  UNIMPLEMENTED;
}

UINT  W32kGetWinMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer,
                             INT  MapMode,
                             HDC  Ref)
{
  UNIMPLEMENTED;
}

BOOL  W32kPlayEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  hemf,
                          CONST PRECT  Rect)
{
  UNIMPLEMENTED;
}

BOOL  W32kPlayEnhMetaFileRecord(HDC  hDC,
                                LPHANDLETABLE  Handletable,
                                CONST ENHMETARECORD *EnhMetaRecord,
                                UINT  Handles)
{
  UNIMPLEMENTED;
}

BOOL  W32kPlayMetaFile(HDC  hDC,
                       HMETAFILE  hmf)
{
  UNIMPLEMENTED;
}

BOOL  W32kPlayMetaFileRecord(HDC  hDC,
                             LPHANDLETABLE  Handletable,
                             LPMETARECORD  MetaRecord,
                             UINT  Handles)
{
  UNIMPLEMENTED;
}

HENHMETAFILE  W32kSetEnhMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Data)
{
  UNIMPLEMENTED;
}

HMETAFILE  W32kSetMetaFileBitsEx(UINT  Size,
                                 CONST PBYTE  Data)
{
  UNIMPLEMENTED;
}

HENHMETAFILE  W32kSetWinMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Buffer,
                                     HDC  Ref,
                                     CONST METAFILEPICT *mfp)
{
  UNIMPLEMENTED;
}


