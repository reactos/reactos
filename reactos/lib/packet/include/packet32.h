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

/** @ingroup packetapi
 *  @{ 
 */

/** @defgroup packet32h Packet.dll definitions and data structures
 *  Packet32.h contains the data structures and the definitions used by packet.dll.
 *  The file is used both by the Win9x and the WinNTx versions of packet.dll, and can be included
 *  by the applications that use the functions of this library
 *  @{
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

#define	 pBIOCSETBUFFERSIZE 9592		///< IOCTL code: set kernel buffer size.
#define	 pBIOCSETF 9030					///< IOCTL code: set packet filtering program.
#define  pBIOCGSTATS 9031				///< IOCTL code: get the capture stats.
#define	 pBIOCSRTIMEOUT 7416			///< IOCTL code: set the read timeout.
#define	 pBIOCSMODE 7412				///< IOCTL code: set working mode.
#define	 pBIOCSWRITEREP 7413			///< IOCTL code: set number of physical repetions of every packet written by the app.
#define	 pBIOCSMINTOCOPY 7414			///< IOCTL code: set minimum amount of data in the kernel buffer that unlocks a read call.
#define	 pBIOCSETOID 2147483648			///< IOCTL code: set an OID value.
#define	 pBIOCQUERYOID 2147483652		///< IOCTL code: get an OID value.
#define	 pATTACHPROCESS 7117			///< IOCTL code: attach a process to the driver. Used in Win9x only.
#define	 pDETACHPROCESS 7118			///< IOCTL code: detach a process from the driver. Used in Win9x only.
#define  pBIOCSETDUMPFILENAME 9029		///< IOCTL code: set the name of a the file used by kernel dump mode.
#define  pBIOCEVNAME 7415				///< IOCTL code: get the name of the event that the driver signals when some data is present in the buffer.
#define  pBIOCSENDPACKETSNOSYNC 9032	///< IOCTL code: Send a buffer containing multiple packets to the network, ignoring the timestamps associated with the packets.
#define  pBIOCSENDPACKETSSYNC 9033		///< IOCTL code: Send a buffer containing multiple packets to the network, respecting the timestamps associated with the packets.
#define  pBIOCSETDUMPLIMITS 9034		///< IOCTL code: Set the dump file limits. See the PacketSetDumpLimits() function.
#define  pBIOCISDUMPENDED 7411			///< IOCTL code: Get the status of the kernel dump process. See the PacketIsDumpEnded() function.

#define  pBIOCSTIMEZONE 7471			///< IOCTL code: set time zone. Used in Win9x only.


/// Alignment macro. Defines the alignment size.
#define Packet_ALIGNMENT sizeof(int)
/// Alignment macro. Rounds up to the next even multiple of Packet_ALIGNMENT. 
#define Packet_WORDALIGN(x) (((x)+(Packet_ALIGNMENT-1))&~(Packet_ALIGNMENT-1))

/*!
  \brief Network type structure.

  This structure is used by the PacketGetNetType() function to return information on the current adapter's type and speed.
*/
typedef struct NetType
{
	UINT LinkType;	///< The MAC of the current network adapter (see function PacketGetNetType() for more information)
	UINT LinkSpeed;	///< The speed of the network in bits per second
}NetType;


//some definitions stolen from libpcap

#ifndef BPF_MAJOR_VERSION

/*!
  \brief A BPF pseudo-assembly program.

  The program will be injected in the kernel by the PacketSetBPF() function and applied to every incoming packet. 
*/
struct bpf_program {
	UINT bf_len;				///< Indicates the number of instructions of the program, i.e. the number of struct bpf_insn that will follow.
	struct bpf_insn *bf_insns;	///< A pointer to the first instruction of the program.
};

/*!
  \brief A single BPF pseudo-instruction.

  bpf_insn contains a single instruction for the BPF register-machine. It is used to send a filter program to the driver.
*/
struct bpf_insn {
	USHORT	code;		///< Instruction type and addressing mode.
	UCHAR 	jt;			///< Jump if true
	UCHAR 	jf;			///< Jump if false
	int k;				///< Generic field used for various purposes.
};

/*!
  \brief Structure that contains a couple of statistics values on the current capture.

  It is used by packet.dll to return statistics about a capture session.
*/
struct bpf_stat {
	UINT bs_recv;		///< Number of packets that the driver received from the network adapter 
						///< from the beginning of the current capture. This value includes the packets 
						///< lost by the driver.
	UINT bs_drop;		///< number of packets that the driver lost from the beginning of a capture. 
						///< Basically, a packet is lost when the the buffer of the driver is full. 
						///< In this situation the packet cannot be stored and the driver rejects it.
	UINT ps_ifdrop;		///< drops by interface. XXX not yet supported
	UINT bs_capt;		///< number of packets that pass the filter, find place in the kernel buffer and
						///< thus reach the application.
};
						
/*!
  \brief Packet header.
						
  This structure defines the header associated with every packet delivered to the application.
*/
struct bpf_hdr {
	struct timeval	bh_tstamp;	///< The timestamp associated with the captured packet. 
								///< It is stored in a TimeVal structure.
	UINT	bh_caplen;			///< Length of captured portion. The captured portion <b>can be different</b>
								///< from the original packet, because it is possible (with a proper filter)
								///< to instruct the driver to capture only a portion of the packets.
	UINT	bh_datalen;			///< Original length of packet
	USHORT		bh_hdrlen;		///< Length of bpf header (this struct plus alignment padding). In some cases,
								///< a padding could be added between the end of this structure and the packet
								///< data for performance reasons. This filed can be used to retrieve the actual data 
								///< of the packet.
};
						
