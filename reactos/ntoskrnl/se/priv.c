/* $Id: priv.c,v 1.1 2000/01/26 10:07:30 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/priv.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

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
		  if (!(Current[i].Attributes & SE_PRIVILEGE_ENABLED) &&
		      Privileges[i].Luid.u.LowPart == 
		      Current[i].Luid.u.LowPart &&
		      Privileges[i].Luid.u.HighPart == 
		      Current[i].Luid.u.HighPart)
		    {
		       Privileges[i].Attributes = 
			 Privileges[i].Attributes | 
			 SE_PRIVILEGE_USED_FOR_ACCESS;
		       j++;
		       break;
		    }
	       }
	     k--;
	  } while (k > 0);
     }
   
   if ((PrivilegeControl & PRIVILEGE_SET_ALL_NECESSARY) && 
       PrivilegeCount == j)       
     {
	return(TRUE);
     }
       
   if (j > 0 && 
       !(PrivilegeControl & PRIVILEGE_SET_ALL_NECESSARY))
     {
	return(TRUE);
     }

   return(FALSE);
}


NTSTATUS SeCaptureLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Src,
					 ULONG PrivilegeCount,
					 KPROCESSOR_MODE PreviousMode,
					 PLUID_AND_ATTRIBUTES AllocatedMem,
					 ULONG AllocatedLength,
					 POOL_TYPE PoolType,
					 ULONG d,
					 PLUID_AND_ATTRIBUTES* Dest,
					 PULONG Length)
{
   PLUID_AND_ATTRIBUTES* NewMem;
   ULONG SrcLength;
   
   if (PrivilegeCount == 0)
     {
	*Dest = 0;
	*Length = 0;
	return(STATUS_SUCCESS);
     }
   if (PreviousMode == 0 && d == 0)
     {
	*Dest = Src;
	return(STATUS_SUCCESS);
     }
   SrcLength = ((PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES)) + 3) & 0xfc;
   *Length = SrcLength;
   if (AllocatedMem == NULL)
     {
	NewMem = ExAllocatePool(PoolType, SrcLength);
	*Dest =  (PLUID_AND_ATTRIBUTES)NewMem;
	if (NewMem == NULL)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
     }
   else
     {
	if (SrcLength > AllocatedLength)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	*Dest = AllocatedMem;
     }
   memmove(*Dest, Src, SrcLength);
   return(STATUS_SUCCESS);
}
					 
VOID SeReleaseLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Privilege,
				     KPROCESSOR_MODE PreviousMode,
				     ULONG a)
{
   ExFreePool(Privilege);
}

NTSTATUS STDCALL NtPrivilegeCheck (IN	HANDLE		ClientToken,
				   IN	PPRIVILEGE_SET	RequiredPrivileges,  
				   IN	PBOOLEAN	Result)
{
   NTSTATUS Status;
   PACCESS_TOKEN Token;
   ULONG PrivilegeCount;
   BOOLEAN TResult;
   ULONG PrivilegeControl;
   PLUID_AND_ATTRIBUTES Privilege;
   ULONG Length;
   
   Status = ObReferenceObjectByHandle(ClientToken,
				      0,
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (Token->TokenType == TokenImpersonation &&
       Token->ImpersonationLevel < SecurityAnonymous)
     {
	ObDereferenceObject(Token);
	return(STATUS_UNSUCCESSFUL);
     }
   PrivilegeCount = RequiredPrivileges->PrivilegeCount;
   PrivilegeControl = RequiredPrivileges->Control;
   Privilege = 0;
   Status = SeCaptureLuidAndAttributesArray(RequiredPrivileges->Privilege,
					    PrivilegeCount,
					    1,
					    0,
					    0,
					    1,
					    1,
					    &Privilege,
					    &Length);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(Token);
	return(STATUS_UNSUCCESSFUL);
     }
   TResult = SepPrivilegeCheck(Token,
			       Privilege,
			       PrivilegeCount,
			       PrivilegeControl,
			       UserMode);
   memmove(RequiredPrivileges->Privilege, Privilege, Length);
   *Result = TResult;
   SeReleaseLuidAndAttributesArray(Privilege, UserMode, 1);
   return(STATUS_SUCCESS);
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
#if 0
	SePrivilegeServiceAuditAlarm(0,
				     &SubjectContext,
				     &PrivilegeValue);
#endif
     }
   SeReleaseSubjectContext(&SubjectContext);
   return(r);
}

