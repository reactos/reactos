/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/semgr.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL ZwQueryInformationToken(IN HANDLE TokenHandle,     
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,  
  	IN ULONG TokenInformationLength,
  	OUT PULONG ReturnLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryInformationToken(IN HANDLE TokenHandle,     
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,  
  	IN ULONG TokenInformationLength,
  	OUT PULONG ReturnLength)
{
   return(ZwQueryInformationToken(TokenHandle,
				  TokenInformationClass,
				  TokenInformation,
				  TokenInformationLength,
				  ReturnLength));
}

NTSTATUS STDCALL ZwQuerySecurityObject(IN HANDLE Object,
				       IN CINT SecurityObjectInformationClass,
				       OUT PVOID SecurityObjectInformation,
				       IN ULONG Length,
				       OUT PULONG ReturnLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQuerySecurityObject(IN HANDLE Object,
				       IN CINT SecurityObjectInformationClass,
				       OUT PVOID SecurityObjectInformation,
				       IN ULONG Length,
				       OUT PULONG ReturnLength)
{
   return(ZwQuerySecurityObject(Object,
				SecurityObjectInformationClass,
				SecurityObjectInformation,
				Length,
				ReturnLength));
}

NTSTATUS
STDCALL
NtSetSecurityObject(
	IN HANDLE Handle, 
	IN SECURITY_INFORMATION SecurityInformation, 
	IN PSECURITY_DESCRIPTOR SecurityDescriptor 
	)
{
}

NTSTATUS
STDCALL
NtSetInformationToken(
	IN HANDLE TokenHandle,            
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,       
	IN ULONG TokenInformationLength   
	)
{
}

NTSTATUS
STDCALL
NtPrivilegeCheck(
	IN HANDLE ClientToken,             
	IN PPRIVILEGE_SET RequiredPrivileges,  
	IN PBOOLEAN Result                    
	)
{
}

NTSTATUS
STDCALL
NtPrivilegedServiceAuditAlarm(
	IN PUNICODE_STRING SubsystemName,	
	IN PUNICODE_STRING ServiceName,	
	IN HANDLE ClientToken,
	IN PPRIVILEGE_SET Privileges,	
	IN BOOLEAN AccessGranted 	
	)
{
}

NTSTATUS
STDCALL
NtPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,	
	IN HANDLE ClientToken,
	IN ULONG DesiredAccess,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted 
	)
{
}

NTSTATUS
STDCALL
NtOpenObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,	
	IN PVOID HandleId,	
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE ClientToken,	
	IN ULONG DesiredAccess,	
	IN ULONG GrantedAccess,	
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN ObjectCreation,	
	IN BOOLEAN AccessGranted,	
	OUT PBOOLEAN GenerateOnClose 	
	)
{
}

NTSTATUS
STDCALL
NtOpenProcessToken(  
	IN HANDLE ProcessHandle, 
	IN ACCESS_MASK DesiredAccess,  
	OUT PHANDLE TokenHandle  
	)
{
}

NTSTATUS
STDCALL
NtOpenThreadToken(  
	IN HANDLE ThreadHandle,  
  	IN ACCESS_MASK DesiredAccess,  
  	IN BOOLEAN OpenAsSelf,     
  	OUT PHANDLE TokenHandle  
	)
{
}

NTSTATUS
STDCALL
NtDuplicateToken(  
	IN HANDLE ExistingToken, 
  	IN ACCESS_MASK DesiredAccess, 
 	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
  	IN TOKEN_TYPE TokenType,  
  	OUT PHANDLE NewToken     
	)
{
}


NTSTATUS STDCALL NtImpersonateClientOfPort(VOID)
{
}

NTSTATUS
STDCALL 
NtImpersonateThread(
	IN HANDLE ThreadHandle,
	IN HANDLE ThreadToImpersonate,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
	)
{
}

NTSTATUS STDCALL NtCreateToken(VOID)
{
}

NTSTATUS
STDCALL
NtDeleteObjectAuditAlarm ( 
	IN PUNICODE_STRING SubsystemName, 
	IN PVOID HandleId, 
	IN BOOLEAN GenerateOnClose 
	)
{
}


NTSTATUS
STDCALL
NtAllocateLocallyUniqueId(
	OUT LUID *LocallyUniqueId
	)
{
}

NTSTATUS
STDCALL
ZwAllocateLocallyUniqueId(
	OUT LUID *LocallyUniqueId
	)
{
}

