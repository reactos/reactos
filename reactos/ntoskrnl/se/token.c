/* $Id: token.c,v 1.20 2002/09/07 15:13:07 chorns Exp $
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

#undef SYSTEM_LUID
#define SYSTEM_LUID 999

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
}

NTSTATUS SeExchangePrimaryToken(PEPROCESS Process,
				PIACCESS_TOKEN NewToken,
				PIACCESS_TOKEN* OldTokenP)
{
   PIACCESS_TOKEN OldToken;
   
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


static ULONG
RtlLengthSidAndAttributes(ULONG Count,
			  PSID_AND_ATTRIBUTES_ARRAY Src)
{
  ULONG i;
  ULONG uLength;

  uLength = Count * sizeof(SID_AND_ATTRIBUTES);
  for (i = 0; i < Count; i++)
    uLength += RtlLengthSid(Src[i]->Sid);

  return(uLength);
}


NTSTATUS
SepFindPrimaryGroupAndDefaultOwner(PIACCESS_TOKEN Token,
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
	  RtlEqualSid(Token->UserAndGroups[i]->Sid, DefaultOwner))
	{
	  Token->DefaultOwnerIndex = i;
	}

      if (RtlEqualSid(Token->UserAndGroups[i]->Sid, PrimaryGroup))
	{
	  Token->PrimaryGroup = Token->UserAndGroups[i]->Sid;
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
SepDuplicateToken(PIACCESS_TOKEN Token,
		  POBJECT_ATTRIBUTES ObjectAttributes,
		  TOKEN_TYPE TokenType,
		  SECURITY_IMPERSONATION_LEVEL Level,
		  SECURITY_IMPERSONATION_LEVEL ExistingLevel,
		  KPROCESSOR_MODE PreviousMode,
		  PIACCESS_TOKEN* NewAccessToken)
{
  NTSTATUS Status;
  ULONG uLength;
  ULONG i;
  
  PVOID EndMem;

  PIACCESS_TOKEN AccessToken;

  Status = ObRosCreateObject(0,
			  TOKEN_ALL_ACCESS,
			  ObjectAttributes,
			  SepTokenObjectType,
			  (PVOID*)&AccessToken);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObRosCreateObject() failed (Status %lx)\n");
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
    uLength += RtlLengthSid(Token->UserAndGroups[i]->Sid);

  AccessToken->UserAndGroups = 
    (PSID_AND_ATTRIBUTES_ARRAY)ExAllocatePoolWithTag(NonPagedPool,
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
	(PLUID_AND_ATTRIBUTES_ARRAY)ExAllocatePoolWithTag(NonPagedPool,
						    uLength,
						    TAG('T', 'O', 'K', 'p'));

      for (i = 0; i < AccessToken->PrivilegeCount; i++)
	{
	  RtlCopyLuid(&AccessToken->Privileges[i]->Luid,
	    &Token->Privileges[i]->Luid);
	  AccessToken->Privileges[i]->Attributes = 
	    Token->Privileges[i]->Attributes;
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
  PIACCESS_TOKEN pNewToken;
  PIACCESS_TOKEN pParentToken;
  
  OBJECT_ATTRIBUTES ObjectAttributes;

  pParentToken = (PIACCESS_TOKEN) ParentProcess->Token;

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

NTSTATUS SeCopyClientToken(PIACCESS_TOKEN Token,
			   SECURITY_IMPERSONATION_LEVEL Level,
			   KPROCESSOR_MODE PreviousMode,
			   PIACCESS_TOKEN* NewToken)
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
   PIACCESS_TOKEN iToken;
   ULONG g;
   PIACCESS_TOKEN NewToken;
   
   iToken = PsReferenceEffectiveToken(Thread,
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
	     if (iToken != NULL)
	       {
		  ObDereferenceObject(iToken);
	       }
	     return(STATUS_UNSUCCESSFUL);
	  }
	if (ImpersonationLevel == 0 ||
	    ImpersonationLevel == 1 ||
	    (RemoteClient != FALSE && ImpersonationLevel != 3))
	  {
	     if (iToken != NULL)
	       {
		  ObDereferenceObject(iToken);
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
	g = SeCopyClientToken(iToken, ImpersonationLevel, 0, &NewToken);
	if (g >= 0)
	  {
//	     ObDeleteCapturedInsertInfo(NewToken);
	  }
	if (TokenType == TokenPrimary || iToken != NULL)
	  {
	     ObDereferenceObject(iToken);
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
//	     SeGetTokenControlInformation(iToken, &ClientContext->Unknown11);
	  }
	NewToken = iToken;
    }
  ClientContext->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  ClientContext->SecurityQos.ImpersonationLevel = Qos->ImpersonationLevel;
  ClientContext->SecurityQos.ContextTrackingMode = Qos->ContextTrackingMode;
  ClientContext->SecurityQos.EffectiveOnly = Qos->EffectiveOnly;
  ClientContext->ServerIsRemote = RemoteClient;
  ClientContext->ClientToken = NewToken;

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
		      ClientContext->ClientToken,
		      1,
		      (ULONG)b,
		      ClientContext->SecurityQos.ImpersonationLevel);
}


VOID STDCALL
SepDeleteToken(PVOID ObjectBody)
{
  PIACCESS_TOKEN AccessToken = (PIACCESS_TOKEN)ObjectBody;

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
  SepTokenObjectType->NonpagedPoolCharge = sizeof(IACCESS_TOKEN);
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
  PIACCESS_TOKEN iToken;
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
				     (PVOID*)&iToken,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  switch (TokenInformationClass)
    {
      case TokenUser:
        DPRINT("NtQueryInformationToken(TokenUser)\n");
	uLength = RtlLengthSidAndAttributes(1, iToken->UserAndGroups);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    Status = RtlCopySidAndAttributesArray(1,
						  iToken->UserAndGroups,
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
	uLength = RtlLengthSidAndAttributes(iToken->UserAndGroupCount - 1, &iToken->UserAndGroups[1]) + sizeof(DWORD);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    EndMem = TokenInformation + iToken->UserAndGroupCount * sizeof(SID_AND_ATTRIBUTES);
	    PtrTokenGroups = (PTOKEN_GROUPS)TokenInformation;
	    PtrTokenGroups->GroupCount = iToken->UserAndGroupCount - 1;
	    Status = RtlCopySidAndAttributesArray(iToken->UserAndGroupCount - 1,
						  (PSID_AND_ATTRIBUTES_ARRAY)&iToken->UserAndGroups[1],
						  TokenInformationLength,
						  (PSID_AND_ATTRIBUTES_ARRAY)&PtrTokenGroups->Groups[0],
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
	uLength = sizeof(DWORD) + iToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
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

	    pPriv->PrivilegeCount = iToken->PrivilegeCount;
	    for (i = 0; i < iToken->PrivilegeCount; i++)
	      {
		RtlCopyLuid(&pPriv->Privileges[i].Luid, &iToken->Privileges[i]->Luid);
		pPriv->Privileges[i].Attributes = iToken->Privileges[i]->Attributes;
	      }
	    Status = STATUS_SUCCESS;
	  }
	break;

      case TokenOwner:
        DPRINT("NtQueryInformationToken(TokenOwner)\n");
	uLength = RtlLengthSid(iToken->UserAndGroups[iToken->DefaultOwnerIndex]->Sid) + sizeof(TOKEN_OWNER);
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
		       iToken->UserAndGroups[iToken->DefaultOwnerIndex]->Sid);
	    Status = STATUS_SUCCESS;
	  }
	break;

      case TokenPrimaryGroup:
        DPRINT("NtQueryInformationToken(TokenPrimaryGroup),"
	       "Token->PrimaryGroup = 0x%08x\n", Token->PrimaryGroup);
	uLength = RtlLengthSid(iToken->PrimaryGroup) + sizeof(TOKEN_PRIMARY_GROUP);
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
		       iToken->PrimaryGroup);
	    Status = STATUS_SUCCESS;
	  }
	break;

      case TokenDefaultDacl:
        DPRINT("NtQueryInformationToken(TokenDefaultDacl)\n");
	PtrDefaultDacl = (PTOKEN_DEFAULT_DACL) TokenInformation;
	uLength = (iToken->DefaultDacl ? iToken->DefaultDacl->AclSize : 0) + sizeof(TOKEN_DEFAULT_DACL);
	if (TokenInformationLength < uLength)
	  {
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else if (!iToken->DefaultDacl)
	  {
	    PtrDefaultDacl->DefaultDacl = 0;
	    Status = MmCopyToCaller(ReturnLength, &uLength, sizeof(ULONG));
	  }
	else
	  {
	    PtrDefaultDacl->DefaultDacl = (PACL) (PtrDefaultDacl + 1);
	    memmove(PtrDefaultDacl->DefaultDacl,
		    iToken->DefaultDacl,
		    iToken->DefaultDacl->AclSize);
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
	    Status = MmCopyToCaller(TokenInformation, &iToken->TokenSource, sizeof(TOKEN_SOURCE));
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
	    Status = MmCopyToCaller(TokenInformation, &iToken->TokenType, sizeof(TOKEN_TYPE));
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
	    Status = MmCopyToCaller(TokenInformation, &iToken->ImpersonationLevel,
        sizeof(SECURITY_IMPERSONATION_LEVEL));
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
	    PtrTokenStatistics->TokenId = iToken->TokenId;
	    PtrTokenStatistics->AuthenticationId = iToken->AuthenticationId;
	    PtrTokenStatistics->ExpirationTime = iToken->ExpirationTime;
	    PtrTokenStatistics->TokenType = iToken->TokenType;
	    PtrTokenStatistics->ImpersonationLevel = iToken->ImpersonationLevel;
	    PtrTokenStatistics->DynamicCharged = iToken->DynamicCharged;
	    PtrTokenStatistics->DynamicAvailable = iToken->DynamicAvailable;
	    PtrTokenStatistics->GroupCount = iToken->UserAndGroupCount - 1;
	    PtrTokenStatistics->PrivilegeCount = iToken->PrivilegeCount;
	    PtrTokenStatistics->ModifiedId = iToken->ModifiedId;

	    Status = STATUS_SUCCESS;
	  }
	break;
  default:
    break;
    }

  ObDereferenceObject(iToken);

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
NtDuplicateToken(IN HANDLE  ExistingTokenHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  EffectiveOnly,
  IN TOKEN_TYPE  TokenType,
  OUT PHANDLE  NewTokenHandle)
{
#if 0
   PIACCESS_TOKEN Token;
   PIACCESS_TOKEN NewToken;
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

VOID SepAdjustGroups(PIACCESS_TOKEN Token,
		     ULONG a,
		     BOOLEAN ResetToDefault,
		     PSID_AND_ATTRIBUTES_ARRAY Groups,
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
   PIACCESS_TOKEN Token;
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
NTSTATUS SepAdjustPrivileges(PIACCESS_TOKEN Token,           // 0x8
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

  PIACCESS_TOKEN AccessToken;

  PVOID SidArea;

  DPRINT("SepCreateSystemProcessToken\n");

 /*
  * Initialize the token
  */
  Status = ObRosCreateObject(NULL,
			 TOKEN_ALL_ACCESS,
			 NULL,
			 SepTokenObjectType,
			 (PVOID*)&AccessToken);
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
    (PSID_AND_ATTRIBUTES_ARRAY)ExAllocatePoolWithTag(NonPagedPool,
					       uSize,
					       TAG('T', 'O', 'K', 'u'));
  SidArea = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

  i = 0;
  AccessToken->UserAndGroups[i]->Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++]->Attributes = 0;
  RtlCopySid(uLocalSystemLength, SidArea, SeLocalSystemSid);
  SidArea += uLocalSystemLength;

  AccessToken->DefaultOwnerIndex = i;
  AccessToken->UserAndGroups[i]->Sid = (PSID) SidArea;
  AccessToken->PrimaryGroup = (PSID) SidArea;
  AccessToken->UserAndGroups[i++]->Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT;
  Status = RtlCopySid(uAdminsLength, SidArea, SeAliasAdminsSid);
  SidArea += uAdminsLength;

  AccessToken->UserAndGroups[i]->Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++]->Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
  RtlCopySid(uWorldLength, SidArea, SeWorldSid);
  SidArea += uWorldLength;

  AccessToken->UserAndGroups[i]->Sid = (PSID) SidArea;
  AccessToken->UserAndGroups[i++]->Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
  RtlCopySid(uAuthUserLength, SidArea, SeAuthenticatedUserSid);
  SidArea += uAuthUserLength;

  AccessToken->PrivilegeCount = 20;

  uSize = AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
  AccessToken->Privileges =
	(PLUID_AND_ATTRIBUTES_ARRAY)ExAllocatePoolWithTag(NonPagedPool,
						    uSize,
						    TAG('T', 'O', 'K', 'p'));

  i = 0;
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeTcbPrivilege;

  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeCreateTokenPrivilege;

  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeTakeOwnershipPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeCreatePagefilePrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeLockMemoryPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeAssignPrimaryTokenPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeIncreaseQuotaPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeIncreaseBasePriorityPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeCreatePermanentPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeDebugPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeAuditPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeSecurityPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeSystemEnvironmentPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeChangeNotifyPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeBackupPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeRestorePrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeShutdownPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeLoadDriverPrivilege;
  
  AccessToken->Privileges[i]->Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
  AccessToken->Privileges[i++]->Luid = SeProfileSingleProcessPrivilege;
  
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeSystemtimePrivilege;
#if 0
  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeUndockPrivilege;

  AccessToken->Privileges[i]->Attributes = 0;
  AccessToken->Privileges[i++]->Luid = SeManageVolumePrivilege;
