/*
 * ndis.h
 *
 * Network Device Interface Specification definitions
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * DEFINES: i386                 - Target platform is i386
 *          _NDIS_               - Define only for NDIS library
 *          NDIS_MINIPORT_DRIVER - Define only for NDIS miniport drivers
 *          NDIS40               - Use NDIS 4.0 structures by default
 *          NDIS50               - Use NDIS 5.0 structures by default
 *          NDIS50_MINIPORT      - Building NDIS 5.0 miniport driver
 *          NDIS51_MINIPORT      - Building NDIS 5.1 miniport driver
 */
#ifndef __NDIS_H
#define __NDIS_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include "ntddndis.h"
#include "netpnp.h"
#include "netevent.h"
#include <winsock2.h>

#if defined(_NDIS_)
  #define NDISAPI DECLSPEC_EXPORT
#else
  #define NDISAPI DECLSPEC_IMPORT
#endif

#if defined(NDIS50_MINIPORT)
#ifndef NDIS50
#define NDIS50
#endif
#endif /* NDIS50_MINIPORT */

#if defined(NDIS51_MINIPORT)
#ifndef NDIS51
#define NDIS51
#endif
#endif /* NDIS51_MINIPORT */

/* NDIS 3.0 is default */
#if !defined(NDIS30) || !defined(NDIS40) || !defined(NDIS50) || !defined(NDIS51)
#define NDIS30
#endif /* !NDIS30 || !NDIS40 || !NDIS50 || !NDIS51 */

#if 1
/* FIXME: */
typedef PVOID QUEUED_CLOSE;
#endif

typedef ULONG NDIS_OID, *PNDIS_OID;

typedef struct _X_FILTER FDDI_FILTER, *PFDDI_FILTER;
typedef struct _X_FILTER TR_FILTER, *PTR_FILTER;
typedef struct _X_FILTER NULL_FILTER, *PNULL_FILTER;

typedef struct _REFERENCE {
	KSPIN_LOCK  SpinLock;
	USHORT  ReferenceCount;
	BOOLEAN  Closing;
} REFERENCE, * PREFERENCE;


/* NDIS base types */

typedef struct _NDIS_SPIN_LOCK {
  KSPIN_LOCK  SpinLock;
  KIRQL  OldIrql;
} NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

typedef struct _NDIS_EVENT {
  KEVENT  Event;
} NDIS_EVENT, *PNDIS_EVENT;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;
typedef int NDIS_STATUS, *PNDIS_STATUS;

typedef ANSI_STRING NDIS_ANSI_STRING, *PNDIS_ANSI_STRING;
typedef UNICODE_STRING NDIS_STRING, *PNDIS_STRING;

typedef MDL NDIS_BUFFER, *PNDIS_BUFFER;
typedef ULONG NDIS_ERROR_CODE, *PNDIS_ERROR_CODE;


/* NDIS_STATUS constants */
#define NDIS_STATUS_SUCCESS                     ((NDIS_STATUS)STATUS_SUCCESS)
#define NDIS_STATUS_PENDING                     ((NDIS_STATUS)STATUS_PENDING)
#define NDIS_STATUS_NOT_RECOGNIZED              ((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED                  ((NDIS_STATUS)0x00010002L)
#define NDIS_STATUS_NOT_ACCEPTED                ((NDIS_STATUS)0x00010003L)
#define NDIS_STATUS_CALL_ACTIVE                 ((NDIS_STATUS)0x00010007L)
#define NDIS_STATUS_ONLINE                      ((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START                 ((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END                   ((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS                 ((NDIS_STATUS)0x40010006L)
#define NDIS_STATUS_CLOSED                      ((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP                 ((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN               ((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT                ((NDIS_STATUS)0x4001000AL)
#define	NDIS_STATUS_MEDIA_CONNECT               ((NDIS_STATUS)0x4001000BL)
#define	NDIS_STATUS_MEDIA_DISCONNECT            ((NDIS_STATUS)0x4001000CL)
#define NDIS_STATUS_HARDWARE_LINE_UP            ((NDIS_STATUS)0x4001000DL)
#define NDIS_STATUS_HARDWARE_LINE_DOWN          ((NDIS_STATUS)0x4001000EL)
#define NDIS_STATUS_INTERFACE_UP                ((NDIS_STATUS)0x4001000FL)
#define NDIS_STATUS_INTERFACE_DOWN              ((NDIS_STATUS)0x40010010L)
#define NDIS_STATUS_MEDIA_BUSY                  ((NDIS_STATUS)0x40010011L)
#define NDIS_STATUS_MEDIA_SPECIFIC_INDICATION   ((NDIS_STATUS)0x40010012L)
#define NDIS_STATUS_WW_INDICATION               NDIS_STATUS_MEDIA_SPECIFIC_INDICATION
#define NDIS_STATUS_LINK_SPEED_CHANGE           ((NDIS_STATUS)0x40010013L)
#define NDIS_STATUS_WAN_GET_STATS               ((NDIS_STATUS)0x40010014L)
#define NDIS_STATUS_WAN_CO_FRAGMENT             ((NDIS_STATUS)0x40010015L)
#define NDIS_STATUS_WAN_CO_LINKPARAMS           ((NDIS_STATUS)0x40010016L)

#define NDIS_STATUS_NOT_RESETTABLE              ((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS	                ((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS                 ((NDIS_STATUS)0x80010004L)
#define NDIS_STATUS_BUFFER_OVERFLOW	            ((NDIS_STATUS)STATUS_BUFFER_OVERFLOW)

#define NDIS_STATUS_FAILURE	                    ((NDIS_STATUS)STATUS_UNSUCCESSFUL)
#define NDIS_STATUS_RESOURCES                   ((NDIS_STATUS)STATUS_INSUFFICIENT_RESOURCES)
#define NDIS_STATUS_CLOSING	                    ((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION	                ((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS         ((NDIS_STATUS)0xC0010005L)
#define NDIS_STATUS_ADAPTER_NOT_FOUND           ((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED	                ((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED               ((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL              ((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS            ((NDIS_STATUS)0xC001000AL)
#define NDIS_STATUS_MULTICAST_NOT_FOUND	        ((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED	            ((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS           ((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING          ((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_NOT_SUPPORTED               ((NDIS_STATUS)STATUS_NOT_SUPPORTED)
#define NDIS_STATUS_INVALID_PACKET              ((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL              ((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY           ((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN            ((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING              ((NDIS_STATUS)0xC0010013L)
#define NDIS_STATUS_INVALID_LENGTH              ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA                ((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT            ((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID	                ((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED	            ((NDIS_STATUS)0xC0010018L)
#define NDIS_STATUS_UNSUPPORTED_MEDIA           ((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE        ((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND              ((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE          ((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED              ((NDIS_STATUS)0xC001001DL)
#define NDIS_STATUS_RESOURCE_CONFLICT           ((NDIS_STATUS)0xC001001EL)
#define NDIS_STATUS_NO_CABLE                    ((NDIS_STATUS)0xC001001FL)

#define NDIS_STATUS_INVALID_SAP	                ((NDIS_STATUS)0xC0010020L)
#define NDIS_STATUS_SAP_IN_USE                  ((NDIS_STATUS)0xC0010021L)
#define NDIS_STATUS_INVALID_ADDRESS             ((NDIS_STATUS)0xC0010022L)
#define NDIS_STATUS_VC_NOT_ACTIVATED            ((NDIS_STATUS)0xC0010023L)
#define NDIS_STATUS_DEST_OUT_OF_ORDER           ((NDIS_STATUS)0xC0010024L)
#define NDIS_STATUS_VC_NOT_AVAILABLE            ((NDIS_STATUS)0xC0010025L)
#define NDIS_STATUS_CELLRATE_NOT_AVAILABLE      ((NDIS_STATUS)0xC0010026L)
#define NDIS_STATUS_INCOMPATABLE_QOS            ((NDIS_STATUS)0xC0010027L)
#define NDIS_STATUS_AAL_PARAMS_UNSUPPORTED      ((NDIS_STATUS)0xC0010028L)
#define NDIS_STATUS_NO_ROUTE_TO_DESTINATION     ((NDIS_STATUS)0xC0010029L)

#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR       ((NDIS_STATUS)0xC0011000L)
#define NDIS_STATUS_INVALID_DEVICE_REQUEST      ((NDIS_STATUS)STATUS_INVALID_DEVICE_REQUEST)
#define NDIS_STATUS_NETWORK_UNREACHABLE         ((NDIS_STATUS)STATUS_NETWORK_UNREACHABLE)


/* NDIS error codes for error logging */

#define NDIS_ERROR_CODE_RESOURCE_CONFLICT			            EVENT_NDIS_RESOURCE_CONFLICT
#define NDIS_ERROR_CODE_OUT_OF_RESOURCES			            EVENT_NDIS_OUT_OF_RESOURCE
#define NDIS_ERROR_CODE_HARDWARE_FAILURE			            EVENT_NDIS_HARDWARE_FAILURE
#define NDIS_ERROR_CODE_ADAPTER_NOT_FOUND			            EVENT_NDIS_ADAPTER_NOT_FOUND
#define NDIS_ERROR_CODE_INTERRUPT_CONNECT			            EVENT_NDIS_INTERRUPT_CONNECT
#define NDIS_ERROR_CODE_DRIVER_FAILURE				            EVENT_NDIS_DRIVER_FAILURE
#define NDIS_ERROR_CODE_BAD_VERSION					              EVENT_NDIS_BAD_VERSION
#define NDIS_ERROR_CODE_TIMEOUT						                EVENT_NDIS_TIMEOUT
#define NDIS_ERROR_CODE_NETWORK_ADDRESS				            EVENT_NDIS_NETWORK_ADDRESS
#define NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION	        EVENT_NDIS_UNSUPPORTED_CONFIGURATION
#define NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER	      EVENT_NDIS_INVALID_VALUE_FROM_ADAPTER
#define NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER	  EVENT_NDIS_MISSING_CONFIGURATION_PARAMETER
#define NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS			          EVENT_NDIS_BAD_IO_BASE_ADDRESS
#define NDIS_ERROR_CODE_RECEIVE_SPACE_SMALL			          EVENT_NDIS_RECEIVE_SPACE_SMALL
#define NDIS_ERROR_CODE_ADAPTER_DISABLED			            EVENT_NDIS_ADAPTER_DISABLED


/* Memory allocation flags. Used by Ndis[Allocate|Free]Memory */
#define NDIS_MEMORY_CONTIGUOUS            0x00000001
#define NDIS_MEMORY_NONCACHED             0x00000002

/* NIC attribute flags. Used by NdisMSetAttributes(Ex) */
#define	NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT    0x00000001
#define NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT   0x00000002
#define NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS 0x00000004
#define NDIS_ATTRIBUTE_BUS_MASTER               0x00000008
#define NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER      0x00000010
#define NDIS_ATTRIBUTE_DESERIALIZE              0x00000020
#define NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND       0x00000040
#define NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK       0x00000080
#define NDIS_ATTRIBUTE_NOT_CO_NDIS              0x00000100
#define NDIS_ATTRIBUTE_USES_SAFE_BUFFER_APIS    0x00000200


/* Lock */

typedef union _NDIS_RW_LOCK_REFCOUNT {
  UINT  RefCount;
  UCHAR  cacheLine[16];
} NDIS_RW_LOCK_REFCOUNT;

typedef struct _NDIS_RW_LOCK {
  union {
    struct {
      KSPIN_LOCK  SpinLock;
      PVOID  Context;
    } s;
    UCHAR  Reserved[16];
  } u;

  NDIS_RW_LOCK_REFCOUNT  RefCount[MAXIMUM_PROCESSORS];
} NDIS_RW_LOCK, *PNDIS_RW_LOCK;

typedef struct _LOCK_STATE {
  USHORT  LockState;
  KIRQL  OldIrql;
} LOCK_STATE, *PLOCK_STATE;



/* Timer */

typedef VOID DDKAPI
(*PNDIS_TIMER_FUNCTION)(
	IN PVOID  SystemSpecific1,
	IN PVOID  FunctionContext,
	IN PVOID  SystemSpecific2,
	IN PVOID  SystemSpecific3);

typedef struct _NDIS_TIMER {
  KTIMER  Timer;
  KDPC  Dpc;
} NDIS_TIMER, *PNDIS_TIMER;



/* Hardware */

typedef CM_MCA_POS_DATA NDIS_MCA_POS_DATA, *PNDIS_MCA_POS_DATA;
typedef CM_EISA_SLOT_INFORMATION NDIS_EISA_SLOT_INFORMATION, *PNDIS_EISA_SLOT_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION NDIS_EISA_FUNCTION_INFORMATION, *PNDIS_EISA_FUNCTION_INFORMATION;
typedef CM_PARTIAL_RESOURCE_LIST NDIS_RESOURCE_LIST, *PNDIS_RESOURCE_LIST;

