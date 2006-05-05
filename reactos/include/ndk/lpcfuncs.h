/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    lpcfuncs.h

Abstract:

    Function definitions for the Executive.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _LPCFUNCS_H
#define _LPCFUNCS_H

//
// Dependencies
//
#include <umtypes.h>

//
// Native calls
//
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

NTSTATUS
NTAPI
NtCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage
);

NTSTATUS
NTAPI
NtCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSTATUS
NTAPI
NtListenPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ConnectionRequest
);

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
NtReadRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcReply
);

NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage
);

NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE ReplyMessage
);

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

NTSTATUS
NTAPI
ZwCompleteConnectPort(
    HANDLE PortHandle
);

NTSYSCALLAPI
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

NTSTATUS
NTAPI
ZwCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage
);

NTSTATUS
NTAPI
ZwCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSTATUS
NTAPI
ZwListenPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ConnectionRequest
);

NTSTATUS
NTAPI
ZwQueryInformationPort(
    HANDLE PortHandle,
    PORT_INFORMATION_CLASS PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
);

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

NTSTATUS
NTAPI
ZwReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcReply
);

NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage
);

NTSTATUS
NTAPI
ZwReplyWaitReceivePortEx(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE ReplyMessage
);

NTSTATUS
NTAPI
ZwRequestPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE LpcMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE LpcReply,
    IN PPORT_MESSAGE LpcRequest
);

NTSYSCALLAPI
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
