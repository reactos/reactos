#if !defined(__INCLUDE_WIN_BASE_H)
#define __INCLUDE_WIN_BASE_H

#include <csr/protocol.h>

/* w32 base server */

#define WIN_SRV_BASE 1

typedef VOID (CALLBACK * BASE_PROCESS_CREATE_NOTIFY_ROUTINE)(PVOID);

NTSTATUS STDCALL BaseSetProcessCreateNotify (BASE_PROCESS_CREATE_NOTIFY_ROUTINE);
NTSTATUS STDCALL ServerDllInitialization (ULONG,LPWSTR*);

#endif /* ndef __INCLUDE_WIN_BASE_H */

