#if !defined(__INCLUDE_WIN_WINDOWS_H)
#define __INCLUDE_WIN_WINDOWS_H

#include <csr/protocol.h>

/* w32 console server */
#define WIN_SRV_WIN_CONSOLE  2
NTSTATUS STDCALL ConServerDllInitialization ();

/* w32 user server */
#define WIN_SRV_WIN_USER     3
NTSTATUS STDCALL UserServerDllInitialization ();

#endif /* ndef __INCLUDE_WIN_WINDOWS_H */


