#if !defined(__INCLUDE_WIN_WINDOWS_H)
#define __INCLUDE_WIN_WINDOWS_H

#include <csr/protocol.h>

/* w32 console server */
#define WIN_SRV_WIN_CONSOLE  2
NTSTATUS WINAPI ConServerDllInitialization (ULONG,LPWSTR*);

/* w32 user server */
#define WIN_SRV_WIN_USER     3
NTSTATUS WINAPI UserServerDllInitialization (ULONG,LPWSTR*);

#endif /* ndef __INCLUDE_WIN_WINDOWS_H */


