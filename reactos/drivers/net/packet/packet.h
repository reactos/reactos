/*
 * Copyright (c) 1999, 2000
 *  Politecnico di Torino.  All rights reserved.
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

/** @ingroup NPF 
 *  @{
 */

/** @defgroup NPF_include NPF structures and definitions 
 *  @{
 */

#ifndef __PACKET_INCLUDE______
#define __PACKET_INCLUDE______

#define NTKERNEL    ///< Forces the compilation of the jitter with kernel calls 

#ifdef __GNUC__
#undef EXIT_SUCCESS
#undef EXIT_FAILURE
#define UNICODE_NULL ((WCHAR)0) // winnt
#include "win_bpf.h"
#include <internal/ps.h>
#endif

#include "jitter.h"
#include "tme.h"

#define  MAX_REQUESTS   32 ///< Maximum number of simultaneous IOCTL requests.

#define Packet_ALIGNMENT sizeof(int) ///< Alignment macro. Defines the alignment size.
#define Packet_WORDALIGN(x) (((x)+(Packet_ALIGNMENT-1))&~(Packet_ALIGNMENT-1))  ///< Alignment macro. Rounds up to the next 
                                                                                ///< even multiple of Packet_ALIGNMENT. 


/***************************/
/*         IOCTLs          */
/***************************/

/*!
  \brief IOCTL code: set kernel buffer size.

  This IOCTL is used to set a new size of the circular buffer associated with an instance of NPF.
  When a BIOCSETBUFFERSIZE command is received, the driver frees the old buffer, allocates the new one 
  and resets all the parameters associated with the buffer in the OPEN_INSTANCE structure. The currently 
  buffered packets are lost.
*/
#define  BIOCSETBUFFERSIZE 9592

/*!
  \brief IOCTL code: set packet filtering program.

  This IOCTL sets a new packet filter in the driver. Before allocating any memory for the new filter, the 
  bpf_validate() function is called to check the correctness of the filter. If this function returns TRUE, 
  the filter is copied to the driver's memory, its address is stored in the bpfprogram field of the 
  OPEN_INSTANCE structure associated with current instance of the driver, and the filter will be applied to 
  every incoming packet. This command also empties the circular buffer used by current instance 
  to store packets. This is done to avoid the presence in the buffer of packets that do not match the filter.
*/
#define  BIOCSETF 9030

/*!
  \brief IOCTL code: get the capture stats

  This command returns to the application the number of packets received and the number of packets dropped by 
  an instance of the driver.
*/
#define  BIOCGSTATS 9031

/*!
  \brief IOCTL code: set the read timeout

  This command sets the maximum timeout after which a read is released, also if no data packets were received.
*/
#define  BIOCSRTIMEOUT 7416

/*!
  \brief IOCTL code: set working mode

  This IOCTL can be used to set the working mode of a NPF instance. The new mode, received by the driver in the
  buffer associated with the IOCTL command, can be #MODE_CAPT for capture mode (the default), #MODE_STAT for
  statistical mode or #MODE_DUMP for dump mode.
*/
#define  BIOCSMODE 7412

/*!
  \brief IOCTL code: set number of physical repetions of every packet written by the app

  Sets the number of times a single write call must be repeated. This command sets the OPEN_INSTANCE::Nwrites 
  member, and is used to implement the 'multiple write' feature of the driver.
*/
#define  BIOCSWRITEREP 7413

/*!
  \brief IOCTL code: set minimum amount of data in the kernel buffer that unlocks a read call

  This command sets the OPEN_INSTANCE::MinToCopy member.
*/
#define  BIOCSMINTOCOPY 7414

/*!
  \brief IOCTL code: set an OID value

  This IOCTL is used to perform an OID set operation on the NIC driver. 
*/
#define  BIOCSETOID 2147483648

/*!
  \brief IOCTL code: get an OID value

  This IOCTL is used to perform an OID get operation on the NIC driver. 
*/
#define  BIOCQUERYOID 2147483652

/*!
  \brief IOCTL code: set the name of a the file used by kernel dump mode

  This command opens a file whose name is contained in the IOCTL buffer and associates it with current NPf instance.
  The dump thread uses it to copy the content of the circular buffer to file.
  If a file was already opened, the driver closes it before opening the new one.
*/
#define  BIOCSETDUMPFILENAME 9029

/*!
  \brief IOCTL code: get the name of the event that the driver signals when some data is present in the buffer

  Command used by the application to retrieve the name of the global event associated with a NPF instance.
  The event is signaled by the driver when the kernel buffer contains enough data for a transfer.
*/
#define  BIOCGEVNAME 7415

/*!
  \brief IOCTL code: Send a buffer containing multiple packets to the network, ignoring the timestamps.

  Command used to send a buffer of packets in a single system call. Every packet in the buffer is preceded by
  a sf_pkthdr structure. The timestamps of the packets are ignored, i.e. the packets are sent as fast as 
  possible. The NPF_BufferedWrite() function is invoked to send the packets.
*/
#define  BIOCSENDPACKETSNOSYNC 9032

