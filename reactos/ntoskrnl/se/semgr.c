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



NTSTATUS STDCALL NtPrivilegeCheck (IN	HANDLE		ClientToken,
				   IN	PPRIVILEGE_SET	RequiredPrivileges,  
				   IN	PBOOLEAN	Result)
{
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtPrivilegedServiceAuditAlarm (
	IN	PUNICODE_STRING	SubsystemName,	
	IN	PUNICODE_STRING	ServiceName,	
	IN	HANDLE		ClientToken,
	IN	PPRIVILEGE_SET	Privileges,	
	IN	BOOLEAN		AccessGranted 	
	)
{
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtPrivilegeObjectAuditAlarm (
	IN	PUNICODE_STRING	SubsystemName,
	IN	PVOID		HandleId,	
	IN	HANDLE		ClientToken,
	IN	ULONG		DesiredAccess,
	IN	PPRIVILEGE_SET	Privileges,
	IN	BOOLEAN		AccessGranted 
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenObjectAuditAlarm (
	IN	PUNICODE_STRING		SubsystemName,	
	IN	PVOID			HandleId,	
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	HANDLE			ClientToken,	
	IN	ULONG			DesiredAccess,	
	IN	ULONG			GrantedAccess,	
	IN	PPRIVILEGE_SET		Privileges,
	IN	BOOLEAN			ObjectCreation,	
	IN	BOOLEAN			AccessGranted,	
	OUT	PBOOLEAN		GenerateOnClose 	
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenProcessToken (  
	IN	HANDLE		ProcessHandle,
	IN	ACCESS_MASK	DesiredAccess,  
	OUT	PHANDLE		TokenHandle  
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenThreadToken (  
	IN	HANDLE		ThreadHandle,  
  	IN	ACCESS_MASK	DesiredAccess,  
  	IN	BOOLEAN		OpenAsSelf,     
  	OUT	PHANDLE		TokenHandle  
	)
{
	UNIMPLEMENTED;
}




NTSTATUS STDCALL NtImpersonateThread (IN HANDLE ThreadHandle,
				     IN	HANDLE ThreadToImpersonate,
				     IN	PSECURITY_QUALITY_OF_SERVICE	
				          SecurityQualityOfService)
{
   UNIMPLEMENTED;
}



NTSTATUS
STDCALL
NtAccessCheckAndAuditAlarm (
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
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtAdjustGroupsToken (
	IN	HANDLE		TokenHandle,
	IN	BOOLEAN		ResetToDefault,	
	IN	PTOKEN_GROUPS	NewState, 
	IN	ULONG		BufferLength,	
	OUT	PTOKEN_GROUPS	PreviousState		OPTIONAL,	
	OUT	PULONG		ReturnLength
	)
{
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtAdjustPrivilegesToken(IN HANDLE  TokenHandle,	
					 IN BOOLEAN  DisableAllPrivileges,
					 IN PTOKEN_PRIVILEGES  NewState,	
					 IN ULONG  BufferLength,	
					 OUT PTOKEN_PRIVILEGES  PreviousState,	
					 OUT PULONG ReturnLength)
{
	UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtAllocateUuids (
	PLARGE_INTEGER	Time,
	PULONG		Version, // ???
	PULONG		ClockCycle
	)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtCloseObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,	
					 IN PVOID HandleId,	
					 IN BOOLEAN GenerateOnClose)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			       IN HANDLE ClientToken,
			       IN ACCESS_MASK DesiredAccess,
			       IN PGENERIC_MAPPING GenericMapping,
                               OUT PPRIVILEGE_SET PrivilegeSet,
			       OUT PULONG ReturnLength,
			       OUT PULONG GrantedAccess,
			       OUT PBOOLEAN AccessStatus)
{
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtDeleteObjectAuditAlarm ( 
	IN PUNICODE_STRING SubsystemName, 
	IN PVOID HandleId, 
	IN BOOLEAN GenerateOnClose 
	)
{
 UNIMPLEMENTED;
}

VOID SeReleaseSubjectContext(PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
   
}

VOID SeCaptureSubjectContext(PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
   PEPROCESS Process;
   ULONG a;
   ULONG b;
   
   Process = PsGetCurrentThread()->ThreadsProcess;
   
   SubjectContext->ProcessAuditId = Process;
   SubjectContext->ClientToken = 
     PsReferenceImpersonationToken(PsGetCurrentThread(),
				   &a,
				   &b,
				   &SubjectContext->ImpersonationLevel);
   SubjectContext->PrimaryToken = PsReferencePrimaryToken(Process);
}

BOOLEAN SepPrivilegeCheck(PACCESS_TOKEN Token,
			  PLUID_AND_ATTRIBUTES Privileges,
			  ULONG PrivilegeCount,
			  ULONG PrivilegeControl,
			  KPROCESSOR_MODE PreviousMode)
{
   ULONG i;
   PLUID_AND_ATTRIBUTES Current;
   ULONG j;
   ULONG k;
   
   if (PreviousMode == KernelMode)
     {
	return(TRUE);
     }
   
   j = 0;
   if (PrivilegeCount != 0)
     {
	k = PrivilegeCount;
	do
	  {
	     i = Token->PrivilegeCount;
	     Current = Token->Privileges;
	     for (i = 0; i < Token->PrivilegeCount; i++)
	       {
		  if (!(Current[i].Attributes & 2) &&
		      Privileges[i].Luid.u.LowPart == 
		      Current[i].Luid.u.LowPart &&
		      Privileges[i].Luid.u.HighPart == 
		      Current[i].Luid.u.HighPart)
		    {
		       Privileges[i].Attributes = 
			 Privileges[i].Attributes | 0x80;
		       j++;
		       break;
		    }
	       }
	     k--;
	  } while (k > 0);
     }
   
   if ((PrivilegeControl & 0x2) && PrivilegeCount == j)       
     {
	return(TRUE);
     }
       
   if (j > 0 && !(PrivilegeControl & 0x2))
     {
	return(TRUE);
     }

   return(FALSE);
}
   
BOOLEAN SePrivilegeCheck(PPRIVILEGE_SET Privileges,
			 PSECURITY_SUBJECT_CONTEXT SubjectContext,
			 KPROCESSOR_MODE PreviousMode)
{
   PACCESS_TOKEN Token = NULL;
   
   if (SubjectContext->ClientToken == NULL)
     {
	Token = SubjectContext->PrimaryToken;
     }
   else
     {
	Token = SubjectContext->ClientToken;
	if (SubjectContext->ImpersonationLevel < 2)
	  {
	     return(FALSE);
	  }
     }
   
   return(SepPrivilegeCheck(Token,
			    Privileges->Privilege,
			    Privileges->PrivilegeCount,
			    Privileges->Control,
			    PreviousMode));			    
}

BOOLEAN SeSinglePrivilegeCheck(LUID PrivilegeValue,
			       KPROCESSOR_MODE PreviousMode)
{
   SECURITY_SUBJECT_CONTEXT SubjectContext;
   BOOLEAN r;
   PRIVILEGE_SET Priv;
   
   SeCaptureSubjectContext(&SubjectContext);
   
   Priv.PrivilegeCount = 1;
   Priv.Control = 1;
   Priv.Privilege[0].Luid = PrivilegeValue;
   Priv.Privilege[0].Attributes = 0;
   
   r = SePrivilegeCheck(&Priv,
			&SubjectContext,
			PreviousMode);
      
   if (PreviousMode != KernelMode)
     {
/*	SePrivilegeServiceAuditAlarm(0,
				     &SubjectContext,
				     &PrivilegeValue);*/
     }
   SeReleaseSubjectContext(&SubjectContext);
   return(r);
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
 *      SecurityDescriptor = Security descriptor protecting the object
 *      SubjectSecurityContext = Subject's captured security context
 *      SubjectContextLocked = Indicates the user's subject context is locked
 *      DesiredAccess = Access rights the caller is trying to acquire
 *      PreviouslyGrantedAccess = Specified the access rights already granted
 *      Privileges = ?
 *      GenericMapping = Generic mapping associated with the object
 *      AccessMode = Access mode used for the check
 *      GrantedAccess (OUT) = On return specifies the access granted
 *      AccessStatus (OUT) = Status indicating why access was denied
 * RETURNS: If access was granted, returns TRUE
 */
{
   ULONG i;
   PACL Dacl;
   BOOLEAN Present;
   BOOLEAN Defaulted;
   NTSTATUS Status;
   PACE CurrentAce;
   PSID Sid;
   ACCESS_MASK CurrentAccess;
   
   CurrentAccess = PreviouslyGrantedAccess;
   
   /*    
    * Ignore the SACL for now
    */
   
   /*
    * Check the DACL
    */
   Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
					 &Present,
					 &Dacl,
					 &Defaulted);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   CurrentAce = (PACE)(Dacl + 1);
   for (i = 0; i < Dacl->AceCount; i++)
     {
	Sid = (PSID)(CurrentAce + 1);
	if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
	  {
	     if (RtlEqualSid(Sid, NULL))
	       {
		  *AccessStatus = STATUS_ACCESS_DENIED;
		  *GrantedAccess = 0;
		  return(STATUS_SUCCESS);
	       }
	  }
	if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
	  {
	     if (RtlEqualSid(Sid, NULL))
	       {
		  CurrentAccess = CurrentAccess | 
		    CurrentAce->Header.AccessMask;		  
	       }
	  }
     }
   if (!(CurrentAccess & DesiredAccess) &&
       !((~CurrentAccess) & DesiredAccess))
     {
	*AccessStatus = STATUS_ACCESS_DENIED;	
     }
   else
     {
	*AccessStatus = STATUS_SUCCESS;
     }
   *GrantedAccess = CurrentAccess;
   
   return(STATUS_SUCCESS);
}


