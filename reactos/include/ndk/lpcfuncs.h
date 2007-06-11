/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    lpcfuncs.h

Abstract:

    Function definitions for the Executive.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _LPCFUNCS_H
#define _LPCFUNCS_H

//
// Dependencies
//
#include <umtypes.h>

//
// LPC Exports
//
#ifndef NTOS_MODE_USER
NTKERNELAPI
NTSTATUS
NTAPI
LpcRequestWaitReplyPort(
    IN PVOID Port,
    IN PPORT_MESSAGE LpcMessageRequest,
    OUT PPORT_MESSAGE LpcMessageReply
);

NTSTATUS
NTAPI
LpcRequestPort(
    IN PVOID Port,
    IN PPORT_MESSAGE LpcMessage
);
#endif

//
// Native calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcceptConnectPort(
    PHANDLE PortHandle,
    PVOID PortContext OPTIONAL,
    PPORT_MESSAGE ConnectionRequest,
    BOOLEAN AcceptConnection,
    PPORT_VIEW ServerView OPTIONAL,
    PREMOTE_PORT_VIEW ClientView OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompleteConnectPort(
    HANDLE PortHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtListenPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ConnectionRequest
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationPort(
    HANDLE PortHandle,
    PORT_INFORMATION_CLASS PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
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
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcReply
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE ReplyMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE LpcReply,
    IN PPORT_MESSAGE LpcRequest
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSecureConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PSID Sid OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
    PHANDLE PortHandle,
    PVOID PortContext OPTIONAL,
    PPORT_MESSAGE ConnectionRequest,
    BOOLEAN AcceptConnection,
    PPORT_VIEW ServerView OPTIONAL,
    PREMOTE_PORT_VIEW ClientView OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
    HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwListenPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ConnectionRequest
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
    HANDLE PortHandle,
    PORT_INFORMATION_CLASS PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcReply
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePortEx(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE ReplyMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE LpcReply,
    IN PPORT_MESSAGE LpcRequest
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSecureConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PSID Sid OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

#endif
