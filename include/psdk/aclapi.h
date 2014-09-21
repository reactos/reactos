#ifndef _ACLAPI_H
#define _ACLAPI_H

#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

VOID WINAPI BuildExplicitAccessWithNameA(_Inout_ PEXPLICIT_ACCESS_A, _In_opt_ LPSTR, _In_ DWORD, _In_ ACCESS_MODE, _In_ DWORD);
VOID WINAPI BuildExplicitAccessWithNameW(_Inout_ PEXPLICIT_ACCESS_W, _In_opt_ LPWSTR, _In_ DWORD, _In_ ACCESS_MODE, _In_ DWORD);

DWORD
WINAPI
BuildSecurityDescriptorA(
  _In_opt_ PTRUSTEE_A pOwner,
  _In_opt_ PTRUSTEE_A pGroup,
  _In_ ULONG cCountOfAccessEntries,
  _In_reads_opt_(cCountOfAccessEntries) PEXPLICIT_ACCESS_A pListOfAccessEntries,
  _In_ ULONG cCountOfAuditEntries,
  _In_reads_opt_(cCountOfAuditEntries) PEXPLICIT_ACCESS_A pListOfAuditEntries,
  _In_opt_ PSECURITY_DESCRIPTOR pOldSD,
  _Out_ PULONG pSizeNewSD,
  _Outptr_result_bytebuffer_(*pSizeNewSD) PSECURITY_DESCRIPTOR *pNewSD);

DWORD
WINAPI
BuildSecurityDescriptorW(
  _In_opt_ PTRUSTEE_W pOwner,
  _In_opt_ PTRUSTEE_W pGroup,
  _In_ ULONG cCountOfAccessEntries,
  _In_reads_opt_(cCountOfAccessEntries) PEXPLICIT_ACCESS_W pListOfAccessEntries,
  _In_ ULONG cCountOfAuditEntries,
  _In_reads_opt_(cCountOfAuditEntries) PEXPLICIT_ACCESS_W pListOfAuditEntries,
  _In_opt_ PSECURITY_DESCRIPTOR pOldSD,
  _Out_ PULONG pSizeNewSD,
  _Outptr_result_bytebuffer_(*pSizeNewSD) PSECURITY_DESCRIPTOR *pNewSD);

VOID WINAPI BuildTrusteeWithNameA(_Inout_ PTRUSTEE_A, _In_opt_ LPSTR);
VOID WINAPI BuildTrusteeWithNameW(_Inout_ PTRUSTEE_W, _In_opt_ LPWSTR);
VOID WINAPI BuildTrusteeWithObjectsAndNameA(_Inout_ PTRUSTEE_A, _In_opt_ POBJECTS_AND_NAME_A, _In_opt_ SE_OBJECT_TYPE, _In_opt_ LPSTR, _In_opt_ LPSTR, _In_opt_ LPSTR);
VOID WINAPI BuildTrusteeWithObjectsAndNameW(_Inout_ PTRUSTEE_W, _In_opt_ POBJECTS_AND_NAME_W, _In_opt_ SE_OBJECT_TYPE, _In_opt_ LPWSTR, _In_opt_ LPWSTR, _In_opt_ LPWSTR);
VOID WINAPI BuildTrusteeWithObjectsAndSidA(_Inout_ PTRUSTEE_A, _In_opt_ POBJECTS_AND_SID, _In_opt_ GUID*, _In_opt_ GUID*, _In_opt_ PSID);
VOID WINAPI BuildTrusteeWithObjectsAndSidW(_Inout_ PTRUSTEE_W, _In_opt_ POBJECTS_AND_SID, _In_opt_ GUID*, _In_opt_ GUID*, _In_opt_ PSID);
VOID WINAPI BuildTrusteeWithSidA(_Inout_ PTRUSTEE_A, _In_opt_ PSID);
VOID WINAPI BuildTrusteeWithSidW(_Inout_ PTRUSTEE_W, _In_opt_ PSID);

#if (_WIN32_WINNT >= 0x0501)
DWORD
WINAPI
FreeInheritedFromArray(
  _In_reads_(AceCnt) PINHERITED_FROMW pInheritArray,
  _In_ USHORT AceCnt,
  _In_opt_ PFN_OBJECT_MGR_FUNCTS pfnArray);
