/* $Id: semgr.c,v 1.14 2000/01/05 21:57:00 dwelch Exp $
 *
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

VOID SepReferenceLogonSession(PLUID AuthenticationId)
{
   UNIMPLEMENTED;
}

VOID SepDeReferenceLogonSession(PLUID AuthenticationId)
{
   UNIMPLEMENTED;
}

VOID SepFreeProxyData(PVOID ProxyData)
{
   UNIMPLEMENTED;
}

NTSTATUS SepCopyProxyData(PVOID* Dest, PVOID Src)
{
   UNIMPLEMENTED;
}

NTSTATUS SepDuplicationToken(PACCESS_TOKEN Token,
			     PULONG a,
			     ULONG b,
			     TOKEN_TYPE TokenType,
			     SECURITY_IMPERSONATION_LEVEL Level,
			     ULONG d,
			     PACCESS_TOKEN* e)
{
#if 0
   PVOID mem1;
   PVOID mem2;
   PVOID f;
   PACCESS_TOKEN g;
   NTSTATUS Status;
   
   if (TokenType == 2 && 
       (Level > 3 || Level < 0))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   SepReferenceLogonSession(&Token->AuthenticationId);
   
   mem1 = ExAllocatePool(NonPagedPool, Token->DynamicCharged);
   if (mem1 == NULL)
     {
	SepDeReferenceLogonSession(&Token->AuthenticationId);
	return(STATUS_UNSUCCESSFUL);
     }
   if (Token->ProxyData != NULL)
     {
	Status = SepCopyProxyData(&f, Token->ProxyData);
	if (!NT_SUCCESS(Status))
	  {
	     SepDeReferenceLogonSession(&Token->AuthenticationId);
	     ExFreePool(mem1);
	     return(Status);
	  }
     }
   else
     {
	f = 0;
     }
   if (Token->AuditData != NULL)
     {
	mem2 = ExAllocatePool(NonPagedPool, 0xc);
	if (mem2 == NULL)
	  {
	     SepFreeProxyData(f);
	     SepDeReferenceLogonSession(&Token->AuthenticationId);
	     ExFreePool(mem1);
	     return(STATUS_UNSUCCESSFUL);
	  }
	memcpy(mem2, Token->AuditData, 0xc);
     }
   else
     {
	mem2 = NULL;
     }
   
   Status = ObCreateObject(d,
			   SeTokenType,
			   b,
			   d,
			   0,
			   Token->VariableLength + 0x78,
			   Token->DynamicCharged,
			   Token->VariableLength + 0x78,
			   &g);
   if (!NT_SUCCESS(Status))
     {
	SepDeReferenceLogonSession(Token->AuthenticationId);
	ExFreePool(mem1);
	SepFreeProxyData(f);
	if (mem2 != NULL)
	  {
	     ExFreePool(mem2);
	  }
	return(Status);
     }
   
   g->TokenId = Token->TokenId;
   g->ModifiedId = Token->ModifiedId;
   g->ExpirationTime = Token->ExpirationTime;
   memcpy(&g->TokenSource, &Token->TokenSource, sizeof(TOKEN_SOURCE));
   g->DynamicCharged = Token->DynamicCharged;
   g->DynamicAvailable = Token->DynamicAvailable;
   g->DefaultOwnerIndex = Token->DefaultOwnerIndex;
   g->UserAndGroupCount = Token->UserAndGroupCount;
   g->PrivilegeCount = Token->PrivilegeCount;
   g->VariableLength = Token->VariableLength;
   g->TokenFlags = Token->TokenFlags;
   g->ProxyData = f;
   g->AuditData = mem2;
   //g->TokenId = ExInterlockedExchangeAdd();
   g->TokenInUse = 0;
   g->TokenType = TokenType;
   g->ImpersonationLevel = Level;
   memmove(g->VariablePart, Token->VariablePart, Token->VariableLength);
   /* ... */
   *e = g;
   return(STATUS_SUCCESS);
#endif
   UNIMPLEMENTED;   
}

NTSTATUS SeCopyClientToken(PACCESS_TOKEN Token,
			   SECURITY_IMPERSONATION_LEVEL Level,
			   ULONG a,
			   PACCESS_TOKEN* b)
{
   ULONG c;
   PACCESS_TOKEN d;
   NTSTATUS Status;
   
   c = 18;
   Status = SepDuplicationToken(Token,
				&c,
				0,
				TokenImpersonation,
				Level,
				a,
				&d);
   *b = d;
   return(Status);
}