NTSTATUS
STDCALL
NtAccessCheckAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PHANDLE ObjectHandle,	
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ACCESS_MASK DesiredAccess,	
	IN PGENERIC_MAPPING GenericMapping,	
	IN BOOLEAN ObjectCreation,	
	OUT PULONG GrantedAccess,	
	OUT PBOOLEAN AccessStatus,	
	OUT PBOOLEAN GenerateOnClose 	
	)
{
}

NTSTATUS 
STDCALL 
NtAdjustGroupsToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN  ResetToDefault,	
	IN PTOKEN_GROUPS  NewState, 
	IN ULONG  BufferLength,	
	OUT PTOKEN_GROUPS  PreviousState OPTIONAL,	
	OUT PULONG  ReturnLength 	
	)
{
}

NTSTATUS 
STDCALL 
NtAdjustPrivilegesToken(
	IN HANDLE  TokenHandle,	
	IN BOOLEAN  DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES  NewState,	
	IN ULONG  BufferLength,	
	OUT PTOKEN_PRIVILEGES  PreviousState,	
	OUT PULONG ReturnLength 	
	)
{
}
NTSTATUS 
STDCALL 
ZwAdjustPrivilegesToken(
	IN HANDLE  TokenHandle,	
	IN BOOLEAN  DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES  NewState,	
	IN ULONG  BufferLength,	
	OUT PTOKEN_PRIVILEGES  PreviousState,	
	OUT PULONG ReturnLength 	
	)
{
}

NTSTATUS 
STDCALL 
NtAllocateUuids(
	PLARGE_INTEGER Time,
	PULONG Version, // ???
	PULONG ClockCycle
	)
{
}

NTSTATUS 
STDCALL 
ZwAllocateUuids(
	PLARGE_INTEGER Time,
	PULONG Version, // ???
	PULONG ClockCycle
	)
{
}

NTSTATUS
STDCALL
NtCloseObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,	
	IN PVOID HandleId,	
	IN BOOLEAN GenerateOnClose 	
	)
{
}

NTSTATUS
STDCALL
NtAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ACCESS_MASK DesiredAcces,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus
	)
{
}

NTSTATUS
STDCALL
ZwAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ACCESS_MASK DesiredAcces,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus
	)
{
}

NTSTATUS RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				     ULONG Revision)
{
   UNIMPLEMENTED;
}

ULONG RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				      BOOLEAN DaclPresent,
				      PACL Dacl,
				      BOOLEAN DaclDefaulted)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   UNIMPLEMENTED;
}

BOOLEAN SeSinglePrivilegeCheck(LUID PrivilegeValue,
			       KPROCESSOR_MODE PreviousMode)
{
   UNIMPLEMENTED;
}

NTSTATUS SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor)
{
   UNIMPLEMENTED;
}

NTSTATUS SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
			  PSECURITY_DESCRIPTOR ExplicitDescriptor,
			  BOOLEAN IsDirectoryObject,
			  PSECURITY_SUBJECT_CONTEXT SubjectContext,
			  PGENERIC_MAPPING GenericMapping,
			  POOL_TYPE PoolType)
{
   UNIMPLEMENTED;
}

BOOLEAN SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN PSECURITY_DESCRIPTOR_CONTEXT SubjectSecurityContext,
		      IN BOOLEAN SubjectContextLocked,
		      IN ACCESS_MASK DesiredAccess,
		      IN ACCESS_MASK PreviouslyGrantedAccess,
		      OUT PPRIVILEGE_SET* Privileges,
		      IN PGENERIC_MAPPING GenericMapping,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PACCESS_MODE GrantedAccess,
		      OUT PNTSTATUS AccessStatus)
/*
 * FUNCTION: Determines whether the requested access rights can be granted
 * to an object protected by a security descriptor and an object owner
 * ARGUMENTS:
 *      SecurityDescriptor = Security descriptor protected the object
 *      SubjectSecurityContext = Subject's captured security context
 *      SubjectContextLocked = Indicates the user's subject context is locked
 *      DesiredAccess = Access rights the caller is trying to acquire
 *      PreviouslyGrantedAccess = Specified the access rights already granted
 *      Priveleges = ?
 *      GenericMapping = Generic mapping associated with the object
 *      AccessMode = Access mode used for the check
 *      GrantedAccess (OUT) = On return specifies the access granted
 *      AccessStatus (OUT) = Status indicating why access was denied
 * RETURNS: If access was granted, returns TRUE
 */
{
   UNIMPLEMENTED;
}


