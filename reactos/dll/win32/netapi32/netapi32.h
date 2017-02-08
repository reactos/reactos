
#ifndef __WINE_NETAPI32_H__
#define __WINE_NETAPI32_H__

#include <stdarg.h>

//#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
//#include "winerror.h"
//#include "lmcons.h"
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmbrowsr.h>
#include <lmerr.h>
//#include "lmshare.h"
//#include "lmuse.h"
#include <ntsecapi.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <wine/debug.h>
//#include "wine/unicode.h"
#include <wine/list.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ntsam.h>


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

#endif