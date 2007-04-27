#ifndef _WIN32K_TEXT_H
#define _WIN32K_TEXT_H

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
