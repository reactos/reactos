#ifndef __INCLUDE_INTERNAL_PORT_H
#define __INCLUDE_INTERNAL_PORT_H

#include <napi/lpc.h>

typedef struct _EPORT
{
   KSPIN_LOCK Lock;
   KEVENT Event;
   
   ULONG State;
   
   struct _EPORT* OtherPort;
   
   ULONG QueueLength;
   LIST_ENTRY QueueListHead;
   
   ULONG ConnectQueueLength;
   LIST_ENTRY ConnectQueueListHead;
   
   ULONG MaxDataLength;
   ULONG MaxConnectInfoLength;
} EPORT, *PEPORT;

typedef struct _EPORT_TERMINATION_REQUEST
{
   LIST_ENTRY ThreadListEntry;
   PEPORT Port;
} EPORT_TERMINATION_REQUEST, *PEPORT_TERMINATION_REQUEST;

NTSTATUS STDCALL LpcRequestPort(PEPORT Port,
				PLPC_MESSAGE LpcMessage);
NTSTATUS STDCALL LpcSendTerminationPort(PEPORT Port,
					TIME CreationTime);

#define PORT_ALL_ACCESS               (0x1)

extern POBJECT_TYPE ExPortType;

#endif /* __INCLUDE_INTERNAL_PORT_H */
