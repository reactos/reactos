/* $Id: token.c,v 1.7 2000/06/29 23:35:45 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/token.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED SeTokenType = NULL;

/* FUNCTIONS *****************************************************************/

VOID SepFreeProxyData(PVOID ProxyData)
{
   UNIMPLEMENTED;
}

NTSTATUS SepCopyProxyData(PVOID* Dest, PVOID Src)
{
   UNIMPLEMENTED;
}

NTSTATUS SeExchangePrimaryToken(PEPROCESS Process,
				PACCESS_TOKEN NewToken,
				PACCESS_TOKEN* OldTokenP)
{
   PACCESS_TOKEN OldToken;
   
   if (NewToken->TokenType != TokenPrimary)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (NewToken->TokenInUse != 0)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   OldToken = Process->Token;
   Process->Token = NewToken;
   NewToken->TokenInUse = 1;
   ObReferenceObjectByPointer(NewToken,
			      GENERIC_ALL,
			      SeTokenType,
			      KernelMode);
   OldToken->TokenInUse = 0;
   *OldTokenP = OldToken;
   return(STATUS_SUCCESS);
}

NTSTATUS SepDuplicationToken(PACCESS_TOKEN Token,
			     POBJECT_ATTRIBUTES ObjectAttributes,
			     TOKEN_TYPE TokenType,
			     SECURITY_IMPERSONATION_LEVEL Level,
			     SECURITY_IMPERSONATION_LEVEL ExistingLevel,
			     KPROCESSOR_MODE PreviousMode,
			     PACCESS_TOKEN* NewAccessToken)
{
   UNIMPLEMENTED;
}

NTSTATUS SeCopyClientToken(PACCESS_TOKEN Token,
			   SECURITY_IMPERSONATION_LEVEL Level,
			   KPROCESSOR_MODE PreviousMode,
			   PACCESS_TOKEN* NewToken)
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
     
   InitializeObjectAttributes(&ObjectAttributes,
			      NULL,
			      0,
			      NULL,
			      NULL);
   Status = SepDuplicationToken(Token,
				&ObjectAttributes,
				0,
				SecurityIdentification,
				Level,
				PreviousMode,
				NewToken);
   return(Status);
}

NTSTATUS
STDCALL
SeCreateClientSecurity (
	PETHREAD			Thread,
	PSECURITY_QUALITY_OF_SERVICE	Qos,
	ULONG				e,
	PSE_SOME_STRUCT2		f
	)
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


VOID
STDCALL
SeImpersonateClient (
	PSE_SOME_STRUCT2	a,
	PETHREAD		Thread
	)
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

VOID SeInitializeTokenManager(VOID)
{
   UNICODE_STRING TypeName;
   
   RtlInitUnicodeString(&TypeName, L"Token");
   
   SeTokenType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   SeTokenType->MaxObjects = ULONG_MAX;
   SeTokenType->MaxHandles = ULONG_MAX;
   SeTokenType->TotalObjects = 0;
   SeTokenType->TotalHandles = 0;
   SeTokenType->PagedPoolCharge = 0;
   SeTokenType->NonpagedPoolCharge = 0;
   SeTokenType->Dump = NULL;
   SeTokenType->Open = NULL;
   SeTokenType->Close = NULL;
   SeTokenType->Delete = NULL;
   SeTokenType->Parse = NULL;
   SeTokenType->Security = NULL;
   SeTokenType->QueryName = NULL;
   SeTokenType->OkayToClose = NULL;
   SeTokenType->Create = NULL;
   
}

