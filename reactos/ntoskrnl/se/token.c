/* $Id$
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE SepTokenObjectType = NULL;

static GENERIC_MAPPING SepTokenMapping = {TOKEN_READ,
					  TOKEN_WRITE,
					  TOKEN_EXECUTE,
					  TOKEN_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/

VOID SepFreeProxyData(PVOID ProxyData)
{
   UNIMPLEMENTED;
}

NTSTATUS SepCopyProxyData(PVOID* Dest, PVOID Src)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS SeExchangePrimaryToken(PEPROCESS Process,
				PACCESS_TOKEN NewTokenP,
				PACCESS_TOKEN* OldTokenP)
{
   PTOKEN OldToken;
   PTOKEN NewToken = (PTOKEN)NewTokenP;
   
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
   *OldTokenP = (PACCESS_TOKEN)OldToken;
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
SepFindPrimaryGroupAndDefaultOwner(PTOKEN Token,
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
SepDuplicateToken(PTOKEN Token,
		  POBJECT_ATTRIBUTES ObjectAttributes,
		  BOOLEAN EffectiveOnly,
		  TOKEN_TYPE TokenType,
		  SECURITY_IMPERSONATION_LEVEL Level,
		  KPROCESSOR_MODE PreviousMode,
		  PTOKEN* NewAccessToken)
{
  NTSTATUS Status;
  ULONG uLength;
  ULONG i;
  
  PVOID EndMem;

  PTOKEN AccessToken;

  Status = ObCreateObject(PreviousMode,
			  SepTokenObjectType,
			  ObjectAttributes,
			  PreviousMode,
			  NULL,
			  sizeof(TOKEN),
			  0,
			  0,
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
  RtlCopyLuid(&AccessToken->AuthenticationId, &Token->AuthenticationId);

  AccessToken->TokenSource.SourceIdentifier.LowPart = Token->TokenSource.SourceIdentifier.LowPart;
  AccessToken->TokenSource.SourceIdentifier.HighPart = Token->TokenSource.SourceIdentifier.HighPart;
  memcpy(AccessToken->TokenSource.SourceName,
	 Token->TokenSource.SourceName,
	 sizeof(Token->TokenSource.SourceName));
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
  PTOKEN pNewToken;
  PTOKEN pParentToken;
  
  OBJECT_ATTRIBUTES ObjectAttributes;

  pParentToken = (PACCESS_TOKEN) ParentProcess->Token;

  InitializeObjectAttributes(&ObjectAttributes,
			    NULL,
			    0,
			    NULL,
			    NULL);

  Status = SepDuplicateToken(pParentToken,
			     &ObjectAttributes,
			     FALSE,
			     TokenPrimary,
			     pParentToken->ImpersonationLevel,
			     KernelMode,
			     &pNewToken);
  if ( ! NT_SUCCESS(Status) )
    return Status;

  NewProcess->Token = pNewToken;
  return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeAppendPrivileges(
	PACCESS_STATE AccessState,
	PPRIVILEGE_SET Privileges
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
SeCopyClientToken(PACCESS_TOKEN Token,
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
				FALSE,
				TokenImpersonation,
				Level,
				PreviousMode,
			    (PTOKEN*)&NewToken);
   
   return(Status);
}


/*
 * @implemented
 */
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
   if (TokenType != TokenImpersonation)
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
	if (ImpersonationLevel == SecurityAnonymous ||
	    ImpersonationLevel == SecurityIdentification ||
	    (RemoteClient != FALSE && ImpersonationLevel != SecurityDelegation))
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
  ClientContext->ClientToken = NewToken;

  return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeCreateClientSecurityFromSubjectContext(
	IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
	IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
	IN BOOLEAN ServerIsRemote,
	OUT PSECURITY_CLIENT_CONTEXT ClientContext
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeFilterToken(
	IN PACCESS_TOKEN ExistingToken,
	IN ULONG Flags,
	IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
	IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
	IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
	OUT PACCESS_TOKEN * FilteredToken
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SeFreePrivileges(
	IN PPRIVILEGE_SET Privileges
	)
{
	UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeImpersonateClientEx(
	IN PSECURITY_CLIENT_CONTEXT ClientContext,
	IN PETHREAD ServerThread OPTIONAL
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
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
		      ClientContext->ClientToken,
		      1,
		      (ULONG)b,
		      ClientContext->SecurityQos.ImpersonationLevel);
}


VOID STDCALL
SepDeleteToken(PVOID ObjectBody)
{
  PTOKEN AccessToken = (PTOKEN)ObjectBody;

  if (AccessToken->UserAndGroups)
    ExFreePool(AccessToken->UserAndGroups);

  if (AccessToken->Privileges)
    ExFreePool(AccessToken->Privileges);

  if (AccessToken->DefaultDacl)
    ExFreePool(AccessToken->DefaultDacl);
}


VOID INIT_FUNCTION
SepInitializeTokenImplementation(VOID)
{
  SepTokenObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

  SepTokenObjectType->Tag = TAG('T', 'O', 'K', 'T');
  SepTokenObjectType->PeakObjects = 0;
  SepTokenObjectType->PeakHandles = 0;
  SepTokenObjectType->TotalObjects = 0;
  SepTokenObjectType->TotalHandles = 0;
  SepTokenObjectType->PagedPoolCharge = 0;
  SepTokenObjectType->NonpagedPoolCharge = sizeof(TOKEN);
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
  ObpCreateTypeObject (SepTokenObjectType);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryInformationToken(IN HANDLE TokenHandle,
			IN TOKEN_INFORMATION_CLASS TokenInformationClass,
			OUT PVOID TokenInformation,
			IN ULONG TokenInformationLength,
			OUT PULONG ReturnLength)
{
  NTSTATUS Status, LengthStatus;
  PVOID UnusedInfo;
  PVOID EndMem;
  PTOKEN Token;
  ULONG Length;
  PTOKEN_GROUPS PtrTokenGroups;
  PTOKEN_DEFAULT_DACL PtrDefaultDacl;
  PTOKEN_STATISTICS PtrTokenStatistics;

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
	Length = RtlLengthSidAndAttributes(1, Token->UserAndGroups);
	if (TokenInformationLength < Length)
	  {
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = RtlCopySidAndAttributesArray(1,
						  Token->UserAndGroups,
						  TokenInformationLength,
						  TokenInformation,
						  (char*)TokenInformation + 8,
						  &UnusedInfo,
						  &Length);
	    if (NT_SUCCESS(Status))
	      {
		Length = TokenInformationLength - Length;
		Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	      }
	  }
	break;
	
      case TokenGroups:
	DPRINT("NtQueryInformationToken(TokenGroups)\n");
	Length = RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]) + sizeof(ULONG);
	if (TokenInformationLength < Length)
	  {
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    EndMem = (char*)TokenInformation + Token->UserAndGroupCount * sizeof(SID_AND_ATTRIBUTES);
	    PtrTokenGroups = (PTOKEN_GROUPS)TokenInformation;
	    PtrTokenGroups->GroupCount = Token->UserAndGroupCount - 1;
	    Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
						  &Token->UserAndGroups[1],
						  TokenInformationLength,
						  PtrTokenGroups->Groups,
						  EndMem,
						  &UnusedInfo,
						  &Length);
	    if (NT_SUCCESS(Status))
	      {
		Length = TokenInformationLength - Length;
		Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	      }
	  }
	break;

      case TokenPrivileges:
	DPRINT("NtQueryInformationToken(TokenPrivileges)\n");
	Length = sizeof(ULONG) + Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
	if (TokenInformationLength < Length)
	  {
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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
	Length = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid) + sizeof(TOKEN_OWNER);
	if (TokenInformationLength < Length)
	  {
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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
	Length = RtlLengthSid(Token->PrimaryGroup) + sizeof(TOKEN_PRIMARY_GROUP);
	if (TokenInformationLength < Length)
	  {
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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
	Length = (Token->DefaultDacl ? Token->DefaultDacl->AclSize : 0) + sizeof(TOKEN_DEFAULT_DACL);
	if (TokenInformationLength < Length)
	  {
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else if (!Token->DefaultDacl)
	  {
	    PtrDefaultDacl->DefaultDacl = 0;
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	  }
	else
	  {
	    PtrDefaultDacl->DefaultDacl = (PACL) (PtrDefaultDacl + 1);
	    memmove(PtrDefaultDacl->DefaultDacl,
		    Token->DefaultDacl,
		    Token->DefaultDacl->AclSize);
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	  }
	break;

      case TokenSource:
	DPRINT("NtQueryInformationToken(TokenSource)\n");
	if (TokenInformationLength < sizeof(TOKEN_SOURCE))
	  {
	    Length = sizeof(TOKEN_SOURCE);
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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
	    Length = sizeof(TOKEN_TYPE);
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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
	    Length = sizeof(SECURITY_IMPERSONATION_LEVEL);
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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
	    Length = sizeof(TOKEN_STATISTICS);
	    Status = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
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

      case TokenOrigin:
	DPRINT("NtQueryInformationToken(TokenOrigin)\n");
	if (TokenInformationLength < sizeof(TOKEN_ORIGIN))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = MmCopyToCaller(&((PTOKEN_ORIGIN)TokenInformation)->OriginatingLogonSession,
	                            &Token->AuthenticationId, sizeof(LUID));
	  }
	Length = sizeof(TOKEN_ORIGIN);
	LengthStatus = MmCopyToCaller(ReturnLength, &Length, sizeof(ULONG));
	if (NT_SUCCESS(Status))
	  {
	    Status = LengthStatus;
	  }
	break;

      case TokenGroupsAndPrivileges:
	DPRINT1("NtQueryInformationToken(TokenGroupsAndPrivileges) not implemented\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case TokenRestrictedSids:
	DPRINT1("NtQueryInformationToken(TokenRestrictedSids) not implemented\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case TokenSandBoxInert:
	DPRINT1("NtQueryInformationToken(TokenSandboxInert) not implemented\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case TokenSessionId:
	DPRINT1("NtQueryInformationToken(TokenSessionId) not implemented\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	DPRINT1("NtQueryInformationToken(%d) invalid parameter\n");
	Status = STATUS_INVALID_PARAMETER;
	break;
    }

  ObDereferenceObject(Token);

  return(Status);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeQueryInformationToken(
	IN PACCESS_TOKEN Token,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID *TokenInformation
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeQuerySessionIdToken(
	IN PACCESS_TOKEN Token,
	IN PULONG pSessionId
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * NtSetTokenInformation: Partly implemented.
 * Unimplemented:
 *  TokenOrigin, TokenDefaultDacl, TokenSessionId
 */

NTSTATUS STDCALL
NtSetInformationToken(IN HANDLE TokenHandle,
		      IN TOKEN_INFORMATION_CLASS TokenInformationClass,
		      OUT PVOID TokenInformation,
		      IN ULONG TokenInformationLength)
{
  NTSTATUS Status;
  PTOKEN Token;
  TOKEN_OWNER TokenOwnerSet = { 0 };
  TOKEN_PRIMARY_GROUP TokenPrimaryGroupSet = { 0 };
  DWORD NeededAccess = 0;

  switch (TokenInformationClass) 
    {
    case TokenOwner:
    case TokenPrimaryGroup:
      NeededAccess = TOKEN_ADJUST_DEFAULT;
      break;

    case TokenDefaultDacl:
      if (TokenInformationLength < sizeof(TOKEN_DEFAULT_DACL))
        return STATUS_BUFFER_TOO_SMALL;
      NeededAccess = TOKEN_ADJUST_DEFAULT;
      break;

    default:
      DPRINT1("NtSetInformationToken: lying about success (stub) - %x\n", TokenInformationClass);
      return STATUS_SUCCESS;  

    }

  Status = ObReferenceObjectByHandle(TokenHandle,
				     NeededAccess,
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
    case TokenOwner:
      MmCopyFromCaller( &TokenOwnerSet, TokenInformation,
			min(sizeof(TokenOwnerSet),TokenInformationLength) );
      RtlCopySid(TokenInformationLength - sizeof(TOKEN_OWNER),
		 Token->UserAndGroups[Token->DefaultOwnerIndex].Sid,
		 TokenOwnerSet.Owner);
      Status = STATUS_SUCCESS;
      DPRINT("NtSetInformationToken(TokenOwner)\n");
      break;
      
    case TokenPrimaryGroup:
      MmCopyFromCaller( &TokenPrimaryGroupSet, TokenInformation, 
			min(sizeof(TokenPrimaryGroupSet),
			    TokenInformationLength) );
      RtlCopySid(TokenInformationLength - sizeof(TOKEN_PRIMARY_GROUP),
		 Token->PrimaryGroup,
		 TokenPrimaryGroupSet.PrimaryGroup);
      Status = STATUS_SUCCESS;
      DPRINT("NtSetInformationToken(TokenPrimaryGroup),"
	     "Token->PrimaryGroup = 0x%08x\n", Token->PrimaryGroup);
      break;

    case TokenDefaultDacl:
      {
        TOKEN_DEFAULT_DACL TokenDefaultDacl = { 0 };
        ACL OldAcl;
        PACL NewAcl;

        Status = MmCopyFromCaller( &TokenDefaultDacl, TokenInformation, 
                                   sizeof(TOKEN_DEFAULT_DACL) );
        if (!NT_SUCCESS(Status))
          {
            Status = STATUS_INVALID_PARAMETER;
            break;
          }

        Status = MmCopyFromCaller( &OldAcl, TokenDefaultDacl.DefaultDacl,
                                   sizeof(ACL) );
        if (!NT_SUCCESS(Status))
          {
            Status = STATUS_INVALID_PARAMETER;
            break;
          }

        NewAcl = ExAllocatePool(NonPagedPool, sizeof(ACL));
        if (NewAcl == NULL)
          {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
          }

        Status = MmCopyFromCaller( NewAcl, TokenDefaultDacl.DefaultDacl,
                                   OldAcl.AclSize );
        if (!NT_SUCCESS(Status))
          {
            Status = STATUS_INVALID_PARAMETER;
            ExFreePool(NewAcl);
            break;
          }

        if (Token->DefaultDacl)
          {
            ExFreePool(Token->DefaultDacl);
          }

        Token->DefaultDacl = NewAcl;

        Status = STATUS_SUCCESS;
        break;
      }

    default:
      Status = STATUS_NOT_IMPLEMENTED;
      break;
    }

  ObDereferenceObject(Token);

  return(Status);
}


/*
 * @implemented
 *
 * NOTE: Some sources claim 4th param is ImpersonationLevel, but on W2K
 * this is certainly NOT true, thou i can't say for sure that EffectiveOnly
 * is correct either. -Gunnar
 * This is true. EffectiveOnly overrides SQOS.EffectiveOnly. - IAI
 */
NTSTATUS STDCALL
NtDuplicateToken(IN HANDLE ExistingTokenHandle,
		 IN ACCESS_MASK DesiredAccess,
       IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL /*is it really optional?*/,
       IN BOOLEAN EffectiveOnly,
		 IN TOKEN_TYPE TokenType,
		 OUT PHANDLE NewTokenHandle)
{
  KPROCESSOR_MODE PreviousMode;
  PTOKEN Token;
  PTOKEN NewToken;
  NTSTATUS Status;

  PreviousMode = KeGetPreviousMode();
  Status = ObReferenceObjectByHandle(ExistingTokenHandle,
				     TOKEN_DUPLICATE,
				     SepTokenObjectType,
				     PreviousMode,
				     (PVOID*)&Token,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to reference token (Status %lx)\n", Status);
      return Status;
    }

  Status = SepDuplicateToken(Token,
			     ObjectAttributes,
			     EffectiveOnly,
			     TokenType,
              ObjectAttributes->SecurityQualityOfService ? 
                  ((PSECURITY_QUALITY_OF_SERVICE)(ObjectAttributes->SecurityQualityOfService))->ImpersonationLevel : 
                  0 /*SecurityAnonymous*/,
			     PreviousMode,
			     &NewToken);

  ObDereferenceObject(Token);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to duplicate token (Status %lx)\n", Status);
      return Status;
    }

  Status = ObInsertObject((PVOID)NewToken,
			  NULL,
			  DesiredAccess,
			  0,
			  NULL,
			  NewTokenHandle);

  ObDereferenceObject(NewToken);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create token handle (Status %lx)\n");
      return Status;
    }

  return STATUS_SUCCESS;
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
   return(STATUS_NOT_IMPLEMENTED);
#endif
}


#if 0
NTSTATUS
SepAdjustPrivileges(PACCESS_TOKEN Token,
		    ULONG a,
		    KPROCESSOR_MODE PreviousMode,
		    ULONG PrivilegeCount,
		    PLUID_AND_ATTRIBUTES Privileges,
		    PTOKEN_PRIVILEGES* PreviousState,
		    PULONG b,
		    PULONG c,
		    PULONG d)
{
  ULONG i;

  *c = 0;

  if (Token->PrivilegeCount > 0)
    {
      for (i = 0; i < Token->PrivilegeCount; i++)
	{
	  if (PreviousMode != KernelMode)
	    {
	      if (Token->Privileges[i]->Attributes & SE_PRIVILEGE_ENABLED == 0)
		{
		  if (a != 0)
		    {
		      if (PreviousState != NULL)
			{
			  memcpy(&PreviousState[i],
				 &Token->Privileges[i],
				 sizeof(LUID_AND_ATTRIBUTES));
			}
		      Token->Privileges[i].Attributes &= (~SE_PRIVILEGE_ENABLED);
		    }
		}
	    }
	}
    }

  if (PreviousMode != KernelMode)
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


/*
 * @implemented
 */
NTSTATUS STDCALL
NtAdjustPrivilegesToken (IN HANDLE TokenHandle,
			 IN BOOLEAN DisableAllPrivileges,
			 IN PTOKEN_PRIVILEGES NewState,
			 IN ULONG BufferLength,
			 OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
			 OUT PULONG ReturnLength OPTIONAL)
{
//  PLUID_AND_ATTRIBUTES Privileges;
  KPROCESSOR_MODE PreviousMode;
//  ULONG PrivilegeCount;
  PTOKEN Token;
//  ULONG Length;
  ULONG i;
  ULONG j;
  ULONG k;
  ULONG Count;
#if 0
   ULONG a;
   ULONG b;
   ULONG c;
#endif
  NTSTATUS Status;

  DPRINT ("NtAdjustPrivilegesToken() called\n");

//  PrivilegeCount = NewState->PrivilegeCount;
  PreviousMode = KeGetPreviousMode ();
//  SeCaptureLuidAndAttributesArray(NewState->Privileges,
//				  PrivilegeCount,
//				  PreviousMode,
//				  NULL,
//				  0,
//				  NonPagedPool,
//				  1,
//				  &Privileges,
//				  &Length);

  Status = ObReferenceObjectByHandle (TokenHandle,
				      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				      SepTokenObjectType,
				      PreviousMode,
				      (PVOID*)&Token,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("Failed to reference token (Status %lx)\n", Status);
//      SeReleaseLuidAndAttributesArray(Privileges,
//				      PreviousMode,
//				      0);
      return Status;
    }


#if 0
   SepAdjustPrivileges(Token,
		       0,
		       PreviousMode,
		       PrivilegeCount,
		       Privileges,
		       PreviousState,
		       &a,
		       &b,
		       &c);
#endif

  k = 0;
  if (DisableAllPrivileges == TRUE)
    {
      for (i = 0; i < Token->PrivilegeCount; i++)
	{
	  if (Token->Privileges[i].Attributes != 0)
	    {
	      DPRINT ("Attributes differ\n");

	      /* Save current privilege */
	      if (PreviousState != NULL && k < PreviousState->PrivilegeCount)
		{
		  PreviousState->Privileges[k].Luid = Token->Privileges[i].Luid;
		  PreviousState->Privileges[k].Attributes = Token->Privileges[i].Attributes;
		  k++;
		}

	      /* Update current privlege */
	      Token->Privileges[i].Attributes &= ~SE_PRIVILEGE_ENABLED;
	    }
	}
      Status = STATUS_SUCCESS;
    }
  else
    {
      Count = 0;
      for (i = 0; i < Token->PrivilegeCount; i++)
	{
	  for (j = 0; j < NewState->PrivilegeCount; j++)
	    {
	      if (Token->Privileges[i].Luid.LowPart == NewState->Privileges[j].Luid.LowPart &&
		  Token->Privileges[i].Luid.HighPart == NewState->Privileges[j].Luid.HighPart)
		{
		  DPRINT ("Found privilege\n");

		  if ((Token->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) !=
		      (NewState->Privileges[j].Attributes & SE_PRIVILEGE_ENABLED))
		    {
		      DPRINT ("Attributes differ\n");
		      DPRINT ("Current attributes %lx  desired attributes %lx\n",
			      Token->Privileges[i].Attributes,
			      NewState->Privileges[j].Attributes);

		      /* Save current privilege */
		      if (PreviousState != NULL && k < PreviousState->PrivilegeCount)
			{
			  PreviousState->Privileges[k].Luid = Token->Privileges[i].Luid;
			  PreviousState->Privileges[k].Attributes = Token->Privileges[i].Attributes;
			  k++;
			}

		      /* Update current privlege */
		      Token->Privileges[i].Attributes &= ~SE_PRIVILEGE_ENABLED;
		      Token->Privileges[i].Attributes |= 
			(NewState->Privileges[j].Attributes & SE_PRIVILEGE_ENABLED);
		      DPRINT ("New attributes %lx\n",
			      Token->Privileges[i].Attributes);
		    }
                  Count++;
		}
	    }
	}
      Status = Count < NewState->PrivilegeCount ? STATUS_NOT_ALL_ASSIGNED : STATUS_SUCCESS;
    }

  if (ReturnLength != NULL)
    {
      *ReturnLength = sizeof(TOKEN_PRIVILEGES) +
		      (sizeof(LUID_AND_ATTRIBUTES) * (k - 1));
    }

  ObDereferenceObject (Token);

//  SeReleaseLuidAndAttributesArray(Privileges,
//				  PreviousMode,
//				  0);

  DPRINT ("NtAdjustPrivilegesToken() done\n");

  return Status;
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

  PTOKEN AccessToken;

  PVOID SidArea;

 /*
  * Initialize the token
  */
  Status = ObCreateObject(KernelMode,
			  SepTokenObjectType,
			  NULL,
			  KernelMode,
			  NULL,
			  sizeof(TOKEN),
			  0,
			  0,
			  (PVOID*)&AccessToken);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = NtAllocateLocallyUniqueId(&AccessToken->TokenId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  Status = NtAllocateLocallyUniqueId(&AccessToken->ModifiedId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  Status = NtAllocateLocallyUniqueId(&AccessToken->AuthenticationId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return Status;
    }

  AccessToken->TokenType = TokenPrimary;
  AccessToken->ImpersonationLevel = SecurityDelegation;
  AccessToken->TokenSource.SourceIdentifier.LowPart = 0;
  AccessToken->TokenSource.SourceIdentifier.HighPart = 0;
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
  SidArea = (char*)SidArea + uLocalSystemLength;

  AccessToken->DefaultOwnerIndex = i;
  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->PrimaryGroup = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT;
  Status = RtlCopySid(uAdminsLength, SidArea, SeAliasAdminsSid);
  SidArea = (char*)SidArea + uAdminsLength;

  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
  RtlCopySid(uWorldLength, SidArea, SeWorldSid);
  SidArea = (char*)SidArea + uWorldLength;

  AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
  RtlCopySid(uAuthUserLength, SidArea, SeAuthenticatedUserSid);
  SidArea = (char*)SidArea + uAuthUserLength;

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

  ASSERT(i == 20);

  uSize = sizeof(ACL);
  uSize += sizeof(ACE) + uLocalSystemLength;
  uSize += sizeof(ACE) + uAdminsLength;
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
  PTOKEN AccessToken;
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

  Status = ObCreateObject(ExGetPreviousMode(),
			  SepTokenObjectType,
			  ObjectAttributes,
			  ExGetPreviousMode(),
			  NULL,
			  sizeof(TOKEN),
			  0,
			  0,
			  (PVOID*)&AccessToken);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObCreateObject() failed (Status %lx)\n");
      return(Status);
    }

  Status = ObInsertObject ((PVOID)AccessToken,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   &TokenHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObInsertObject() failed (Status %lx)\n");
      ObDereferenceObject (AccessToken);
      return Status;
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
  AccessToken->ImpersonationLevel = ((PSECURITY_QUALITY_OF_SERVICE)
                                     (ObjectAttributes->SecurityQualityOfService))->ImpersonationLevel;

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


/*
 * @implemented
 */
NTSTATUS STDCALL
SeQueryAuthenticationIdToken(IN PACCESS_TOKEN Token,
			     OUT PLUID LogonId)
{
  *LogonId = ((PTOKEN)Token)->AuthenticationId;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
SECURITY_IMPERSONATION_LEVEL
STDCALL
SeTokenImpersonationLevel(IN PACCESS_TOKEN Token)
{
  return ((PTOKEN)Token)->ImpersonationLevel;
}


/*
 * @implemented
 */
TOKEN_TYPE STDCALL
SeTokenType(IN PACCESS_TOKEN Token)
{
  return ((PTOKEN)Token)->TokenType;
}


/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeTokenIsAdmin(
	IN PACCESS_TOKEN Token
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeTokenIsRestricted(
	IN PACCESS_TOKEN Token
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeTokenIsWriteRestricted(
	IN PACCESS_TOKEN Token
	)
{
	UNIMPLEMENTED;
	return FALSE;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenThreadTokenEx(IN HANDLE ThreadHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN BOOLEAN OpenAsSelf,
                    IN ULONG HandleAttributes,
                    OUT PHANDLE TokenHandle)
{
  PETHREAD Thread;
  PTOKEN Token, NewToken, PrimaryToken;
  BOOLEAN CopyOnOpen, EffectiveOnly;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  SE_IMPERSONATION_STATE ImpersonationState;
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_DESCRIPTOR SecurityDescriptor;
  PACL Dacl = NULL;
  NTSTATUS Status;

  /*
   * At first open the thread token for information access and verify
   * that the token associated with thread is valid.
   */

  Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_QUERY_INFORMATION,
                                     PsThreadType, UserMode, (PVOID*)&Thread,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Token = PsReferenceImpersonationToken(Thread, &CopyOnOpen, &EffectiveOnly,
                                        &ImpersonationLevel);
  if (Token == NULL)
    {
      ObfDereferenceObject(Thread);
      return STATUS_NO_TOKEN;
    }

  ObDereferenceObject(Thread);

  if (ImpersonationLevel == SecurityAnonymous)
    {
      ObfDereferenceObject(Token);
      return STATUS_CANT_OPEN_ANONYMOUS;
    }

  /*
   * Revert to self if OpenAsSelf is specified.
   */

  if (OpenAsSelf)
    {
      PsDisableImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }

  if (CopyOnOpen)
    {
      Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS,
                                         PsThreadType, UserMode,
                                         (PVOID*)&Thread, NULL);
      if (!NT_SUCCESS(Status))
        {
          ObfDereferenceObject(Token);
          if (OpenAsSelf)
            {
              PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
            }
          return Status;
        }
   
      PrimaryToken = PsReferencePrimaryToken(Thread->ThreadsProcess);
      Status = SepCreateImpersonationTokenDacl(Token, PrimaryToken, &Dacl);
      ObfDereferenceObject(PrimaryToken);
      ObfDereferenceObject(Thread);
      if (!NT_SUCCESS(Status))
        {
          ObfDereferenceObject(Token);
          if (OpenAsSelf)
            {
              PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
            }
          return Status;
        }
      
      RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                  SECURITY_DESCRIPTOR_REVISION);
      RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl,
                                   FALSE);

      InitializeObjectAttributes(&ObjectAttributes, NULL, HandleAttributes,
                                 NULL, &SecurityDescriptor);

      Status = SepDuplicateToken(Token, &ObjectAttributes, EffectiveOnly,
                                 TokenImpersonation, ImpersonationLevel,
                                 KernelMode, &NewToken);
      ExFreePool(Dacl);
      if (!NT_SUCCESS(Status))
        {
          ObfDereferenceObject(Token);
          if (OpenAsSelf)
            {
              PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
            }
          return Status;
        }

      Status = ObInsertObject(NewToken, NULL, DesiredAccess, 0, NULL,
                              TokenHandle);

      ObfDereferenceObject(NewToken);
    }
  else
    {
      Status = ObOpenObjectByPointer(Token, HandleAttributes,
                                     NULL, DesiredAccess, SepTokenObjectType,
                                     ExGetPreviousMode(), TokenHandle);
    }

  ObfDereferenceObject(Token);

  if (OpenAsSelf)
    {
      PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }

  return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtOpenThreadToken(IN HANDLE ThreadHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN BOOLEAN OpenAsSelf,
                  OUT PHANDLE TokenHandle)
{
  return NtOpenThreadTokenEx(ThreadHandle, DesiredAccess, OpenAsSelf, 0,
                             TokenHandle);
}

/* EOF */
