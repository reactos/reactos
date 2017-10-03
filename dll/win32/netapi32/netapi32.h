#ifndef __WINE_NETAPI32_H__
#define __WINE_NETAPI32_H__

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <ntsecapi.h>
#include <nb30.h>
#include <iphlpapi.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <ntsam.h>

#include "nbnamecache.h"
#include "netbios.h"

NET_API_STATUS
WINAPI
NetpNtStatusToApiStatus(NTSTATUS Status);

/* misc.c */

NTSTATUS
GetAccountDomainSid(IN PUNICODE_STRING ServerName,
                    OUT PSID *AccountDomainSid);

NTSTATUS
GetBuiltinDomainSid(OUT PSID *BuiltinDomainSid);

NTSTATUS
OpenAccountDomain(IN SAM_HANDLE ServerHandle,
                  IN PUNICODE_STRING ServerName,
                  IN ULONG DesiredAccess,
                  OUT PSAM_HANDLE DomainHandle);

NTSTATUS
OpenBuiltinDomain(IN SAM_HANDLE ServerHandle,
                  IN ULONG DesiredAccess,
                  OUT SAM_HANDLE *DomainHandle);

NET_API_STATUS
BuildSidFromSidAndRid(IN PSID SrcSid,
                      IN ULONG RelativeId,
                      OUT PSID *DestSid);

/* wksta.c */

BOOL
NETAPI_IsLocalComputer(LMCSTR ServerName);

#endif /* __WINE_NETAPI32_H__ */