/* Hardware status codes (OID_GEN_HARDWARE_STATUS) */
typedef enum _NDIS_HARDWARE_STATUS {
  NdisHardwareStatusReady,
  NdisHardwareStatusInitializing,
  NdisHardwareStatusReset,
  NdisHardwareStatusClosing,
  NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;

/* OID_GEN_GET_TIME_CAPS */
typedef struct _GEN_GET_TIME_CAPS {
  ULONG  Flags;
  ULONG  ClockPrecision;
} GEN_GET_TIME_CAPS, *PGEN_GET_TIME_CAPS;

/* Flag bits */
#define	READABLE_LOCAL_CLOCK                    0x00000001
#define	CLOCK_NETWORK_DERIVED                   0x00000002
#define	CLOCK_PRECISION                         0x00000004
#define	RECEIVE_TIME_INDICATION_CAPABLE         0x00000008
#define	TIMED_SEND_CAPABLE                      0x00000010
#define	TIME_STAMP_CAPABLE                      0x00000020

/* OID_GEN_GET_NETCARD_TIME */
typedef struct _GEN_GET_NETCARD_TIME {
  ULONGLONG  ReadTime;
} GEN_GET_NETCARD_TIME, *PGEN_GET_NETCARD_TIME;

/* NDIS driver medium (OID_GEN_MEDIA_SUPPORTED / OID_GEN_MEDIA_IN_USE) */
typedef enum _NDIS_MEDIUM {
  NdisMedium802_3,
  NdisMedium802_5,
  NdisMediumFddi,
  NdisMediumWan,
  NdisMediumLocalTalk,
  NdisMediumDix,
  NdisMediumArcnetRaw,
  NdisMediumArcnet878_2,
  NdisMediumAtm,
  NdisMediumWirelessWan,
  NdisMediumIrda,
  NdisMediumBpc,
  NdisMediumCoWan,
  NdisMedium1394,
  NdisMediumMax
} NDIS_MEDIUM, *PNDIS_MEDIUM;

/* NDIS packet filter bits (OID_GEN_CURRENT_PACKET_FILTER) */
#define NDIS_PACKET_TYPE_DIRECTED               0x00000001
#define NDIS_PACKET_TYPE_MULTICAST              0x00000002
#define NDIS_PACKET_TYPE_ALL_MULTICAST          0x00000004
#define NDIS_PACKET_TYPE_BROADCAST              0x00000008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING         0x00000010
#define NDIS_PACKET_TYPE_PROMISCUOUS            0x00000020
#define NDIS_PACKET_TYPE_SMT                    0x00000040
#define NDIS_PACKET_TYPE_ALL_LOCAL              0x00000080
#define NDIS_PACKET_TYPE_GROUP                  0x00001000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL         0x00002000
#define NDIS_PACKET_TYPE_FUNCTIONAL             0x00004000
#define NDIS_PACKET_TYPE_MAC_FRAME              0x00008000

/* NDIS protocol option bits (OID_GEN_PROTOCOL_OPTIONS) */
#define NDIS_PROT_OPTION_ESTIMATED_LENGTH       0x00000001
#define NDIS_PROT_OPTION_NO_LOOPBACK            0x00000002
#define NDIS_PROT_OPTION_NO_RSVD_ON_RCVPKT      0x00000004

/* NDIS MAC option bits (OID_GEN_MAC_OPTIONS) */
#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA     0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED      0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND      0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK             0x00000008
#define NDIS_MAC_OPTION_FULL_DUPLEX             0x00000010
#define	NDIS_MAC_OPTION_EOTX_INDICATION         0x00000020
#define	NDIS_MAC_OPTION_8021P_PRIORITY          0x00000040
#define NDIS_MAC_OPTION_RESERVED                0x80000000

/* State of the LAN media (OID_GEN_MEDIA_CONNECT_STATUS) */
typedef enum _NDIS_MEDIA_STATE {
	NdisMediaStateConnected,
	NdisMediaStateDisconnected
} NDIS_MEDIA_STATE, *PNDIS_MEDIA_STATE;

/* OID_GEN_SUPPORTED_GUIDS */
typedef struct _NDIS_GUID {
	GUID  Guid;
	union {
		NDIS_OID  Oid;
		NDIS_STATUS  Status;
	} u;
	ULONG  Size;
	ULONG  Flags;
} NDIS_GUID, *PNDIS_GUID;

#define	NDIS_GUID_TO_OID                  0x00000001
#define	NDIS_GUID_TO_STATUS               0x00000002
#define	NDIS_GUID_ANSI_STRING             0x00000004
#define	NDIS_GUID_UNICODE_STRING          0x00000008
#define	NDIS_GUID_ARRAY	                  0x00000010


typedef struct _NDIS_PACKET_POOL {
  NDIS_SPIN_LOCK  SpinLock;
  struct _NDIS_PACKET *FreeList;
  UINT  PacketLength;
  UCHAR  Buffer[1];
} NDIS_PACKET_POOL, * PNDIS_PACKET_POOL;

/* NDIS_PACKET_PRIVATE.Flags constants */
#define fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO    0x40
#define fPACKET_ALLOCATED_BY_NDIS               0x80

typedef struct _NDIS_PACKET_PRIVATE {
  UINT  PhysicalCount;
  UINT  TotalLength;
  PNDIS_BUFFER  Head;
  PNDIS_BUFFER  Tail;
  PNDIS_PACKET_POOL  Pool;
  UINT  Count;
  ULONG  Flags;
  BOOLEAN	 ValidCounts;
  UCHAR  NdisPacketFlags;
  USHORT  NdisPacketOobOffset;
} NDIS_PACKET_PRIVATE, * PNDIS_PACKET_PRIVATE;

typedef struct _NDIS_PACKET {
  NDIS_PACKET_PRIVATE  Private;
  union {
    struct {
      UCHAR  MiniportReserved[2 * sizeof(PVOID)];
      UCHAR  WrapperReserved[2 * sizeof(PVOID)];
    } s1;
    struct {
      UCHAR  MiniportReservedEx[3 * sizeof(PVOID)];
      UCHAR  WrapperReservedEx[sizeof(PVOID)];
    } s2;
    struct {
      UCHAR  MacReserved[4 * sizeof(PVOID)];
    } s3;
  } u;
  ULONG_PTR  Reserved[2];
  UCHAR  ProtocolReserved[1];
} NDIS_PACKET, *PNDIS_PACKET, **PPNDIS_PACKET;

typedef enum _NDIS_CLASS_ID {
	NdisClass802_3Priority,
	NdisClassWirelessWanMbxMailbox,
	NdisClassIrdaPacketInfo,
	NdisClassAtmAALInfo
} NDIS_CLASS_ID;

typedef struct MediaSpecificInformation {
  UINT  NextEntryOffset;
  NDIS_CLASS_ID  ClassId;
  UINT  Size;
  UCHAR  ClassInformation[1];
} MEDIA_SPECIFIC_INFORMATION;

typedef struct _NDIS_PACKET_OOB_DATA {
  _ANONYMOUS_UNION union {
    ULONGLONG  TimeToSend;
    ULONGLONG  TimeSent;
  } DUMMYUNIONNAME;
  ULONGLONG  TimeReceived;
  UINT  HeaderSize;
  UINT  SizeMediaSpecificInfo;
  PVOID  MediaSpecificInformation;
  NDIS_STATUS  Status;
} NDIS_PACKET_OOB_DATA, *PNDIS_PACKET_OOB_DATA;

typedef struct _NDIS_PM_PACKET_PATTERN {
  ULONG  Priority;
  ULONG  Reserved;
  ULONG  MaskSize;
  ULONG  PatternOffset;
  ULONG  PatternSize;
  ULONG  PatternFlags;
} NDIS_PM_PACKET_PATTERN,  *PNDIS_PM_PACKET_PATTERN;


/* Request types used by NdisRequest */
typedef enum _NDIS_REQUEST_TYPE {
  NdisRequestQueryInformation,
  NdisRequestSetInformation,
  NdisRequestQueryStatistics,
  NdisRequestOpen,
  NdisRequestClose,
  NdisRequestSend,
  NdisRequestTransferData,
  NdisRequestReset,
  NdisRequestGeneric1,
  NdisRequestGeneric2,
  NdisRequestGeneric3,
  NdisRequestGeneric4
} NDIS_REQUEST_TYPE, *PNDIS_REQUEST_TYPE;

typedef struct _NDIS_REQUEST {
  UCHAR  MacReserved[4 * sizeof(PVOID)];
  NDIS_REQUEST_TYPE  RequestType;
  union _DATA {
    struct QUERY_INFORMATION {
      NDIS_OID  Oid;
      PVOID  InformationBuffer;
      UINT  InformationBufferLength;
      UINT  BytesWritten;
      UINT  BytesNeeded;
    } QUERY_INFORMATION;
    struct SET_INFORMATION {
      NDIS_OID  Oid;
      PVOID  InformationBuffer;
      UINT  InformationBufferLength;
      UINT  BytesRead;
      UINT  BytesNeeded;
    } SET_INFORMATION;
 } DATA;
#if (defined(NDIS50) || defined(NDIS51))
  UCHAR  NdisReserved[9 * sizeof(PVOID)];
  union {
    UCHAR  CallMgrReserved[2 * sizeof(PVOID)];
    UCHAR  ProtocolReserved[2 * sizeof(PVOID)];
  };
  UCHAR  MiniportReserved[2 * sizeof(PVOID)];
#endif
} NDIS_REQUEST, *PNDIS_REQUEST;



/* Wide Area Networks definitions */

typedef struct _NDIS_WAN_PACKET {
  LIST_ENTRY  WanPacketQueue;
  PUCHAR  CurrentBuffer;
  ULONG  CurrentLength;
  PUCHAR  StartBuffer;
  PUCHAR  EndBuffer;
  PVOID  ProtocolReserved1;
  PVOID  ProtocolReserved2;
  PVOID  ProtocolReserved3;
  PVOID  ProtocolReserved4;
  PVOID  MacReserved1;
  PVOID  MacReserved2;
  PVOID  MacReserved3;
  PVOID  MacReserved4;
} NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;



/* DMA channel information */

typedef struct _NDIS_DMA_DESCRIPTION {
  BOOLEAN  DemandMode;
  BOOLEAN  AutoInitialize;
  BOOLEAN  DmaChannelSpecified;
  DMA_WIDTH  DmaWidth;
  DMA_SPEED  DmaSpeed;
  ULONG  DmaPort;
  ULONG  DmaChannel;
} NDIS_DMA_DESCRIPTION, *PNDIS_DMA_DESCRIPTION;

typedef struct _NDIS_DMA_BLOCK {
  PVOID  MapRegisterBase;
  KEVENT  AllocationEvent;
  PADAPTER_OBJECT  SystemAdapterObject;
  PVOID  Miniport;
  BOOLEAN  InProgress;
} NDIS_DMA_BLOCK, *PNDIS_DMA_BLOCK;


/* Possible hardware architecture */
typedef enum _NDIS_INTERFACE_TYPE {
	NdisInterfaceInternal = Internal,
	NdisInterfaceIsa = Isa,
	NdisInterfaceEisa = Eisa,
	NdisInterfaceMca = MicroChannel,
	NdisInterfaceTurboChannel = TurboChannel,
	NdisInterfacePci = PCIBus,
	NdisInterfacePcMcia = PCMCIABus,
	NdisInterfaceCBus = CBus,
	NdisInterfaceMPIBus = MPIBus,
	NdisInterfaceMPSABus = MPSABus,
	NdisInterfaceProcessorInternal = ProcessorInternal,
	NdisInterfaceInternalPowerBus = InternalPowerBus,
	NdisInterfacePNPISABus = PNPISABus,
	NdisInterfacePNPBus = PNPBus,
	NdisMaximumInterfaceType
} NDIS_INTERFACE_TYPE, *PNDIS_INTERFACE_TYPE;

#define NdisInterruptLevelSensitive       LevelSensitive
#define NdisInterruptLatched              Latched
typedef KINTERRUPT_MODE NDIS_INTERRUPT_MODE, *PNDIS_INTERRUPT_MODE;


typedef enum _NDIS_PARAMETER_TYPE {
  NdisParameterInteger,
  NdisParameterHexInteger,
  NdisParameterString,
  NdisParameterMultiString,
  NdisParameterBinary
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef struct {
	USHORT  Length;
	PVOID  Buffer;
} BINARY_DATA;

typedef struct _NDIS_CONFIGURATION_PARAMETER {
  NDIS_PARAMETER_TYPE  ParameterType;
  union {
    ULONG  IntegerData;
    NDIS_STRING  StringData;
    BINARY_DATA  BinaryData;
  } ParameterData;
} NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;


typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;

typedef struct _NDIS_PHYSICAL_ADDRESS_UNIT {
  NDIS_PHYSICAL_ADDRESS  PhysicalAddress;
  UINT  Length;
} NDIS_PHYSICAL_ADDRESS_UNIT, *PNDIS_PHYSICAL_ADDRESS_UNIT;

typedef struct _NDIS_WAN_LINE_DOWN {
  UCHAR  RemoteAddress[6];
  UCHAR  LocalAddress[6];
} NDIS_WAN_LINE_DOWN, *PNDIS_WAN_LINE_DOWN;

typedef struct _NDIS_WAN_LINE_UP {
  ULONG  LinkSpeed;
  ULONG  MaximumTotalSize;
  NDIS_WAN_QUALITY  Quality;
  USHORT  SendWindow;
  UCHAR  RemoteAddress[6];
  OUT UCHAR  LocalAddress[6];
  ULONG  ProtocolBufferLength;
  PUCHAR  ProtocolBuffer;
  USHORT  ProtocolType;
  NDIS_STRING  DeviceName;
} NDIS_WAN_LINE_UP, *PNDIS_WAN_LINE_UP;


typedef VOID DDKAPI
(*ADAPTER_SHUTDOWN_HANDLER)(
  IN PVOID  ShutdownContext);


typedef struct _OID_LIST    OID_LIST, *POID_LIST;

/* PnP state */

typedef enum _NDIS_PNP_DEVICE_STATE {
  NdisPnPDeviceAdded,
  NdisPnPDeviceStarted,
  NdisPnPDeviceQueryStopped,
  NdisPnPDeviceStopped,
  NdisPnPDeviceQueryRemoved,
  NdisPnPDeviceRemoved,
  NdisPnPDeviceSurpriseRemoved
} NDIS_PNP_DEVICE_STATE;

#define	NDIS_DEVICE_NOT_STOPPABLE                 0x00000001
#define	NDIS_DEVICE_NOT_REMOVEABLE                0x00000002
#define	NDIS_DEVICE_NOT_SUSPENDABLE	              0x00000004
#define NDIS_DEVICE_DISABLE_PM                    0x00000008
#define NDIS_DEVICE_DISABLE_WAKE_UP               0x00000010
#define NDIS_DEVICE_DISABLE_WAKE_ON_RECONNECT     0x00000020
#define NDIS_DEVICE_RESERVED                      0x00000040
#define NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET  0x00000080
#define NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH 0x00000100


/* OID_GEN_NETWORK_LAYER_ADDRESSES */
typedef struct _NETWORK_ADDRESS {
  USHORT  AddressLength; 
  USHORT  AddressType; 
  UCHAR  Address[1]; 
} NETWORK_ADDRESS, *PNETWORK_ADDRESS;

typedef struct _NETWORK_ADDRESS_LIST {
	LONG  AddressCount; 
	USHORT  AddressType; 
	NETWORK_ADDRESS  Address[1]; 
} NETWORK_ADDRESS_LIST, *PNETWORK_ADDRESS_LIST;

/* Protocol types supported by NDIS */
#define	NDIS_PROTOCOL_ID_DEFAULT        0x00
#define	NDIS_PROTOCOL_ID_TCP_IP         0x02
#define	NDIS_PROTOCOL_ID_IPX            0x06
#define	NDIS_PROTOCOL_ID_NBF            0x07
#define	NDIS_PROTOCOL_ID_MAX            0x0F
#define	NDIS_PROTOCOL_ID_MASK           0x0F


/* OID_GEN_TRANSPORT_HEADER_OFFSET */
typedef struct _TRANSPORT_HEADER_OFFSET {
	USHORT  ProtocolType; 
	USHORT  HeaderOffset; 
} TRANSPORT_HEADER_OFFSET, *PTRANSPORT_HEADER_OFFSET;


/* OID_GEN_CO_LINK_SPEED / OID_GEN_CO_MINIMUM_LINK_SPEED */
typedef struct _NDIS_CO_LINK_SPEED {
  ULONG  Outbound;
  ULONG  Inbound;
} NDIS_CO_LINK_SPEED, *PNDIS_CO_LINK_SPEED;

typedef ULONG NDIS_AF, *PNDIS_AF;
#define CO_ADDRESS_FAMILY_Q2931           ((NDIS_AF)0x1)
#define CO_ADDRESS_FAMILY_PSCHED          ((NDIS_AF)0x2)
#define CO_ADDRESS_FAMILY_L2TP            ((NDIS_AF)0x3)
#define CO_ADDRESS_FAMILY_IRDA            ((NDIS_AF)0x4)
#define CO_ADDRESS_FAMILY_1394            ((NDIS_AF)0x5)
#define CO_ADDRESS_FAMILY_PPP             ((NDIS_AF)0x6)
#define CO_ADDRESS_FAMILY_TAPI            ((NDIS_AF)0x800)
#define CO_ADDRESS_FAMILY_TAPI_PROXY      ((NDIS_AF)0x801)

#define CO_ADDRESS_FAMILY_PROXY           0x80000000

typedef struct {
  NDIS_AF  AddressFamily;
  ULONG  MajorVersion;
  ULONG  MinorVersion;
} CO_ADDRESS_FAMILY, *PCO_ADDRESS_FAMILY;

typedef struct _CO_FLOW_PARAMETERS {
  ULONG  TokenRate;
  ULONG  TokenBucketSize;
  ULONG  PeakBandwidth;
  ULONG  Latency;
  ULONG  DelayVariation;
  SERVICETYPE  ServiceType;
  ULONG  MaxSduSize;
  ULONG  MinimumPolicedSize;
} CO_FLOW_PARAMETERS, *PCO_FLOW_PARAMETERS;

typedef struct _CO_SPECIFIC_PARAMETERS {
  ULONG  ParamType;
  ULONG  Length;
  UCHAR  Parameters[1];
} CO_SPECIFIC_PARAMETERS, *PCO_SPECIFIC_PARAMETERS;

typedef struct _CO_CALL_MANAGER_PARAMETERS {
  CO_FLOW_PARAMETERS  Transmit;
  CO_FLOW_PARAMETERS  Receive;
  CO_SPECIFIC_PARAMETERS  CallMgrSpecific;
} CO_CALL_MANAGER_PARAMETERS, *PCO_CALL_MANAGER_PARAMETERS;

/* CO_MEDIA_PARAMETERS.Flags constants */
#define RECEIVE_TIME_INDICATION           0x00000001
#define USE_TIME_STAMPS                   0x00000002
#define TRANSMIT_VC	                      0x00000004
#define RECEIVE_VC                        0x00000008
#define INDICATE_ERRED_PACKETS            0x00000010
#define INDICATE_END_OF_TX                0x00000020
#define RESERVE_RESOURCES_VC              0x00000040
#define	ROUND_DOWN_FLOW	                  0x00000080
#define	ROUND_UP_FLOW                     0x00000100

typedef struct _CO_MEDIA_PARAMETERS {
  ULONG  Flags;
  ULONG  ReceivePriority;
  ULONG  ReceiveSizeHint;
  CO_SPECIFIC_PARAMETERS  MediaSpecific;
} CO_MEDIA_PARAMETERS, *PCO_MEDIA_PARAMETERS;

/* CO_CALL_PARAMETERS.Flags constants */
#define PERMANENT_VC                      0x00000001
#define CALL_PARAMETERS_CHANGED           0x00000002
#define QUERY_CALL_PARAMETERS             0x00000004
#define BROADCAST_VC                      0x00000008
#define MULTIPOINT_VC                     0x00000010

typedef struct _CO_CALL_PARAMETERS {
  ULONG  Flags;
  PCO_CALL_MANAGER_PARAMETERS  CallMgrParameters;
  PCO_MEDIA_PARAMETERS  MediaParameters;
} CO_CALL_PARAMETERS, *PCO_CALL_PARAMETERS;

typedef struct _CO_SAP {
  ULONG  SapType;
  ULONG  SapLength;
  UCHAR  Sap[1];
} CO_SAP, *PCO_SAP;

typedef struct _NDIS_IPSEC_PACKET_INFO {
  _ANONYMOUS_UNION union {
    struct {
      NDIS_HANDLE  OffloadHandle;
      NDIS_HANDLE  NextOffloadHandle;
    } Transmit;
    struct {
      ULONG  SA_DELETE_REQ : 1;
      ULONG  CRYPTO_DONE : 1;
      ULONG  NEXT_CRYPTO_DONE : 1;
      ULONG  CryptoStatus;
    } Receive;
  } DUMMYUNIONNAME;
} NDIS_IPSEC_PACKET_INFO, *PNDIS_IPSEC_PACKET_INFO;

/* NDIS_MAC_FRAGMENT.Errors constants */
#define WAN_ERROR_CRC               			0x00000001
#define WAN_ERROR_FRAMING           			0x00000002
#define WAN_ERROR_HARDWAREOVERRUN   			0x00000004
#define WAN_ERROR_BUFFEROVERRUN     			0x00000008
#define WAN_ERROR_TIMEOUT           			0x00000010
#define WAN_ERROR_ALIGNMENT         			0x00000020

typedef struct _NDIS_MAC_FRAGMENT {
  NDIS_HANDLE  NdisLinkContext;
  ULONG  Errors;
} NDIS_MAC_FRAGMENT, *PNDIS_MAC_FRAGMENT;

typedef struct _NDIS_MAC_LINE_DOWN {
  NDIS_HANDLE  NdisLinkContext;
} NDIS_MAC_LINE_DOWN, *PNDIS_MAC_LINE_DOWN;

typedef struct _NDIS_MAC_LINE_UP {
  ULONG  LinkSpeed;
  NDIS_WAN_QUALITY  Quality;
  USHORT  SendWindow;
  NDIS_HANDLE  ConnectionWrapperID;
  NDIS_HANDLE  NdisLinkHandle;
  NDIS_HANDLE  NdisLinkContext;
} NDIS_MAC_LINE_UP, *PNDIS_MAC_LINE_UP;

typedef struct _NDIS_PACKET_8021Q_INFO {
	_ANONYMOUS_UNION union {
		struct {
			UINT32  UserPriority : 3;
			UINT32  CanonicalFormatId : 1;
			UINT32  VlanId : 12;
			UINT32  Reserved : 16;
		} TagHeader;
		PVOID  Value;
	} DUMMYUNIONNAME;
} NDIS_PACKET_8021Q_INFO, *PNDIS_PACKET_8021Q_INFO;

typedef enum _NDIS_PER_PACKET_INFO {
	TcpIpChecksumPacketInfo,
	IpSecPacketInfo,
	TcpLargeSendPacketInfo,
	ClassificationHandlePacketInfo,
	NdisReserved,
	ScatterGatherListPacketInfo,
	Ieee8021QInfo,
	OriginalPacketInfo,
	PacketCancelId,
	MaxPerPacketInfo
} NDIS_PER_PACKET_INFO, *PNDIS_PER_PACKET_INFO;

typedef struct _NDIS_PACKET_EXTENSION {
  PVOID  NdisPacketInfo[MaxPerPacketInfo];
} NDIS_PACKET_EXTENSION, *PNDIS_PACKET_EXTENSION;

/*
 * PNDIS_PACKET
 * NDIS_GET_ORIGINAL_PACKET(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_ORIGINAL_PACKET(Packet) \
  NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, OriginalPacketInfo)

/*
 * PVOID
 * NDIS_GET_PACKET_CANCEL_ID(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_CANCEL_ID(Packet) \
  NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, PacketCancelId)

/*
 * PNDIS_PACKET_EXTENSION
 * NDIS_PACKET_EXTENSION_FROM_PACKET(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_PACKET_EXTENSION_FROM_PACKET(Packet) \
  ((PNDIS_PACKET_EXTENSION)((PUCHAR)(Packet) \
    + (Packet)->Private.NdisPacketOobOffset + sizeof(NDIS_PACKET_OOB_DATA)))

/*
 * PVOID
 * NDIS_PER_PACKET_INFO_FROM_PACKET(
 *   IN OUT  PNDIS_PACKET  Packet,
 *   IN NDIS_PER_PACKET_INFO  InfoType);
 */
#define NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, InfoType) \
  ((PNDIS_PACKET_EXTENSION)((PUCHAR)(Packet) + (Packet)->Private.NdisPacketOobOffset \
    + sizeof(NDIS_PACKET_OOB_DATA)))->NdisPacketInfo[(InfoType)]

/*
 * VOID
 * NDIS_SET_ORIGINAL_PACKET(
 *   IN OUT  PNDIS_PACKET  Packet,
 *   IN PNDIS_PACKET  OriginalPacket);
 */
#define NDIS_SET_ORIGINAL_PACKET(Packet, OriginalPacket) \
  NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, OriginalPacketInfo) = (OriginalPacket)

/*
 * VOID
 * NDIS_SET_PACKET_CANCEL_ID(
 *  IN PNDIS_PACKET  Packet
 *  IN ULONG_PTR  CancelId);
 */
#define NDIS_SET_PACKET_CANCEL_ID(Packet, CancelId) \
  NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, PacketCancelId) = (CancelId)

typedef enum _NDIS_TASK {
  TcpIpChecksumNdisTask,
  IpSecNdisTask,
  TcpLargeSendNdisTask,
  MaxNdisTask
} NDIS_TASK, *PNDIS_TASK;

typedef struct _NDIS_TASK_IPSEC {
  struct {
    ULONG  AH_ESP_COMBINED;
    ULONG  TRANSPORT_TUNNEL_COMBINED;
    ULONG  V4_OPTIONS;
    ULONG  RESERVED;
  } Supported;
 
  struct {
    ULONG  MD5 : 1;
    ULONG  SHA_1 : 1;
    ULONG  Transport : 1;
    ULONG  Tunnel : 1;
    ULONG  Send : 1;
    ULONG  Receive : 1;
  } V4AH;
 
  struct {
    ULONG  DES : 1;
    ULONG  RESERVED : 1;
    ULONG  TRIPLE_DES : 1;
    ULONG  NULL_ESP : 1;
    ULONG  Transport : 1;
    ULONG  Tunnel : 1;
    ULONG  Send : 1;
    ULONG  Receive : 1;
  } V4ESP;
} NDIS_TASK_IPSEC, *PNDIS_TASK_IPSEC;

typedef struct _NDIS_TASK_OFFLOAD {
  ULONG  Version;
  ULONG  Size;
  NDIS_TASK  Task;
  ULONG  OffsetNextTask;
  ULONG  TaskBufferLength;
  UCHAR  TaskBuffer[1];
} NDIS_TASK_OFFLOAD, *PNDIS_TASK_OFFLOAD;

/* NDIS_TASK_OFFLOAD_HEADER.Version constants */
#define NDIS_TASK_OFFLOAD_VERSION 1

typedef enum _NDIS_ENCAPSULATION {
  UNSPECIFIED_Encapsulation,
  NULL_Encapsulation,
  IEEE_802_3_Encapsulation,
  IEEE_802_5_Encapsulation,
  LLC_SNAP_ROUTED_Encapsulation,
  LLC_SNAP_BRIDGED_Encapsulation
} NDIS_ENCAPSULATION;

typedef struct _NDIS_ENCAPSULATION_FORMAT {
  NDIS_ENCAPSULATION  Encapsulation;
  struct {
    ULONG  FixedHeaderSize : 1;
    ULONG  Reserved : 31;
  } Flags;
  ULONG  EncapsulationHeaderSize;
} NDIS_ENCAPSULATION_FORMAT, *PNDIS_ENCAPSULATION_FORMAT;

typedef struct _NDIS_TASK_TCP_IP_CHECKSUM {
  struct {
    ULONG  IpOptionsSupported:1;
    ULONG  TcpOptionsSupported:1;
    ULONG  TcpChecksum:1;
    ULONG  UdpChecksum:1;
    ULONG  IpChecksum:1;
  } V4Transmit;
 
  struct {
    ULONG  IpOptionsSupported : 1;
    ULONG  TcpOptionsSupported : 1;
    ULONG  TcpChecksum : 1;
    ULONG  UdpChecksum : 1;
    ULONG  IpChecksum : 1;
  } V4Receive;
 
  struct {
    ULONG  IpOptionsSupported : 1;
    ULONG  TcpOptionsSupported : 1;
    ULONG  TcpChecksum : 1;
    ULONG  UdpChecksum : 1;
  } V6Transmit;
 
  struct {
    ULONG  IpOptionsSupported : 1;
    ULONG  TcpOptionsSupported : 1;
    ULONG  TcpChecksum : 1;
    ULONG  UdpChecksum : 1;
  } V6Receive;
} NDIS_TASK_TCP_IP_CHECKSUM, *PNDIS_TASK_TCP_IP_CHECKSUM;

typedef struct _NDIS_TASK_TCP_LARGE_SEND {
  ULONG  Version;
  ULONG  MaxOffLoadSize;
  ULONG  MinSegmentCount;
  BOOLEAN  TcpOptions;
  BOOLEAN  IpOptions;
} NDIS_TASK_TCP_LARGE_SEND, *PNDIS_TASK_TCP_LARGE_SEND;

typedef struct _NDIS_TCP_IP_CHECKSUM_PACKET_INFO {
  _ANONYMOUS_UNION union {
    struct {
      ULONG  NdisPacketChecksumV4 : 1;
      ULONG  NdisPacketChecksumV6 : 1;
      ULONG  NdisPacketTcpChecksum : 1;
      ULONG  NdisPacketUdpChecksum : 1;
      ULONG  NdisPacketIpChecksum : 1;
      } Transmit;
 
    struct {
      ULONG  NdisPacketTcpChecksumFailed : 1;
      ULONG  NdisPacketUdpChecksumFailed : 1;
      ULONG  NdisPacketIpChecksumFailed : 1;
      ULONG  NdisPacketTcpChecksumSucceeded : 1;
      ULONG  NdisPacketUdpChecksumSucceeded : 1;
      ULONG  NdisPacketIpChecksumSucceeded : 1;
      ULONG  NdisPacketLoopback : 1;
    } Receive;
    ULONG  Value;
  } DUMMYUNIONNAME;
} NDIS_TCP_IP_CHECKSUM_PACKET_INFO, *PNDIS_TCP_IP_CHECKSUM_PACKET_INFO;

typedef struct _NDIS_WAN_CO_FRAGMENT {
  ULONG  Errors;
} NDIS_WAN_CO_FRAGMENT, *PNDIS_WAN_CO_FRAGMENT;

typedef struct _NDIS_WAN_FRAGMENT {
  UCHAR  RemoteAddress[6];
  UCHAR  LocalAddress[6];
} NDIS_WAN_FRAGMENT, *PNDIS_WAN_FRAGMENT;

typedef struct _WAN_CO_LINKPARAMS {
  ULONG  TransmitSpeed; 
  ULONG  ReceiveSpeed; 
  ULONG  SendWindow; 
} WAN_CO_LINKPARAMS, *PWAN_CO_LINKPARAMS;


/* Call Manager */

