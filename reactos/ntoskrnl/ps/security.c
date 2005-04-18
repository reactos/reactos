/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/security.c
 * PURPOSE:         Process Manager Security (Tokens, Impersionation)
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS 
STDCALL 
NtOpenProcessToken(IN HANDLE ProcessHandle,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PHANDLE TokenHandle)
{
    return NtOpenProcessTokenEx(ProcessHandle,
                                DesiredAccess,
                                0,
                                TokenHandle);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenProcessTokenEx(IN HANDLE ProcessHandle,
                     IN ACCESS_MASK DesiredAccess,
                     IN ULONG HandleAttributes,
                     OUT PHANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   HANDLE hToken;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PAGED_CODE();
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode == UserMode)
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

   Status = PsOpenTokenOfProcess(ProcessHandle,
				 &Token);
   if(NT_SUCCESS(Status))
   {
     Status = ObCreateHandle(PsGetCurrentProcess(),
			     Token,
			     DesiredAccess,
			     FALSE,
			     &hToken);
     ObDereferenceObject(Token);

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
   }
   
   return Status;
}

/*
 * @implemented
 */
PACCESS_TOKEN 
STDCALL
PsReferencePrimaryToken(PEPROCESS Process)
{
    /* Reference and return the Token */
    ObReferenceObjectByPointer(Process->Token,
                               TOKEN_ALL_ACCESS,
                               SepTokenObjectType,
                               KernelMode);
    return(Process->Token);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsOpenTokenOfProcess(HANDLE ProcessHandle,
                     PACCESS_TOKEN* Token)
{
    PEPROCESS Process;
    NTSTATUS Status;
   
    /* Get the Token */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       PsProcessType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Process,
                                       NULL);
    
    /* Reference it */
    if(NT_SUCCESS(Status)) {
        
        *Token = PsReferencePrimaryToken(Process);
        ObDereferenceObject(Process);
    }
   
    /* Return */
    return Status;
}

NTSTATUS
STDCALL
PspInitializeProcessSecurity(PEPROCESS Process,
                             PEPROCESS Parent OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
            
    /* If we have a parent, then duplicate the Token */
    if (Parent) {
    
        PTOKEN pNewToken;
        PTOKEN pParentToken;
        OBJECT_ATTRIBUTES ObjectAttributes;

        pParentToken = (PACCESS_TOKEN)Parent->Token;

        /* Initialize the Object Attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
        
        /* Duplicate the Token */
        Status = SepDuplicateToken(pParentToken,
                                   &ObjectAttributes,
                                   FALSE,
                                   TokenPrimary,
                                   pParentToken->ImpersonationLevel,
                                   KernelMode,
                                   &pNewToken);
        
        if(!NT_SUCCESS(Status)) {
        
            DPRINT1("Failed to Duplicate Token\n");
            return Status;
        }
     
        Process->Token = pNewToken;    
    
    } else {
        
#ifdef SCHED_REWRITE
        PTOKEN BootToken;
        
        /* No parent, this is the Initial System Process. Assign Boot Token */
        BootToken = SepCreateSystemProcessToken();
        BootToken->TokenInUse = TRUE;
        Process->Token = BootToken;
        ObReferenceObject(BootToken);
#else
        DPRINT1("PspInitializeProcessSecurity called with no parent.\n");
#endif
    }
    
    /* Return to caller */
    return Status;
}


