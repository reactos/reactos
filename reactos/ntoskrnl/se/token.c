/* $Id: token.c,v 1.17 2002/06/17 22:52:32 joeg Exp $
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
#include <internal/ps.h>
#include <internal/se.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE SepTokenObjectType = NULL;

static GENERIC_MAPPING SepTokenMapping = {TOKEN_READ,
					  TOKEN_WRITE,
					  TOKEN_EXECUTE,
					  TOKEN_ALL_ACCESS};

#define SYSTEM_LUID                      0x3E7;

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
			      TOKEN_ALL_ACCESS,
			      SepTokenObjectType,
			      KernelMode);
   OldToken->TokenInUse = 0;
   *OldTokenP = OldToken;
   return(STATUS_SUCCESS);
}


NTSTATUS
RtlCopySidAndAttributesArray(ULONG Count,			// ebp + 8
			     PSID_AND_ATTRIBUTES Src,		// ebp + C
			     ULONG SidAreaSize,			// ebp + 10
			     PSID_AND_ATTRIBUTES Dest,		// ebp + 14
			     PVOID SidArea,			// ebp + 18
			     PVOID* RemainingSidArea,		// ebp + 1C
			     PULONG RemainingSidAreaSize)	// ebp + 20
{
   ULONG Length; // ebp - 4
   ULONG i;
   
   Length = SidAreaSize;
   
   for (i=0; i<Count; i++)
     {
	if (RtlLengthSid(Src[i].Sid) > Length)
	  {
	     return(STATUS_BUFFER_TOO_SMALL);
	  }
	Length = Length - RtlLengthSid(Src[i].Sid);
	Dest[i].Sid = SidArea;
	Dest[i].Attributes = Src[i].Attributes;
	RtlCopySid(RtlLengthSid(Src[i].Sid), SidArea, Src[i].Sid);
	SidArea = SidArea + RtlLengthSid(Src[i].Sid);
     }
   *RemainingSidArea = SidArea;
   *RemainingSidAreaSize = Length;
   return(STATUS_SUCCESS);
}


static ULONG
RtlLengthSidAndAttributes(ULONG Count,
			  PSID_AND_ATTRIBUTES Src)
{
  ULONG i;
  ULONG uLength;

  uLength = Count * sizeof(SID_AND_ATTRIBUTES);
  for (i = 0; i < Count; i++)
    uLength += RtlLengthSid(Src[i].Sid);

  return(uLength);
}


NTSTATUS
SepFindPrimaryGroupAndDefaultOwner(PACCESS_TOKEN Token,
				   PSID PrimaryGroup,
				   PSID DefaultOwner)
{
  ULONG i;

  Token->PrimaryGroup = 0;

  if (DefaultOwner)
    {
      Token->DefaultOwnerIndex = Token->UserAndGroupCount;
    }

  /* Validate and set the primary group and user pointers */
  for (i = 0; i < Token->UserAndGroupCount; i++)
    {
      if (DefaultOwner &&
	  RtlEqualSid(Token->UserAndGroups[i].Sid, DefaultOwner))
	{
	  Token->DefaultOwnerIndex = i;
	}

      if (RtlEqualSid(Token->UserAndGroups[i].Sid, PrimaryGroup))
	{
	  Token->PrimaryGroup = Token->UserAndGroups[i].Sid;
	}
    }

  if (Token->DefaultOwnerIndex == Token->UserAndGroupCount)
    {
      return(STATUS_INVALID_OWNER);
    }

  if (Token->PrimaryGroup == 0)
    {
      return(STATUS_INVALID_PRIMARY_GROUP);
    }

  return(STATUS_SUCCESS);
}


