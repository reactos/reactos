
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
  ((HFONT) GDIOBJ_AllocObj (sizeof (TEXTOBJ), GDI_OBJECT_TYPE_FONT, NULL))
#define  TEXTOBJ_FreeText(hBMObj)  GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_FONT, GDIOBJFLAG_DEFAULT)
#define  TEXTOBJ_LockText(hBMObj) ((PTEXTOBJ) GDIOBJ_LockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_FONT))
#define  TEXTOBJ_UnlockText(hBMObj) GDIOBJ_UnlockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_FONT)

NTSTATUS FASTCALL TextIntRealizeFont(HFONT FontHandle);
NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);

int
STDCALL
NtGdiAddFontResource(PUNICODE_STRING Filename,
					 DWORD fl);

HFONT
STDCALL
NtGdiCreateFont(int  Height,
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
NtGdiCreateFontIndirect(CONST LPLOGFONTW lf);

BOOL
STDCALL
NtGdiCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath);

int
STDCALL
NtGdiEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROCW  FontFunc,
                   LPARAM  lParam);

BOOL
STDCALL
NtGdiExtTextOut(HDC  hdc,
                     int  X,
                     int  Y,
                     UINT  fuOptions,
                     CONST RECT  *lprc,
                     LPCWSTR  lpString,
                     UINT  cbCount,
                     CONST INT  *lpDx);

BOOL
STDCALL
NtGdiGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio);

BOOL
STDCALL
NtGdiGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc);

BOOL
STDCALL
NtGdiGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF);

DWORD
STDCALL
NtGdiGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTSW Results,
                                 DWORD  Flags);

BOOL
STDCALL
NtGdiGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer);

BOOL
STDCALL
NtGdiGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer);

BOOL
STDCALL
NtGdiGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer);

DWORD
STDCALL
NtGdiGetFontLanguageInfo(HDC  hDC);

DWORD
STDCALL
NtGdiGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2);

DWORD
STDCALL
NtGdiGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair);

UINT
STDCALL
NtGdiGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRICW otm);

BOOL
STDCALL
NtGdiGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size);

UINT
STDCALL
NtGdiGetTextCharset(HDC  hDC);

UINT
STDCALL
NtGdiGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags);

BOOL
STDCALL
NtGdiGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size);

BOOL
STDCALL
NtGdiGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size);

BOOL
STDCALL
NtGdiGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size);

int
STDCALL
NtGdiGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName);

BOOL
STDCALL
NtGdiGetTextMetrics(HDC  hDC,
                         LPTEXTMETRICW  tm);

BOOL
STDCALL
NtGdiPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXTW txt,
                      int  Count);

BOOL
STDCALL
NtGdiRemoveFontResource(LPCWSTR  FileName);

DWORD
STDCALL
NtGdiSetMapperFlags(HDC  hDC,
                          DWORD  Flag);

UINT
STDCALL
NtGdiSetTextAlign(HDC  hDC,
                       UINT  Mode);

COLORREF
STDCALL
NtGdiSetTextColor(HDC  hDC,
                           COLORREF  Color);

BOOL
STDCALL
NtGdiSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount);

BOOL
STDCALL
NtGdiTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count);

#endif

