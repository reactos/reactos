#ifndef _WIN32K_TEXT_H
#define _WIN32K_TEXT_H

BOOL FASTCALL InitFontSupport(VOID);
BOOL FASTCALL IntIsFontRenderingEnabled(VOID);
BOOL FASTCALL IntIsFontRenderingEnabled(VOID);
VOID FASTCALL IntEnableFontRendering(BOOL Enable);
INT FASTCALL FontGetObject(PTEXTOBJ TextObj, INT Count, PVOID Buffer);

#define IntLockProcessPrivateFonts(W32Process) \
  ExAcquireFastMutex(&W32Process->PrivateFontListLock)

#define IntUnLockProcessPrivateFonts(W32Process) \
  ExReleaseFastMutex(&W32Process->PrivateFontListLock)

#define IntLockGlobalFonts \
  ExAcquireFastMutex(&FontListLock)

#define IntUnLockGlobalFonts \
  ExReleaseFastMutex(&FontListLock)

#define IntLockFreeType \
  ExAcquireFastMutex(&FreeTypeLock)

#define IntUnLockFreeType \
  ExReleaseFastMutex(&FreeTypeLock)

#endif /* _WIN32K_TEXT_H */