/*!
  \brief IOCTL code: Send a buffer containing multiple packets to the network, considering the timestamps.

  Command used to send a buffer of packets in a single system call. Every packet in the buffer is preceded by
  a sf_pkthdr structure. The timestamps of the packets are used to synchronize the write, i.e. the packets 
  are sent to the network respecting the intervals specified in the sf_pkthdr structure assiciated with each
  packet. NPF_BufferedWrite() function is invoked to send the packets. 
*/
#define  BIOCSENDPACKETSSYNC 9033

/*!
  \brief IOCTL code: Set the dump file limits.

  This IOCTL sets the limits (maximum size and maximum number of packets) of the dump file created when the
  driver works in dump mode.
*/
#define  BIOCSETDUMPLIMITS 9034

/*!
  \brief IOCTL code: Get the status of the kernel dump process.

  This command returns TRUE if the kernel dump is ended, i.e if one of the limits set with BIOCSETDUMPLIMITS
  (amount of bytes or number of packets) has been reached.
*/
#define BIOCISDUMPENDED 7411

// Working modes
#define MODE_CAPT 0x0       ///< Capture working mode
#define MODE_STAT 0x1       ///< Statistical working mode
#define MODE_MON  0x2       ///< Kernel monitoring mode
#define MODE_DUMP 0x10      ///< Kernel dump working mode


#define IMMEDIATE 1         ///< Immediate timeout. Forces a read call to return immediately.


// The following definitions are used to provide compatibility 
// of the dump files with the ones of libpcap
#define TCPDUMP_MAGIC 0xa1b2c3d4    ///< Libpcap magic number. Used by programs like tcpdump to recognize a driver's generated dump file.
#define PCAP_VERSION_MAJOR 2        ///< Major libpcap version of the dump file. Used by programs like tcpdump to recognize a driver's generated dump file.
#define PCAP_VERSION_MINOR 4        ///< Minor libpcap version of the dump file. Used by programs like tcpdump to recognize a driver's generated dump file.

/*!
  \brief Header of a libpcap dump file.

  Used when a driver instance is set in dump mode to create a libpcap-compatible file.
*/
struct packet_file_header 
{
    UINT magic;             ///< Libpcap magic number
    USHORT version_major;   ///< Libpcap major version
    USHORT version_minor;   ///< Libpcap minor version
    UINT thiszone;          ///< Gmt to local correction
    UINT sigfigs;           ///< Accuracy of timestamps
    UINT snaplen;           ///< Length of the max saved portion of each packet
    UINT linktype;          ///< Data link type (DLT_*). See win_bpf.h for details.
};

/*!
  \brief Header associated to a packet in the driver's buffer when the driver is in dump mode.
  Similar to the bpf_hdr structure, but simpler.
*/
struct sf_pkthdr {
    struct timeval  ts;         ///< time stamp
    UINT            caplen;     ///< Length of captured portion. The captured portion can be different from 
                                ///< the original packet, because it is possible (with a proper filter) to 
                                ///< instruct the driver to capture only a portion of the packets. 
    UINT            len;        ///< Length of the original packet (off wire).
};

/*!
  \brief Stores an OID request.
  
  This structure is used by the driver to perform OID query or set operations on the underlying NIC driver. 
  The OID operations be performed usually only by network drivers, but NPF exports this mechanism to user-level 
  applications through an IOCTL interface. The driver uses this structure to wrap a NDIS_REQUEST structure.
  This allows to handle correctly the callback structure of NdisRequest(), handling multiple requests and
  maintaining information about the IRPs to complete.
*/
typedef struct _INTERNAL_REQUEST {
    LIST_ENTRY      ListElement;        ///< Used to handle lists of requests.
    PIRP            Irp;                ///< Irp that performed the request
    BOOLEAN         Internal;           ///< True if the request is for internal use of npf.sys. False if the request is performed by the user through an IOCTL.
    NDIS_REQUEST    Request;            ///< The structure with the actual request, that will be passed to NdisRequest().
} INTERNAL_REQUEST, *PINTERNAL_REQUEST;

/*!
  \brief Contains a NDIS packet.
  
  The driver uses this structure to wrap a NDIS_PACKET  structure.
  This allows to handle correctly the callback structure of NdisTransferData(), handling multiple requests and
  maintaining information about the IRPs to complete.
*/
typedef struct _PACKET_RESERVED {
    LIST_ENTRY      ListElement;        ///< Used to handle lists of packets.
    PIRP            Irp;                ///< Irp that performed the request
    PMDL            pMdl;               ///< MDL mapping the buffer of the packet.
    BOOLEAN         FreeBufAfterWrite;  ///< True if the memory buffer associated with the packet must be freed 
                                        ///< after a call to NdisSend().
}  PACKET_RESERVED, *PPACKET_RESERVED;

