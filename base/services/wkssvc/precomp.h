#ifndef _WKSSVC_PCH_
#define _WKSSVC_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <winreg.h>
#include <winsvc.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <lmserver.h>
#include <ntsecapi.h>
//#include <ntstatus.h>
#include <ndk/rtlfuncs.h>

#include <wkssvc_s.h>

#include <wine/debug.h>

extern OSVERSIONINFOW VersionInfo;
extern HANDLE LsaHandle;
extern ULONG LsaAuthenticationPackage;

/* domain.c */

NET_API_STATUS
NetpJoinWorkgroup(
    _In_ LPCWSTR WorkgroupName);

NET_API_STATUS
NetpGetJoinInformation(
    LPWSTR *NameBuffer,
    PNETSETUP_JOIN_STATUS BufferType);


/* rpcserver.c */

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _WKSSVC_PCH_ */
