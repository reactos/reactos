#ifndef _ACLAPI_H
#define _ACLAPI_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

VOID WINAPI BuildExplicitAccessWithNameA(PEXPLICIT_ACCESS_A,LPSTR,DWORD,ACCESS_MODE,DWORD);
VOID WINAPI BuildExplicitAccessWithNameW(PEXPLICIT_ACCESS_W,LPWSTR,DWORD,ACCESS_MODE,DWORD);
DWORD WINAPI BuildSecurityDescriptorA(PTRUSTEE_A,PTRUSTEE_A ,ULONG,PEXPLICIT_ACCESS_A,
  ULONG,PEXPLICIT_ACCESS_A,PSECURITY_DESCRIPTOR,PULONG,PSECURITY_DESCRIPTOR*);
DWORD WINAPI BuildSecurityDescriptorW(PTRUSTEE_W,PTRUSTEE_W ,ULONG,PEXPLICIT_ACCESS_W,
  ULONG,PEXPLICIT_ACCESS_W,PSECURITY_DESCRIPTOR,PULONG,PSECURITY_DESCRIPTOR*);
VOID WINAPI BuildTrusteeWithNameA(PTRUSTEE_A,LPSTR);
VOID WINAPI BuildTrusteeWithNameW(PTRUSTEE_W,LPWSTR);
VOID WINAPI BuildTrusteeWithObjectsAndNameA(PTRUSTEE_A,POBJECTS_AND_NAME_A,SE_OBJECT_TYPE,
  LPSTR,LPSTR,LPSTR);
VOID WINAPI BuildTrusteeWithObjectsAndNameW(PTRUSTEE_W,POBJECTS_AND_NAME_W,SE_OBJECT_TYPE,
  LPWSTR,LPWSTR,LPWSTR);
VOID WINAPI BuildTrusteeWithObjectsAndSidA(PTRUSTEE_A,POBJECTS_AND_SID,GUID*,GUID*,PSID);
VOID WINAPI BuildTrusteeWithObjectsAndSidW(PTRUSTEE_W,POBJECTS_AND_SID,GUID*,GUID*,PSID);
VOID WINAPI BuildTrusteeWithSidA(PTRUSTEE_A,PSID);
VOID WINAPI BuildTrusteeWithSidW(PTRUSTEE_W,PSID);
DWORD WINAPI GetAuditedPermissionsFromAclA(PACL,PTRUSTEE_A,PACCESS_MASK,PACCESS_MASK);
DWORD WINAPI GetAuditedPermissionsFromAclW(PACL,PTRUSTEE_W,PACCESS_MASK,PACCESS_MASK);
DWORD WINAPI GetEffectiveRightsFromAclA(PACL,PTRUSTEE_A,PACCESS_MASK);
DWORD WINAPI GetEffectiveRightsFromAclW(PACL,PTRUSTEE_W,PACCESS_MASK);
DWORD WINAPI GetExplicitEntriesFromAclA(PACL,PULONG,PEXPLICIT_ACCESS_A*);
DWORD WINAPI GetExplicitEntriesFromAclW(PACL,PULONG,PEXPLICIT_ACCESS_W*);
#if (_WIN32_WINNT >= 0x0501)
DWORD WINAPI GetInheritanceSourceA(LPSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,BOOL,GUID**,DWORD,PACL,void*,PGENERIC_MAPPING,PINHERITED_FROMA);
DWORD WINAPI GetInheritanceSourceW(LPWSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,BOOL,GUID**,DWORD,PACL,void*,PGENERIC_MAPPING,PINHERITED_FROMW);
#endif
DWORD WINAPI GetNamedSecurityInfoA(LPSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,
  PSID*,PSID*,PACL*,PACL*,PSECURITY_DESCRIPTOR*);
DWORD WINAPI GetNamedSecurityInfoW(LPWSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,
  PSID*,PSID*,PACL*,PACL*,PSECURITY_DESCRIPTOR*);
DWORD WINAPI GetSecurityInfo(HANDLE,SE_OBJECT_TYPE,SECURITY_INFORMATION,
  PSID*,PSID*,PACL*,PACL*,PSECURITY_DESCRIPTOR*);
TRUSTEE_FORM WINAPI GetTrusteeFormA(PTRUSTEE_A);
TRUSTEE_FORM WINAPI GetTrusteeFormW(PTRUSTEE_W);
LPSTR WINAPI GetTrusteeNameA(PTRUSTEE_A);
LPWSTR WINAPI GetTrusteeNameW(PTRUSTEE_W);
TRUSTEE_TYPE WINAPI GetTrusteeTypeA(PTRUSTEE_A);
TRUSTEE_TYPE WINAPI GetTrusteeTypeW(PTRUSTEE_W);
DWORD WINAPI LookupSecurityDescriptorPartsA(PTRUSTEE_A*,PTRUSTEE_A*,PULONG,PEXPLICIT_ACCESS_A*,
  PULONG,PEXPLICIT_ACCESS_A*,PSECURITY_DESCRIPTOR);
DWORD WINAPI LookupSecurityDescriptorPartsW(PTRUSTEE_W*,PTRUSTEE_W*,PULONG,PEXPLICIT_ACCESS_W*,
  PULONG,PEXPLICIT_ACCESS_W*,PSECURITY_DESCRIPTOR);
DWORD WINAPI SetEntriesInAclA(ULONG,PEXPLICIT_ACCESS_A,PACL,PACL*);
DWORD WINAPI SetEntriesInAclW(ULONG,PEXPLICIT_ACCESS_W,PACL,PACL*);
DWORD WINAPI SetNamedSecurityInfoA(LPSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID,PSID,PACL,PACL);
DWORD WINAPI SetNamedSecurityInfoW(LPWSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID,PSID,PACL,PACL);
DWORD WINAPI SetSecurityInfo(HANDLE,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID,PSID,PACL,PACL);
VOID WINAPI BuildImpersonateExplicitAccessWithNameA(PEXPLICIT_ACCESS_A,LPSTR,PTRUSTEE_A,DWORD,ACCESS_MODE,DWORD);
VOID WINAPI BuildImpersonateExplicitAccessWithNameW(PEXPLICIT_ACCESS_W,LPWSTR,PTRUSTEE_W,DWORD,ACCESS_MODE,DWORD);
VOID WINAPI BuildImpersonateTrusteeA(PTRUSTEE_A,PTRUSTEE_A);
VOID WINAPI BuildImpersonateTrusteeW(PTRUSTEE_W,PTRUSTEE_W);
PTRUSTEE_A WINAPI GetMultipleTrusteeA(PTRUSTEE_A);
PTRUSTEE_W WINAPI GetMultipleTrusteeW(PTRUSTEE_W);
MULTIPLE_TRUSTEE_OPERATION WINAPI GetMultipleTrusteeOperationA(PTRUSTEE_A);
MULTIPLE_TRUSTEE_OPERATION WINAPI GetMultipleTrusteeOperationW(PTRUSTEE_W);

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
