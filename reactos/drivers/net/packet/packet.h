/*
 * Copyright (c) 1999, 2000
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#define UNICODE_NULL ((WCHAR)0) // winnt

#define  MAX_REQUESTS   32

#define MAX_PACKET_LENGTH 1514

struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};

/*
 * Alignment macros.  Packet_WORDALIGN rounds up to the next 
 * even multiple of Packet_ALIGNMENT. 
 */
#define Packet_ALIGNMENT sizeof(int)
#define Packet_WORDALIGN(x) (((x)+(Packet_ALIGNMENT-1))&~(Packet_ALIGNMENT-1))


//IOCTLs
#define	 BIOCSETBUFFERSIZE 9592
#define	 BIOCSETF 9030
#define  BIOCGSTATS 9031
#define	 BIOCSRTIMEOUT 7416
#define	 BIOCSMODE 7412
#define	 BIOCSWRITEREP 7413
#define	 BIOCSMINTOCOPY 7414
#define	 BIOCSETOID 2147483648
#define	 BIOCQUERYOID 2147483652
#define  BIOCGEVNAME 7415

//working modes
#define MODE_CAPT 0
#define MODE_STAT 1

//immediate timeout
#define IMMEDIATE 1

struct bpf_insn {
	USHORT	code;
	UCHAR 	jt;
	UCHAR 	jf;
	int k;
};

struct bpf_hdr {
	struct timeval		bh_tstamp;	/* time stamp */
	UINT				bh_caplen;	/* length of captured portion */
	UINT				bh_datalen;	/* original length of packet */
	USHORT				bh_hdrlen;	/* length of bpf header (this struct plus alignment padding) */
};


typedef struct _INTERNAL_REQUEST {
    LIST_ENTRY     ListElement;
    PIRP           Irp;
    NDIS_REQUEST   Request;

    } INTERNAL_REQUEST, *PINTERNAL_REQUEST;



//
// Port device extension.
//
typedef struct _DEVICE_EXTENSION {

    PDEVICE_OBJECT DeviceObject;

    NDIS_HANDLE    NdisProtocolHandle;

    NDIS_STRING    AdapterName;

    PWSTR          BindString;
    PWSTR          ExportString;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Open Instance
//
typedef struct _OPEN_INSTANCE
    {
        PDEVICE_EXTENSION   DeviceExtension;
        NDIS_HANDLE         AdapterHandle;
        NDIS_HANDLE         PacketPool;
		KSPIN_LOCK		    RcvQSpinLock;
        LIST_ENTRY          RcvList;
        PIRP                OpenCloseIrp;
		KSPIN_LOCK			RequestSpinLock;
        LIST_ENTRY          RequestList;
        LIST_ENTRY          ResetIrpList;
        INTERNAL_REQUEST    Requests[MAX_REQUESTS];
		PUCHAR				Buffer;
		PMDL				BufferMdl;
		PKEVENT				ReadEvent;
		HANDLE				ReadEventHandle;
		UNICODE_STRING		ReadEventName;
		int					Dropped;			
		int					Received;
		PUCHAR				bpfprogram;
		LARGE_INTEGER		StartTime;  
		UINT				Bhead;
		UINT				Btail;
		UINT				BufSize;
		UINT				BLastByte;
	    PMDL                TransferMdl;
		NDIS_SPIN_LOCK		BufLock;
		UINT				MinToCopy;
		LARGE_INTEGER		TimeOut;
		int					mode;
		LARGE_INTEGER		Nbytes;
		LARGE_INTEGER		Npackets;
		NDIS_SPIN_LOCK		CountersLock;
		UINT				Nwrites;
		UINT				Multiple_Write_Counter;
		NDIS_EVENT			WriteEvent;    
		NDIS_EVENT			IOEvent;
		NDIS_STATUS			IOStatus;
		BOOLEAN				Bound;
    } 
OPEN_INSTANCE, *POPEN_INSTANCE;


typedef struct _PACKET_RESERVED {
    LIST_ENTRY     ListElement;
    PIRP           Irp;
    PMDL           pMdl;
    }  PACKET_RESERVED, *PPACKET_RESERVED;


#define  ETHERNET_HEADER_LENGTH   14

#define RESERVED(_p) ((PPACKET_RESERVED)((_p)->ProtocolReserved))

#define  TRANSMIT_PACKETS    128

//
// Prototypes
//

PKEY_VALUE_PARTIAL_INFORMATION getTcpBindings(
	VOID
	);

PWCHAR getAdaptersList(
	VOID
	);

BOOLEAN createDevice(
	IN OUT PDRIVER_OBJECT adriverObjectP,
	IN PUNICODE_STRING amacNameP,
	NDIS_HANDLE aProtoHandle);


VOID
PacketCancelRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
	);


VOID
PacketOpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus
    );

VOID
PacketCloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );


NDIS_STATUS
Packet_tap(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    );


VOID
PacketReceiveComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    );


VOID
PacketRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST pRequest,
    IN NDIS_STATUS   Status
    );

VOID
PacketSendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    );


VOID
PacketResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );


VOID
PacketStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    );


VOID
PacketStatusComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    );

VOID
PacketTransferDataComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    );


VOID
PacketRemoveReference(
    IN PDEVICE_EXTENSION DeviceExtension
    );


NTSTATUS
PacketCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP FlushIrp
    );


NTSTATUS
PacketShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
STDCALL
PacketUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
STDCALL
PacketOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
PacketClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
PacketWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
PacketRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
PacketIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
PacketReadRegistry(
    IN  PWSTR              *MacDriverName,
    IN  PWSTR              *PacketDriverName,
    IN  PUNICODE_STRING     RegistryPath
    );

NTSTATUS
PacketCreateSymbolicLink(
    IN  PUNICODE_STRING  DeviceName,
    IN  BOOLEAN          Create
    );
/*
typedef NTSTATUS STDCALL
(*PRTL_QUERY_REGISTRY_ROUTINE)(PWSTR ValueName,
			       ULONG ValueType,
			       PVOID ValueData,
			       ULONG ValueLength,
			       PVOID Context,
			       PVOID EntryContext);
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PacketOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = PacketClose;
    DriverObject->MajorFunction[IRP_MJ_READ]   = PacketRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = PacketWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = PacketIoControl;
    DriverObject->DriverUnload = PacketUnload;
 */
NTSTATUS
STDCALL
PacketQueryRegistryRoutine(
    IN PWSTR     ValueName,
    IN ULONG     ValueType,
    IN PVOID     ValueData,
    IN ULONG     ValueLength,
    IN PVOID     Context,
    IN PVOID     EntryContext
    );

INT
Packet_multiple_tap(
    IN    NDIS_HANDLE         ProtocolBindingContext,
    IN    PNDIS_PACKET        Packet
    );

VOID PacketBindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            DeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    );

VOID
PacketUnbindAdapter(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         UnbindContext
    );


int bpf_validate(struct bpf_insn *f,int len);

UINT bpf_filter(register struct bpf_insn *pc,
				register UCHAR *p,
				UINT wirelen,
				register UINT buflen);

UINT bpf_filter_with_2_buffers(register struct bpf_insn *pc,
							   register UCHAR *p,
							   register UCHAR *pd,
							   register int headersize,
							   UINT wirelen,
							   register UINT buflen);

VOID ReadTimeout(IN PKDPC Dpc,
				 IN PVOID DeferredContext,
				 IN PVOID SystemContext1,
				 IN PVOID SystemContext2);