#endif

DWORD WINAPI GetAuditedPermissionsFromAclA(_In_ PACL, _In_ PTRUSTEE_A, _Out_ PACCESS_MASK, _Out_ PACCESS_MASK);
DWORD WINAPI GetAuditedPermissionsFromAclW(_In_ PACL, _In_ PTRUSTEE_W, _Out_ PACCESS_MASK, _Out_ PACCESS_MASK);
DWORD WINAPI GetEffectiveRightsFromAclA(_In_ PACL, _In_ PTRUSTEE_A, _Out_ PACCESS_MASK);
DWORD WINAPI GetEffectiveRightsFromAclW(_In_ PACL, _In_ PTRUSTEE_W, _Out_ PACCESS_MASK);

DWORD
WINAPI
GetExplicitEntriesFromAclA(
  _In_ PACL pacl,
  _Out_ PULONG pcCountOfExplicitEntries,
  _Outptr_result_buffer_(*pcCountOfExplicitEntries) PEXPLICIT_ACCESS_A *pListOfExplicitEntries);

DWORD
WINAPI
GetExplicitEntriesFromAclW(
  _In_ PACL pacl,
  _Out_ PULONG pcCountOfExplicitEntries,
  _Outptr_result_buffer_(*pcCountOfExplicitEntries) PEXPLICIT_ACCESS_W *pListOfExplicitEntries);

#if (_WIN32_WINNT >= 0x0501)

DWORD
WINAPI
GetInheritanceSourceA(
  _In_ LPSTR pObjectName,
  _In_ SE_OBJECT_TYPE ObjectType,
  _In_ SECURITY_INFORMATION SecurityInfo,
  _In_ BOOL Container,
  _In_reads_opt_(GuidCount) GUID **pObjectClassGuids,
  _In_ DWORD GuidCount,
  _In_ PACL pAcl,
  _In_opt_ PFN_OBJECT_MGR_FUNCTS pfnArray,
  _In_ PGENERIC_MAPPING pGenericMapping,
  _Out_ PINHERITED_FROMA pInheritArray);

DWORD
WINAPI
GetInheritanceSourceW(
  _In_ LPWSTR pObjectName,
  _In_ SE_OBJECT_TYPE ObjectType,
  _In_ SECURITY_INFORMATION SecurityInfo,
  _In_ BOOL Container,
  _In_reads_opt_(GuidCount) GUID **pObjectClassGuids,
  _In_ DWORD GuidCount,
  _In_ PACL pAcl,
  _In_opt_ PFN_OBJECT_MGR_FUNCTS pfnArray,
  _In_ PGENERIC_MAPPING pGenericMapping,
  _Out_ PINHERITED_FROMW pInheritArray);

#endif

DWORD
WINAPI
GetNamedSecurityInfoA(
  _In_ LPSTR pObjectName,
  _In_ SE_OBJECT_TYPE ObjectType,
  _In_ SECURITY_INFORMATION SecurityInfo,
  _Out_opt_ PSID *ppsidOwner,
  _Out_opt_ PSID *ppsidGroup,
  _Out_opt_ PACL *ppDacl,
  _Out_opt_ PACL *ppSacl,
  _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor);

DWORD
WINAPI
GetNamedSecurityInfoW(
  _In_ LPWSTR pObjectName,
  _In_ SE_OBJECT_TYPE ObjectType,
  _In_ SECURITY_INFORMATION SecurityInfo,
  _Out_opt_ PSID *ppsidOwner,
  _Out_opt_ PSID *ppsidGroup,
  _Out_opt_ PACL *ppDacl,
  _Out_opt_ PACL *ppSacl,
  _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor);

DWORD
WINAPI
GetSecurityInfo(
  _In_ HANDLE handle,
  _In_ SE_OBJECT_TYPE ObjectType,
  _In_ SECURITY_INFORMATION SecurityInfo,
  _Out_opt_ PSID *ppsidOwner,
  _Out_opt_ PSID *ppsidGroup,
  _Out_opt_ PACL *ppDacl,
  _Out_opt_ PACL *ppSacl,
  _Out_opt_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor);

