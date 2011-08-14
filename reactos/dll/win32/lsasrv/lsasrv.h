/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsasrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#include <string.h>

#include "lsass.h"
#include "lsa_s.h"

#include <wine/debug.h>


/* authport.c */
NTSTATUS StartAuthenticationPort(VOID);

/* lsarpc.c */
VOID LsarStartRpcServer(VOID);

/* privileges.c */
NTSTATUS
LsarpLookupPrivilegeName(PLUID Value,
                         PUNICODE_STRING *Name);

NTSTATUS
LsarpLookupPrivilegeValue(PUNICODE_STRING Name,
                          PLUID Value);

/* sids.h */
NTSTATUS
LsapInitSids(VOID);

NTSTATUS
LsapLookupSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
               PLSAPR_TRANSLATED_NAME OutputNames);

