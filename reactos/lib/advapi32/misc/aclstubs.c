#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
/*
 * @unimplemented
 */
VOID STDCALL BuildExplicitAccessWithNameA(PEXPLICIT_ACCESS_A pExplicitAccess,LPSTR pTrusteeName,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildExplicitAccessWithNameW(PEXPLICIT_ACCESS_W pExplicitAccess,LPWSTR pTrusteeName,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance)
{
}
/*
 * @unimplemented
 */
DWORD STDCALL BuildSecurityDescriptorA(PTRUSTEE_A pOwner,PTRUSTEE_A pGroup,ULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_A pListOfAccessEntries,
 ULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_A pListOfAuditEntries,PSECURITY_DESCRIPTOR pOldSD,PULONG pSizeNewSD,PSECURITY_DESCRIPTOR* pNewSD)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL BuildSecurityDescriptorW(PTRUSTEE_W pOwner,PTRUSTEE_W pGroup,ULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_W pListOfAccessEntries,
 ULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_W pListOfAuditEntries,PSECURITY_DESCRIPTOR pOldSD,PULONG pSizeNewSD,PSECURITY_DESCRIPTOR* pNewSD)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithNameA(PTRUSTEE_A pTrustee,LPSTR pName)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithNameW(PTRUSTEE_W pTrustee,LPWSTR pName)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithObjectsAndNameA(PTRUSTEE_A pTrustee,POBJECTS_AND_NAME_A pObjName,SE_OBJECT_TYPE ObjectType,
 LPSTR ObjectTypeName,LPSTR InheritedObjectTypeName,LPSTR Name)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithObjectsAndNameW(PTRUSTEE_W pTrustee,POBJECTS_AND_NAME_W pObjName,SE_OBJECT_TYPE ObjectType,
 LPWSTR ObjectTypeName,LPWSTR InheritedObjectTypeName,LPWSTR Name)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithObjectsAndSidA(PTRUSTEE_A pTrustee,POBJECTS_AND_SID pObjSid,GUID* pObjectGuid,GUID* pInheritedObjectGuid,PSID pSid)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithObjectsAndSidW(PTRUSTEE_W pTrustee,POBJECTS_AND_SID pObjSid,GUID* pObjectGuid,GUID* pInheritedObjectGuid,PSID pSid)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithSidA(PTRUSTEE_A pTrustee,PSID pSid)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildTrusteeWithSidW(PTRUSTEE_W pTrustee,PSID pSid)
{
}
/*
 * @unimplemented
 */
DWORD STDCALL GetAuditedPermissionsFromAclA(PACL pacl,PTRUSTEE_A pTrustee,PACCESS_MASK pSuccessfulAuditedRights,PACCESS_MASK pFailedAuditRights)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetAuditedPermissionsFromAclW(PACL pacl,PTRUSTEE_W pTrustee,PACCESS_MASK pSuccessfulAuditedRights,PACCESS_MASK pFailedAuditRights)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetEffectiveRightsFromAclA(PACL pacl,PTRUSTEE_A pTrustee,PACCESS_MASK pAccessRights)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetEffectiveRightsFromAclW(PACL pacl,PTRUSTEE_W pTrustee,PACCESS_MASK pAccessRights)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetExplicitEntriesFromAclA(PACL pacl,PULONG pcCountOfExplicitEntries,PEXPLICIT_ACCESS_A* pListOfExplicitEntries)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetExplicitEntriesFromAclW(PACL pacl,PULONG pcCountOfExplicitEntries,PEXPLICIT_ACCESS_W* pListOfExplicitEntries)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetNamedSecurityInfoA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,
 PSID* ppsidOwner,PSID* ppsidGroup,PACL* ppDacl,PACL* ppSacl,PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetNamedSecurityInfoW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,
 PSID* ppsidOwner,PSID* ppsidGroup,PACL* ppDacl,PACL* ppSacl,PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
TRUSTEE_FORM STDCALL GetTrusteeFormA(PTRUSTEE_A pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
TRUSTEE_FORM STDCALL GetTrusteeFormW(PTRUSTEE_W pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
LPSTR STDCALL GetTrusteeNameA(PTRUSTEE_A pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
LPWSTR STDCALL GetTrusteeNameW(PTRUSTEE_W pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
TRUSTEE_TYPE STDCALL GetTrusteeTypeA(PTRUSTEE_A pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
TRUSTEE_TYPE STDCALL GetTrusteeTypeW(PTRUSTEE_W pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL LookupSecurityDescriptorPartsA(PTRUSTEE_A* pOwner,PTRUSTEE_A* pGroup,PULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_A* pListOfAccessEntries,
 PULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_A* pListOfAuditEntries,PSECURITY_DESCRIPTOR pSD)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL LookupSecurityDescriptorPartsW(PTRUSTEE_W* pOwner,PTRUSTEE_W* pGroup,PULONG cCountOfAccessEntries,PEXPLICIT_ACCESS_W* pListOfAccessEntries,
 PULONG cCountOfAuditEntries,PEXPLICIT_ACCESS_W* pListOfAuditEntries,PSECURITY_DESCRIPTOR pSD)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL SetEntriesInAclA(ULONG cCountOfExplicitEntries,PEXPLICIT_ACCESS_A pListOfExplicitEntries,PACL OldAcl,PACL* NewAcl)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL SetEntriesInAclW(ULONG cCountOfExplicitEntries,PEXPLICIT_ACCESS_W pListOfExplicitEntries,PACL OldAcl,PACL* NewAcl)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL SetNamedSecurityInfoA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID psidOwner,PSID psidGroup,PACL pDacl,PACL pSacl)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL SetNamedSecurityInfoW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID psidOwner,PSID psidGroup,PACL pDacl,PACL pSacl)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL SetSecurityInfo(HANDLE handle,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID psidOwner,PSID psidGroup,PACL pDacl,PACL pSacl)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetInheritanceSourceA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo, BOOL Container,GUID ** pObjectClassGuids ,DWORD GuidCount,PACL pAcl,PFN_OBJECT_MGR_FUNCTS pfnArray OPTIONAL,PGENERIC_MAPPING pGenericMapping,PINHERITED_FROMA pInheritArray)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL GetInheritanceSourceW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo, BOOL Container,GUID ** pObjectClassGuids ,DWORD GuidCount,PACL pAcl,PFN_OBJECT_MGR_FUNCTS pfnArray OPTIONAL,PGENERIC_MAPPING pGenericMapping,PINHERITED_FROMW pInheritArray)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL FreeInheritedFromArray(PINHERITED_FROMW pInheritArray,USHORT AceCnt,PFN_OBJECT_MGR_FUNCTS pfnArray)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL TreeResetNamedSecurityInfoA(LPSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID pOwner,PSID pGroup,PACL pDacl,PACL pSacl,BOOL KeepExplicit,FN_PROGRESS fnProgress,PROG_INVOKE_SETTING ProgressInvokeSetting,PVOID Args)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
DWORD STDCALL TreeResetNamedSecurityInfoW(LPWSTR pObjectName,SE_OBJECT_TYPE ObjectType,SECURITY_INFORMATION SecurityInfo,PSID pOwner,PSID pGroup,PACL pDacl,PACL pSacl,BOOL KeepExplicit,FN_PROGRESS fnProgress,PROG_INVOKE_SETTING ProgressInvokeSetting,PVOID Args)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
VOID STDCALL BuildImpersonateExplicitAccessWithNameA(PEXPLICIT_ACCESS_A pExplicitAccess,LPSTR pTrusteeName,PTRUSTEE_A pTrustee,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildImpersonateExplicitAccessWithNameW(PEXPLICIT_ACCESS_W pExplicitAccess,LPWSTR pTrusteeName,PTRUSTEE_W pTrustee,DWORD AccessPermissions,ACCESS_MODE AccessMode,DWORD Inheritance)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildImpersonateTrusteeA(PTRUSTEE_A pTrustee,PTRUSTEE_A pImpersonateTrustee)
{
}
/*
 * @unimplemented
 */
VOID STDCALL BuildImpersonateTrusteeW(PTRUSTEE_W pTrustee,PTRUSTEE_W pImpersonateTrustee)
{
}
/*
 * @unimplemented
 */
MULTIPLE_TRUSTEE_OPERATION STDCALL GetMultipleTrusteeOperationA(PTRUSTEE_A pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
MULTIPLE_TRUSTEE_OPERATION STDCALL GetMultipleTrusteeOperationW(PTRUSTEE_W pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
PTRUSTEE_A STDCALL GetMultipleTrusteeA(PTRUSTEE_A pTrustee)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
PTRUSTEE_W STDCALL GetMultipleTrusteeW(PTRUSTEE_W pTrustee)
{
  return(FALSE);
}