typedef VOID DDKAPI
(*CM_ACTIVATE_VC_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  CallMgrVcContext,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef NDIS_STATUS DDKAPI
(*CM_ADD_PARTY_HANDLER)(
  IN NDIS_HANDLE  CallMgrVcContext,
  IN OUT PCO_CALL_PARAMETERS  CallParameters,
  IN NDIS_HANDLE  NdisPartyHandle,
  OUT PNDIS_HANDLE  CallMgrPartyContext);

typedef NDIS_STATUS DDKAPI
(*CM_CLOSE_AF_HANDLER)(
  IN NDIS_HANDLE  CallMgrAfContext);

typedef NDIS_STATUS DDKAPI
(*CM_CLOSE_CALL_HANDLER)(
  IN NDIS_HANDLE  CallMgrVcContext,
  IN NDIS_HANDLE  CallMgrPartyContext  OPTIONAL,
  IN PVOID  CloseData  OPTIONAL,
  IN UINT  Size  OPTIONAL);

typedef NDIS_STATUS DDKAPI
(*CM_DEREG_SAP_HANDLER)(
  IN NDIS_HANDLE  CallMgrSapContext);

typedef VOID DDKAPI
(*CM_DEACTIVATE_VC_COMPLETE_HANDLER)(
	IN NDIS_STATUS  Status,
	IN NDIS_HANDLE  CallMgrVcContext);

typedef NDIS_STATUS DDKAPI
(*CM_DROP_PARTY_HANDLER)(
  IN NDIS_HANDLE  CallMgrPartyContext,
  IN PVOID  CloseData  OPTIONAL,
  IN UINT  Size  OPTIONAL);

typedef VOID DDKAPI
(*CM_INCOMING_CALL_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  CallMgrVcContext,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef NDIS_STATUS DDKAPI
(*CM_MAKE_CALL_HANDLER)(
  IN NDIS_HANDLE  CallMgrVcContext,
  IN OUT PCO_CALL_PARAMETERS  CallParameters,
  IN NDIS_HANDLE  NdisPartyHandle	OPTIONAL,
  OUT PNDIS_HANDLE  CallMgrPartyContext  OPTIONAL);

typedef NDIS_STATUS DDKAPI
(*CM_MODIFY_CALL_QOS_HANDLER)(
  IN NDIS_HANDLE  CallMgrVcContext,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef NDIS_STATUS DDKAPI
(*CM_OPEN_AF_HANDLER)(
	IN NDIS_HANDLE  CallMgrBindingContext,
	IN PCO_ADDRESS_FAMILY  AddressFamily,
	IN NDIS_HANDLE  NdisAfHandle,
	OUT PNDIS_HANDLE  CallMgrAfContext);

typedef NDIS_STATUS DDKAPI
(*CM_REG_SAP_HANDLER)(
  IN NDIS_HANDLE  CallMgrAfContext,
  IN PCO_SAP  Sap,
  IN NDIS_HANDLE  NdisSapHandle,
  OUT	PNDIS_HANDLE  CallMgrSapContext);

typedef NDIS_STATUS DDKAPI
(*CO_CREATE_VC_HANDLER)(
  IN NDIS_HANDLE  ProtocolAfContext,
  IN NDIS_HANDLE  NdisVcHandle,
  OUT PNDIS_HANDLE  ProtocolVcContext);

typedef NDIS_STATUS DDKAPI
(*CO_DELETE_VC_HANDLER)(
  IN NDIS_HANDLE  ProtocolVcContext);

typedef VOID DDKAPI
(*CO_REQUEST_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolAfContext  OPTIONAL,
  IN NDIS_HANDLE  ProtocolVcContext  OPTIONAL,
  IN NDIS_HANDLE  ProtocolPartyContext  OPTIONAL,
  IN PNDIS_REQUEST  NdisRequest);

typedef NDIS_STATUS DDKAPI
(*CO_REQUEST_HANDLER)(
  IN NDIS_HANDLE  ProtocolAfContext,
  IN NDIS_HANDLE  ProtocolVcContext  OPTIONAL,
  IN NDIS_HANDLE	ProtocolPartyContext  OPTIONAL,
  IN OUT PNDIS_REQUEST  NdisRequest);

typedef struct _NDIS_CALL_MANAGER_CHARACTERISTICS {
	UCHAR  MajorVersion;
	UCHAR  MinorVersion;
	USHORT  Filler;
	UINT  Reserved;
	CO_CREATE_VC_HANDLER  CmCreateVcHandler;
	CO_DELETE_VC_HANDLER  CmDeleteVcHandler;
	CM_OPEN_AF_HANDLER  CmOpenAfHandler;
	CM_CLOSE_AF_HANDLER	 CmCloseAfHandler;
	CM_REG_SAP_HANDLER  CmRegisterSapHandler;
	CM_DEREG_SAP_HANDLER  CmDeregisterSapHandler;
	CM_MAKE_CALL_HANDLER  CmMakeCallHandler;
	CM_CLOSE_CALL_HANDLER  CmCloseCallHandler;
	CM_INCOMING_CALL_COMPLETE_HANDLER  CmIncomingCallCompleteHandler;
	CM_ADD_PARTY_HANDLER  CmAddPartyHandler;
	CM_DROP_PARTY_HANDLER  CmDropPartyHandler;
	CM_ACTIVATE_VC_COMPLETE_HANDLER  CmActivateVcCompleteHandler;
	CM_DEACTIVATE_VC_COMPLETE_HANDLER  CmDeactivateVcCompleteHandler;
	CM_MODIFY_CALL_QOS_HANDLER  CmModifyCallQoSHandler;
	CO_REQUEST_HANDLER  CmRequestHandler;
	CO_REQUEST_COMPLETE_HANDLER  CmRequestCompleteHandler;
} NDIS_CALL_MANAGER_CHARACTERISTICS, *PNDIS_CALL_MANAGER_CHARACTERISTICS;



/* Call Manager clients */

typedef VOID (*CL_OPEN_AF_COMPLETE_HANDLER)(
  IN NDIS_STATUS Status,
  IN NDIS_HANDLE ProtocolAfContext,
  IN NDIS_HANDLE NdisAfHandle);

typedef VOID DDKAPI
(*CL_CLOSE_AF_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolAfContext);

typedef VOID DDKAPI
(*CL_REG_SAP_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolSapContext,
  IN PCO_SAP  Sap,
  IN NDIS_HANDLE  NdisSapHandle);

typedef VOID DDKAPI
(*CL_DEREG_SAP_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolSapContext);

typedef VOID DDKAPI
(*CL_MAKE_CALL_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN NDIS_HANDLE  NdisPartyHandle  OPTIONAL,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef VOID DDKAPI
(*CL_MODIFY_CALL_QOS_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef VOID DDKAPI
(*CL_CLOSE_CALL_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN NDIS_HANDLE  ProtocolPartyContext  OPTIONAL);

typedef VOID DDKAPI
(*CL_ADD_PARTY_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolPartyContext,
  IN NDIS_HANDLE  NdisPartyHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef VOID DDKAPI
(*CL_DROP_PARTY_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolPartyContext);

typedef NDIS_STATUS DDKAPI
(*CL_INCOMING_CALL_HANDLER)(
  IN NDIS_HANDLE  ProtocolSapContext,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN OUT PCO_CALL_PARAMETERS  CallParameters);

typedef VOID DDKAPI
(*CL_INCOMING_CALL_QOS_CHANGE_HANDLER)(
  IN NDIS_HANDLE  ProtocolVcContext,
  IN PCO_CALL_PARAMETERS  CallParameters);

typedef VOID DDKAPI
(*CL_INCOMING_CLOSE_CALL_HANDLER)(
  IN NDIS_STATUS  CloseStatus,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN PVOID  CloseData  OPTIONAL,
  IN UINT  Size  OPTIONAL);

typedef VOID DDKAPI
(*CL_INCOMING_DROP_PARTY_HANDLER)(
  IN NDIS_STATUS  DropStatus,
  IN NDIS_HANDLE  ProtocolPartyContext,
  IN PVOID  CloseData  OPTIONAL,
  IN UINT  Size  OPTIONAL);

typedef VOID DDKAPI
(*CL_CALL_CONNECTED_HANDLER)(
  IN NDIS_HANDLE  ProtocolVcContext);


typedef struct _NDIS_CLIENT_CHARACTERISTICS {
  UCHAR  MajorVersion;
  UCHAR  MinorVersion;
  USHORT  Filler;
  UINT  Reserved;
  CO_CREATE_VC_HANDLER  ClCreateVcHandler;
  CO_DELETE_VC_HANDLER  ClDeleteVcHandler;
  CO_REQUEST_HANDLER  ClRequestHandler;
  CO_REQUEST_COMPLETE_HANDLER  ClRequestCompleteHandler;
  CL_OPEN_AF_COMPLETE_HANDLER  ClOpenAfCompleteHandler;
  CL_CLOSE_AF_COMPLETE_HANDLER  ClCloseAfCompleteHandler;
  CL_REG_SAP_COMPLETE_HANDLER  ClRegisterSapCompleteHandler;
  CL_DEREG_SAP_COMPLETE_HANDLER  ClDeregisterSapCompleteHandler;
  CL_MAKE_CALL_COMPLETE_HANDLER  ClMakeCallCompleteHandler;
  CL_MODIFY_CALL_QOS_COMPLETE_HANDLER	 ClModifyCallQoSCompleteHandler;
  CL_CLOSE_CALL_COMPLETE_HANDLER  ClCloseCallCompleteHandler;
  CL_ADD_PARTY_COMPLETE_HANDLER  ClAddPartyCompleteHandler;
  CL_DROP_PARTY_COMPLETE_HANDLER  ClDropPartyCompleteHandler;
  CL_INCOMING_CALL_HANDLER  ClIncomingCallHandler;
  CL_INCOMING_CALL_QOS_CHANGE_HANDLER  ClIncomingCallQoSChangeHandler;
  CL_INCOMING_CLOSE_CALL_HANDLER  ClIncomingCloseCallHandler;
  CL_INCOMING_DROP_PARTY_HANDLER  ClIncomingDropPartyHandler;
  CL_CALL_CONNECTED_HANDLER  ClCallConnectedHandler;
} NDIS_CLIENT_CHARACTERISTICS, *PNDIS_CLIENT_CHARACTERISTICS;


/* NDIS protocol structures */

/* Prototypes for NDIS 3.0 protocol characteristics */

typedef VOID DDKAPI
(*OPEN_ADAPTER_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_STATUS  Status,
  IN NDIS_STATUS  OpenErrorStatus);

typedef VOID DDKAPI
(*CLOSE_ADAPTER_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*RESET_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*REQUEST_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNDIS_REQUEST  NdisRequest,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*STATUS_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_STATUS  GeneralStatus,
  IN PVOID  StatusBuffer,
  IN UINT  StatusBufferSize);

typedef VOID DDKAPI
(*STATUS_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext);

typedef VOID DDKAPI
(*SEND_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNDIS_PACKET  Packet,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*WAN_SEND_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNDIS_WAN_PACKET  Packet,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*TRANSFER_DATA_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNDIS_PACKET  Packet,
  IN NDIS_STATUS  Status,
  IN UINT  BytesTransferred);

typedef VOID DDKAPI
(*WAN_TRANSFER_DATA_COMPLETE_HANDLER)(
    VOID);


typedef NDIS_STATUS DDKAPI
(*RECEIVE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_HANDLE  MacReceiveContext,
  IN PVOID  HeaderBuffer,
  IN UINT  HeaderBufferSize,
  IN PVOID  LookAheadBuffer,
  IN UINT  LookaheadBufferSize,
  IN UINT  PacketSize);

typedef NDIS_STATUS DDKAPI
(*WAN_RECEIVE_HANDLER)(
  IN NDIS_HANDLE  NdisLinkHandle,
  IN PUCHAR  Packet,
  IN ULONG  PacketSize);

typedef VOID DDKAPI
(*RECEIVE_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext);


/* Protocol characteristics for NDIS 3.0 protocols */

#define NDIS30_PROTOCOL_CHARACTERISTICS_S \
  UCHAR  MajorNdisVersion; \
  UCHAR  MinorNdisVersion; \
  _ANONYMOUS_UNION union { \
    UINT  Reserved; \
    UINT  Flags; \
  } DUMMYUNIONNAME; \
  OPEN_ADAPTER_COMPLETE_HANDLER  OpenAdapterCompleteHandler; \
  CLOSE_ADAPTER_COMPLETE_HANDLER  CloseAdapterCompleteHandler; \
  _ANONYMOUS_UNION union { \
    SEND_COMPLETE_HANDLER  SendCompleteHandler; \
    WAN_SEND_COMPLETE_HANDLER  WanSendCompleteHandler; \
  } DUMMYUNIONNAME2; \
  _ANONYMOUS_UNION union { \
    TRANSFER_DATA_COMPLETE_HANDLER  TransferDataCompleteHandler; \
    WAN_TRANSFER_DATA_COMPLETE_HANDLER  WanTransferDataCompleteHandler; \
  } DUMMYUNIONNAME3; \
  RESET_COMPLETE_HANDLER  ResetCompleteHandler; \
  REQUEST_COMPLETE_HANDLER  RequestCompleteHandler; \
  _ANONYMOUS_UNION union { \
    RECEIVE_HANDLER	 ReceiveHandler; \
    WAN_RECEIVE_HANDLER  WanReceiveHandler; \
  } DUMMYUNIONNAME4; \
  RECEIVE_COMPLETE_HANDLER  ReceiveCompleteHandler; \
  STATUS_HANDLER  StatusHandler; \
  STATUS_COMPLETE_HANDLER  StatusCompleteHandler; \
  NDIS_STRING  Name;

typedef struct _NDIS30_PROTOCOL_CHARACTERISTICS {
  NDIS30_PROTOCOL_CHARACTERISTICS_S
} NDIS30_PROTOCOL_CHARACTERISTICS, *PNDIS30_PROTOCOL_CHARACTERISTICS;


/* Prototypes for NDIS 4.0 protocol characteristics */

typedef INT DDKAPI
(*RECEIVE_PACKET_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNDIS_PACKET  Packet);

typedef VOID DDKAPI
(*BIND_HANDLER)(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  BindContext,
  IN PNDIS_STRING  DeviceName,
  IN PVOID  SystemSpecific1,
  IN PVOID  SystemSpecific2);

typedef VOID DDKAPI
(*UNBIND_HANDLER)(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_HANDLE  UnbindContext);

typedef NDIS_STATUS DDKAPI
(*PNP_EVENT_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNET_PNP_EVENT  NetPnPEvent);

typedef VOID DDKAPI
(*UNLOAD_PROTOCOL_HANDLER)(
  VOID);


/* Protocol characteristics for NDIS 4.0 protocols */

#ifdef __cplusplus

#define NDIS40_PROTOCOL_CHARACTERISTICS_S \
  NDIS30_PROTOCOL_CHARACTERISTICS  Ndis30Chars; \
  RECEIVE_PACKET_HANDLER  ReceivePacketHandler; \
  BIND_HANDLER  BindAdapterHandler; \
  UNBIND_HANDLER  UnbindAdapterHandler; \
  PNP_EVENT_HANDLER  PnPEventHandler; \
  UNLOAD_PROTOCOL_HANDLER  UnloadHandler; 

#else /* !__cplusplus */

#define NDIS40_PROTOCOL_CHARACTERISTICS_S \
  NDIS30_PROTOCOL_CHARACTERISTICS_S \
  RECEIVE_PACKET_HANDLER  ReceivePacketHandler; \
  BIND_HANDLER  BindAdapterHandler; \
  UNBIND_HANDLER  UnbindAdapterHandler; \
  PNP_EVENT_HANDLER  PnPEventHandler; \
  UNLOAD_PROTOCOL_HANDLER  UnloadHandler; 

#endif /* __cplusplus */

typedef struct _NDIS40_PROTOCOL_CHARACTERISTICS {
  NDIS40_PROTOCOL_CHARACTERISTICS_S
} NDIS40_PROTOCOL_CHARACTERISTICS, *PNDIS40_PROTOCOL_CHARACTERISTICS;

/* Prototypes for NDIS 5.0 protocol characteristics */

typedef VOID DDKAPI
(*CO_SEND_COMPLETE_HANDLER)(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN PNDIS_PACKET  Packet);

typedef VOID DDKAPI
(*CO_STATUS_HANDLER)(
	IN NDIS_HANDLE  ProtocolBindingContext,
	IN NDIS_HANDLE  ProtocolVcContext  OPTIONAL,
	IN NDIS_STATUS  GeneralStatus,
	IN PVOID  StatusBuffer,
	IN UINT  StatusBufferSize);

typedef UINT DDKAPI
(*CO_RECEIVE_PACKET_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN PNDIS_PACKET  Packet);

typedef VOID DDKAPI
(*CO_AF_REGISTER_NOTIFY_HANDLER)(
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PCO_ADDRESS_FAMILY  AddressFamily);

#ifdef __cplusplus \

#define NDIS50_PROTOCOL_CHARACTERISTICS_S \
  NDIS40_PROTOCOL_CHARACTERISTICS  Ndis40Chars; \
  PVOID  ReservedHandlers[4]; \
  CO_SEND_COMPLETE_HANDLER  CoSendCompleteHandler; \
  CO_STATUS_HANDLER  CoStatusHandler; \
  CO_RECEIVE_PACKET_HANDLER  CoReceivePacketHandler; \
  CO_AF_REGISTER_NOTIFY_HANDLER  CoAfRegisterNotifyHandler;

#else /* !__cplusplus */

#define NDIS50_PROTOCOL_CHARACTERISTICS_S \
  NDIS40_PROTOCOL_CHARACTERISTICS_S \
  PVOID  ReservedHandlers[4]; \
  CO_SEND_COMPLETE_HANDLER  CoSendCompleteHandler; \
  CO_STATUS_HANDLER  CoStatusHandler; \
  CO_RECEIVE_PACKET_HANDLER  CoReceivePacketHandler; \
  CO_AF_REGISTER_NOTIFY_HANDLER  CoAfRegisterNotifyHandler;

#endif /* !__cplusplus */

typedef struct _NDIS50_PROTOCOL_CHARACTERISTICS {
  NDIS50_PROTOCOL_CHARACTERISTICS_S
} NDIS50_PROTOCOL_CHARACTERISTICS, *PNDIS50_PROTOCOL_CHARACTERISTICS;

#if defined(NDIS50) || defined(NDIS51)
typedef struct _NDIS_PROTOCOL_CHARACTERISTICS {
  NDIS50_PROTOCOL_CHARACTERISTICS_S;
} NDIS_PROTOCOL_CHARACTERISTICS, *PNDIS_PROTOCOL_CHARACTERISTICS;
#elif defined(NDIS40)
typedef struct _NDIS_PROTOCOL_CHARACTERISTICS {
  NDIS40_PROTOCOL_CHARACTERISTICS_S;
} NDIS_PROTOCOL_CHARACTERISTICS, *PNDIS_PROTOCOL_CHARACTERISTICS;
#elif defined(NDIS30)
typedef struct _NDIS_PROTOCOL_CHARACTERISTICS {
  NDIS30_PROTOCOL_CHARACTERISTICS_S
} NDIS_PROTOCOL_CHARACTERISTICS, *PNDIS_PROTOCOL_CHARACTERISTICS;
#else
#error Define an NDIS version
#endif /* NDIS30 */



/* Buffer management routines */

NDISAPI
VOID
DDKAPI
NdisAllocateBuffer(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_BUFFER  *Buffer,
  IN NDIS_HANDLE  PoolHandle,
  IN PVOID  VirtualAddress,
  IN UINT  Length);


NDISAPI
VOID
DDKAPI
NdisAllocateBufferPool(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_HANDLE  PoolHandle,
  IN UINT  NumberOfDescriptors);

NDISAPI
VOID
DDKAPI
NdisAllocatePacket(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_PACKET  *Packet,
  IN NDIS_HANDLE  PoolHandle);

NDISAPI
VOID
DDKAPI
NdisAllocatePacketPool(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_HANDLE  PoolHandle,
  IN UINT  NumberOfDescriptors,
  IN UINT  ProtocolReservedLength);

NDISAPI
VOID
DDKAPI
NdisCopyBuffer(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_BUFFER  *Buffer,
  IN NDIS_HANDLE  PoolHandle,
  IN PVOID  MemoryDescriptor,
  IN UINT  Offset,
  IN UINT  Length);

NDISAPI
VOID
DDKAPI
NdisCopyFromPacketToPacket(
  IN PNDIS_PACKET  Destination,
  IN UINT  DestinationOffset,
  IN UINT  BytesToCopy,
  IN PNDIS_PACKET  Source,
  IN UINT  SourceOffset,
  OUT PUINT  BytesCopied);

NDISAPI
VOID
DDKAPI
NdisDprAllocatePacket(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_PACKET  *Packet,
  IN NDIS_HANDLE  PoolHandle);

NDISAPI
VOID
DDKAPI
NdisDprAllocatePacketNonInterlocked(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_PACKET  *Packet,
  IN NDIS_HANDLE  PoolHandle);

NDISAPI
VOID
DDKAPI
NdisDprFreePacket(
  IN PNDIS_PACKET  Packet);

NDISAPI
VOID
DDKAPI
NdisDprFreePacketNonInterlocked(
  IN PNDIS_PACKET  Packet);

NDISAPI
VOID
DDKAPI
NdisFreeBufferPool(
  IN NDIS_HANDLE  PoolHandle);

NDISAPI
VOID
DDKAPI
NdisFreePacket(
  IN PNDIS_PACKET  Packet);

NDISAPI
VOID
DDKAPI
NdisFreePacketPool(
  IN NDIS_HANDLE  PoolHandle);

NDISAPI
VOID
DDKAPI
NdisReturnPackets(
  IN PNDIS_PACKET  *PacketsToReturn,
  IN UINT  NumberOfPackets);

NDISAPI
VOID
DDKAPI
NdisUnchainBufferAtBack(
  IN OUT PNDIS_PACKET  Packet,
  OUT PNDIS_BUFFER  *Buffer);

NDISAPI
VOID
DDKAPI
NdisUnchainBufferAtFront(
  IN OUT PNDIS_PACKET  Packet,
  OUT PNDIS_BUFFER  *Buffer);

NDISAPI
VOID
DDKAPI
NdisAdjustBufferLength(
  IN PNDIS_BUFFER  Buffer,
  IN UINT  Length);

NDISAPI
ULONG
DDKAPI
NdisBufferLength(
  IN PNDIS_BUFFER  Buffer);

NDISAPI
PVOID
DDKAPI
NdisBufferVirtualAddress(
  IN PNDIS_BUFFER  Buffer);

NDISAPI
ULONG
DDKAPI
NDIS_BUFFER_TO_SPAN_PAGES(
  IN PNDIS_BUFFER  Buffer);

NDISAPI
VOID
DDKAPI
NdisFreeBuffer(
  IN PNDIS_BUFFER  Buffer);

NDISAPI
VOID
DDKAPI
NdisGetBufferPhysicalArraySize(
  IN PNDIS_BUFFER  Buffer,
  OUT PUINT  ArraySize);

NDISAPI
VOID
DDKAPI
NdisGetFirstBufferFromPacket(
  IN PNDIS_PACKET  _Packet,
  OUT PNDIS_BUFFER  *_FirstBuffer,
  OUT PVOID  *_FirstBufferVA,
  OUT PUINT  _FirstBufferLength,
  OUT PUINT  _TotalBufferLength);

NDISAPI
VOID
DDKAPI
NdisQueryBuffer(
  IN PNDIS_BUFFER  Buffer,
  OUT PVOID  *VirtualAddress OPTIONAL,
  OUT PUINT  Length);

NDISAPI
VOID
DDKAPI
NdisQueryBufferOffset(
  IN PNDIS_BUFFER  Buffer,
  OUT PUINT  Offset,
  OUT PUINT  Length);

NDISAPI
VOID
DDKAPI
NdisFreeBuffer(
  IN PNDIS_BUFFER  Buffer);


/*
 * VOID
 * NdisGetBufferPhysicalArraySize(
 *   IN PNDIS_BUFFER  Buffer,
 *   OUT PUINT  ArraySize);
 */
#define NdisGetBufferPhysicalArraySize(Buffer,        \
                                       ArraySize)     \
{                                                     \
  (*(ArraySize) = NDIS_BUFFER_TO_SPAN_PAGES(Buffer))  \
}


/*
 * VOID
 * NdisGetFirstBufferFromPacket(
 *   IN PNDIS_PACKET  _Packet,
 *   OUT PNDIS_BUFFER  * _FirstBuffer,
 *   OUT PVOID  * _FirstBufferVA,
 *   OUT PUINT  _FirstBufferLength,
 *   OUT PUINT  _TotalBufferLength)
 */
#define	NdisGetFirstBufferFromPacket(_Packet,             \
                                     _FirstBuffer,        \
                                     _FirstBufferVA,      \
                                     _FirstBufferLength,  \
                                     _TotalBufferLength)  \
{                                                         \
  PNDIS_BUFFER _Buffer;                                   \
                                                          \
  _Buffer         = (_Packet)->Private.Head;              \
  *(_FirstBuffer) = _Buffer;                              \
  if (_Buffer != NULL)                                    \
    {                                                     \
	    *(_FirstBufferVA)     = MmGetSystemAddressForMdl(_Buffer);  \
	    *(_FirstBufferLength) = MmGetMdlByteCount(_Buffer);	        \
	    _Buffer = _Buffer->Next;                                    \
		  *(_TotalBufferLength) = *(_FirstBufferLength);              \
		  while (_Buffer != NULL) {                                   \
		    *(_TotalBufferLength) += MmGetMdlByteCount(_Buffer);      \
		    _Buffer = _Buffer->Next;                                  \
		  }                                                           \
    }                             \
  else                            \
    {                             \
      *(_FirstBufferVA) = 0;      \
      *(_FirstBufferLength) = 0;  \
      *(_TotalBufferLength) = 0;  \
    } \
}

/*
 * VOID
 * NdisQueryBuffer(
 *   IN PNDIS_BUFFER  Buffer,
 *   OUT PVOID  *VirtualAddress OPTIONAL,
 *   OUT PUINT  Length)
 */
#define NdisQueryBuffer(Buffer,         \
                        VirtualAddress, \
                        Length)         \
{                                       \
	if (VirtualAddress)                   \
		*((PVOID*)VirtualAddress) = MmGetSystemAddressForMdl(Buffer); \
                                        \
	*((PUINT)Length) = MmGetMdlByteCount(Buffer); \
}


/*
 * VOID
 * NdisQueryBufferOffset(
 *   IN PNDIS_BUFFER  Buffer,
 *   OUT PUINT  Offset,
 *   OUT PUINT  Length);
 */
#define NdisQueryBufferOffset(Buffer,             \
                              Offset,             \
                              Length)             \
{                                                 \
  *((PUINT)Offset) = MmGetMdlByteOffset(Buffer);  \
  *((PUINT)Length) = MmGetMdlByteCount(Buffer);   \
}


/*
 * PVOID
 * NDIS_BUFFER_LINKAGE(
 *   IN PNDIS_BUFFER  Buffer);
 */
#define NDIS_BUFFER_LINKAGE(Buffer)(Buffer)->Next;


/*
 * VOID
 * NdisChainBufferAtBack(
 *   IN OUT PNDIS_PACKET  Packet,
 *   IN OUT PNDIS_BUFFER  Buffer)
 */
#define NdisChainBufferAtBack(Packet,           \
                              Buffer)           \
{                                               \
	PNDIS_BUFFER NdisBuffer = (Buffer);           \
                                                \
	while (NdisBuffer->Next != NULL)              \
   NdisBuffer = NdisBuffer->Next;               \
	                                              \
	NdisBuffer->Next = NULL;                      \
	                                              \
	if ((Packet)->Private.Head != NULL)           \
    (Packet)->Private.Tail->Next = (Buffer);    \
	else                                          \
    (Packet)->Private.Head = (Buffer);          \
	                                              \
	(Packet)->Private.Tail = NdisBuffer;          \
	(Packet)->Private.ValidCounts = FALSE;        \
}


/*
 * VOID
 * NdisChainBufferAtFront(
 *   IN OUT PNDIS_PACKET  Packet,
 *   IN OUT PNDIS_BUFFER  Buffer)
 */
#define NdisChainBufferAtFront(Packet,        \
                               Buffer)        \
{                                             \
	PNDIS_BUFFER _NdisBuffer = (Buffer);        \
                                              \
  while (_NdisBuffer->Next != NULL)           \
    _NdisBuffer = _NdisBuffer->Next;          \
                                              \
  if ((Packet)->Private.Head == NULL)         \
    (Packet)->Private.Tail = _NdisBuffer;     \
                                              \
	_NdisBuffer->Next = (Packet)->Private.Head; \
	(Packet)->Private.Head = (Buffer);          \
	(Packet)->Private.ValidCounts = FALSE;      \
}


