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

#ifndef __PACKET32
#define __PACKET32

#include <winsock2.h>
#include "devioctl.h"

// Working modes
#define PACKET_MODE_CAPT 0x0 ///< Capture mode
#define PACKET_MODE_STAT 0x1 ///< Statistical mode
#define PACKET_MODE_DUMP 0x10 ///< Dump mode
#define PACKET_MODE_STAT_DUMP MODE_DUMP | MODE_STAT ///< Statistical dump Mode

// ioctls
#define FILE_DEVICE_PROTOCOL        0x8000

#define IOCTL_PROTOCOL_STATISTICS   CTL_CODE(FILE_DEVICE_PROTOCOL, 2 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_RESET        CTL_CODE(FILE_DEVICE_PROTOCOL, 3 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_READ         CTL_CODE(FILE_DEVICE_PROTOCOL, 4 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_WRITE        CTL_CODE(FILE_DEVICE_PROTOCOL, 5 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_MACNAME      CTL_CODE(FILE_DEVICE_PROTOCOL, 6 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OPEN                  CTL_CODE(FILE_DEVICE_PROTOCOL, 7 , METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLOSE                 CTL_CODE(FILE_DEVICE_PROTOCOL, 8 , METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	 pBIOCSETBUFFERSIZE 9592
#define	 pBIOCSETF 9030
#define  pBIOCGSTATS 9031
#define	 pBIOCSRTIMEOUT 7416
#define	 pBIOCSMODE 7412
#define	 pBIOCSWRITEREP 7413
#define	 pBIOCSMINTOCOPY 7414
#define	 pBIOCSETOID 2147483648
#define	 pBIOCQUERYOID 2147483652
#define	 pATTACHPROCESS 7117
#define	 pDETACHPROCESS 7118
#define  pBIOCSETDUMPFILENAME 9029
#define  pBIOCEVNAME 7415

#define  pBIOCSTIMEZONE 7471

// Alignment macros.  Packet_WORDALIGN rounds up to the next 
// even multiple of Packet_ALIGNMENT. 
#define Packet_ALIGNMENT sizeof(int)
#define Packet_WORDALIGN(x) (((x)+(Packet_ALIGNMENT-1))&~(Packet_ALIGNMENT-1))

typedef struct NetType
{
	UINT LinkType;	
	UINT LinkSpeed;
}NetType;


//some definitions stolen from libpcap

#ifndef BPF_MAJOR_VERSION

struct bpf_program {
	UINT bf_len;				
	struct bpf_insn *bf_insns;	
};

struct bpf_insn {
	USHORT	code;		
	UCHAR 	jt;			
	UCHAR 	jf;			
	int k;				
};

struct bpf_stat {
	UINT bs_recv;		
						
						
	UINT bs_drop;		
						
						
};

struct bpf_hdr {
	struct timeval	bh_tstamp;	
	UINT	bh_caplen;			
	UINT	bh_datalen;			
	USHORT		bh_hdrlen;										
};

#endif

#define        DOSNAMEPREFIX   TEXT("Packet_")
#define        MAX_LINK_NAME_LENGTH   64
#define        NMAX_PACKET 65535  

typedef struct _ADAPTER  { 
	HANDLE hFile;				
	TCHAR  SymbolicLink[MAX_LINK_NAME_LENGTH]; 
	int NumWrites;				
	HANDLE ReadEvent;			
	UINT ReadTimeOut;			
}  ADAPTER, *LPADAPTER;

typedef struct _PACKET {  
	HANDLE       hEvent;		
	OVERLAPPED   OverLapped;	
	PVOID        Buffer;										
	UINT         Length;		
	UINT         ulBytesReceived;										
	BOOLEAN      bIoComplete;	
}  PACKET, *LPPACKET;

struct _PACKET_OID_DATA {
    ULONG Oid;					
    ULONG Length;				
    UCHAR Data[1];				
								
}; 
typedef struct _PACKET_OID_DATA PACKET_OID_DATA, *PPACKET_OID_DATA;

typedef struct npf_if_addr {
	struct sockaddr IPAddress;	
	struct sockaddr SubnetMask;	
	struct sockaddr Broadcast;	
}npf_if_addr;

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// FUNCTIONS
//---------------------------------------------------------------------------

PCHAR PacketGetVersion();
BOOLEAN PacketSetMinToCopy(LPADAPTER AdapterObject,int nbytes);
BOOLEAN PacketSetNumWrites(LPADAPTER AdapterObject,int nwrites);
BOOLEAN PacketSetMode(LPADAPTER AdapterObject,int mode);
BOOLEAN PacketSetReadTimeout(LPADAPTER AdapterObject,int timeout);
BOOLEAN PacketSetBpf(LPADAPTER AdapterObject,struct bpf_program *fp);
BOOLEAN PacketGetStats(LPADAPTER AdapterObject,struct bpf_stat *s);
BOOLEAN PacketSetBuff(LPADAPTER AdapterObject,int dim);
BOOLEAN PacketGetNetType (LPADAPTER AdapterObject,NetType *type);
LPADAPTER PacketOpenAdapter(LPTSTR AdapterName);
BOOLEAN PacketSendPacket(LPADAPTER AdapterObject,LPPACKET pPacket,BOOLEAN Sync);
LPPACKET PacketAllocatePacket(void);
VOID PacketInitPacket(LPPACKET lpPacket,PVOID  Buffer,UINT  Length);
VOID PacketFreePacket(LPPACKET lpPacket);
BOOLEAN PacketReceivePacket(LPADAPTER AdapterObject,LPPACKET lpPacket,BOOLEAN Sync);
BOOLEAN PacketSetHwFilter(LPADAPTER AdapterObject,ULONG Filter);
BOOLEAN PacketGetAdapterNames(PTSTR pStr,PULONG  BufferSize);
BOOLEAN PacketGetNetInfo(LPTSTR AdapterName, PULONG netp, PULONG maskp);
BOOLEAN PacketGetNetInfoEx(LPTSTR AdapterName, npf_if_addr* buffer, PLONG NEntries);
BOOLEAN PacketRequest(LPADAPTER  AdapterObject,BOOLEAN Set,PPACKET_OID_DATA  OidData);
HANDLE PacketGetReadEvent(LPADAPTER AdapterObject);
BOOLEAN PacketSetDumpName(LPADAPTER AdapterObject, void *name, int len);
BOOL PacketStopDriver();
VOID PacketCloseAdapter(LPADAPTER lpAdapter);

#ifdef __cplusplus
}
#endif 

#endif //__PACKET32
