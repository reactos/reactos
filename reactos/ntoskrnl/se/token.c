/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/token.c
 * PURPOSE:         Security manager
 * 
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE SepTokenObjectType = NULL;
ERESOURCE SepTokenLock;

static GENERIC_MAPPING SepTokenMapping = {TOKEN_READ,
					  TOKEN_WRITE,
					  TOKEN_EXECUTE,
					  TOKEN_ALL_ACCESS};

static const INFORMATION_CLASS_INFO SeTokenInformationClass[] = {

    /* Class 0 not used, blame M$! */
    ICI_SQ_SAME( 0, 0, 0),

    /* TokenUser */
    ICI_SQ_SAME( sizeof(TOKEN_USER),                   sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenGroups */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenPrivileges */
    ICI_SQ_SAME( sizeof(TOKEN_PRIVILEGES),             sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenOwner */
    ICI_SQ_SAME( sizeof(TOKEN_OWNER),                  sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenPrimaryGroup */
    ICI_SQ_SAME( sizeof(TOKEN_PRIMARY_GROUP),          sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenDefaultDacl */
    ICI_SQ_SAME( sizeof(TOKEN_DEFAULT_DACL),           sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenSource */
    ICI_SQ_SAME( sizeof(TOKEN_SOURCE),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenType */
    ICI_SQ_SAME( sizeof(TOKEN_TYPE),                   sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenImpersonationLevel */
    ICI_SQ_SAME( sizeof(SECURITY_IMPERSONATION_LEVEL), sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenStatistics */
    ICI_SQ_SAME( sizeof(TOKEN_STATISTICS),             sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenRestrictedSids */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSessionId */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_QUERY | ICIF_SET ),
    /* TokenGroupsAndPrivileges */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS_AND_PRIVILEGES),  sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSessionReference */
    ICI_SQ_SAME( /* FIXME */0,                         sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSandBoxInert */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenAuditPolicy */
    ICI_SQ_SAME( /* FIXME */0,                         sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenOrigin */
    ICI_SQ_SAME( sizeof(TOKEN_ORIGIN),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
};

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
   
   PAGED_CODE();
   
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
  
  PAGED_CODE();

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
  ULONG uLength;
  ULONG i;
  PVOID EndMem;
  PTOKEN AccessToken;
  NTSTATUS Status;
  
  PAGED_CODE();

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

  AccessToken->TokenLock = &SepTokenLock;

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
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
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
	(PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
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
	    (PACL) ExAllocatePoolWithTag(PagedPool,
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
   
   PAGED_CODE();
     
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
			        (PTOKEN*)NewToken);
   
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
   
   PAGED_CODE();
   
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
  
  PAGED_CODE();
  
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
  ExInitializeResource(&SepTokenLock);

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

  RtlInitUnicodeString(&SepTokenObjectType->TypeName, L"Token");
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
  union
  {
    PVOID Ptr;
    ULONG Ulong;
  } Unused;
  PTOKEN Token;
  ULONG RequiredLength;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();
  
  /* Check buffers and class validity */
  DefaultQueryInfoBufferCheck(TokenInformationClass,
                              SeTokenInformationClass,
                              TokenInformation,
                              TokenInformationLength,
                              ReturnLength,
                              PreviousMode,
                              &Status);

  if(!NT_SUCCESS(Status))
  {
    DPRINT("NtQueryInformationToken() failed, Status: 0x%x\n", Status);
    return Status;
  }

  Status = ObReferenceObjectByHandle(TokenHandle,
				     (TokenInformationClass == TokenSource) ? TOKEN_QUERY_SOURCE : TOKEN_QUERY,
				     SepTokenObjectType,
				     PreviousMode,
				     (PVOID*)&Token,
				     NULL);
  if (NT_SUCCESS(Status))
  {
    switch (TokenInformationClass)
    {
      case TokenUser:
      {
        PTOKEN_USER tu = (PTOKEN_USER)TokenInformation;
        
        DPRINT("NtQueryInformationToken(TokenUser)\n");
        RequiredLength = sizeof(TOKEN_USER) +
                         RtlLengthSid(Token->UserAndGroups[0].Sid);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            Status = RtlCopySidAndAttributesArray(1,
                                                  &Token->UserAndGroups[0],
                                                  RequiredLength - sizeof(TOKEN_USER),
                                                  &tu->User,
                                                  (PSID)(tu + 1),
                                                  &Unused.Ptr,
                                                  &Unused.Ulong);
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }
          
          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        break;
      }
	
      case TokenGroups:
      {
        PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;
        
        DPRINT("NtQueryInformationToken(TokenGroups)\n");
        RequiredLength = sizeof(tg->GroupCount) +
                         RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                           ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES));
            PSID_AND_ATTRIBUTES Sid = (PSID_AND_ATTRIBUTES)((ULONG_PTR)TokenInformation + sizeof(tg->GroupCount) +
                                                            ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES)));

            tg->GroupCount = Token->UserAndGroupCount - 1;
            Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
                                                  &Token->UserAndGroups[1],
                                                  SidLen,
                                                  &tg->Groups[0],
                                                  (PSID)Sid,
                                                  &Unused.Ptr,
                                                  &Unused.Ulong);
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
	break;
      }
      
      case TokenPrivileges:
      {
        PTOKEN_PRIVILEGES tp = (PTOKEN_PRIVILEGES)TokenInformation;
        
        DPRINT("NtQueryInformationToken(TokenPrivileges)\n");
        RequiredLength = sizeof(tp->PrivilegeCount) +
                         (Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            tp->PrivilegeCount = Token->PrivilegeCount;
            RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                          Token->Privileges,
                                          &tp->Privileges[0]);
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }
          
          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        break;
      }
      
      case TokenOwner:
      {
        ULONG SidLen;
        PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
        
        DPRINT("NtQueryInformationToken(TokenOwner)\n");
        SidLen = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
        RequiredLength = sizeof(TOKEN_OWNER) + SidLen;

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            to->Owner = (PSID)(to + 1);
            Status = RtlCopySid(SidLen,
                                to->Owner,
                                Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }
          
          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        break;
      }
      
      case TokenPrimaryGroup:
      {
        ULONG SidLen;
        PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenPrimaryGroup)\n");
        SidLen = RtlLengthSid(Token->PrimaryGroup);
        RequiredLength = sizeof(TOKEN_PRIMARY_GROUP) + SidLen;

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            tpg->PrimaryGroup = (PSID)(tpg + 1);
            Status = RtlCopySid(SidLen,
                                tpg->PrimaryGroup,
                                Token->PrimaryGroup);
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }
      
      case TokenDefaultDacl:
      {
        PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenDefaultDacl)\n");
        RequiredLength = sizeof(TOKEN_DEFAULT_DACL);
        
        if(Token->DefaultDacl != NULL)
        {
          RequiredLength += Token->DefaultDacl->AclSize;
        }

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            if(Token->DefaultDacl != NULL)
            {
              tdd->DefaultDacl = (PACL)(tdd + 1);
              RtlCopyMemory(tdd->DefaultDacl,
                            Token->DefaultDacl,
                            Token->DefaultDacl->AclSize);
            }
            else
            {
              tdd->DefaultDacl = NULL;
            }
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }
      
      case TokenSource:
      {
        PTOKEN_SOURCE ts = (PTOKEN_SOURCE)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenSource)\n");
        RequiredLength = sizeof(TOKEN_SOURCE);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            *ts = Token->TokenSource;
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }
      
      case TokenType:
      {
        PTOKEN_TYPE tt = (PTOKEN_TYPE)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenType)\n");
        RequiredLength = sizeof(TOKEN_TYPE);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            *tt = Token->TokenType;
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }
      
      case TokenImpersonationLevel:
      {
        PSECURITY_IMPERSONATION_LEVEL sil = (PSECURITY_IMPERSONATION_LEVEL)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenImpersonationLevel)\n");
        RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            *sil = Token->ImpersonationLevel;
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }
      
      case TokenStatistics:
      {
        PTOKEN_STATISTICS ts = (PTOKEN_STATISTICS)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenStatistics)\n");
        RequiredLength = sizeof(TOKEN_STATISTICS);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            ts->TokenId = Token->TokenId;
            ts->AuthenticationId = Token->AuthenticationId;
            ts->ExpirationTime = Token->ExpirationTime;
            ts->TokenType = Token->TokenType;
            ts->ImpersonationLevel = Token->ImpersonationLevel;
            ts->DynamicCharged = Token->DynamicCharged;
            ts->DynamicAvailable = Token->DynamicAvailable;
            ts->GroupCount = Token->UserAndGroupCount - 1;
            ts->PrivilegeCount = Token->PrivilegeCount;
            ts->ModifiedId = Token->ModifiedId;
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }
      
      case TokenOrigin:
      {
        PTOKEN_ORIGIN to = (PTOKEN_ORIGIN)TokenInformation;

        DPRINT("NtQueryInformationToken(TokenOrigin)\n");
        RequiredLength = sizeof(TOKEN_ORIGIN);

        _SEH_TRY
        {
          if(TokenInformationLength >= RequiredLength)
          {
            RtlCopyLuid(&to->OriginatingLogonSession,
                        &Token->AuthenticationId);
          }
          else
          {
            Status = STATUS_BUFFER_TOO_SMALL;
          }

          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredLength;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        break;
      }

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
      {
        ULONG SessionId = 0;

        DPRINT("NtQueryInformationToken(TokenSessionId)\n");
        
        Status = SeQuerySessionIdToken(Token,
                                       &SessionId);

        if(NT_SUCCESS(Status))
        {
          _SEH_TRY
          {
            /* buffer size was already verified, no need to check here again */
            *(PULONG)TokenInformation = SessionId;

            if(ReturnLength != NULL)
            {
              *ReturnLength = sizeof(ULONG);
            }
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
        }
        
        break;
      }

      default:
	DPRINT1("NtQueryInformationToken(%d) invalid information class\n", TokenInformationClass);
	Status = STATUS_INVALID_INFO_CLASS;
	break;
    }

    ObDereferenceObject(Token);
  }

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
 * @implemented
 */