/*
 * VOID
 * NdisGetNextBuffer(
 *   IN PNDIS_BUFFER  CurrentBuffer,
 *   OUT PNDIS_BUFFER  * NextBuffer)
 */
#define NdisGetNextBuffer(CurrentBuffer,  \
                          NextBuffer)     \
{                                         \
  *(NextBuffer) = (CurrentBuffer)->Next;  \
}


/*
 * UINT
 * NdisGetPacketFlags(
 *   IN PNDIS_PACKET  Packet); 
 */
#define NdisGetPacketFlags(Packet)(Packet)->Private.Flags;


/*
 * VOID
 * NdisClearPacketFlags(
 *   IN PNDIS_PACKET  Packet,
 *   IN UINT  Flags);
 */
#define NdisClearPacketFlags(Packet, Flags) \
  (Packet)->Private.Flags &= ~(Flags)


/*
 * VOID
 * NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(
 *   IN PNDIS_PACKET    Packet,
 *   IN PPVOID          pMediaSpecificInfo,
 *   IN PUINT           pSizeMediaSpecificInfo);
 */
#define NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(_Packet,                                  \
                                            _pMediaSpecificInfo,                      \
                                            _pSizeMediaSpecificInfo)                  \
{                                                                                     \
  if (!((_Packet)->Private.NdisPacketFlags & fPACKET_ALLOCATED_BY_NDIS) ||            \
      !((_Packet)->Private.NdisPacketFlags & fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO))   \
	  {                                                                                 \
	    *(_pMediaSpecificInfo) = NULL;                                                  \
	    *(_pSizeMediaSpecificInfo) = 0;                                                 \
	  }                                                                                 \
  else                                                                                \
	  {                                                                                 \
	    *(_pMediaSpecificInfo) = ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +           \
        (_Packet)->Private.NdisPacketOobOffset))->MediaSpecificInformation;           \
	    *(_pSizeMediaSpecificInfo) = ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +       \
	      (_Packet)->Private.NdisPacketOobOffset))->SizeMediaSpecificInfo;              \
	  }                                                                                 \
}


/*
 * ULONG
 * NDIS_GET_PACKET_PROTOCOL_TYPE(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_PROTOCOL_TYPE(_Packet) \
  ((_Packet)->Private.Flags & NDIS_PROTOCOL_ID_MASK)

/*
 * ULONG
 * NDIS_GET_PACKET_HEADER_SIZE(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_HEADER_SIZE(_Packet) \
	((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) + \
	(_Packet)->Private.NdisPacketOobOffset))->HeaderSize


/*
 * NDIS_STATUS
 * NDIS_GET_PACKET_STATUS(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_STATUS(_Packet) \
	((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) + \
	(_Packet)->Private.NdisPacketOobOffset))->Status


/*
 * ULONGLONG
 * NDIS_GET_PACKET_TIME_RECEIVED(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_TIME_RECEIVED(_Packet)  \
	((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +  \
	(_Packet)->Private.NdisPacketOobOffset))->TimeReceived


/*
 * ULONGLONG
 * NDIS_GET_PACKET_TIME_SENT(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_TIME_SENT(_Packet)      \
	((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +  \
	(_Packet)->Private.NdisPacketOobOffset))->TimeSent


/*
 * ULONGLONG
 * NDIS_GET_PACKET_TIME_TO_SEND(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_GET_PACKET_TIME_TO_SEND(_Packet)   \
	((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +  \
	(_Packet)->Private.NdisPacketOobOffset))->TimeToSend


/*
 * PNDIS_PACKET_OOB_DATA
 * NDIS_OOB_DATA_FROM_PACKET(
 *   IN PNDIS_PACKET  Packet);
 */
#define NDIS_OOB_DATA_FROM_PACKET(_Packet)    \
  (PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) + \
  (_Packet)->Private.NdisPacketOobOffset)

 
/*
 * VOID
 * NdisQueryPacket(
 *   IN PNDIS_PACKET  Packet,
 *   OUT PUINT  PhysicalBufferCount  OPTIONAL,
 *   OUT PUINT  BufferCount  OPTIONAL,
 *   OUT PNDIS_BUFFER  *FirstBuffer  OPTIONAL,
 *   OUT PUINT  TotalPacketLength  OPTIONAL);
 */
#define NdisQueryPacket(Packet,                                           \
                        PhysicalBufferCount,                              \
                        BufferCount,                                      \
                        FirstBuffer,                                      \
                        TotalPacketLength)                                \
{                                                                         \
  if (FirstBuffer)                                                        \
    *((PNDIS_BUFFER*)FirstBuffer) = (Packet)->Private.Head;               \
  if ((TotalPacketLength) || (BufferCount) || (PhysicalBufferCount))      \
  {                                                                       \
    if (!(Packet)->Private.ValidCounts) {                                 \
      UINT _Offset;                                                       \
      UINT _PacketLength;                                                 \
      PNDIS_BUFFER _NdisBuffer;                                           \
      UINT _PhysicalBufferCount = 0;                                      \
      UINT _TotalPacketLength   = 0;                                      \
      UINT _Count               = 0;                                      \
                                                                          \
      for (_NdisBuffer = (Packet)->Private.Head;                          \
        _NdisBuffer != (PNDIS_BUFFER)NULL;                                \
        _NdisBuffer = _NdisBuffer->Next)                                  \
      {                                                                   \
        _PhysicalBufferCount += NDIS_BUFFER_TO_SPAN_PAGES(_NdisBuffer);   \
        NdisQueryBufferOffset(_NdisBuffer, &_Offset, &_PacketLength);     \
        _TotalPacketLength += _PacketLength;                              \
        _Count++;                                                         \
      }                                                                   \
      (Packet)->Private.PhysicalCount = _PhysicalBufferCount;             \
      (Packet)->Private.TotalLength   = _TotalPacketLength;               \
      (Packet)->Private.Count         = _Count;                           \
      (Packet)->Private.ValidCounts   = TRUE;                             \
	}                                                                       \
                                                                          \
  if (PhysicalBufferCount)                                                \
      *((PUINT)PhysicalBufferCount) = (Packet)->Private.PhysicalCount;    \
                                                                          \
  if (BufferCount)                                                        \
      *((PUINT)BufferCount) = (Packet)->Private.Count;                    \
                                                                          \
  if (TotalPacketLength)                                                  \
      *((PUINT)TotalPacketLength) = (Packet)->Private.TotalLength;        \
  } \
}

/*
 * VOID
 * NdisQueryPacketLength(
 *   IN PNDIS_PACKET  Packet,
 *   OUT PUINT  PhysicalBufferCount  OPTIONAL,
 *   OUT PUINT  BufferCount  OPTIONAL,
 *   OUT PNDIS_BUFFER  *FirstBuffer  OPTIONAL,
 *   OUT PUINT  TotalPacketLength  OPTIONAL);
 */
#define NdisQueryPacketLength(Packet,                                     \
                              TotalPacketLength)                          \
{                                                                         \
  if ((TotalPacketLength))                                                \
  {                                                                       \
    if (!(Packet)->Private.ValidCounts) {                                 \
      UINT _Offset;                                                       \
      UINT _PacketLength;                                                 \
      PNDIS_BUFFER _NdisBuffer;                                           \
      UINT _PhysicalBufferCount = 0;                                      \
      UINT _TotalPacketLength   = 0;                                      \
      UINT _Count               = 0;                                      \
                                                                          \
      for (_NdisBuffer = (Packet)->Private.Head;                          \
        _NdisBuffer != (PNDIS_BUFFER)NULL;                                \
        _NdisBuffer = _NdisBuffer->Next)                                  \
      {                                                                   \
        _PhysicalBufferCount += NDIS_BUFFER_TO_SPAN_PAGES(_NdisBuffer);   \
        NdisQueryBufferOffset(_NdisBuffer, &_Offset, &_PacketLength);     \
        _TotalPacketLength += _PacketLength;                              \
        _Count++;                                                         \
      }                                                                   \
      (Packet)->Private.PhysicalCount = _PhysicalBufferCount;             \
      (Packet)->Private.TotalLength   = _TotalPacketLength;               \
      (Packet)->Private.Count         = _Count;                           \
      (Packet)->Private.ValidCounts   = TRUE;                             \
  }                                                                       \
                                                                          \
  if (TotalPacketLength)                                                  \
      *((PUINT)TotalPacketLength) = (Packet)->Private.TotalLength;        \
  } \
}


/*
 * VOID
 * NdisRecalculatePacketCounts(
 *   IN OUT  PNDIS_PACKET  Packet);
 */
#define NdisRecalculatePacketCounts(Packet)       \
{                                                 \
  PNDIS_BUFFER _Buffer = (Packet)->Private.Head;  \
  if (_Buffer != NULL)                            \
  {                                               \
      while (_Buffer->Next != NULL)               \
      {                                           \
          ´_Buffer = _Buffer->Next;               \
      }                                           \
      (Packet)->Private.Tail = _Buffer;           \
  }                                               \
  (Packet)->Private.ValidCounts = FALSE;          \
}


/*
 * VOID
 * NdisReinitializePacket(
 *   IN OUT  PNDIS_PACKET  Packet);
 */
#define NdisReinitializePacketCounts(Packet)    \
{                                               \
	(Packet)->Private.Head = (PNDIS_BUFFER)NULL;  \
	(Packet)->Private.ValidCounts = FALSE;        \
}


/*
 * VOID
 * NdisSetPacketFlags(
 *   IN PNDIS_PACKET  Packet,
 *   IN UINT  Flags); 
 */
#define NdisSetPacketFlags(Packet, Flags) \
  (Packet)->Private.Flags |= (Flags);


/*
 * VOID
 * NDIS_SET_PACKET_HEADER_SIZE(
 *   IN PNDIS_PACKET  Packet,
 *   IN UINT  HdrSize);
 */
#define NDIS_SET_PACKET_HEADER_SIZE(_Packet, _HdrSize)              \
  ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +                      \
  (_Packet)->Private.NdisPacketOobOffset))->HeaderSize = (_HdrSize)


/*
 * VOID
 * NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(
 *   IN PNDIS_PACKET  Packet,
 *   IN PVOID  MediaSpecificInfo,
 *   IN UINT  SizeMediaSpecificInfo);
 */
#define NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(_Packet,                      \
                                            _MediaSpecificInfo,           \
                                            _SizeMediaSpecificInfo)       \
{                                                                         \
  if ((_Packet)->Private.NdisPacketFlags & fPACKET_ALLOCATED_BY_NDIS)     \
	  {                                                                     \
      (_Packet)->Private.NdisPacketFlags |= fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO; \
      ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +                        \
        (_Packet)->Private.NdisPacketOobOffset))->MediaSpecificInformation = \
          (_MediaSpecificInfo);                                           \
      ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +                        \
        (_Packet)->Private.NdisPacketOobOffset))->SizeMediaSpecificInfo = \
          (_SizeMediaSpecificInfo);                                       \
	  }                                                                     \
}


/*
 * VOID
 * NDIS_SET_PACKET_STATUS(
 *   IN PNDIS_PACKET    Packet,
 *   IN NDIS_STATUS     Status);
 */
#define NDIS_SET_PACKET_STATUS(_Packet, _Status)  \
  ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +    \
  (_Packet)->Private.NdisPacketOobOffset))->Status = (_Status)


/*
 * VOID
 * NDIS_SET_PACKET_TIME_RECEIVED(
 *   IN PNDIS_PACKET  Packet,
 *   IN ULONGLONG  TimeReceived);
 */
#define NDIS_SET_PACKET_TIME_RECEIVED(_Packet, _TimeReceived) \
  ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +                \
  (_Packet)->Private.NdisPacketOobOffset))->TimeReceived = (_TimeReceived)


/*
 * VOID
 * NDIS_SET_PACKET_TIME_SENT(
 *   IN PNDIS_PACKET  Packet,
 *   IN ULONGLONG  TimeSent);
 */
#define NDIS_SET_PACKET_TIME_SENT(_Packet, _TimeSent) \
  ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +        \
  (_Packet)->Private.NdisPacketOobOffset))->TimeSent = (_TimeSent)


/*
 * VOID
 * NDIS_SET_PACKET_TIME_TO_SEND(
 *   IN PNDIS_PACKET  Packet,
 *   IN ULONGLONG  TimeToSend);
 */
#define NDIS_SET_PACKET_TIME_TO_SEND(_Packet, _TimeToSend)  \
  ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +              \
  (_Packet)->Private.NdisPacketOobOffset))->TimeToSend = (_TimeToSend)


/*
 * VOID
 * NdisSetSendFlags(
 *   IN PNDIS_PACKET  Packet,
 *   IN UINT  Flags);
 */
#define NdisSetSendFlags(_Packet,_Flags)(_Packet)->Private.Flags = (_Flags)



/* Memory management routines */

/*
 * VOID
 * NdisCreateLookaheadBufferFromSharedMemory(
 *   IN PVOID  pSharedMemory,
 *   IN UINT  LookaheadLength,
 *   OUT PVOID  *pLookaheadBuffer)
 */
#define NdisCreateLookaheadBufferFromSharedMemory(_pSharedMemory,     \
                                                  _LookaheadLength,   \
                                                  _pLookaheadBuffer)  \
  ((*(_pLookaheadBuffer)) = (_pSharedMemory))

/*
 * VOID
 * NdisDestroyLookaheadBufferFromSharedMemory(
 *   IN PVOID  pLookaheadBuffer)
 */
#define NdisDestroyLookaheadBufferFromSharedMemory(_pLookaheadBuffer)

#if defined(i386)

/*
 * VOID
 * NdisMoveFromMappedMemory(
 *   OUT PVOID  Destination,
 *   IN PVOID  Source,
 *   IN ULONG  Length);
 */
#define NdisMoveFromMappedMemory(Destination, Source, Length) \
  NdisMoveMappedMemory(Destination, Source, Length)

/*
 * VOID
 * NdisMoveMappedMemory(
 *   OUT PVOID  Destination,
 *   IN PVOID  Source,
 *   IN ULONG  Length);
 */
#define NdisMoveMappedMemory(Destination, Source, Length) \
  RtlCopyMemory(Destination, Source, Length)

/*
 * VOID
 * NdisMoveToMappedMemory(
 *   OUT PVOID  Destination,
 *   IN PVOID  Source,
 *   IN ULONG  Length);
 */
#define NdisMoveToMappedMemory(Destination, Source, Length) \
  NdisMoveMappedMemory(Destination, Source, Length)

#endif /* i386 */

/*
 * VOID
 * NdisMUpdateSharedMemory(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN ULONG  Length,
 *   IN PVOID  VirtualAddress,
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress);
 */
#define NdisMUpdateSharedMemory(_H, _L, _V, _P) \
  NdisUpdateSharedMemory(_H, _L, _V, _P)

NDISAPI
NDIS_STATUS
DDKAPI
NdisAllocateMemory(
  OUT  PVOID  *VirtualAddress,
  IN UINT  Length,
  IN UINT  MemoryFlags,
  IN NDIS_PHYSICAL_ADDRESS  HighestAcceptableAddress);

NDISAPI
VOID
DDKAPI
NdisFreeMemory(
  IN PVOID  VirtualAddress,
  IN UINT  Length,
  IN UINT  MemoryFlags);

NDISAPI
VOID
DDKAPI
NdisImmediateReadSharedMemory(
  IN NDIS_HANDLE WrapperConfigurationContext,
  IN ULONG       SharedMemoryAddress,
  OUT PUCHAR      Buffer,
  IN ULONG       Length);

NDISAPI
VOID
DDKAPI
NdisImmediateWriteSharedMemory(
  IN NDIS_HANDLE WrapperConfigurationContext,
  IN ULONG       SharedMemoryAddress,
  IN PUCHAR      Buffer,
  IN ULONG       Length);

NDISAPI
VOID
DDKAPI
NdisMAllocateSharedMemory(
  IN	NDIS_HANDLE  MiniportAdapterHandle,
  IN	ULONG  Length,
  IN	BOOLEAN  Cached,
  OUT	 PVOID  *VirtualAddress,
  OUT	 PNDIS_PHYSICAL_ADDRESS  PhysicalAddress);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMAllocateSharedMemoryAsync(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN ULONG  Length,
  IN BOOLEAN  Cached,
  IN PVOID  Context);

#if defined(NDIS50)

#define NdisUpdateSharedMemory(NdisAdapterHandle, \
                               Length,            \
                               VirtualAddress,    \
                               PhysicalAddress)

#else

NDISAPI
VOID
DDKAPI
NdisUpdateSharedMemory(
  IN NDIS_HANDLE             NdisAdapterHandle,
  IN ULONG                   Length,
  IN PVOID                   VirtualAddress,
  IN NDIS_PHYSICAL_ADDRESS   PhysicalAddress);

#endif /* defined(NDIS50) */

/*
 * ULONG
 * NdisGetPhysicalAddressHigh(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress);
 */
#define NdisGetPhysicalAddressHigh(PhysicalAddress) \
  ((PhysicalAddress).HighPart)

/*
 * VOID
 * NdisSetPhysicalAddressHigh(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress,
 *   IN ULONG  Value);
 */
#define NdisSetPhysicalAddressHigh(PhysicalAddress, Value) \
  ((PhysicalAddress).HighPart) = (Value)

/*
 * ULONG
 * NdisGetPhysicalAddressLow(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress);
 */
#define NdisGetPhysicalAddressLow(PhysicalAddress) \
  ((PhysicalAddress).LowPart)


/*
 * VOID
 * NdisSetPhysicalAddressLow(
 *   IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress,
 *   IN ULONG  Value);
 */
#define NdisSetPhysicalAddressLow(PhysicalAddress, Value) \
  ((PhysicalAddress).LowPart) = (Value)

/*
 * VOID
 * NDIS_PHYSICAL_ADDRESS_CONST(
 *   IN ULONG  Low,
 *   IN LONG  High); 
 */
#define NDIS_PHYSICAL_ADDRESS_CONST(Low, High)  \
    { {(ULONG)(Low), (LONG)(High)} }

/*
 * ULONG
 * NdisEqualMemory(
 *  IN CONST VOID  *Source1,
 *  IN CONST VOID  *Source2,
 *  IN ULONG  Length);
 */
#define NdisEqualMemory(Source1, Source2, Length) \
  RtlEqualMemory(Source1, Source2, Length)

/*
 * VOID
 * NdisFillMemory(
 *   IN PVOID  Destination,
 *   IN ULONG  Length,
 *   IN UCHAR  Fill);
 */
