#ifndef __WIN32K_MOUSE_H
#define __WIN32K_MOUSE_H

#include <internal/kbd.h>

NTSTATUS FASTCALL
InitInputImpl(VOID);
PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue(VOID);
PKBDTABLES W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout);

#endif /* __WIN32K_MOUSE_H */