#define RESERVED(_p) ((PPACKET_RESERVED)((_p)->ProtocolReserved)) ///< Macro to obtain a NDIS_PACKET from a PACKET_RESERVED

/*!
  \brief Port device extension.
  
  Structure containing some data relative to every adapter on which NPF is bound.
*/
typedef struct _DEVICE_EXTENSION {
    NDIS_HANDLE    NdisProtocolHandle;  ///< NDIS handle of NPF.
    NDIS_STRING    AdapterName;         ///< Name of the adapter.
    PWSTR          ExportString;        ///< Name of the exported device, i.e. name that the applications will use 
                                        ///< to open this adapter through WinPcap.
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/*!
  \brief Contains the state of a running instance of the NPF driver.
  
  This is the most important structure of NPF: it is used by almost all the functions of the driver. An
  _OPEN_INSTANCE structure is associated with every user-level session, allowing concurrent access
  to the driver.
*/
typedef struct _OPEN_INSTANCE
{
    PDEVICE_EXTENSION   DeviceExtension;    ///< Pointer to the _DEVICE_EXTENSION structure of the device on which
                                            ///< the instance is bound.
    NDIS_HANDLE         AdapterHandle;      ///< NDIS idetifier of the adapter used by this instance.
    UINT                Medium;             ///< Type of physical medium the underlying NDIS driver uses. See the
                                            ///< documentation of NdisOpenAdapter in the MS DDK for details.
    NDIS_HANDLE         PacketPool;         ///< Pool of NDIS_PACKET structures used to transfer the packets from and to the NIC driver.
    PIRP                OpenCloseIrp;       ///< Pointer used to store the open/close IRP requests and provide them to the 
                                            ///< callbacks of NDIS.
    KSPIN_LOCK          RequestSpinLock;    ///< SpinLock used to synchronize the OID requests.
    LIST_ENTRY          RequestList;        ///< List of pending OID requests.
    LIST_ENTRY          ResetIrpList;       ///< List of pending adapter reset requests.
    INTERNAL_REQUEST    Requests[MAX_REQUESTS]; ///< Array of structures that wrap every single OID request.
    PMDL                BufferMdl;          ///< Pointer to a Memory descriptor list (MDL) that maps the circular buffer's memory.
    PKEVENT             ReadEvent;          ///< Pointer to the event on which the read calls on this instance must wait.
    HANDLE              ReadEventHandle;    ///< Handle of the event on which the read calls on this instance must wait.
    UNICODE_STRING      ReadEventName;      ///< Name of the event on which the read calls on this instance must wait.
                                            ///< The event is created with a name, so it can be used at user level to know when it 
                                            ///< is possible to access the driver without being blocked. This fiels stores the name 
                                            ///< that and is used by the BIOCGEVNAME IOCTL call.
    INT                 Received;           ///< Number of packets received by current instance from its opening, i.e. number of 
                                            ///< packet received by the network adapter since the beginning of the 
                                            ///< capture/monitoring/dump session.
    INT                 Dropped;            ///< Number of packet that current instance had to drop, from its opening. A packet 
                                            ///< is dropped if there is no more space to store it in the circular buffer that the 
                                            ///< driver associates to current instance.
    INT                 Accepted;           ///< Number of packet that current capture instance acepted, from its opening. A packet 
                                            ///< is accepted if it passes the filter and fits in the buffer. Accepted packets are the
                                            ///< ones that reach the application.
    PUCHAR              bpfprogram;         ///< Pointer to the filtering pseudo-code associated with current instance of the driver.
                                            ///< This code is used only in particular situations (for example when the packet received
                                            ///< from the NIC driver is stored in two non-consecutive buffers. In normal situations
                                            ///< the filtering routine created by the JIT compiler and pointed by the next field 
                                            ///< is used. See \ref NPF for details on the filtering process.
    JIT_BPF_Filter      *Filter;            ///< Pointer to the native filtering function created by the jitter. 
                                            ///< See BPF_jitter() for details.
    PUCHAR              Buffer;             ///< Pointer to the circular buffer associated with every driver instance. It contains the 
                                            ///< data that will be passed to the application. See \ref NPF for details.
    UINT                Bhead;              ///< Head of the circular buffer.
    UINT                Btail;              ///< Tail of the circular buffer.
    UINT                BufSize;            ///< Size of the circular buffer.
    UINT                BLastByte;          ///< Position of the last valid byte in the circular buffer.
    PMDL                TransferMdl;        ///< MDL used to map the portion of the buffer that will contain an incoming packet. 
                                            ///< Used by NdisTransferData().
    NDIS_SPIN_LOCK      BufLock;            ///< SpinLock that protects the access tho the circular buffer variables.
    UINT                MinToCopy;          ///< Minimum amount of data in the circular buffer that unlocks a read. Set with the
                                            ///< BIOCSMINTOCOPY IOCTL.
    LARGE_INTEGER       TimeOut;            ///< Timeout after which a read is released, also if the amount of data in the buffer is 
                                            ///< less than MinToCopy. Set with the BIOCSRTIMEOUT IOCTL.
                                            
    int                 mode;               ///< Working mode of the driver. See PacketSetMode() for details.
    LARGE_INTEGER       Nbytes;             ///< Amount of bytes accepted by the filter when this instance is in statistical mode.
    LARGE_INTEGER       Npackets;           ///< Number of packets accepted by the filter when this instance is in statistical mode.
    NDIS_SPIN_LOCK      CountersLock;       ///< SpinLock that protects the statistical mode counters.
    UINT                Nwrites;            ///< Number of times a single write must be physically repeated. See \ref NPF for an 
                                            ///< explanation
    UINT                Multiple_Write_Counter; ///< Counts the number of times a single write has already physically repeated.
    NDIS_EVENT          WriteEvent;         ///< Event used to synchronize the multiple write process.
    NDIS_EVENT          IOEvent;            ///< Event used to synchronize I/O requests with the callback structure of NDIS.
    NDIS_STATUS         IOStatus;           ///< Maintains the status of and OID request call, that will be passed to the application.
    BOOLEAN             Bound;              ///< Specifies if NPF is still bound to the adapter used by this instance. Bound can be
                                            ///< FALSE if a Plug and Play adapter has been removed or disabled by the user.
    HANDLE              DumpFileHandle;     ///< Handle of the file used in dump mode.
    PFILE_OBJECT        DumpFileObject;     ///< Pointer to the object of the file used in dump mode.
    PKTHREAD            DumpThreadObject;   ///< Pointer to the object of the thread used in dump mode.
    HANDLE              DumpThreadHandle;   ///< Handle of the thread created by dump mode to asynchronously move the buffer to disk.
    NDIS_EVENT          DumpEvent;          ///< Event used to synchronize the dump thread with the tap when the instance is in dump mode.
    LARGE_INTEGER       DumpOffset;         ///< Current offset in the dump file.
    UNICODE_STRING      DumpFileName;       ///< String containing the name of the dump file.
    UINT                MaxDumpBytes;       ///< Maximum dimension in bytes of the dump file. If the dump file reaches this size it 
                                            ///< will be closed. A value of 0 means unlimited size.
    UINT                MaxDumpPacks;       ///< Maximum number of packets that will be saved in the dump file. If this number of 
                                            ///< packets is reached the dump will be closed. A value of 0 means unlimited number of 
                                            ///< packets.
    BOOLEAN             DumpLimitReached;   ///< TRUE if the maximum dimension of the dump file (MaxDumpBytes or MaxDumpPacks) is 
                                            ///< reached.
    MEM_TYPE            mem_ex;             ///< Memory used by the TME virtual co-processor
    TME_CORE            tme;                ///< Data structure containing the virtualization of the TME co-processor
    NDIS_SPIN_LOCK      machine_lock;       ///< SpinLock that protects the mem_ex buffer
    UINT                MaxFrameSize;       ///< Maximum frame size that the underlying MAC acceptes. Used to perform a check on the 
                                            ///< size of the frames sent with NPF_Write() or NPF_BufferedWrite().
} OPEN_INSTANCE, *POPEN_INSTANCE;


#define TRANSMIT_PACKETS 256    ///< Maximum number of packets in the transmit packet pool. This value is an upper bound to the number
                                ///< of packets that can be transmitted at the same time or with a single call to NdisSendPackets.


/// Macro used in the I/O routines to return the control to user-mode with a success status.
#define EXIT_SUCCESS(quantity) Irp->IoStatus.Information=quantity;\
    Irp->IoStatus.Status = STATUS_SUCCESS;\
    IoCompleteRequest(Irp, IO_NO_INCREMENT);\
    return STATUS_SUCCESS;\

/// Macro used in the I/O routines to return the control to user-mode with a failure status.
#define EXIT_FAILURE(quantity) Irp->IoStatus.Information=quantity;\
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;\
    IoCompleteRequest(Irp, IO_NO_INCREMENT);\
    return STATUS_UNSUCCESSFUL;\

/**
 *  @}
 */


/***************************/
/*       Prototypes        */
/***************************/

/** @defgroup NPF_code NPF functions 
 *  @{
 */


/*!
  \brief The initialization routine of the driver.
  \param DriverObject The driver object of NPF created by the system.
  \param RegistryPath The registry path containing the keys related to the driver.
  \return A string containing a list of network adapters.

  DriverEntry is a mandatory function in a device driver. Like the main() of a user level program, it is called
  by the system when the driver is loaded in memory and started. Its purpose is to initialize the driver, 
  performing all the allocations and the setup. In particular, DriverEntry registers all the driver's I/O
  callbacks, creates the devices, defines NPF as a protocol inside NDIS.
*/ 
//NTSTATUS
//DriverEntry(
//    IN PDRIVER_OBJECT DriverObject,
//    IN PUNICODE_STRING RegistryPath
//    );

/*!
  \brief Returns the list of the MACs available on the system.
  \return A string containing a list of network adapters.

  The list of adapters is retrieved from the 
  SYSTEM\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002BE10318} registry key. 
  NPF tries to create its bindings from this list. In this way it is possible to be loaded
  and unloaded dynamically without passing from the control panel.
*/
PWCHAR getAdaptersList(VOID);

/*!
  \brief Returns the MACs that bind to TCP/IP.
  \return Pointer to the registry key containing the list of adapters on which TCP/IP is bound.

  If getAdaptersList() fails, NPF tries to obtain the TCP/IP bindings through this function.
*/
PKEY_VALUE_PARTIAL_INFORMATION getTcpBindings(VOID);

/*!
  \brief Creates a device for a given MAC.
  \param adriverObjectP The driver object that will be associated with the device, i.e. the one of NPF.
  \param amacNameP The name of the network interface that the device will point.
  \param aProtoHandle NDIS protocol handle of NPF.
  \return If the function succeeds, the return value is nonzero.

  NPF creates a device for every valid network adapter. The new device points to the NPF driver, but contains
  information about the original device. In this way, when the user opens the new device, NPF will be able to
  determine the correct adapter to use.
*/
BOOLEAN createDevice(
    IN OUT PDRIVER_OBJECT adriverObjectP,
    IN PUNICODE_STRING amacNameP,
    NDIS_HANDLE aProtoHandle);

/*!
  \brief Opens a new instance of the driver.
  \param DeviceObject Pointer to the device object utilized by the user.
  \param Irp Pointer to the IRP containing the user request.
  \return The status of the operation. See ntstatus.h in the DDK.

  This function is called by the OS when a new instance of the driver is opened, i.e. when a user application 
  performs a CreateFile on a device created by NPF. NPF_Open allocates and initializes variables, objects
  and buffers needed by the new instance, fills the OPEN_INSTANCE structure associated with it and opens the 
  adapter with a call to NdisOpenAdapter.
*/
NTSTATUS
NPF_Open(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

/*!
  \brief Ends the opening of an adapter.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.
  \param Status Status of the opening operation performed by NDIS.
  \param OpenErrorStatus not used by NPF.

  Callback function associated with the NdisOpenAdapter() NDIS function. It is invoked by NDIS when the NIC 
  driver has finished an open operation that was previously started by NPF_Open().
*/
VOID
NPF_OpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus
    );

/*!
  \brief Closes an instance of the driver.
  \param DeviceObject Pointer to the device object utilized by the user.
  \param Irp Pointer to the IRP containing the user request.
  \return The status of the operation. See ntstatus.h in the DDK.

  This function is called when a running instance of the driver is closed by the user with a CloseHandle(). 
  It stops the capture/monitoring/dump process, deallocates the memory and the objects associated with the 
  instance and closing the files. The network adapter is then closed with a call to NdisCloseAdapter. 
*/
NTSTATUS
NPF_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

/*!
  \brief Ends the closing of an adapter.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.
  \param Status Status of the close operation performed by NDIS.

  Callback function associated with the NdisCloseAdapter() NDIS function. It is invoked by NDIS when the NIC 
  driver has finished a close operation that was previously started by NPF_Close().
*/
VOID
NPF_CloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );

/*!
  \brief Callback invoked by NDIS when a packet arrives from the network.
  \param ProtocolBindingContext Context of the function. Points to a OPEN_INSTANCE structure that identifies 
   the NPF instance to which the packets are destined.
  \param MacReceiveContext Handle that identifies the underlying NIC driver that generated the request. 
   This value must be used when the packet is transferred from the NIC driver with NdisTransferData().
  \param HeaderBuffer Pointer to the buffer in the NIC driver memory that contains the header of the packet.
  \param HeaderBufferSize Size in bytes of the header.
  \param LookAheadBuffer Pointer to the buffer in the NIC driver's memory that contains the incoming packet's 
   data <b>available to NPF</b>. This value does not necessarily coincide with the actual size of the packet,
   since only a portion can be available at this time. The remaining portion can be obtained with the
   NdisTransferData() NDIS function.
  \param LookaheadBufferSize Size in bytes of the lookahead buffer.
  \param PacketSize Total size of the incoming packet, excluded the header.
  \return The status of the operation. See ntstatus.h in the DDK.

  NPF_tap() is called by the underlying NIC for every incoming packet. It is the most important and one of 
  the most complex functions of NPF: it executes the filter, runs the statistical engine (if the instance is in 
  statistical mode), gathers the timestamp, moves the packet in the buffer. NPF_tap() is the only function,
  along with the filtering ones, that is executed for every incoming packet, therefore it is carefully 
  optimized.
*/
NDIS_STATUS
NPF_tap(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    );