#define NdisFillMemory(Destination, Length, Fill) \
  RtlFillMemory(Destination, Length, Fill)

/*
 * VOID
 * NdisZeroMappedMemory(
 *   IN PVOID  Destination,
 *   IN ULONG  Length);
 */
#define NdisZeroMappedMemory(Destination, Length) \
  RtlZeroMemory(Destination, Length)

/*
 * VOID
 * NdisMoveMemory(
 *   OUT  PVOID  Destination,
 *   IN PVOID  Source,
 *   IN ULONG  Length);
 */
#define NdisMoveMemory(Destination, Source, Length) \
  RtlCopyMemory(Destination, Source, Length)


/*
 * VOID
 * NdisRetrieveUlong(
 *   IN PULONG  DestinationAddress,
 *   IN PULONG  SourceAddress);
 */
#define NdisRetrieveUlong(DestinationAddress, SourceAddress) \
  RtlRetrieveUlong(DestinationAddress, SourceAddress)


/*
 * VOID
 * NdisStoreUlong(
 *   IN PULONG  DestinationAddress,
 *   IN ULONG  Value); 
 */
#define NdisStoreUlong(DestinationAddress, Value) \
  RtlStoreUlong(DestinationAddress, Value)


/*
 * VOID
 * NdisZeroMemory(
 *   IN PVOID  Destination,
 *   IN ULONG  Length)
 */
#define NdisZeroMemory(Destination, Length) \
  RtlZeroMemory(Destination, Length)



/* Configuration routines */

NDISAPI
VOID
DDKAPI
NdisOpenConfiguration(
  OUT  PNDIS_STATUS  Status,
  OUT  PNDIS_HANDLE  ConfigurationHandle,
  IN NDIS_HANDLE  WrapperConfigurationContext);

NDISAPI
VOID
DDKAPI
NdisReadNetworkAddress(
  OUT PNDIS_STATUS  Status,
  OUT PVOID  *NetworkAddress,
  OUT PUINT  NetworkAddressLength,
  IN NDIS_HANDLE  ConfigurationHandle);

NDISAPI
VOID
DDKAPI
NdisReadEisaSlotInformation(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  WrapperConfigurationContext,
  OUT PUINT  SlotNumber,
  OUT PNDIS_EISA_FUNCTION_INFORMATION  EisaData);

NDISAPI
VOID
DDKAPI
NdisReadEisaSlotInformationEx(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  WrapperConfigurationContext,
  OUT PUINT  SlotNumber,
  OUT PNDIS_EISA_FUNCTION_INFORMATION  *EisaData,
  OUT PUINT  NumberOfFunctions);

NDISAPI
ULONG
DDKAPI
NdisReadPciSlotInformation(
  IN NDIS_HANDLE  NdisAdapterHandle,
  IN ULONG  SlotNumber,
  IN ULONG  Offset,
  IN PVOID  Buffer,
  IN ULONG  Length);

NDISAPI
ULONG 
DDKAPI
NdisWritePciSlotInformation(
  IN NDIS_HANDLE  NdisAdapterHandle,
  IN ULONG  SlotNumber,
  IN ULONG  Offset,
  IN PVOID  Buffer,
  IN ULONG  Length);



/* String management routines */

NDISAPI
NDIS_STATUS
DDKAPI
NdisAnsiStringToUnicodeString(
  IN OUT PNDIS_STRING  DestinationString,
  IN PNDIS_ANSI_STRING  SourceString);

/*
 * BOOLEAN
 * NdisEqualString(
 *   IN PNDIS_STRING  String1,
 *   IN PNDIS_STRING  String2,
 *   IN BOOLEAN  CaseInsensitive);
 */
#define NdisEqualString(_String1, _String2, _CaseInsensitive) \
  RtlEqualUnicodeString(_String1, _String2, _CaseInsensitive)

NDISAPI
VOID
DDKAPI
NdisInitAnsiString(
  IN OUT PNDIS_ANSI_STRING  DestinationString,
  IN PCSTR  SourceString);

NDISAPI
VOID
DDKAPI
NdisInitUnicodeString(
  IN OUT PNDIS_STRING  DestinationString,
  IN PCWSTR  SourceString);

NDISAPI
NDIS_STATUS
DDKAPI
NdisUnicodeStringToAnsiString(
  IN OUT PNDIS_ANSI_STRING  DestinationString,
  IN PNDIS_STRING  SourceString);

#define NdisFreeString(_s)  NdisFreeMemory((_s).Buffer, (_s).MaximumLength, 0)
#define NdisPrintString(_s) DbgPrint("%ls", (_s).Buffer)


/* Spin lock reoutines */

/*
 * VOID
 * NdisAllocateSpinLock(
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisAllocateSpinLock(_SpinLock) \
  KeInitializeSpinLock(&(_SpinLock)->SpinLock)

/*
 * VOID
 * NdisFreeSpinLock(
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisFreeSpinLock(_SpinLock)

/*
 * VOID
 * NdisAcquireSpinLock(
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisAcquireSpinLock(_SpinLock) \
  KeAcquireSpinLock(&(_SpinLock)->SpinLock, &(_SpinLock)->OldIrql)

/*
 * VOID
 * NdisReleaseSpinLock(
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisReleaseSpinLock(_SpinLock) \
  KeReleaseSpinLock(&(_SpinLock)->SpinLock, (_SpinLock)->OldIrql)

/*
 * VOID
 * NdisDprAcquireSpinLock(
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisDprAcquireSpinLock(_SpinLock)                \
{                                                       \
    KeAcquireSpinLockAtDpcLevel(&(_SpinLock)->SpinLock); \
    (_SpinLock)->OldIrql = DISPATCH_LEVEL;               \
}

/*
 * VOID
 * NdisDprReleaseSpinLock(
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisDprReleaseSpinLock(_SpinLock) \
  KeReleaseSpinLockFromDpcLevel(&(_SpinLock)->SpinLock)



/* I/O routines */

/*
 * VOID
 * NdisRawReadPortBufferUchar(
 *   IN ULONG  Port,
 *   OUT PUCHAR  Buffer,
 *   IN ULONG  Length);
 */
#define NdisRawReadPortBufferUchar(Port, Buffer, Length)    \
  READ_PORT_BUFFER_UCHAR((PUCHAR)(Port), (PUCHAR)(Buffer), (Length))

/*
 * VOID
 * NdisRawReadPortBufferUlong(
 *   IN ULONG  Port,
 *   OUT PULONG  Buffer,
 *   IN ULONG  Length);
 */
#define NdisRawReadPortBufferUlong(Port, Buffer, Length)  \
  READ_PORT_BUFFER_ULONG((PULONG)(Port), (PULONG)(Buffer), (Length))

/*
 * VOID
 * NdisRawReadPortBufferUshort(
 *   IN ULONG  Port,
 *   OUT PUSHORT  Buffer,
 *   IN ULONG  Length);
 */
#define NdisRawReadPortBufferUshort(Port, Buffer, Length) \
  READ_PORT_BUFFER_USHORT((PUSHORT)(Port), (PUSHORT)(Buffer), (Length))


/*
 * VOID
 * NdisRawReadPortUchar(
 *   IN ULONG  Port,
 *   OUT PUCHAR  Data);
 */
#define NdisRawReadPortUchar(Port, Data) \
  *(Data) = READ_PORT_UCHAR((PUCHAR)(Port))

/*
 * VOID
 * NdisRawReadPortUlong(
 *   IN ULONG  Port,
 *   OUT PULONG  Data);
 */
#define NdisRawReadPortUlong(Port, Data) \
  *(Data) = READ_PORT_ULONG((PULONG)(Port))

/*
 * VOID
 * NdisRawReadPortUshort(
 *   IN ULONG   Port,
 *   OUT PUSHORT Data);
 */
#define NdisRawReadPortUshort(Port, Data) \
  *(Data) = READ_PORT_USHORT((PUSHORT)(Port))


/*
 * VOID
 * NdisRawWritePortBufferUchar(
 *   IN ULONG  Port,
 *   IN PUCHAR  Buffer,
 *   IN ULONG  Length);
 */
#define NdisRawWritePortBufferUchar(Port, Buffer, Length) \
  WRITE_PORT_BUFFER_UCHAR((PUCHAR)(Port), (PUCHAR)(Buffer), (Length))

/*
 * VOID
 * NdisRawWritePortBufferUlong(
 *   IN ULONG  Port,
 *   IN PULONG  Buffer,
 *   IN ULONG  Length);
 */
#define NdisRawWritePortBufferUlong(Port, Buffer, Length) \
  WRITE_PORT_BUFFER_ULONG((PULONG)(Port), (PULONG)(Buffer), (Length))

/*
 * VOID
 * NdisRawWritePortBufferUshort(
 *   IN ULONG   Port,
 *   IN PUSHORT Buffer,
 *   IN ULONG   Length);
 */
#define NdisRawWritePortBufferUshort(Port, Buffer, Length) \
  WRITE_PORT_BUFFER_USHORT((PUSHORT)(Port), (PUSHORT)(Buffer), (Length))


/*
 * VOID
 * NdisRawWritePortUchar(
 *   IN ULONG  Port,
 *   IN UCHAR  Data);
 */
#define NdisRawWritePortUchar(Port, Data) \
  WRITE_PORT_UCHAR((PUCHAR)(Port), (UCHAR)(Data))

/*
 * VOID
 * NdisRawWritePortUlong(
 *   IN ULONG  Port,
 *   IN ULONG  Data);
 */
#define NdisRawWritePortUlong(Port, Data)   \
  WRITE_PORT_ULONG((PULONG)(Port), (ULONG)(Data))

/*
 * VOID
 * NdisRawWritePortUshort(
 *   IN ULONG  Port,
 *   IN USHORT  Data);
 */
#define NdisRawWritePortUshort(Port, Data) \
  WRITE_PORT_USHORT((PUSHORT)(Port), (USHORT)(Data))


/*
 * VOID
 * NdisReadRegisterUchar(
 *   IN PUCHAR  Register,
 *   OUT PUCHAR  Data);
 */
#define NdisReadRegisterUchar(Register, Data) \
  *(Data) = *(Register)

/*
 * VOID
 * NdisReadRegisterUlong(
 *   IN PULONG  Register,
 *   OUT PULONG  Data);
 */
#define NdisReadRegisterUlong(Register, Data)   \
  *(Data) = *(Register)

/*
 * VOID
 * NdisReadRegisterUshort(
 *   IN PUSHORT  Register,
 *   OUT PUSHORT  Data);
 */
#define NdisReadRegisterUshort(Register, Data)  \
    *(Data) = *(Register)

/*
 * VOID
 * NdisReadRegisterUchar(
 *   IN PUCHAR  Register,
 *   IN UCHAR  Data);
 */
#define NdisWriteRegisterUchar(Register, Data) \
  WRITE_REGISTER_UCHAR((Register), (Data))

/*
 * VOID
 * NdisReadRegisterUlong(
 *   IN PULONG  Register,
 *   IN ULONG  Data);
 */
#define NdisWriteRegisterUlong(Register, Data) \
  WRITE_REGISTER_ULONG((Register), (Data))

/*
 * VOID
 * NdisReadRegisterUshort(
 *   IN PUSHORT  Register,
 *   IN USHORT  Data);
 */
#define NdisWriteRegisterUshort(Register, Data) \
  WRITE_REGISTER_USHORT((Register), (Data))


/* Linked lists */

/*
 * VOID
 * NdisInitializeListHead(
 *   IN PLIST_ENTRY  ListHead);
 */
#define NdisInitializeListHead(_ListHead) \
  InitializeListHead(_ListHead)

/*
 * PLIST_ENTRY
 * NdisInterlockedInsertHeadList(
 *   IN PLIST_ENTRY  ListHead,
 *   IN PLIST_ENTRY  ListEntry,
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisInterlockedInsertHeadList(_ListHead, _ListEntry, _SpinLock) \
  ExInterlockedInsertHeadList(_ListHead, _ListEntry, &(_SpinLock)->SpinLock)

/*
 * PLIST_ENTRY
 * NdisInterlockedInsertTailList(
 *   IN PLIST_ENTRY  ListHead,
 *   IN PLIST_ENTRY  ListEntry,
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisInterlockedInsertTailList(_ListHead, _ListEntry, _SpinLock) \
  ExInterlockedInsertTailList(_ListHead, _ListEntry, &(_SpinLock)->SpinLock)

/*
 * PLIST_ENTRY
 * NdisInterlockedRemoveHeadList(
 *   IN PLIST_ENTRY  ListHead,
 *   IN PNDIS_SPIN_LOCK  SpinLock);
*/
#define NdisInterlockedRemoveHeadList(_ListHead, _SpinLock) \
  ExInterlockedRemoveHeadList(_ListHead, &(_SpinLock)->SpinLock)

/*
 * VOID
 * NdisInitializeSListHead(
 *   IN PSLIST_HEADER  SListHead);
 */
#define NdisInitializeSListHead(SListHead) ExInitializeSListHead(SListHead)

/*
 * USHORT NdisQueryDepthSList(
 *   IN PSLIST_HEADER  SListHead);
 */
#define NdisQueryDepthSList(SListHead) ExQueryDepthSList(SListHead)



/* Interlocked routines */

/*
 * LONG
 * NdisInterlockedDecrement(
 *   IN PLONG  Addend);
 */
#define NdisInterlockedDecrement(Addend) InterlockedDecrement(Addend)

/*
 * LONG
 * NdisInterlockedIncrement(
 *   IN PLONG  Addend);
 */
#define NdisInterlockedIncrement(Addend) InterlockedIncrement(Addend)

/*
 * VOID
 * NdisInterlockedAddUlong(
 *   IN PULONG  Addend,
 *   IN ULONG  Increment,
 *   IN PNDIS_SPIN_LOCK  SpinLock);
 */
#define NdisInterlockedAddUlong(_Addend, _Increment, _SpinLock) \
  ExInterlockedAddUlong(_Addend, _Increment, &(_SpinLock)->SpinLock)



/* Miscellaneous routines */

NDISAPI
VOID
DDKAPI
NdisCloseConfiguration(
  IN NDIS_HANDLE  ConfigurationHandle);

NDISAPI
VOID
DDKAPI
NdisReadConfiguration(
  OUT  PNDIS_STATUS  Status,
  OUT  PNDIS_CONFIGURATION_PARAMETER  *ParameterValue,
  IN NDIS_HANDLE  ConfigurationHandle,
  IN PNDIS_STRING  Keyword,
  IN NDIS_PARAMETER_TYPE  ParameterType);

NDISAPI
VOID
DDKAPI
NdisWriteConfiguration(
  OUT  PNDIS_STATUS  Status,
  IN NDIS_HANDLE  WrapperConfigurationContext,
  IN PNDIS_STRING  Keyword,
  IN PNDIS_CONFIGURATION_PARAMETER  *ParameterValue);

NDISAPI
VOID
DDKCDECLAPI
NdisWriteErrorLogEntry(
	IN NDIS_HANDLE  NdisAdapterHandle,
	IN NDIS_ERROR_CODE  ErrorCode,
	IN ULONG  NumberOfErrorValues,
	IN ...);

/*
 * VOID
 * NdisStallExecution(
 *   IN UINT  MicrosecondsToStall)
 */
#define NdisStallExecution KeStallExecutionProcessor

/*
 * VOID
 * NdisGetCurrentSystemTime(
 *   IN PLARGE_INTEGER  pSystemTime);
 */
#define NdisGetCurrentSystemTime KeQuerySystemTime

NDISAPI
VOID
DDKAPI
NdisGetCurrentProcessorCpuUsage(
  OUT PULONG  pCpuUsage);



/* NDIS helper macros */

/*
 * VOID
 * NDIS_INIT_FUNCTION(FunctionName)
 */
#define NDIS_INIT_FUNCTION(FunctionName)    \
  alloc_text(init, FunctionName)

/*
 * VOID
 * NDIS_PAGABLE_FUNCTION(FunctionName) 
 */
#define NDIS_PAGEABLE_FUNCTION(FunctionName) \
  alloc_text(page, FunctionName)

#define NDIS_PAGABLE_FUNCTION NDIS_PAGEABLE_FUNCTION


/* NDIS 4.0 extensions */

NDISAPI
VOID
DDKAPI
NdisMFreeSharedMemory(
	IN NDIS_HANDLE  MiniportAdapterHandle,
	IN ULONG  Length,
	IN BOOLEAN  Cached,
	IN PVOID  VirtualAddress,
	IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress);

NDISAPI
VOID
DDKAPI
NdisMWanIndicateReceive(
	OUT PNDIS_STATUS  Status,
	IN NDIS_HANDLE  MiniportAdapterHandle,
	IN NDIS_HANDLE  NdisLinkContext,
	IN PUCHAR  PacketBuffer,
	IN UINT  PacketSize);

NDISAPI
VOID
DDKAPI
NdisMWanIndicateReceiveComplete(
  IN NDIS_HANDLE  MiniportAdapterHandle);

NDISAPI
VOID
DDKAPI
NdisMWanSendComplete(
	IN NDIS_HANDLE  MiniportAdapterHandle,
	IN PNDIS_WAN_PACKET  Packet,
	IN NDIS_STATUS  Status);

NDISAPI
NDIS_STATUS
DDKAPI
NdisPciAssignResources(
	IN NDIS_HANDLE  NdisMacHandle,
	IN NDIS_HANDLE  NdisWrapperHandle,
	IN NDIS_HANDLE  WrapperConfigurationContext,
	IN ULONG  SlotNumber,
	OUT PNDIS_RESOURCE_LIST  *AssignedResources);


/* NDIS 5.0 extensions */

NDISAPI
VOID
DDKAPI
NdisAcquireReadWriteLock(
  IN PNDIS_RW_LOCK  Lock,
  IN BOOLEAN  fWrite,
  IN PLOCK_STATE  LockState);

NDISAPI
NDIS_STATUS
DDKAPI
NdisAllocateMemoryWithTag(
  OUT PVOID  *VirtualAddress,
  IN UINT  Length,
  IN ULONG  Tag);

NDISAPI
VOID
DDKAPI
NdisAllocatePacketPoolEx(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_HANDLE  PoolHandle,
  IN UINT  NumberOfDescriptors,
  IN UINT  NumberOfOverflowDescriptors,
  IN UINT  ProtocolReservedLength);

NDISAPI
VOID
DDKAPI
NdisCompletePnPEvent(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisBindingHandle,
  IN PNET_PNP_EVENT  NetPnPEvent);

NDISAPI
VOID
DDKAPI
NdisGetCurrentProcessorCounts(
  OUT PULONG  pIdleCount,
  OUT PULONG  pKernelAndUser,
  OUT PULONG  pIndex);

NDISAPI
VOID
DDKAPI
NdisGetDriverHandle(
  IN PNDIS_HANDLE  NdisBindingHandle,
  OUT PNDIS_HANDLE  NdisDriverHandle);

NDISAPI
PNDIS_PACKET
DDKAPI
NdisGetReceivedPacket(
  IN PNDIS_HANDLE  NdisBindingHandle,
  IN PNDIS_HANDLE  MacContext);

NDISAPI
VOID
DDKAPI
NdisGetSystemUptime(
  OUT PULONG  pSystemUpTime);

NDISAPI
VOID
DDKAPI
NdisInitializeReadWriteLock(
  IN PNDIS_RW_LOCK  Lock);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMDeregisterDevice(
  IN NDIS_HANDLE  NdisDeviceHandle);

