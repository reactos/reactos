/*
 * Some of these functions may be wrong
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

/*
 * @unimplemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
AddAuditAccessAceEx(
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD dwAccessMask,
 PSID pSid,
 WINBOOL bAuditSuccess,
 WINBOOL bAuditFailure
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
AddAuditAccessObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid,
 WINBOOL bAuditSuccess,
 WINBOOL bAuditFailure
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
APIENTRY
CheckTokenMembership(
 HANDLE TokenHandle,
 PSID SidToCheck,
 PWINBOOL IsMember
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
WINBOOL
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
WINBOOL
STDCALL
ConvertToAutoInheritPrivateObjectSecurity(
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
 PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
 GUID *ObjectType,
 WINBOOL IsDirectoryObject,
 PGENERIC_MAPPING GenericMapping
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
CreatePrivateObjectSecurity (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 WINBOOL IsDirectoryObject,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
CreatePrivateObjectSecurityEx (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 GUID *ObjectType,
 WINBOOL IsContainerObject,
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
WINBOOL
STDCALL
CreatePrivateObjectSecurityWithMultipleInheritance (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 GUID **ObjectTypes,
 ULONG GuidCount,
 WINBOOL IsContainerObject,
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
WINBOOL
STDCALL
CreateProcessWithLogonW(
 LPCWSTR lpUsername,
 LPCWSTR lpDomain,
 LPCWSTR lpPassword,
 DWORD dwLogonFlags,
 LPCWSTR lpApplicationName,
 LPWSTR lpCommandLine,
 DWORD dwCreationFlags,
 LPVOID lpEnvironment,
 LPCWSTR lpCurrentDirectory,
 LPSTARTUPINFOW lpStartupInfo,
 LPPROCESS_INFORMATION lpProcessInformation
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
EqualDomainSid(
 PSID pSid1,
 PSID pSid2,
 WINBOOL *pfEqual
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
CreateProcessAsUserA (
 HANDLE hToken,
 LPCSTR lpApplicationName,
 LPSTR lpCommandLine,
 LPSECURITY_ATTRIBUTES lpProcessAttributes,
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 WINBOOL bInheritHandles,
 DWORD dwCreationFlags,
 LPVOID lpEnvironment,
 LPCSTR lpCurrentDirectory,
 LPSTARTUPINFOA lpStartupInfo,
 LPPROCESS_INFORMATION lpProcessInformation
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
GetCurrentHwProfileA (
 LPHW_PROFILE_INFOA lpHwProfileInfo
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
LogonUserA (
 LPCSTR lpszUsername,
 LPCSTR lpszDomain,
 LPCSTR lpszPassword,
 DWORD dwLogonType,
 DWORD dwLogonProvider,
 PHANDLE phToken
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
LogonUserExA (
 LPCSTR lpszUsername,
 LPCSTR lpszDomain,
 LPCSTR lpszPassword,
 DWORD dwLogonType,
 DWORD dwLogonProvider,
 PHANDLE phToken ,
 PSID *ppLogonSid ,
 PVOID *ppProfileBuffer ,
 LPDWORD pdwProfileLength ,
 PQUOTA_LIMITS pQuotaLimits
 )
{
  return(FALSE);
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
CreateProcessAsUserW (
 HANDLE hToken,
 LPCWSTR lpApplicationName,
 LPWSTR lpCommandLine,
 LPSECURITY_ATTRIBUTES lpProcessAttributes,
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 WINBOOL bInheritHandles,
 DWORD dwCreationFlags,
 LPVOID lpEnvironment,
 LPCWSTR lpCurrentDirectory,
 LPSTARTUPINFOW lpStartupInfo,
 LPPROCESS_INFORMATION lpProcessInformation
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
LogonUserW (
 LPCWSTR lpszUsername,
 LPCWSTR lpszDomain,
 LPCWSTR lpszPassword,
 DWORD dwLogonType,
 DWORD dwLogonProvider,
 PHANDLE phToken
 )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
LogonUserExW (
 LPCWSTR lpszUsername,
 LPCWSTR lpszDomain,
 LPCWSTR lpszPassword,
 DWORD dwLogonType,
 DWORD dwLogonProvider,
 PHANDLE phToken ,
 PSID *ppLogonSid ,
 PVOID *ppProfileBuffer ,
 LPDWORD pdwProfileLength ,
 PQUOTA_LIMITS pQuotaLimits
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
WINBOOL
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
WINBOOL
STDCALL
InitiateSystemShutdownExA(LPSTR lpMachineName,LPSTR lpMessage,DWORD dwTimeout,BOOL bForceAppsClosed,BOOL bRebootAfterShutdown,DWORD dwReason)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
InitiateSystemShutdownExW(LPWSTR lpMachineName,LPWSTR lpMessage,DWORD dwTimeout,BOOL bForceAppsClosed,BOOL bRebootAfterShutdown,DWORD dwReason)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
NotifyBootConfigStatus(WINBOOL BootAcceptable)
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL ObjectCreation,
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
WINBOOL
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
WINBOOL ObjectCreation,
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
WINBOOL
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
WINBOOL ObjectCreation,
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
WINBOOL
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
WINBOOL ObjectCreation,
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
WINBOOL
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
WINBOOL ObjectCreation,
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
WINBOOL
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
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
)
{
  return(FALSE);
}
