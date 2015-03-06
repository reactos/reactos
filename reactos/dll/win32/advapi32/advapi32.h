/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/advapi32/advapi32.h
 * PURPOSE:         Win32 Advanced API Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */
#ifndef __ADVAPI32_H
#define __ADVAPI32_H

/* INCLUDES ******************************************************************/

/* C Headers */
#include <limits.h>
#include <stdio.h>

/* PSDK/NDK Headers */
#define WINE_STRICT_PROTOTYPES
#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define _WMI_SOURCE_
#include <aclapi.h>
#include <winsafer.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/sefuncs.h>

/* this has to go after the NDK when being used with the NDK */
#include <ntsecapi.h>

#include <services/services.h>
#include <svcctl_c.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "wine/crypt.h"

#ifndef HAS_FN_PROGRESSW
#define FN_PROGRESSW FN_PROGRESS
#endif
#ifndef HAS_FN_PROGRESSA
#define FN_PROGRESSA FN_PROGRESS
#endif

/* logon.c */

NTSTATUS
CloseLogonLsaHandle(VOID);

/* rpc.c */

RPC_STATUS EvtBindRpc(LPCWSTR pszMachine,
                      RPC_BINDING_HANDLE* BindingHandle);
RPC_STATUS EvtUnbindRpc(RPC_BINDING_HANDLE *BindingHandle);

BOOL
EvtGetLocalHandle(RPC_BINDING_HANDLE *BindingHandle);
RPC_STATUS EvtUnbindLocalHandle(void);

/* scm.c */
DWORD
ScmRpcStatusToWinError(RPC_STATUS Status);

/* Interface to ntmarta.dll **************************************************/

typedef struct _NTMARTA
{
    HINSTANCE hDllInstance;

    /* 2CC */PVOID LookupAccountTrustee;
    /* 2D0 */PVOID LookupAccountName;
    /* 2D4 */PVOID LookupAccountSid;
    /* 2D8 */PVOID SetEntriesInAList;
    /* 2DC */PVOID ConvertAccessToSecurityDescriptor;
    /* 2E0 */PVOID ConvertSDToAccess;
    /* 2E4 */PVOID ConvertAclToAccess;
    /* 2E8 */PVOID GetAccessForTrustee;
    /* 2EC */PVOID GetExplicitEntries;
    /* 2F0 */
    DWORD (WINAPI *RewriteGetNamedRights)(LPWSTR pObjectName,
                                           SE_OBJECT_TYPE ObjectType,
                                           SECURITY_INFORMATION SecurityInfo,
                                           PSID* ppsidOwner,
                                           PSID* ppsidGroup,
                                           PACL* ppDacl,
                                           PACL* ppSacl,
                                           PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

    /* 2F4 */
    DWORD (WINAPI *RewriteSetNamedRights)(LPWSTR pObjectName,
                                           SE_OBJECT_TYPE ObjectType,
                                           SECURITY_INFORMATION SecurityInfo,
                                           PSECURITY_DESCRIPTOR pSecurityDescriptor);

    /*2F8*/
    DWORD (WINAPI *RewriteGetHandleRights)(HANDLE handle,
                                            SE_OBJECT_TYPE ObjectType,
                                            SECURITY_INFORMATION SecurityInfo,
                                            PSID* ppsidOwner,
                                            PSID* ppsidGroup,
                                            PACL* ppDacl,
                                            PACL* ppSacl,
                                            PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

    /* 2FC */
    DWORD (WINAPI *RewriteSetHandleRights)(HANDLE handle,
                                            SE_OBJECT_TYPE ObjectType,
                                            SECURITY_INFORMATION SecurityInfo,
                                            PSECURITY_DESCRIPTOR pSecurityDescriptor);

    /* 300 */
    DWORD (WINAPI *RewriteSetEntriesInAcl)(ULONG cCountOfExplicitEntries,
                                            PEXPLICIT_ACCESS_W pListOfExplicitEntries,
                                            PACL OldAcl,
                                            PACL* NewAcl);

    /* 304 */
    DWORD (WINAPI *RewriteGetExplicitEntriesFromAcl)(PACL pacl,
                                                      PULONG pcCountOfExplicitEntries,
                                                      PEXPLICIT_ACCESS_W* pListOfExplicitEntries);

    /* 308 */
    DWORD (WINAPI *TreeResetNamedSecurityInfo)(LPWSTR pObjectName,
                                                SE_OBJECT_TYPE ObjectType,
                                                SECURITY_INFORMATION SecurityInfo,
                                                PSID pOwner,
                                                PSID pGroup,
                                                PACL pDacl,
                                                PACL pSacl,
                                                BOOL KeepExplicit,
                                                FN_PROGRESSW fnProgress,
                                                PROG_INVOKE_SETTING ProgressInvokeSetting,
                                                PVOID Args);
    /* 30C */
    DWORD (WINAPI *GetInheritanceSource)(LPWSTR pObjectName,
                                          SE_OBJECT_TYPE ObjectType,
                                          SECURITY_INFORMATION SecurityInfo,
                                          BOOL Container,
                                          GUID** pObjectClassGuids,
                                          DWORD GuidCount,
                                          PACL pAcl,
                                          PFN_OBJECT_MGR_FUNCTS pfnArray,
                                          PGENERIC_MAPPING pGenericMapping,
                                          PINHERITED_FROMW pInheritArray);

    /* 310 */
    DWORD (WINAPI *FreeIndexArray)(PINHERITED_FROMW pInheritArray,
                                    USHORT AceCnt,
                                    PFN_OBJECT_MGR_FUNCTS pfnArray  OPTIONAL);
} NTMARTA, *PNTMARTA;

#define AccLookupAccountTrustee NtMartaStatic.LookupAccountTrustee
#define AccLookupAccountName NtMartaStatic.LookupAccountName
#define AccLookupAccountSid NtMartaStatic.LookupAccountSid
#define AccSetEntriesInAList NtMartaStatic.SetEntriesInAList
#define AccConvertAccessToSecurityDescriptor NtMartaStatic.ConvertAccessToSecurityDescriptor
#define AccConvertSDToAccess NtMartaStatic.ConvertSDToAccess
#define AccConvertAclToAccess NtMartaStatic.ConvertAclToAccess
#define AccGetAccessForTrustee NtMartaStatic.GetAccessForTrustee
#define AccGetExplicitEntries NtMartaStatic.GetExplicitEntries
#define AccRewriteGetNamedRights NtMartaStatic.RewriteGetNamedRights
#define AccRewriteSetNamedRights NtMartaStatic.RewriteSetNamedRights
#define AccRewriteGetHandleRights NtMartaStatic.RewriteGetHandleRights
#define AccRewriteSetHandleRights NtMartaStatic.RewriteSetHandleRights
#define AccRewriteSetEntriesInAcl NtMartaStatic.RewriteSetEntriesInAcl
#define AccRewriteGetExplicitEntriesFromAcl NtMartaStatic.RewriteGetExplicitEntriesFromAcl
#define AccTreeResetNamedSecurityInfo NtMartaStatic.TreeResetNamedSecurityInfo
#define AccGetInheritanceSource NtMartaStatic.GetInheritanceSource
#define AccFreeIndexArray NtMartaStatic.FreeIndexArray

extern NTMARTA NtMartaStatic;

DWORD CheckNtMartaPresent(VOID);

/* heap allocation helpers */
static void *heap_alloc( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

#endif /* __ADVAPI32_H */
