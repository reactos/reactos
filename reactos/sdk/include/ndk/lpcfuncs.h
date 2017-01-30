/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    lpcfuncs.h

Abstract:

    Function definitions for the Local Procedure Call.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _LPCFUNCS_H
#define _LPCFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <lpctypes.h>

//
// LPC Exports
//
#ifndef NTOS_MODE_USER
NTKERNELAPI
NTSTATUS
NTAPI
LpcRequestWaitReplyPort(
    _In_ PVOID Port,
    _In_ PPORT_MESSAGE LpcMessageRequest,
    _Out_ PPORT_MESSAGE LpcMessageReply
);

NTSTATUS
NTAPI
LpcRequestPort(
    _In_ PVOID Port,
    _In_ PPORT_MESSAGE LpcMessage
);
#endif

//
// Native calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcceptConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ PVOID PortContext,
    _In_ PPORT_MESSAGE ConnectionRequest,
    _In_ BOOLEAN AcceptConnection,
    _Inout_opt_ PPORT_VIEW ServerView,
    _Out_opt_ PREMOTE_PORT_VIEW ClientView
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompleteConnectPort(
    _In_ HANDLE PortHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectionInfoLength,
    _In_ ULONG MaxMessageLength,
    _In_ ULONG MaxPoolUsage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectInfoLength,
    _In_ ULONG MaxDataLength,
    _In_opt_ ULONG NPMessageQueueSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ClientMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtListenPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ConnectionRequest
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationPort(
    _In_ HANDLE PortHandle,
    _In_ PORT_INFORMATION_CLASS PortInformationClass,
    _Out_bytecap_(PortInformationLength) PVOID PortInformation,
    _In_ ULONG PortInformationLength,
    _Out_ PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryPortInformationProcess(
    VOID
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _Out_bytecap_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcReply
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _In_opt_ PLARGE_INTEGER Timeout
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE ReplyMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE LpcReply,
    _In_ PPORT_MESSAGE LpcRequest
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSecureConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _In_opt_ PSID ServerSid,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _In_bytecount_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ PVOID PortContext,
    _In_ PPORT_MESSAGE ConnectionRequest,
    _In_ BOOLEAN AcceptConnection,
    _In_opt_ PPORT_VIEW ServerView,
    _In_opt_ PREMOTE_PORT_VIEW ClientView
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
    _In_ HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _In_opt_ PPORT_VIEW ClientView,
    _In_opt_ PREMOTE_PORT_VIEW ServerView,
    _In_opt_ PULONG MaxMessageLength,
    _In_opt_ PVOID ConnectionInformation,
    _In_opt_ PULONG ConnectionInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectionInfoLength,
    _In_ ULONG MaxMessageLength,
    _In_ ULONG MaxPoolUsage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectInfoLength,
    _In_ ULONG MaxDataLength,
    _In_opt_ ULONG NPMessageQueueSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ClientMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwListenPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ConnectionRequest
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
    _In_ HANDLE PortHandle,
    _In_ PORT_INFORMATION_CLASS PortInformationClass,
    _Out_bytecap_(PortInformationLength) PVOID PortInformation,
    _In_ ULONG PortInformationLength,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _Out_bytecap_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcReply
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePortEx(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _In_opt_ PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE ReplyMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE LpcReply,
    _In_ PPORT_MESSAGE LpcRequest
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSecureConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _In_opt_ PSID Sid,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _In_bytecount_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
);

#endif