/*!
  \brief Ends the transfer of a packet.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.
  \param Packet Pointer to the NDIS_PACKET structure that received the packet data.
  \param Status Status of the transfer operation.
  \param BytesTransferred Amount of bytes transferred.

  Callback function associated with the NdisTransferData() NDIS function. It is invoked by NDIS when the NIC 
  driver has finished the transfer of a packet from the NIC driver memory to the NPF circular buffer.
*/
VOID
NPF_TransferDataComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    );

/*!
  \brief Callback function that signals the end of a packet reception.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.

  does nothing in NPF
*/
VOID
NPF_ReceiveComplete(IN NDIS_HANDLE  ProtocolBindingContext);

/*!
  \brief Handles the IOCTL calls.
  \param DeviceObject Pointer to the device object utilized by the user.
  \param Irp Pointer to the IRP containing the user request.
  \return The status of the operation. See ntstatus.h in the DDK.

  Once the packet capture driver is opened it can be configured from user-level applications with IOCTL commands
  using the DeviceIoControl() system call. NPF_IoControl receives and serves all the IOCTL calls directed to NPF.
  The following commands are recognized: 
  - #BIOCSETBUFFERSIZE 
  - #BIOCSETF 
  - #BIOCGSTATS 
  - #BIOCSRTIMEOUT
  - #BIOCSMODE 
  - #BIOCSWRITEREP 
  - #BIOCSMINTOCOPY 
  - #BIOCSETOID 
  - #BIOCQUERYOID 
  - #BIOCSETDUMPFILENAME
  - #BIOCGEVNAME
  - #BIOCSENDPACKETSSYNC
  - #BIOCSENDPACKETSNOSYNC
*/
NTSTATUS
NPF_IoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