NDISAPI
VOID
DDKAPI
NdisMGetDeviceProperty(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN OUT PDEVICE_OBJECT  *PhysicalDeviceObject  OPTIONAL,
  IN OUT PDEVICE_OBJECT  *FunctionalDeviceObject  OPTIONAL,
  IN OUT PDEVICE_OBJECT  *NextDeviceObject  OPTIONAL,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources  OPTIONAL,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResourcesTranslated  OPTIONAL);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMInitializeScatterGatherDma(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN BOOLEAN  Dma64BitAddresses,
  IN ULONG  MaximumPhysicalMapping);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMPromoteMiniport(
  IN NDIS_HANDLE  MiniportAdapterHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMQueryAdapterInstanceName(
  OUT PNDIS_STRING  AdapterInstanceName,
  IN NDIS_HANDLE  MiniportAdapterHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMRegisterDevice(
  IN NDIS_HANDLE  NdisWrapperHandle,
  IN PNDIS_STRING  DeviceName,
  IN PNDIS_STRING  SymbolicName,
  IN PDRIVER_DISPATCH  MajorFunctions[],
  OUT PDEVICE_OBJECT  *pDeviceObject,
  OUT NDIS_HANDLE  *NdisDeviceHandle);

NDISAPI
VOID
DDKAPI
NdisMRegisterUnloadHandler(
  IN NDIS_HANDLE  NdisWrapperHandle,
  IN PDRIVER_UNLOAD  UnloadHandler);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMRemoveMiniport(
  IN NDIS_HANDLE  MiniportAdapterHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMSetMiniportSecondary(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_HANDLE  PrimaryMiniportAdapterHandle);

NDISAPI
VOID
DDKAPI
NdisOpenConfigurationKeyByIndex(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  ConfigurationHandle,
  IN ULONG  Index,
  OUT PNDIS_STRING  KeyName,
  OUT PNDIS_HANDLE  KeyHandle);

NDISAPI
VOID
DDKAPI
NdisOpenConfigurationKeyByName(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  ConfigurationHandle,
  IN PNDIS_STRING  SubKeyName,
  OUT PNDIS_HANDLE  SubKeyHandle);

NDISAPI
UINT
DDKAPI
NdisPacketPoolUsage(
  IN NDIS_HANDLE  PoolHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisQueryAdapterInstanceName(
  OUT PNDIS_STRING  AdapterInstanceName,
  IN NDIS_HANDLE  NdisBindingHandle);

NDISAPI
ULONG
DDKAPI
NdisReadPcmciaAttributeMemory(
  IN NDIS_HANDLE  NdisAdapterHandle,
  IN ULONG  Offset,
  IN PVOID  Buffer,
  IN ULONG  Length);

NDISAPI
VOID
DDKAPI
NdisReleaseReadWriteLock(
  IN PNDIS_RW_LOCK  Lock,
  IN PLOCK_STATE  LockState);

NDISAPI
NDIS_STATUS
DDKAPI
NdisWriteEventLogEntry(
  IN PVOID  LogHandle,
  IN NDIS_STATUS  EventCode,
  IN ULONG  UniqueEventValue,
  IN USHORT  NumStrings,
  IN PVOID  StringsList  OPTIONAL,
  IN ULONG  DataSize,
  IN PVOID  Data  OPTIONAL);

NDISAPI
ULONG
DDKAPI
NdisWritePcmciaAttributeMemory(
  IN NDIS_HANDLE  NdisAdapterHandle,
  IN ULONG  Offset,
  IN PVOID  Buffer,
  IN ULONG  Length);


/* Connectionless services */

NDISAPI
NDIS_STATUS
DDKAPI
NdisClAddParty(
  IN NDIS_HANDLE  NdisVcHandle,
  IN NDIS_HANDLE  ProtocolPartyContext,
  IN OUT PCO_CALL_PARAMETERS  CallParameters,
  OUT PNDIS_HANDLE  NdisPartyHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisClCloseAddressFamily(
  IN NDIS_HANDLE  NdisAfHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisClCloseCall(
  IN NDIS_HANDLE NdisVcHandle,
  IN NDIS_HANDLE NdisPartyHandle  OPTIONAL,
  IN PVOID  Buffer  OPTIONAL,
  IN UINT  Size);

NDISAPI
NDIS_STATUS
DDKAPI
NdisClDeregisterSap(
  IN NDIS_HANDLE  NdisSapHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisClDropParty(
  IN NDIS_HANDLE  NdisPartyHandle,
  IN PVOID  Buffer  OPTIONAL,
  IN UINT  Size);

NDISAPI
VOID
DDKAPI
NdisClIncomingCallComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
NDIS_STATUS
DDKAPI
NdisClMakeCall(
  IN NDIS_HANDLE  NdisVcHandle,
  IN OUT PCO_CALL_PARAMETERS  CallParameters,
  IN NDIS_HANDLE  ProtocolPartyContext  OPTIONAL,
  OUT PNDIS_HANDLE  NdisPartyHandle  OPTIONAL);

NDISAPI
NDIS_STATUS 
DDKAPI
NdisClModifyCallQoS(
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);


NDISAPI
NDIS_STATUS
DDKAPI
NdisClOpenAddressFamily(
  IN NDIS_HANDLE  NdisBindingHandle,
  IN PCO_ADDRESS_FAMILY  AddressFamily,
  IN NDIS_HANDLE  ProtocolAfContext,
  IN PNDIS_CLIENT_CHARACTERISTICS  ClCharacteristics,
  IN UINT  SizeOfClCharacteristics,
  OUT PNDIS_HANDLE  NdisAfHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisClRegisterSap(
  IN NDIS_HANDLE  NdisAfHandle,
  IN NDIS_HANDLE  ProtocolSapContext,
  IN PCO_SAP  Sap,
  OUT PNDIS_HANDLE  NdisSapHandle);


/* Call Manager services */

NDISAPI
NDIS_STATUS
DDKAPI
NdisCmActivateVc(
  IN NDIS_HANDLE  NdisVcHandle,
  IN OUT PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisCmAddPartyComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisPartyHandle,
  IN NDIS_HANDLE  CallMgrPartyContext  OPTIONAL,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisCmCloseAddressFamilyComplete(
  IN NDIS_STATUS Status,
  IN NDIS_HANDLE NdisAfHandle);

NDISAPI
VOID
DDKAPI
NdisCmCloseCallComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle,
  IN NDIS_HANDLE  NdisPartyHandle  OPTIONAL);

NDISAPI
NDIS_STATUS
DDKAPI
NdisCmDeactivateVc(
  IN NDIS_HANDLE  NdisVcHandle);

NDISAPI
VOID
DDKAPI
NdisCmDeregisterSapComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisSapHandle);

NDISAPI
VOID
DDKAPI
NdisCmDispatchCallConnected(
  IN NDIS_HANDLE  NdisVcHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisCmDispatchIncomingCall(
  IN NDIS_HANDLE  NdisSapHandle,
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisCmDispatchIncomingCallQoSChange(
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisCmDispatchIncomingCloseCall(
  IN NDIS_STATUS  CloseStatus,
  IN NDIS_HANDLE  NdisVcHandle,
  IN PVOID  Buffer  OPTIONAL,
  IN UINT  Size);

NDISAPI
VOID
DDKAPI
NdisCmDispatchIncomingDropParty(
  IN NDIS_STATUS  DropStatus,
  IN NDIS_HANDLE  NdisPartyHandle,
  IN PVOID  Buffer  OPTIONAL,
  IN UINT  Size);

NDISAPI
VOID
DDKAPI
NdisCmDropPartyComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisPartyHandle);

NDISAPI
VOID
DDKAPI
NdisCmMakeCallComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle,
  IN NDIS_HANDLE  NdisPartyHandle  OPTIONAL,
  IN NDIS_HANDLE  CallMgrPartyContext  OPTIONAL,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisCmModifyCallQoSComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisCmOpenAddressFamilyComplete(
  IN NDIS_STATUS Status,
  IN NDIS_HANDLE NdisAfHandle,
  IN NDIS_HANDLE CallMgrAfContext);

NDISAPI
NDIS_STATUS
DDKAPI
NdisCmRegisterAddressFamily(
  IN NDIS_HANDLE  NdisBindingHandle,
  IN PCO_ADDRESS_FAMILY  AddressFamily,
  IN PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
  IN UINT  SizeOfCmCharacteristics);

NDISAPI
VOID
DDKAPI
NdisCmRegisterSapComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisSapHandle,
  IN NDIS_HANDLE  CallMgrSapContext);


NDISAPI
NDIS_STATUS
DDKAPI
NdisMCmActivateVc(
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMCmCreateVc(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_HANDLE  NdisAfHandle,
  IN NDIS_HANDLE  MiniportVcContext,
  OUT  PNDIS_HANDLE  NdisVcHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMCmDeactivateVc(
  IN NDIS_HANDLE  NdisVcHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMCmDeleteVc(
  IN NDIS_HANDLE  NdisVcHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMCmRegisterAddressFamily(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN PCO_ADDRESS_FAMILY  AddressFamily,
  IN PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
  IN UINT  SizeOfCmCharacteristics);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMCmRequest(
  IN NDIS_HANDLE  NdisAfHandle,
  IN NDIS_HANDLE  NdisVcHandle  OPTIONAL,
  IN NDIS_HANDLE  NdisPartyHandle  OPTIONAL,
  IN OUT  PNDIS_REQUEST  NdisRequest);


/* Connection-oriented services */

NDISAPI
NDIS_STATUS
DDKAPI
NdisCoCreateVc(
  IN NDIS_HANDLE  NdisBindingHandle,
  IN NDIS_HANDLE  NdisAfHandle  OPTIONAL,
  IN NDIS_HANDLE  ProtocolVcContext,
  IN OUT PNDIS_HANDLE  NdisVcHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisCoDeleteVc(
  IN NDIS_HANDLE  NdisVcHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisCoRequest(
  IN NDIS_HANDLE  NdisBindingHandle,
  IN NDIS_HANDLE  NdisAfHandle  OPTIONAL,
  IN NDIS_HANDLE  NdisVcHandle  OPTIONAL,
  IN NDIS_HANDLE  NdisPartyHandle  OPTIONAL,
  IN OUT  PNDIS_REQUEST  NdisRequest);

NDISAPI
VOID
DDKAPI
NdisCoRequestComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisAfHandle,
  IN NDIS_HANDLE  NdisVcHandle  OPTIONAL,
  IN NDIS_HANDLE  NdisPartyHandle  OPTIONAL,
  IN PNDIS_REQUEST  NdisRequest);

NDISAPI
VOID
DDKAPI
NdisCoSendPackets(
  IN NDIS_HANDLE  NdisVcHandle,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);

NDISAPI
VOID
DDKAPI
NdisMCoActivateVcComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle,
  IN PCO_CALL_PARAMETERS  CallParameters);

NDISAPI
VOID
DDKAPI
NdisMCoDeactivateVcComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle);

NDISAPI
VOID
DDKAPI
NdisMCoIndicateReceivePacket(
  IN NDIS_HANDLE  NdisVcHandle,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);

NDISAPI
VOID
DDKAPI
NdisMCoIndicateStatus(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_HANDLE  NdisVcHandle  OPTIONAL,
  IN NDIS_STATUS  GeneralStatus,
  IN PVOID  StatusBuffer  OPTIONAL,
  IN ULONG  StatusBufferSize);

NDISAPI
VOID
DDKAPI
NdisMCoReceiveComplete(
  IN NDIS_HANDLE  MiniportAdapterHandle);

NDISAPI
VOID
DDKAPI
NdisMCoRequestComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN PNDIS_REQUEST  Request);

NDISAPI
VOID 
DDKAPI
NdisMCoSendComplete(
  IN NDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisVcHandle,
  IN PNDIS_PACKET  Packet);


/* NDIS 5.0 extensions for intermediate drivers */

NDISAPI
VOID
DDKAPI
NdisIMAssociateMiniport(
  IN NDIS_HANDLE  DriverHandle,
  IN NDIS_HANDLE  ProtocolHandle);

NDISAPI
NDIS_STATUS 
DDKAPI
NdisIMCancelInitializeDeviceInstance(
  IN NDIS_HANDLE  DriverHandle,
  IN PNDIS_STRING  DeviceInstance);

NDISAPI
VOID
DDKAPI
NdisIMCopySendCompletePerPacketInfo(
  IN PNDIS_PACKET  DstPacket,
  IN PNDIS_PACKET  SrcPacket);

NDISAPI
VOID
DDKAPI
NdisIMCopySendPerPacketInfo(
  IN PNDIS_PACKET  DstPacket,
  IN PNDIS_PACKET  SrcPacket);

NDISAPI
VOID
DDKAPI
NdisIMDeregisterLayeredMiniport(
  IN NDIS_HANDLE  DriverHandle);

NDISAPI
NDIS_HANDLE
DDKAPI
NdisIMGetBindingContext(
  IN NDIS_HANDLE  NdisBindingHandle);

NDISAPI
NDIS_HANDLE
DDKAPI
NdisIMGetDeviceContext(
  IN NDIS_HANDLE  MiniportAdapterHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisIMInitializeDeviceInstanceEx(
  IN NDIS_HANDLE  DriverHandle,
  IN PNDIS_STRING  DriverInstance,
  IN NDIS_HANDLE  DeviceContext  OPTIONAL);

NDISAPI
PSINGLE_LIST_ENTRY
DDKAPI
NdisInterlockedPopEntrySList(
  IN PSLIST_HEADER  ListHead,
  IN PKSPIN_LOCK  Lock);

NDISAPI
PSINGLE_LIST_ENTRY
DDKAPI
NdisInterlockedPushEntrySList(
  IN PSLIST_HEADER  ListHead,
  IN PSINGLE_LIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock);

NDISAPI
VOID
DDKAPI
NdisQueryBufferSafe(
  IN PNDIS_BUFFER  Buffer,
  OUT PVOID  *VirtualAddress  OPTIONAL,
  OUT PUINT  Length,
  IN UINT  Priority);


/* Prototypes for NDIS_MINIPORT_CHARACTERISTICS */

typedef BOOLEAN DDKAPI
(*W_CHECK_FOR_HANG_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext);

typedef VOID DDKAPI
(*W_DISABLE_INTERRUPT_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext);

typedef VOID DDKAPI
(*W_ENABLE_INTERRUPT_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext);

typedef VOID DDKAPI
(*W_HALT_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext);

typedef VOID DDKAPI
(*W_HANDLE_INTERRUPT_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext);

typedef NDIS_STATUS DDKAPI
(*W_INITIALIZE_HANDLER)(
  OUT PNDIS_STATUS  OpenErrorStatus,
  OUT PUINT  SelectedMediumIndex,
  IN PNDIS_MEDIUM  MediumArray,
  IN UINT  MediumArraySize,
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_HANDLE  WrapperConfigurationContext);

typedef VOID DDKAPI
(*W_ISR_HANDLER)(
  OUT PBOOLEAN  InterruptRecognized,
  OUT PBOOLEAN  QueueMiniportHandleInterrupt,
  IN	NDIS_HANDLE  MiniportAdapterContext);

typedef NDIS_STATUS DDKAPI
(*W_QUERY_INFORMATION_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_OID  Oid,
  IN PVOID  InformationBuffer,
  IN ULONG  InformationBufferLength,
  OUT PULONG  BytesWritten,
  OUT PULONG  BytesNeeded);

typedef NDIS_STATUS DDKAPI
(*W_RECONFIGURE_HANDLER)(
  OUT PNDIS_STATUS  OpenErrorStatus,
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_HANDLE	WrapperConfigurationContext);

typedef NDIS_STATUS DDKAPI
(*W_RESET_HANDLER)(
  OUT PBOOLEAN  AddressingReset,
  IN NDIS_HANDLE  MiniportAdapterContext);

typedef NDIS_STATUS DDKAPI
(*W_SEND_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PNDIS_PACKET  Packet,
  IN UINT  Flags);

typedef NDIS_STATUS DDKAPI
(*WM_SEND_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_HANDLE  NdisLinkHandle,
  IN PNDIS_WAN_PACKET  Packet);

typedef NDIS_STATUS DDKAPI
(*W_SET_INFORMATION_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_OID  Oid,
  IN PVOID  InformationBuffer,
  IN ULONG  InformationBufferLength,
  OUT PULONG  BytesRead,
  OUT PULONG  BytesNeeded);

typedef NDIS_STATUS DDKAPI
(*W_TRANSFER_DATA_HANDLER)(
  OUT PNDIS_PACKET  Packet,
  OUT PUINT  BytesTransferred,
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_HANDLE  MiniportReceiveContext,
  IN UINT  ByteOffset,
  IN UINT  BytesToTransfer);

typedef NDIS_STATUS DDKAPI
(*WM_TRANSFER_DATA_HANDLER)(
  VOID);


/* NDIS structures available only to miniport drivers */

#define NDIS30_MINIPORT_CHARACTERISTICS_S \
  UCHAR  MajorNdisVersion; \
  UCHAR  MinorNdisVersion; \
  UINT  Reserved; \
  W_CHECK_FOR_HANG_HANDLER  CheckForHangHandler; \
  W_DISABLE_INTERRUPT_HANDLER  DisableInterruptHandler; \
  W_ENABLE_INTERRUPT_HANDLER  EnableInterruptHandler; \
  W_HALT_HANDLER  HaltHandler; \
  W_HANDLE_INTERRUPT_HANDLER  HandleInterruptHandler; \
  W_INITIALIZE_HANDLER  InitializeHandler; \
  W_ISR_HANDLER  ISRHandler; \
  W_QUERY_INFORMATION_HANDLER  QueryInformationHandler; \
  W_RECONFIGURE_HANDLER  ReconfigureHandler; \
  W_RESET_HANDLER  ResetHandler; \
  _ANONYMOUS_UNION union { \
    W_SEND_HANDLER  SendHandler; \
    WM_SEND_HANDLER  WanSendHandler; \
  } _UNION_NAME(u1); \
  W_SET_INFORMATION_HANDLER  SetInformationHandler; \
  _ANONYMOUS_UNION union { \
    W_TRANSFER_DATA_HANDLER  TransferDataHandler; \
    WM_TRANSFER_DATA_HANDLER  WanTransferDataHandler; \
  } _UNION_NAME(u2);

typedef struct _NDIS30_MINIPORT_CHARACTERISTICS {
  NDIS30_MINIPORT_CHARACTERISTICS_S
} NDIS30_MINIPORT_CHARACTERISTICS, *PSNDIS30_MINIPORT_CHARACTERISTICS;


/* Extensions for NDIS 4.0 miniports */

typedef VOID DDKAPI
(*W_SEND_PACKETS_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);

typedef VOID DDKAPI
(*W_RETURN_PACKET_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PNDIS_PACKET  Packet);

typedef VOID DDKAPI
(*W_ALLOCATE_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PVOID  VirtualAddress,
  IN PNDIS_PHYSICAL_ADDRESS  PhysicalAddress,
  IN ULONG  Length,
  IN PVOID  Context);

#ifdef __cplusplus

#define NDIS40_MINIPORT_CHARACTERISTICS_S \
  NDIS30_MINIPORT_CHARACTERISTICS  Ndis30Chars; \
  W_RETURN_PACKET_HANDLER  ReturnPacketHandler; \
  W_SEND_PACKETS_HANDLER  SendPacketsHandler; \
  W_ALLOCATE_COMPLETE_HANDLER  AllocateCompleteHandler;

#else /* !__cplusplus */

#define NDIS40_MINIPORT_CHARACTERISTICS_S \
  NDIS30_MINIPORT_CHARACTERISTICS_S \
  W_RETURN_PACKET_HANDLER  ReturnPacketHandler; \
  W_SEND_PACKETS_HANDLER  SendPacketsHandler; \
  W_ALLOCATE_COMPLETE_HANDLER  AllocateCompleteHandler;

#endif /* !__cplusplus */

typedef struct _NDIS40_MINIPORT_CHARACTERISTICS {
  NDIS40_MINIPORT_CHARACTERISTICS_S
} NDIS40_MINIPORT_CHARACTERISTICS, *PNDIS40_MINIPORT_CHARACTERISTICS;


/* Extensions for NDIS 5.0 miniports */

typedef NDIS_STATUS DDKAPI
(*W_CO_CREATE_VC_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_HANDLE  NdisVcHandle,
  OUT PNDIS_HANDLE  MiniportVcContext);

typedef NDIS_STATUS DDKAPI
(*W_CO_DELETE_VC_HANDLER)(
  IN NDIS_HANDLE  MiniportVcContext);

typedef NDIS_STATUS DDKAPI
(*W_CO_ACTIVATE_VC_HANDLER)(
  IN NDIS_HANDLE  MiniportVcContext,
  IN OUT PCO_CALL_PARAMETERS  CallParameters);

typedef NDIS_STATUS DDKAPI
(*W_CO_DEACTIVATE_VC_HANDLER)(
  IN NDIS_HANDLE  MiniportVcContext);

typedef VOID DDKAPI
(*W_CO_SEND_PACKETS_HANDLER)(
  IN NDIS_HANDLE  MiniportVcContext,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);

typedef NDIS_STATUS DDKAPI
(*W_CO_REQUEST_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN NDIS_HANDLE  MiniportVcContext  OPTIONAL,
  IN OUT PNDIS_REQUEST  NdisRequest);

#ifdef __cplusplus

#define NDIS50_MINIPORT_CHARACTERISTICS_S \
  NDIS40_MINIPORT_CHARACTERISTICS  Ndis40Chars; \
  W_CO_CREATE_VC_HANDLER  CoCreateVcHandler; \
  W_CO_DELETE_VC_HANDLER  CoDeleteVcHandler; \
  W_CO_ACTIVATE_VC_HANDLER  CoActivateVcHandler; \
  W_CO_DEACTIVATE_VC_HANDLER  CoDeactivateVcHandler; \
  W_CO_SEND_PACKETS_HANDLER  CoSendPacketsHandler; \
  W_CO_REQUEST_HANDLER  CoRequestHandler;

#else /* !__cplusplus */

#define NDIS50_MINIPORT_CHARACTERISTICS_S \
  NDIS40_MINIPORT_CHARACTERISTICS_S \
  W_CO_CREATE_VC_HANDLER  CoCreateVcHandler; \
  W_CO_DELETE_VC_HANDLER  CoDeleteVcHandler; \
  W_CO_ACTIVATE_VC_HANDLER  CoActivateVcHandler; \
  W_CO_DEACTIVATE_VC_HANDLER  CoDeactivateVcHandler; \
  W_CO_SEND_PACKETS_HANDLER  CoSendPacketsHandler; \
  W_CO_REQUEST_HANDLER  CoRequestHandler;

#endif /* !__cplusplus */

typedef struct _NDIS50_MINIPORT_CHARACTERISTICS {
   NDIS50_MINIPORT_CHARACTERISTICS_S
} NDIS50_MINIPORT_CHARACTERISTICS, *PSNDIS50_MINIPORT_CHARACTERISTICS;


/* Extensions for NDIS 5.1 miniports */

typedef VOID DDKAPI
(*W_CANCEL_SEND_PACKETS_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PVOID  CancelId);


#if defined(NDIS51)
typedef struct _NDIS_MINIPORT_CHARACTERISTICS {
  NDIS50_MINIPORT_CHARACTERISTICS_S
} NDIS_MINIPORT_CHARACTERISTICS, *PNDIS_MINIPORT_CHARACTERISTICS;
#elif defined(NDIS50)
typedef struct _NDIS_MINIPORT_CHARACTERISTICS {
  NDIS50_MINIPORT_CHARACTERISTICS_S
} NDIS_MINIPORT_CHARACTERISTICS, *PNDIS_MINIPORT_CHARACTERISTICS;
#elif defined(NDIS40)
typedef struct _NDIS_MINIPORT_CHARACTERISTICS {
  NDIS40_MINIPORT_CHARACTERISTICS_S
} NDIS_MINIPORT_CHARACTERISTICS, *PNDIS_MINIPORT_CHARACTERISTICS;
#elif defined(NDIS30)
typedef struct _NDIS_MINIPORT_CHARACTERISTICS {
  NDIS30_MINIPORT_CHARACTERISTICS_S
} NDIS_MINIPORT_CHARACTERISTICS, *PNDIS_MINIPORT_CHARACTERISTICS;
#endif /* NDIS30 */


typedef NDIS_STATUS DDKAPI
(*SEND_HANDLER)(
  IN NDIS_HANDLE  MacBindingHandle,
  IN PNDIS_PACKET  Packet);

typedef NDIS_STATUS DDKAPI
(*TRANSFER_DATA_HANDLER)(
  IN NDIS_HANDLE  MacBindingHandle,
  IN NDIS_HANDLE  MacReceiveContext,
  IN UINT  ByteOffset,
  IN UINT  BytesToTransfer,
  OUT PNDIS_PACKET  Packet,
  OUT PUINT  BytesTransferred);

typedef NDIS_STATUS DDKAPI
(*RESET_HANDLER)(
  IN NDIS_HANDLE  MacBindingHandle);

typedef NDIS_STATUS DDKAPI
(*REQUEST_HANDLER)(
  IN NDIS_HANDLE   MacBindingHandle,
  IN PNDIS_REQUEST   NdisRequest);



/* Structures available only to full MAC drivers */

typedef BOOLEAN DDKAPI
(*PNDIS_INTERRUPT_SERVICE)(
  IN PVOID  InterruptContext);

typedef VOID DDKAPI
(*PNDIS_DEFERRED_PROCESSING)(
  IN PVOID  SystemSpecific1,
  IN PVOID  InterruptContext,
  IN PVOID  SystemSpecific2,
  IN PVOID  SystemSpecific3);



typedef struct _NDIS_MINIPORT_BLOCK NDIS_MINIPORT_BLOCK, *PNDIS_MINIPORT_BLOCK;
typedef struct _NDIS_PROTOCOL_BLOCK NDIS_PROTOCOL_BLOCK, *PNDIS_PROTOCOL_BLOCK;
typedef struct _NDIS_OPEN_BLOCK		NDIS_OPEN_BLOCK,     *PNDIS_OPEN_BLOCK;
typedef struct _NDIS_M_DRIVER_BLOCK NDIS_M_DRIVER_BLOCK, *PNDIS_M_DRIVER_BLOCK;
typedef	struct _NDIS_AF_LIST        NDIS_AF_LIST,        *PNDIS_AF_LIST;


typedef struct _NDIS_MINIPORT_INTERRUPT {
  PKINTERRUPT  InterruptObject;
  KSPIN_LOCK  DpcCountLock;
  PVOID  MiniportIdField;
  W_ISR_HANDLER  MiniportIsr;
  W_HANDLE_INTERRUPT_HANDLER  MiniportDpc;
  KDPC  InterruptDpc;
  PNDIS_MINIPORT_BLOCK  Miniport;
  UCHAR  DpcCount;
  BOOLEAN  Filler1;
  KEVENT  DpcsCompletedEvent;
  BOOLEAN  SharedInterrupt;
  BOOLEAN	 IsrRequested;
} NDIS_MINIPORT_INTERRUPT, *PNDIS_MINIPORT_INTERRUPT;

typedef struct _NDIS_MINIPORT_TIMER {
  KTIMER  Timer;
  KDPC  Dpc;
  PNDIS_TIMER_FUNCTION  MiniportTimerFunction;
  PVOID  MiniportTimerContext;
  PNDIS_MINIPORT_BLOCK  Miniport;
  struct _NDIS_MINIPORT_TIMER  *NextDeferredTimer;
} NDIS_MINIPORT_TIMER, *PNDIS_MINIPORT_TIMER;

typedef struct _NDIS_INTERRUPT {
  PKINTERRUPT  InterruptObject;
  KSPIN_LOCK  DpcCountLock;
  PNDIS_INTERRUPT_SERVICE  MacIsr;
  PNDIS_DEFERRED_PROCESSING  MacDpc;
  KDPC  InterruptDpc;
  PVOID  InterruptContext;
  UCHAR  DpcCount;
  BOOLEAN	 Removing;
  KEVENT  DpcsCompletedEvent;
} NDIS_INTERRUPT, *PNDIS_INTERRUPT;


typedef struct _MAP_REGISTER_ENTRY {
	PVOID  MapRegister;
	BOOLEAN  WriteToDevice;
} MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;


typedef enum _NDIS_WORK_ITEM_TYPE {
  NdisWorkItemRequest,
  NdisWorkItemSend,
  NdisWorkItemReturnPackets,
  NdisWorkItemResetRequested,
  NdisWorkItemResetInProgress,
  NdisWorkItemHalt,
  NdisWorkItemSendLoopback,
  NdisWorkItemMiniportCallback,
  NdisMaxWorkItems
} NDIS_WORK_ITEM_TYPE, *PNDIS_WORK_ITEM_TYPE;

#define	NUMBER_OF_WORK_ITEM_TYPES         NdisMaxWorkItems
#define	NUMBER_OF_SINGLE_WORK_ITEMS       6

typedef struct _NDIS_MINIPORT_WORK_ITEM {
	SINGLE_LIST_ENTRY  Link;
	NDIS_WORK_ITEM_TYPE  WorkItemType;
	PVOID  WorkItemContext;
} NDIS_MINIPORT_WORK_ITEM, *PNDIS_MINIPORT_WORK_ITEM;


typedef struct _NDIS_BIND_PATHS {
	UINT  Number;
	NDIS_STRING  Paths[1];
} NDIS_BIND_PATHS, *PNDIS_BIND_PATHS;

#define DECLARE_UNKNOWN_STRUCT(BaseName) \
  typedef struct _##BaseName BaseName, *P##BaseName;

#define DECLARE_UNKNOWN_PROTOTYPE(Name) \
  typedef VOID (*(Name))(VOID);

#define ETH_LENGTH_OF_ADDRESS 6

DECLARE_UNKNOWN_STRUCT(ETH_BINDING_INFO)

DECLARE_UNKNOWN_PROTOTYPE(ETH_ADDRESS_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(ETH_FILTER_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(ETH_DEFERRED_CLOSE)

typedef struct _ETH_FILTER {
  PNDIS_SPIN_LOCK  Lock;
  CHAR  (*MCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
  struct _NDIS_MINIPORT_BLOCK  *Miniport;
  UINT  CombinedPacketFilter;
  PETH_BINDING_INFO  OpenList;
  ETH_ADDRESS_CHANGE  AddressChangeAction;
  ETH_FILTER_CHANGE  FilterChangeAction;
  ETH_DEFERRED_CLOSE  CloseAction;
  UINT  MaxMulticastAddresses;
  UINT  NumAddresses;
  UCHAR AdapterAddress[ETH_LENGTH_OF_ADDRESS];
  UINT  OldCombinedPacketFilter;
  CHAR  (*OldMCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
  UINT  OldNumAddresses;
  PETH_BINDING_INFO  DirectedList;
  PETH_BINDING_INFO  BMList;
  PETH_BINDING_INFO  MCastSet;
#if defined(_NDIS_)
  UINT  NumOpens;
  PVOID  BindListLock;
#endif
} ETH_FILTER, *PETH_FILTER;

typedef VOID DDKAPI
(*ETH_RCV_COMPLETE_HANDLER)(
  IN PETH_FILTER  Filter);

typedef VOID DDKAPI
(*ETH_RCV_INDICATE_HANDLER)(
  IN PETH_FILTER  Filter,
  IN NDIS_HANDLE  MacReceiveContext,
  IN PCHAR  Address,
  IN PVOID  HeaderBuffer,
  IN UINT  HeaderBufferSize,
  IN PVOID  LookaheadBuffer,
  IN UINT  LookaheadBufferSize,
  IN UINT  PacketSize);

typedef VOID DDKAPI
(*FDDI_RCV_COMPLETE_HANDLER)(
  IN PFDDI_FILTER  Filter);

typedef VOID DDKAPI
(*FDDI_RCV_INDICATE_HANDLER)(
  IN PFDDI_FILTER  Filter,
  IN NDIS_HANDLE  MacReceiveContext,
  IN PCHAR  Address,
  IN UINT  AddressLength,
  IN PVOID  HeaderBuffer,
  IN UINT  HeaderBufferSize,
  IN PVOID  LookaheadBuffer,
  IN UINT  LookaheadBufferSize,
  IN UINT  PacketSize);

typedef VOID DDKAPI
(*FILTER_PACKET_INDICATION_HANDLER)(
  IN NDIS_HANDLE  Miniport,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);

typedef VOID DDKAPI
(*TR_RCV_COMPLETE_HANDLER)(
  IN PTR_FILTER  Filter);

typedef VOID DDKAPI
(*TR_RCV_INDICATE_HANDLER)(
  IN PTR_FILTER  Filter,
  IN NDIS_HANDLE  MacReceiveContext,
  IN PVOID  HeaderBuffer,
  IN UINT  HeaderBufferSize,
  IN PVOID  LookaheadBuffer,
  IN UINT  LookaheadBufferSize,
  IN UINT  PacketSize);

typedef VOID DDKAPI
(*WAN_RCV_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_HANDLE  NdisLinkContext);

typedef VOID DDKAPI
(*WAN_RCV_HANDLER)(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_HANDLE  NdisLinkContext,
  IN PUCHAR  Packet,
  IN ULONG  PacketSize);

typedef VOID DDKFASTAPI
(*NDIS_M_DEQUEUE_WORK_ITEM)(
  IN PNDIS_MINIPORT_BLOCK  Miniport,
  IN NDIS_WORK_ITEM_TYPE  WorkItemType,
  OUT PVOID  *WorkItemContext);

typedef NDIS_STATUS DDKFASTAPI
(*NDIS_M_QUEUE_NEW_WORK_ITEM)(
  IN PNDIS_MINIPORT_BLOCK  Miniport,
  IN NDIS_WORK_ITEM_TYPE  WorkItemType,
  IN PVOID  WorkItemContext);

typedef NDIS_STATUS DDKFASTAPI
(*NDIS_M_QUEUE_WORK_ITEM)(
  IN PNDIS_MINIPORT_BLOCK  Miniport,
  IN NDIS_WORK_ITEM_TYPE  WorkItemType,
  IN PVOID  WorkItemContext);

typedef VOID DDKAPI
(*NDIS_M_REQ_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*NDIS_M_RESET_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_STATUS  Status,
  IN BOOLEAN  AddressingReset);

typedef VOID DDKAPI
(*NDIS_M_SEND_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN PNDIS_PACKET  Packet,
  IN NDIS_STATUS  Status);

typedef VOID DDKAPI
(*NDIS_M_SEND_RESOURCES_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle);

typedef BOOLEAN DDKFASTAPI
(*NDIS_M_START_SENDS)(
  IN PNDIS_MINIPORT_BLOCK  Miniport);

typedef VOID DDKAPI
(*NDIS_M_STATUS_HANDLER)(
  IN NDIS_HANDLE  MiniportHandle,
  IN NDIS_STATUS  GeneralStatus,
  IN PVOID  StatusBuffer,
  IN UINT  StatusBufferSize);

typedef VOID DDKAPI
(*NDIS_M_STS_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle);

typedef VOID DDKAPI
(*NDIS_M_TD_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN PNDIS_PACKET  Packet,
  IN NDIS_STATUS  Status,
  IN UINT  BytesTransferred);

typedef VOID (DDKAPI *NDIS_WM_SEND_COMPLETE_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN PVOID  Packet,
  IN NDIS_STATUS  Status);


#if ARCNET

#define ARC_SEND_BUFFERS                  8
#define ARC_HEADER_SIZE                   4

typedef struct _NDIS_ARC_BUF {
  NDIS_HANDLE  ArcnetBufferPool;
  PUCHAR  ArcnetLookaheadBuffer;
  UINT  NumFree;
  ARC_BUFFER_LIST ArcnetBuffers[ARC_SEND_BUFFERS];
} NDIS_ARC_BUF, *PNDIS_ARC_BUF;

#endif /* ARCNET */

#define NDIS_MINIPORT_WORK_QUEUE_SIZE 10

typedef struct _NDIS_LOG {
  PNDIS_MINIPORT_BLOCK  Miniport;
  KSPIN_LOCK  LogLock;
  PIRP  Irp;
  UINT  TotalSize;
  UINT  CurrentSize;
  UINT  InPtr;
  UINT  OutPtr;
  UCHAR  LogBuf[1];
} NDIS_LOG, *PNDIS_LOG;

typedef struct _FILTERDBS {
  _ANONYMOUS_UNION union {
    PETH_FILTER  EthDB;
    PNULL_FILTER  NullDB;
  } DUMMYUNIONNAME;
  PTR_FILTER  TrDB;
  PFDDI_FILTER  FddiDB;
#if ARCNET
  PARC_FILTER  ArcDB;
#else /* !ARCNET */
  PVOID  XXXDB;
#endif /* !ARCNET */
} FILTERDBS, *PFILTERDBS;


struct _NDIS_MINIPORT_BLOCK {
  PVOID  Signature;
  PNDIS_MINIPORT_BLOCK  NextMiniport;
  PNDIS_M_DRIVER_BLOCK  DriverHandle;
  NDIS_HANDLE  MiniportAdapterContext;
  UNICODE_STRING  MiniportName;
  PNDIS_BIND_PATHS  BindPaths;
  NDIS_HANDLE  OpenQueue;
  REFERENCE  Ref;
  NDIS_HANDLE  DeviceContext;
  UCHAR  Padding1;
  UCHAR  LockAcquired;
  UCHAR  PmodeOpens;
  UCHAR  AssignedProcessor;
  KSPIN_LOCK  Lock;
  PNDIS_REQUEST  MediaRequest;
  PNDIS_MINIPORT_INTERRUPT  Interrupt;
  ULONG  Flags;
  ULONG  PnPFlags;
  LIST_ENTRY  PacketList;
  PNDIS_PACKET  FirstPendingPacket;
  PNDIS_PACKET  ReturnPacketsQueue;
  ULONG  RequestBuffer;
  PVOID  SetMCastBuffer;
  PNDIS_MINIPORT_BLOCK  PrimaryMiniport;
  PVOID  WrapperContext;
  PVOID  BusDataContext;
  ULONG  PnPCapabilities;
  PCM_RESOURCE_LIST  Resources;
  NDIS_TIMER  WakeUpDpcTimer;
  UNICODE_STRING  BaseName;
  UNICODE_STRING  SymbolicLinkName;
  ULONG  CheckForHangSeconds;
  USHORT  CFHangTicks;
  USHORT  CFHangCurrentTick;
  NDIS_STATUS  ResetStatus;
  NDIS_HANDLE  ResetOpen;
  FILTERDBS  FilterDbs;
  FILTER_PACKET_INDICATION_HANDLER  PacketIndicateHandler;
  NDIS_M_SEND_COMPLETE_HANDLER  SendCompleteHandler;
  NDIS_M_SEND_RESOURCES_HANDLER  SendResourcesHandler;
  NDIS_M_RESET_COMPLETE_HANDLER  ResetCompleteHandler;
  NDIS_MEDIUM  MediaType;
  ULONG  BusNumber;
  NDIS_INTERFACE_TYPE  BusType;
  NDIS_INTERFACE_TYPE  AdapterType;
  PDEVICE_OBJECT  DeviceObject;
  PDEVICE_OBJECT  PhysicalDeviceObject;
  PDEVICE_OBJECT  NextDeviceObject;
  PMAP_REGISTER_ENTRY  MapRegisters;
  PNDIS_AF_LIST  CallMgrAfList;
  PVOID  MiniportThread;
  PVOID  SetInfoBuf;
  USHORT  SetInfoBufLen;
  USHORT  MaxSendPackets;
  NDIS_STATUS  FakeStatus;
  PVOID  LockHandler;
  PUNICODE_STRING  pAdapterInstanceName;
  PNDIS_MINIPORT_TIMER  TimerQueue;
  UINT  MacOptions;
  PNDIS_REQUEST  PendingRequest;
  UINT  MaximumLongAddresses;
  UINT  MaximumShortAddresses;
  UINT  CurrentLookahead;
  UINT  MaximumLookahead;
  W_HANDLE_INTERRUPT_HANDLER  HandleInterruptHandler;
  W_DISABLE_INTERRUPT_HANDLER  DisableInterruptHandler;
  W_ENABLE_INTERRUPT_HANDLER  EnableInterruptHandler;
  W_SEND_PACKETS_HANDLER  SendPacketsHandler;
  NDIS_M_START_SENDS  DeferredSendHandler;
  ETH_RCV_INDICATE_HANDLER  EthRxIndicateHandler;
  TR_RCV_INDICATE_HANDLER  TrRxIndicateHandler;
  FDDI_RCV_INDICATE_HANDLER  FddiRxIndicateHandler;
  ETH_RCV_COMPLETE_HANDLER  EthRxCompleteHandler;
  TR_RCV_COMPLETE_HANDLER  TrRxCompleteHandler;
  FDDI_RCV_COMPLETE_HANDLER  FddiRxCompleteHandler;
  NDIS_M_STATUS_HANDLER  StatusHandler;
  NDIS_M_STS_COMPLETE_HANDLER  StatusCompleteHandler;
  NDIS_M_TD_COMPLETE_HANDLER  TDCompleteHandler;
  NDIS_M_REQ_COMPLETE_HANDLER  QueryCompleteHandler;
  NDIS_M_REQ_COMPLETE_HANDLER  SetCompleteHandler;
  NDIS_WM_SEND_COMPLETE_HANDLER  WanSendCompleteHandler;
  WAN_RCV_HANDLER  WanRcvHandler;
  WAN_RCV_COMPLETE_HANDLER  WanRcvCompleteHandler;
#if defined(_NDIS_)
  PNDIS_MINIPORT_BLOCK  NextGlobalMiniport;
  SINGLE_LIST_ENTRY  WorkQueue[NUMBER_OF_WORK_ITEM_TYPES];
  SINGLE_LIST_ENTRY  SingleWorkItems[NUMBER_OF_SINGLE_WORK_ITEMS];
  UCHAR  SendFlags;
  UCHAR  TrResetRing;
  UCHAR  ArcnetAddress;
  UCHAR  XState;
  _ANONYMOUS_UNION union {
#if ARCNET
    PNDIS_ARC_BUF  ArcBuf;
#endif
    PVOID  BusInterface;
  } DUMMYUNIONNAME;
  PNDIS_LOG  Log;
  ULONG  SlotNumber;
  PCM_RESOURCE_LIST  AllocatedResources;
  PCM_RESOURCE_LIST  AllocatedResourcesTranslated;
  SINGLE_LIST_ENTRY  PatternList;
  NDIS_PNP_CAPABILITIES  PMCapabilities;
  DEVICE_CAPABILITIES  DeviceCaps;
  ULONG  WakeUpEnable;
  DEVICE_POWER_STATE  CurrentDevicePowerState;
  PIRP  pIrpWaitWake;
  SYSTEM_POWER_STATE  WaitWakeSystemState;
  LARGE_INTEGER  VcIndex;
  KSPIN_LOCK  VcCountLock;
  LIST_ENTRY  WmiEnabledVcs;
  PNDIS_GUID  pNdisGuidMap;
  PNDIS_GUID  pCustomGuidMap;
  USHORT  VcCount;
  USHORT  cNdisGuidMap;
  USHORT  cCustomGuidMap;
  USHORT  CurrentMapRegister;
  PKEVENT  AllocationEvent;
  USHORT  BaseMapRegistersNeeded;
  USHORT  SGMapRegistersNeeded;
  ULONG  MaximumPhysicalMapping;
  NDIS_TIMER  MediaDisconnectTimer;
  USHORT  MediaDisconnectTimeOut;
  USHORT  InstanceNumber;
  NDIS_EVENT  OpenReadyEvent;
  NDIS_PNP_DEVICE_STATE  PnPDeviceState;
  NDIS_PNP_DEVICE_STATE  OldPnPDeviceState;
  PGET_SET_DEVICE_DATA  SetBusData;
  PGET_SET_DEVICE_DATA  GetBusData;
  KDPC  DeferredDpc;
#if 0
  /* FIXME: */
  NDIS_STATS  NdisStats;
#else
  ULONG  NdisStats;
#endif
  PNDIS_PACKET  IndicatedPacket[MAXIMUM_PROCESSORS];
  PKEVENT  RemoveReadyEvent;
  PKEVENT  AllOpensClosedEvent;
  PKEVENT  AllRequestsCompletedEvent;
  ULONG  InitTimeMs;
  NDIS_MINIPORT_WORK_ITEM  WorkItemBuffer[NUMBER_OF_SINGLE_WORK_ITEMS];
  PDMA_ADAPTER  SystemAdapterObject;
  ULONG  DriverVerifyFlags;
  POID_LIST  OidList;
  USHORT  InternalResetCount;
  USHORT  MiniportResetCount;
  USHORT  MediaSenseConnectCount;
  USHORT  MediaSenseDisconnectCount;
  PNDIS_PACKET  *xPackets;
  ULONG  UserModeOpenReferences;
  _ANONYMOUS_UNION union {
    PVOID  SavedSendHandler;
    PVOID  SavedWanSendHandler;
  } DUMMYUNIONNAME2;
  PVOID  SavedSendPacketsHandler;
  PVOID  SavedCancelSendPacketsHandler;
  W_SEND_PACKETS_HANDLER  WSendPacketsHandler;                
  ULONG  MiniportAttributes;
  PDMA_ADAPTER  SavedSystemAdapterObject;
  USHORT  NumOpens;
  USHORT  CFHangXTicks; 
  ULONG  RequestCount;
  ULONG  IndicatedPacketsCount;
  ULONG  PhysicalMediumType;
  PNDIS_REQUEST  LastRequest;
  LONG  DmaAdapterRefCount;
  PVOID  FakeMac;
  ULONG  LockDbg;
  ULONG  LockDbgX;
  PVOID  LockThread;
  ULONG  InfoFlags;
  KSPIN_LOCK  TimerQueueLock;
  PKEVENT  ResetCompletedEvent;
  PKEVENT  QueuedBindingCompletedEvent;
  PKEVENT  DmaResourcesReleasedEvent;
  FILTER_PACKET_INDICATION_HANDLER  SavedPacketIndicateHandler;
  ULONG  RegisteredInterrupts;
  PNPAGED_LOOKASIDE_LIST  SGListLookasideList;
  ULONG  ScatterGatherListSize;
#endif /* _NDIS_ */
};


/* Handler prototypes for NDIS_OPEN_BLOCK */

typedef NDIS_STATUS (DDKAPI *WAN_SEND_HANDLER)(
  IN NDIS_HANDLE  MacBindingHandle,
  IN NDIS_HANDLE  LinkHandle,
  IN PVOID  Packet);

/* NDIS 4.0 extension */

typedef VOID (DDKAPI *SEND_PACKETS_HANDLER)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);


typedef struct _NDIS_COMMON_OPEN_BLOCK {
  PVOID  MacHandle;
  NDIS_HANDLE  BindingHandle;
  PNDIS_MINIPORT_BLOCK  MiniportHandle;
  PNDIS_PROTOCOL_BLOCK  ProtocolHandle;
  NDIS_HANDLE  ProtocolBindingContext;
  PNDIS_OPEN_BLOCK  MiniportNextOpen;
  PNDIS_OPEN_BLOCK  ProtocolNextOpen;
  NDIS_HANDLE  MiniportAdapterContext;
  BOOLEAN  Reserved1;
  BOOLEAN  Reserved2;
  BOOLEAN  Reserved3;
  BOOLEAN  Reserved4;
  PNDIS_STRING  BindDeviceName;
  KSPIN_LOCK  Reserved5;
  PNDIS_STRING  RootDeviceName;
  _ANONYMOUS_UNION union {
    SEND_HANDLER  SendHandler;
    WAN_SEND_HANDLER  WanSendHandler;
  } DUMMYUNIONNAME;
  TRANSFER_DATA_HANDLER  TransferDataHandler;
  SEND_COMPLETE_HANDLER  SendCompleteHandler;
  TRANSFER_DATA_COMPLETE_HANDLER  TransferDataCompleteHandler;
  RECEIVE_HANDLER  ReceiveHandler;
  RECEIVE_COMPLETE_HANDLER  ReceiveCompleteHandler;
  WAN_RECEIVE_HANDLER  WanReceiveHandler;
  REQUEST_COMPLETE_HANDLER  RequestCompleteHandler;
  RECEIVE_PACKET_HANDLER  ReceivePacketHandler;
  SEND_PACKETS_HANDLER  SendPacketsHandler;
  RESET_HANDLER  ResetHandler;
  REQUEST_HANDLER  RequestHandler;
  RESET_COMPLETE_HANDLER  ResetCompleteHandler;
  STATUS_HANDLER  StatusHandler;
  STATUS_COMPLETE_HANDLER  StatusCompleteHandler;
#if defined(_NDIS_)
  ULONG  Flags;
  ULONG  References;
  KSPIN_LOCK  SpinLock;
  NDIS_HANDLE  FilterHandle;
  ULONG  ProtocolOptions;
  USHORT  CurrentLookahead;
  USHORT  ConnectDampTicks;
  USHORT  DisconnectDampTicks;
  W_SEND_HANDLER  WSendHandler;
  W_TRANSFER_DATA_HANDLER  WTransferDataHandler;
  W_SEND_PACKETS_HANDLER  WSendPacketsHandler;
  W_CANCEL_SEND_PACKETS_HANDLER  CancelSendPacketsHandler;
  ULONG  WakeUpEnable;
  PKEVENT  CloseCompleteEvent;
  QUEUED_CLOSE  QC;
  ULONG  AfReferences;
  PNDIS_OPEN_BLOCK  NextGlobalOpen;
#endif /* _NDIS_ */
} NDIS_COMMON_OPEN_BLOCK;

struct _NDIS_OPEN_BLOCK
{
    NDIS_COMMON_OPEN_BLOCK NdisCommonOpenBlock;
#if defined(_NDIS_)
    struct _NDIS_OPEN_CO
    {
        struct _NDIS_CO_AF_BLOCK *  NextAf;
        W_CO_CREATE_VC_HANDLER      MiniportCoCreateVcHandler;
        W_CO_REQUEST_HANDLER        MiniportCoRequestHandler;
        CO_CREATE_VC_HANDLER        CoCreateVcHandler;
        CO_DELETE_VC_HANDLER        CoDeleteVcHandler;
        PVOID                       CmActivateVcCompleteHandler;
        PVOID                       CmDeactivateVcCompleteHandler;
        PVOID                       CoRequestCompleteHandler;
        LIST_ENTRY                  ActiveVcHead;
        LIST_ENTRY                  InactiveVcHead;
        LONG                        PendingAfNotifications;
        PKEVENT                     AfNotifyCompleteEvent;
    };
#endif /* _NDIS_ */
};



/* Routines for NDIS miniport drivers */

NDISAPI
VOID
DDKAPI
NdisInitializeWrapper(
  OUT PNDIS_HANDLE  NdisWrapperHandle,
  IN PVOID  SystemSpecific1,
  IN PVOID  SystemSpecific2,
  IN PVOID  SystemSpecific3);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMAllocateMapRegisters(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN UINT  DmaChannel,
  IN BOOLEAN  Dma32BitAddresses,
  IN ULONG  PhysicalMapRegistersNeeded,
  IN ULONG  MaximumPhysicalMapping);

/*
 * VOID
 * NdisMArcIndicateReceive(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN PUCHAR  HeaderBuffer,
 *   IN PUCHAR  DataBuffer,
 *   IN UINT  Length);
 */
#define NdisMArcIndicateReceive(MiniportAdapterHandle, \
                                HeaderBuffer,          \
                                DataBuffer,            \
                                Length)                \
{                                                      \
    ArcFilterDprIndicateReceive(                       \
        (((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FilterDbs.ArcDB), \
        (HeaderBuffer), \
        (DataBuffer),   \
        (Length));      \
}

/*
 * VOID
 * NdisMArcIndicateReceiveComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle);
 */
#define NdisMArcIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                              \
    if (((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->EthDB)  \
	    {                                                        \
	        NdisMEthIndicateReceiveComplete(_H);                 \
	    }                                                        \
                                                               \
    ArcFilterDprIndicateReceiveComplete(                       \
      ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->ArcDB);   \
}

NDISAPI
VOID
DDKAPI
NdisMCloseLog(
  IN NDIS_HANDLE  LogHandle);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMCreateLog(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN UINT  Size,
  OUT PNDIS_HANDLE  LogHandle);

NDISAPI
VOID
DDKAPI
NdisMDeregisterAdapterShutdownHandler(
  IN NDIS_HANDLE  MiniportHandle);

NDISAPI
VOID
DDKAPI
NdisMDeregisterInterrupt(
  IN PNDIS_MINIPORT_INTERRUPT  Interrupt);

NDISAPI
VOID
DDKAPI
NdisMDeregisterIoPortRange(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN UINT  InitialPort,
  IN UINT  NumberOfPorts,
  IN PVOID  PortOffset);

/*
 * VOID
 * NdisMEthIndicateReceive(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN NDIS_HANDLE  MiniportReceiveContext,
 *   IN PVOID  HeaderBuffer,
 *   IN UINT  HeaderBufferSize,
 *   IN PVOID  LookaheadBuffer,
 *   IN UINT  LookaheadBufferSize,
 *   IN UINT  PacketSize);
 */
#define NdisMEthIndicateReceive(MiniportAdapterHandle,  \
                                MiniportReceiveContext, \
                                HeaderBuffer,           \
                                HeaderBufferSize,       \
                                LookaheadBuffer,        \
                                LookaheadBufferSize,    \
                                PacketSize)             \
{                                                       \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->EthRxIndicateHandler)( \
        (((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FilterDbs.EthDB), \
		(MiniportReceiveContext), \
		(HeaderBuffer),           \
		(HeaderBuffer),           \
		(HeaderBufferSize),       \
		(LookaheadBuffer),        \
		(LookaheadBufferSize),    \
		(PacketSize));            \
}

/*
 * VOID
 * NdisMEthIndicateReceiveComplete(
 *   IN NDIS_HANDLE MiniportAdapterHandle);
 */
#define NdisMEthIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                              \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->EthRxCompleteHandler)( \
        ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.EthDB);    \
}

/*
 * VOID
 * NdisMFddiIndicateReceive(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN NDIS_HANDLE  MiniportReceiveContext,
 *   IN PVOID  HeaderBuffer,
 *   IN UINT  HeaderBufferSize,
 *   IN PVOID  LookaheadBuffer,
 *   IN UINT  LookaheadBufferSize,
 *   IN UINT  PacketSize);
 */
#define NdisMFddiIndicateReceive(MiniportAdapterHandle,  \
                                 MiniportReceiveContext, \
                                 HeaderBuffer,           \
                                 HeaderBufferSize,       \
                                 LookaheadBuffer,        \
                                 LookaheadBufferSize,    \
                                 PacketSize)             \
{                                                        \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FddiRxIndicateHandler)( \
        (((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FilterDbs.FddiDB),   \
        (MiniportReceiveContext),              \
        (PUCHAR)(HeaderBuffer) + 1,            \
        (((*(PUCHAR*)(HeaderBuffer)) & 0x40) ? \
            FDDI_LENGTH_OF_LONG_ADDRESS :      \
		    FDDI_LENGTH_OF_SHORT_ADDRESS),     \
        (HeaderBuffer),                        \
        (HeaderBufferSize),                    \
        (LookaheadBuffer),                     \
        (LookaheadBufferSize),                 \
        (PacketSize));                         \
}



/*
 * VOID
 * NdisMFddiIndicateReceiveComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle);
 */
#define NdisMFddiIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                               \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FddiRxCompleteHandler)( \
        ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.FddiDB);      \
}

NDISAPI
VOID
DDKAPI
NdisMFlushLog(
  IN NDIS_HANDLE  LogHandle);

NDISAPI
VOID
DDKAPI
NdisMFreeMapRegisters(
  IN NDIS_HANDLE  MiniportAdapterHandle);

/*
 * VOID
 * NdisMIndicateStatus(
 *  IN NDIS_HANDLE  MiniportAdapterHandle,
 *  IN NDIS_STATUS  GeneralStatus,
 *  IN PVOID  StatusBuffer,
 *  IN UINT  StatusBufferSize);
 */

#define NdisMIndicateStatus(MiniportAdapterHandle,  \
   GeneralStatus, StatusBuffer, StatusBufferSize)   \
  (*((PNDIS_MINIPORT_BLOCK)(_M))->StatusHandler)(   \
  MiniportAdapterHandle, GeneralStatus, StatusBuffer, StatusBufferSize)

/*
 * VOID
 * NdisMIndicateStatusComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle);
 */
#define NdisMIndicateStatusComplete(MiniportAdapterHandle) \
  (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->StatusCompleteHandler)( \
    MiniportAdapterHandle)

/*
 * VOID
 * NdisMInitializeWrapper(
 *   OUT PNDIS_HANDLE  NdisWrapperHandle,
 *   IN PVOID  SystemSpecific1,
 *   IN PVOID  SystemSpecific2,
 *   IN PVOID  SystemSpecific3);
 */
#define NdisMInitializeWrapper(NdisWrapperHandle, \
                               SystemSpecific1,   \
                               SystemSpecific2,   \
                               SystemSpecific3)   \
    NdisInitializeWrapper((NdisWrapperHandle),    \
                          (SystemSpecific1),      \
                          (SystemSpecific2),      \
                          (SystemSpecific3))

NDISAPI
NDIS_STATUS
DDKAPI
NdisMMapIoSpace(
  OUT PVOID  *VirtualAddress,
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_PHYSICAL_ADDRESS  PhysicalAddress,
  IN UINT  Length);

/*
 * VOID
 * NdisMQueryInformationComplete(
 *  IN NDIS_HANDLE  MiniportAdapterHandle,
 *  IN NDIS_STATUS  Status);
 */
#define NdisMQueryInformationComplete(MiniportAdapterHandle, Status) \
  (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->QueryCompleteHandler)(MiniportAdapterHandle, Status)

NDISAPI
VOID
DDKAPI
NdisMRegisterAdapterShutdownHandler(
  IN NDIS_HANDLE  MiniportHandle,
  IN PVOID  ShutdownContext,
  IN ADAPTER_SHUTDOWN_HANDLER  ShutdownHandler);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMRegisterInterrupt(
  OUT PNDIS_MINIPORT_INTERRUPT  Interrupt,
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN UINT  InterruptVector,
  IN UINT  InterruptLevel,
  IN BOOLEAN  RequestIsr,
  IN BOOLEAN  SharedInterrupt,
  IN NDIS_INTERRUPT_MODE  InterruptMode);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMRegisterIoPortRange(
  OUT PVOID  *PortOffset,
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN UINT  InitialPort,
  IN UINT  NumberOfPorts);

NDISAPI
NDIS_STATUS
DDKAPI
NdisMRegisterMiniport(
  IN NDIS_HANDLE  NdisWrapperHandle,
  IN PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
  IN UINT  CharacteristicsLength);


#if !defined(_NDIS_)

/*
 * VOID
 * NdisMResetComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN NDIS_STATUS  Status,
 *   IN BOOLEAN  AddressingReset);
 */
#define	NdisMResetComplete(MiniportAdapterHandle, \
                           Status,                \
                           AddressingReset)       \
{                                                 \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->ResetCompleteHandler)( \
        MiniportAdapterHandle, Status, AddressingReset); \
}

/*
 * VOID
 * NdisMSendComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN PNDIS_PACKET  Packet,
 *   IN NDIS_STATUS  Status);
 */
#define	NdisMSendComplete(MiniportAdapterHandle, \
                          Packet,                \
                          Status)                \
{                                                \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->SendCompleteHandler)( \
        MiniportAdapterHandle, Packet, Status);  \
}

/*
 * VOID
 * NdisMSendResourcesAvailable(
 *   IN NDIS_HANDLE  MiniportAdapterHandle);
 */
#define	NdisMSendResourcesAvailable(MiniportAdapterHandle) \
{                                                \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->SendResourcesHandler)( \
        MiniportAdapterHandle); \
}

/*
 * VOID
 * NdisMTransferDataComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN PNDIS_PACKET  Packet,
 *   IN NDIS_STATUS  Status,
 *   IN UINT  BytesTransferred);
 */
#define	NdisMTransferDataComplete(MiniportAdapterHandle, \
                                  Packet,                \
                                  Status,                \
                                  BytesTransferred)      \
{                                                        \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->TDCompleteHandler)( \
        MiniportAdapterHandle, Packet, Status, BytesTransferred)           \
}

#endif /* !_NDIS_ */


/*
 * VOID
 * NdisMSetAttributes(
 *  IN NDIS_HANDLE  MiniportAdapterHandle,
 *  IN NDIS_HANDLE  MiniportAdapterContext,
 *  IN BOOLEAN  BusMaster,
 *  IN NDIS_INTERFACE_TYPE  AdapterType);
 */
#define NdisMSetAttributes(MiniportAdapterHandle,   \
                           MiniportAdapterContext,  \
                           BusMaster,               \
                           AdapterType)             \
  NdisMSetAttributesEx(MiniportAdapterHandle,       \
    MiniportAdapterContext,                         \
    0,                                              \
    (BusMaster) ? NDIS_ATTRIBUTE_BUS_MASTER : 0,    \
    AdapterType)

NDISAPI
VOID 
DDKAPI
NdisMSetAttributesEx(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN UINT  CheckForHangTimeInSeconds   OPTIONAL,
  IN ULONG  AttributeFlags,
  IN NDIS_INTERFACE_TYPE AdapterType); 

/*
 * VOID
 * NdisMSetInformationComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN NDIS_STATUS  Status);
 */
#define NdisMSetInformationComplete(MiniportAdapterHandle, \
                                    Status) \
  (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->SetCompleteHandler)( \
    MiniportAdapterHandle, Status)

NDISAPI
VOID
DDKAPI
NdisMSleep(
  IN ULONG  MicrosecondsToSleep);

NDISAPI
BOOLEAN
DDKAPI
NdisMSynchronizeWithInterrupt(
  IN PNDIS_MINIPORT_INTERRUPT  Interrupt,
  IN PVOID  SynchronizeFunction,
  IN PVOID  SynchronizeContext);

/*
 * VOID
 * NdisMTrIndicateReceive(
 *   IN NDIS_HANDLE  MiniportAdapterHandle,
 *   IN NDIS_HANDLE  MiniportReceiveContext,
 *   IN PVOID  HeaderBuffer,
 *   IN UINT  HeaderBufferSize,
 *   IN PVOID  LookaheadBuffer,
 *   IN UINT  LookaheadBufferSize,
 *   IN UINT  PacketSize);
 */
#define NdisMTrIndicateReceive(MiniportAdapterHandle,  \
                               MiniportReceiveContext, \
                               HeaderBuffer,           \
                               HeaderBufferSize,       \
                               LookaheadBuffer,        \
                               LookaheadBufferSize,    \
                               PacketSize)             \
{                                                      \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->TrRxIndicateHandler)( \
      (((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FilterDbs.TrDB),     \
		(MiniportReceiveContext), \
		(HeaderBuffer),           \
		(HeaderBuffer),           \
		(HeaderBufferSize),       \
		(LookaheadBuffer),        \
		(LookaheadBufferSize),    \
		(PacketSize));            \
}

/*
 * VOID
 * NdisMTrIndicateReceiveComplete(
 *   IN NDIS_HANDLE  MiniportAdapterHandle);
 */
#define NdisMTrIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                             \
	(*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->TrRxCompleteHandler)( \
    ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.TrDB);    \
}