NTSTATUS RtlCopySidAndAttributesArray(ULONG Count,             // ebp + 8
				      PSID_AND_ATTRIBUTES Src,   // ebp + C
				      ULONG MaxLength,         // ebp + 10
				      PSID_AND_ATTRIBUTES Dest, // ebp + 14
				      PVOID e,    // ebp + 18
				      PVOID* f,                 // ebp + 1C
				      PULONG g)                 // ebp + 20
{
   ULONG Length; // ebp - 4
   ULONG i;
   
   Length = MaxLength;
   
   for (i=0; i<Count; i++)
     {
	if (RtlLengthSid(Src[i].Sid) > Length)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	Length = Length - RtlLengthSid(Src[i].Sid);
	Dest[i].Sid = e;
	Dest[i].Attributes = Src[i].Attributes;
	RtlCopySid(RtlLengthSid(Src[i].Sid), e, Src[i].Sid);
	e = e + RtlLengthSid(Src[i].Sid) + sizeof(ULONG);
     }
   *f = e;
   *g = Length;
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtQueryInformationToken(IN HANDLE TokenHandle,     
					 IN TOKEN_INFORMATION_CLASS 
					 TokenInformationClass,
					 OUT PVOID TokenInformation,  
					 IN ULONG TokenInformationLength,
					 OUT PULONG ReturnLength)
{
   NTSTATUS Status;
   PACCESS_TOKEN Token;
   PVOID UnusedInfo;
   PVOID EndMem;
   PTOKEN_GROUPS PtrTokenGroups;
   PTOKEN_DEFAULT_DACL PtrDefaultDacl;
   PTOKEN_STATISTICS PtrTokenStatistics;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
				      0,
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   switch (TokenInformationClass)
     {
      case TokenUser:
	Status = RtlCopySidAndAttributesArray(1,
					      Token->UserAndGroups,
					      TokenInformationLength,
					      TokenInformation,
					      TokenInformation + 8,
					      &UnusedInfo,
					      ReturnLength);
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Token);
	     return(Status);
	  }
	break;
	
	
      case TokenGroups:
	EndMem = TokenInformation + 
	  Token->UserAndGroupCount * sizeof(SID_AND_ATTRIBUTES);
	PtrTokenGroups = (PTOKEN_GROUPS)TokenInformation;
	PtrTokenGroups->GroupCount = Token->UserAndGroupCount - 1;
	Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
					      &Token->UserAndGroups[1],
					      TokenInformationLength,
					      PtrTokenGroups->Groups,
					      EndMem,
					      &UnusedInfo,
					      ReturnLength);
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Token);
	     return(Status);
	  }
	break;
	
      case TokenPrivileges:	
	break;
	
      case TokenOwner:
	((PTOKEN_OWNER)TokenInformation)->Owner = 
	  (PSID)(((PTOKEN_OWNER)TokenInformation) + 1);
	RtlCopySid(TokenInformationLength - sizeof(TOKEN_OWNER),
		   ((PTOKEN_OWNER)TokenInformation)->Owner,
		   Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
	break;
	
      case TokenPrimaryGroup:
	((PTOKEN_PRIMARY_GROUP)TokenInformation)->PrimaryGroup = 
	  (PSID)(((PTOKEN_PRIMARY_GROUP)TokenInformation) + 1);
	RtlCopySid(TokenInformationLength - sizeof(TOKEN_OWNER),
		   ((PTOKEN_PRIMARY_GROUP)TokenInformation)->PrimaryGroup,
		   Token->PrimaryGroup);
	break;
	
      case TokenDefaultDacl:
	PtrDefaultDacl = (PTOKEN_DEFAULT_DACL)TokenInformation;
	PtrDefaultDacl->DefaultDacl = (PACL)(PtrDefaultDacl + 1);
	memmove(PtrDefaultDacl->DefaultDacl,
		Token->DefaultDacl,
		Token->DefaultDacl->AclSize);
	break;
	
      case TokenSource:
	memcpy(TokenInformation, &Token->TokenSource, sizeof(TOKEN_SOURCE));
	break;
	
      case TokenType:
	*((PTOKEN_TYPE)TokenInformation) = Token->TokenType;
	break;
	
      case TokenImpersonationLevel:
	*((PSECURITY_IMPERSONATION_LEVEL)TokenInformation) =
	  Token->ImpersonationLevel;
	break;
	
      case TokenStatistics:
	PtrTokenStatistics = (PTOKEN_STATISTICS)TokenInformation;
	PtrTokenStatistics->TokenId = Token->TokenId;
	PtrTokenStatistics->AuthenticationId = Token->AuthenticationId;
	PtrTokenStatistics->ExpirationTime = Token->ExpirationTime;
	PtrTokenStatistics->TokenType = Token->TokenType;
	PtrTokenStatistics->ImpersonationLevel = Token->ImpersonationLevel;
	PtrTokenStatistics->DynamicCharged = Token->DynamicCharged;
	PtrTokenStatistics->DynamicAvailable = Token->DynamicAvailable;
	PtrTokenStatistics->GroupCount = Token->UserAndGroupCount - 1;
	PtrTokenStatistics->PrivilegeCount = Token->PrivilegeCount;
	PtrTokenStatistics->ModifiedId = Token->ModifiedId;
	break;
     }
   
   ObDereferenceObject(Token);
   return(STATUS_SUCCESS);
}




NTSTATUS
STDCALL
NtSetInformationToken(
	IN	HANDLE			TokenHandle,            
	IN	TOKEN_INFORMATION_CLASS	TokenInformationClass,
	OUT	PVOID			TokenInformation,       
	IN	ULONG			TokenInformationLength   
	)
{
	UNIMPLEMENTED;
}

