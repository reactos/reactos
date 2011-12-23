#ifndef _WIN32K_KBD_H
#define _WIN32K_KBD_H

#include <ndk/kbd.h>

typedef struct _KBL
{
  LIST_ENTRY List;
  DWORD Flags;
  WCHAR Name[KL_NAMELENGTH];    // used w GetKeyboardLayoutName same as wszKLID.
  struct _KBDTABLES* KBTables;  // KBDTABLES in ntoskrnl/include/internal/kbd.h
  HANDLE hModule;
  ULONG RefCount;
  HKL hkl;
  DWORD klid; // Low word - language id. High word - device id.
} KBL, *PKBL;

#define KBL_UNLOAD 1
#define KBL_PRELOAD 2
#define KBL_RESET 4

BOOL UserInitDefaultKeyboardLayout();
PKBL UserGetDefaultKeyBoardLayout(VOID);
PKBL UserHklToKbl(HKL hKl);

#endif /* _WIN32K_KBD_H */