/*!
  \brief Ends an OID request.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.
  \param pRequest Pointer to the completed OID request. 
  \param Status Status of the operation.

  Callback function associated with the NdisRequest() NDIS function. It is invoked by NDIS when the NIC 
  driver has finished an OID request operation that was previously started by NPF_IoControl().
*/
VOID
NPF_RequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST pRequest,
    IN NDIS_STATUS   Status
    );

/*!
  \brief Writes a raw packet to the network.
  \param DeviceObject Pointer to the device object on which the user wrote the packet.
  \param Irp Pointer to the IRP containing the user request.
  \return The status of the operation. See ntstatus.h in the DDK.

  This function is called by the OS in consequence of user WriteFile() call, with the data of the packet that must
  be sent on the net. The data is contained in the buffer associated with Irp, NPF_Write takes it and
  delivers it to the NIC driver via the NdisSend() function. The Nwrites field of the OPEN_INSTANCE structure 
  associated with Irp indicates the number of copies of the packet that will be sent: more than one copy of the
  packet can be sent for performance reasons.
*/
NTSTATUS
NPF_Write(
            IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp
            );


/*!
  \brief Writes a buffer of raw packets to the network.
  \param Irp Pointer to the IRP containing the user request.
  \param UserBuff Pointer to the buffer containing the packets to send.
  \param UserBuffSize Size of the buffer with the packets.
  \return The amount of bytes actually sent. If the return value is smaller than the Size parameter, an
          error occurred during the send. The error can be caused by an adapter problem or by an
          inconsistent/bogus user buffer.

  This function is called by the OS in consequence of a BIOCSENDPACKETSNOSYNC or a BIOCSENDPACKETSSYNC IOCTL.
  The buffer received as input parameter contains an arbitrary number of packets, each of which preceded by a
  sf_pkthdr structure. NPF_BufferedWrite() scans the buffer and sends every packet via the NdisSend() function.
  When Sync is set to TRUE, the packets are synchronized with the KeQueryPerformanceCounter() function.
  This requires a remarkable amount of CPU, but allows to respect the timestamps associated with packets with a precision 
  of some microseconds (depending on the precision of the performance counter of the machine).
  If Sync is false, the timestamps are ignored and the packets are sent as fat as possible.
*/

INT NPF_BufferedWrite(IN PIRP Irp, 
                        IN PCHAR UserBuff, 
                        IN ULONG UserBuffSize,
                        BOOLEAN sync);

/*!
  \brief Ends a send operation.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.
  \param pRequest Pointer to the NDIS PACKET structure used by NPF_Write() to send the packet. 
  \param Status Status of the operation.

  Callback function associated with the NdisSend() NDIS function. It is invoked by NDIS when the NIC 
  driver has finished an OID request operation that was previously started by NPF_Write().
*/
VOID
NPF_SendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    );

/*!
  \brief Ends a reset of the adapter.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with the current instance.
  \param Status Status of the operation.

  Callback function associated with the NdisReset() NDIS function. It is invoked by NDIS when the NIC 
  driver has finished an OID request operation that was previously started by NPF_IoControl(), in an IOCTL_PROTOCOL_RESET 
  command.
*/
VOID
NPF_ResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    );

/*!
  \brief Callback for NDIS StatusHandler. Not used by NPF
*/
VOID
NPF_Status(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    );