NDISAPI
NDIS_STATUS
DDKAPI
NdisMWriteLogData(
  IN NDIS_HANDLE  LogHandle,
  IN PVOID  LogBuffer,
  IN UINT  LogBufferSize);

NDISAPI
VOID
DDKAPI
NdisMQueryAdapterResources(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  WrapperConfigurationContext,
  OUT PNDIS_RESOURCE_LIST  ResourceList,
  IN OUT PUINT  BufferSize);

NDISAPI
VOID
DDKAPI
NdisTerminateWrapper(
  IN NDIS_HANDLE  NdisWrapperHandle,
  IN PVOID  SystemSpecific);

NDISAPI
VOID
DDKAPI
NdisMUnmapIoSpace(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN PVOID  VirtualAddress,
  IN UINT  Length);



/* NDIS intermediate miniport structures */

typedef VOID (DDKAPI *W_MINIPORT_CALLBACK)(
  IN NDIS_HANDLE  MiniportAdapterContext,
  IN PVOID  CallbackContext);



/* Routines for intermediate miniport drivers */

NDISAPI
NDIS_STATUS
DDKAPI
NdisIMDeInitializeDeviceInstance(
  IN NDIS_HANDLE NdisMiniportHandle);

/*
 * NDIS_STATUS
 * NdisIMInitializeDeviceInstance(
 *   IN NDIS_HANDLE  DriverHandle,
 *   IN PNDIS_STRING  DeviceInstance);
 */
