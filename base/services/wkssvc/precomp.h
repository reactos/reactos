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
#include <ntmsv1_0.h>
//#include <ntstatus.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/sefuncs.h>

#include <wkssvc_s.h>

#include <wine/debug.h>

#define WKSTA_KEEPCONN_PARMNUM 13
#define WKSTA_MAXCMDS_PARMNUM 15
#define WKSTA_SESSTIMEOUT_PARMNUM 18
#define WKSTA_DORMANTFILELIMIT_PARMNUM 46

extern OSVERSIONINFOW VersionInfo;
extern HANDLE LsaHandle;
extern ULONG LsaAuthenticationPackage;

extern WKSTA_INFO_502 WkstaInfo502;


/* domain.c */

NET_API_STATUS
NetpJoinWorkgroup(
    _In_ LPCWSTR WorkgroupName);

NET_API_STATUS
NetpGetJoinInformation(
    LPWSTR *NameBuffer,
    PNETSETUP_JOIN_STATUS BufferType);

/* info */

VOID
InitWorkstationInfo(VOID);

VOID
SaveWorkstationInfo(
    _In_ DWORD Level);

/* rpcserver.c */

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _WKSSVC_PCH_ */
