/* $Id: lsa.c,v 1.7 2003/07/10 19:44:20 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/secur32/lsa.c
 * PURPOSE:         Client-side LSA functions
 * UPDATE HISTORY:
 *                  Created 05/08/00
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <napi/lpc.h>
#include <lsass/lsass.h>
#include <string.h>

/* GLOBALS *******************************************************************/

extern HANDLE Secur32Heap;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
LsaDeregisterLogonProcess(HANDLE LsaHandle)
{
   LSASS_REQUEST Request;
   LSASS_REPLY Reply;
   NTSTATUS Status;
      
   Request.Header.DataSize = 0;
   Request.Header.MessageSize = sizeof(LSASS_REQUEST);
   Request.Type = LSASS_REQUEST_DEREGISTER_LOGON_PROCESS;
   Status = NtRequestWaitReplyPort(LsaHandle,
				   &Request.Header,
				   &Reply.Header);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (!NT_SUCCESS(Reply.Status))
     {
	return(Reply.Status);
     }
   
   return(Status);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
LsaConnectUntrusted(PHANDLE LsaHandle)
{
  return(STATUS_UNSUCCESSFUL);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
LsaCallAuthenticationPackage(HANDLE LsaHandle,
			     ULONG AuthenticationPackage,
			     PVOID ProtocolSubmitBuffer,
			     ULONG SubmitBufferLength,
			     PVOID* ProtocolReturnBuffer,
			     PULONG ReturnBufferLength,
			     PNTSTATUS ProtocolStatus)
{
   PLSASS_REQUEST Request;
   PLSASS_REPLY Reply;
   UCHAR RawRequest[MAX_MESSAGE_DATA];
   UCHAR RawReply[MAX_MESSAGE_DATA];
   NTSTATUS Status;
   ULONG OutBufferSize;

   Request = (PLSASS_REQUEST)RawRequest;
   Reply = (PLSASS_REPLY)RawReply;
   
   Request->Header.DataSize = sizeof(LSASS_REQUEST) + SubmitBufferLength -
     sizeof(LPC_MESSAGE);
   Request->Header.MessageSize = 
     Request->Header.DataSize + sizeof(LPC_MESSAGE);
   Request->Type = LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE;
   Request->d.CallAuthenticationPackageRequest.AuthenticationPackage =
     AuthenticationPackage;
   Request->d.CallAuthenticationPackageRequest.InBufferLength =
     SubmitBufferLength;
   memcpy(Request->d.CallAuthenticationPackageRequest.InBuffer,
	  ProtocolSubmitBuffer,
	  SubmitBufferLength);
   
   Status = NtRequestWaitReplyPort(LsaHandle,
				   &Request->Header,
				   &Reply->Header);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (!NT_SUCCESS(Reply->Status))
     {
	return(Reply->Status);
     }
   
   OutBufferSize = Reply->d.CallAuthenticationPackageReply.OutBufferLength;
   *ProtocolReturnBuffer = RtlAllocateHeap(Secur32Heap,
					   0,
					   OutBufferSize);
   *ReturnBufferLength = OutBufferSize;
   memcpy(*ProtocolReturnBuffer,
	  Reply->d.CallAuthenticationPackageReply.OutBuffer,
	  *ReturnBufferLength);
   
   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LsaFreeReturnBuffer(PVOID Buffer)
{
   return(RtlFreeHeap(Secur32Heap, 0, Buffer));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LsaLookupAuthenticationPackage(HANDLE LsaHandle,
			       PLSA_STRING PackageName,
			       PULONG AuthenticationPackage)
{
   NTSTATUS Status;
   PLSASS_REQUEST Request;
   UCHAR RawRequest[MAX_MESSAGE_DATA];
   LSASS_REPLY Reply;
   
   Request = (PLSASS_REQUEST)RawRequest;
   Request->Header.DataSize = sizeof(LSASS_REQUEST) + PackageName->Length -
     sizeof(LPC_MESSAGE);
   Request->Header.MessageSize = Request->Header.DataSize +
     sizeof(LPC_MESSAGE);
   Request->Type = LSASS_REQUEST_LOOKUP_AUTHENTICATION_PACKAGE;
   
   Status = NtRequestWaitReplyPort(LsaHandle,
			  &Request->Header,
			  &Reply.Header);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (!NT_SUCCESS(Reply.Status))
     {
	return(Reply.Status);
     }
   
   *AuthenticationPackage = Reply.d.LookupAuthenticationPackageReply.Package;
   
   return(Reply.Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LsaLogonUser(HANDLE LsaHandle,
	     PLSA_STRING OriginName,
	     SECURITY_LOGON_TYPE LogonType,
	     ULONG AuthenticationPackage,
	     PVOID AuthenticationInformation,
	     ULONG AuthenticationInformationLength,
	     PTOKEN_GROUPS LocalGroups,
	     PTOKEN_SOURCE SourceContext,
	     PVOID* ProfileBuffer,
	     PULONG ProfileBufferLength,
	     PLUID LogonId,
	     PHANDLE Token,
	     PQUOTA_LIMITS Quotas,
	     PNTSTATUS SubStatus)
{
   ULONG RequestLength;
   ULONG CurrentLength;
   PLSASS_REQUEST Request;
   UCHAR RawMessage[MAX_MESSAGE_DATA];
   PLSASS_REPLY Reply;
   UCHAR RawReply[MAX_MESSAGE_DATA];
   NTSTATUS Status;
   
   RequestLength = sizeof(LSASS_REQUEST) - sizeof(LPC_MESSAGE);
   RequestLength = RequestLength + (OriginName->Length * sizeof(WCHAR));
   RequestLength = RequestLength + AuthenticationInformationLength;
   RequestLength = RequestLength + 
     (LocalGroups->GroupCount * sizeof(SID_AND_ATTRIBUTES));
   
   CurrentLength = 0;
   Request = (PLSASS_REQUEST)RawMessage;
   
   Request->d.LogonUserRequest.OriginNameLength = OriginName->Length;
   Request->d.LogonUserRequest.OriginName = (PWSTR)&RawMessage[CurrentLength];
   memcpy((PWSTR)&RawMessage[CurrentLength],
	  OriginName->Buffer,
	  OriginName->Length * sizeof(WCHAR));
   CurrentLength = CurrentLength + (OriginName->Length * sizeof(WCHAR));
   
   Request->d.LogonUserRequest.LogonType = LogonType;
   
   Request->d.LogonUserRequest.AuthenticationPackage =
     AuthenticationPackage;
   
   Request->d.LogonUserRequest.AuthenticationInformation = 
     (PVOID)&RawMessage[CurrentLength];
   Request->d.LogonUserRequest.AuthenticationInformationLength =
     AuthenticationInformationLength;
   memcpy((PVOID)&RawMessage[CurrentLength],
	  AuthenticationInformation,
	  AuthenticationInformationLength);
   CurrentLength = CurrentLength + AuthenticationInformationLength;
   
   Request->d.LogonUserRequest.LocalGroupsCount = LocalGroups->GroupCount;
   Request->d.LogonUserRequest.LocalGroups = 
     (PSID_AND_ATTRIBUTES)&RawMessage[CurrentLength];
   memcpy((PSID_AND_ATTRIBUTES)&RawMessage[CurrentLength],
	  LocalGroups->Groups,
	  LocalGroups->GroupCount * sizeof(SID_AND_ATTRIBUTES));
   
   Request->d.LogonUserRequest.SourceContext = *SourceContext;
   
   Request->Type = LSASS_REQUEST_LOGON_USER;
   Request->Header.DataSize = RequestLength - sizeof(LPC_MESSAGE);
   Request->Header.MessageSize = RequestLength + sizeof(LPC_MESSAGE);
   
   Reply = (PLSASS_REPLY)RawReply;
   
   Status = NtRequestWaitReplyPort(LsaHandle,
				   &Request->Header,
				   &Reply->Header);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   *SubStatus = Reply->d.LogonUserReply.SubStatus;
   
   if (!NT_SUCCESS(Reply->Status))
     {
	return(Status);
     }
   
   *ProfileBuffer = RtlAllocateHeap(Secur32Heap,
				    0,
				  Reply->d.LogonUserReply.ProfileBufferLength);
   memcpy(*ProfileBuffer,
	  (PVOID)((ULONG)Reply->d.LogonUserReply.Data +
		  (ULONG)Reply->d.LogonUserReply.ProfileBuffer),
	  Reply->d.LogonUserReply.ProfileBufferLength);
   *LogonId = Reply->d.LogonUserReply.LogonId;
   *Token = Reply->d.LogonUserReply.Token;
   memcpy(Quotas, 
	  &Reply->d.LogonUserReply.Quotas,
	  sizeof(Reply->d.LogonUserReply.Quotas));
   
   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LsaRegisterLogonProcess(PLSA_STRING LsaLogonProcessName,
			PHANDLE Handle,
			PLSA_OPERATIONAL_MODE OperationalMode)
{
   UNICODE_STRING Portname = UNICODE_STRING_INITIALIZER(L"\\SeLsaCommandPort");
   ULONG ConnectInfoLength;
   NTSTATUS Status;
   LSASS_REQUEST Request;
   LSASS_REPLY Reply;

   ConnectInfoLength = 0;
   Status = NtConnectPort(Handle,
			  &Portname,
			  NULL,
			  NULL,
			  NULL,
			  NULL,
			  NULL,
			  &ConnectInfoLength);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Request.Type = LSASS_REQUEST_REGISTER_LOGON_PROCESS;
   Request.Header.DataSize = sizeof(LSASS_REQUEST) - 
     sizeof(LPC_MESSAGE);
   Request.Header.MessageSize = sizeof(LSASS_REQUEST);
   
   Request.d.RegisterLogonProcessRequest.Length = LsaLogonProcessName->Length;
   wcscpy(Request.d.RegisterLogonProcessRequest.LogonProcessNameBuffer,
	  LsaLogonProcessName->Buffer);
   
   Status = NtRequestWaitReplyPort(*Handle,
				   &Request.Header,
				   &Reply.Header);
   if (!NT_SUCCESS(Status))
     {
	NtClose(*Handle);
	*Handle = INVALID_HANDLE_VALUE;
	return(Status);
     }
   
   if (!NT_SUCCESS(Reply.Status))
     {
	NtClose(*Handle);
	*Handle = INVALID_HANDLE_VALUE;
	return(Status);
     }
   
   *OperationalMode = Reply.d.RegisterLogonProcessReply.OperationalMode;
   
   return(Reply.Status);
}

