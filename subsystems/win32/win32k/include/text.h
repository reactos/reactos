#pragma once

/* GDI logical font object */
typedef struct _LFONT TEXTOBJ, *PTEXTOBJ;

/*  Internal interface  */

#define  TEXTOBJ_UnlockText(pBMObj) GDIOBJ_vUnlockObject ((POBJ)pBMObj)
NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);
BOOL FASTCALL InitFontSupport(VOID);
INT FASTCALL FontGetObject(PTEXTOBJ TextObj, INT Count, PVOID Buffer);
DWORD FASTCALL GreGetGlyphIndicesW(HDC,LPWSTR,INT,LPWORD,DWORD,DWORD);

#define IntLockProcessPrivateFonts(W32Process) \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&W32Process->PrivateFontListLock)

#define IntUnLockProcessPrivateFonts(W32Process) \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&W32Process->PrivateFontListLock)
BOOL
NTAPI
GreExtTextOutW(
    IN HDC,
    IN INT,
    IN INT,
    IN UINT,
    IN OPTIONAL RECTL*,
    IN LPWSTR,
    IN INT,
    IN OPTIONAL LPINT,
    IN DWORD);

BOOL
NTAPI
GreGetTextExtentW(
    HDC hdc,
    LPWSTR lpwsz,
    INT cwc,
    LPSIZE psize,
    UINT flOpts);
