#ifndef _WIN32K_INPUT_H
#define _WIN32K_INPUT_H

#include <internal/kbd.h>

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

NTSTATUS FASTCALL
InitInputImpl(VOID);
NTSTATUS FASTCALL
InitKeyboardImpl(VOID);
PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue(VOID);
VOID W32kUnregisterPrimitiveMessageQueue(VOID);
PKBL W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout, BYTE Prefix);
BOOL FASTCALL IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi);
BOOL FASTCALL IntKeyboardInput(KEYBDINPUT *ki);

BOOL UserInitDefaultKeyboardLayout();

#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

#endif /* _WIN32K_INPUT_H */
