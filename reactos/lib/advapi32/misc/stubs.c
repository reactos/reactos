/*
 * Some of these functions may be wrong
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <string.h>

/*
 * @unimplemented
 */
BOOL
STDCALL
DeregisterEventSource (
		       HANDLE hEventLog
			)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterEventSourceA (
    LPCSTR lpUNCServerName,
    LPCSTR lpSourceName
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterEventSourceW (
    LPCWSTR lpUNCServerName,
    LPCWSTR lpSourceName
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReportEventA (
     HANDLE     hEventLog,
     WORD       wType,
     WORD       wCategory,
     DWORD      dwEventID,
     PSID       lpUserSid,
     WORD       wNumStrings,
     DWORD      dwDataSize,
     LPCSTR   *lpStrings,
     LPVOID     lpRawData
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReportEventW (
     HANDLE     hEventLog,
     WORD       wType,
     WORD       wCategory,
     DWORD      dwEventID,
     PSID       lpUserSid,
     WORD       wNumStrings,
     DWORD      dwDataSize,
     LPCWSTR   *lpStrings,
     LPVOID     lpRawData
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetFileSecurityW (
    LPCWSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ChangeServiceConfig2A(
    SC_HANDLE    hService,
    DWORD        dwInfoLevel,
    LPVOID       lpInfo
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfig2A(
    SC_HANDLE   hService,
    DWORD       dwInfoLevel,
    LPBYTE      lpBuffer,
    DWORD       cbBufSize,
    LPDWORD     pcbBytesNeeded
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
SERVICE_STATUS_HANDLE
STDCALL
RegisterServiceCtrlHandlerExA(
    LPCSTR                lpServiceName,
    LPHANDLER_FUNCTION_EX   lpHandlerProc,
    LPVOID                  lpContext
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ChangeServiceConfig2W(
    SC_HANDLE    hService,
    DWORD        dwInfoLevel,
    LPVOID       lpInfo
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfig2W(
    SC_HANDLE   hService,
    DWORD       dwInfoLevel,
    LPBYTE      lpBuffer,
    DWORD       cbBufSize,
    LPDWORD     pcbBytesNeeded
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
SERVICE_STATUS_HANDLE
STDCALL
RegisterServiceCtrlHandlerExW(
    LPCWSTR                lpServiceName,
    LPHANDLER_FUNCTION_EX   lpHandlerProc,
    LPVOID                  lpContext
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddAccessAllowedAceEx (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 PSID pSid
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddAccessAllowedObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddAccessDeniedAceEx (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 PSID pSid
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddAccessDeniedObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddAuditAccessAceEx(
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD dwAccessMask,
 PSID pSid,
 BOOL bAuditSuccess,
 BOOL bAuditFailure
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddAuditAccessObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid,
 BOOL bAuditSuccess,
 BOOL bAuditFailure
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
CheckTokenMembership(
 HANDLE TokenHandle,
 PSID SidToCheck,
 PBOOL IsMember
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID
STDCALL
CloseEncryptedFileRaw(
 PVOID pvContext
 )
{
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CloseEventLog (
 HANDLE hEventLog
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ConvertToAutoInheritPrivateObjectSecurity(
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
 PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
 GUID *ObjectType,
 BOOL IsDirectoryObject,
 PGENERIC_MAPPING GenericMapping
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreatePrivateObjectSecurity (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 BOOL IsDirectoryObject,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreatePrivateObjectSecurityEx (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 GUID *ObjectType,
 BOOL IsContainerObject,
 ULONG AutoInheritFlags,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreatePrivateObjectSecurityWithMultipleInheritance (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 GUID **ObjectTypes,
 ULONG GuidCount,
 BOOL IsContainerObject,
 ULONG AutoInheritFlags,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
CreateRestrictedToken(
 HANDLE ExistingTokenHandle,
 DWORD Flags,
 DWORD DisableSidCount,
 PSID_AND_ATTRIBUTES SidsToDisable,
 DWORD DeletePrivilegeCount,
 PLUID_AND_ATTRIBUTES PrivilegesToDelete,
 DWORD RestrictedSidCount,
 PSID_AND_ATTRIBUTES SidsToRestrict,
 PHANDLE NewTokenHandle
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreateWellKnownSid(
 WELL_KNOWN_SID_TYPE WellKnownSidType,
 PSID DomainSid ,
 PSID pSid,
 DWORD *cbSid
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DestroyPrivateObjectSecurity (
 PSECURITY_DESCRIPTOR * ObjectDescriptor
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EqualDomainSid(
 PSID pSid1,
 PSID pSid2,
 BOOL *pfEqual
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetEventLogInformation (
 HANDLE hEventLog,
 DWORD dwInfoLevel,
 LPVOID lpBuffer,
 DWORD cbBufSize,
 LPDWORD pcbBytesNeeded
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumberOfEventLogRecords (
 HANDLE hEventLog,
 PDWORD NumberOfRecords
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetOldestEventLogRecord (
 HANDLE hEventLog,
 PDWORD OldestRecord
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetPrivateObjectSecurity (
 PSECURITY_DESCRIPTOR ObjectDescriptor,
 SECURITY_INFORMATION SecurityInformation,
 PSECURITY_DESCRIPTOR ResultantDescriptor,
 DWORD DescriptorLength,
 PDWORD ReturnLength
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetSecurityDescriptorRMControl(
 PSECURITY_DESCRIPTOR SecurityDescriptor,
 PUCHAR RMControl
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetWindowsAccountDomainSid(
 PSID pSid,
 PSID ppDomainSid,
 DWORD *cbSid
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
ImpersonateAnonymousToken(
 HANDLE ThreadHandle
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ImpersonateNamedPipeClient(
 HANDLE hNamedPipe
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsTextUnicode(
 CONST VOID* lpBuffer,
 int cb,
 LPINT lpi
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsTokenRestricted(
 HANDLE TokenHandle
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsTokenUntrusted(
 HANDLE TokenHandle
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsWellKnownSid (
 PSID pSid,
 WELL_KNOWN_SID_TYPE WellKnownSidType
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
MakeAbsoluteSD2 (
 PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
 LPDWORD lpdwBufferSize
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
NotifyChangeEventLog(
 HANDLE hEventLog,
 HANDLE hEvent
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
ReadEncryptedFileRaw(
 PFE_EXPORT_FUNC pfExportCallback,
 PVOID pvCallbackContext,
 PVOID pvContext
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
WriteEncryptedFileRaw(
 PFE_IMPORT_FUNC pfImportCallback,
 PVOID pvCallbackContext,
 PVOID pvContext
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetPrivateObjectSecurity (
 SECURITY_INFORMATION SecurityInformation,
 PSECURITY_DESCRIPTOR ModificationDescriptor,
 PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
 PGENERIC_MAPPING GenericMapping,
 HANDLE Token
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetPrivateObjectSecurityEx (
 SECURITY_INFORMATION SecurityInformation,
 PSECURITY_DESCRIPTOR ModificationDescriptor,
 PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
 ULONG AutoInheritFlags,
 PGENERIC_MAPPING GenericMapping,
 HANDLE Token
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetSecurityDescriptorControl (
 PSECURITY_DESCRIPTOR pSecurityDescriptor,
 SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
 SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
SetSecurityDescriptorRMControl(
 PSECURITY_DESCRIPTOR SecurityDescriptor,
 PUCHAR RMControl
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
BackupEventLogA (
 HANDLE hEventLog,
 LPCSTR lpBackupFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ClearEventLogA (
 HANDLE hEventLog,
 LPCSTR lpBackupFileName
 )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DecryptFileA(
 LPCSTR lpFileName,
 DWORD dwReserved
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EncryptFileA(
 LPCSTR lpFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FileEncryptionStatusA(
 LPCSTR lpFileName,
 LPDWORD lpStatus
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetCurrentHwProfileA (
 LPHW_PROFILE_INFOA lpHwProfileInfo
 )
{
  lpHwProfileInfo->dwDockInfo = 2 /*DOCKINFO_DOCKED*/;
  strcpy(lpHwProfileInfo->szHwProfileGuid,"{12340001-1234-1234-1234-1233456789012}");
  strcpy(lpHwProfileInfo->szHwProfileName,"ReactOS Profile");
  return(TRUE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenBackupEventLogA (
 LPCSTR lpUNCServerName,
 LPCSTR lpFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
OpenEncryptedFileRawA(
 LPCSTR lpFileName,
 ULONG ulFlags,
 PVOID * pvContext
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenEventLogA (
 LPCSTR lpUNCServerName,
 LPCSTR lpSourceName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReadEventLogA (
 HANDLE hEventLog,
 DWORD dwReadFlags,
 DWORD dwRecordOffset,
 LPVOID lpBuffer,
 DWORD nNumberOfBytesToRead,
 DWORD *pnBytesRead,
 DWORD *pnMinNumberOfBytesNeeded
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
BackupEventLogW (
 HANDLE hEventLog,
 LPCWSTR lpBackupFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ClearEventLogW (
 HANDLE hEventLog,
 LPCWSTR lpBackupFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DecryptFileW(
 LPCWSTR lpFileName,
 DWORD dwReserved
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EncryptFileW(
 LPCWSTR lpFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FileEncryptionStatusW(
 LPCWSTR lpFileName,
 LPDWORD lpStatus
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetCurrentHwProfileW (
 LPHW_PROFILE_INFOW lpHwProfileInfo
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenBackupEventLogW (
 LPCWSTR lpUNCServerName,
 LPCWSTR lpFileName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
OpenEncryptedFileRawW(
 LPCWSTR lpFileName,
 ULONG ulFlags,
 PVOID * pvContext
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenEventLogW (
 LPCWSTR lpUNCServerName,
 LPCWSTR lpSourceName
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReadEventLogW (
 HANDLE hEventLog,
 DWORD dwReadFlags,
 DWORD dwRecordOffset,
 LPVOID lpBuffer,
 DWORD nNumberOfBytesToRead,
 DWORD *pnBytesRead,
 DWORD *pnMinNumberOfBytesNeeded
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
InitiateSystemShutdownExA(LPSTR lpMachineName,LPSTR lpMessage,DWORD dwTimeout,BOOL bForceAppsClosed,BOOL bRebootAfterShutdown,DWORD dwReason)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
InitiateSystemShutdownExW(LPWSTR lpMachineName,LPWSTR lpMessage,DWORD dwTimeout,BOOL bForceAppsClosed,BOOL bRebootAfterShutdown,DWORD dwReason)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
NotifyBootConfigStatus(BOOL BootAcceptable)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
RegDisablePredefinedCache(VOID)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
RegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
RegOpenUserClassesRoot(HANDLE hToken,DWORD  dwOptions,REGSAM samDesired,PHKEY  phkResult)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
RegOverridePredefKey (HKEY hKey,HKEY hNewHKey)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
RegSaveKeyExA (HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
RegSaveKeyExW (HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
LONG
STDCALL
Wow64Win32ApiEntry (DWORD dwFuncNumber,DWORD dwFlag,DWORD dwRes)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByType(
PSECURITY_DESCRIPTOR pSecurityDescriptor,
PSID PrincipalSelfSid,
HANDLE ClientToken,
DWORD DesiredAccess,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
PPRIVILEGE_SET PrivilegeSet,
LPDWORD PrivilegeSetLength,
LPDWORD GrantedAccess,
LPBOOL AccessStatus
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeResultList(
PSECURITY_DESCRIPTOR pSecurityDescriptor,
PSID PrincipalSelfSid,
HANDLE ClientToken,
DWORD DesiredAccess,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
PPRIVILEGE_SET PrivilegeSet,
LPDWORD PrivilegeSetLength,
LPDWORD GrantedAccessList,
LPDWORD AccessStatusList
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeAndAuditAlarmA(
LPCSTR SubsystemName,
LPVOID HandleId,
LPCSTR ObjectTypeName,
LPCSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
BOOL ObjectCreation,
LPDWORD GrantedAccess,
LPBOOL AccessStatus,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeAndAuditAlarmW(
LPCWSTR SubsystemName,
LPVOID HandleId,
LPCWSTR ObjectTypeName,
LPCWSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
BOOL ObjectCreation,
LPDWORD GrantedAccess,
LPBOOL AccessStatus,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmA(
LPCSTR SubsystemName,
LPVOID HandleId,
LPCSTR ObjectTypeName,
LPCSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
BOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmW(
LPCWSTR SubsystemName,
LPVOID HandleId,
LPCWSTR ObjectTypeName,
LPCWSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
BOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmByHandleA(
LPCSTR SubsystemName,
LPVOID HandleId,
HANDLE ClientToken,
LPCSTR ObjectTypeName,
LPCSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
BOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmByHandleW(
LPCWSTR SubsystemName,
LPVOID HandleId,
HANDLE ClientToken,
LPCWSTR ObjectTypeName,
LPCWSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
BOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
VOID STDCALL MD4Init(PMD4_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
VOID STDCALL MD4Update(PMD4_CONTEXT Context, PVOID Buffer, UINT BufferSize)
{
}

/*
 * @unimplemented
 */
VOID STDCALL MD4Final(PMD4_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
VOID STDCALL MD5Init(PMD5_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
VOID STDCALL MD5Update(PMD5_CONTEXT Context, PVOID Buffer, UINT BufferSize)
{
}

/*
 * @unimplemented
 */
VOID STDCALL MD5Final(PMD5_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
VOID STDCALL A_SHAInit(PSHA_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
VOID STDCALL A_SHAUpdate(PSHA_CONTEXT Context, PVOID Buffer, UINT BufferSize)
{
}

/*
 * @unimplemented
 */
VOID STDCALL A_SHAFinal(PSHA_CONTEXT Context, PVOID Result)
{
}

/*
 * @unimplemented
 */
BOOL STDCALL SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,DWORD x4 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
BOOL STDCALL QueryWindows31FilesMigration( DWORD x1 )
{
  return(FALSE);
}
