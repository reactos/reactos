#ifndef _WIN32K_TEXT_H
#define _WIN32K_TEXT_H

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