NTSTATUS
SepDuplicateToken(PACCESS_TOKEN Token,
		  POBJECT_ATTRIBUTES ObjectAttributes,
		  TOKEN_TYPE TokenType,
		  SECURITY_IMPERSONATION_LEVEL Level,
		  SECURITY_IMPERSONATION_LEVEL ExistingLevel,
		  KPROCESSOR_MODE PreviousMode,
		  PACCESS_TOKEN* NewAccessToken)
{
  NTSTATUS Status;
  ULONG uLength;
  ULONG i;
  
  PVOID EndMem;

  PACCESS_TOKEN AccessToken;

  Status = ObCreateObject(0,
			  TOKEN_ALL_ACCESS,
			  ObjectAttributes,
			  SepTokenObjectType,
			  (PVOID*)&AccessToken);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObCreateObject() failed (Status %lx)\n");
      return(Status);
    }

  Status = ZwAllocateLocallyUniqueId(&AccessToken->TokenId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  Status = ZwAllocateLocallyUniqueId(&AccessToken->ModifiedId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  AccessToken->TokenInUse = 0;
  AccessToken->TokenType  = TokenType;
  AccessToken->ImpersonationLevel = Level;
  AccessToken->AuthenticationId.QuadPart = SYSTEM_LUID;

  AccessToken->TokenSource.SourceIdentifier.QuadPart = Token->TokenSource.SourceIdentifier.QuadPart;
  memcpy(AccessToken->TokenSource.SourceName, Token->TokenSource.SourceName, sizeof(Token->TokenSource.SourceName));
  AccessToken->ExpirationTime.QuadPart = Token->ExpirationTime.QuadPart;
  AccessToken->UserAndGroupCount = Token->UserAndGroupCount;
  AccessToken->DefaultOwnerIndex = Token->DefaultOwnerIndex;

  uLength = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
  for (i = 0; i < Token->UserAndGroupCount; i++)
    uLength += RtlLengthSid(Token->UserAndGroups[i].Sid);

  AccessToken->UserAndGroups = 
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(NonPagedPool,
					       uLength,
					       TAG('T', 'O', 'K', 'u'));

  EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

  Status = RtlCopySidAndAttributesArray(AccessToken->UserAndGroupCount,
					Token->UserAndGroups,
					uLength,
					AccessToken->UserAndGroups,
					EndMem,
					&EndMem,
					&uLength);
  if (NT_SUCCESS(Status))
    {
      Status = SepFindPrimaryGroupAndDefaultOwner(
	AccessToken,
	Token->PrimaryGroup,
	0);
    }

  if (NT_SUCCESS(Status))
    {
      AccessToken->PrivilegeCount = Token->PrivilegeCount;

      uLength = AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
      AccessToken->Privileges =
	(PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(NonPagedPool,
						    uLength,
						    TAG('T', 'O', 'K', 'p'));

      for (i = 0; i < AccessToken->PrivilegeCount; i++)
	{
	  RtlCopyLuid(&AccessToken->Privileges[i].Luid,
	    &Token->Privileges[i].Luid);
	  AccessToken->Privileges[i].Attributes = 
	    Token->Privileges[i].Attributes;
	}

      if ( Token->DefaultDacl )
	{
	  AccessToken->DefaultDacl =
	    (PACL) ExAllocatePoolWithTag(NonPagedPool,
					 Token->DefaultDacl->AclSize,
					 TAG('T', 'O', 'K', 'd'));
	  memcpy(AccessToken->DefaultDacl,
		 Token->DefaultDacl,
		 Token->DefaultDacl->AclSize);
	}
      else
	{
	  AccessToken->DefaultDacl = 0;
	}
    }

  if ( NT_SUCCESS(Status) )
    {
      *NewAccessToken = AccessToken;
      return(STATUS_SUCCESS);
    }

  ObDereferenceObject(AccessToken);
  return(Status);
}


NTSTATUS
SepInitializeNewProcess(struct _EPROCESS* NewProcess,
			struct _EPROCESS* ParentProcess)
{
  NTSTATUS Status;
  PACCESS_TOKEN pNewToken;
  PACCESS_TOKEN pParentToken;
  
  OBJECT_ATTRIBUTES ObjectAttributes;

  pParentToken = (PACCESS_TOKEN) ParentProcess->Token;

  InitializeObjectAttributes(&ObjectAttributes,
			    NULL,
			    0,
			    NULL,
			    NULL);

  Status = SepDuplicateToken(pParentToken,
			     &ObjectAttributes,
			     TokenPrimary,
			     pParentToken->ImpersonationLevel,
			     pParentToken->ImpersonationLevel,
			     KernelMode,
			     &pNewToken);
  if ( ! NT_SUCCESS(Status) )
    return Status;

  NewProcess->Token = pNewToken;
  return(STATUS_SUCCESS);
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
   Status = SepDuplicateToken(Token,
				&ObjectAttributes,
				0,
				SecurityIdentification,
				Level,
				PreviousMode,
				NewToken);
   return(Status);
}


NTSTATUS STDCALL
SeCreateClientSecurity(IN struct _ETHREAD *Thread,
		       IN PSECURITY_QUALITY_OF_SERVICE Qos,
		       IN BOOLEAN RemoteClient,
		       OUT PSECURITY_CLIENT_CONTEXT ClientContext)
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
	ClientContext->DirectAccessEffectiveOnly = Qos->EffectiveOnly;
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
	    (RemoteClient != FALSE && ImpersonationLevel != 3))
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
	     ClientContext->DirectAccessEffectiveOnly = TRUE;
	  }
	else
	  {
	     ClientContext->DirectAccessEffectiveOnly = FALSE;
	  }
     }
   
   if (Qos->ContextTrackingMode == 0)
     {
	ClientContext->DirectlyAccessClientToken = FALSE;
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
	ClientContext->DirectlyAccessClientToken = TRUE;
	if (RemoteClient != FALSE)
	  {
//	     SeGetTokenControlInformation(Token, &ClientContext->Unknown11);
	  }
	NewToken = Token;
    }
  ClientContext->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  ClientContext->SecurityQos.ImpersonationLevel = Qos->ImpersonationLevel;
  ClientContext->SecurityQos.ContextTrackingMode = Qos->ContextTrackingMode;
  ClientContext->SecurityQos.EffectiveOnly = Qos->EffectiveOnly;
  ClientContext->ServerIsRemote = RemoteClient;
  ClientContext->Token = NewToken;

  return(STATUS_SUCCESS);
}