#define NdisIMInitializeDeviceInstance(DriverHandle, DeviceInstance) \
  NdisIMInitializeDeviceInstanceEx(DriverHandle, DeviceInstance, NULL)

NDISAPI
NDIS_STATUS
DDKAPI
NdisIMRegisterLayeredMiniport(
  IN NDIS_HANDLE  NdisWrapperHandle,
  IN PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
  IN UINT  CharacteristicsLength,
  OUT PNDIS_HANDLE  DriverHandle);


/* Functions obsoleted by NDIS 5.0 */

NDISAPI
VOID
DDKAPI
NdisFreeDmaChannel(
  IN PNDIS_HANDLE  NdisDmaHandle);

NDISAPI
VOID
DDKAPI
NdisSetupDmaTransfer(
  OUT PNDIS_STATUS  Status,
  IN PNDIS_HANDLE  NdisDmaHandle,
  IN PNDIS_BUFFER  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length,
  IN BOOLEAN  WriteToDevice);

NDISAPI
NTSTATUS
DDKAPI
NdisUpcaseUnicodeString(
  OUT PUNICODE_STRING  DestinationString,  
  IN PUNICODE_STRING  SourceString);


/* Routines for NDIS protocol drivers */

NDISAPI
VOID
DDKAPI
NdisRequest(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisBindingHandle,
  IN PNDIS_REQUEST  NdisRequest);

NDISAPI
VOID
DDKAPI
NdisReset(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisBindingHandle);

NDISAPI
VOID
DDKAPI
NdisSend(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisBindingHandle,
  IN PNDIS_PACKET  Packet);

NDISAPI
VOID
DDKAPI
NdisSendPackets(
  IN NDIS_HANDLE  NdisBindingHandle,
  IN PPNDIS_PACKET  PacketArray,
  IN UINT  NumberOfPackets);

NDISAPI
VOID
DDKAPI
NdisTransferData(
  OUT PNDIS_STATUS        Status,
  IN NDIS_HANDLE  NdisBindingHandle,
  IN NDIS_HANDLE  MacReceiveContext,
  IN UINT  ByteOffset,
  IN UINT  BytesToTransfer,
  IN OUT PNDIS_PACKET  Packet,
  OUT PUINT  BytesTransferred);

NDISAPI
VOID
DDKAPI
NdisCloseAdapter(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisBindingHandle);

NDISAPI
VOID
DDKAPI
NdisCompleteBindAdapter(
  IN NDIS_HANDLE  BindAdapterContext,
  IN NDIS_STATUS  Status,
  IN NDIS_STATUS  OpenStatus);

NDISAPI
VOID
DDKAPI
NdisCompleteUnbindAdapter(
  IN NDIS_HANDLE  UnbindAdapterContext,
  IN NDIS_STATUS  Status);

NDISAPI
VOID
DDKAPI
NdisDeregisterProtocol(
  OUT PNDIS_STATUS  Status,
  IN NDIS_HANDLE  NdisProtocolHandle);

NDISAPI
VOID
DDKAPI
NdisOpenAdapter(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_STATUS  OpenErrorStatus,
  OUT PNDIS_HANDLE  NdisBindingHandle,
  OUT PUINT  SelectedMediumIndex,
  IN PNDIS_MEDIUM  MediumArray,
  IN UINT  MediumArraySize,
  IN NDIS_HANDLE  NdisProtocolHandle,
  IN NDIS_HANDLE  ProtocolBindingContext,
  IN PNDIS_STRING  AdapterName,
  IN UINT  OpenOptions,
  IN PSTRING  AddressingInformation);

NDISAPI
VOID
DDKAPI
NdisOpenProtocolConfiguration(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_HANDLE  ConfigurationHandle,
  IN PNDIS_STRING  ProtocolSection);

NDISAPI
VOID
DDKAPI
NdisRegisterProtocol(
  OUT PNDIS_STATUS  Status,
  OUT PNDIS_HANDLE  NdisProtocolHandle,
  IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
  IN UINT  CharacteristicsLength);

/* Obsoleted in Windows XP */

/* Prototypes for NDIS_MAC_CHARACTERISTICS */

typedef NDIS_STATUS (*OPEN_ADAPTER_HANDLER)(
  OUT PNDIS_STATUS  OpenErrorStatus,
  OUT NDIS_HANDLE  *MacBindingHandle,
  OUT PUINT  SelectedMediumIndex,
  IN PNDIS_MEDIUM  MediumArray,
  IN UINT  MediumArraySize,
  IN NDIS_HANDLE  NdisBindingContext,
  IN NDIS_HANDLE  MacAdapterContext,
  IN UINT  OpenOptions,
  IN PSTRING  AddressingInformation  OPTIONAL);

typedef NDIS_STATUS (DDKAPI *CLOSE_ADAPTER_HANDLER)(
  IN NDIS_HANDLE  MacBindingHandle);

typedef NDIS_STATUS (DDKAPI *WAN_TRANSFER_DATA_HANDLER)(
  VOID);

typedef NDIS_STATUS (DDKAPI *QUERY_GLOBAL_STATISTICS_HANDLER)(
  IN NDIS_HANDLE  MacAdapterContext,
  IN PNDIS_REQUEST  NdisRequest);

typedef VOID (DDKAPI *UNLOAD_MAC_HANDLER)(
  IN NDIS_HANDLE  MacMacContext);

typedef NDIS_STATUS (DDKAPI *ADD_ADAPTER_HANDLER)(
  IN NDIS_HANDLE  MacMacContext,
  IN NDIS_HANDLE  WrapperConfigurationContext,
  IN PNDIS_STRING  AdapterName);

typedef VOID (*REMOVE_ADAPTER_HANDLER)(
  IN NDIS_HANDLE  MacAdapterContext);

typedef struct _NDIS_MAC_CHARACTERISTICS {
  UCHAR  MajorNdisVersion;
  UCHAR  MinorNdisVersion;
  UINT  Reserved;
  OPEN_ADAPTER_HANDLER  OpenAdapterHandler;
  CLOSE_ADAPTER_HANDLER  CloseAdapterHandler;
  SEND_HANDLER  SendHandler;
  TRANSFER_DATA_HANDLER  TransferDataHandler;
  RESET_HANDLER  ResetHandler;
  REQUEST_HANDLER  RequestHandler;
  QUERY_GLOBAL_STATISTICS_HANDLER  QueryGlobalStatisticsHandler;
  UNLOAD_MAC_HANDLER  UnloadMacHandler;
  ADD_ADAPTER_HANDLER  AddAdapterHandler;
  REMOVE_ADAPTER_HANDLER  RemoveAdapterHandler;
  NDIS_STRING  Name;
} NDIS_MAC_CHARACTERISTICS, *PNDIS_MAC_CHARACTERISTICS;

typedef	NDIS_MAC_CHARACTERISTICS        NDIS_WAN_MAC_CHARACTERISTICS;
typedef	NDIS_WAN_MAC_CHARACTERISTICS    *PNDIS_WAN_MAC_CHARACTERISTICS;

#ifdef __cplusplus
}
#endif

#endif /* __NDIS_H */

/* EOF */
