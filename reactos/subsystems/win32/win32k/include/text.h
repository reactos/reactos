#ifndef _WIN32K_TEXT_H
#define _WIN32K_TEXT_H

#define TAG_FINF        TAG('F', 'I', 'N', 'F')
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
  LONG      lCurrentFont;
  POINTL    ptlBaseLineAdjust;
  INT       cTTSysGlyphs;
  INT       cSysGlyphs;
  INT       cDefGlyphs;
  INT       cNumFaceNameGlyphs;
  PVOID     pacFaceNameGlyphs;
  ULONG     acFaceNameGlyphs[8];    
} STRGDI, *PSTRGDI;

/* GDI logical font object */
typedef struct
{
   ENUMLOGFONTEXDVW logfont;  //LOGFONTW   logfont;
   FONTOBJ    *Font;
   BOOLEAN Initialized; /* Don't reinitialize for each DC */
} TEXTOBJ, *PTEXTOBJ;

/*  Internal interface  */

#define  TEXTOBJ_AllocText() \
  ((HFONT) GDIOBJ_AllocObj (GdiHandleTable, GDI_OBJECT_TYPE_FONT))
#define  TEXTOBJ_FreeText(hBMObj)  GDIOBJ_FreeObj(GdiHandleTable, (HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_FONT)
#define  TEXTOBJ_LockText(hBMObj) ((PTEXTOBJ) GDIOBJ_LockObj (GdiHandleTable, (HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_FONT))
#define  TEXTOBJ_UnlockText(pBMObj) GDIOBJ_UnlockObjByPtr (GdiHandleTable, pBMObj)

NTSTATUS FASTCALL TextIntRealizeFont(HFONT FontHandle);
NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);
BOOL FASTCALL InitFontSupport(VOID);
BOOL FASTCALL IntIsFontRenderingEnabled(VOID);
BOOL FASTCALL IntIsFontRenderingEnabled(VOID);
VOID FASTCALL IntEnableFontRendering(BOOL Enable);
INT FASTCALL FontGetObject(PTEXTOBJ TextObj, INT Count, PVOID Buffer);
VOID FASTCALL IntLoadSystemFonts(VOID);
INT FASTCALL IntGdiAddFontResource(PUNICODE_STRING FileName, DWORD Characteristics);

#define IntLockProcessPrivateFonts(W32Process) \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&W32Process->PrivateFontListLock)

#define IntUnLockProcessPrivateFonts(W32Process) \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&W32Process->PrivateFontListLock)

#define IntLockGlobalFonts \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&FontListLock)

#define IntUnLockGlobalFonts \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&FontListLock)

#define IntLockFreeType \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&FreeTypeLock)

#define IntUnLockFreeType \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&FreeTypeLock)

#endif /* _WIN32K_TEXT_H */
