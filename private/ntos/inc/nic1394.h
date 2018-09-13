/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	nic1394.h

Abstract:

	This module defines the structures, macros, and manifests available
	to IEE1394-aware network components.

Revision History:

	09/14/1998 JosephJ	Created.

--*/

#ifndef	_NIC1394_H_
#define	_NIC1394_H_

///////////////////////////////////////////////////////////////////////////////////
//         					ADDRESS FAMILY VERSION INFORMATION
///////////////////////////////////////////////////////////////////////////////////

//
// The current major and minor version, respectively, of the NIC1394 address family.
//
#define NIC1394_AF_CURRENT_MAJOR_VERSION	5
#define NIC1394_AF_CURRENT_MINOR_VERSION	0

///////////////////////////////////////////////////////////////////////////////////
//         					MEDIA PARAMETERS 									 // 
///////////////////////////////////////////////////////////////////////////////////

//
// 1394 FIFO Address, consisting of the 64-bit UniqueID and the
// 48-bit address offset.
//
typedef struct _NIC1394_FIFO_ADDRESS
{
	UINT64 				UniqueID;
	ULONG           	Off_Low;
	USHORT          	Off_High;

} NIC1394_FIFO_ADDRESS, *PNIC1394_FIFO_ADDRESS;


//	enum to identify which of the two modes of transmission on a 1394 is to be used
//
//

typedef enum _NIC1394_ADDRESS_TYPE
{
	NIC1394AddressType_Channel,		 // Indicates this is a channel address
	NIC1394AddressType_FIFO,		 // Indicates this is a FIFO address

} NIC1394_ADDRESS_TYPE, *PNIC1394_ADDRESS_TYPE;



//
// General form of a 1394 destination, which can specify either a 1394 channel or
// a FIFO address. This structure forms part of the 1394 media-specific
// parameters.
//
typedef struct _NIC1394_DESTINATION
{
	union
	{
		UINT                    Channel;     // IEEE1394 channel number.
		NIC1394_FIFO_ADDRESS    FifoAddress; // IEEE1394 NodeID and address offset.
	};


	NIC1394_ADDRESS_TYPE        AddressType; // Address- asynch or isoch  

} NIC1394_DESTINATION, *PNIC1394_DESTINATION;

//
// Special channels  values
//
#define NIC1394_ANY_CHANNEL 		((UINT)-1) // miniport should pick channel.
#define NIC1394_BROADCAST_CHANNEL	((UINT)-2) // special broadcast channel.

//
// This is the value of the ParamType field in the CO_SPECIFIC_PARAMETERS structure
// when the Parameters[] field contains IEEE1394 media specific values in the
// structure NIC1394_MEDIA_PARAMETERS.
//
#define NIC1394_MEDIA_SPECIFIC		0x13940000


//
// NOTE:
// The CO_MEDIA_PARAMETERS.Flags field for FIFO vcs must specify either TRANSMIT_VC
// or RECEIVE_VC, not both. If RECEIVE_VC is specified for a FIFO vc, this vc is
// used to receive on a local FIFO. In this case, the  Destination.RecvFIFO field
// must be set to all-0s when creating the vc. On activation of the vc,
// this field of the updated media parameters will contain the local nodes unique ID
// and the allocated FIFO address.
// 