/*!
  \brief Dump packet header.
						
  This structure defines the header associated with the packets in a buffer to be used with PacketSendPackets().
  It is simpler than the bpf_hdr, because it corresponds to the header associated by WinPcap and libpcap to a
  packet in a dump file. This makes straightforward sending WinPcap dump files to the network.
*/
struct dump_bpf_hdr{
    struct timeval	ts;			///< Time stamp of the packet
    UINT			caplen;		///< Length of captured portion. The captured portion can smaller than the 
								///< the original packet, because it is possible (with a proper filter) to 
								///< instruct the driver to capture only a portion of the packets. 
    UINT			len;		///< Length of the original packet (off wire).
};


#endif

#define        DOSNAMEPREFIX   TEXT("Packet_")	///< Prefix added to the adapters device names to create the WinPcap devices
#define        MAX_LINK_NAME_LENGTH	64			//< Maximum length of the devices symbolic links
#define        NMAX_PACKET 65535  

/*!
  \brief Describes a network adapter.

  This structure is the most important for the functioning of packet.dll, but the great part of its fields
  should be ignored by the user, since the library offers functions that avoid to cope with low-level parameters
*/
typedef struct _ADAPTER  { 
	HANDLE hFile;				///< \internal Handle to an open instance of the NPF driver.
	CHAR  SymbolicLink[MAX_LINK_NAME_LENGTH]; ///< \internal A string containing the name of the network adapter currently opened.
	int NumWrites;				///< \internal Number of times a packets written on this adapter will be repeated 
								///< on the wire.
	HANDLE ReadEvent;			///< A notification event associated with the read calls on the adapter.
								///< It can be passed to standard Win32 functions (like WaitForSingleObject
								///< or WaitForMultipleObjects) to wait until the driver's buffer contains some 
								///< data. It is particularly useful in GUI applications that need to wait 
								///< concurrently on several events. In Windows NT/2000 the PacketSetMinToCopy()
								///< function can be used to define the minimum amount of data in the kernel buffer
								///< that will cause the event to be signalled. 
	
	UINT ReadTimeOut;			///< \internal The amount of time after which a read on the driver will be released and 
								///< ReadEvent will be signaled, also if no packets were captured
}  ADAPTER, *LPADAPTER;

/*!
  \brief Structure that contains a group of packets coming from the driver.

  This structure defines the header associated with every packet delivered to the application.
*/
typedef struct _PACKET {  
	HANDLE       hEvent;		///< \deprecated Still present for compatibility with old applications.
	OVERLAPPED   OverLapped;	///< \deprecated Still present for compatibility with old applications.
	PVOID        Buffer;		///< Buffer with containing the packets. See the PacketReceivePacket() for
								///< details about the organization of the data in this buffer
	UINT         Length;		///< Length of the buffer
	DWORD        ulBytesReceived;	///< Number of valid bytes present in the buffer, i.e. amount of data
									///< received by the last call to PacketReceivePacket()
	BOOLEAN      bIoComplete;	///< \deprecated Still present for compatibility with old applications.
}  PACKET, *LPPACKET;

/*!
  \brief Structure containing an OID request.
								
  It is used by the PacketRequest() function to send an OID to the interface card driver. 
  It can be used, for example, to retrieve the status of the error counters on the adapter, its MAC address, 
  the list of the multicast groups defined on it, and so on.
*/
struct _PACKET_OID_DATA {
    ULONG Oid;					///< OID code. See the Microsoft DDK documentation or the file ntddndis.h
								///< for a complete list of valid codes.
    ULONG Length;				///< Length of the data field
    UCHAR Data[1];				///< variable-lenght field that contains the information passed to or received 
								///< from the adapter.
}; 
typedef struct _PACKET_OID_DATA PACKET_OID_DATA, *PPACKET_OID_DATA;

/*!
  \brief Addresses of a network adapter.

  This structure is used by the PacketGetNetInfoEx() function to return the IP addresses associated with 
  an adapter.
*/
typedef struct npf_if_addr {
	struct sockaddr IPAddress;	///< IP address.
	struct sockaddr SubnetMask;	///< Netmask for that address.
	struct sockaddr Broadcast;	///< Broadcast address.
}npf_if_addr;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @}
 */

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
BOOLEAN PacketGetStatsEx(LPADAPTER AdapterObject,struct bpf_stat *s);
BOOLEAN PacketSetBuff(LPADAPTER AdapterObject,int dim);
BOOLEAN PacketGetNetType (LPADAPTER AdapterObject,NetType *type);
LPADAPTER PacketOpenAdapter(LPTSTR AdapterName);
BOOLEAN PacketSendPacket(LPADAPTER AdapterObject,LPPACKET pPacket,BOOLEAN Sync);
INT PacketSendPackets(LPADAPTER AdapterObject,PVOID PacketBuff,ULONG Size, BOOLEAN Sync);
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
BOOLEAN PacketSetDumpLimits(LPADAPTER AdapterObject, UINT maxfilesize, UINT maxnpacks);
BOOLEAN PacketIsDumpEnded(LPADAPTER AdapterObject, BOOLEAN sync);
BOOL PacketStopDriver();
VOID PacketCloseAdapter(LPADAPTER lpAdapter);

#ifdef __cplusplus
}
#endif 

#endif //__PACKET32