/*!
  \brief Callback for NDIS StatusCompleteHandler. Not used by NPF
*/
VOID
NPF_StatusComplete(IN NDIS_HANDLE  ProtocolBindingContext);

/*!
  \brief Function called by the OS when NPF is unloaded.
  \param DriverObject The driver object of NPF created by the system.

  This is the last function executed when the driver is unloaded from the system. It frees global resources,
  delete the devices and deregisters the protocol. The driver can be unloaded by the user stopping the NPF
  service (from control panel or with a console 'net stop npf').
*/
VOID
NPF_Unload(IN PDRIVER_OBJECT DriverObject);


/*!
  \brief Function that serves the user's reads.
  \param DeviceObject Pointer to the device used by the user.
  \param Irp Pointer to the IRP containing the user request.
  \return The status of the operation. See ntstatus.h in the DDK.

  This function is called by the OS in consequence of user ReadFile() call. It moves the data present in the
  kernel buffer to the user buffer associated with Irp.
  First of all, NPF_Read checks the amount of data in kernel buffer associated with current NPF instance. 
  - If the instance is in capture mode and the buffer contains more than OPEN_INSTANCE::MinToCopy bytes,
  NPF_Read moves the data in the user buffer and returns immediatly. In this way, the read performed by the
  user is not blocking.
  - If the buffer contains less than MinToCopy bytes, the application's request isn't 
  satisfied immediately, but it's blocked until at least MinToCopy bytes arrive from the net 
  or the timeout on this read expires. The timeout is kept in the OPEN_INSTANCE::TimeOut field.
  - If the instance is in statistical mode or in dump mode, the application's request is blocked until the 
  timeout kept in OPEN_INSTANCE::TimeOut expires.
*/
NTSTATUS
NPF_Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

/*!
  \brief Reads the registry keys associated woth NPF if the driver is manually installed via the control panel.

  Normally not used in recent versions of NPF.
*/
NTSTATUS
NPF_ReadRegistry(
    IN  PWSTR              *MacDriverName,
    IN  PWSTR              *PacketDriverName,
    IN  PUNICODE_STRING     RegistryPath
    );

/*!
  \brief Function used by NPF_ReadRegistry() to quesry the registry keys associated woth NPF if the driver 
  is manually installed via the control panel.

  Normally not used in recent versions of NPF.
*/
NTSTATUS
NPF_QueryRegistryRoutine(
    IN PWSTR     ValueName,
    IN ULONG     ValueType,
    IN PVOID     ValueData,
    IN ULONG     ValueLength,
    IN PVOID     Context,
    IN PVOID     EntryContext
    );

/*!
  \brief Callback for NDIS BindAdapterHandler. Not used by NPF.
  
  Function called by NDIS when a new adapter is installed on the machine With Plug and Play.
*/
VOID NPF_BindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            DeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    );

/*!
  \brief Callback for NDIS UnbindAdapterHandler.
  \param Status out variable filled by NPF_UnbindAdapter with the status of the unbind operation.
  \param ProtocolBindingContext Context of the function. Contains a pointer to the OPEN_INSTANCE structure associated with current instance.
  \param UnbindContext Specifies a handle, supplied by NDIS, that NPF can use to complete the opration.
  
  Function called by NDIS when a new adapter is removed from the machine without shutting it down.
  NPF_UnbindAdapter closes the adapter calling NdisCloseAdapter() and frees the memory and the structures
  associated with it. It also releases the waiting user-level app and closes the dump thread if the instance
  is in dump mode.
*/
VOID
NPF_UnbindAdapter(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         UnbindContext
    );

/*!
  \brief Validates a filtering program arriving from the user-level app.
  \param f The filter.
  \param len Its length, in pseudo instructions.
  \param mem_ex_size The length of the extended memory, used to validate LD/ST to that memory
  \return true if f is a valid filter program..
  
  The kernel needs to be able to verify an application's filter code. Otherwise, a bogus program could easily 
  crash the system.
  This function returns true if f is a valid filter program. The constraints are that each jump be forward and 
  to a valid code.  The code must terminate with either an accept or reject. 
*/
int bpf_validate(struct bpf_insn *f,int len, uint32 mem_ex_size);

/*!
  \brief The filtering pseudo-machine interpreter.
  \param pc The filter.
  \param p Pointer to a memory buffer containing the packet on which the filter will be executed.
  \param wirelen Original length of the packet.
  \param buflen Current length of the packet. In some cases (for example when the transfer of the packet to the RAM
  has not yet finished), bpf_filter can be executed on a portion of the packet.
  \param mem_ex The extended memory.
  \param tme The virtualization of the TME co-processor
  \param time_ref Data structure needed by the TME co-processor to timestamp data
  \return The portion of the packet to keep, in bytes. 0 means that the packet must be rejected, -1 means that
   the whole packet must be kept.
  
  \note this function is not used in normal situations, because the jitter creates a native filtering function
  that is faster than the interpreter.
*/
UINT bpf_filter(register struct bpf_insn *pc,
                register UCHAR *p,
                UINT wirelen,
                register UINT buflen,
                PMEM_TYPE mem_ex,
                PTME_CORE tme,
                struct time_conv *time_ref);