// 
// 1394 Specific Media parameters - this is the Media specific structure for 1394
// that goes into MediaParameters->MediaSpecific.Parameters.
//
typedef struct _NIC1394_MEDIA_PARAMETERS
{
	//
	// Identifies destination type (channel or FIFO) and type-specific address.
	//
	NIC1394_DESTINATION 	Destination;

	//
	// Bitmap encoding characteristics of the vc. One or  more NIC1394_VCFLAG_*
	// values.
	//
	ULONG					Flags;  	  	

 	//
	// Maximum size, in bytes, of blocks to be sent on this vc. Must be set to 0
	// if this is a recv-only VCs. The miniport will choose a block size that is a
	// minimum of this value and the value dictated by the bus speed map.
	// Special value (ULONG -1) indicates "maximum possible block size."
	UINT 					MaxSendBlockSize;

	//
	// One of the SCODE_* constants defined in 1394.h. Indicates
	// the maximum speed to be used for blocks sent on this vc. Must be set to 0
	// if this is a recv-only VC. The miniport will choose a speed that is a minimum
	// of this value and the value dicated by the bus speed map.
	// Special value (ULONG -1) indicates "maximum possible speed."
	//
	// TODO: change to ... MaxSendSpeedCode;
	//
	UINT 					MaxSendSpeed;

	//
	// Size, in bytes, of the largest packet that will be sent or received on
	// this VC. The miniport may use this information to set up internal buffers
	// for link-layer fragmentation and reassembly. The miniport will
	// fail attempts to send packets and will discard received packets if the
	// size of these packets is larger than the MTU.
	//
	UINT					MTU;
	//
 	// Amount of bandwidth to reserve, in units of bytes per isochronous frame.
	// Applies only for isochronous transmission, and must be set to 0 for
	// asynchronous transmission (i.e., if the NIC1394_VCFLAG_ISOCHRONOUS bit is 0).
	//
	UINT 					Bandwidth;	

	//
	// One or more NIC1394_FRAMETYPE_* values. The miniport will attempt to send up
	// only pkts with these protocols. However it may send other pkts.
	// The client should be able to deal with this. Must be set to 0 if
	// no framing is used (i.e., if the NIC1394_VCFLAG_FRAMED bit is 0).
	//
	ULONG 					RecvFrameTypes;

} NIC1394_MEDIA_PARAMETERS, *PNIC1394_MEDIA_PARAMETERS;


//
// NIC1394_MEDIA_PARAMETERS.Flags bitfield values
//

//
// Indicates VC will be used for isochronous transmission.
//
#define NIC1394_VCFLAG_ISOCHRONOUS		(0x1 << 1)

//
// Indicates that the vc is used for framed data. If set, the miniport will
// implement link-level fragmentation and reassembly. If clear, the miniport
// will treat data sent and received on this vc as raw data.
//
#define NIC1394_VCFLAG_FRAMED			(0x1 << 2)

//
// Indicates the miniport should allocate the necessary bus resources.
// Currently this only applies for non-broadcast channels, in which case
// the bus resources consist of the network channel number and (for isochronous
// vc's) the bandwidth specified in Bandwidth field.
// This bit does not apply (and should be 0) when creating the broadcast channel
// and either transmit or receive FIFO vcs.
//
#define NIC1394_VCFLAG_ALLOCATE			(0x1 << 3)

//
// End of NIC1394_MEDIA_PARAMETERS.Flags bitfield values.
//

//
// NIC1394_MEDIA_PARAMETERS.FrameType bitfield values
//
#define NIC1394_FRAMETYPE_ARP	 	(0x1<<0) // Ethertype 0x806
#define NIC1394_FRAMETYPE_IPV4	 	(0x1<<1) // Ethertype 0x800
#define NIC1394_FRAMETYPE_IPV4MCAP	(0x1<<2) // Ethertype 0x8861



///////////////////////////////////////////////////////////////////////////////////
//                          INFORMATIONAL OIDs                                   // 
///////////////////////////////////////////////////////////////////////////////////

//
// the structure for returning basic information about the miniport
// returned in response to OID_NIC1394_LOCAL_NODE_INFO. Associated with
// the address family handle.
//
typedef struct _NIC1394_LOCAL_NODE_INFO
{
	UINT64					UniqueID;			// This node's 64-bit Unique ID.
	ULONG					BusGeneration;  	// 1394 Bus generation ID.
	NODE_ADDRESS			NodeAddress; 		// Local nodeID for the current bus
												// generation.
	USHORT					Reserved;			// Padding.
	UINT 					MaxRecvBlockSize; 	// Maximum size, in bytes, of blocks
												// that can be read.
	UINT 					MaxRecvSpeed;		// Max speed which can be accepted
												// -- minimum
												// of the max local link speed and
												// the max local PHY speed.

} NIC1394_LOCAL_NODE_INFO, *PNIC1394_LOCAL_NODE_INFO;


//
// The structure for returning basic information about the specified vc
// returned in response to OID_NIC1394_VC_INFO. Associated with
// a vc handle
//
typedef struct _NIC1394_VC_INFO
{
	//
	// Channel or (unique-ID,offset). In the case of a recv (local) FIFO vc,
	// this will be set to the local node's unique ID and address offset.
	//
	NIC1394_DESTINATION Destination;

} NIC1394_VC_INFO, *PNIC1394_VC_INFO;



///////////////////////////////////////////////////////////////////////////////////
//                          INDICATIONS                                          // 
///////////////////////////////////////////////////////////////////////////////////
// Bus Reset
// Params: NIC1394_LOCAL_NODE_INFO