TRUSTEE_FORM WINAPI GetTrusteeFormA(_In_ PTRUSTEE_A);
TRUSTEE_FORM WINAPI GetTrusteeFormW(_In_ PTRUSTEE_W);
LPSTR WINAPI GetTrusteeNameA(_In_ PTRUSTEE_A);
LPWSTR WINAPI GetTrusteeNameW(_In_ PTRUSTEE_W);
TRUSTEE_TYPE WINAPI GetTrusteeTypeA(_In_opt_ PTRUSTEE_A);
TRUSTEE_TYPE WINAPI GetTrusteeTypeW(_In_opt_ PTRUSTEE_W);

DWORD
WINAPI
LookupSecurityDescriptorPartsA(
  _Out_opt_ PTRUSTEE_A *ppOwner,
  _Out_opt_ PTRUSTEE_A *ppGroup,
  _Out_opt_ PULONG pcCountOfAccessEntries,
  _Outptr_result_buffer_maybenull_(*pcCountOfAccessEntries) PEXPLICIT_ACCESS_A *ppListOfAccessEntries,
  _Out_opt_ PULONG pcCountOfAuditEntries,
  _Outptr_result_buffer_maybenull_(*pcCountOfAuditEntries) PEXPLICIT_ACCESS_A *ppListOfAuditEntries,
  _In_ PSECURITY_DESCRIPTOR pSD);

DWORD
WINAPI
LookupSecurityDescriptorPartsW(
  _Out_opt_ PTRUSTEE_W *ppOwner,
  _Out_opt_ PTRUSTEE_W *ppGroup,
  _Out_opt_ PULONG pcCountOfAccessEntries,
  _Outptr_result_buffer_maybenull_(*pcCountOfAccessEntries) PEXPLICIT_ACCESS_W *ppListOfAccessEntries,
  _Out_opt_ PULONG pcCountOfAuditEntries,
  _Outptr_result_buffer_maybenull_(*pcCountOfAuditEntries) PEXPLICIT_ACCESS_W *ppListOfAuditEntries,
  _In_ PSECURITY_DESCRIPTOR pSD);

DWORD
WINAPI
SetEntriesInAclA(
  _In_ ULONG cCountOfExplicitEntries,
  _In_reads_opt_(cCountOfExplicitEntries) PEXPLICIT_ACCESS_A pListOfExplicitEntries,
  _In_opt_ PACL OldAcl,
  _Out_ PACL *NewAcl);

DWORD
WINAPI
SetEntriesInAclW(
  _In_ ULONG cCountOfExplicitEntries,
  _In_reads_opt_(cCountOfExplicitEntries) PEXPLICIT_ACCESS_W pListOfExplicitEntries,
  _In_opt_ PACL OldAcl,
  _Out_ PACL *NewAcl);

DWORD WINAPI SetNamedSecurityInfoA(_In_ LPSTR, _In_ SE_OBJECT_TYPE, _In_ SECURITY_INFORMATION, _In_opt_ PSID, _In_opt_ PSID, _In_opt_ PACL, _In_opt_ PACL);
DWORD WINAPI SetNamedSecurityInfoW(_In_ LPWSTR, _In_ SE_OBJECT_TYPE, _In_ SECURITY_INFORMATION, _In_opt_ PSID, _In_opt_ PSID, _In_opt_ PACL, _In_opt_ PACL);
DWORD WINAPI SetSecurityInfo(_In_ HANDLE, _In_ SE_OBJECT_TYPE, _In_ SECURITY_INFORMATION, _In_opt_ PSID, _In_opt_ PSID, _In_opt_ PACL, _In_opt_ PACL);
VOID WINAPI BuildImpersonateExplicitAccessWithNameA(_Inout_ PEXPLICIT_ACCESS_A, _In_opt_ LPSTR, _In_opt_ PTRUSTEE_A, _In_ DWORD, _In_ ACCESS_MODE, _In_ DWORD);
VOID WINAPI BuildImpersonateExplicitAccessWithNameW(_Inout_ PEXPLICIT_ACCESS_W, _In_opt_ LPWSTR, _In_opt_ PTRUSTEE_W, _In_ DWORD, _In_ ACCESS_MODE, _In_ DWORD);
VOID WINAPI BuildImpersonateTrusteeA(_Inout_ PTRUSTEE_A, _In_opt_ PTRUSTEE_A);
VOID WINAPI BuildImpersonateTrusteeW(_Inout_ PTRUSTEE_W, _In_opt_ PTRUSTEE_W);
PTRUSTEE_A WINAPI GetMultipleTrusteeA(_In_opt_ PTRUSTEE_A);
PTRUSTEE_W WINAPI GetMultipleTrusteeW(_In_opt_ PTRUSTEE_W);
MULTIPLE_TRUSTEE_OPERATION WINAPI GetMultipleTrusteeOperationA(_In_opt_ PTRUSTEE_A);
MULTIPLE_TRUSTEE_OPERATION WINAPI GetMultipleTrusteeOperationW(_In_opt_ PTRUSTEE_W);

