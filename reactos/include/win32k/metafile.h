
#ifndef __WIN32K_METAFILE_H
#define __WIN32K_METAFILE_H

HENHMETAFILE  W32kCloseEnhMetaFile(HDC  hDC);

HMETAFILE  W32kCloseMetaFile(HDC  hDC);

HENHMETAFILE  W32kCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCTSTR  File);

HMETAFILE  W32kCopyMetaFile(HMETAFILE  Src,
                            LPCTSTR  File);

HDC  W32kCreateEnhMetaFile(HDC  hDCRef,
                           LPCTSTR  File,
                           CONST LPRECT  Rect,
                           LPCTSTR  Description);

HDC  W32kCreateMetaFile(LPCTSTR  File);

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

HENHMETAFILE  W32kGetEnhMetaFile(LPCTSTR  MetaFile);


#endif