///////////////////////////////////////////////////////////////////////////////////
//                          PACKET FORMATS                                       // 
///////////////////////////////////////////////////////////////////////////////////


//
// GASP Header, which prefixes all ip/1394 pkts sent over channels.
// TODO: move this withing NIC1394, because it is not exposed to protocols.
//
typedef struct _NIC1394_GASP_HEADER
{
	USHORT	source_ID;
	USHORT	specifier_ID_hi;
	UCHAR	specifier_ID_lo;
	UCHAR	version[3];

}  NIC1394_GASP_HEADER;

//
// Unfragmented encapsulation header.
//
typedef struct _NIC1394_ENCAPSULATION_HEADER
{
	// The Reserved field must be set to 0.
	//
	USHORT Reserved;

	// The EtherType field is set to the byte-swapped version of one of the
	// constants defined immediately below. 
	//
	USHORT EtherType;

	// Ethertypes in machine byte order. These values need to be byteswapped
	// before they are sent on the wire.
	//
	#define NIC1394_ETHERTYPE_IP	0x800
	#define NIC1394_ETHERTYPE_ARP	0x806
	#define NIC1394_ETHERTYPE_MCAP	0x8861

} NIC1394_ENCAPSULATION_HEADER, *PNIC1394_ENCAPSULATION_HEADER;

//
// TODO: get rid of NIC1394_ENCAPSULATION_HEADER
//
typedef
NIC1394_ENCAPSULATION_HEADER
NIC1394_UNFRAGMENTED_HEADER, *PNIC1394_UNFRAGMENTED_HEADER;


//
//			FRAGMENTED PACKET FORMATS
//
//		TODO: move these to inside NIC1394, because they are only
//		used within NIC1394.
//

//
// Fragmented Encapsulation header: first fragment
//
typedef struct _NIC1394_FIRST_FRAGMENT_HEADER
{
	// Contains the 2-bit "lf" field and the 12-bit "buffer_size" field.
	// Use the macros immediately below to extract the above fields from
	// the lfbufsz. This field needs to be byteswapped before it is sent out
	// on the wire.
	//
	USHORT	lfbufsz;

	#define NIC1394_LF_FROM_LFBUFSZ(_lfbufsz) \
							((_lfbufz) >> 14)

	#define NIC1394_BUFFER_SIZE_FROM_LFBUFSZ(_lfbufsz) \
							((_lfbufz) & 0xfff)

	#define NIC1394_MAX_FRAGMENT_BUFFER_SIZE	0xfff

	//
	// specifies what the packet is - an IPV4, ARP, or MCAP packet
	//
	USHORT EtherType;


	// Opaque datagram label. There is no need to byteswap this field before it
	// is sent out on the wire.
	//
	USHORT dgl;

	// Must be set to 0
	//
	USHORT reserved;

}  NIC1394_FIRST_FRAGMENT_HEADER, *PNIC1394_FIRST_FRAGMENT_HEADER;

//
// Fragmented Encapsulation header: second and subsequent fragments
//
typedef struct _NIC1394_FRAGMENT_HEADER
{
#if OBSOLETE
	ULONG lf:2;                         // Bits 0-1
	ULONG rsv0:2;                       // Bits 2-3
	ULONG buffer_size:12;               // Bits 4-15

	ULONG rsv1:4;                       // Bits 16-19
	ULONG fragment_offset:12;           // Bits 20-31

	ULONG dgl:16;                       // Bits 0-15

	ULONG reserved:16;                 	// Bits 16-32 
#endif // OBSOLETE

	// Contains the 2-bit "lf" field and the 12-bit "buffer_size" field.
	// The format is the same as NIC1394_FIRST_FRAGMENT_HEADER.lfbufsz.
	//
	USHORT	lfbufsz;

	// Opaque datagram label. There is no need to byteswap this field before it
	// is setn out on the wire.
	//
	USHORT dgl;

	// Fragment offset. Must be less than or equal to NIC1394_MAX_FRAGMENT_OFFSET.
	// This field needs to be byteswapped before it is sent out on the wire.
	//
	USHORT fragment_offset;

	#define NIC1394_MAX_FRAGMENT_OFFSET 0xfff

}  NIC1394_FRAGMENT_HEADER, *PNIC1394_FRAGMENT_HEADER;





#define OID_1394_ISSUE_BUS_RESET		0x0C010201

#endif	//	 _NIC1394_H_



