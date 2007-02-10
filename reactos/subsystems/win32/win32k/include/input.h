#ifndef _WIN32K_INPUT_H
#define _WIN32K_INPUT_H

#include <internal/kbd.h>

typedef struct _KBDRVFILE
{
  PSINGLE_LIST_ENTRY pkbdfChain;
  WCHAR wcKBDF[9];              // used w GetKeyboardLayoutName same as wszKLID.
  struct _KBDTABLES* KBTables;  // KBDTABLES in ntoskrnl/include/internal/kbd.h
} KBDRVFILE, *PKBDRVFILE;

typedef struct _KBL
{
  PLIST_ENTRY pklChain;
  DWORD dwKBLFlags;
  HKL hkl;
  PKBDRVFILE pkbdf;
} KBL, *PKBL;

#define KBL_UNLOADED 0x20000000
#define KBL_RESET    0x40000000

NTSTATUS FASTCALL
InitInputImpl(VOID);
NTSTATUS FASTCALL
InitKeyboardImpl(VOID);
PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue(VOID);
VOID W32kUnregisterPrimitiveMessageQueue(VOID);
PKBDTABLES W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout, BYTE Prefix);
BOOL FASTCALL IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi);
BOOL FASTCALL IntKeyboardInput(KEYBDINPUT *ki);

#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

#endif /* _WIN32K_INPUT_H */