NTSTATUS
STDCALL
SeQuerySessionIdToken(
	IN PACCESS_TOKEN Token,
	IN PULONG pSessionId
	)
{
  *pSessionId = ((PTOKEN)Token)->SessionId;
  return STATUS_SUCCESS;
}

/*
 * NtSetTokenInformation: Partly implemented.
 * Unimplemented:
 *  TokenOrigin, TokenDefaultDacl
 */

NTSTATUS STDCALL
NtSetInformationToken(IN HANDLE TokenHandle,
		      IN TOKEN_INFORMATION_CLASS TokenInformationClass,
		      OUT PVOID TokenInformation,
		      IN ULONG TokenInformationLength)
{
  PTOKEN Token;
  KPROCESSOR_MODE PreviousMode;
  ULONG NeededAccess = TOKEN_ADJUST_DEFAULT;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();
  
  DefaultSetInfoBufferCheck(TokenInformationClass,
                            SeTokenInformationClass,
                            TokenInformation,
                            TokenInformationLength,
                            PreviousMode,
                            &Status);

  if(!NT_SUCCESS(Status))
  {
    /* Invalid buffers */
    DPRINT("NtSetInformationToken() failed, Status: 0x%x\n", Status);
    return Status;
  }
  
  if(TokenInformationClass == TokenSessionId)
  {
    NeededAccess |= TOKEN_ADJUST_SESSIONID;
  }

  Status = ObReferenceObjectByHandle(TokenHandle,
				     NeededAccess,
				     SepTokenObjectType,
				     PreviousMode,
				     (PVOID*)&Token,
				     NULL);
  if (NT_SUCCESS(Status))
  {
    switch (TokenInformationClass)
    {
      case TokenOwner:
      {
        if(TokenInformationLength >= sizeof(TOKEN_OWNER))
        {
          PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
          PSID InputSid = NULL;
          
          _SEH_TRY
          {
            InputSid = to->Owner;
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
          
          if(NT_SUCCESS(Status))
          {
            PSID CapturedSid;
            
            Status = SepCaptureSid(InputSid,
                                   PreviousMode,
                                   PagedPool,
                                   FALSE,
                                   &CapturedSid);
            if(NT_SUCCESS(Status))
            {
              RtlCopySid(RtlLengthSid(CapturedSid),
                         Token->UserAndGroups[Token->DefaultOwnerIndex].Sid,
                         CapturedSid);
              SepReleaseSid(CapturedSid,
                            PreviousMode,
                            FALSE);
            }
          }
        }
        else
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
      }
      
      case TokenPrimaryGroup:
      {
        if(TokenInformationLength >= sizeof(TOKEN_PRIMARY_GROUP))
        {
          PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
          PSID InputSid = NULL;

          _SEH_TRY
          {
            InputSid = tpg->PrimaryGroup;
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;

          if(NT_SUCCESS(Status))
          {
            PSID CapturedSid;

            Status = SepCaptureSid(InputSid,
                                   PreviousMode,
                                   PagedPool,
                                   FALSE,
                                   &CapturedSid);
            if(NT_SUCCESS(Status))
            {
              RtlCopySid(RtlLengthSid(CapturedSid),
                         Token->PrimaryGroup,
                         CapturedSid);
              SepReleaseSid(CapturedSid,
                            PreviousMode,
                            FALSE);
            }
          }
        }
        else
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
      }
      
      case TokenDefaultDacl:
      {
        if(TokenInformationLength >= sizeof(TOKEN_DEFAULT_DACL))
        {
          PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;
          PACL InputAcl = NULL;

          _SEH_TRY
          {
            InputAcl = tdd->DefaultDacl;
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;

          if(NT_SUCCESS(Status))
          {
            if(InputAcl != NULL)
            {
              PACL CapturedAcl;

              /* capture and copy the dacl */
              Status = SepCaptureAcl(InputAcl,
                                     PreviousMode,
                                     PagedPool,
                                     TRUE,
                                     &CapturedAcl);
              if(NT_SUCCESS(Status))
              {
                /* free the previous dacl if present */
                if(Token->DefaultDacl != NULL)
                {
                  ExFreePool(Token->DefaultDacl);
                }
                
                /* set the new dacl */
                Token->DefaultDacl = CapturedAcl;
              }
            }
            else
            {
              /* clear and free the default dacl if present */
              if(Token->DefaultDacl != NULL)
              {
                ExFreePool(Token->DefaultDacl);
                Token->DefaultDacl = NULL;
              }
            }
          }
        }
        else
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
      }
      
      case TokenSessionId:
      {
        ULONG SessionId = 0;

        _SEH_TRY
        {
          /* buffer size was already verified, no need to check here again */
          SessionId = *(PULONG)TokenInformation;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
          if(!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                     PreviousMode))
          {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
          }

          Token->SessionId = SessionId;
        }
        break;
      }

      default:
      {
        Status = STATUS_NOT_IMPLEMENTED;
        break;
      }
    }

    ObDereferenceObject(Token);
  }

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
                 IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                 IN BOOLEAN EffectiveOnly,
                 IN TOKEN_TYPE TokenType,
                 OUT PHANDLE NewTokenHandle)
{
  KPROCESSOR_MODE PreviousMode;
  HANDLE hToken;
  PTOKEN Token;
  PTOKEN NewToken;
  PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService;
  BOOLEAN QoSPresent;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PAGED_CODE();

  PreviousMode = KeGetPreviousMode();
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(NewTokenHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  
  Status = SepCaptureSecurityQualityOfService(ObjectAttributes,
                                              PreviousMode,
                                              PagedPool,
                                              FALSE,
                                              &CapturedSecurityQualityOfService,
                                              &QoSPresent);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("NtDuplicateToken() failed to capture QoS! Status: 0x%x\n", Status);
    return Status;
  }
  
  Status = ObReferenceObjectByHandle(ExistingTokenHandle,
				     TOKEN_DUPLICATE,
				     SepTokenObjectType,
				     PreviousMode,
				     (PVOID*)&Token,
				     NULL);
  if (NT_SUCCESS(Status))
  {
    Status = SepDuplicateToken(Token,
                               ObjectAttributes,
                               EffectiveOnly,
                               TokenType,
                               (QoSPresent ? CapturedSecurityQualityOfService->ImpersonationLevel : SecurityAnonymous),
  			       PreviousMode,
  			       &NewToken);

    ObDereferenceObject(Token);

    if (NT_SUCCESS(Status))
    {
      Status = ObInsertObject((PVOID)NewToken,
    			  NULL,
    			  DesiredAccess,
    			  0,
    			  NULL,
    			  &hToken);

      ObDereferenceObject(NewToken);

      if (NT_SUCCESS(Status))
      {
        _SEH_TRY
        {
          *NewTokenHandle = hToken;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
      }
    }
  }
  
  /* free the captured structure */
  SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                     PreviousMode,
                                     FALSE);

  return Status;
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
   
   PAGED_CODE();
   
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
  ULONG PrivilegeCount;
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
  
  PAGED_CODE();

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
				      TOKEN_ADJUST_PRIVILEGES | (PreviousState != NULL ? TOKEN_QUERY : 0),
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

  PrivilegeCount = (BufferLength - FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges)) /
                   sizeof(LUID_AND_ATTRIBUTES);

  if (PreviousState != NULL)
    PreviousState->PrivilegeCount = 0;

  k = 0;
  if (DisableAllPrivileges == TRUE)
    {
      for (i = 0; i < Token->PrivilegeCount; i++)
	{
	  if (Token->Privileges[i].Attributes != 0)
	    {
	      DPRINT ("Attributes differ\n");

	      /* Save current privilege */
	      if (PreviousState != NULL)
		{
                  if (k < PrivilegeCount)
                    {
                      PreviousState->PrivilegeCount++;
                      PreviousState->Privileges[k].Luid = Token->Privileges[i].Luid;
                      PreviousState->Privileges[k].Attributes = Token->Privileges[i].Attributes;
                    }
                  else
                    {
                      /* FIXME: Should revert all the changes, calculate how
                       * much space would be needed, set ResultLength
                       * accordingly and fail.
                       */
                    }
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
		      if (PreviousState != NULL)
			{
                          if (k < PrivilegeCount)
                            {
                              PreviousState->PrivilegeCount++;
                              PreviousState->Privileges[k].Luid = Token->Privileges[i].Luid;
                              PreviousState->Privileges[k].Attributes = Token->Privileges[i].Attributes;
                            }
                          else
                            {
                              /* FIXME: Should revert all the changes, calculate how
                               * much space would be needed, set ResultLength
                               * accordingly and fail.
                               */
                            }
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
  
  PAGED_CODE();

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

  Status = ExpAllocateLocallyUniqueId(&AccessToken->TokenId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  Status = ExpAllocateLocallyUniqueId(&AccessToken->ModifiedId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return(Status);
    }

  Status = ExpAllocateLocallyUniqueId(&AccessToken->AuthenticationId);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(AccessToken);
      return Status;
    }

  AccessToken->TokenLock = &SepTokenLock;

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
NtCreateToken(OUT PHANDLE TokenHandle,
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
  HANDLE hToken;
  PTOKEN AccessToken;
  LUID TokenId;
  LUID ModifiedId;
  PVOID EndMem;
  ULONG uLength;
  ULONG i;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(TokenHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
      ProbeForRead(AuthenticationId,
                   sizeof(LUID),
                   sizeof(ULONG));
      ProbeForRead(ExpirationTime,
                   sizeof(LARGE_INTEGER),
                   sizeof(ULONG));
      ProbeForRead(TokenUser,
                   sizeof(TOKEN_USER),
                   sizeof(ULONG));
      ProbeForRead(TokenGroups,
                   sizeof(TOKEN_GROUPS),
                   sizeof(ULONG));
      ProbeForRead(TokenPrivileges,
                   sizeof(TOKEN_PRIVILEGES),
                   sizeof(ULONG));
      ProbeForRead(TokenOwner,
                   sizeof(TOKEN_OWNER),
                   sizeof(ULONG));
      ProbeForRead(TokenPrimaryGroup,
                   sizeof(TOKEN_PRIMARY_GROUP),
                   sizeof(ULONG));
      ProbeForRead(TokenDefaultDacl,
                   sizeof(TOKEN_DEFAULT_DACL),
                   sizeof(ULONG));
      ProbeForRead(TokenSource,
                   sizeof(TOKEN_SOURCE),
                   sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  Status = ZwAllocateLocallyUniqueId(&TokenId);
  if (!NT_SUCCESS(Status))
    return(Status);

  Status = ZwAllocateLocallyUniqueId(&ModifiedId);
  if (!NT_SUCCESS(Status))
    return(Status);

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

  AccessToken->TokenLock = &SepTokenLock;

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
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
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
	(PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
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
	(PACL) ExAllocatePoolWithTag(PagedPool,
				     TokenDefaultDacl->DefaultDacl->AclSize,
				     TAG('T', 'O', 'K', 'd'));
      memcpy(AccessToken->DefaultDacl,
	     TokenDefaultDacl->DefaultDacl,
	     TokenDefaultDacl->DefaultDacl->AclSize);
    }

  Status = ObInsertObject ((PVOID)AccessToken,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   &hToken);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObInsertObject() failed (Status %lx)\n", Status);
    }

  ObDereferenceObject(AccessToken);

  if (NT_SUCCESS(Status))
    {
      _SEH_TRY
      {
        *TokenHandle = hToken;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
    }

  return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
SeQueryAuthenticationIdToken(IN PACCESS_TOKEN Token,
			     OUT PLUID LogonId)
{
  PAGED_CODE();
  
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
  PAGED_CODE();
  
  return ((PTOKEN)Token)->ImpersonationLevel;
}


/*
 * @implemented
 */
TOKEN_TYPE STDCALL
SeTokenType(IN PACCESS_TOKEN Token)
{
  PAGED_CODE();
  
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
  HANDLE hToken;
  PTOKEN Token, NewToken, PrimaryToken;
  BOOLEAN CopyOnOpen, EffectiveOnly;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  SE_IMPERSONATION_STATE ImpersonationState;
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_DESCRIPTOR SecurityDescriptor;
  PACL Dacl = NULL;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(TokenHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  /*
   * At first open the thread token for information access and verify
   * that the token associated with thread is valid.
   */

  Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_QUERY_INFORMATION,
                                     PsThreadType, PreviousMode, (PVOID*)&Thread,
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
                                         PsThreadType, PreviousMode,
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
                              &hToken);

      ObfDereferenceObject(NewToken);
    }
  else
    {
      Status = ObOpenObjectByPointer(Token, HandleAttributes,
                                     NULL, DesiredAccess, SepTokenObjectType,
                                     PreviousMode, &hToken);
    }

  ObfDereferenceObject(Token);

  if (OpenAsSelf)
    {
      PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }

  if(NT_SUCCESS(Status))
  {
    _SEH_TRY
    {
      *TokenHandle = hToken;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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
