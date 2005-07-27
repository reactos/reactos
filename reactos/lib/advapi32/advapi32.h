/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/advapi32/advapi32.h
 * PURPOSE:         Win32 Advanced API Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* PSDK/NDK Headers */
#include <windows.h>
#include <ntsecapi.h>
#include <accctrl.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Interface to ntmarta.dll **************************************************/

typedef struct _NTMARTA
{
    HINSTANCE hDllInstance;

    PVOID LookupAccountTrustee;
    PVOID LookupAccountName;
    PVOID LookupAccountSid;
    PVOID SetEntriesInAList;
    PVOID ConvertAccessToSecurityDescriptor;
    PVOID ConvertSDToAccess;
    PVOID ConvertAclToAccess;
    PVOID GetAccessForTrustee;
    PVOID GetExplicitEntries;

    DWORD (STDCALL *RewriteGetNamedRights)(LPWSTR pObjectName,
                                           SE_OBJECT_TYPE ObjectType,
                                           SECURITY_INFORMATION SecurityInfo,
                                           PSID* ppsidOwner,
                                           PSID* ppsidGroup,
                                           PACL* ppDacl,
                                           PACL* ppSacl,
                                           PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

    DWORD (STDCALL *RewriteSetNamedRights)(LPWSTR pObjectName,
                                           SE_OBJECT_TYPE ObjectType,
                                           SECURITY_INFORMATION SecurityInfo,
                                           PSECURITY_DESCRIPTOR pSecurityDescriptor);

    DWORD (STDCALL *RewriteGetHandleRights)(HANDLE handle,
                                            SE_OBJECT_TYPE ObjectType,
                                            SECURITY_INFORMATION SecurityInfo,
                                            PSID* ppsidOwner,
                                            PSID* ppsidGroup,
                                            PACL* ppDacl,
                                            PACL* ppSacl,
                                            PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

    DWORD (STDCALL *RewriteSetHandleRights)(HANDLE handle,
                                            SE_OBJECT_TYPE ObjectType,
                                            SECURITY_INFORMATION SecurityInfo,
                                            PSECURITY_DESCRIPTOR pSecurityDescriptor);

    DWORD (STDCALL *RewriteSetEntriesInAcl)(ULONG cCountOfExplicitEntries,
                                            PEXPLICIT_ACCESS_W pListOfExplicitEntries,
                                            PACL OldAcl,
                                            PACL* NewAcl);

    PVOID RewriteGetExplicitEntriesFromAcl;
    PVOID TreeResetNamedSecurityInfo;

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

/* EOF */