VOID STDCALL
SeImpersonateClient(IN PSECURITY_CLIENT_CONTEXT ClientContext,
		    IN PETHREAD ServerThread OPTIONAL)
{
  UCHAR b;
  
  if (ClientContext->DirectlyAccessClientToken == FALSE)
    {
      b = ClientContext->SecurityQos.EffectiveOnly;
    }
  else
    {
      b = ClientContext->DirectAccessEffectiveOnly;
    }
  if (ServerThread == NULL)
    {
      ServerThread = PsGetCurrentThread();
    }
  PsImpersonateClient(ServerThread,
		      ClientContext->Token,
		      1,
		      (ULONG)b,
		      ClientContext->SecurityQos.ImpersonationLevel);
}


VOID STDCALL
SepDeleteToken(PVOID ObjectBody)
{
  PACCESS_TOKEN AccessToken = (PACCESS_TOKEN)ObjectBody;

  if (AccessToken->UserAndGroups)
    ExFreePool(AccessToken->UserAndGroups);

  if (AccessToken->Privileges)
    ExFreePool(AccessToken->Privileges);

  if (AccessToken->DefaultDacl)
    ExFreePool(AccessToken->DefaultDacl);
}


VOID
SepInitializeTokenImplementation(VOID)
{
  SepTokenObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

  SepTokenObjectType->Tag = TAG('T', 'O', 'K', 'T');
  SepTokenObjectType->MaxObjects = ULONG_MAX;
  SepTokenObjectType->MaxHandles = ULONG_MAX;
  SepTokenObjectType->TotalObjects = 0;
  SepTokenObjectType->TotalHandles = 0;
  SepTokenObjectType->PagedPoolCharge = 0;
  SepTokenObjectType->NonpagedPoolCharge = sizeof(ACCESS_TOKEN);
  SepTokenObjectType->Mapping = &SepTokenMapping;
  SepTokenObjectType->Dump = NULL;
  SepTokenObjectType->Open = NULL;
  SepTokenObjectType->Close = NULL;
  SepTokenObjectType->Delete = SepDeleteToken;
  SepTokenObjectType->Parse = NULL;
  SepTokenObjectType->Security = NULL;
  SepTokenObjectType->QueryName = NULL;
  SepTokenObjectType->OkayToClose = NULL;
  SepTokenObjectType->Create = NULL;
  SepTokenObjectType->DuplicationNotify = NULL;

  RtlCreateUnicodeString(&SepTokenObjectType->TypeName,
			 L"Token");
}


