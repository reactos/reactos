#ifndef _WIN32K_INPUT_H
#define _WIN32K_INPUT_H

#include <internal/kbd.h>

NTSTATUS FASTCALL
InitInputImpl(VOID);
PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue(VOID);
PKBDTABLES W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout);

#endif /* _WIN32K_INPUT_H */
