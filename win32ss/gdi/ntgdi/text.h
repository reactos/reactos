#pragma once

#define TAG_FINF        'FNIF'
//
// EXSTROBJ flags.
//
#define TO_MEM_ALLOCATED    0x0001
#define TO_ALL_PTRS_VALID   0x0002
#define TO_VALID            0x0004
#define TO_ESC_NOT_ORIENT   0x0008
#define TO_PWSZ_ALLOCATED   0x0010
#define TSIM_UNDERLINE1     0x0020
#define TSIM_UNDERLINE2     0x0040
#define TSIM_STRIKEOUT      0x0080
#define TO_HIGHRESTEXT      0x0100
#define TO_BITMAPS          0x0200
#define TO_PARTITION_INIT   0x0400
#define TO_ALLOC_FACENAME   0x0800
#define TO_SYS_PARTITION    0x1000
//
// Extended STROBJ
//
typedef struct _STRGDI
{
  STROBJ    StrObj; // Text string object header.
  FLONG     flTO;
  INT       cgposCopied;
  INT       cgposPositionsEnumerated;
  PVOID     prfo;  // PRFONT -> PFONTGDI
  PGLYPHPOS pgpos;
  POINTFIX  ptfxRef;
  POINTFIX  ptfxUpdate;
  POINTFIX  ptfxEscapement;
  RECTFX    rcfx;
  FIX       fxExtent;
  FIX       fxExtra;
  FIX       fxBreakExtra;
  DWORD     dwCodePage;
  INT       cExtraRects;
  RECTL     arclExtra[3];
  RECTL     rclBackGroundSave;
  PWCHAR    pwcPartition;
  PLONG     plPartition;
  PLONG     plNext;
  PGLYPHPOS pgpNext;
  PLONG     plCurrentFont;
  POINTL    ptlBaseLineAdjust;
  INT       cTTSysGlyphs;
  INT       cSysGlyphs;
  INT       cDefGlyphs;
  INT       cNumFaceNameGlyphs;
  PVOID     pacFaceNameGlyphs;
  ULONG     acFaceNameGlyphs[8];
} STRGDI, *PSTRGDI;

/* GDI logical font object */
typedef struct _LFONT
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
   BASEOBJECT    BaseObject;
   LFTYPE        lft;
   FLONG         fl;
   ENUMLOGFONTEXDVW logfont;
} LFONT;

#define LFONT_AllocFontWithHandle() ((PLFONT)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_FONT, sizeof(LFONT)))
#define LFONT_ShareLockFont(hfont) (PLFONT)GDIOBJ_ReferenceObjectByHandle(hfont, GDIObjType_LFONT_TYPE)
#define LFONT_ShareUnlockFont(plfnt) GDIOBJ_vDereferenceObject((POBJ)plfnt)
#define LFONT_UnlockFont(plfnt) GDIOBJ_vUnlockObject((POBJ)plfnt)
ULONG LFONT_GetObject(PLFONT plfont, ULONG cjBuffer, PVOID pvBuffer);
PRFONT LFONT_Realize(PLFONT pLFont, PPDEVOBJ hdevConsumer, DHPDEV dhpdev);

/* Realized GDI font object */
typedef struct _RFONT
{
    FONTOBJ      *Font;
    WCHAR         FullName[LF_FULLFACESIZE];
    WCHAR         Style[LF_FACESIZE];
    WCHAR         FaceName[LF_FACESIZE];

    PPDEVOBJ      hdevConsumer;
    DHPDEV        dhpdev;

    LONG lfHeight;
    LONG lfWidth;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfQuality;
} RFONT;

#define RFONT_Alloc() ((PRFONT)ExAllocatePoolWithTag(PagedPool, sizeof(RFONT), GDITAG_TEXT))
#define RFONT_Free(prfnt) (ExFreePoolWithTag(prfnt, GDITAG_TEXT))

PRFONT DC_prfnt(PDC pdc);

NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);
BOOL FASTCALL InitFontSupport(VOID);
BOOL FASTCALL IntIsFontRenderingEnabled(VOID);
BOOL FASTCALL IntIsFontRenderingEnabled(VOID);
VOID FASTCALL IntEnableFontRendering(BOOL Enable);
VOID FASTCALL IntLoadSystemFonts(VOID);
VOID FASTCALL IntGdiCleanupPrivateFontsForProcess(VOID);
INT FASTCALL IntGdiAddFontResource(PUNICODE_STRING FileName, DWORD Characteristics);
HANDLE FASTCALL IntGdiAddFontMemResource(PVOID Buffer, DWORD dwSize, PDWORD pNumAdded);
BOOL FASTCALL IntGdiRemoveFontMemResource(HANDLE hMMFont);
ULONG FASTCALL ftGdiGetGlyphOutline(PDC,WCHAR,UINT,LPGLYPHMETRICS,ULONG,PVOID,LPMAT2,BOOL);
INT FASTCALL IntGetOutlineTextMetrics(PRFONT,UINT,OUTLINETEXTMETRICW *);
BOOL TextIntUpdateSize(PRFONT,BOOL);
BOOL FASTCALL ftGdiGetRasterizerCaps(LPRASTERIZER_STATUS);
BOOL FASTCALL TextIntGetTextExtentPoint(PDC,PRFONT,LPCWSTR,INT,ULONG,LPINT,LPINT,LPSIZE,FLONG);
BOOL FASTCALL ftGdiGetTextMetricsW(HDC,PTMW_INTERNAL);
DWORD FASTCALL IntGetFontLanguageInfo(PDC);
INT FASTCALL ftGdiGetTextCharsetInfo(PDC,PFONTSIGNATURE,DWORD);
DWORD FASTCALL ftGetFontUnicodeRanges(PRFONT, PGLYPHSET);
DWORD FASTCALL ftGdiGetFontData(PRFONT,DWORD,DWORD,PVOID,DWORD);
BOOL FASTCALL IntGdiGetFontResourceInfo(PUNICODE_STRING,PVOID,DWORD*,DWORD);
BOOL FASTCALL ftGdiRealizationInfo(PRFONT,PREALIZATION_INFO);
DWORD FASTCALL ftGdiGetKerningPairs(PRFONT,DWORD,LPKERNINGPAIR);
BOOL NTAPI GreExtTextOutW(IN HDC,IN INT,IN INT,IN UINT,IN OPTIONAL RECTL*,
    IN LPCWSTR, IN INT, IN OPTIONAL LPINT, IN DWORD);
DWORD FASTCALL IntGetCharDimensions(HDC, PTEXTMETRICW, PDWORD);
BOOL FASTCALL GreGetTextExtentW(HDC,LPCWSTR,INT,LPSIZE,UINT);
BOOL FASTCALL GreGetTextExtentExW(HDC,LPCWSTR,ULONG,ULONG,PULONG,PULONG,LPSIZE,FLONG);
BOOL FASTCALL GreTextOutW(HDC,int,int,LPCWSTR,int);
HFONT FASTCALL GreCreateFontIndirectW( LOGFONTW * );
BOOL WINAPI GreGetTextMetricsW( _In_  HDC hdc, _Out_ LPTEXTMETRICW lptm);

#define IntLockProcessPrivateFonts(W32Process) \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&W32Process->PrivateFontListLock)

#define IntUnLockProcessPrivateFonts(W32Process) \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&W32Process->PrivateFontListLock)
