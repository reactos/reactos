

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/metafile.h>

// #define NDEBUG
#include <win32k/debug1.h>

HENHMETAFILE
STDCALL
W32kCloseEnhMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
W32kCloseMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
W32kCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
W32kCopyMetaFile(HMETAFILE  Src,
                            LPCWSTR  File)
{
  UNIMPLEMENTED;
}

HDC
STDCALL
W32kCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
  UNIMPLEMENTED;
}

HDC
STDCALL
W32kCreateMetaFile(LPCWSTR  File)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kDeleteEnhMetaFile(HENHMETAFILE  emf)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kDeleteMetaFile(HMETAFILE  mf)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kEnumEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  emf,
                          ENHMFENUMPROC  EnhMetaFunc,
                          LPVOID  Data,
                          CONST LPRECT  Rect)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kEnumMetaFile(HDC  hDC,
                       HMETAFILE  mf,
                       MFENUMPROC  MetaFunc,
                       LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGdiComment(HDC  hDC,
                     UINT  Size,
                     CONST LPBYTE  Data)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
W32kGetEnhMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetEnhMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetEnhMetaFileDescription(HENHMETAFILE  hemf,
                                    UINT  BufSize,
                                    LPWSTR  Description)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetEnhMetaFileHeader(HENHMETAFILE  hemf,
                               UINT  BufSize,
                               LPENHMETAHEADER  emh)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetEnhMetaFilePaletteEntries(HENHMETAFILE  hemf,
                                       UINT  Entries,
                                       LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
W32kGetMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetMetaFileBitsEx(HMETAFILE  hmf,
                            UINT  Size,
                            LPVOID  Data)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetWinMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer,
                             INT  MapMode,
                             HDC  Ref)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPlayEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  hemf,
                          CONST PRECT  Rect)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPlayEnhMetaFileRecord(HDC  hDC,
                                LPHANDLETABLE  Handletable,
                                CONST ENHMETARECORD *EnhMetaRecord,
                                UINT  Handles)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPlayMetaFile(HDC  hDC,
                       HMETAFILE  hmf)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPlayMetaFileRecord(HDC  hDC,
                             LPHANDLETABLE  Handletable,
                             LPMETARECORD  MetaRecord,
                             UINT  Handles)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
W32kSetEnhMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Data)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
W32kSetMetaFileBitsEx(UINT  Size,
                                 CONST PBYTE  Data)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
W32kSetWinMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Buffer,
                                     HDC  Ref,
//                                     CONST METAFILEPICT *mfp)
				     PVOID mfp)
{
  UNIMPLEMENTED;
}


