/*** Public header for BASESRV and the rest ***/

#ifndef __INCLUDE_WIN_BASE_H
#define __INCLUDE_WIN_BASE_H

//#include <csr/protocol.h>

/* Base Server */

typedef VOID (CALLBACK * BASE_PROCESS_CREATE_NOTIFY_ROUTINE)(PVOID);

NTSTATUS WINAPI BaseSetProcessCreateNotify (BASE_PROCESS_CREATE_NOTIFY_ROUTINE);
CSR_SERVER_DLL_INIT(ServerDllInitialization);

#endif // __INCLUDE_WIN_BASE_H

/* EOF */
