#if !defined(__INCLUDE_WIN_BASE_H)
#define __INCLUDE_WIN_BASE_H

#include <csr/protocol.h>

/* w32 base server */

#define WIN_SRV_BASE 1

NTSTATUS STDCALL BaseSetProcessCreateNotify ();
NTSTATUS STDCALL ServerDllInitialization ();

#endif /* ndef __INCLUDE_WIN_BASE_H */

