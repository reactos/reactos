typedef struct _EPORT
{
   KSPIN_LOCK Lock;
   KEVENT Event;
   
   struct _EPORT* OtherPort;
   
   ULONG QueueLength;
   LIST_ENTRY QueueListHead;
   
   ULONG ConnectQueueLength;
   LIST_ENTRY ConnectQueueListHead;
   
   ULONG MaxDataLength;
   ULONG MaxConnectInfoLength;
} EPORT, *PEPORT;

NTSTATUS STDCALL LpcRequestPort(PEPORT Port,
				PLPCMESSAGE LpcMessage);

#define PORT_ALL_ACCESS               (0x1)

extern POBJECT_TYPE ExPortType;
