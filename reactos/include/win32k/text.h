
#ifndef __WIN32K_TEXT_H
#define __WIN32K_TEXT_H

/* GDI logical font object */
typedef struct
{
   LOGFONTW   logfont;
   HFONT      GDIFontHandle;
} TEXTOBJ, *PTEXTOBJ;

/*  Internal interface  */

#define  TEXTOBJ_AllocText() \
  ((HFONT) GDIOBJ_AllocObj (sizeof (TEXTOBJ), GO_FONT_MAGIC))
#define  TEXTOBJ_FreeText(hBMObj)  GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GO_FONT_MAGIC, GDIOBJFLAG_DEFAULT)
/*
#define  TEXTOBJ_HandleToPtr(hBMObj)  \
  ((PTEXTOBJ) GDIOBJ_HandleToPtr ((HGDIOBJ) hBMObj, GO_FONT_MAGIC))
#define  TEXTOBJ_PtrToHandle(hBMObj)  \
  ((HFONT) GDIOBJ_PtrToHandle ((PGDIOBJ) hBMObj, GO_FONT_MAGIC))
*/
#define  TEXTOBJ_LockText(hBMObj) ((PTEXTOBJ) GDIOBJ_LockObj ((HGDIOBJ) hBMObj, GO_FONT_MAGIC))
#define  TEXTOBJ_UnlockText(hBMObj) GDIOBJ_UnlockObj ((HGDIOBJ) hBMObj, GO_FONT_MAGIC)

int
STDCALL
W32kAddFontResource(LPCWSTR  Filename);

HFONT
STDCALL
W32kCreateFont(int  Height,
                      int  Width,
                      int  Escapement,
                      int  Orientation,
                      int  Weight,
                      DWORD  Italic,
                      DWORD  Underline,
                      DWORD  StrikeOut,
                      DWORD  CharSet,
                      DWORD  OutputPrecision,
                      DWORD  ClipPrecision,
                      DWORD  Quality,
                      DWORD  PitchAndFamily,
                      LPCWSTR  Face);

HFONT
STDCALL
W32kCreateFontIndirect(CONST LPLOGFONTW lf);

BOOL
STDCALL
W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath);

int
STDCALL
W32kEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROC  EnumFontFamProc,
                          LPARAM  lParam);

int
STDCALL
W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONTW  Logfont,
                            FONTENUMPROC  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags);

int
STDCALL
W32kEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROC  FontFunc,
                   LPARAM  lParam);

BOOL
STDCALL
W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCWSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx);

BOOL
STDCALL
W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio);

BOOL
STDCALL
W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc);

BOOL
STDCALL
W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF);

DWORD
STDCALL
W32kGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags);

BOOL
STDCALL
W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer);

BOOL
STDCALL
W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer);

BOOL
STDCALL
W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer);

DWORD
STDCALL
W32kGetFontLanguageInfo(HDC  hDC);

DWORD
STDCALL
W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2);

DWORD
STDCALL
W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair);

UINT
STDCALL
W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRICW otm);

BOOL
STDCALL
W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size);

UINT
STDCALL
W32kGetTextCharset(HDC  hDC);

UINT
STDCALL
W32kGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags);

BOOL
STDCALL
W32kGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size);

BOOL
STDCALL
W32kGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size);

BOOL
STDCALL
W32kGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size);

int
STDCALL
W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName);

BOOL
STDCALL
W32kGetTextMetrics(HDC  hDC,
                         LPTEXTMETRICW  tm);

BOOL
STDCALL
W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count);

BOOL
STDCALL
W32kRemoveFontResource(LPCWSTR  FileName);

DWORD
STDCALL
W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag);

UINT
STDCALL
W32kSetTextAlign(HDC  hDC,
                       UINT  Mode);

COLORREF
STDCALL
W32kSetTextColor(HDC  hDC,
                           COLORREF  Color);

BOOL
STDCALL
W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount);

BOOL
STDCALL
W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count);

UINT
STDCALL
W32kTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,
                               DWORD  Flags);

#endif