NTSTATUS
STDCALL
PspAssignPrimaryToken(PEPROCESS Process,
                      HANDLE TokenHandle)
{
    PACCESS_TOKEN Token;
    PACCESS_TOKEN OldToken;
    NTSTATUS Status;
   
    /* Reference the Token */
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       0,
                                       SepTokenObjectType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status)) {

        return(Status);
    }
    
    /* Exchange them */
    Status = SeExchangePrimaryToken(Process, Token, &OldToken);
        
    /* Derefernece Tokens and Return */
    if (NT_SUCCESS(Status)) ObDereferenceObject(OldToken);
    ObDereferenceObject(Token);
    return(Status);
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
PsAssignImpersonationToken(PETHREAD Thread,
                           HANDLE TokenHandle)
{
    PACCESS_TOKEN Token;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    NTSTATUS Status;

    if (TokenHandle != NULL) {

        Status = ObReferenceObjectByHandle(TokenHandle,
                                           TOKEN_IMPERSONATE,
                                           SepTokenObjectType,
                                           KeGetPreviousMode(),
                                           (PVOID*)&Token,
                                           NULL);
        
        if (!NT_SUCCESS(Status)) {
            
            return(Status);
        }
        
        ImpersonationLevel = SeTokenImpersonationLevel(Token);
    
    } else {
        
        Token = NULL;
        ImpersonationLevel = 0;
    }

    PsImpersonateClient(Thread,
                        Token,
                        FALSE,
                        FALSE,
                        ImpersonationLevel);
    
    if (Token != NULL) ObDereferenceObject(Token);
    return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
VOID STDCALL
PsRevertToSelf (VOID)
{
    PsRevertThreadToSelf(PsGetCurrentThread());
}

/*
 * @implemented
 */
VOID
STDCALL
PsRevertThreadToSelf(IN PETHREAD Thread)
{
    if (Thread->ActiveImpersonationInfo == TRUE) {
        
        ObDereferenceObject (Thread->ImpersonationInfo->Token);
        Thread->ActiveImpersonationInfo = FALSE;
    }
}

/*
 * @implemented
 */
VOID 
STDCALL
PsImpersonateClient(IN PETHREAD Thread,
                    IN PACCESS_TOKEN Token,
                    IN BOOLEAN CopyOnOpen,
                    IN BOOLEAN EffectiveOnly,
                    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    
    if (Token == NULL) {
      
        if (Thread->ActiveImpersonationInfo == TRUE) {
            
            Thread->ActiveImpersonationInfo = FALSE;
            
            if (Thread->ImpersonationInfo->Token != NULL) {
                
                ObDereferenceObject (Thread->ImpersonationInfo->Token);
            }
        }
        
        return;
    }

    if (Thread->ImpersonationInfo == NULL) {
        
        Thread->ImpersonationInfo = ExAllocatePool(NonPagedPool,
                                                   sizeof(PS_IMPERSONATION_INFORMATION));
    }

    Thread->ImpersonationInfo->ImpersonationLevel = ImpersonationLevel;
    Thread->ImpersonationInfo->CopyOnOpen = CopyOnOpen;
    Thread->ImpersonationInfo->EffectiveOnly = EffectiveOnly;
    Thread->ImpersonationInfo->Token = Token;
    
    ObReferenceObjectByPointer(Token,
                               0,
                               SepTokenObjectType,
                               KernelMode);
    
    Thread->ActiveImpersonationInfo = TRUE;
}


PACCESS_TOKEN
STDCALL
PsReferenceEffectiveToken(PETHREAD Thread,
                          PTOKEN_TYPE TokenType,
                          PBOOLEAN EffectiveOnly,
                          PSECURITY_IMPERSONATION_LEVEL Level)
{
    PEPROCESS Process;
    PACCESS_TOKEN Token;
   
    if (Thread->ActiveImpersonationInfo == FALSE) {
        
        Process = Thread->ThreadsProcess;
        *TokenType = TokenPrimary;
        *EffectiveOnly = FALSE;
        Token = Process->Token;
    
    } else {

        Token = Thread->ImpersonationInfo->Token;
        *TokenType = TokenImpersonation;
        *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
        *Level = Thread->ImpersonationInfo->ImpersonationLevel;
    }
    
    return(Token);
}

NTSTATUS 
STDCALL
NtImpersonateThread(IN HANDLE ThreadHandle,
                    IN HANDLE ThreadToImpersonateHandle,
                    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService)
{
  SECURITY_QUALITY_OF_SERVICE SafeServiceQoS;
  SECURITY_CLIENT_CONTEXT ClientContext;
  PETHREAD Thread;
  PETHREAD ThreadToImpersonate;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForRead(SecurityQualityOfService,
                   sizeof(SECURITY_QUALITY_OF_SERVICE),
                   sizeof(ULONG));
      SafeServiceQoS = *SecurityQualityOfService;
      SecurityQualityOfService = &SafeServiceQoS;
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

  Status = ObReferenceObjectByHandle(ThreadHandle,
				     THREAD_IMPERSONATE,
				     PsThreadType,
				     PreviousMode,
				     (PVOID*)&Thread,
				     NULL);
  if(NT_SUCCESS(Status))
  {
    Status = ObReferenceObjectByHandle(ThreadToImpersonateHandle,
				       THREAD_DIRECT_IMPERSONATION,
				       PsThreadType,
				       PreviousMode,
				       (PVOID*)&ThreadToImpersonate,
				       NULL);
    if(NT_SUCCESS(Status))
    {
      Status = SeCreateClientSecurity(ThreadToImpersonate,
				      SecurityQualityOfService,
				      0,
				     &ClientContext);
      if(NT_SUCCESS(Status))
      {
        SeImpersonateClient(&ClientContext,
		            Thread);
        if(ClientContext.ClientToken != NULL)
        {
          ObDereferenceObject (ClientContext.ClientToken);
        }
      }

      ObDereferenceObject(ThreadToImpersonate);
    }
    ObDereferenceObject(Thread);
  }

  return Status;
}

/*
 * @implemented
 */
PACCESS_TOKEN 
STDCALL
PsReferenceImpersonationToken(IN PETHREAD Thread,
                              OUT PBOOLEAN CopyOnOpen,
                              OUT PBOOLEAN EffectiveOnly,
                              OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    
    if (Thread->ActiveImpersonationInfo == FALSE) {
        
        return NULL;
    }

    *ImpersonationLevel = Thread->ImpersonationInfo->ImpersonationLevel;
    *CopyOnOpen = Thread->ImpersonationInfo->CopyOnOpen;
    *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
    
    ObReferenceObjectByPointer(Thread->ImpersonationInfo->Token,
                               TOKEN_ALL_ACCESS,
                               SepTokenObjectType,
                               KernelMode);

    return Thread->ImpersonationInfo->Token;
}

#ifdef PsDereferencePrimaryToken
#undef PsDereferenceImpersonationToken
#endif
/*
 * @implemented
 */
VOID
STDCALL
PsDereferenceImpersonationToken(IN PACCESS_TOKEN ImpersonationToken)
{
    if (ImpersonationToken) {
        
        ObDereferenceObject(ImpersonationToken);
    }
}

#ifdef PsDereferencePrimaryToken
#undef PsDereferencePrimaryToken
#endif
/*
 * @implemented
 */
VOID
STDCALL
PsDereferencePrimaryToken(IN PACCESS_TOKEN PrimaryToken)
{
    ObDereferenceObject(PrimaryToken);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsDisableImpersonation(IN PETHREAD Thread,
                       IN PSE_IMPERSONATION_STATE ImpersonationState)
{
    if (Thread->ActiveImpersonationInfo == FALSE) {
        ImpersonationState->Token = NULL;
        ImpersonationState->CopyOnOpen = FALSE;
        ImpersonationState->EffectiveOnly = FALSE;
        ImpersonationState->Level = 0;
        return TRUE;
    }

/* FIXME */
/*   ExfAcquirePushLockExclusive(&Thread->ThreadLock); */

    Thread->ActiveImpersonationInfo = FALSE;
    ImpersonationState->Token = Thread->ImpersonationInfo->Token;
    ImpersonationState->CopyOnOpen = Thread->ImpersonationInfo->CopyOnOpen;
    ImpersonationState->EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
    ImpersonationState->Level = Thread->ImpersonationInfo->ImpersonationLevel;

/* FIXME */
/*   ExfReleasePushLock(&Thread->ThreadLock); */

    return TRUE;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsRestoreImpersonation(IN PETHREAD Thread,
                       IN PSE_IMPERSONATION_STATE ImpersonationState)
{
    
    PsImpersonateClient(Thread, 
                        ImpersonationState->Token,
                        ImpersonationState->CopyOnOpen,
                        ImpersonationState->EffectiveOnly,
                        ImpersonationState->Level);
    
    ObfDereferenceObject(ImpersonationState->Token);
}

/* EOF */