NTSTATUS STDCALL
NtQueryInformationToken(IN HANDLE TokenHandle,
			IN TOKEN_INFORMATION_CLASS TokenInformationClass,
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
  ULONG uLength;

  Status = ObReferenceObjectByHandle(TokenHandle,
				     (TokenInformationClass == TokenSource) ? TOKEN_QUERY_SOURCE : TOKEN_QUERY,
				     SepTokenObjectType,
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
        DPRINT("NtQueryInformationToken(TokenUser)\n");
	uLength = RtlLengthSidAndAttributes(1, Token->UserAndGroups);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = RtlCopySidAndAttributesArray(1,
						  Token->UserAndGroups,
						  TokenInformationLength,
						  TokenInformation,
						  TokenInformation + 8,
						  &UnusedInfo,
						  &uLength);
	    if (NT_SUCCESS(Status))
	      {
		uLength = TokenInformationLength - uLength;
		Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	      }
	  }
	break;
	
      case TokenGroups:
        DPRINT("NtQueryInformationToken(TokenGroups)\n");
	uLength = RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]) + sizeof(DWORD);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    EndMem = TokenInformation + Token->UserAndGroupCount * sizeof(SID_AND_ATTRIBUTES);
	    PtrTokenGroups = (PTOKEN_GROUPS)TokenInformation;
	    PtrTokenGroups->GroupCount = Token->UserAndGroupCount - 1;
	    Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
						  &Token->UserAndGroups[1],
						  TokenInformationLength,
						  PtrTokenGroups->Groups,
						  EndMem,
						  &UnusedInfo,
						  &uLength);
	    if (NT_SUCCESS(Status))
	      {
		uLength = TokenInformationLength - uLength;
		Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	      }
	  }
	break;

      case TokenPrivileges:
        DPRINT("NtQueryInformationToken(TokenPrivileges)\n");
	uLength = sizeof(DWORD) + Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    ULONG i;
	    TOKEN_PRIVILEGES* pPriv = (TOKEN_PRIVILEGES*)TokenInformation;

	    pPriv->PrivilegeCount = Token->PrivilegeCount;
	    for (i = 0; i < Token->PrivilegeCount; i++)
	      {
		RtlCopyLuid(&pPriv->Privileges[i].Luid, &Token->Privileges[i].Luid);
		pPriv->Privileges[i].Attributes = Token->Privileges[i].Attributes;
	      }
	    Status = STATUS_SUCCESS;
	  }
	break;

      case TokenOwner:
        DPRINT("NtQueryInformationToken(TokenOwner)\n");
	uLength = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid) + sizeof(TOKEN_OWNER);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    ((PTOKEN_OWNER)TokenInformation)->Owner = 
	      (PSID)(((PTOKEN_OWNER)TokenInformation) + 1);
	    RtlCopySid(TokenInformationLength - sizeof(TOKEN_OWNER),
		       ((PTOKEN_OWNER)TokenInformation)->Owner,
		       Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
	    Status = STATUS_SUCCESS;
	  }
	break;

      case TokenPrimaryGroup:
        DPRINT("NtQueryInformationToken(TokenPrimaryGroup),"
	       "Token->PrimaryGroup = 0x%08x\n", Token->PrimaryGroup);
	uLength = RtlLengthSid(Token->PrimaryGroup) + sizeof(TOKEN_PRIMARY_GROUP);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    ((PTOKEN_PRIMARY_GROUP)TokenInformation)->PrimaryGroup = 
	      (PSID)(((PTOKEN_PRIMARY_GROUP)TokenInformation) + 1);
	    RtlCopySid(TokenInformationLength - sizeof(TOKEN_PRIMARY_GROUP),
		       ((PTOKEN_PRIMARY_GROUP)TokenInformation)->PrimaryGroup,
		       Token->PrimaryGroup);
	    Status = STATUS_SUCCESS;
	  }
	break;

      case TokenDefaultDacl:
        DPRINT("NtQueryInformationToken(TokenDefaultDacl)\n");
	PtrDefaultDacl = (PTOKEN_DEFAULT_DACL) TokenInformation;
	uLength = (Token->DefaultDacl ? Token->DefaultDacl->AclSize : 0) + sizeof(TOKEN_DEFAULT_DACL);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else if (!Token->DefaultDacl)
	  {
	    PtrDefaultDacl->DefaultDacl = 0;
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	  }
	else
	  {
	    PtrDefaultDacl->DefaultDacl = (PACL) (PtrDefaultDacl + 1);
	    memmove(PtrDefaultDacl->DefaultDacl,
		    Token->DefaultDacl,
		    Token->DefaultDacl->AclSize);
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	  }
	break;

      case TokenSource:
        DPRINT("NtQueryInformationToken(TokenSource)\n");
	if (TokenInformationLength < sizeof(TOKEN_SOURCE))
	  {
	    uLength = sizeof(TOKEN_SOURCE);
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = MmCopyToCaller(TokenInformation, &Token->TokenSource, sizeof(TOKEN_SOURCE));
	  }
	break;

      case TokenType:
        DPRINT("NtQueryInformationToken(TokenType)\n");
	if (TokenInformationLength < sizeof(TOKEN_TYPE))
	  {
	    uLength = sizeof(TOKEN_TYPE);
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = MmCopyToCaller(TokenInformation, &Token->TokenType, sizeof(TOKEN_TYPE));
	  }
	break;

      case TokenImpersonationLevel:
        DPRINT("NtQueryInformationToken(TokenImpersonationLevel)\n");
	if (TokenInformationLength < sizeof(SECURITY_IMPERSONATION_LEVEL))
	  {
	    uLength = sizeof(SECURITY_IMPERSONATION_LEVEL);
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = MmCopyToCaller(TokenInformation, &Token->ImpersonationLevel, sizeof(SECURITY_IMPERSONATION_LEVEL));
	  }
	break;

      case TokenStatistics:
        DPRINT("NtQueryInformationToken(TokenStatistics)\n");
	if (TokenInformationLength < sizeof(TOKEN_STATISTICS))
	  {
	    uLength = sizeof(TOKEN_STATISTICS);
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
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

	    Status = STATUS_SUCCESS;
	  }
	break;
    }

  ObDereferenceObject(Token);

  return(Status);
}


