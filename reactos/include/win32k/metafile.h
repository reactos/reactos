
#ifndef __WIN32K_METAFILE_H
#define __WIN32K_METAFILE_H

HENHMETAFILE  W32kCloseEnhMetaFile(HDC  hDC);

HMETAFILE  W32kCloseMetaFile(HDC  hDC);

HENHMETAFILE  W32kCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File);

HMETAFILE  W32kCopyMetaFile(HMETAFILE  Src,
                            LPCWSTR  File);

HDC  W32kCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description);

HDC  W32kCreateMetaFile(LPCWSTR  File);

BOOL  W32kDeleteEnhMetaFile(HENHMETAFILE  emf);

BOOL  W32kDeleteMetaFile(HMETAFILE  mf);

BOOL  W32kEnumEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  emf,
                          ENHMFENUMPROC  EnhMetaFunc,
                          LPVOID  Data,
                          CONST LPRECT  Rect);

BOOL  W32kEnumMetaFile(HDC  hDC,
                       HMETAFILE  mf,
                       MFENUMPROC  MetaFunc,
                       LPARAM  lParam);

BOOL  W32kGdiComment(HDC  hDC,
                     UINT  Size,
                     CONST LPBYTE  Data);

HENHMETAFILE  W32kGetEnhMetaFile(LPCWSTR  MetaFile);

UINT  W32kGetEnhMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer);

UINT  W32kGetEnhMetaFileDescription(HENHMETAFILE  hemf,
                                    UINT  BufSize,
                                    LPWSTR  Description);

UINT  W32kGetEnhMetaFileHeader(HENHMETAFILE  hemf,
                               UINT  BufSize,
                               LPENHMETAHEADER  emh);

UINT  W32kGetEnhMetaFilePaletteEntries(HENHMETAFILE  hemf,
                                       UINT  Entries,
                                       LPPALETTEENTRY  pe);

HMETAFILE  W32kGetMetaFile(LPCWSTR  MetaFile);

UINT  W32kGetMetaFileBitsEx(HMETAFILE  hmf,
                            UINT  Size,
                            LPVOID  Data);

UINT  W32kGetWinMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer,
                             INT  MapMode,
                             HDC  Ref);

BOOL  W32kPlayEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  hemf,
                          CONST PRECT  Rect);

BOOL  W32kPlayEnhMetaFileRecord(HDC  hDC,
                                LPHANDLETABLE  Handletable,
                                CONST ENHMETARECORD *EnhMetaRecord,
                                UINT  Handles);

BOOL  W32kPlayMetaFile(HDC  hDC,
                       HMETAFILE  hmf);

BOOL  W32kPlayMetaFileRecord(HDC  hDC,
                             LPHANDLETABLE  Handletable,
                             LPMETARECORD  MetaRecord,
                             UINT  Handles);

HENHMETAFILE  W32kSetEnhMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Data);

HMETAFILE  W32kSetMetaFileBitsEx(UINT  Size,
                                 CONST PBYTE  Data);

HENHMETAFILE  W32kSetWinMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Buffer,
                                     HDC  Ref,
                                     CONST METAFILEPICT *mfp);

#endif

