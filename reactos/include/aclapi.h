/*
	aclapi.h

	This file is part of a free library for the Win32 API.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/
#ifndef _ACLAPI_H
#define _ACLAPI_H

#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef VOID (*FN_PROGRESS) (LPWSTR pObjectName,DWORD Status,PPROG_INVOKE_SETTING pInvokeSetting,PVOID Args,BOOL SecuritySet);

VOID STDCALL BuildExplicitAccessWithNameA(PEXPLICIT_ACCESS_A pExplicitAccess,LPSTR pTrusteeName,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance);
VOID STDCALL BuildExplicitAccessWithNameW(PEXPLICIT_ACCESS_W pExplicitAccess,LPWSTR pTrusteeName,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance);
DWORD STDCALL BuildSecurityDescriptorA(PTRUSTEE_A pOwner,PTRUSTEE_A pGroup,ULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_A pListOfAccessEntries,
 ULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_A pListOfAuditEntries,PSECURITY_DESCRIPTOR pOldSD,PULONG pSizeNewSD,PSECURITY_DESCRIPTOR* pNewSD);
DWORD STDCALL BuildSecurityDescriptorW(PTRUSTEE_W pOwner,PTRUSTEE_W pGroup,ULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_W pListOfAccessEntries,
 ULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_W pListOfAuditEntries,PSECURITY_DESCRIPTOR pOldSD,PULONG pSizeNewSD,PSECURITY_DESCRIPTOR* pNewSD);
VOID STDCALL BuildTrusteeWithNameA(PTRUSTEE_A pTrustee,LPSTR pName);
VOID STDCALL BuildTrusteeWithNameW(PTRUSTEE_W pTrustee,LPWSTR pName);
VOID STDCALL BuildTrusteeWithObjectsAndNameA(PTRUSTEE_A pTrustee,POBJECTS_AND_NAME_A pObjName,SE_OBJECT_TYPE ObjectType,
 LPSTR ObjectTypeName,LPSTR InheritedObjectTypeName,LPSTR Name);
VOID STDCALL BuildTrusteeWithObjectsAndNameW(PTRUSTEE_W pTrustee,POBJECTS_AND_NAME_W pObjName,SE_OBJECT_TYPE ObjectType,
 LPWSTR ObjectTypeName,LPWSTR InheritedObjectTypeName,LPWSTR Name);
VOID STDCALL BuildTrusteeWithObjectsAndSidA(PTRUSTEE_A pTrustee,POBJECTS_AND_SID pObjSid,GUID* pObjectGuid,GUID* pInheritedObjectGuid,PSID pSid);
VOID STDCALL BuildTrusteeWithObjectsAndSidW(PTRUSTEE_W pTrustee,POBJECTS_AND_SID pObjSid,GUID* pObjectGuid,GUID* pInheritedObjectGuid,PSID pSid);
VOID STDCALL BuildTrusteeWithSidA(PTRUSTEE_A pTrustee,PSID pSid);
VOID STDCALL BuildTrusteeWithSidW(PTRUSTEE_W pTrustee,PSID pSid);
DWORD STDCALL GetAuditedPermissionsFromAclA(PACL pacl,PTRUSTEE_A pTrustee,PACCESS_MASK pSuccessfulAuditedRights,PACCESS_MASK pFailedAuditRights);
DWORD STDCALL GetAuditedPermissionsFromAclW(PACL pacl,PTRUSTEE_W pTrustee,PACCESS_MASK pSuccessfulAuditedRights,PACCESS_MASK pFailedAuditRights);
DWORD STDCALL GetEffectiveRightsFromAclA(PACL pacl,PTRUSTEE_A pTrustee,PACCESS_MASK pAccessRights);
DWORD STDCALL GetEffectiveRightsFromAclW(PACL pacl,PTRUSTEE_W pTrustee,PACCESS_MASK pAccessRights);
DWORD STDCALL GetExplicitEntriesFromAclA(PACL pacl,PULONG pcCountOfExplicitEntries,PEXPLICIT_ACCESS_A* pListOfExplicitEntries);
DWORD STDCALL GetExplicitEntriesFromAclW(PACL pacl,PULONG pcCountOfExplicitEntries,PEXPLICIT_ACCESS_W* pListOfExplicitEntries);
DWORD STDCALL GetNamedSecurityInfoA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,
 PSID* ppsidOwner,PSID* ppsidGroup,PACL* ppDacl,PACL* ppSacl,PSECURITY_DESCRIPTOR* ppSecurityDescriptor);
DWORD STDCALL GetNamedSecurityInfoW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,
 PSID* ppsidOwner,PSID* ppsidGroup,PACL* ppDacl,PACL* ppSacl,PSECURITY_DESCRIPTOR* ppSecurityDescriptor);
DWORD STDCALL GetSecurityInfo(HANDLE handle,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,
 PSID* ppsidOwner,PSID* ppsidGroup,PACL* ppDacl,PACL* ppSacl,PSECURITY_DESCRIPTOR* ppSecurityDescriptor);
TRUSTEE_FORM STDCALL GetTrusteeFormA(PTRUSTEE_A pTrustee);
TRUSTEE_FORM STDCALL GetTrusteeFormW(PTRUSTEE_W pTrustee);
LPSTR STDCALL GetTrusteeNameA(PTRUSTEE_A pTrustee);
LPWSTR STDCALL GetTrusteeNameW(PTRUSTEE_W pTrustee);
TRUSTEE_TYPE STDCALL GetTrusteeTypeA(PTRUSTEE_A pTrustee);
TRUSTEE_TYPE STDCALL GetTrusteeTypeW(PTRUSTEE_W pTrustee);
DWORD STDCALL LookupSecurityDescriptorPartsA(PTRUSTEE_A* pOwner,PTRUSTEE_A* pGroup,PULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_A* pListOfAccessEntries,
 PULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_A* pListOfAuditEntries,PSECURITY_DESCRIPTOR pSD);
DWORD STDCALL LookupSecurityDescriptorPartsW(PTRUSTEE_W* pOwner,PTRUSTEE_W* pGroup,PULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_W* pListOfAccessEntries,
 PULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_W* pListOfAuditEntries,PSECURITY_DESCRIPTOR pSD);
DWORD STDCALL SetEntriesInAclA(ULONG cCountOfExplicitEntries,PEXPLICIT_ACCESS_A pListOfExplicitEntries,PACL OldAcl,PACL* NewAcl);
DWORD STDCALL SetEntriesInAclW(ULONG cCountOfExplicitEntries,PEXPLICIT_ACCESS_W pListOfExplicitEntries,PACL OldAcl,PACL* NewAcl);
DWORD STDCALL SetNamedSecurityInfoA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID psidOwner,PSID psidGroup,PACL pDacl,PACL pSacl);
DWORD STDCALL SetNamedSecurityInfoW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID psidOwner,PSID psidGroup,PACL pDacl,PACL pSacl);
DWORD STDCALL SetSecurityInfo(HANDLE handle,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID psidOwner,PSID psidGroup,PACL pDacl,PACL pSacl);
DWORD STDCALL GetInheritanceSourceA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,BOOL Container,GUID ** pObjectClassGuids ,DWORD GuidCount,PACL pAcl,PFN_OBJECT_MGR_FUNCTS pfnArray OPTIONAL,PGENERIC_MAPPING pGenericMapping,PINHERITED_FROMA pInheritArray);
DWORD STDCALL GetInheritanceSourceW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,BOOL Container,GUID ** pObjectClassGuids ,DWORD GuidCount,PACL pAcl,PFN_OBJECT_MGR_FUNCTS pfnArray OPTIONAL,PGENERIC_MAPPING pGenericMapping,PINHERITED_FROMW pInheritArray);
DWORD STDCALL FreeInheritedFromArray(PINHERITED_FROMW pInheritArray,USHORT AceCnt,PFN_OBJECT_MGR_FUNCTS pfnArray);
DWORD STDCALL TreeResetNamedSecurityInfoA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID pOwner,PSID pGroup,PACL pDacl,PACL pSacl,BOOL KeepExplicit,FN_PROGRESS fnProgress,PROG_INVOKE_SETTING ProgressInvokeSetting,PVOID Args);
DWORD STDCALL TreeResetNamedSecurityInfoW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID pOwner,PSID pGroup,PACL pDacl,PACL pSacl,BOOL KeepExplicit,FN_PROGRESS fnProgress,PROG_INVOKE_SETTING ProgressInvokeSetting,PVOID Args);
VOID STDCALL BuildImpersonateExplicitAccessWithNameA(PEXPLICIT_ACCESS_A pExplicitAccess,LPSTR pTrusteeName,PTRUSTEE_A pTrustee,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance);
VOID STDCALL BuildImpersonateExplicitAccessWithNameW(PEXPLICIT_ACCESS_W pExplicitAccess,LPWSTR pTrusteeName,PTRUSTEE_W pTrustee,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance);
VOID STDCALL BuildImpersonateTrusteeA(PTRUSTEE_A pTrustee,PTRUSTEE_A pImpersonateTrustee);
VOID STDCALL BuildImpersonateTrusteeW(PTRUSTEE_W pTrustee,PTRUSTEE_W pImpersonateTrustee);
MULTIPLE_TRUSTEE_OPERATION STDCALL GetMultipleTrusteeOperationA(PTRUSTEE_A pTrustee);
MULTIPLE_TRUSTEE_OPERATION STDCALL GetMultipleTrusteeOperationW(PTRUSTEE_W pTrustee);
PTRUSTEE_A STDCALL GetMultipleTrusteeA(PTRUSTEE_A pTrustee);
PTRUSTEE_W STDCALL GetMultipleTrusteeW(PTRUSTEE_W pTrustee);

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define BuildExplicitAccessWithName BuildExplicitAccessWithNameW
#define BuildSecurityDescriptor BuildSecurityDescriptorW
#define BuildTrusteeWithName BuildTrusteeWithNameW
#define BuildTrusteeWithObjectsAndName BuildTrusteeWithObjectsAndNameW
#define BuildTrusteeWithObjectsAndSid BuildTrusteeWithObjectsAndSidW
#define BuildTrusteeWithSid BuildTrusteeWithSidW
#define GetAuditedPermissionsFromAcl GetAuditedPermissionsFromAclW
#define GetEffectiveRightsFromAcl GetEffectiveRightsFromAclW
#define GetExplicitEntriesFromAcl GetExplicitEntriesFromAclW
#define GetNamedSecurityInfo GetNamedSecurityInfoW
#define GetTrusteeForm GetTrusteeFormW
#define GetTrusteeName GetTrusteeNameW
#define GetTrusteeType GetTrusteeTypeW
#define LookupSecurityDescriptorParts LookupSecurityDescriptorPartsW
#define SetEntriesInAcl SetEntriesInAclW
#define SetNamedSecurityInfo SetNamedSecurityInfoW
#define GetInheritanceSource GetInheritanceSourceW
#define TreeResetNamedSecurityInfo TreeResetNamedSecurityInfoW
#define BuildImpersonateExplicitAccessWithName BuildImpersonateExplicitAccessWithNameW
#define BuildImpersonateTrustee BuildImpersonateTrusteeW
#define GetMultipleTrusteeOperation GetMultipleTrusteeOperationW
#define GetMultipleTrustee GetMultipleTrusteeW
#else
#define BuildExplicitAccessWithName BuildExplicitAccessWithNameA
#define BuildSecurityDescriptor BuildSecurityDescriptorA
#define BuildTrusteeWithName BuildTrusteeWithNameA
#define BuildTrusteeWithObjectsAndName BuildTrusteeWithObjectsAndNameA
#define BuildTrusteeWithObjectsAndSid BuildTrusteeWithObjectsAndSidA
#define BuildTrusteeWithSid BuildTrusteeWithSidA
#define GetAuditedPermissionsFromAcl GetAuditedPermissionsFromAclA
#define GetEffectiveRightsFromAcl GetEffectiveRightsFromAclA
#define GetExplicitEntriesFromAcl GetExplicitEntriesFromAclA
#define GetNamedSecurityInfo GetNamedSecurityInfoA
#define GetTrusteeForm GetTrusteeFormA
#define GetTrusteeName GetTrusteeNameA
#define GetTrusteeType GetTrusteeTypeA
#define LookupSecurityDescriptorParts LookupSecurityDescriptorPartsA
#define SetEntriesInAcl SetEntriesInAclA
#define SetNamedSecurityInfo SetNamedSecurityInfoA
#define GetInheritanceSource GetInheritanceSourceA
#define TreeResetNamedSecurityInfo TreeResetNamedSecurityInfoA
#define BuildImpersonateExplicitAccessWithName BuildImpersonateExplicitAccessWithNameA
#define BuildImpersonateTrustee BuildImpersonateTrusteeA
#define GetMultipleTrusteeOperation GetMultipleTrusteeOperationA
#define GetMultipleTrustee GetMultipleTrusteeA
#endif /* UNICODE */
#endif

#ifdef __cplusplus
}
#endif
#endif