NTSTATUS STDCALL
NtSetInformationToken(IN HANDLE TokenHandle,
		      IN TOKEN_INFORMATION_CLASS TokenInformationClass,
		      OUT PVOID TokenInformation,
		      IN ULONG TokenInformationLength)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtDuplicateToken(IN HANDLE ExistingTokenHandle,
		 IN ACCESS_MASK DesiredAccess,
		 IN POBJECT_ATTRIBUTES ObjectAttributes,
		 IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
		 IN TOKEN_TYPE TokenType,
		 OUT PHANDLE NewTokenHandle)
{
#if 0
   PACCESS_TOKEN Token;
   PACCESS_TOKEN NewToken;
   NTSTATUS Status;
   ULONG ExistingImpersonationLevel;
   
   Status = ObReferenceObjectByHandle(ExistingTokenHandle,
				      TOKEN_DUPLICATE,
				      SepTokenObjectType,
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


NTSTATUS STDCALL
NtAdjustGroupsToken(IN HANDLE TokenHandle,
		    IN BOOLEAN ResetToDefault,
		    IN PTOKEN_GROUPS NewState,
		    IN ULONG BufferLength,
		    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
		    OUT PULONG ReturnLength)
{
#if 0
   NTSTATUS Status;
   PACCESS_TOKEN Token;
   ULONG a;
   ULONG b;
   ULONG c;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
				      ?,
				      SepTokenObjectType,
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


NTSTATUS STDCALL
NtAdjustPrivilegesToken(IN HANDLE TokenHandle,
			IN BOOLEAN DisableAllPrivileges,
			IN PTOKEN_PRIVILEGES NewState,
			IN ULONG BufferLength,
			OUT PTOKEN_PRIVILEGES PreviousState,
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


NTSTATUS
SepCreateSystemProcessToken(struct _EPROCESS* Process)
{
  NTSTATUS Status;
  ULONG uSize;
  ULONG i;

  ULONG uLocalSystemLength = RtlLengthSid(SeLocalSystemSid);
  ULONG uWorldLength       = RtlLengthSid(SeWorldSid);
  ULONG uAuthUserLength    = RtlLengthSid(SeAuthenticatedUserSid);
  ULONG uAdminsLength      = RtlLengthSid(SeAliasAdminsSid);

  PACCESS_TOKEN AccessToken;

  PVOID SidArea;

 /*
  * Initialize the token
  */
  Status = ObCreateObject(NULL,
			 TOKEN_ALL_ACCESS,
			 NULL,
			 SepTokenObjectType,
			 (PVOID*)&AccessToken);

  Status = ZwAllocateLocallyUniqueId(&AccessToken->TokenId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  Status = ZwAllocateLocallyUniqueId(&AccessToken->ModifiedId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  AccessToken->AuthenticationId.QuadPart = SYSTEM_LUID;

  AccessToken->TokenType = TokenPrimary;
  AccessToken->ImpersonationLevel = SecurityDelegation;
  AccessToken->TokenSource.SourceIdentifier.QuadPart = 0;
  memcpy(AccessToken->TokenSource.SourceName, "SeMgr\0\0\0", 8);
  AccessToken->ExpirationTime.QuadPart = -1;
  AccessToken->UserAndGroupCount = 4;

  uSize = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
  uSize += uLocalSystemLength;
  uSize += uWorldLength;
  uSize += uAuthUserLength;
  uSize += uAdminsLength;

  AccessToken->UserAndGroups = 
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(NonPagedPool,
					       uSize,
					       TAG('T', 'O', 'K', 'u'));
  SidArea = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

  i = 0;
  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = 0;
  RtlCopySid(uLocalSystemLength, SidArea, SeLocalSystemSid);
  SidArea += uLocalSystemLength;

  AccessToken->DefaultOwnerIndex = i;
  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->PrimaryGroup = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT;
  Status = RtlCopySid(uAdminsLength, SidArea, SeAliasAdminsSid);
  SidArea += uAdminsLength;

  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
  RtlCopySid(uWorldLength, SidArea, SeWorldSid);
  SidArea += uWorldLength;

  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
  RtlCopySid(uAuthUserLength, SidArea, SeAuthenticatedUserSid);
  SidArea += uAuthUserLength;

  AccessToken->PrivilegeCount = 20;

  uSize = AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
  AccessToken->Privileges =
	(PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(NonPagedPool,
						    uSize,
						    TAG('T', 'O', 'K', 'p'));

  i = 0;
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeTcbPrivilege;

  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeCreateTokenPrivilege;

  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeTakeOwnershipPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeCreatePagefilePrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeLockMemoryPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeAssignPrimaryTokenPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeIncreaseQuotaPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeIncreaseBasePriorityPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeCreatePermanentPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeDebugPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeAuditPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeSecurityPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeSystemEnvironmentPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeChangeNotifyPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeBackupPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeRestorePrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeShutdownPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeLoadDriverPrivilege;
  
  AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++].Luid = SeProfileSingleProcessPrivilege;
  
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeSystemtimePrivilege;
#if 0
  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeUndockPrivilege;

  AccessToken->Privileges[i].Attributes = 0;
  AccessToken->Privileges[i++].Luid = SeManageVolumePrivilege;
#endif

  assert( i == 20 );

  uSize = sizeof(ACL);
  uSize += sizeof(ACE_HEADER) + uLocalSystemLength;
  uSize += sizeof(ACE_HEADER) + uAdminsLength;
  uSize = (uSize & (~3)) + 8;
  AccessToken->DefaultDacl =
    (PACL) ExAllocatePoolWithTag(NonPagedPool,
				 uSize,
				 TAG('T', 'O', 'K', 'd'));
  Status = RtlCreateAcl(AccessToken->DefaultDacl, uSize, ACL_REVISION);
  if ( NT_SUCCESS(Status) )
    {
      Status = RtlAddAccessAllowedAce(AccessToken->DefaultDacl, ACL_REVISION, GENERIC_ALL, SeLocalSystemSid);
    }

  if ( NT_SUCCESS(Status) )
    {
      Status = RtlAddAccessAllowedAce(AccessToken->DefaultDacl, ACL_REVISION, GENERIC_READ|GENERIC_EXECUTE|READ_CONTROL, SeAliasAdminsSid);
    }

  if ( ! NT_SUCCESS(Status) )
    {
      ObDereferenceObject(AccessToken);
      return Status;
    }

  Process->Token = AccessToken;
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
NtCreateToken(OUT PHANDLE UnsafeTokenHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES UnsafeObjectAttributes,
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
  HANDLE TokenHandle;
  PACCESS_TOKEN AccessToken;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES SafeObjectAttributes;
  POBJECT_ATTRIBUTES ObjectAttributes;
  LUID TokenId;
  LUID ModifiedId;
  PVOID EndMem;
  ULONG uLength;
  ULONG i;

  Status = MmCopyFromCaller(&SafeObjectAttributes,
			    UnsafeObjectAttributes,
			    sizeof(OBJECT_ATTRIBUTES));
  if (!NT_SUCCESS(Status))
    return(Status);

  ObjectAttributes = &SafeObjectAttributes;

  Status = ZwAllocateLocallyUniqueId(&TokenId);
  if (!NT_SUCCESS(Status))
    return(Status);

  Status = ZwAllocateLocallyUniqueId(&ModifiedId);
  if (!NT_SUCCESS(Status))
    return(Status);

  Status = ObCreateObject(&TokenHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  SepTokenObjectType,
			  (PVOID*)&AccessToken);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObCreateObject() failed (Status %lx)\n");
      return(Status);
    }

  RtlCopyLuid(&AccessToken->TokenSource.SourceIdentifier,
	      &TokenSource->SourceIdentifier);
  memcpy(AccessToken->TokenSource.SourceName,
	 TokenSource->SourceName,
	 sizeof(TokenSource->SourceName));

  RtlCopyLuid(&AccessToken->TokenId, &TokenId);
  RtlCopyLuid(&AccessToken->AuthenticationId, AuthenticationId);
  AccessToken->ExpirationTime = *ExpirationTime;
  RtlCopyLuid(&AccessToken->ModifiedId, &ModifiedId);

  AccessToken->UserAndGroupCount = TokenGroups->GroupCount + 1;
  AccessToken->PrivilegeCount    = TokenPrivileges->PrivilegeCount;
  AccessToken->UserAndGroups     = 0;
  AccessToken->Privileges        = 0;

  AccessToken->TokenType = TokenType;
  AccessToken->ImpersonationLevel = ObjectAttributes->SecurityQualityOfService->ImpersonationLevel;

  /*
   * Normally we would just point these members into the variable information
   * area; however, our ObCreateObject() call can't allocate a variable information
   * area, so we allocate them seperately and provide a destroy function.
   */

  uLength = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
  uLength += RtlLengthSid(TokenUser->User.Sid);
  for (i = 0; i < TokenGroups->GroupCount; i++)
    uLength += RtlLengthSid(TokenGroups->Groups[i].Sid);

  AccessToken->UserAndGroups = 
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(NonPagedPool,
					       uLength,
					       TAG('T', 'O', 'K', 'u'));

  EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

  Status = RtlCopySidAndAttributesArray(1,
					&TokenUser->User,
					uLength,
					AccessToken->UserAndGroups,
					EndMem,
					&EndMem,
					&uLength);
  if (NT_SUCCESS(Status))
    {
      Status = RtlCopySidAndAttributesArray(TokenGroups->GroupCount,
					    TokenGroups->Groups,
					    uLength,
					    &AccessToken->UserAndGroups[1],
					    EndMem,
					    &EndMem,
					    &uLength);
    }

  if (NT_SUCCESS(Status))
    {
      Status = SepFindPrimaryGroupAndDefaultOwner(
	AccessToken, 
	TokenPrimaryGroup->PrimaryGroup,
	TokenOwner->Owner);
    }

  if (NT_SUCCESS(Status))
    {
      uLength = TokenPrivileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
      AccessToken->Privileges =
	(PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(NonPagedPool,
						    uLength,
						    TAG('T', 'O', 'K', 'p'));

      for (i = 0; i < TokenPrivileges->PrivilegeCount; i++)
	{
	  Status = MmCopyFromCaller(&AccessToken->Privileges[i],
				    &TokenPrivileges->Privileges[i],
				    sizeof(LUID_AND_ATTRIBUTES));
	  if (!NT_SUCCESS(Status))
	    break;
	}
    }

  if (NT_SUCCESS(Status))
    {
      AccessToken->DefaultDacl =
	(PACL) ExAllocatePoolWithTag(NonPagedPool,
				     TokenDefaultDacl->DefaultDacl->AclSize,
				     TAG('T', 'O', 'K', 'd'));
      memcpy(AccessToken->DefaultDacl,
	     TokenDefaultDacl->DefaultDacl,
	     TokenDefaultDacl->DefaultDacl->AclSize);
    }

  ObDereferenceObject(AccessToken);

  if (NT_SUCCESS(Status))
    {
      Status = MmCopyToCaller(UnsafeTokenHandle,
			      &TokenHandle,
			      sizeof(HANDLE));
    }

  if (!NT_SUCCESS(Status))
    {
      ZwClose(TokenHandle);
      return(Status);
    }

  return(STATUS_SUCCESS);
}


SECURITY_IMPERSONATION_LEVEL STDCALL
SeTokenImpersonationLevel(IN PACCESS_TOKEN Token)
{
  return(Token->ImpersonationLevel);
}


TOKEN_TYPE STDCALL
SeTokenType(IN PACCESS_TOKEN Token)
{
  return(Token->TokenType);
}

/* EOF */
