/* $Id: token.c,v 1.3 2000/01/05 21:57:00 dwelch Exp $
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

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE SeTokenType = NULL;

/* FUNCTIONS *****************************************************************/

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

NTSTATUS
STDCALL
NtDuplicateToken (
	IN	HANDLE				ExistingToken, 
	IN	ACCESS_MASK			DesiredAccess, 
	IN	POBJECT_ATTRIBUTES		ObjectAttributes,
	IN	SECURITY_IMPERSONATION_LEVEL	ImpersonationLevel,
	IN	TOKEN_TYPE			TokenType,  
	OUT	PHANDLE				NewToken
	)
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