/*!
  \brief The filtering pseudo-machine interpreter with two buffers. This function is slower than bpf_filter(), 
  but works correctly also if the MAC header and the data of the packet are in two different buffers.
  \param pc The filter.
  \param p Pointer to a memory buffer containing the MAC header of the packet.
  \param pd Pointer to a memory buffer containing the data of the packet.
  \param wirelen Original length of the packet.
  \param buflen Current length of the packet. In some cases (for example when the transfer of the packet to the RAM
  has not yet finished), bpf_filter can be executed on a portion of the packet.
  \param mem_ex The extended memory.
  \param tme The virtualization of the TME co-processor
  \param time_ref Data structure needed by the TME co-processor to timestamp data
  \return The portion of the packet to keep, in bytes. 0 means that the packet must be rejected, -1 means that
   the whole packet must be kept.
  
  This function is used when NDIS passes the packet to NPF_tap() in two buffers instaed than in a single one.
*/
UINT bpf_filter_with_2_buffers(register struct bpf_insn *pc,
                               register UCHAR *p,
                               register UCHAR *pd,
                               register int headersize,
                               UINT wirelen,
                               register UINT buflen,
                               PMEM_TYPE mem_ex,
                               PTME_CORE tme,
                               struct time_conv *time_ref);

/*!
  \brief Creates the file that will receive the packets when the driver is in dump mode.
  \param Open The NPF instance that opens the file.
  \param fileName Pointer to a UNICODE string containing the name of the file.
  \param append Boolean value that specifies if the data must be appended to the file.
  \return The status of the operation. See ntstatus.h in the DDK.
*/
NTSTATUS NPF_OpenDumpFile(POPEN_INSTANCE Open , PUNICODE_STRING fileName, BOOLEAN append);

/*!
  \brief Starts dump to file.
  \param Open The NPF instance that opens the file.
  \return The status of the operation. See ntstatus.h in the DDK.

  This function performs two operations. First, it writes the libpcap header at the beginning of the file.
  Second, it starts the thread that asynchronously dumps the network data to the file.
*/
NTSTATUS NPF_StartDump(POPEN_INSTANCE Open);

/*!
  \brief The dump thread.
  \param Open The NPF instance that creates the thread.

  This function moves the content of the NPF kernel buffer to file. It runs in the user context, so at lower 
  priority than the TAP.
*/
VOID NPF_DumpThread(POPEN_INSTANCE Open);

/*!
  \brief Saves the content of the packet buffer to the file associated with current instance.
  \param Open The NPF instance that creates the thread.

  Used by NPF_DumpThread() and NPF_CloseDumpFile().
*/
NTSTATUS NPF_SaveCurrentBuffer(POPEN_INSTANCE Open);

/*!
  \brief Writes a block of packets on the dump file.
  \param FileObject The file object that will receive the packets.
  \param Offset The offset in the file where the packets will be put.
  \param Length The amount of bytes to write.
  \param Mdl MDL mapping the memory buffer that will be written to disk.
  \param IoStatusBlock Used by the function to return the status of the operation.
  \return The status of the operation. See ntstatus.h in the DDK.

  NPF_WriteDumpFile addresses directly the file system, creating a custom IRP and using it to send a portion
  of the NPF circular buffer to disk. This function is used by NPF_DumpThread().
*/
VOID NPF_WriteDumpFile(PFILE_OBJECT FileObject,
                                PLARGE_INTEGER Offset,
                                ULONG Length,
                                PMDL Mdl,
                                PIO_STATUS_BLOCK IoStatusBlock);



/*!
  \brief Closes the dump file associated with an instance of the driver.
  \param Open The NPF instance that closes the file.
  \return The status of the operation. See ntstatus.h in the DDK.
*/
NTSTATUS NPF_CloseDumpFile(POPEN_INSTANCE Open);

/*!
  \brief Returns the amount of bytes present in the packet buffer.
  \param Open The NPF instance that closes the file.
*/
UINT GetBuffOccupation(POPEN_INSTANCE Open);

/*!
  \brief Called by NDIS to notify us of a PNP event. The most significant one for us is power state change.

  \param ProtocolBindingContext Pointer to open context structure. This is NULL for global reconfig 
  events.
  \param pNetPnPEvent Pointer to the PnP event

  If there is a power state change, the driver is forced to resynchronize the global timer.
  This hopefully avoids the synchronization issues caused by hibernation or standby.
  This function is excluded from the NT4 driver, where PnP is not supported
*/
#ifdef NDIS50
NDIS_STATUS NPF_PowerChange(IN NDIS_HANDLE ProtocolBindingContext, IN PNET_PNP_EVENT pNetPnPEvent);
#endif

/**
 *  @}
 */

/**
 *  @}
 */

#endif  /*main ifndef/define*/