#ifdef UNICODE
#define BuildExplicitAccessWithName  BuildExplicitAccessWithNameW
#define BuildSecurityDescriptor  BuildSecurityDescriptorW
#define BuildTrusteeWithName  BuildTrusteeWithNameW
#define BuildTrusteeWithObjectsAndName  BuildTrusteeWithObjectsAndNameW
#define BuildTrusteeWithObjectsAndSid  BuildTrusteeWithObjectsAndSidW
#define BuildTrusteeWithSid  BuildTrusteeWithSidW
#define GetAuditedPermissionsFromAcl  GetAuditedPermissionsFromAclW
#define GetEffectiveRightsFromAcl  GetEffectiveRightsFromAclW
#define GetExplicitEntriesFromAcl  GetExplicitEntriesFromAclW
#define GetInheritanceSource  GetInheritanceSourceW
#define GetNamedSecurityInfo  GetNamedSecurityInfoW
#define GetTrusteeForm  GetTrusteeFormW
#define GetTrusteeName  GetTrusteeNameW
#define GetTrusteeType  GetTrusteeTypeW
#define LookupSecurityDescriptorParts  LookupSecurityDescriptorPartsW
#define SetEntriesInAcl  SetEntriesInAclW
#define SetNamedSecurityInfo  SetNamedSecurityInfoW
#define BuildImpersonateExplicitAccessWithName  BuildImpersonateExplicitAccessWithNameW
#define BuildImpersonateTrustee  BuildImpersonateTrusteeW
#define GetMultipleTrustee  GetMultipleTrusteeW
#define GetMultipleTrusteeOperation  GetMultipleTrusteeOperationW
#else
#define BuildExplicitAccessWithName  BuildExplicitAccessWithNameA
#define BuildSecurityDescriptor  BuildSecurityDescriptorA
#define BuildTrusteeWithName  BuildTrusteeWithNameA
#define BuildTrusteeWithObjectsAndName  BuildTrusteeWithObjectsAndNameA
#define BuildTrusteeWithObjectsAndSid  BuildTrusteeWithObjectsAndSidA
#define BuildTrusteeWithSid  BuildTrusteeWithSidA
#define GetAuditedPermissionsFromAcl  GetAuditedPermissionsFromAclA
#define GetEffectiveRightsFromAcl  GetEffectiveRightsFromAclA
#define GetExplicitEntriesFromAcl  GetExplicitEntriesFromAclA
#define GetInheritanceSource  GetInheritanceSourceA
#define GetNamedSecurityInfo  GetNamedSecurityInfoA
#define GetTrusteeForm  GetTrusteeFormA
#define GetTrusteeName  GetTrusteeNameA
#define GetTrusteeType  GetTrusteeTypeA
#define LookupSecurityDescriptorParts  LookupSecurityDescriptorPartsA
#define SetEntriesInAcl  SetEntriesInAclA
#define SetNamedSecurityInfo  SetNamedSecurityInfoA
#define BuildImpersonateExplicitAccessWithName  BuildImpersonateExplicitAccessWithNameA
#define BuildImpersonateTrustee  BuildImpersonateTrusteeA
#define GetMultipleTrustee  GetMultipleTrusteeA
#define GetMultipleTrusteeOperation  GetMultipleTrusteeOperationA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif
#endif
