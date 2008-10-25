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
#include <stdio.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#define _WMI_SOURCE_
#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <wmistr.h>
#include <evntrace.h>
#include <sddl.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* this has to go after the NDK when being used with the NDK */
#include <ntsecapi.h>

#include <services/services.h>
#include "svcctl_c.h"
#include "lsa_c.h"
#include "eventlogrpc_c.h"

#ifndef HAS_FN_PROGRESSW
#define FN_PROGRESSW FN_PROGRESS
#endif
#ifndef HAS_FN_PROGRESSA
#define FN_PROGRESSA FN_PROGRESS
#endif

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
    DWORD (STDCALL *RewriteGetNamedRights)(LPWSTR pObjectName,
                                           SE_OBJECT_TYPE ObjectType,
                                           SECURITY_INFORMATION SecurityInfo,
                                           PSID* ppsidOwner,
                                           PSID* ppsidGroup,
                                           PACL* ppDacl,
                                           PACL* ppSacl,
                                           PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

    /* 2F4 */
    DWORD (STDCALL *RewriteSetNamedRights)(LPWSTR pObjectName,
                                           SE_OBJECT_TYPE ObjectType,
                                           SECURITY_INFORMATION SecurityInfo,
                                           PSECURITY_DESCRIPTOR pSecurityDescriptor);

    /*2F8*/
    DWORD (STDCALL *RewriteGetHandleRights)(HANDLE handle,
                                            SE_OBJECT_TYPE ObjectType,
                                            SECURITY_INFORMATION SecurityInfo,
                                            PSID* ppsidOwner,
                                            PSID* ppsidGroup,
                                            PACL* ppDacl,
                                            PACL* ppSacl,
                                            PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

    /* 2FC */
    DWORD (STDCALL *RewriteSetHandleRights)(HANDLE handle,
                                            SE_OBJECT_TYPE ObjectType,
                                            SECURITY_INFORMATION SecurityInfo,
                                            PSECURITY_DESCRIPTOR pSecurityDescriptor);

    /* 300 */
    DWORD (STDCALL *RewriteSetEntriesInAcl)(ULONG cCountOfExplicitEntries,
                                            PEXPLICIT_ACCESS_W pListOfExplicitEntries,
                                            PACL OldAcl,
                                            PACL* NewAcl);

    /* 304 */
    DWORD (STDCALL *RewriteGetExplicitEntriesFromAcl)(PACL pacl,
                                                      PULONG pcCountOfExplicitEntries,
                                                      PEXPLICIT_ACCESS_W* pListOfExplicitEntries);

    /* 308 */
    DWORD (STDCALL *TreeResetNamedSecurityInfo)(LPWSTR pObjectName,
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
    DWORD (STDCALL *GetInheritanceSource)(LPWSTR pObjectName,
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
    DWORD (STDCALL *FreeIndexArray)(PINHERITED_FROMW pInheritArray,
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

#endif /* __ADVAPI32_H */
