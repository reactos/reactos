

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
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
                                  LPCTSTR  File)
{
  UNIMPLEMENTED;
}

HMETAFILE  W32kCopyMetaFile(HMETAFILE  Src,
                            LPCTSTR  File)
{
  UNIMPLEMENTED;
}

HDC  W32kCreateEnhMetaFile(HDC  hDCRef,
                           LPCTSTR  File,
                           CONST LPRECT  Rect,
                           LPCTSTR  Description)
{
  UNIMPLEMENTED;
}

HDC  W32kCreateMetaFile(LPCTSTR  File)
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

HENHMETAFILE  W32kGetEnhMetaFile(LPCTSTR  MetaFile)
{
  UNIMPLEMENTED;
}