NTSTATUS SeCreateClientSecurity(PETHREAD Thread,
				PSECURITY_QUALITY_OF_SERVICE Qos,
				ULONG e,
				PSE_SOME_STRUCT2 f)
{
   TOKEN_TYPE TokenType;
   UCHAR b;
   SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
   PACCESS_TOKEN Token;
   ULONG g;   
   PACCESS_TOKEN NewToken;
   
   Token = PsReferenceEffectiveToken(Thread,
				     &TokenType,
				     &b,
				     &ImpersonationLevel);
   if (TokenType != 2)
     {
	f->Unknown9 = Qos->EffectiveOnly;
     }
   else
     {
	if (Qos->ImpersonationLevel > ImpersonationLevel)
	  {
	     if (Token != NULL)
	       {
		  ObDereferenceObject(Token);
	       }
	     return(STATUS_UNSUCCESSFUL);
	  }
	if (ImpersonationLevel == 0 ||
	    ImpersonationLevel == 1 ||
	    (e != 0 && ImpersonationLevel != 3))
	  {
	     if (Token != NULL)
	       {
		  ObDereferenceObject(Token);
	       }
	     return(STATUS_UNSUCCESSFUL);
	  }
	if (b != 0 ||
	    Qos->EffectiveOnly != 0)
	  {
	     f->Unknown9 = 1;
	  }
	else
	  {
	     f->Unknown9 = 0;
	  }       
     }
   
   if (Qos->ContextTrackingMode == 0)
     {
	f->Unknown8 = 0;
	g = SeCopyClientToken(Token, ImpersonationLevel, 0, &NewToken);
	if (g >= 0)
	  {
//	     ObDeleteCapturedInsertInfo(NewToken);
	  }
	if (TokenType == TokenPrimary || Token != NULL)		 
	  {
	     ObDereferenceObject(Token);
	  }
	if (g < 0)
	  {
	     return(g);
	  }
     }
   else
     {
	f->Unknown8 = 1;
	if (e != 0)
	  {
//	     SeGetTokenControlInformation(Token, &f->Unknown11);
	  }
	NewToken = Token;
     }
   f->Unknown1 = 0xc;
   f->Level = Qos->ImpersonationLevel;
   f->ContextTrackingMode = Qos->ContextTrackingMode;
   f->EffectiveOnly = Qos->EffectiveOnly;
   f->Unknown10 = e;
   f->Token = NewToken;
   
   return(STATUS_SUCCESS);
}


VOID SeImpersonateClient(PSE_SOME_STRUCT2 a,
			 PETHREAD Thread)
{
   UCHAR b;
   
   if (a->Unknown8 == 0)
     {
	b = a->EffectiveOnly;
     }
   else
     {
	b = a->Unknown9;
     }
   if (Thread == NULL)
     {
	Thread = PsGetCurrentThread();
     }
   PsImpersonateClient(Thread, 
		       a->Token, 
		       1, 
		       (ULONG)b, 
		       a->Level);
}




NTSTATUS STDCALL NtPrivilegeCheck (IN	HANDLE		ClientToken,
				   IN	PPRIVILEGE_SET	RequiredPrivileges,  
				   IN	PBOOLEAN	Result)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtPrivilegedServiceAuditAlarm(
					     IN PUNICODE_STRING SubsystemName, 
					     IN	PUNICODE_STRING	ServiceName,   
					     IN	HANDLE		ClientToken,
					     IN	PPRIVILEGE_SET	Privileges,    
					     IN	BOOLEAN		AccessGranted)
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

VOID STDCALL SeReleaseSubjectContext (PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
   ObDereferenceObject(SubjectContext->PrimaryToken);
   if (SubjectContext->ClientToken != NULL)
     {
	ObDereferenceObject(SubjectContext->ClientToken);
     }   
}

VOID STDCALL SeCaptureSubjectContext (PSECURITY_SUBJECT_CONTEXT SubjectContext)
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
   
BOOLEAN STDCALL SePrivilegeCheck(PPRIVILEGE_SET Privileges,
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

BOOLEAN STDCALL SeSinglePrivilegeCheck(LUID PrivilegeValue,
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

NTSTATUS STDCALL SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
				  PSECURITY_DESCRIPTOR ExplicitDescriptor,
				  BOOLEAN IsDirectoryObject,
				  PSECURITY_SUBJECT_CONTEXT SubjectContext,
				  PGENERIC_MAPPING GenericMapping,
				  POOL_TYPE PoolType)
{
   UNIMPLEMENTED;
}

BOOLEAN SepSidInToken(PACCESS_TOKEN Token,
		      PSID Sid)
{
   ULONG i;
   
   if (Token->UserAndGroupCount == 0)
     {
	return(FALSE);
     }
   
   for (i=0; i<Token->UserAndGroupCount; i++)
     {
	if (RtlEqualSid(Sid, Token->UserAndGroups[i].Sid))
	  {
	     if (i == 0 ||
		 (!(Token->UserAndGroups[i].Attributes & 0x4)))
	       {
		  return(TRUE);
	       }
	     return(FALSE);
	  }
     }
   return(FALSE);
}

BOOLEAN STDCALL SeAccessCheck (IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
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
	     if (SepSidInToken(SubjectSecurityContext->ClientToken, Sid))
	       {
		  *AccessStatus = STATUS_ACCESS_DENIED;
		  *GrantedAccess = 0;
		  return(STATUS_SUCCESS);
	       }
	  }
	if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
	  {
	     if (SepSidInToken(SubjectSecurityContext->ClientToken, Sid))
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


/* EOF */
