/*
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

NTSTATUS STDCALL NtQueryInformationToken(IN HANDLE TokenHandle,     
					 IN TOKEN_INFORMATION_CLASS 
					 TokenInformationClass,
					 OUT PVOID TokenInformation,  
					 IN ULONG TokenInformationLength,
					 OUT PULONG ReturnLength)
{
   NTSTATUS Status;
   PACCESS_TOKEN Token;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
//				      TOKEN_QUERY_INFORMATION,
				      0,
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
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
	UNIMPLEMENTED;
}