#endif

  assert( i == 20 );

  uSize = sizeof(ACL);
  uSize += sizeof(ROS_ACE_HEADER) + uLocalSystemLength;
  uSize += sizeof(ROS_ACE_HEADER) + uAdminsLength;
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
  PIACCESS_TOKEN AccessToken;
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

  Status = ObRosCreateObject(&TokenHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  SepTokenObjectType,
			  (PVOID*)&AccessToken);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObRosCreateObject() failed (Status %lx)\n");
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
  AccessToken->ImpersonationLevel = ((PSECURITY_QUALITY_OF_SERVICE)
		ObjectAttributes->SecurityQualityOfService)->ImpersonationLevel;

  /*
   * Normally we would just point these members into the variable information
   * area; however, our ObRosCreateObject() call can't allocate a variable information
   * area, so we allocate them seperately and provide a destroy function.
   */

  uLength = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
  uLength += RtlLengthSid(TokenUser->User.Sid);
  for (i = 0; i < TokenGroups->GroupCount; i++)
    uLength += RtlLengthSid(TokenGroups->Groups[i].Sid);

  AccessToken->UserAndGroups = 
    (PSID_AND_ATTRIBUTES_ARRAY)ExAllocatePoolWithTag(NonPagedPool,
					       uLength,
					       TAG('T', 'O', 'K', 'u'));

  EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

  Status = RtlCopySidAndAttributesArray(1,
					(PSID_AND_ATTRIBUTES_ARRAY)&TokenUser->User,
					uLength,
					(PSID_AND_ATTRIBUTES_ARRAY)AccessToken->UserAndGroups,
					EndMem,
					&EndMem,
					&uLength);
  if (NT_SUCCESS(Status))
    {
      Status = RtlCopySidAndAttributesArray(TokenGroups->GroupCount,
					    (PSID_AND_ATTRIBUTES_ARRAY)&TokenGroups->Groups[0],
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
	(PLUID_AND_ATTRIBUTES_ARRAY)ExAllocatePoolWithTag(NonPagedPool,
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
  PIACCESS_TOKEN iToken = (PIACCESS_TOKEN)Token;

  return(iToken->ImpersonationLevel);
}


TOKEN_TYPE STDCALL
SeTokenType(IN PACCESS_TOKEN Token)
{
  PIACCESS_TOKEN iToken = (PIACCESS_TOKEN)Token;

  return(iToken->TokenType);
}

/* EOF */
