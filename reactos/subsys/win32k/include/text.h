#ifndef _WIN32K_TEXT_H
#define _WIN32K_TEXT_H

BOOL INTERNAL_CALL InitFontSupport(VOID);
BOOL INTERNAL_CALL IntIsFontRenderingEnabled(VOID);
BOOL INTERNAL_CALL IntIsFontRenderingEnabled(VOID);
VOID INTERNAL_CALL IntEnableFontRendering(BOOL Enable);
INT  INTERNAL_CALL FontGetObject(PTEXTOBJ TextObj, INT Count, PVOID Buffer);

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
