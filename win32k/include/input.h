#ifndef _WIN32K_INPUT_H
#define _WIN32K_INPUT_H

#include <internal/kbd.h>

NTSTATUS FASTCALL
InitInputImpl(VOID);
NTSTATUS FASTCALL
InitKeyboardImpl(VOID);
PW32THREAD W32kGetPrimitiveWThread(VOID);
VOID W32kUnregisterPrimitiveWThread(VOID);
PKBDTABLES W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout, BYTE Prefix);
BOOL FASTCALL IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi);
BOOL FASTCALL IntKeyboardInput(KEYBDINPUT *ki);

#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

#endif /* _WIN32K_INPUT_H */
