#ifndef __INCLUDE_NAPI_LPC_H
#define __INCLUDE_NAPI_LPC_H

#include <ntos/security.h>

#define MAX_MESSAGE_DATA   (0x130)

#define UNUSED_MSG_TYPE    (0x0)
#define LPC_REQUEST        (0x1)
#define LPC_REPLY          (0x2)
#define LPC_DATAGRAM       (0x3)
#define LPC_LOST_REPLY     (0x4)
#define LPC_PORT_CLOSED    (0x5)
#define LPC_CLIENT_DIED    (0x6)
#define LPC_EXCEPTION      (0x7)
#define LPC_DEBUG_EVENT    (0x8)
#define LPC_ERROR_EVENT    (0x9)
#define LPC_CONNECTION_REQUEST (0xa)
#define LPC_CONNECTION_REFUSED (0xb)

typedef struct _LPC_SECTION_WRITE
{
   ULONG Length;
   HANDLE SectionHandle;
   ULONG SectionOffset;
   ULONG ViewSize;
   PVOID ViewBase;
   PVOID TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

typedef struct _LPC_SECTION_READ
{
   ULONG Length;
   ULONG ViewSize;
   PVOID ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ;

typedef struct _LPC_MESSAGE_HEADER
{
   USHORT DataSize;
   USHORT MessageSize;
   USHORT MessageType;
   USHORT VirtualRangesOffset;
   CLIENT_ID Cid;
   ULONG MessageId;
   ULONG SharedSectionSize;
} LPC_MESSAGE_HEADER, *PLPC_MESSAGE_HEADER;

typedef struct _LPC_TERMINATION_MESSAGE
{
   LPC_MESSAGE_HEADER Header;
   TIME CreationTime;
} LPC_TERMINATION_MESSAGE, *PLPC_TERMINATION_MESSAGE;

typedef LPC_MESSAGE_HEADER LPC_MESSAGE, *PLPC_MESSAGE;

typedef struct _LPC_MAX_MESSAGE
{
   LPC_MESSAGE_HEADER Header;
   BYTE Data[MAX_MESSAGE_DATA];
} LPC_MAX_MESSAGE, *PLPC_MAX_MESSAGE;

#define PORT_MESSAGE_TYPE(m) (USHORT)((m).Header.MessageType)

NTSTATUS STDCALL NtAcceptConnectPort (PHANDLE	PortHandle,
				      HANDLE NamedPortHandle,
				      PLPC_MESSAGE ServerReply,
				      BOOLEAN AcceptIt,
				      PLPC_SECTION_WRITE WriteMap,
				      PLPC_SECTION_READ ReadMap);

NTSTATUS STDCALL NtCompleteConnectPort (HANDLE PortHandle);

NTSTATUS STDCALL NtConnectPort(PHANDLE PortHandle,
			       PUNICODE_STRING PortName,
			       PSECURITY_QUALITY_OF_SERVICE SecurityQos,
			       PLPC_SECTION_WRITE SectionInfo,
			       PLPC_SECTION_READ MapInfo,
			       PULONG MaxMessageSize,
			       PVOID ConnectInfo,
			       PULONG ConnectInfoLength);

NTSTATUS STDCALL NtReplyWaitReplyPort (HANDLE PortHandle,
				       PLPC_MESSAGE ReplyMessage);
 
NTSTATUS STDCALL NtCreatePort(PHANDLE PortHandle,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      ULONG MaxConnectInfoLength,
			      ULONG MaxDataLength,
			      ULONG NPMessageQueueSize OPTIONAL);

NTSTATUS STDCALL NtCreateWaitablePort(PHANDLE PortHandle,
			              POBJECT_ATTRIBUTES ObjectAttributes,
			              ULONG MaxConnectInfoLength,
			              ULONG MaxDataLength,
			              ULONG NPMessageQueueSize OPTIONAL);

NTSTATUS STDCALL NtImpersonateClientOfPort (HANDLE PortHandle,
					    PLPC_MESSAGE ClientMessage);

NTSTATUS STDCALL NtListenPort (HANDLE PortHandle,
			       PLPC_MESSAGE LpcMessage);

NTSTATUS STDCALL NtQueryInformationPort (HANDLE PortHandle,
					 CINT PortInformationClass,	
					 PVOID PortInformation,	
					 ULONG PortInformationLength,	
					 PULONG ReturnLength);
NTSTATUS STDCALL NtReplyPort (HANDLE PortHandle,
			      PLPC_MESSAGE LpcReply);
NTSTATUS STDCALL NtReplyWaitReceivePort (HANDLE	PortHandle,
					 PULONG PortId,
					 PLPC_MESSAGE MessageReply,
					 PLPC_MESSAGE MessageRequest);
NTSTATUS STDCALL NtRequestPort (HANDLE PortHandle,
			        PLPC_MESSAGE LpcMessage);

NTSTATUS STDCALL NtRequestWaitReplyPort (HANDLE PortHandle,
					 PLPC_MESSAGE LpcReply,      
					 PLPC_MESSAGE LpcRequest);

NTSTATUS STDCALL NtReadRequestData (HANDLE PortHandle,
				    PLPC_MESSAGE Message,
				    ULONG Index,
				    PVOID Buffer,
				    ULONG BufferLength,
				    PULONG ReturnLength);

NTSTATUS STDCALL NtWriteRequestData (HANDLE PortHandle,
				     PLPC_MESSAGE Message,
				     ULONG Index,
				     PVOID Buffer,
				     ULONG BufferLength,
				     PULONG ReturnLength);


#endif /* __INCLUDE_NAPI_LPC_H */