NTSTATUS STDCALL NtDuplicateToken(IN HANDLE ExistingTokenHandle,
				  IN ACCESS_MASK DesiredAccess, 
				  IN POBJECT_ATTRIBUTES ObjectAttributes,
				  IN SECURITY_IMPERSONATION_LEVEL 
				                      ImpersonationLevel,
				  IN TOKEN_TYPE TokenType,  
				  OUT PHANDLE NewTokenHandle)
{
#if 0
   PACCESS_TOKEN Token;
   PACCESS_TOKEN NewToken;
   NTSTATUS Status;
   ULONG ExistingImpersonationLevel;
   
   Status = ObReferenceObjectByHandle(ExistingTokenHandle,
				      ?,
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   
   ExistingImpersonationLevel = Token->ImpersonationLevel;
   SepDuplicateToken(Token,
		     ObjectAttributes,
		     ImpersonationLevel,
		     TokenType,
		     ExistingImpersonationLevel,
		     KeGetPreviousMode(),
		     &NewToken);
#else
   UNIMPLEMENTED;
#endif
}

VOID SepAdjustGroups(PACCESS_TOKEN Token,
		     ULONG a,
		     BOOLEAN ResetToDefault,
		     PSID_AND_ATTRIBUTES Groups,
		     ULONG b,
		     KPROCESSOR_MODE PreviousMode,
		     ULONG c,
		     PULONG d,
		     PULONG e,
		     PULONG f)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtAdjustGroupsToken(IN	HANDLE		TokenHandle,
				     IN	BOOLEAN		ResetToDefault,	
				     IN	PTOKEN_GROUPS	NewState, 
				     IN	ULONG		BufferLength,	
				     OUT PTOKEN_GROUPS	PreviousState OPTIONAL,
				     OUT PULONG		ReturnLength)
{
#if 0
   NTSTATUS Status;
   PACCESS_TOKEN Token;
   ULONG a;
   ULONG b;
   ULONG c;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
				      ?,
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   
   
   SepAdjustGroups(Token,
		   0,
		   ResetToDefault,
		   NewState->Groups,
		   ?,
		   PreviousState,
		   0,
		   &a,
		   &b,
		   &c);
#else
   UNIMPLEMENTED;
#endif
}

#if 0
NTSTATUS SepAdjustPrivileges(PACCESS_TOKEN Token,           // 0x8
			     ULONG a,                       // 0xC
			     KPROCESSOR_MODE PreviousMode,  // 0x10
			     ULONG PrivilegeCount,          // 0x14
			     PLUID_AND_ATTRIBUTES Privileges, // 0x18
			     PTOKEN_PRIVILEGES* PreviousState, // 0x1C
			     PULONG b, // 0x20
			     PULONG c, // 0x24
			     PULONG d) // 0x28
{
   ULONG i;
   
   *c = 0;
   if (Token->PrivilegeCount > 0)
     {
	for (i=0; i<Token->PrivilegeCount; i++)
	  {
	     if (PreviousMode != 0)
	       {
		  if (!(Token->Privileges[i]->Attributes & 
			SE_PRIVILEGE_ENABLED))
		    {
		       if (a != 0)
			 {
			    if (PreviousState != NULL)
			      {
				 memcpy(&PreviousState[i],
					&Token->Privileges[i],
					sizeof(LUID_AND_ATTRIBUTES));
			      }
			    Token->Privileges[i].Attributes = 
			      Token->Privileges[i].Attributes & 
			      (~SE_PRIVILEGE_ENABLED);
			 }
		    }
	       }
	  }
     }
   if (PreviousMode != 0)
     {
	Token->TokenFlags = Token->TokenFlags & (~1);
     }
   else
     {
	if (PrivilegeCount <= ?)
	  {
	     
	  }
     }
   if (
}
#endif
   
NTSTATUS STDCALL NtAdjustPrivilegesToken(IN HANDLE  TokenHandle,	
					 IN BOOLEAN  DisableAllPrivileges,
					 IN PTOKEN_PRIVILEGES  NewState,
					 IN ULONG  BufferLength,
					 OUT PTOKEN_PRIVILEGES  PreviousState,
					 OUT PULONG ReturnLength)
{
#if 0
   ULONG PrivilegeCount;
   ULONG Length;
   PSID_AND_ATTRIBUTES Privileges;
   ULONG a;
   ULONG b;
   ULONG c;
   
   PrivilegeCount = NewState->PrivilegeCount;
   
   SeCaptureLuidAndAttributesArray(NewState->Privileges,
				   &PrivilegeCount,
				   KeGetPreviousMode(),
				   NULL,
				   0,
				   NonPagedPool,
				   1,
				   &Privileges.
				   &Length);
   SepAdjustPrivileges(Token,
		       0,
		       KeGetPreviousMode(),
		       PrivilegeCount,
		       Privileges,
		       PreviousState,
		       &a,
		       &b,
		       &c);
#else
   UNIMPLEMENTED;
#endif
}

NTSTATUS STDCALL NtCreateToken(OUT PHANDLE TokenHandle,
			       IN ACCESS_MASK DesiredAccess,
			       IN POBJECT_ATTRIBUTES ObjectAttributes,
			       IN TOKEN_TYPE TokenType,
			       IN PLUID AuthenticationId,
			       IN PLARGE_INTEGER ExpirationTime,
			       IN PTOKEN_USER TokenUser,
			       IN PTOKEN_GROUPS TokenGroups,
			       IN PTOKEN_PRIVILEGES TokenPrivileges,
			       IN PTOKEN_OWNER TokenOwner,
			       IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
			       IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
			       IN PTOKEN_SOURCE TokenSource)
{
#if 0
   PACCESS_TOKEN AccessToken;
   NTSTATUS Status;
   
   Status = ObCreateObject(TokenHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   SeTokenType);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   AccessToken->TokenType = TokenType;
   RtlCopyLuid(&AccessToken->AuthenticationId, AuthenticationId);
   AccessToken->ExpirationTime = *ExpirationTime;
   AccessToken->
#endif
     UNIMPLEMENTED;
}


/* EOF */
