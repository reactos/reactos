

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/kapi.h>

// #define NDEBUG
#include <internal/debug.h>

int
STDCALL
W32kAddFontResource(LPCWSTR  Filename)
{
  UNIMPLEMENTED;
}

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
                      LPCWSTR  Face)
{
  UNIMPLEMENTED;
}

HFONT
STDCALL
W32kCreateFontIndirect(CONST LPLOGFONT  lf)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROC  EnumFontFamProc,
                          LPARAM  lParam)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONT  Logfont,
                            FONTENUMPROC  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROC  FontFunc,
                   LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCWSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2)
{
  UNIMPLEMENTED;


}

DWORD
STDCALL
W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRIC  otm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharset(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextMetrics(HDC  hDC,
                         LPTEXTMETRIC  tm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kRemoveFontResource(LPCWSTR  FileName)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UINT prevAlign;
  DC *dc;
  
  dc = DC_HandleToPtr(hDC);
  if (!dc) 
    {
      return  0;
    }
  prevAlign = dc->w.textAlign;
  dc->w.textAlign = Mode;
  DC_UnlockDC (hDC);
  
  return  prevAlign;
}

COLORREF
STDCALL
W32kSetTextColor(HDC hDC, 
                 COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_HandleToPtr(hDC);
  
  if (!dc) 
    {
      return 0x80000000;
    }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  DC_UnlockDC(hDC);

  return  oldColor;
}

BOOL
STDCALL
W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count)
{
    DC			*dc      = DC_HandleToPtr(hDC);
    SURFOBJ		*SurfObj = AccessUserObject(dc->Surface);
    PUNICODE_STRING	UString;
    PANSI_STRING	AString;

    RtlCreateUnicodeString(UString, (PWSTR)String);
    RtlUnicodeStringToAnsiString(AString, UString, TRUE);

    // For now we're just going to use an internal font
    grWriteCellString(SurfObj, XStart, YStart, AString->Buffer, 0xffffff);

    RtlFreeAnsiString(AString);
    RtlFreeUnicodeString(UString);
}

UINT
STDCALL
W32kTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,   
                               DWORD  Flags)
{
  UNIMPLEMENTED;
}

