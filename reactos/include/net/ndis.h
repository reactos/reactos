/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        include/net/ndis.h
 * PURPOSE:     Structures used by NDIS drivers
 * DEFINES:     i386                 - Target platform is i386
 *              NDIS_WRAPPER         - Define only for NDIS wrapper library
 *              NDIS_MINIPORT_DRIVER - Define only for NDIS miniport drivers
 *              BINARY_COMPATIBLE    - 0 = Use macros for some features
 *                                   - 1 = Use imports for features not available
 *              NDIS40               - Use NDIS 4.0 structures by default
 *              NDIS50               - Use NDIS 5.0 structures by default
 */
#ifndef __NDIS_H
#define __NDIS_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef NDIS50
#undef NDIS40
#define NDIS40
#endif


/* Windows 9x compatibillity for miniports on x86 platform */
#ifndef BINARY_COMPATIBLE
#if defined(NDIS_MINIPORT_DRIVER) && defined(i386)
#define BINARY_COMPATIBLE 1
#else
#define BINARY_COMPATIBLE 0
#endif
#endif

#ifndef UNALIGNED
#define UNALIGNED
#endif

#ifndef FASTCALL
#define FASTCALL  __attribute__((fastcall))
#endif

/* The NDIS library export functions. NDIS miniport drivers import functions */
#ifdef NDIS_WRAPPER

#ifdef _MSC_VER
#define EXPIMP __declspec(dllexport)
#else
#define EXPIMP STDCALL
#endif

#else /* NDIS_WRAPPER */

#ifdef _MSC_VER
#define EXPIMP __declspec(dllimport)
#else
#define EXPIMP STDCALL
#endif

#endif /* NDIS_WRAPPER */



#ifdef NDIS_MINIPORT_DRIVER

#include "miniport.h"

#else /* NDIS_MINIPORT_DRIVER */

#ifdef _MSC_VER
#include <ntddk.h>

typedef ULONG ULONG_PTR, *PULONG_PTR;

#else /* _MSC_VER */
#include <ddk/ntddk.h>

/* FIXME: Missed some definitions in there */

typedef struct _DMA_CONFIGURATION_BYTE0
{
    UCHAR   Channel:3;
    UCHAR   Reserved:3;
    UCHAR   Shared:1;
    UCHAR   MoreEntries:1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1
{
    UCHAR   Reserved0:2;
    UCHAR   TransferSize:2;
    UCHAR   Timing:2;
    UCHAR   Reserved1:2;
} DMA_CONFIGURATION_BYTE1;


typedef struct _CM_MCA_POS_DATA
{
    USHORT  AdapterId;
    UCHAR   PosData1;
    UCHAR   PosData2;
    UCHAR   PosData3;
    UCHAR   PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;

typedef struct _EISA_MEMORY_TYPE
{
    UCHAR   ReadWrite:1;
    UCHAR   Cached:1;
    UCHAR   Reserved0:1;
    UCHAR   Type:2;
    UCHAR   Shared:1;
    UCHAR   Reserved1:1;
    UCHAR   MoreEntries:1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

typedef struct _EISA_MEMORY_CONFIGURATION
{
    EISA_MEMORY_TYPE    ConfigurationByte;
    UCHAR   DataSize;
    USHORT  AddressLowWord;
    UCHAR   AddressHighByte;
    USHORT  MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;


typedef struct _EISA_IRQ_DESCRIPTOR
{
    UCHAR   Interrupt:4;
    UCHAR   Reserved:1;
    UCHAR   LevelTriggered:1;
    UCHAR   Shared:1;
    UCHAR   MoreEntries:1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION
{
    EISA_IRQ_DESCRIPTOR ConfigurationByte;
    UCHAR   Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;

typedef struct _EISA_DMA_CONFIGURATION
{
    DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
    DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;


typedef struct _EISA_PORT_DESCRIPTOR
{
    UCHAR   NumberPorts:5;
    UCHAR   Reserved:1;
    UCHAR   Shared:1;
    UCHAR   MoreEntries:1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION
{
    EISA_PORT_DESCRIPTOR    Configuration;
    USHORT  PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;

typedef struct _CM_EISA_SLOT_INFORMATION
{
    UCHAR   ReturnCode;
    UCHAR   ReturnFlags;
    UCHAR   MajorRevision;
    UCHAR   MinorRevision;
    USHORT  Checksum;
    UCHAR   NumberFunctions;
    UCHAR   FunctionInformation;
    ULONG   CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;

typedef struct _CM_EISA_FUNCTION_INFORMATION
{
    ULONG   CompressedId;
    UCHAR   IdSlotFlags1;
    UCHAR   IdSlotFlags2;
    UCHAR   MinorRevision;
    UCHAR   MajorRevision;
    UCHAR   Selections[26];
    UCHAR   FunctionFlags;
    UCHAR   TypeString[80];
    EISA_MEMORY_CONFIGURATION   EisaMemory[9];
    EISA_IRQ_CONFIGURATION      EisaIrq[7];
    EISA_DMA_CONFIGURATION      EisaDma[4];
    EISA_PORT_CONFIGURATION     EisaPort[20];
    UCHAR   InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;

#endif /* _MSC_VER */

/* FIXME: Missed some definitions in there */

typedef CONST CHAR *PCSTR;

#endif /* NDIS_MINIPORT_DRIVER */

#include "netevent.h"
#include "ndisoid.h"
#include "ntddndis.h"


#if defined(NDIS_MINIPORT_DRIVER) || !defined(_MSC_VER)

#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID {
    ULONG   Data1;
    USHORT  Data2;
    USHORT  Data3;
    UCHAR   Data4[8];
} GUID;

#endif /* GUID_DEFINED */

#endif /* NDIS_MINIPORT_DRIVER || _MSC_VER */


/* NDIS base types */

typedef struct _NDIS_SPIN_LOCK
{
    KSPIN_LOCK  SpinLock;
    KIRQL       OldIrql;
} NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

typedef struct _NDIS_EVENT
{
    KEVENT  Event;
} NDIS_EVENT, *PNDIS_EVENT;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;
typedef int NDIS_STATUS, *PNDIS_STATUS;

typedef UNICODE_STRING NDIS_STRING, *PNDIS_STRING;

typedef PCSTR NDIS_ANSI_STRING, *PNDIS_ANSI_STRING;

typedef MDL NDIS_BUFFER, *PNDIS_BUFFER;

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
#define	NDIS_STATUS_WW_INDICATION               ((NDIS_STATUS)0x40010012L)
#define NDIS_STATUS_TAPI_INDICATION             ((NDIS_STATUS)0x40010080L)

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


/* NDIS error codes for error logging */

#define NDIS_ERROR_CODE ULONG

#define NDIS_ERROR_CODE_RESOURCE_CONFLICT			EVENT_NDIS_RESOURCE_CONFLICT
#define NDIS_ERROR_CODE_OUT_OF_RESOURCES			EVENT_NDIS_OUT_OF_RESOURCE
#define NDIS_ERROR_CODE_HARDWARE_FAILURE			EVENT_NDIS_HARDWARE_FAILURE
#define NDIS_ERROR_CODE_ADAPTER_NOT_FOUND			EVENT_NDIS_ADAPTER_NOT_FOUND
#define NDIS_ERROR_CODE_INTERRUPT_CONNECT			EVENT_NDIS_INTERRUPT_CONNECT
#define NDIS_ERROR_CODE_DRIVER_FAILURE				EVENT_NDIS_DRIVER_FAILURE
#define NDIS_ERROR_CODE_BAD_VERSION					EVENT_NDIS_BAD_VERSION
#define NDIS_ERROR_CODE_TIMEOUT						EVENT_NDIS_TIMEOUT
#define NDIS_ERROR_CODE_NETWORK_ADDRESS				EVENT_NDIS_NETWORK_ADDRESS
#define NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION	EVENT_NDIS_UNSUPPORTED_CONFIGURATION
#define NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER	EVENT_NDIS_INVALID_VALUE_FROM_ADAPTER
#define NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER	EVENT_NDIS_MISSING_CONFIGURATION_PARAMETER
#define NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS			EVENT_NDIS_BAD_IO_BASE_ADDRESS
#define NDIS_ERROR_CODE_RECEIVE_SPACE_SMALL			EVENT_NDIS_RECEIVE_SPACE_SMALL
#define NDIS_ERROR_CODE_ADAPTER_DISABLED			EVENT_NDIS_ADAPTER_DISABLED


/* Memory allocation flags. Used by Ndis(Allocate|Free)Memory */
#define NDIS_MEMORY_CONTIGUOUS  0x00000001
#define NDIS_MEMORY_NONCACHED   0x00000002

/* NIC attribute flags. Used by NdisMSetAttributes(Ex) */
#define	NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT    0x00000001
#define NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT   0x00000002
#define NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS 0x00000004
#define NDIS_ATTRIBUTE_BUS_MASTER               0x00000008
#define NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER      0x00000010



#define	MAXIMUM_PROCESSORS  32



/* Lock */

typedef union _NDIS_RW_LOCK_REFCOUNT
{
    UINT    RefCount;
    UCHAR   cacheLine[16];
} NDIS_RW_LOCK_REFCOUNT;

typedef struct _NDIS_RW_LOCK
{
    union
    {
        struct
        {
            KSPIN_LOCK          SpinLock;
            PVOID               Context;
        } s;
        UCHAR                   Reserved[16];
    } u;

    NDIS_RW_LOCK_REFCOUNT       RefCount[MAXIMUM_PROCESSORS];
} NDIS_RW_LOCK, *PNDIS_RW_LOCK;

typedef struct _LOCK_STATE
{
    USHORT  LockState;
    KIRQL   OldIrql;
} LOCK_STATE, *PLOCK_STATE;



/* Timer */

typedef VOID (*PNDIS_TIMER_FUNCTION)(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   FunctionContext,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3);

typedef struct _NDIS_TIMER
{
    KTIMER  Timer;
    KDPC    Dpc;
} NDIS_TIMER, *PNDIS_TIMER;



/* Hardware */

typedef CM_MCA_POS_DATA NDIS_MCA_POS_DATA, *PNDIS_MCA_POS_DATA;
typedef CM_EISA_SLOT_INFORMATION NDIS_EISA_SLOT_INFORMATION, *PNDIS_EISA_SLOT_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION NDIS_EISA_FUNCTION_INFORMATION, *PNDIS_EISA_FUNCTION_INFORMATION;
typedef CM_PARTIAL_RESOURCE_LIST NDIS_RESOURCE_LIST, *PNDIS_RESOURCE_LIST;

/* Hardware status codes (OID_GEN_HARDWARE_STATUS) */
typedef enum _NDIS_HARDWARE_STATUS
{
    NdisHardwareStatusReady,
    NdisHardwareStatusInitializing,
    NdisHardwareStatusReset,
    NdisHardwareStatusClosing,
    NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;

/* OID_GEN_GET_TIME_CAPS */
typedef struct _GEN_GET_TIME_CAPS
{
    ULONG                       Flags;
    ULONG                       ClockPrecision;
} GEN_GET_TIME_CAPS, *PGEN_GET_TIME_CAPS;

/* Flag bits */
#define	READABLE_LOCAL_CLOCK                    0x00000001
#define	CLOCK_NETWORK_DERIVED                   0x00000002
#define	CLOCK_PRECISION                         0x00000004
#define	RECEIVE_TIME_INDICATION_CAPABLE         0x00000008
#define	TIMED_SEND_CAPABLE                      0x00000010
#define	TIME_STAMP_CAPABLE                      0x00000020

/* OID_GEN_GET_NETCARD_TIME */
typedef struct _GEN_GET_NETCARD_TIME
{
    ULONGLONG   ReadTime;
} GEN_GET_NETCARD_TIME, *PGEN_GET_NETCARD_TIME;

/* NDIS driver medium (OID_GEN_MEDIA_SUPPORTED / OID_GEN_MEDIA_IN_USE) */
typedef enum _NDIS_MEDIUM
{
    NdisMedium802_3,
    NdisMedium802_5,
    NdisMediumFddi,
    NdisMediumWan,
    NdisMediumLocalTalk,
    NdisMediumDix,              /* Defined for convenience, not a real medium */
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
typedef enum _NDIS_MEDIA_STATE
{
    NdisMediaStateConnected,
    NdisMediaStateDisconnected
} NDIS_MEDIA_STATE, *PNDIS_MEDIA_STATE;

/* OID_GEN_SUPPORTED_GUIDS */
typedef struct _NDIS_GUID
{
    GUID            Guid;
    union
    {
        NDIS_OID    Oid;
        NDIS_STATUS Status;
    } u;
    ULONG           Size;
    ULONG           Flags;
} NDIS_GUID, *PNDIS_GUID;

#define	NDIS_GUID_TO_OID            0x00000001
#define	NDIS_GUID_TO_STATUS         0x00000002
#define	NDIS_GUID_ANSI_STRING       0x00000004
#define	NDIS_GUID_UNICODE_STRING    0x00000008
#define	NDIS_GUID_ARRAY	            0x00000010



typedef struct _NDIS_PACKET_POOL
{
    NDIS_SPIN_LOCK      SpinLock;
    struct _NDIS_PACKET *FreeList;
    UINT                PacketLength;
    UCHAR               Buffer[1];
} NDIS_PACKET_POOL, * PNDIS_PACKET_POOL;

typedef struct _NDIS_PACKET_PRIVATE
{
    UINT                PhysicalCount;
    UINT                TotalLength;
    PNDIS_BUFFER        Head;
    PNDIS_BUFFER        Tail;
    PNDIS_PACKET_POOL   Pool;
    UINT                Count;
    ULONG               Flags;                  /* See fPACKET_xxx bits below */
    BOOLEAN	            ValidCounts;
    UCHAR               NdisPacketFlags;
    USHORT              NdisPacketOobOffset;
} NDIS_PACKET_PRIVATE, * PNDIS_PACKET_PRIVATE;

#define fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO    0x40
#define fPACKET_ALLOCATED_BY_NDIS               0x80

typedef struct _NDIS_PACKET {
    NDIS_PACKET_PRIVATE  Private;
    union {
        struct {
             UCHAR       MiniportReserved[2*sizeof(PVOID)];
             UCHAR       WrapperReserved[2*sizeof(PVOID)];
        } s1;
        struct {
             UCHAR       MiniportReservedEx[3*sizeof(PVOID)];
             UCHAR       WrapperReservedEx[sizeof(PVOID)];
        } s2;
        struct {
             UCHAR       MacReserved[4*sizeof(PVOID)];
        } s3;
    } u;
    ULONG_PTR            Reserved[2];
    UCHAR                ProtocolReserved[1];
} NDIS_PACKET, *PNDIS_PACKET, **PPNDIS_PACKET;

typedef struct _NDIS_PACKET_OOB_DATA {
    union {
        ULONGLONG  TimeToSend;
        ULONGLONG  TimeSent;
    } u;
    ULONGLONG      TimeReceived;
    UINT           HeaderSize;
    UINT           SizeMediaSpecificInfo;
    PVOID          MediaSpecificInformation;
    NDIS_STATUS    Status;
} NDIS_PACKET_OOB_DATA, *PNDIS_PACKET_OOB_DATA;

typedef struct _NDIS_PM_PACKET_PATTERN
{
    ULONG  Priority;
    ULONG  Reserved;
    ULONG  MaskSize;
    ULONG  PatternOffset;
    ULONG  PatternSize;
    ULONG  PatternFlags;
} NDIS_PM_PACKET_PATTERN,  *PNDIS_PM_PACKET_PATTERN;


/* Request types used by NdisRequest */
typedef enum _NDIS_REQUEST_TYPE
{
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
    UCHAR             MacReserved[16];
    NDIS_REQUEST_TYPE RequestType;
    union _DATA {
        struct QUERY_INFORMATION {
            NDIS_OID  Oid;
            PVOID     InformationBuffer;
            UINT      InformationBufferLength;
            UINT      BytesWritten;
            UINT      BytesNeeded;
        } QUERY_INFORMATION;
        struct SET_INFORMATION {
            NDIS_OID  Oid;
            PVOID     InformationBuffer;
            UINT      InformationBufferLength;
            UINT      BytesRead;
            UINT      BytesNeeded;
        } SET_INFORMATION;
   } DATA;
} NDIS_REQUEST, *PNDIS_REQUEST;



/* Wide Area Networks definitions */

typedef struct _NDIS_WAN_PACKET
{
    LIST_ENTRY  WanPacketQueue;
    PUCHAR      CurrentBuffer;
    ULONG       CurrentLength;
    PUCHAR      StartBuffer;
    PUCHAR      EndBuffer;
    PVOID       ProtocolReserved1;
    PVOID       ProtocolReserved2;
    PVOID       ProtocolReserved3;
    PVOID       ProtocolReserved4;
    PVOID       MacReserved1;
    PVOID       MacReserved2;
    PVOID       MacReserved3;
    PVOID       MacReserved4;
} NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;



/* DMA channel information */

typedef struct _NDIS_DMA_DESCRIPTION
{
    BOOLEAN     DemandMode;
    BOOLEAN     AutoInitialize;
    BOOLEAN     DmaChannelSpecified;
    DMA_WIDTH   DmaWidth;
    DMA_SPEED   DmaSpeed;
    ULONG       DmaPort;
    ULONG       DmaChannel;
} NDIS_DMA_DESCRIPTION, *PNDIS_DMA_DESCRIPTION;

typedef struct _NDIS_DMA_BLOCK
{
    PVOID           MapRegisterBase;
    KEVENT          AllocationEvent;
    PADAPTER_OBJECT SystemAdapterObject;
    BOOLEAN         InProgress;
} NDIS_DMA_BLOCK, *PNDIS_DMA_BLOCK;


/* Possible hardware architecture */
typedef enum _NDIS_INTERFACE_TYPE
{
	NdisInterfaceInternal       = Internal,
	NdisInterfaceIsa            = Isa,
	NdisInterfaceEisa           = Eisa,
	NdisInterfaceMca            = MicroChannel,
	NdisInterfaceTurboChannel   = TurboChannel,
	NdisInterfacePci            = PCIBus,
	NdisInterfacePcMcia         = PCMCIABus
} NDIS_INTERFACE_TYPE, *PNDIS_INTERFACE_TYPE;

#define NdisInterruptLevelSensitive LevelSensitive
#define NdisInterruptLatched        Latched
typedef KINTERRUPT_MODE NDIS_INTERRUPT_MODE, *PNDIS_INTERRUPT_MODE;


typedef enum _NDIS_PARAMETER_TYPE
{
    NdisParameterInteger,
    NdisParameterHexInteger,
    NdisParameterString,
    NdisParameterMultiString
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef struct _NDIS_CONFIGURATION_PARAMETER
{
    NDIS_PARAMETER_TYPE ParameterType;
    union
    {
        ULONG IntegerData;
        NDIS_STRING StringData;
    } ParameterData;
} NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;


typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;

typedef struct _NDIS_PHYSICAL_ADDRESS_UNIT
{
	NDIS_PHYSICAL_ADDRESS		PhysicalAddress;
	UINT						Length;
} NDIS_PHYSICAL_ADDRESS_UNIT, *PNDIS_PHYSICAL_ADDRESS_UNIT;


typedef VOID (*ADAPTER_SHUTDOWN_HANDLER)(
    IN  PVOID   ShutdownContext);



#ifdef NDIS_WRAPPER

typedef struct _OID_LIST    OID_LIST, *POID_LIST;

/* PnP state */

typedef enum _NDIS_PNP_DEVICE_STATE
{
    NdisPnPDeviceAdded,
    NdisPnPDeviceStarted,
    NdisPnPDeviceQueryStopped,
    NdisPnPDeviceStopped,
    NdisPnPDeviceQueryRemoved,
    NdisPnPDeviceRemoved,
    NdisPnPDeviceSurpriseRemoved
} NDIS_PNP_DEVICE_STATE;

#define	NDIS_DEVICE_NOT_STOPPABLE               0x00000001
#define	NDIS_DEVICE_NOT_REMOVEABLE              0x00000002
#define	NDIS_DEVICE_NOT_SUSPENDABLE	            0x00000004
#define NDIS_DEVICE_DISABLE_PM                  0x00000008
#define NDIS_DEVICE_DISABLE_WAKE_UP             0x00000010
#define NDIS_DEVICE_DISABLE_WAKE_ON_RECONNECT   0x00000020

#endif /* NDIS_WRAPPER */


#ifdef NDIS50

typedef struct _ATM_ADDRESS ATM_ADDRESS, *PATM_ADDRESS;


/* OID_GEN_NETWORK_LAYER_ADDRESSES */
typedef struct _NETWORK_ADDRESS
{
    USHORT  AddressLength; 
    USHORT  AddressType; 
    UCHAR   Address[1]; 
} NETWORK_ADDRESS, *PNETWORK_ADDRESS;

typedef struct _NETWORK_ADDRESS_LIST 
{
    LONG    AddressCount; 
    USHORT  AddressType; 
    NETWORK_ADDRESS Address[1]; 
} NETWORK_ADDRESS_LIST, *PNETWORK_ADDRESS_LIST;

/* Protocol types supported by NDIS */
#define	NDIS_PROTOCOL_ID_DEFAULT        0x00
#define	NDIS_PROTOCOL_ID_TCP_IP         0x02
#define	NDIS_PROTOCOL_ID_IPX            0x06
#define	NDIS_PROTOCOL_ID_NBF            0x07
#define	NDIS_PROTOCOL_ID_MAX            0x0F
#define	NDIS_PROTOCOL_ID_MASK           0x0F

/* OID_GEN_TRANSPORT_HEADER_OFFSET */
typedef struct _TRANSPORT_HEADER_OFFSET
{
    USHORT  ProtocolType; 
    USHORT  HeaderOffset; 
} TRANSPORT_HEADER_OFFSET, *PTRANSPORT_HEADER_OFFSET;


/* OID_GEN_CO_LINK_SPEED / OID_GEN_CO_MINIMUM_LINK_SPEED */
typedef struct _NDIS_CO_LINK_SPEED
{
    ULONG   Outbound;
    ULONG   Inbound;
} NDIS_CO_LINK_SPEED, *PNDIS_CO_LINK_SPEED;


typedef enum _NDIS_AF
{
    CO_ADDRESS_FAMILY_Q2931 = 1,
    CO_ADDRESS_FAMILY_SPANS,
} NDIS_AF, *PNDIS_AF;

typedef struct
{
    NDIS_AF  AddressFamily;
    ULONG    MajorVersion;
    ULONG    MinorVersion;
} CO_ADDRESS_FAMILY, *PCO_ADDRESS_FAMILY;

typedef enum
{
    BestEffortService,
    PredictiveService,
    GuaranteedService
} GUARANTEE;

typedef struct _CO_FLOW_PARAMETERS
{
    ULONG       TokenRate;              /* In Bytes/sec */
    ULONG       TokenBucketSize;        /* In Bytes */
    ULONG       PeakBandwidth;          /* In Bytes/sec */
    ULONG       Latency;                /* In microseconds */
    ULONG       DelayVariation;         /* In microseconds */
    GUARANTEE   LevelOfGuarantee;       /* Guaranteed, Predictive or Best Effort */
    ULONG       CostOfCall;             /* Reserved for future use, */
                                        /* must be set to 0 now */
    ULONG       NetworkAvailability;    /* read-only: 1 if accessible, 0 if not */
    ULONG       MaxSduSize;             /* In Bytes */
} CO_FLOW_PARAMETERS, *PCO_FLOW_PARAMETERS;

typedef struct _CO_SPECIFIC_PARAMETERS
{
    ULONG   ParamType;
    ULONG   Length;
    UCHAR   Parameters[1];
} CO_SPECIFIC_PARAMETERS, *PCO_SPECIFIC_PARAMETERS;

typedef struct _CO_CALL_MANAGER_PARAMETERS {
    CO_FLOW_PARAMETERS      Transmit;
    CO_FLOW_PARAMETERS      Receive;
    CO_SPECIFIC_PARAMETERS  CallMgrSpecific;
} CO_CALL_MANAGER_PARAMETERS, *PCO_CALL_MANAGER_PARAMETERS;

typedef struct _CO_MEDIA_PARAMETERS
{
    ULONG                       Flags;
    ULONG                       ReceivePriority;
    ULONG                       ReceiveSizeHint;
    CO_SPECIFIC_PARAMETERS      MediaSpecific;
} CO_MEDIA_PARAMETERS, *PCO_MEDIA_PARAMETERS;

/* Definitions for the flags in CO_MEDIA_PARAMETERS */
#define RECEIVE_TIME_INDICATION         0x00000001
#define USE_TIME_STAMPS                 0x00000002
#define TRANSMIT_VC	                    0x00000004
#define RECEIVE_VC                      0x00000008
#define INDICATE_ERRED_PACKETS          0x00000010
#define INDICATE_END_OF_TX              0x00000020
#define RESERVE_RESOURCES_VC            0x00000040
#define	ROUND_DOWN_FLOW	                0x00000080
#define	ROUND_UP_FLOW                   0x00000100

typedef struct _CO_CALL_PARAMETERS
{
    ULONG                           Flags;
    PCO_CALL_MANAGER_PARAMETERS     CallMgrParameters;
    PCO_MEDIA_PARAMETERS            MediaParameters;
} CO_CALL_PARAMETERS, *PCO_CALL_PARAMETERS;

typedef struct _CO_SAP {
    ULONG   SapType;
    ULONG   SapLength;
    UCHAR   Sap[1];
} CO_SAP, *PCO_SAP;

typedef struct _NDIS_IPSEC_PACKET_INFO
{
    union
    {
        struct
        {
            NDIS_HANDLE    OffloadHandle;
            NDIS_HANDLE    NextOffloadHandle;
        } Transmit;
 
        struct
        {
            ULONG    SA_DELETE_REQ:1;
            ULONG    CRYPTO_DONE:1;
            ULONG    NEXT_CRYPTO_DONE:1;
            ULONG    CryptoStatus;
        } Receive;
    } u;
} NDIS_IPSEC_PACKET_INFO, *PNDIS_IPSEC_PACKET_INFO;


/* Plug and play and power management */

/* PnP and PM event codes */
typedef enum _NET_PNP_EVENT_CODE
{
    NetEventSetPower,
    NetEventQueryPower,
    NetEventQueryRemoveDevice,
    NetEventCancelRemoveDevice,
    NetEventReconfigure,
    NetEventBindList,
    NetEventBindsComplete,
    NetEventPnPCapabilities,
    NetEventMaximum
} NET_PNP_EVENT_CODE, *PNET_PNP_EVENT_CODE;

/* Networking PnP event indication structure */
typedef struct _NET_PNP_EVENT
{
    /* Event code */
    NET_PNP_EVENT_CODE  NetEvent;
    /* Event specific data */
    PVOID               Buffer;
    /* Length of event specific data */
    ULONG               BufferLength;

    /* Reserved areas */
	ULONG_PTR           NdisReserved[4];
	ULONG_PTR           TransportReserved[4];
	ULONG_PTR           TdiReserved[4];
	ULONG_PTR           TdiClientReserved[4];
} NET_PNP_EVENT, *PNET_PNP_EVENT;

/* Device power state structure */
typedef enum _NET_DEVICE_POWER_STATE
{
    NetDeviceStateUnspecified = 0,
    NetDeviceStateD0,
    NetDeviceStateD1,
    NetDeviceStateD2,
    NetDeviceStateD3,
    NetDeviceStateMaximum
} NET_DEVICE_POWER_STATE, *PNET_DEVICE_POWER_STATE;



/* Call Manager */

typedef NDIS_STATUS (*CO_CREATE_VC_HANDLER)(
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     NdisVcHandle,
    OUT PNDIS_HANDLE    ProtocolVcContext);

typedef NDIS_STATUS (*CO_DELETE_VC_HANDLER)(
    IN  NDIS_HANDLE ProtocolVcContext);

typedef NDIS_STATUS (*CO_REQUEST_HANDLER)(
    IN  NDIS_HANDLE         ProtocolAfContext,
    IN  NDIS_HANDLE         ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE	        ProtocolPartyContext    OPTIONAL,
    IN  OUT PNDIS_REQUEST   NdisRequest);

typedef VOID (*CO_REQUEST_COMPLETE_HANDLER)(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext       OPTIONAL,
    IN  NDIS_HANDLE     ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE     ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST   NdisRequest);


typedef NDIS_STATUS (*CM_OPEN_AF_HANDLER)(
	IN	NDIS_HANDLE				CallMgrBindingContext,
	IN	PCO_ADDRESS_FAMILY		AddressFamily,
	IN	NDIS_HANDLE				NdisAfHandle,
	OUT	PNDIS_HANDLE			CallMgrAfContext
	);

typedef
NDIS_STATUS
(*CM_CLOSE_AF_HANDLER)(
	IN	NDIS_HANDLE				CallMgrAfContext
	);

typedef
NDIS_STATUS
(*CM_REG_SAP_HANDLER)(
	IN	NDIS_HANDLE				CallMgrAfContext,
	IN	PCO_SAP					Sap,
	IN	NDIS_HANDLE				NdisSapHandle,
	OUT	PNDIS_HANDLE			CallMgrSapContext
	);

typedef
NDIS_STATUS
(*CM_DEREG_SAP_HANDLER)(
	IN	NDIS_HANDLE				CallMgrSapContext
	);

typedef
NDIS_STATUS
(*CM_MAKE_CALL_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters,
	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
	OUT	PNDIS_HANDLE			CallMgrPartyContext OPTIONAL
	);

typedef
NDIS_STATUS
(*CM_CLOSE_CALL_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	NDIS_HANDLE				CallMgrPartyContext	OPTIONAL,
	IN	PVOID					CloseData			OPTIONAL,
	IN	UINT					Size				OPTIONAL
	);

typedef
VOID
(*CM_INCOMING_CALL_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
NDIS_STATUS
(*CM_ADD_PARTY_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters,
	IN	NDIS_HANDLE				NdisPartyHandle,
	OUT	PNDIS_HANDLE			CallMgrPartyContext
	);

typedef
NDIS_STATUS
(*CM_DROP_PARTY_HANDLER)(
	IN	NDIS_HANDLE				CallMgrPartyContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

typedef
VOID
(*CM_ACTIVATE_VC_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CM_DEACTIVATE_VC_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				CallMgrVcContext
	);

typedef
NDIS_STATUS
(*CM_MODIFY_CALL_QOS_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef struct _NDIS_CALL_MANAGER_CHARACTERISTICS
{
    UCHAR   MajorVersion;
    UCHAR   MinorVersion;

    USHORT  Filler;
    UINT    Reserved;

    CO_CREATE_VC_HANDLER                CmCreateVcHandler;
    CO_DELETE_VC_HANDLER                CmDeleteVcHandler;
    CM_OPEN_AF_HANDLER                  CmOpenAfHandler;
    CM_CLOSE_AF_HANDLER	                CmCloseAfHandler;
    CM_REG_SAP_HANDLER                  CmRegisterSapHandler;
    CM_DEREG_SAP_HANDLER                CmDeregisterSapHandler;
    CM_MAKE_CALL_HANDLER                CmMakeCallHandler;
    CM_CLOSE_CALL_HANDLER               CmCloseCallHandler;
    CM_INCOMING_CALL_COMPLETE_HANDLER   CmIncomingCallCompleteHandler;
    CM_ADD_PARTY_HANDLER                CmAddPartyHandler;
    CM_DROP_PARTY_HANDLER               CmDropPartyHandler;
    CM_ACTIVATE_VC_COMPLETE_HANDLER     CmActivateVcCompleteHandler;
    CM_DEACTIVATE_VC_COMPLETE_HANDLER   CmDeactivateVcCompleteHandler;
    CM_MODIFY_CALL_QOS_HANDLER          CmModifyCallQoSHandler;
    CO_REQUEST_HANDLER                  CmRequestHandler;
    CO_REQUEST_COMPLETE_HANDLER         CmRequestCompleteHandler;
} NDIS_CALL_MANAGER_CHARACTERISTICS, *PNDIS_CALL_MANAGER_CHARACTERISTICS;



/* Call Manager clients */

typedef VOID (*CL_OPEN_AF_COMPLETE_HANDLER)(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE NdisAfHandle);

typedef VOID (*CL_CLOSE_AF_COMPLETE_HANDLER)(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext);

typedef VOID (*CL_REG_SAP_COMPLETE_HANDLER)(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext,
    IN  PCO_SAP     Sap,
    IN  NDIS_HANDLE NdisSapHandle);

typedef VOID (*CL_DEREG_SAP_COMPLETE_HANDLER)(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext);

typedef VOID (*CL_MAKE_CALL_COMPLETE_HANDLER)(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         ProtocolVcContext,
    IN  NDIS_HANDLE         NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS CallParameters);

typedef VOID (*CL_MODIFY_CALL_QOS_COMPLETE_HANDLER)(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS CallParameters);

typedef VOID (*CL_CLOSE_CALL_COMPLETE_HANDLER)(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolVcContext,
    IN  NDIS_HANDLE ProtocolPartyContext    OPTIONAL);

typedef VOID (*CL_ADD_PARTY_COMPLETE_HANDLER)(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         ProtocolPartyContext,
    IN  NDIS_HANDLE         NdisPartyHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

typedef VOID (*CL_DROP_PARTY_COMPLETE_HANDLER)(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolPartyContext);

typedef NDIS_STATUS (*CL_INCOMING_CALL_HANDLER)(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  OUT PCO_CALL_PARAMETERS CallParameters);

typedef VOID (*CL_INCOMING_CALL_QOS_CHANGE_HANDLER)(
    IN  NDIS_HANDLE         ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS CallParameters);

typedef VOID (*CL_INCOMING_CLOSE_CALL_HANDLER)(
    IN  NDIS_STATUS CloseStatus,
    IN  NDIS_HANDLE ProtocolVcContext,
    IN  PVOID       CloseData   OPTIONAL,
    IN  UINT        Size        OPTIONAL);

typedef VOID (*CL_INCOMING_DROP_PARTY_HANDLER)(
    IN  NDIS_STATUS DropStatus,
    IN  NDIS_HANDLE ProtocolPartyContext,
    IN  PVOID       CloseData   OPTIONAL,
    IN  UINT        Size        OPTIONAL);

typedef VOID (*CL_CALL_CONNECTED_HANDLER)(
    IN  NDIS_HANDLE ProtocolVcContext);


typedef struct _NDIS_CLIENT_CHARACTERISTICS
{
    UCHAR   MajorVersion;
    UCHAR   MinorVersion;

    USHORT  Filler;
    UINT    Reserved;

    CO_CREATE_VC_HANDLER                ClCreateVcHandler;
    CO_DELETE_VC_HANDLER                ClDeleteVcHandler;
    CO_REQUEST_HANDLER                  ClRequestHandler;
    CO_REQUEST_COMPLETE_HANDLER         ClRequestCompleteHandler;
    CL_OPEN_AF_COMPLETE_HANDLER         ClOpenAfCompleteHandler;
    CL_CLOSE_AF_COMPLETE_HANDLER        ClCloseAfCompleteHandler;
    CL_REG_SAP_COMPLETE_HANDLER         ClRegisterSapCompleteHandler;
    CL_DEREG_SAP_COMPLETE_HANDLER       ClDeregisterSapCompleteHandler;
    CL_MAKE_CALL_COMPLETE_HANDLER       ClMakeCallCompleteHandler;
    CL_MODIFY_CALL_QOS_COMPLETE_HANDLER	ClModifyCallQoSCompleteHandler;
    CL_CLOSE_CALL_COMPLETE_HANDLER      ClCloseCallCompleteHandler;
    CL_ADD_PARTY_COMPLETE_HANDLER       ClAddPartyCompleteHandler;
    CL_DROP_PARTY_COMPLETE_HANDLER      ClDropPartyCompleteHandler;
    CL_INCOMING_CALL_HANDLER            ClIncomingCallHandler;
    CL_INCOMING_CALL_QOS_CHANGE_HANDLER ClIncomingCallQoSChangeHandler;
    CL_INCOMING_CLOSE_CALL_HANDLER      ClIncomingCloseCallHandler;
    CL_INCOMING_DROP_PARTY_HANDLER      ClIncomingDropPartyHandler;
    CL_CALL_CONNECTED_HANDLER           ClCallConnectedHandler;
} NDIS_CLIENT_CHARACTERISTICS, *PNDIS_CLIENT_CHARACTERISTICS;

#endif /* NDIS50 */



/* NDIS protocol structures */

/* Prototypes for NDIS 3.0 protocol characteristics */

typedef VOID (*OPEN_ADAPTER_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_STATUS     Status,
    IN  NDIS_STATUS     OpenErrorStatus);

typedef VOID (*CLOSE_ADAPTER_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_STATUS     Status);

typedef VOID (*RESET_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_STATUS     Status);

typedef VOID (*REQUEST_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status);

typedef VOID (*STATUS_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_STATUS     GeneralStatus,
    IN  PVOID           StatusBuffer,
    IN  UINT            StatusBufferSize);

typedef VOID (*STATUS_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext);

typedef VOID (*SEND_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status);

typedef VOID (*WAN_SEND_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_WAN_PACKET    Packet,
    IN  NDIS_STATUS         Status);

typedef VOID (*TRANSFER_DATA_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred);

typedef VOID (*WAN_TRANSFER_DATA_COMPLETE_HANDLER)(
    VOID);

typedef NDIS_STATUS (*RECEIVE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  PVOID           HeaderBuffer,
    IN  UINT            HeaderBufferSize,
    IN  PVOID           LookAheadBuffer,
    IN  UINT            LookaheadBufferSize,
    IN  UINT            PacketSize);

typedef NDIS_STATUS (*WAN_RECEIVE_HANDLER)(
    IN  NDIS_HANDLE     NdisLinkHandle,
    IN  PUCHAR          Packet,
    IN  ULONG           PacketSize);

typedef VOID (*RECEIVE_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext);


/* Protocol characteristics for NDIS 3.0 protocols */
#ifdef _MSC_VER
typedef struct _NDIS30_PROTOCOL_CHARACTERISTICS
{
    UCHAR                           MajorNdisVersion;
    UCHAR                           MinorNdisVersion;
    union
    {
        UINT                        Reserved;
        UINT                        Flags;
    } u1;
    OPEN_ADAPTER_COMPLETE_HANDLER   OpenAdapterCompleteHandler;
    CLOSE_ADAPTER_COMPLETE_HANDLER  CloseAdapterCompleteHandler;
    union
    {
        SEND_COMPLETE_HANDLER       SendCompleteHandler;
        WAN_SEND_COMPLETE_HANDLER   WanSendCompleteHandler;
    } u2;
    union
    {
        TRANSFER_DATA_COMPLETE_HANDLER      TransferDataCompleteHandler;
        WAN_TRANSFER_DATA_COMPLETE_HANDLER  WanTransferDataCompleteHandler;
    } u3;

    RESET_COMPLETE_HANDLER          ResetCompleteHandler;
    REQUEST_COMPLETE_HANDLER        RequestCompleteHandler;
    union
    {
        RECEIVE_HANDLER	            ReceiveHandler;
        WAN_RECEIVE_HANDLER         WanReceiveHandler;
    } u4;
    RECEIVE_COMPLETE_HANDLER        ReceiveCompleteHandler;
    STATUS_HANDLER                  StatusHandler;
    STATUS_COMPLETE_HANDLER	        StatusCompleteHandler;
    NDIS_STRING	                    Name;
} NDIS30_PROTOCOL_CHARACTERISTICS;
typedef NDIS30_PROTOCOL_CHARACTERISTICS NDIS30_PROTOCOL_CHARACTERISTICS_S;
#else
#define NDIS30_PROTOCOL_CHARACTERISTICS \
    UCHAR                           MajorNdisVersion; \
    UCHAR                           MinorNdisVersion; \
    union \
    { \
        UINT                        Reserved; \
        UINT                        Flags; \
    } u1; \
    OPEN_ADAPTER_COMPLETE_HANDLER   OpenAdapterCompleteHandler; \
    CLOSE_ADAPTER_COMPLETE_HANDLER  CloseAdapterCompleteHandler; \
    union \
    { \
        SEND_COMPLETE_HANDLER       SendCompleteHandler; \
        WAN_SEND_COMPLETE_HANDLER   WanSendCompleteHandler; \
    } u2; \
    union \
    { \
        TRANSFER_DATA_COMPLETE_HANDLER      TransferDataCompleteHandler; \
        WAN_TRANSFER_DATA_COMPLETE_HANDLER  WanTransferDataCompleteHandler; \
    } u3; \
    RESET_COMPLETE_HANDLER          ResetCompleteHandler; \
    REQUEST_COMPLETE_HANDLER        RequestCompleteHandler; \
    union \
    { \
        RECEIVE_HANDLER	            ReceiveHandler; \
        WAN_RECEIVE_HANDLER         WanReceiveHandler; \
    } u4; \
    RECEIVE_COMPLETE_HANDLER        ReceiveCompleteHandler; \
    STATUS_HANDLER                  StatusHandler; \
    STATUS_COMPLETE_HANDLER	        StatusCompleteHandler; \
    NDIS_STRING	                    Name; 
typedef struct _NDIS30_PROTOCOL_CHARACTERISTICS_S
{
   NDIS30_PROTOCOL_CHARACTERISTICS;
} NDIS30_PROTOCOL_CHARACTERISTICS_S, *PNDIS30_PROTOCOL_CHARACTERISTICS_S;
#endif

/* Prototypes for NDIS 4.0 protocol characteristics */

typedef INT (*RECEIVE_PACKET_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_PACKET    Packet);

typedef VOID (*BIND_HANDLER)(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     BindContext,
    IN  PNDIS_STRING    DeviceName,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2);

typedef VOID (*UNBIND_HANDLER)(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     UnbindContext);

typedef VOID (*TRANSLATE_HANDLER)(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    OUT PNET_PNP_ID     IdList,
    IN  ULONG           IdListLength,
    OUT PULONG          BytesReturned);

typedef VOID (*UNLOAD_PROTOCOL_HANDLER)(
    VOID);


/* Protocol characteristics for NDIS 4.0 protocols */
#ifdef _MSC_VER
typedef struct _NDIS40_PROTOCOL_CHARACTERISTICS
{
    NDIS30_PROTOCOL_CHARACTERISTICS;

    RECEIVE_PACKET_HANDLER  ReceivePacketHandler;
    BIND_HANDLER            BindAdapterHandler;
    UNBIND_HANDLER          UnbindAdapterHandler;
    TRANSLATE_HANDLER       TranslateHandler;
    UNLOAD_PROTOCOL_HANDLER UnloadHandler;
} NDIS40_PROTOCOL_CHARACTERISTICS;
typedef NDIS40_PROTOCOL_CHARACTERISTICS NDIS40_PROTOCOL_CHARACTERISTICS_S;
#else
#define NDIS40_PROTOCOL_CHARACTERISTICS \
    NDIS30_PROTOCOL_CHARACTERISTICS; \
    RECEIVE_PACKET_HANDLER  ReceivePacketHandler; \
    BIND_HANDLER            BindAdapterHandler; \
    UNBIND_HANDLER          UnbindAdapterHandler; \
    TRANSLATE_HANDLER       TranslateHandler; \
    UNLOAD_PROTOCOL_HANDLER UnloadHandler; 
typedef struct _NDIS40_PROTOCOL_CHARACTERISTICS_S
{
   NDIS40_PROTOCOL_CHARACTERISTICS;
} NDIS40_PROTOCOL_CHARACTERISTICS_S, *PNDIS40_PROTOCOL_CHARACTERISTICS_S;
#endif


/* Prototypes for NDIS 5.0 protocol characteristics */

#ifdef NDIS50

typedef VOID (*CO_SEND_COMPLETE_HANDLER)(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    Packet);

typedef VOID (*CO_STATUS_HANDLER)(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_HANDLE ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize);

typedef UINT (*CO_RECEIVE_PACKET_HANDLER)(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    Packet);

typedef VOID (*CO_AF_REGISTER_NOTIFY_HANDLER)(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY  AddressFamily);

#ifdef _MSC_VER
typedef struct _NDIS50_PROTOCOL_CHARACTERISTICS
{
    NDIS40_PROTOCOL_CHARACTERISTICS;

    PVOID                           ReservedHandlers[4];

    CO_SEND_COMPLETE_HANDLER        CoSendCompleteHandler;
    CO_STATUS_HANDLER               CoStatusHandler;
    CO_RECEIVE_PACKET_HANDLER       CoReceivePacketHandler;
    CO_AF_REGISTER_NOTIFY_HANDLER   CoAfRegisterNotifyHandler;
} NDIS50_PROTOCOL_CHARACTERISTICS;
typedef NDIS50_PROTOCOL_CHARACTERISTICS NDIS50_PROTOCOL_CHARACTERISTICS_S;
#else
#define NDIS50_PROTOCOL_CHARACTERISTICS \
    NDIS40_PROTOCOL_CHARACTERISTICS; \
    PVOID                           ReservedHandlers[4]; \
    CO_SEND_COMPLETE_HANDLER        CoSendCompleteHandler; \
    CO_STATUS_HANDLER               CoStatusHandler; \
    CO_RECEIVE_PACKET_HANDLER       CoReceivePacketHandler; \
    CO_AF_REGISTER_NOTIFY_HANDLER   CoAfRegisterNotifyHandler;
typedef struct _NDIS50_PROTOCOL_CHARACTERISTICS_S
{
   NDIS50_PROTOCOL_CHARACTERISTICS;
} NDIS50_PROTOCOL_CHARACTERISTICS_S, *PNDIS50_PROTOCOL_CHARACTERISTICS_S;
#endif
#endif /* NDIS50 */


#ifndef NDIS50
#ifndef NDIS40
typedef struct _NDIS_PROTOCOL_CHARACTERISTICS 
{
   NDIS30_PROTOCOL_CHARACTERISTICS;
} NDIS_PROTOCOL_CHARACTERISTICS;
#else /* NDIS40 */
typedef struct _NDIS_PROTOCOL_CHARACTERISTICS 
{
   NDIS40_PROTOCOL_CHARACTERISTICS;
} NDIS_PROTOCOL_CHARACTERISTICS;
#endif /* NDIS40 */
#else /* NDIS50 */
typedef struct _NDIS_PROTOCOL_CHARACTERISTICS 
{
   NDIS50_PROTOCOL_CHARACTERISTICS;
} NDIS_PROTOCOL_CHARACTERISTICS;
#endif /* NDIS50 */

typedef NDIS_PROTOCOL_CHARACTERISTICS *PNDIS_PROTOCOL_CHARACTERISTICS;



/* Buffer management routines */

VOID
EXPIMP
NdisAllocateBuffer(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_BUFFER    *Buffer,
    IN  NDIS_HANDLE     PoolHandle,
    IN  PVOID           VirtualAddress,
    IN  UINT            Length);

VOID
EXPIMP
NdisAllocateBufferPool(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    PoolHandle,
    IN  UINT            NumberOfDescriptors);

VOID
EXPIMP
NdisAllocatePacket(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    *Packet,
    IN  NDIS_HANDLE     PoolHandle);

VOID
EXPIMP
NdisAllocatePacketPool(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    PoolHandle,
    IN  UINT            NumberOfDescriptors,
    IN  UINT            ProtocolReservedLength);

VOID
EXPIMP
NdisCopyBuffer(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_BUFFER    *Buffer,
    IN  NDIS_HANDLE     PoolHandle,
    IN  PVOID           MemoryDescriptor,
    IN  UINT            Offset,
    IN  UINT            Length);

VOID
EXPIMP
NdisCopyFromPacketToPacket(
    IN  PNDIS_PACKET    Destination,
    IN  UINT            DestinationOffset,
    IN  UINT            BytesToCopy,
    IN  PNDIS_PACKET    Source,
    IN  UINT            SourceOffset,
    OUT PUINT           BytesCopied);

VOID
EXPIMP
NdisDprAllocatePacket(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    *Packet,
    IN  NDIS_HANDLE     PoolHandle);

VOID
EXPIMP
NdisDprAllocatePacketNonInterlocked(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    *Packet,
    IN NDIS_HANDLE      PoolHandle);

VOID
EXPIMP
NdisDprFreePacket(
    IN  PNDIS_PACKET    Packet);

VOID
EXPIMP
NdisDprFreePacketNonInterlocked(
    IN  PNDIS_PACKET    Packet);

VOID
EXPIMP
NdisFreeBufferPool(
    IN  NDIS_HANDLE PoolHandle);

VOID
EXPIMP
NdisFreePacket(
    IN   PNDIS_PACKET   Packet);

VOID
EXPIMP
NdisFreePacketPool(
    IN  NDIS_HANDLE PoolHandle);

VOID
EXPIMP
NdisReturnPackets(
    IN  PNDIS_PACKET    *PacketsToReturn,
    IN  UINT            NumberOfPackets);

VOID
EXPIMP
NdisUnchainBufferAtBack(
    IN OUT  PNDIS_PACKET    Packet,
    OUT     PNDIS_BUFFER    *Buffer);

VOID
EXPIMP
NdisUnchainBufferAtFront(
    IN OUT  PNDIS_PACKET    Packet,
    OUT     PNDIS_BUFFER    *Buffer);

#if BINARY_COMPATIBLE

VOID
EXPIMP
NdisAdjustBufferLength(
    IN PNDIS_BUFFER Buffer,
    IN UINT         Length);

ULONG
EXPIMP
NDIS_BUFFER_TO_SPAN_PAGES(
    IN PNDIS_BUFFER  Buffer);

VOID
EXPIMP
NdisFreeBuffer(
    IN  PNDIS_BUFFER    Buffer);

VOID
EXPIMP
NdisGetBufferPhysicalArraySize(
    IN  PNDIS_BUFFER    Buffer,
    OUT PUINT           ArraySize);

VOID
EXPIMP
NdisGetFirstBufferFromPacket(
    IN  PNDIS_PACKET    _Packet,
    OUT PNDIS_BUFFER    *_FirstBuffer,
    OUT PVOID           *_FirstBufferVA,
    OUT PUINT           _FirstBufferLength,
    OUT PUINT           _TotalBufferLength);

VOID
EXPIMP
NdisQueryBuffer(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length);

VOID
EXPIMP
NdisQueryBufferOffset(
    IN  PNDIS_BUFFER    Buffer,
    OUT PUINT           Offset,
    OUT PUINT           Length);

#else /* BINARY_COMPATIBLE */

/*
 * PVOID NdisAdjustBufferLength(
 *     IN  PNDIS_BUFFER    Buffer,
 *     IN  UINT            Length);
 */
#define NdisAdjustBufferLength(Buffer,  \
                               Length)  \
{                                       \
    (Buffer)->ByteCount = (Length);     \
}


/*
 * ULONG NDIS_BUFFER_TO_SPAN_PAGES(
 *     IN  PNDIS_BUFFER    Buffer);
 */
#define NDIS_BUFFER_TO_SPAN_PAGES(Buffer)   \
(                                           \
    MmGetMdlByteCount(Buffer) == 0 ?        \
        1 :                                 \
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(     \
            MmGetMdlVirtualAddress(Buffer), \
            MmGetMdlByteCount(Buffer))      \
)


#if 0

/*
 * VOID NdisFreeBuffer(
 *     IN  PNDIS_BUFFER    Buffer);
 */
#define NdisFreeBuffer(Buffer)  \
{                               \
    IoFreeMdl(Buffer) /* ??? */ \
}

#else

VOID
EXPIMP
NdisFreeBuffer(
    IN  PNDIS_BUFFER    Buffer);

#endif


/*
 * VOID NdisGetBufferPhysicalArraySize(
 *     IN  PNDIS_BUFFER    Buffer,
 *     OUT PUINT           ArraySize);
 */
#define NdisGetBufferPhysicalArraySize(Buffer,      \
                                       ArraySize)   \
{                                                   \
}


/*
 * VOID NdisGetFirstBufferFromPacket(
 *     IN  PNDIS_PACKET    _Packet,
 *     OUT PNDIS_BUFFER    * _FirstBuffer,
 *     OUT PVOID           * _FirstBufferVA,
 *     OUT PUINT           _FirstBufferLength,
 *     OUT PUINT           _TotalBufferLength)
 */
#define	NdisGetFirstBufferFromPacket(Packet,                \
                                     FirstBuffer,           \
                                     FirstBufferVA,         \
                                     FirstBufferLength,     \
                                     TotalBufferLength)     \
{                                                           \
    PNDIS_BUFFER _Buffer;                                   \
                                                            \
    _Buffer              = (Packet)->Private.Head;          \
    *(FirstBuffer)       = _Buffer;                         \
    *(FirstBufferVA)     = MmGetMdlVirtualAddress(_Buffer); \
    if (_Buffer != NULL) {                                  \
        *(FirstBufferLength) = MmGetMdlByteCount(_Buffer);  \
        _Buffer = _Buffer->Next;                            \
    } else                                                  \
        *(FirstBufferLength) = 0;                           \
    *(TotalBufferLength) = *(FirstBufferLength);            \
    while (_Buffer != NULL) {                               \
        *(TotalBufferLength) += MmGetMdlByteCount(_Buffer); \
        _Buffer = _Buffer->Next;                            \
    }                                                       \
}

/*
 * VOID NdisQueryBuffer(
 *     IN  PNDIS_BUFFER    Buffer,
 *     OUT PVOID           *VirtualAddress OPTIONAL,
 *     OUT PUINT           Length)
 */
#define NdisQueryBuffer(Buffer,                                       \
                        VirtualAddress,                               \
                        Length)                                       \
{                                                                     \
	if (VirtualAddress)                                               \
		*((PVOID*)VirtualAddress) = MmGetSystemAddressForMdl(Buffer); \
                                                                      \
	*((PUINT)Length) = MmGetMdlByteCount(Buffer);                     \
}


/*
 * VOID NdisQueryBufferOffset(
 *     IN  PNDIS_BUFFER    Buffer,
 *     OUT PUINT           Offset,
 *     OUT PUINT           Length);
 */
#define NdisQueryBufferOffset(Buffer,               \
                              Offset,               \
                              Length)               \
{                                                   \
    *((PUINT)Offset) = MmGetMdlByteOffset(Buffer);  \
    *((PUINT)Length) = MmGetMdlByteCount(Buffer);   \
}

#endif /* BINARY_COMPATIBLE */


/*
 * PVOID NDIS_BUFFER_LINKAGE(
 *     IN  PNDIS_BUFFER    Buffer);
 */
#define NDIS_BUFFER_LINKAGE(Buffer) \
{                                   \
    (Buffer)->Next;                 \
}


/*
 * VOID NdisChainBufferAtBack(
 *     IN OUT  PNDIS_PACKET    Packet,
 *     IN OUT  PNDIS_BUFFER    Buffer)
 */
#define NdisChainBufferAtBack(Packet,               \
                              Buffer)               \
{                                                   \
	PNDIS_BUFFER NdisBuffer = (Buffer);             \
                                                    \
    while (NdisBuffer->Next != NULL)                \
        NdisBuffer = NdisBuffer->Next;              \
                                                    \
    NdisBuffer->Next = NULL;                        \
                                                    \
    if ((Packet)->Private.Head != NULL)             \
        (Packet)->Private.Tail->Next = (Buffer);    \
    else                                            \
        (Packet)->Private.Head = (Buffer);          \
                                                    \
	(Packet)->Private.Tail        = NdisBuffer;     \
	(Packet)->Private.ValidCounts = FALSE;          \
}


/*
 * VOID NdisChainBufferAtFront(
 *     IN OUT  PNDIS_PACKET    Packet,
 *     IN OUT  PNDIS_BUFFER    Buffer)
 */
#define NdisChainBufferAtFront(Packet,          \
                               Buffer)          \
{                                               \
	PNDIS_BUFFER _NdisBuffer = (Buffer);        \
                                                \
    while (_NdisBuffer->Next != NULL)           \
        _NdisBuffer = _NdisBuffer->Next;        \
                                                \
    if ((Packet)->Private.Head == NULL)         \
        (Packet)->Private.Tail = _NdisBuffer;   \
                                                \
	_NdisBuffer->Next = (Packet)->Private.Head; \
	(Packet)->Private.Head        = (Buffer);   \
	(Packet)->Private.ValidCounts = FALSE;      \
}


/*
 * VOID NdisGetNextBuffer(
 *     IN  PNDIS_BUFFER    CurrentBuffer,
 *     OUT PNDIS_BUFFER    * NextBuffer)
 */
#define NdisGetNextBuffer(CurrentBuffer,    \
                          NextBuffer)       \
{                                           \
    *(NextBuffer) = (CurrentBuffer)->Next;  \
}


/*
 * UINT NdisGetPacketFlags(
 *     IN  PNDIS_PACKET    Packet); 
 */
#define NdisGetPacketFlags(Packet)  \
{                                   \
    (Packet)->Private.Flags;        \
}


/*
 * UINT NDIS_GET_PACKET_HEADER_SIZE(
 *     IN  PNDIS_PACKET    Packet);
 */
#define NDIS_GET_PACKET_HEADER_SIZE(Packet) \
{                                           \
}


/*
 * VOID NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  PPVOID          pMediaSpecificInfo,
 *     IN  PUINT           pSizeMediaSpecificInfo);
 */
#define NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(Packet,                 \
                                            pMediaSpecificInfo,     \
                                            pSizeMediaSpecificInfo) \
{                                                                   \
}


/*
 * VOID NDIS_STATUS NDIS_GET_PACKET_STATUS(
 *     IN  PNDIS_PACKET    Packet);
 */
#define NDIS_GET_PACKET_STATUS (Packet) \
{                                       \
}


/*
 * ULONGLONG NDIS_GET_PACKET_TIME_RECEIVED(
 *     IN  PNDIS_PACKET    Packet);
 */
#define NDIS_GET_PACKET_TIME_RECEIVED(Packet)   \
{                                               \
}


/*
 * ULONGLONG NDIS_GET_PACKET_TIME_SENT(
 *     IN  PNDIS_PACKET    Packet);
 */
#define NDIS_GET_PACKET_TIME_SENT(Packet)   \
{                                           \
}


/*
 * ULONGLONG NDIS_GET_PACKET_TIME_TO_SEND(
 *     IN  PNDIS_PACKET    Packet);
 */
#define NDIS_GET_PACKET_TIME_TO_SEND(Packet)    \
{                                               \
}


/*
 * PNDIS_PACKET_OOB_DATA NDIS_OOB_DATA_FROM_PACKET(
 *     IN  PNDIS_PACKET    _Packet);
 */
#define NDIS_OOB_DATA_FROM_PACKET(_Packet)  \
{                                           \
}

 
/*
 * VOID NdisQueryPacket(
 *     IN  PNDIS_PACKET    Packet,
 *     OUT PUINT           PhysicalBufferCount OPTIONAL,
 *     OUT PUINT           BufferCount         OPTIONAL,
 *     OUT PNDIS_BUFFER    *FirstBuffer        OPTIONAL,
 *     OUT PUINT           TotalPacketLength   OPTIONAL);
 */
#define NdisQueryPacket(Packet,                                                 \
                        PhysicalBufferCount,                                    \
                        BufferCount,                                            \
                        FirstBuffer,                                            \
                        TotalPacketLength)                                      \
{                                                                               \
    if (FirstBuffer)                                                            \
        *((PNDIS_BUFFER*)FirstBuffer) = (Packet)->Private.Head;                 \
    if ((TotalPacketLength) || (BufferCount) || (PhysicalBufferCount)) {        \
        if (!(Packet)->Private.ValidCounts) {                                   \
            UINT _Offset;                                                       \
            UINT _PacketLength;                                                 \
            PNDIS_BUFFER _NdisBuffer;                                           \
            UINT _PhysicalBufferCount = 0;                                      \
            UINT _TotalPacketLength   = 0;                                      \
            UINT _Count               = 0;                                      \
                                                                                \
            for (_NdisBuffer = (Packet)->Private.Head;                          \
                _NdisBuffer != (PNDIS_BUFFER)NULL;                              \
                _NdisBuffer = _NdisBuffer->Next) {                              \
                _PhysicalBufferCount += NDIS_BUFFER_TO_SPAN_PAGES(_NdisBuffer); \
                NdisQueryBufferOffset(_NdisBuffer, &_Offset, &_PacketLength);   \
                _TotalPacketLength += _PacketLength;                            \
                _Count++;                                                       \
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
    }                                                                           \
}


/*
 * VOID NdisRecalculatePacketCounts(
 *     IN OUT  PNDIS_PACKET    Packet);
 */
#define NdisRecalculatePacketCounts(Packet) \
{                                           \
}

VOID
EXPIMP
NdisReinitializePacket(
    IN OUT  PNDIS_PACKET    Packet);


/*
 * VOID NdisSetPacketFlags(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  UINT            Flags); 
 */
#define NdisSetPacketFlags(Packet, Flags)   \
    (Packet)->Private.Flags = (Flags);


/*
 * NDIS_SET_PACKET_HEADER_SIZE(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  UINT            HdrSize);
 */
#define NDIS_SET_PACKET_HEADER_SIZE(Packet,     \
                                    HdrSize)    \
{                                               \
}


/*
 * NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  PVOID           MediaSpecificInfo,
 *     IN  UINT            SizeMediaSpecificInfo);
 */
#define NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(Packet,                 \
                                            MediaSpecificInfo,      \
                                            SizeMediaSpecificInfo)  \
{                                                                   \
}


/*
 * NDIS_SET_PACKET_STATUS(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  NDIS_STATUS     Status);
 */
#define NDIS_SET_PACKET_STATUS(Packet,  \
                               Status)  \
{                                       \
}


/*
 * NDIS_SET_PACKET_TIME_RECEIVED(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  ULONGLONG       TimeReceived);
 */
#define NDIS_SET_PACKET_TIME_RECEIVED(Packet)       \
                                      TimeReceived) \
{                                                   \
}


/*
 * NDIS_SET_PACKET_TIME_SENT(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  ULONGLONG       TimeSent);
 */
#define NDIS_SET_PACKET_TIME_SENT(Packet,   \
                                  TimeSent) \
{                                           \
}


/*
 *
 * NDIS_SET_PACKET_TIME_TO_SEND(
 *     IN  PNDIS_PACKET    Packet,
 *     IN  ULONGLONG       TimeToSend);
 */
#define NDIS_SET_PACKET_TIME_TO_SEND(Packet,        \
                                     TimeToSend)    \
{                                                   \
}


/*
 * VOID NdisSetSendFlags(
 *   IN  PNDIS_PACKET    Packet,
 *   IN  UINT            Flags);
 */
#define NdisSetSendFlags(Packet, Flags)(    \
    NdisSetPacketFlags((Packet), (Flags)))



/* Memory management routines */

VOID
EXPIMP
NdisCreateLookaheadBufferFromSharedMemory(
    IN  PVOID   pSharedMemory,
    IN  UINT    LookaheadLength,
    OUT PVOID   *pLookaheadBuffer);

VOID
EXPIMP
NdisDestroyLookaheadBufferFromSharedMemory(
    IN  PVOID   pLookaheadBuffer);

VOID
EXPIMP
NdisMoveFromMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length);

VOID
EXPIMP
NdisMoveMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length);

VOID
EXPIMP
NdisMoveToMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length);

VOID
EXPIMP
NdisMUpdateSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);

NDIS_STATUS
EXPIMP
NdisAllocateMemory(
    OUT PVOID                   *VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress);

VOID
EXPIMP
NdisFreeMemory(
    IN  PVOID   VirtualAddress,
    IN  UINT    Length,
    IN  UINT    MemoryFlags);

VOID
EXPIMP
NdisImmediateReadSharedMemory(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SharedMemoryAddress,
    OUT PUCHAR      Buffer,
    IN  ULONG       Length);

VOID
EXPIMP
NdisImmediateWriteSharedMemory(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SharedMemoryAddress,
    IN  PUCHAR      Buffer,
    IN  ULONG       Length);

VOID
EXPIMP
NdisMAllocateSharedMemory(
    IN	NDIS_HANDLE             MiniportAdapterHandle,
    IN	ULONG                   Length,
    IN	BOOLEAN                 Cached,
    OUT	PVOID                   *VirtualAddress,
    OUT	PNDIS_PHYSICAL_ADDRESS  PhysicalAddress);

NDIS_STATUS
EXPIMP
NdisMAllocateSharedMemoryAsync(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  ULONG       Length,
    IN  BOOLEAN     Cached,
    IN  PVOID       Context);

VOID
EXPIMP
NdisMFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);

VOID
EXPIMP
NdisUpdateSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);


/*
 * ULONG NdisGetPhysicalAddressHigh(
 *     IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);
 */
#define NdisGetPhysicalAddressHigh(PhysicalAddress) \
    ((PhysicalAddress).HighPart)

/*
 * VOID NdisSetPhysicalAddressHigh(
 *     IN   NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
 *     IN   ULONG                   Value);
 */
#define NdisSetPhysicalAddressHigh(PhysicalAddress, Value)  \
    ((PhysicalAddress).HighPart) = (Value)

/*
 * ULONG NdisGetPhysicalAddressLow(
 *     IN   NDIS_PHYSICAL_ADDRESS   PhysicalAddress);
 */
#define NdisGetPhysicalAddressLow(PhysicalAddress)  \
    ((PhysicalAddress).LowPart)


/*
 * VOID NdisSetPhysicalAddressLow(
 *     IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
 *     IN  ULONG                   Value);
 */
#define NdisSetPhysicalAddressLow(PhysicalAddress, Value)   \
    ((PhysicalAddress).LowPart) = (Value)

/*
 * VOID NDIS_PHYSICAL_ADDRESS_CONST(
 *     IN  ULONG   Low,
 *     IN  LONG    High); 
 */
#define NDIS_PHYSICAL_ADDRESS_CONST(Low, High)  \
    { {(ULONG)(Low), (LONG)(High)} }


/*
 * VOID NdisMoveMemory(
 *     OUT PVOID   Destination,
 *     IN  PVOID   Source,
 *     IN  ULONG   Length);
 */
#define NdisMoveMemory(Destination, Source, Length) \
    RtlCopyMemory(Destination, Source, Length)


/*
 * VOID NdisRetrieveUlong(
 *     IN  PULONG  DestinationAddress,
 *     IN  PULONG  SourceAddress);
 */
#define NdisRetrieveUlong(DestinationAddress, SourceAddress)    \
    RtlRetrieveUlong(DestinationAddress, SourceAddress)


/*
 * VOID NdisStoreUlong(
 *     IN  PULONG  DestinationAddress,
 *     IN  ULONG   Value); 
 */
#define NdisStoreUlong(DestinationAddress, Value)   \
    RtlStoreUlong(DestinationAddress, Value)


/*
 * VOID NdisZeroMemory(
 *     IN PVOID    Destination,
 *     IN ULONG    Length)
 */
#define NdisZeroMemory(Destination, Length) \
    RtlZeroMemory(Destination, Length)



//
// System processor count
//

CCHAR
EXPIMP
NdisSystemProcessorCount(
	VOID
	);

VOID
EXPIMP
NdisImmediateReadPortUchar(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	OUT PUCHAR					Data
	);

VOID
EXPIMP
NdisImmediateReadPortUshort(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	OUT PUSHORT Data
	);

VOID
EXPIMP
NdisImmediateReadPortUlong(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	OUT PULONG Data
	);

VOID
EXPIMP
NdisImmediateWritePortUchar(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	IN	UCHAR					Data
	);

VOID
EXPIMP
NdisImmediateWritePortUshort(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	IN	USHORT					Data
	);

VOID
EXPIMP
NdisImmediateWritePortUlong(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	IN	ULONG					Data
	);

VOID
EXPIMP
NdisImmediateReadSharedMemory(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SharedMemoryAddress,
	IN	PUCHAR					Buffer,
	IN	ULONG					Length
	);

VOID
EXPIMP
NdisImmediateWriteSharedMemory(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SharedMemoryAddress,
	IN	PUCHAR					Buffer,
	IN	ULONG					Length
	);

ULONG
EXPIMP
NdisImmediateReadPciSlotInformation(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SlotNumber,
	IN	ULONG					Offset,
	IN	PVOID					Buffer,
	IN	ULONG					Length
	);

ULONG
EXPIMP
NdisImmediateWritePciSlotInformation(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SlotNumber,
	IN	ULONG					Offset,
	IN	PVOID					Buffer,
	IN	ULONG					Length
	);

/* String management routines */

#if BINARY_COMPATIBLE

NDIS_STATUS
EXPIMP
NdisAnsiStringToUnicodeString(
    IN OUT  PNDIS_STRING        DestinationString,
    IN      PNDIS_ANSI_STRING   SourceString);

BOOLEAN
EXPIMP
NdisEqualString(
    IN  PNDIS_STRING    String1,
    IN  PNDIS_STRING    String2,
    IN  BOOLEAN         CaseInsensitive);

VOID
EXPIMP
NdisInitAnsiString(
    IN OUT  PNDIS_ANSI_STRING   DestinationString,
    IN      PCSTR               SourceString);

VOID
EXPIMP
NdisInitUnicodeString(
    IN OUT  PNDIS_STRING    DestinationString,
    IN      PCWSTR          SourceString);

NDIS_STATUS
EXPIMP
NdisUnicodeStringToAnsiString(
    IN OUT  PNDIS_ANSI_STRING   DestinationString,
    IN      PNDIS_STRING        SourceString);

#else /* BINARY_COMPATIBLE */

/*
 * NDIS_STATUS NdisAnsiStringToUnicodeString(
 *     IN OUT  PNDIS_STRING        DestinationString,
 *     IN      PNDIS_ANSI_STRING   SourceString);
 */
#define	NdisAnsiStringToUnicodeString(DestinationString,    \
                                      SourceString)         \
    RtlAnsiStringToUnicodeString((DestinationString), (SourceString), FALSE)

/*
 * BOOLEAN NdisEqualString(
 *     IN  PNDIS_STRING    String1,
 *     IN  PNDIS_STRING    String2,
 *     IN  BOOLEAN         CaseInsensitive)
 */
#define NdisEqualString(String1,            \
                        String2,            \
                        CaseInsensitive)    \
    RtlEqualUnicodeString((String1), (String2), (CaseInsensitive))

/*
 * VOID NdisInitAnsiString(
 *     IN OUT  PNDIS_ANSI_STRING   DestinationString,
 *     IN      PCSTR               SourceString)
 */
#define	NdisInitAnsiString(DestinationString,   \
                           SourceString)        \
    RtlInitString((DestinationString), (SourceString))

/*
 * VOID NdisInitUnicodeString(
 *     IN OUT  PNDIS_STRING    DestinationString,
 *     IN      PCWSTR          SourceString)
 */
#define	NdisInitUnicodeString(DestinationString,    \
                              SourceString)         \
    RtlInitUnicodeString((DestinationString), (SourceString))

/*
 * NDIS_STATUS NdisUnicodeStringToAnsiString(
 *     IN OUT  PNDIS_ANSI_STRING   DestinationString,
 *     IN      PNDIS_STRING        SourceString)
 */
#define	NdisUnicodeStringToAnsiString(DestinationString,    \
                                      SourceString)         \
    RtlUnicodeStringToAnsiString((DestinationString), (SourceString), FALSE)

#endif /* BINARY_COMPATIBLE */

#define NdisFreeString(_s)  NdisFreeMemory((s).Buffer, (s).MaximumLength, 0)
#define NdisPrintString(_s) DbgPrint("%ls", (s).Buffer)



/* I/O routines */

/*
 * VOID NdisRawReadPortBufferUchar(
 *     IN  ULONG   Port,
 *     OUT PUCHAR  Buffer,
 *     IN  ULONG   Length);
 */
#define NdisRawReadPortBufferUchar(Port, Buffer, Length)    \
    READ_PORT_BUFFER_UCHAR((PUCHAR)(Port), (PUCHAR)(Buffer), (Length))

/*
 * VOID NdisRawReadPortBufferUlong(
 *     IN  ULONG   Port,
 *     OUT PULONG  Buffer,
 *     IN  ULONG   Length);
 */
#define NdisRawReadPortBufferUlong(Port, Buffer, Length)  \
    READ_PORT_BUFFER_ULONG((PULONG)(Port), (PULONG)(Buffer), (Length))

/*
 * VOID NdisRawReadPortBufferUshort(
 *     IN  ULONG   Port,
 *     OUT PUSHORT Buffer,
 *     IN  ULONG   Length);
 */
#define NdisRawReadPortBufferUshort(Port, Buffer, Length)   \
    READ_PORT_BUFFER_USHORT((PUSHORT)(Port), (PUSHORT)(Buffer), (Length))


/*
 * VOID NdisRawReadPortUchar(
 *     IN  ULONG   Port,
 *     OUT PUCHAR  Data);
 */
#define NdisRawReadPortUchar(Port, Data)    \
    *(Data) = READ_PORT_UCHAR((PUCHAR)(Port))

/*
 * VOID NdisRawReadPortUlong(
 *     IN  ULONG   Port,
 *     OUT PULONG  Data);
 */
#define NdisRawReadPortUlong(Port, Data)    \
    *(Data) = READ_PORT_ULONG((PULONG)(Port))

/*
 * VOID NdisRawReadPortUshort(
 *     IN  ULONG   Port,
 *     OUT PUSHORT Data);
 */
#define NdisRawReadPortUshort(Port, Data)   \
    *(Data) = READ_PORT_USHORT((PUSHORT)(Port))


/*
 * VOID NdisRawWritePortBufferUchar(
 *     IN  ULONG   Port,
 *     IN  PUCHAR  Buffer,
 *     IN  ULONG   Length);
 */
#define NdisRawWritePortBufferUchar(Port, Buffer, Length) \
    WRITE_PORT_BUFFER_UCHAR((PUCHAR)(Port), (PUCHAR)(Buffer), (Length))

/*
 * VOID NdisRawWritePortBufferUlong(
 *     IN  ULONG   Port,
 *     IN  PULONG  Buffer,
 *     IN  ULONG   Length);
 */
#define NdisRawWritePortBufferUlong(Port, Buffer, Length)   \
    WRITE_PORT_BUFFER_ULONG((PULONG)(Port), (PULONG)(Buffer), (Length))

/*
 * VOID NdisRawWritePortBufferUshort(
 *     IN  ULONG   Port,
 *     IN  PUSHORT Buffer,
 *     IN  ULONG   Length);
 */
#define NdisRawWritePortBufferUshort(Port, Buffer, Length)  \
    WRITE_PORT_BUFFER_USHORT((PUSHORT)(Port), (PUSHORT)(Buffer), (Length))


/*
 * VOID NdisRawWritePortUchar(
 *     IN  ULONG   Port,
 *     IN  UCHAR   Data);
 */
#define NdisRawWritePortUchar(Port, Data)   \
    WRITE_PORT_UCHAR((PUCHAR)(Port), (UCHAR)(Data))

/*
 * VOID NdisRawWritePortUlong(
 *     IN  ULONG   Port,
 *     IN  ULONG   Data);
 */
#define NdisRawWritePortUlong(Port, Data)   \
    WRITE_PORT_ULONG((PULONG)(Port), (ULONG)(Data))

/*
 * VOID NdisRawWritePortUshort(
 *     IN  ULONG   Port,
 *     IN  USHORT  Data);
 */
#define NdisRawWritePortUshort(Port, Data) \
    WRITE_PORT_USHORT((PUSHORT)(Port), (USHORT)(Data))


/*
 * VOID NdisReadRegisterUchar(
 *     IN  PUCHAR  Register,
 *     OUT PUCHAR  Data);
 */
#define NdisReadRegisterUchar(Register, Data)   \
    *((PUCHAR)(Data)) = *(Register)

/*
 * VOID NdisReadRegisterUlong(
 *     IN  PULONG  Register,
 *     OUT PULONG  Data);
 */
#define NdisReadRegisterUlong(Register, Data)   \
    *((PULONG)(Data)) = *(Register)

/*
 * VOID NdisReadRegisterUshort(
 *     IN  PUSHORT Register,
 *     OUT PUSHORT Data);
 */
#define NdisReadRegisterUshort(Register, Data)  \
    *((PUSHORT)(Data)) = *(Register)


/*
 * VOID NdisReadRegisterUchar(
 *     IN  PUCHAR  Register,
 *     IN  UCHAR   Data);
 */
#define NdisWriteRegisterUchar(Register, Data)  \
    WRITE_REGISTER_UCHAR((Register), (Data))

/*
 * VOID NdisReadRegisterUlong(
 *     IN  PULONG  Register,
 *     IN  ULONG   Data);
 */
#define NdisWriteRegisterUlong(Register, Data)  \
	WRITE_REGISTER_ULONG((Register), (Data))

/*
 * VOID NdisReadRegisterUshort(
 *     IN  PUSHORT Register,
 *     IN  USHORT  Data);
 */
#define NdisWriteRegisterUshort(Register, Data) \
    WRITE_REGISTER_USHORT((Register), (Data))


/* Linked lists */

VOID
EXPIMP
NdisInitializeListHead(
    IN  PLIST_ENTRY ListHead);

VOID
EXPIMP
NdisInterlockedAddUlong(
    IN  PULONG          Addend,
    IN  ULONG           Increment,
    IN  PNDIS_SPIN_LOCK SpinLock);

PLIST_ENTRY
EXPIMP
NdisInterlockedInsertHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock);

PLIST_ENTRY
EXPIMP
NdisInterlockedInsertTailList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock); 

PLIST_ENTRY
EXPIMP
NdisInterlockedRemoveHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PNDIS_SPIN_LOCK SpinLock); 


VOID
EXPIMP
NdisCloseConfiguration(
    IN  NDIS_HANDLE ConfigurationHandle);

VOID
EXPIMP
NdisReadConfiguration(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_CONFIGURATION_PARAMETER   *ParameterValue,
    IN  NDIS_HANDLE                     ConfigurationHandle,
    IN  PNDIS_STRING                    Keyword,
    IN  NDIS_PARAMETER_TYPE             ParameterType);

VOID
EXPIMP
NdisWriteConfiguration(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    IN  PNDIS_STRING                    Keyword,
    IN  PNDIS_CONFIGURATION_PARAMETER   *ParameterValue);

/*
VOID
EXPIMP
NdisWriteErrorLogEntry(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  NDIS_ERROR_CODE ErrorCode,
    IN  ULONG           NumberOfErrorValues,
    IN  ULONG           ...);
*/



/*
 * VOID NdisStallExecution(
 *     IN  UINT    MicrosecondsToStall)
 */
#define NdisStallExecution(MicroSecondsToStall)     \
    KeStallExecutionProcessor(MicroSecondsToStall)


#define NdisZeroMappedMemory(Destination,Length)		RtlZeroMemory(Destination,Length)
/* moved to ndis/memory.c by robd
#define NdisMoveMappedMemory(Destination,Source,Length) RtlCopyMemory(Destination,Source,Length)
 */
/* moved to ndis/control.c by robd
#define NdisReinitializePacket(Packet)										\
{																			\
	(Packet)->Private.Head = (PNDIS_BUFFER)NULL;							\
	(Packet)->Private.ValidCounts = FALSE;									\
}
 */
VOID
EXPIMP
NdisInitializeEvent(
	IN	PNDIS_EVENT				Event
);

VOID
EXPIMP
NdisSetEvent(
	IN	PNDIS_EVENT				Event
);

VOID
EXPIMP
NdisResetEvent(
	IN	PNDIS_EVENT				Event
);

BOOLEAN
EXPIMP
NdisWaitEvent(
	IN	PNDIS_EVENT				Event,
	IN	UINT					msToWait
);


/* NDIS helper macros */

/*
 * NDIS_INIT_FUNCTION(FunctionName)
 */
#define NDIS_INIT_FUNCTION(FunctionName)    \
    alloc_text(init, FunctionName)

/*
 * NDIS_PAGABLE_FUNCTION(FunctionName) 
 */
#define NDIS_PAGABLE_FUNCTION(FunctionName) \
    alloc_text(page, FunctionName)



/* NDIS 4.0 extensions */

#ifdef NDIS40

VOID
EXPIMP
NdisMFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);

VOID
EXPIMP
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisLinkContext,
    IN  PUCHAR          PacketBuffer,
    IN  UINT            PacketSize);

VOID
EXPIMP
NdisMWanIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle);

VOID
EXPIMP
NdisMWanSendComplete(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  PNDIS_WAN_PACKET    Packet,
    IN  NDIS_STATUS         Status);

NDIS_STATUS
EXPIMP
NdisPciAssignResources(
    IN  NDIS_HANDLE         NdisMacHandle,
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  NDIS_HANDLE         WrapperConfigurationContext,
    IN  ULONG               SlotNumber,
    OUT PNDIS_RESOURCE_LIST *AssignedResources);

VOID
EXPIMP
NdisReadEisaSlotInformationEx(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    OUT PUINT                           SlotNumber,
    OUT PNDIS_EISA_FUNCTION_INFORMATION *EisaData,
    OUT PUINT                           NumberOfFunctions);

VOID
EXPIMP
NdisReadMcaPosInformation(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         WrapperConfigurationContext,
    IN  PUINT               ChannelNumber,
    OUT PNDIS_MCA_POS_DATA  McaData);

#endif /* NDIS40 */


#if	USE_KLOCKS

#define	DISPATCH_LEVEL		2

#define NdisAllocateSpinLock(_SpinLock)	KeInitializeSpinLock(&(_SpinLock)->SpinLock)

#define NdisFreeSpinLock(_SpinLock)

#define NdisAcquireSpinLock(_SpinLock)	KeAcquireSpinLock(&(_SpinLock)->SpinLock, &(_SpinLock)->OldIrql)

#define NdisReleaseSpinLock(_SpinLock)	KeReleaseSpinLock(&(_SpinLock)->SpinLock,(_SpinLock)->OldIrql)

#define NdisDprAcquireSpinLock(_SpinLock)						\
{																\
	KeAcquireSpinLockAtDpcLevel(&(_SpinLock)->SpinLock);		\
	(_SpinLock)->OldIrql = DISPATCH_LEVEL;						\
}

#define NdisDprReleaseSpinLock(_SpinLock) KeReleaseSpinLockFromDpcLevel(&(_SpinLock)->SpinLock)

#else

//
// Ndis Spin Locks
//

VOID
EXPIMP
NdisAllocateSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


VOID
EXPIMP
NdisFreeSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


VOID
EXPIMP
NdisAcquireSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


VOID
EXPIMP
NdisReleaseSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


VOID
EXPIMP
NdisDprAcquireSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


VOID
EXPIMP
NdisDprReleaseSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

#endif

VOID
EXPIMP
NdisGetCurrentSystemTime(
	PLONGLONG				pSystemTime
	);


/* NDIS 5.0 extensions */

#ifdef NDIS50

VOID
EXPIMP
NdisAcquireReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock,
    IN  BOOLEAN         fWrite,
    IN  PLOCK_STATE     LockState);

NDIS_STATUS
EXPIMP
NdisAllocateMemoryWithTag(
    OUT PVOID   *VirtualAddress,
    IN  UINT    Length,
    IN  ULONG   Tag);

VOID
EXPIMP
NdisAllocatePacketPoolEx(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    PoolHandle,
    IN  UINT            NumberOfDescriptors,
    IN  UINT            NumberOfOverflowDescriptors,
    IN  UINT            ProtocolReservedLength);

ULONG
EXPIMP
NdisBufferLength(
    IN  PNDIS_BUFFER    Buffer);

PVOID
EXPIMP
NdisBufferVirtualAddress(
    IN  PNDIS_BUFFER    Buffer);

VOID
EXPIMP
NdisCompletePnPEvent(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent);

VOID
EXPIMP
NdisConvertStringToAtmAddress(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_STRING    String,
    OUT PATM_ADDRESS    AtmAddress);

VOID
EXPIMP
NdisGetCurrentProcessorCounts(
    OUT PULONG  pIdleCount,
    OUT PULONG  pKernelAndUser,
    OUT PULONG  pIndex);

VOID
EXPIMP
NdisGetDriverHandle(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    OUT PNDIS_HANDLE    NdisDriverHandle);

PNDIS_PACKET
EXPIMP
NdisGetReceivedPacket(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    IN  PNDIS_HANDLE    MacContext);

VOID
EXPIMP
NdisGetSystemUptime(
    OUT PULONG  pSystemUpTime);

VOID
EXPIMP
NdisInitializeReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock);

LONG
EXPIMP
NdisInterlockedDecrement(
    IN  PLONG   Addend);

LONG
EXPIMP
NdisInterlockedIncrement(
    IN  PLONG   Addend);

PSINGLE_LIST_ENTRY
EXPIMP
NdisInterlockedPopEntrySList(
    IN  PSLIST_HEADER   ListHead,
    IN  PKSPIN_LOCK     Lock);

PSINGLE_LIST_ENTRY
EXPIMP
NdisInterlockedPushEntrySList(
    IN  PSLIST_HEADER       ListHead,
    IN  PSINGLE_LIST_ENTRY  ListEntry,
    IN  PKSPIN_LOCK         Lock);


NDIS_STATUS
EXPIMP
NdisMDeregisterDevice(
    IN  NDIS_HANDLE NdisDeviceHandle);

VOID
EXPIMP
NdisMGetDeviceProperty(
    IN      NDIS_HANDLE         MiniportAdapterHandle,
    IN OUT  PDEVICE_OBJECT      *PhysicalDeviceObject           OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *FunctionalDeviceObject         OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *NextDeviceObject               OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResources             OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResourcesTranslated   OPTIONAL);

NDIS_STATUS
EXPIMP
NdisMInitializeScatterGatherDma(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  BOOLEAN     Dma64BitAddresses,
    IN  ULONG       MaximumPhysicalMapping);

NDIS_STATUS
EXPIMP
NdisMPromoteMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle);

NDIS_STATUS
EXPIMP
NdisMQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     MiniportAdapterHandle);

NDIS_STATUS
EXPIMP
NdisMRegisterDevice(
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  PNDIS_STRING        DeviceName,
    IN  PNDIS_STRING        SymbolicName,
    IN  PDRIVER_DISPATCH    MajorFunctions[],
    OUT PDEVICE_OBJECT      *pDeviceObject,
    OUT NDIS_HANDLE         *NdisDeviceHandle);

VOID
EXPIMP
NdisMRegisterUnloadHandler(
    IN  NDIS_HANDLE     NdisWrapperHandle,
    IN  PDRIVER_UNLOAD  UnloadHandler);

NDIS_STATUS
EXPIMP
NdisMRemoveMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle);

NDIS_STATUS
EXPIMP
NdisMSetMiniportSecondary(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE PrimaryMiniportAdapterHandle);

VOID
EXPIMP
NdisOpenConfigurationKeyByIndex(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ConfigurationHandle,
    IN  ULONG           Index,
    OUT PNDIS_STRING    KeyName,
    OUT PNDIS_HANDLE    KeyHandle);

VOID
EXPIMP
NdisOpenConfigurationKeyByName(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ConfigurationHandle,
    IN  PNDIS_STRING    SubKeyName,
    OUT PNDIS_HANDLE    SubKeyHandle);

UINT
EXPIMP
NdisPacketPoolUsage(
    IN  NDIS_HANDLE PoolHandle);

NDIS_STATUS
EXPIMP
NdisQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     NdisBindingHandle);

VOID
EXPIMP
NdisQueryBufferSafe(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length,
    IN  UINT            Priority);

ULONG
EXPIMP
NdisReadPcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length);

VOID
EXPIMP
NdisReleaseReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock,
    IN  PLOCK_STATE     LockState);


NDIS_STATUS
EXPIMP
NdisWriteEventLogEntry(
    IN  PVOID       LogHandle,
    IN  NDIS_STATUS EventCode,
    IN  ULONG       UniqueEventValue,
    IN  USHORT      NumStrings,
    IN  PVOID       StringsList OPTIONAL,
    IN  ULONG       DataSize,
    IN  PVOID       Data        OPTIONAL);

ULONG
EXPIMP
NdisWritePcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length);


/* Connectionless services */

NDIS_STATUS
EXPIMP
NdisClAddParty(
    IN      NDIS_HANDLE         NdisVcHandle,
    IN      NDIS_HANDLE         ProtocolPartyContext,
    IN OUT  PCO_CALL_PARAMETERS CallParameters,
    OUT     PNDIS_HANDLE        NdisPartyHandle);

NDIS_STATUS
EXPIMP
NdisClCloseAddressFamily(
    IN  NDIS_HANDLE NdisAfHandle);

NDIS_STATUS
EXPIMP
NdisClCloseCall(
    IN  NDIS_HANDLE NdisVcHandle,
    IN  NDIS_HANDLE NdisPartyHandle OPTIONAL,
    IN  PVOID       Buffer          OPTIONAL,
    IN  UINT        Size);

NDIS_STATUS
EXPIMP
NdisClDeregisterSap(
    IN  NDIS_HANDLE NdisSapHandle);

NDIS_STATUS
EXPIMP
NdisClDropParty(
    IN  NDIS_HANDLE NdisPartyHandle,
    IN  PVOID       Buffer  OPTIONAL,
    IN  UINT        Size);

VOID
EXPIMP
NdisClIncomingCallComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

NDIS_STATUS
EXPIMP
NdisClMakeCall(
    IN      NDIS_HANDLE         NdisVcHandle,
    IN OUT  PCO_CALL_PARAMETERS CallParameters,
    IN      NDIS_HANDLE         ProtocolPartyContext    OPTIONAL,
    OUT     PNDIS_HANDLE        NdisPartyHandle         OPTIONAL);

NDIS_STATUS 
EXPIMP
NdisClModifyCallQoS(
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);


NDIS_STATUS
EXPIMP
NdisClOpenAddressFamily(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY              AddressFamily,
    IN  NDIS_HANDLE                     ProtocolAfContext,
    IN  PNDIS_CLIENT_CHARACTERISTICS    ClCharacteristics,
    IN  UINT                            SizeOfClCharacteristics,
    OUT PNDIS_HANDLE                    NdisAfHandle);

NDIS_STATUS
EXPIMP
NdisClRegisterSap(
    IN  NDIS_HANDLE     NdisAfHandle,
    IN  NDIS_HANDLE     ProtocolSapContext,
    IN  PCO_SAP         Sap,
    OUT PNDIS_HANDLE    NdisSapHandle);


/* Call Manager services */

NDIS_STATUS
EXPIMP
NdisCmActivateVc(
    IN      NDIS_HANDLE         NdisVcHandle,
    IN OUT  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisCmAddPartyComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisPartyHandle,
    IN  NDIS_HANDLE         CallMgrPartyContext OPTIONAL,
    IN  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisCmCloseAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisAfHandle);

VOID
EXPIMP
NdisCmCloseCallComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisVcHandle,
    IN  NDIS_HANDLE NdisPartyHandle OPTIONAL);

NDIS_STATUS
EXPIMP
NdisCmDeactivateVc(
    IN  NDIS_HANDLE NdisVcHandle);

VOID
EXPIMP
NdisCmDeregisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisSapHandle);

VOID
EXPIMP
NdisCmDispatchCallConnected(
    IN  NDIS_HANDLE NdisVcHandle);

NDIS_STATUS
EXPIMP
NdisCmDispatchIncomingCall(
    IN  NDIS_HANDLE         NdisSapHandle,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisCmDispatchIncomingCallQoSChange(
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisCmDispatchIncomingCloseCall(
    IN  NDIS_STATUS CloseStatus,
    IN  NDIS_HANDLE NdisVcHandle,
    IN  PVOID       Buffer  OPTIONAL,
    IN  UINT        Size);

VOID
EXPIMP
NdisCmDispatchIncomingDropParty(
    IN  NDIS_STATUS DropStatus,
    IN  NDIS_HANDLE NdisPartyHandle,
    IN  PVOID       Buffer  OPTIONAL,
    IN  UINT        Size);

VOID
EXPIMP
NdisCmDropPartyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisPartyHandle);

VOID
EXPIMP
NdisCmMakeCallComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  NDIS_HANDLE         NdisPartyHandle     OPTIONAL,
    IN  NDIS_HANDLE         CallMgrPartyContext OPTIONAL,
    IN  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisCmModifyCallQoSComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisCmOpenAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisAfHandle,
    IN  NDIS_HANDLE CallMgrAfContext);

NDIS_STATUS
EXPIMP
NdisCmRegisterAddressFamily(
    IN  NDIS_HANDLE                         NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY                  AddressFamily,
    IN  PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
    IN  UINT                                SizeOfCmCharacteristics);

VOID
EXPIMP
NdisCmRegisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisSapHandle,
    IN  NDIS_HANDLE CallMgrSapContext);


NDIS_STATUS
EXPIMP
NdisMCmActivateVc(
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

NDIS_STATUS
EXPIMP
NdisMCmCreateVc(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisAfHandle,
    IN  NDIS_HANDLE     MiniportVcContext,
    OUT PNDIS_HANDLE    NdisVcHandle);

NDIS_STATUS
EXPIMP
NdisMCmDeactivateVc(
    IN  NDIS_HANDLE NdisVcHandle);

NDIS_STATUS
EXPIMP
NdisMCmDeleteVc(
    IN  NDIS_HANDLE NdisVcHandle);

NDIS_STATUS
EXPIMP
NdisMCmRegisterAddressFamily(
    IN  NDIS_HANDLE                         MiniportAdapterHandle,
    IN  PCO_ADDRESS_FAMILY                  AddressFamily,
    IN  PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
    IN  UINT                                SizeOfCmCharacteristics);

NDIS_STATUS
EXPIMP
NdisMCmRequest(
    IN      NDIS_HANDLE     NdisAfHandle,
    IN      NDIS_HANDLE     NdisVcHandle    OPTIONAL,
    IN      NDIS_HANDLE     NdisPartyHandle OPTIONAL,
    IN OUT  PNDIS_REQUEST   NdisRequest);


/* Connection-oriented services */

NDIS_STATUS
EXPIMP
NdisCoCreateVc(
    IN  NDIS_HANDLE         NdisBindingHandle,
    IN  NDIS_HANDLE         NdisAfHandle  OPTIONAL,
    IN  NDIS_HANDLE         ProtocolVcContext,
    IN  OUT PNDIS_HANDLE    NdisVcHandle);

NDIS_STATUS
EXPIMP
NdisCoDeleteVc(
    IN  NDIS_HANDLE NdisVcHandle);

NDIS_STATUS
EXPIMP
NdisCoRequest(
    IN      NDIS_HANDLE     NdisBindingHandle,
    IN      NDIS_HANDLE     NdisAfHandle    OPTIONAL,
    IN      NDIS_HANDLE     NdisVcHandle    OPTIONAL,
    IN      NDIS_HANDLE     NdisPartyHandle OPTIONAL,
    IN OUT  PNDIS_REQUEST   NdisRequest);

VOID
EXPIMP
NdisCoRequestComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisAfHandle,
    IN  NDIS_HANDLE     NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE     NdisPartyHandle OPTIONAL,
    IN  PNDIS_REQUEST   NdisRequest);

VOID
EXPIMP
NdisCoSendPackets(
    IN  NDIS_HANDLE     NdisVcHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);


VOID
EXPIMP
NdisMCoActivateVcComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters);

VOID
EXPIMP
NdisMCoDeactivateVcComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisVcHandle);

VOID
EXPIMP
NdisMCoIndicateReceivePacket(
    IN  NDIS_HANDLE     NdisVcHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);

VOID
EXPIMP
NdisMCoIndicateStatus(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE NdisVcHandle    OPTIONAL,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer    OPTIONAL,
    IN  ULONG       StatusBufferSize);

VOID
EXPIMP
NdisMCoReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle);

VOID
EXPIMP
NdisMCoRequestComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_REQUEST   Request);

VOID 
EXPIMP
NdisMCoSendComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisVcHandle,
    IN  PNDIS_PACKET    Packet);


/* NDIS 5.0 extensions for intermediate drivers */

VOID
EXPIMP
NdisIMAssociateMiniport(
    IN  NDIS_HANDLE DriverHandle,
    IN  NDIS_HANDLE ProtocolHandle);

NDIS_STATUS 
EXPIMP
NdisIMCancelInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance);

VOID
EXPIMP
NdisIMCopySendCompletePerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket);

VOID
EXPIMP
NdisIMCopySendPerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket);

VOID
EXPIMP
NdisIMDeregisterLayeredMiniport(
    IN  NDIS_HANDLE DriverHandle);

NDIS_HANDLE
EXPIMP
NdisIMGetBindingContext(
    IN  NDIS_HANDLE NdisBindingHandle);

NDIS_HANDLE
EXPIMP
NdisIMGetDeviceContext(
    IN  NDIS_HANDLE MiniportAdapterHandle);

NDIS_STATUS
EXPIMP
NdisIMInitializeDeviceInstanceEx(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DriverInstance,
    IN  NDIS_HANDLE     DeviceContext   OPTIONAL);

#endif /* NDIS50 */



/* Prototypes for NDIS_MINIPORT_CHARACTERISTICS */

typedef BOOLEAN (*W_CHECK_FOR_HANG_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext);

typedef VOID (*W_DISABLE_INTERRUPT_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext);

typedef VOID (*W_ENABLE_INTERRUPT_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext);

typedef VOID (*W_HALT_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext);

typedef VOID (*W_HANDLE_INTERRUPT_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext);

typedef NDIS_STATUS (*W_INITIALIZE_HANDLER)(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext);

typedef VOID (*W_ISR_HANDLER)(
    OUT PBOOLEAN				InterruptRecognized,
    OUT PBOOLEAN				QueueMiniportHandleInterrupt,
    IN	NDIS_HANDLE				MiniportAdapterContext);

typedef NDIS_STATUS (*W_QUERY_INFORMATION_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded);

typedef NDIS_STATUS (*W_RECONFIGURE_HANDLER)(
    OUT PNDIS_STATUS    OpenErrorStatus,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE	    WrapperConfigurationContext);

typedef NDIS_STATUS (*W_RESET_HANDLER)(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext);

typedef NDIS_STATUS (*W_SEND_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_PACKET    Packet,
    IN  UINT            Flags);

typedef NDIS_STATUS (*WM_SEND_HANDLER)(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  NDIS_HANDLE         NdisLinkHandle,
    IN  PNDIS_WAN_PACKET    Packet);

typedef NDIS_STATUS (*W_SET_INFORMATION_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded);

typedef NDIS_STATUS (*W_TRANSFER_DATA_HANDLER)(
    OUT PNDIS_PACKET    Packet,
    OUT PUINT           BytesTransferred,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     MiniportReceiveContext,
    IN  UINT            ByteOffset,
    IN  UINT            BytesToTransfer);

typedef NDIS_STATUS (*WM_TRANSFER_DATA_HANDLER)(
    VOID);



/* NDIS structures available only to miniport drivers */

/* Technology specific defines */

#define DECLARE_UNKNOWN_STRUCT(BaseName) \
    typedef struct _##BaseName BaseName, *P##BaseName;

#define DECLARE_UNKNOWN_PROTOTYPE(Name) \
    typedef VOID (*##Name)(VOID);


/* ARCnet */

typedef struct _ARC_BUFFER_LIST
{
    PVOID                   Buffer;
    UINT                    Size;
    UINT                    BytesLeft;
    struct _ARC_BUFFER_LIST *Next;
} ARC_BUFFER_LIST, *PARC_BUFFER_LIST;

DECLARE_UNKNOWN_STRUCT(ARC_FILTER)


VOID
EXPIMP
ArcFilterDprIndicateReceive(
    IN  PARC_FILTER Filter,
    IN  PUCHAR      pRawHeader,
    IN  PUCHAR      pData,
    IN  UINT        Length);

VOID
EXPIMP
ArcFilterDprIndicateReceiveComplete(
    IN  PARC_FILTER Filter);


/* Ethernet */

#define ETH_LENGTH_OF_ADDRESS   6

DECLARE_UNKNOWN_STRUCT(ETH_BINDING_INFO);

DECLARE_UNKNOWN_PROTOTYPE(ETH_ADDRESS_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(ETH_FILTER_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(ETH_DEFERRED_CLOSE)

typedef struct _ETH_FILTER
{
    PNDIS_SPIN_LOCK             Lock;
    CHAR                        (*MCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
    struct _NDIS_MINIPORT_BLOCK *Miniport;
    UINT                        CombinedPacketFilter;
    PETH_BINDING_INFO           OpenList;
    ETH_ADDRESS_CHANGE          AddressChangeAction;
    ETH_FILTER_CHANGE           FilterChangeAction;
    ETH_DEFERRED_CLOSE          CloseAction;
    UINT                        MaxMulticastAddresses;
    UINT                        NumAddresses;
    UCHAR                       AdapterAddress[ETH_LENGTH_OF_ADDRESS];
    UINT                        OldCombinedPacketFilter;
    CHAR                        (*OldMCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
    UINT                        OldNumAddresses;
    PETH_BINDING_INFO           DirectedList;
    PETH_BINDING_INFO           BMList;
    PETH_BINDING_INFO           MCastSet;
#if 0
#ifdef NDIS_WRAPPER
	UINT                        NumOpens;
	NDIS_RW_LOCK                BindListLock;
#endif
#endif
} ETH_FILTER, *PETH_FILTER;


NDIS_STATUS
EXPIMP
EthChangeFilterAddresses(
    IN  PETH_FILTER     Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            AddressCount,
    IN  CHAR            Addresses[] [ETH_LENGTH_OF_ADDRESS],
    IN  BOOLEAN         Set);

BOOLEAN
EXPIMP
EthCreateFilter(
    IN  UINT                MaximumMulticastAddresses,
    IN  ETH_ADDRESS_CHANGE  AddressChangeAction,
    IN  ETH_FILTER_CHANGE   FilterChangeAction,
    IN  ETH_DEFERRED_CLOSE  CloseAction,
    IN  PUCHAR              AdapterAddress,
    IN  PNDIS_SPIN_LOCK     Lock,
    OUT PETH_FILTER         *Filter);

VOID
EXPIMP
EthDeleteFilter(
    IN  PETH_FILTER Filter);

NDIS_STATUS
EXPIMP
EthDeleteFilterOpenAdapter(
    IN  PETH_FILTER	Filter,
    IN  NDIS_HANDLE	NdisFilterHandle,
    IN  PNDIS_REQUEST	NdisRequest);

NDIS_STATUS
EXPIMP
EthFilterAdjust(
    IN  PETH_FILTER     Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            FilterClasses,
    IN  BOOLEAN         Set);

VOID
EXPIMP
EthFilterIndicateReceive(
    IN	PETH_FILTER Filter,
    IN	NDIS_HANDLE MacReceiveContext,
    IN	PCHAR       Address,
    IN	PVOID       HeaderBuffer,
    IN	UINT        HeaderBufferSize,
    IN	PVOID       LookaheadBuffer,
    IN	UINT        LookaheadBufferSize,
    IN	UINT        PacketSize);

VOID
EXPIMP
EthFilterIndicateReceiveComplete(
    IN  PETH_FILTER Filter);

BOOLEAN
EXPIMP
EthNoteFilterOpenAdapter(
    IN  PETH_FILTER     Filter,
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     NdisBindingContext,
    OUT PNDIS_HANDLE    NdisFilterHandle);

UINT
EXPIMP
EthNumberOfOpenFilterAddresses(
    IN  PETH_FILTER Filter,
    IN  NDIS_HANDLE NdisFilterHandle);

VOID
EXPIMP
EthQueryGlobalFilterAddresses (
    OUT PNDIS_STATUS    Status,
    IN  PETH_FILTER     Filter,
    IN  UINT            SizeOfArray,
    OUT PUINT           NumberOfAddresses,
    IN  OUT	CHAR        AddressArray[] [ETH_LENGTH_OF_ADDRESS]);

VOID
EXPIMP
EthQueryOpenFilterAddresses(
    OUT	    PNDIS_STATUS    Status,
    IN	    PETH_FILTER     Filter,
    IN	    NDIS_HANDLE     NdisFilterHandle,
    IN	    UINT            SizeOfArray,
    OUT	    PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray[] [ETH_LENGTH_OF_ADDRESS]);

BOOLEAN
EXPIMP
EthShouldAddressLoopBack(
    IN  PETH_FILTER Filter,
    IN  CHAR        Address[ETH_LENGTH_OF_ADDRESS]);


/* FDDI */

#define FDDI_LENGTH_OF_LONG_ADDRESS     6
#define FDDI_LENGTH_OF_SHORT_ADDRESS    2

DECLARE_UNKNOWN_STRUCT(FDDI_FILTER)

DECLARE_UNKNOWN_PROTOTYPE(FDDI_ADDRESS_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(FDDI_FILTER_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(FDDI_DEFERRED_CLOSE)


NDIS_STATUS
EXPIMP
FddiChangeFilterLongAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            AddressCount,
    IN  CHAR            Addresses[] [FDDI_LENGTH_OF_LONG_ADDRESS],
    IN  BOOLEAN         Set);

NDIS_STATUS
EXPIMP
FddiChangeFilterShortAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            AddressCount,
    IN  CHAR            Addresses[] [FDDI_LENGTH_OF_SHORT_ADDRESS],
    IN  BOOLEAN         Set);

BOOLEAN
EXPIMP
FddiCreateFilter(
    IN  UINT                MaximumMulticastLongAddresses,
    IN  UINT                MaximumMulticastShortAddresses,
    IN  FDDI_ADDRESS_CHANGE AddressChangeAction,
    IN  FDDI_FILTER_CHANGE  FilterChangeAction,
    IN  FDDI_DEFERRED_CLOSE CloseAction,
    IN  PUCHAR              AdapterLongAddress,
    IN  PUCHAR              AdapterShortAddress,
    IN  PNDIS_SPIN_LOCK     Lock,
    OUT PFDDI_FILTER        *Filter);

VOID
EXPIMP
FddiDeleteFilter(
    IN  PFDDI_FILTER    Filter);

NDIS_STATUS
EXPIMP
FddiDeleteFilterOpenAdapter(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest);

NDIS_STATUS
EXPIMP
FddiFilterAdjust(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            FilterClasses,
    IN  BOOLEAN         Set);

VOID
EXPIMP
FddiFilterIndicateReceive(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  PCHAR           Address,
    IN  UINT            AddressLength,
    IN  PVOID           HeaderBuffer,
    IN  UINT            HeaderBufferSize,
    IN  PVOID           LookaheadBuffer,
    IN  UINT            LookaheadBufferSize,
    IN  UINT            PacketSize);

VOID
EXPIMP
FddiFilterIndicateReceiveComplete(
    IN  PFDDI_FILTER    Filter);

BOOLEAN
EXPIMP
FddiNoteFilterOpenAdapter(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     NdisBindingContext,
    OUT PNDIS_HANDLE    NdisFilterHandle);

UINT
EXPIMP
FddiNumberOfOpenFilterLongAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle);

UINT
EXPIMP
FddiNumberOfOpenFilterShortAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle);

VOID
EXPIMP
FddiQueryGlobalFilterLongAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray[] [FDDI_LENGTH_OF_LONG_ADDRESS]);

VOID
EXPIMP
FddiQueryGlobalFilterShortAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray[] [FDDI_LENGTH_OF_SHORT_ADDRESS]);

VOID
EXPIMP
FddiQueryOpenFilterLongAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      NDIS_HANDLE     NdisFilterHandle,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray[] [FDDI_LENGTH_OF_LONG_ADDRESS]);

VOID
EXPIMP
FddiQueryOpenFilterShortAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      NDIS_HANDLE     NdisFilterHandle,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray[] [FDDI_LENGTH_OF_SHORT_ADDRESS]);

BOOLEAN
EXPIMP
FddiShouldAddressLoopBack(
    IN  PFDDI_FILTER    Filter,
    IN  CHAR            Address[],
    IN  UINT            LengthOfAddress);


/* Token Ring */

#define TR_LENGTH_OF_FUNCTIONAL 4
#define TR_LENGTH_OF_ADDRESS    6

DECLARE_UNKNOWN_STRUCT(TR_FILTER)

DECLARE_UNKNOWN_PROTOTYPE(TR_ADDRESS_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(TR_GROUP_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(TR_FILTER_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(TR_DEFERRED_CLOSE)


NDIS_STATUS
EXPIMP
TrChangeFunctionalAddress(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  CHAR            FunctionalAddressArray[TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN         Set);

NDIS_STATUS
EXPIMP
TrChangeGroupAddress(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  CHAR            GroupAddressArray[TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN         Set);

BOOLEAN
EXPIMP
TrCreateFilter(
    IN  TR_ADDRESS_CHANGE   AddressChangeAction,
    IN  TR_GROUP_CHANGE     GroupChangeAction,
    IN  TR_FILTER_CHANGE    FilterChangeAction,
    IN  TR_DEFERRED_CLOSE   CloseAction,
    IN  PUCHAR              AdapterAddress,
    IN  PNDIS_SPIN_LOCK     Lock,
    OUT PTR_FILTER          *Filter);

VOID
EXPIMP
TrDeleteFilter(
    IN  PTR_FILTER  Filter);

NDIS_STATUS
EXPIMP
TrDeleteFilterOpenAdapter (
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest);

NDIS_STATUS
EXPIMP
TrFilterAdjust(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            FilterClasses,
    IN  BOOLEAN         Set);

VOID
EXPIMP
TrFilterIndicateReceive(
    IN  PTR_FILTER  Filter,
    IN  NDIS_HANDLE MacReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize);

VOID
EXPIMP
TrFilterIndicateReceiveComplete(
    IN  PTR_FILTER  Filter);

BOOLEAN
EXPIMP
TrNoteFilterOpenAdapter(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     NdisBindingContext,
    OUT PNDIS_HANDLE    NdisFilterHandle);

BOOLEAN
EXPIMP
TrShouldAddressLoopBack(
    IN  PTR_FILTER  Filter,
    IN  CHAR        DestinationAddress[TR_LENGTH_OF_ADDRESS],
    IN  CHAR        SourceAddress[TR_LENGTH_OF_ADDRESS]);



#ifdef _MSC_VER
typedef struct _NDIS30_MINIPORT_CHARACTERISTICS
{
    UCHAR                           MajorNdisVersion;
    UCHAR                           MinorNdisVersion;
    UINT                            Reserved;
    W_CHECK_FOR_HANG_HANDLER        CheckForHangHandler;
    W_DISABLE_INTERRUPT_HANDLER     DisableInterruptHandler;
    W_ENABLE_INTERRUPT_HANDLER      EnableInterruptHandler;
    W_HALT_HANDLER                  HaltHandler;
    W_HANDLE_INTERRUPT_HANDLER      HandleInterruptHandler;
    W_INITIALIZE_HANDLER            InitializeHandler;
    W_ISR_HANDLER                   ISRHandler;
    W_QUERY_INFORMATION_HANDLER     QueryInformationHandler;
    W_RECONFIGURE_HANDLER           ReconfigureHandler;
    W_RESET_HANDLER                 ResetHandler;
    union
    {
        W_SEND_HANDLER              SendHandler;
        WM_SEND_HANDLER             WanSendHandler;
    } u1;
    W_SET_INFORMATION_HANDLER       SetInformationHandler;
    union
    {
        W_TRANSFER_DATA_HANDLER     TransferDataHandler;
        WM_TRANSFER_DATA_HANDLER    WanTransferDataHandler;
    } u2;
} NDIS30_MINIPORT_CHARACTERISTICS;
typedef NDIS30_MINIPORT_CHARACTERISTICS NDIS30_MINIPORT_CHARACTERISTICS_S;
#else
#define NDIS30_MINIPORT_CHARACTERISTICS \
    UCHAR                           MajorNdisVersion; \
    UCHAR                           MinorNdisVersion; \
    UINT                            Reserved; \
    W_CHECK_FOR_HANG_HANDLER        CheckForHangHandler; \
    W_DISABLE_INTERRUPT_HANDLER     DisableInterruptHandler; \
    W_ENABLE_INTERRUPT_HANDLER      EnableInterruptHandler; \
    W_HALT_HANDLER                  HaltHandler; \
    W_HANDLE_INTERRUPT_HANDLER      HandleInterruptHandler; \
    W_INITIALIZE_HANDLER            InitializeHandler; \
    W_ISR_HANDLER                   ISRHandler; \
    W_QUERY_INFORMATION_HANDLER     QueryInformationHandler; \
    W_RECONFIGURE_HANDLER           ReconfigureHandler; \
    W_RESET_HANDLER                 ResetHandler; \
    union \
    { \
        W_SEND_HANDLER              SendHandler; \
        WM_SEND_HANDLER             WanSendHandler; \
    } u1; \
    W_SET_INFORMATION_HANDLER       SetInformationHandler; \
    union \
    { \
        W_TRANSFER_DATA_HANDLER     TransferDataHandler; \
        WM_TRANSFER_DATA_HANDLER    WanTransferDataHandler; \
    } u2; 
typedef struct _NDIS30_MINIPORT_CHARACTERISTICS_S
{
   NDIS30_MINIPORT_CHARACTERISTICS;
} NDIS30_MINIPORT_CHARACTERISTICS_S, *PSNDIS30_MINIPORT_CHARACTERISTICS_S;
#endif

/* Extensions for NDIS 4.0 miniports */

typedef VOID (*W_SEND_PACKETS_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);

#ifdef NDIS40

typedef VOID (*W_RETURN_PACKET_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_PACKET    Packet);

typedef VOID (*W_ALLOCATE_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PVOID                   VirtualAddress,
    IN  PNDIS_PHYSICAL_ADDRESS  PhysicalAddress,
    IN  ULONG                   Length,
    IN  PVOID                   Context);

#ifdef _MSC_VER
typedef struct _NDIS40_MINIPORT_CHARACTERISTICS
{
    NDIS30_MINIPORT_CHARACTERISTICS;

    W_RETURN_PACKET_HANDLER     ReturnPacketHandler;
    W_SEND_PACKETS_HANDLER      SendPacketsHandler;
    W_ALLOCATE_COMPLETE_HANDLER AllocateCompleteHandler;
} NDIS40_MINIPORT_CHARACTERISTICS;
typedef NDIS40_MINIPORT_CHARACTERISTICS NDIS40_MINIPORT_CHARACTERISTICS_S;
#else
#define NDIS40_MINIPORT_CHARACTERISTICS \
    NDIS30_MINIPORT_CHARACTERISTICS; \
    W_RETURN_PACKET_HANDLER     ReturnPacketHandler; \
    W_SEND_PACKETS_HANDLER      SendPacketsHandler; \
    W_ALLOCATE_COMPLETE_HANDLER AllocateCompleteHandler;
typedef struct _NDIS40_MINIPORT_CHARACTERISTICS_S
{
   NDIS40_MINIPORT_CHARACTERISTICS;
} NDIS40_MINIPORT_CHARACTERISTICS_S, *PSNDIS40_MINIPORT_CHARACTERISTICS_S;
#endif

#endif /* NDIS40 */

/* Extensions for NDIS 5.0 miniports */

#ifdef NDIS50

typedef NDIS_STATUS (*W_CO_CREATE_VC_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     NdisVcHandle,
    OUT PNDIS_HANDLE    MiniportVcContext);

typedef NDIS_STATUS (*W_CO_DELETE_VC_HANDLER)(
    IN  NDIS_HANDLE MiniportVcContext);

typedef NDIS_STATUS (*W_CO_ACTIVATE_VC_HANDLER)(
    IN      NDIS_HANDLE         MiniportVcContext,
    IN OUT  PCO_CALL_PARAMETERS CallParameters);

typedef NDIS_STATUS (*W_CO_DEACTIVATE_VC_HANDLER)(
    IN  NDIS_HANDLE MiniportVcContext);

typedef VOID (*W_CO_SEND_PACKETS_HANDLER)(
    IN  NDIS_HANDLE     MiniportVcContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);

typedef NDIS_STATUS (*W_CO_REQUEST_HANDLER)(
    IN      NDIS_HANDLE     MiniportAdapterContext,
    IN      NDIS_HANDLE     MiniportVcContext   OPTIONAL,
    IN OUT  PNDIS_REQUEST   NdisRequest);

#ifdef _MSC_VER
typedef struct _NDIS50_MINIPORT_CHARACTERISTICS
{
    NDIS40_MINIPORT_CHARACTERISTICS;

    W_CO_CREATE_VC_HANDLER      CoCreateVcHandler;
    W_CO_DELETE_VC_HANDLER	    CoDeleteVcHandler;
    W_CO_ACTIVATE_VC_HANDLER    CoActivateVcHandler;
    W_CO_DEACTIVATE_VC_HANDLER  CoDeactivateVcHandler;
    W_CO_SEND_PACKETS_HANDLER   CoSendPacketsHandler;
    W_CO_REQUEST_HANDLER        CoRequestHandler;
} NDIS50_MINIPORT_CHARACTERISTICS;
typedef NDIS50_MINIPORT_CHARACTERISTICS NDIS50_MINIPORT_CHARACTERISTICS_S;
#else
#define NDIS50_MINIPORT_CHARACTERISTICS \
    NDIS40_MINIPORT_CHARACTERISTICS; \
    W_CO_CREATE_VC_HANDLER      CoCreateVcHandler; \
    W_CO_DELETE_VC_HANDLER	    CoDeleteVcHandler; \
    W_CO_ACTIVATE_VC_HANDLER    CoActivateVcHandler; \
    W_CO_DEACTIVATE_VC_HANDLER  CoDeactivateVcHandler; \
    W_CO_SEND_PACKETS_HANDLER   CoSendPacketsHandler; \
    W_CO_REQUEST_HANDLER        CoRequestHandler;
typedef struct _NDIS50_MINIPORT_CHARACTERISTICS_S
{
   NDIS50_MINIPORT_CHARACTERISTICS;
} NDIS50_MINIPORT_CHARACTERISTICS_S, *PSNDIS50_MINIPORT_CHARACTERISTICS_S;
#endif

#endif /* NDIS50 */


#ifndef NDIS50
#ifndef NDIS40
typedef struct _NDIS_MINIPORT_CHARACTERISTICS	
{
   NDIS30_MINIPORT_CHARACTERISTICS;
} NDIS_MINIPORT_CHARACTERISTICS;
#else /* NDIS40 */
typedef struct _NDIS_MINIPORT_CHARACTERISTICS 
{
   NDIS40_MINIPORT_CHARACTERISTICS;
} NDIS_MINIPORT_CHARACTERISTICS;
#endif /* NDIS40 */
#else /* NDIS50 */
typedef struct _NDIS_MINIPORT_CHARACTERISTICS  
{
   NDIS50_MINIPORT_CHARACTERISTICS;
} NDIS_MINIPORT_CHARACTERISTICS;
#endif /* NDIS50 */

typedef	NDIS_MINIPORT_CHARACTERISTICS *PNDIS_MINIPORT_CHARACTERISTICS;



typedef NDIS_STATUS (*SEND_HANDLER)(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  PNDIS_PACKET    Packet);

typedef NDIS_STATUS (*TRANSFER_DATA_HANDLER)(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  UINT            ByteOffset,
    IN  UINT            BytesToTransfer,
    OUT PNDIS_PACKET    Packet,
    OUT PUINT           BytesTransferred);

typedef NDIS_STATUS (*RESET_HANDLER)(
    IN  NDIS_HANDLE MacBindingHandle);

typedef NDIS_STATUS (*REQUEST_HANDLER)(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest);



/* Structures available only to full MAC drivers */

typedef BOOLEAN (*PNDIS_INTERRUPT_SERVICE)(
    IN  PVOID   InterruptContext);

typedef VOID (*PNDIS_DEFERRED_PROCESSING)(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   InterruptContext,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3);


typedef struct _NDIS_INTERRUPT
{
    PKINTERRUPT                 InterruptObject;
    KSPIN_LOCK                  DpcCountLock;
    PNDIS_INTERRUPT_SERVICE     MacIsr;
    PNDIS_DEFERRED_PROCESSING   MacDpc;
    KDPC                        InterruptDpc;
    PVOID                       InterruptContext;
    UCHAR                       DpcCount;
    BOOLEAN                     Removing;
    /* Used to tell when all DPCs for the adapter are completed */
    KEVENT						DpcsCompletedEvent;
} NDIS_INTERRUPT, *PNDIS_INTERRUPT;


/* NDIS adapter information */

typedef NDIS_STATUS (*PNDIS_ACTIVATE_CALLBACK)(
    IN  NDIS_HANDLE NdisAdatperHandle,
    IN  NDIS_HANDLE MacAdapterContext,
    IN  ULONG       DmaChannel);

typedef struct _NDIS_PORT_DESCRIPTOR
{
    ULONG   InitialPort;
    ULONG   NumberOfPorts;
    PVOID * PortOffset;
} NDIS_PORT_DESCRIPTOR, *PNDIS_PORT_DESCRIPTOR;

typedef struct _NDIS_ADAPTER_INFORMATION
{
    ULONG                   DmaChannel;
    BOOLEAN	                Master;
    BOOLEAN	                Dma32BitAddresses;
    PNDIS_ACTIVATE_CALLBACK ActivateCallback;
    NDIS_INTERFACE_TYPE     AdapterType;
    ULONG                   PhysicalMapRegistersNeeded;
    ULONG                   MaximumPhysicalMapping;
    ULONG                   NumberOfPortDescriptors;
    NDIS_PORT_DESCRIPTOR    PortDescriptors[1];
} NDIS_ADAPTER_INFORMATION, *PNDIS_ADAPTER_INFORMATION;


/* Prototypes for NDIS_MAC_CHARACTERISTICS */

typedef NDIS_STATUS (*OPEN_ADAPTER_HANDLER)(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT NDIS_HANDLE *   MacBindingHandle,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     NdisBindingContext,
    IN  NDIS_HANDLE     MacAdapterContext,
    IN  UINT            OpenOptions,
    IN  PSTRING         AddressingInformation OPTIONAL);

typedef NDIS_STATUS (*CLOSE_ADAPTER_HANDLER)(
    IN  NDIS_HANDLE MacBindingHandle);

typedef NDIS_STATUS (*WAN_TRANSFER_DATA_HANDLER)(
    VOID);

typedef NDIS_STATUS (*QUERY_GLOBAL_STATISTICS_HANDLER)(
    IN  NDIS_HANDLE     MacAdapterContext,
    IN  PNDIS_REQUEST   NdisRequest);

typedef VOID (*UNLOAD_MAC_HANDLER)(
    IN  NDIS_HANDLE MacMacContext);

typedef NDIS_STATUS (*ADD_ADAPTER_HANDLER)(
    IN  NDIS_HANDLE     MacMacContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext,
    IN  PNDIS_STRING    AdapterName);

typedef VOID (*REMOVE_ADAPTER_HANDLER)(
    IN  NDIS_HANDLE MacAdapterContext);

typedef struct _NDIS_MAC_CHARACTERISTICS
{
    UCHAR                           MajorNdisVersion;
    UCHAR                           MinorNdisVersion;
    UINT                            Reserved;
    OPEN_ADAPTER_HANDLER            OpenAdapterHandler;
    CLOSE_ADAPTER_HANDLER           CloseAdapterHandler;
    SEND_HANDLER                    SendHandler;
    TRANSFER_DATA_HANDLER           TransferDataHandler;
    RESET_HANDLER                   ResetHandler;
    REQUEST_HANDLER                 RequestHandler;
    QUERY_GLOBAL_STATISTICS_HANDLER QueryGlobalStatisticsHandler;
    UNLOAD_MAC_HANDLER              UnloadMacHandler;
    ADD_ADAPTER_HANDLER             AddAdapterHandler;
    REMOVE_ADAPTER_HANDLER          RemoveAdapterHandler;
    NDIS_STRING                     Name;
} NDIS_MAC_CHARACTERISTICS, *PNDIS_MAC_CHARACTERISTICS;

typedef	NDIS_MAC_CHARACTERISTICS        NDIS_WAN_MAC_CHARACTERISTICS;
typedef	NDIS_WAN_MAC_CHARACTERISTICS    *PNDIS_WAN_MAC_CHARACTERISTICS;



VOID
EXPIMP
NdisAllocateSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    OUT PVOID                   *VirtualAddress,
    OUT PNDIS_PHYSICAL_ADDRESS  PhysicalAddress);

VOID
EXPIMP
NdisCompleteCloseAdapter(
    IN  NDIS_HANDLE NdisBindingContext,
    IN  NDIS_STATUS Status);

VOID
EXPIMP
NdisCompleteOpenAdapter(
    IN  NDIS_HANDLE NdisBindingContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus);

NDIS_STATUS
EXPIMP
NdisDeregisterAdapter(
    IN  NDIS_HANDLE NdisAdapterHandle);

VOID
EXPIMP
NdisDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE NdisAdapterHandle);

VOID
EXPIMP
NdisFreeSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);

VOID
EXPIMP
NdisInitializeInterrupt(
    OUT     PNDIS_STATUS                Status,
    IN OUT  PNDIS_INTERRUPT             Interrupt,
    IN      NDIS_HANDLE                 NdisAdapterHandle,
    IN      PNDIS_INTERRUPT_SERVICE     InterruptServiceRoutine,
    IN      PVOID                       InterruptContext,
    IN      PNDIS_DEFERRED_PROCESSING   DeferredProcessingRoutine,
    IN      UINT                        InterruptVector,
    IN      UINT                        InterruptLevel,
    IN      BOOLEAN                     SharedInterrupt,
    IN      NDIS_INTERRUPT_MODE         InterruptMode);

VOID
EXPIMP
NdisMapIoSpace(
    OUT PNDIS_STATUS            Status,
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length);

NDIS_STATUS
EXPIMP
NdisRegisterAdapter(
    OUT PNDIS_HANDLE    NdisAdapterHandle,
    IN  NDIS_HANDLE     NdisMacHandle,
    IN  NDIS_HANDLE     MacAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext,
    IN  PNDIS_STRING    AdapterName,
    IN  PVOID           AdapterInformation);

VOID
EXPIMP
NdisRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 NdisAdapterHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler);

VOID
EXPIMP
NdisRegisterMac(
    OUT PNDIS_STATUS                Status,
    OUT PNDIS_HANDLE                NdisMacHandle,
    IN  NDIS_HANDLE                 NdisWrapperHandle,
    IN  NDIS_HANDLE                 MacMacContext,
    IN  PNDIS_MAC_CHARACTERISTICS   MacCharacteristics,
    IN  UINT                        CharacteristicsLength);

VOID
EXPIMP
NdisReleaseAdapterResources(
    IN  NDIS_HANDLE NdisAdapterHandle);

VOID
EXPIMP
NdisRemoveInterrupt(
    IN  PNDIS_INTERRUPT Interrupt);



typedef struct _NDIS_MAC_BLOCK      NDIS_MAC_BLOCK,      *PNDIS_MAC_BLOCK;
typedef struct _NDIS_ADAPTER_BLOCK	NDIS_ADAPTER_BLOCK,  *PNDIS_ADAPTER_BLOCK;
typedef struct _NDIS_MINIPORT_BLOCK NDIS_MINIPORT_BLOCK, *PNDIS_MINIPORT_BLOCK;
typedef struct _NDIS_PROTOCOL_BLOCK NDIS_PROTOCOL_BLOCK, *PNDIS_PROTOCOL_BLOCK;
typedef struct _NDIS_OPEN_BLOCK		NDIS_OPEN_BLOCK,     *PNDIS_OPEN_BLOCK;
typedef struct _NDIS_M_DRIVER_BLOCK NDIS_M_DRIVER_BLOCK, *PNDIS_M_DRIVER_BLOCK;
typedef	struct _NDIS_AF_LIST        NDIS_AF_LIST,        *PNDIS_AF_LIST;
typedef	struct _NULL_FILTER         NULL_FILTER,         *PNULL_FILTER;


typedef struct _REFERENCE
{
    KSPIN_LOCK  SpinLock;
    USHORT      ReferenceCount;
    BOOLEAN     Closing;
} REFERENCE, *PREFERENCE;

typedef struct _NDIS_MINIPORT_INTERRUPT
{
    PKINTERRUPT                 InterruptObject;
    KSPIN_LOCK                  DpcCountLock;
    PVOID                       MiniportIdField;
    W_ISR_HANDLER               MiniportIsr;
    W_HANDLE_INTERRUPT_HANDLER  MiniportDpc;
    KDPC                        InterruptDpc;
    PNDIS_MINIPORT_BLOCK        Miniport;

    UCHAR                       DpcCount;
    BOOLEAN                     Filler1;

    KEVENT                      DpcsCompletedEvent;

    BOOLEAN                     SharedInterrupt;
    BOOLEAN	                    IsrRequested;
} NDIS_MINIPORT_INTERRUPT, *PNDIS_MINIPORT_INTERRUPT;

typedef struct _NDIS_MINIPORT_TIMER
{
    KTIMER                      Timer;
    KDPC                        Dpc;
    PNDIS_TIMER_FUNCTION        MiniportTimerFunction;
    PVOID                       MiniportTimerContext;
    PNDIS_MINIPORT_BLOCK        Miniport;
    struct _NDIS_MINIPORT_TIMER *NextDeferredTimer;
} NDIS_MINIPORT_TIMER, *PNDIS_MINIPORT_TIMER;


typedef struct _MAP_REGISTER_ENTRY
{
    PVOID   MapRegister;
    BOOLEAN WriteToDevice;
} MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;


typedef enum _NDIS_WORK_ITEM_TYPE
{
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

#define	NUMBER_OF_WORK_ITEM_TYPES   NdisMaxWorkItems
#define	NUMBER_OF_SINGLE_WORK_ITEMS 6

typedef struct _NDIS_MINIPORT_WORK_ITEM
{
    SINGLE_LIST_ENTRY   Link;
    NDIS_WORK_ITEM_TYPE WorkItemType;
    PVOID               WorkItemContext;
    BOOLEAN             Allocated;
    NDIS_HANDLE         Initiator;
} NDIS_MINIPORT_WORK_ITEM, *PNDIS_MINIPORT_WORK_ITEM;


typedef struct _NDIS_BIND_PATHS
{
    UINT        Number;
    NDIS_STRING Paths[1];
} NDIS_BIND_PATHS, *PNDIS_BIND_PATHS;

typedef struct _FILTERDBS
{
    union
    {
        PETH_FILTER     EthDB;
        PNULL_FILTER    NullDB;
    } u;
    PTR_FILTER          TrDB;
    PFDDI_FILTER        FddiDB;
    PARC_FILTER         ArcDB;
} FILTERDBS, *PFILTERDBS;


typedef VOID (*ETH_RCV_COMPLETE_HANDLER)(
    IN  PETH_FILTER Filter);

typedef VOID (*ETH_RCV_INDICATE_HANDLER)(
    IN  PETH_FILTER Filter,
    IN  NDIS_HANDLE MacReceiveContext,
    IN  PCHAR       Address,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize);

typedef VOID (*FDDI_RCV_COMPLETE_HANDLER)(
    IN  PFDDI_FILTER    Filter);

typedef VOID (*FDDI_RCV_INDICATE_HANDLER)(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  PCHAR           Address,
    IN  UINT            AddressLength,
    IN  PVOID           HeaderBuffer,
    IN  UINT            HeaderBufferSize,
    IN  PVOID           LookaheadBuffer,
    IN  UINT            LookaheadBufferSize,
    IN  UINT            PacketSize);

typedef VOID (*FILTER_PACKET_INDICATION_HANDLER)(
    IN  NDIS_HANDLE     Miniport,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);

typedef VOID (*TR_RCV_COMPLETE_HANDLER)(
    IN  PTR_FILTER  Filter);

typedef VOID (*TR_RCV_INDICATE_HANDLER)(
    IN  PTR_FILTER  Filter,
    IN  NDIS_HANDLE MacReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize);

typedef VOID (*WAN_RCV_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE NdisLinkContext);

typedef VOID (*WAN_RCV_HANDLER)(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisLinkContext,
    IN  PUCHAR          Packet,
    IN  ULONG           PacketSize);

typedef VOID (FASTCALL *NDIS_M_DEQUEUE_WORK_ITEM)(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_WORK_ITEM_TYPE     WorkItemType,
    OUT PVOID                   *WorkItemContext);

typedef VOID (FASTCALL *NDIS_M_PROCESS_DEFERRED)(
    IN  PNDIS_MINIPORT_BLOCK    Miniport);

typedef NDIS_STATUS (FASTCALL *NDIS_M_QUEUE_NEW_WORK_ITEM)(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_WORK_ITEM_TYPE     WorkItemType,
    IN  PVOID                   WorkItemContext);

typedef NDIS_STATUS (FASTCALL *NDIS_M_QUEUE_WORK_ITEM)(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_WORK_ITEM_TYPE     WorkItemType,
    IN  PVOID                   WorkItemContext);

typedef VOID (*NDIS_M_REQ_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status);

typedef VOID (*NDIS_M_RESET_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status,
    IN  BOOLEAN AddressingReset);

typedef VOID (*NDIS_M_SEND_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status);

typedef VOID (*NDIS_M_SEND_RESOURCES_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterHandle);

typedef BOOLEAN (FASTCALL *NDIS_M_START_SENDS)(
    IN  PNDIS_MINIPORT_BLOCK    Miniport);

typedef VOID (*NDIS_M_STATUS_HANDLER)(
    IN  NDIS_HANDLE MiniportHandle,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize);

typedef VOID (*NDIS_M_STS_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterHandle);

typedef VOID (*NDIS_M_TD_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred);

typedef VOID (*NDIS_WM_SEND_COMPLETE_HANDLER)(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PVOID       Packet,
    IN  NDIS_STATUS Status);


#ifdef NDIS_WRAPPER

#define ARC_SEND_BUFFERS    8
#define ARC_HEADER_SIZE     4

typedef struct _NDIS_ARC_BUF
{
    NDIS_HANDLE ArcnetBufferPool;
    PUCHAR      ArcnetLookaheadBuffer;
    UINT        NumFree;
    ARC_BUFFER_LIST ArcnetBuffers[ARC_SEND_BUFFERS];
} NDIS_ARC_BUF, *PNDIS_ARC_BUF;

#define NDIS_MINIPORT_WORK_QUEUE_SIZE 10

typedef struct _NDIS_LOG
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    KSPIN_LOCK              LogLock;
    PIRP                    Irp;
    UINT                    TotalSize;
    UINT                    CurrentSize;
    UINT                    InPtr;
    UINT                    OutPtr;
    UCHAR                   LogBuf[1];
} NDIS_LOG, *PNDIS_LOG;

#endif /* NDIS_WRAPPER */


struct _NDIS_ADAPTER_BLOCK
{
    PDEVICE_OBJECT          DeviceObject;
    PNDIS_MAC_BLOCK         MacHandle;
    NDIS_HANDLE             MacAdapterContext;
    NDIS_STRING             AdapterName;
    PNDIS_OPEN_BLOCK        OpenQueue;
    PNDIS_ADAPTER_BLOCK     NextAdapter;
    REFERENCE               Ref;
    PVOID                   BusDataContext;
    BOOLEAN                 BeingRemoved;
    UCHAR                   Flags;
    PCM_RESOURCE_LIST       Resources;
    PNDIS_STRING            pAdapterInstanceName;
    PVOID                   WrapperContext;

    ULONG                   BusNumber;
    NDIS_INTERFACE_TYPE     BusType;
    ULONG                   ChannelNumber;
    NDIS_INTERFACE_TYPE     AdapterType;
    BOOLEAN                 Master;
    UCHAR                   AssignedProcessor;
    ULONG                   PhysicalMapRegistersNeeded;
    ULONG                   MaximumPhysicalMapping;
    ULONG                   InitialPort;
    ULONG                   NumberOfPorts;
    PUCHAR                  InitialPortMapping;
    BOOLEAN                 InitialPortMapped;
    PUCHAR                  PortOffset;
    PMAP_REGISTER_ENTRY     MapRegisters;

    KEVENT                  AllocationEvent;
    UINT                    CurrentMapRegister;
    PADAPTER_OBJECT         SystemAdapterObject;
#if 0
#ifdef NDIS_WRAPPER
    ULONG                   BusId;
    ULONG                   SlotNumber;
    NDIS_STRING             BaseName;
    PDEVICE_OBJECT          PhysicalDeviceObject;
    PDEVICE_OBJECT          NextDeviceObject;
    PCM_RESOURCE_LIST       AllocatedResources;
    PCM_RESOURCE_LIST       AllocatedResourcesTranslated;
    NDIS_EVENT              OpenReadyEvent;
    NDIS_PNP_DEVICE_STATE   PnPDeviceState;
    PGET_SET_DEVICE_DATA    SetBusData;
    PGET_SET_DEVICE_DATA    GetBusData;
    POID_LIST               OidList;
    ULONG                   PnPCapabilities;
#endif /* NDIS_WRAPPER */
#endif
};


struct _NDIS_MINIPORT_BLOCK
{
    ULONG                       NullValue;
    PNDIS_MINIPORT_BLOCK        NextMiniport;
    PNDIS_M_DRIVER_BLOCK        DriverHandle;
    NDIS_HANDLE	                MiniportAdapterContext;
    UNICODE_STRING              MiniportName;
    PNDIS_BIND_PATHS            BindPaths;
    NDIS_HANDLE	                OpenQueue;
    REFERENCE                   Ref;
    NDIS_HANDLE	                DeviceContext;
    UCHAR                       Padding1;

    UCHAR                       LockAcquired;
    UCHAR                       PmodeOpens;
    UCHAR                       AssignedProcessor;
    KSPIN_LOCK                  Lock;
    PNDIS_REQUEST               MediaRequest;
    PNDIS_MINIPORT_INTERRUPT    Interrupt;
    ULONG                       Flags;
    ULONG                       PnPFlags;

    LIST_ENTRY	                PacketList;
    PNDIS_PACKET                FirstPendingPacket;
    PNDIS_PACKET                ReturnPacketsQueue;
    ULONG                       RequestBuffer;
    PVOID                       SetMCastBuffer;
    PNDIS_MINIPORT_BLOCK        PrimaryMiniport;
    PVOID                       WrapperContext;

    PVOID                       BusDataContext;
    ULONG                       PnPCapabilities;
    PCM_RESOURCE_LIST           Resources;
    NDIS_TIMER                  WakeUpDpcTimer;
    UNICODE_STRING              BaseName;
    UNICODE_STRING              SymbolicLinkName;

    ULONG                       CheckForHangSeconds;
    USHORT                      CFHangTicks;
    USHORT                      CFHangCurrentTick;
    NDIS_STATUS	                ResetStatus;
    NDIS_HANDLE	                ResetOpen;
    FILTERDBS                   FilterDbs;

    FILTER_PACKET_INDICATION_HANDLER    PacketIndicateHandler;
    NDIS_M_SEND_COMPLETE_HANDLER        SendCompleteHandler;
    NDIS_M_SEND_RESOURCES_HANDLER       SendResourcesHandler;
    NDIS_M_RESET_COMPLETE_HANDLER       ResetCompleteHandler;

    NDIS_MEDIUM	                MediaType;
    ULONG                       BusNumber;
    NDIS_INTERFACE_TYPE         BusType;
    NDIS_INTERFACE_TYPE	        AdapterType;
    PDEVICE_OBJECT              DeviceObject;
    PDEVICE_OBJECT              PhysicalDeviceObject;
    PDEVICE_OBJECT              NextDeviceObject;

    PMAP_REGISTER_ENTRY         MapRegisters;
    PNDIS_AF_LIST               CallMgrAfList;
    PVOID                       MiniportThread;
    PVOID                       SetInfoBuf;
    USHORT                      SetInfoBufLen;
    USHORT                      MaxSendPackets;
    NDIS_STATUS	                FakeStatus;
    PVOID                       LockHandler;
    PUNICODE_STRING	            pAdapterInstanceName;
    PADAPTER_OBJECT             SystemAdapterObject;
    UINT                        MacOptions;
    PNDIS_REQUEST               PendingRequest;
    UINT                        MaximumLongAddresses;
    UINT                        MaximumShortAddresses;
    UINT                        CurrentLookahead;
    UINT                        MaximumLookahead;

    W_HANDLE_INTERRUPT_HANDLER  HandleInterruptHandler;
    W_DISABLE_INTERRUPT_HANDLER DisableInterruptHandler;
    W_ENABLE_INTERRUPT_HANDLER  EnableInterruptHandler;
    W_SEND_PACKETS_HANDLER      SendPacketsHandler;
    NDIS_M_START_SENDS          DeferredSendHandler;

    ETH_RCV_INDICATE_HANDLER    EthRxIndicateHandler;
    TR_RCV_INDICATE_HANDLER	    TrRxIndicateHandler;
    FDDI_RCV_INDICATE_HANDLER   FddiRxIndicateHandler;

    ETH_RCV_COMPLETE_HANDLER    EthRxCompleteHandler;
    TR_RCV_COMPLETE_HANDLER	    TrRxCompleteHandler;
    FDDI_RCV_COMPLETE_HANDLER   FddiRxCompleteHandler;

    NDIS_M_STATUS_HANDLER       StatusHandler;
    NDIS_M_STS_COMPLETE_HANDLER StatusCompleteHandler;
    NDIS_M_TD_COMPLETE_HANDLER  TDCompleteHandler;
    NDIS_M_REQ_COMPLETE_HANDLER QueryCompleteHandler;
    NDIS_M_REQ_COMPLETE_HANDLER SetCompleteHandler;

    NDIS_WM_SEND_COMPLETE_HANDLER   WanSendCompleteHandler;
    WAN_RCV_HANDLER                 WanRcvHandler;
    WAN_RCV_COMPLETE_HANDLER        WanRcvCompleteHandler;
#if 0
#ifdef NDIS_WRAPPER
    SINGLE_LIST_ENTRY           WorkQueue[NUMBER_OF_WORK_ITEM_TYPES];
    SINGLE_LIST_ENTRY           SingleWorkItems[NUMBER_OF_SINGLE_WORK_ITEMS];
    PNDIS_MAC_BLOCK	            FakeMac;
    UCHAR                       SendFlags;
    UCHAR                       TrResetRing;
    UCHAR                       ArcnetAddress;

    union
    {
        PNDIS_ARC_BUF           ArcBuf;
        PVOID                   BusInterface;
    } u1;

    ULONG                       ChannelNumber;
    PNDIS_LOG                   Log;
    ULONG                       BusId;
    ULONG                       SlotNumber;
    PCM_RESOURCE_LIST           AllocatedResources;
    PCM_RESOURCE_LIST           AllocatedResourcesTranslated;
    SINGLE_LIST_ENTRY           PatternList;
    NDIS_PNP_CAPABILITIES       PMCapabilities;
#if 0
    DEVICE_CAPABILITIES	        DeviceCaps;
#endif
    ULONG                       WakeUpEnable;
#if 0
    DEVICE_POWER_STATE          CurrentDeviceState;
#endif
    PIRP                        pIrpWaitWake;
#if 0
    SYSTEM_POWER_STATE          WaitWakeSystemState;
#endif
    LARGE_INTEGER               VcIndex;
    KSPIN_LOCK	                VcCountLock;
    LIST_ENTRY                  WmiEnabledVcs;
    PNDIS_GUID                  pNdisGuidMap;
    PNDIS_GUID                  pCustomGuidMap;
    USHORT                      VcCount;
    USHORT                      cNdisGuidMap;
    USHORT                      cCustomGuidMap;
    USHORT                      CurrentMapRegister;
    PKEVENT                     AllocationEvent;
    USHORT                      PhysicalMapRegistersNeeded;
    USHORT                      SGMapRegistersNeeded;
    ULONG                       MaximumPhysicalMapping;

    NDIS_TIMER                  MediaDisconnectTimer;
    USHORT                      MediaDisconnectTimeOut;
    USHORT                      InstanceNumber;
    NDIS_EVENT                  OpenReadyEvent;
    NDIS_PNP_DEVICE_STATE       PnPDeviceState;
    NDIS_PNP_DEVICE_STATE       OldPnPDeviceState;
#if 0
    PGET_SET_DEVICE_DATA        SetBusData;
    PGET_SET_DEVICE_DATA        GetBusData;
#endif
    POID_LIST                   OidList;
    KDPC                        DeferredDpc;
#if 0
    NDIS_STATS                  NdisStats;
#endif
    PNDIS_PACKET                IndicatedPacket[MAXIMUM_PROCESSORS];
    PKEVENT	                    RemoveReadyEvent;
    PKEVENT	                    AllOpensClosedEvent;
    PKEVENT                     AllRequestsCompletedEvent;
    ULONG                       InitTimeMs;
    NDIS_MINIPORT_WORK_ITEM     WorkItemBuffer[NUMBER_OF_SINGLE_WORK_ITEMS];
    PNDIS_MINIPORT_TIMER        TimerQueue;
	ULONG                       DriverVerifyFlags;

    PNDIS_MINIPORT_BLOCK        NextGlobalMiniport;
	USHORT                      InternalResetCount;
    USHORT                      MiniportResetCount;
    USHORT                      MediaSenseConnectCount;
    USHORT                      MediaSenseDisconnectCount;
    PNDIS_PACKET                *xPackets;
    ULONG                       UserModeOpenReferences;
#endif /* NDIS_WRAPPER */
#endif
};


/* Handler prototypes for NDIS_OPEN_BLOCK */

typedef NDIS_STATUS (*WAN_SEND_HANDLER)(
    IN  NDIS_HANDLE MacBindingHandle,
    IN  NDIS_HANDLE LinkHandle,
    IN  PVOID       Packet);

/* NDIS 4.0 extension */

typedef VOID (*SEND_PACKETS_HANDLER)(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);


struct _NDIS_OPEN_BLOCK
{
    PNDIS_MAC_BLOCK             MacHandle;
    NDIS_HANDLE                 MacBindingHandle;
    PNDIS_ADAPTER_BLOCK         AdapterHandle;
    PNDIS_PROTOCOL_BLOCK        ProtocolHandle;
    NDIS_HANDLE	                ProtocolBindingContext;
    PNDIS_OPEN_BLOCK            AdapterNextOpen;
    PNDIS_OPEN_BLOCK            ProtocolNextOpen;
    PNDIS_OPEN_BLOCK            NextGlobalOpen;
    BOOLEAN                     Closing;
    BOOLEAN	                    Unbinding;
    BOOLEAN                     NoProtRsvdOnRcvPkt;
    BOOLEAN                     ProcessingOpens;
    PNDIS_STRING                BindDeviceName;
    KSPIN_LOCK                  SpinLock;
    PNDIS_STRING                RootDeviceName;

    union
    {
        SEND_HANDLER            SendHandler;
        WAN_SEND_HANDLER        WanSendHandler;
    } u1;
    TRANSFER_DATA_HANDLER       TransferDataHandler;

    SEND_COMPLETE_HANDLER       SendCompleteHandler;
    TRANSFER_DATA_COMPLETE_HANDLER  TransferDataCompleteHandler;
    RECEIVE_HANDLER             ReceiveHandler;
    RECEIVE_COMPLETE_HANDLER    ReceiveCompleteHandler;

    union
    {
        RECEIVE_HANDLER	        PostNt31ReceiveHandler;
        WAN_RECEIVE_HANDLER     WanReceiveHandler;
    } u2;
    RECEIVE_COMPLETE_HANDLER    PostNt31ReceiveCompleteHandler;

    RECEIVE_PACKET_HANDLER      ReceivePacketHandler;
    SEND_PACKETS_HANDLER        SendPacketsHandler;

    RESET_HANDLER               ResetHandler;
    REQUEST_HANDLER	            RequestHandler;
    RESET_COMPLETE_HANDLER      ResetCompleteHandler;
    STATUS_HANDLER              StatusHandler;
    STATUS_COMPLETE_HANDLER     StatusCompleteHandler;
    REQUEST_COMPLETE_HANDLER    RequestCompleteHandler;
};



/* Routines for NDIS miniport drivers */

VOID
EXPIMP
NdisInitializeWrapper(
    OUT PNDIS_HANDLE    NdisWrapperHandle,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2,
    IN  PVOID           SystemSpecific3);

NDIS_STATUS
EXPIMP
NdisMAllocateMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        DmaChannel,
    IN  BOOLEAN     Dma32BitAddresses,
    IN  ULONG       PhysicalMapRegistersNeeded,
    IN  ULONG       MaximumPhysicalMapping);

/*
 * VOID NdisMArcIndicateReceive(
 *     IN  NDIS_HANDLE MiniportAdapterHandle,
 *     IN  PUCHAR      HeaderBuffer,
 *     IN  PUCHAR      DataBuffer,
 *     IN  UINT        Length);
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
 * VOID NdisMArcIndicateReceiveComplete(
 *     IN  NDIS_HANDLE MiniportAdapterHandle);
 */
#define NdisMArcIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                              \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->ArcRxCompleteHandler)( \
        ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.ArcDB);      \
}

VOID
EXPIMP
NdisMCloseLog(
    IN  NDIS_HANDLE LogHandle);

NDIS_STATUS
EXPIMP
NdisMCreateLog(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  UINT            Size,
    OUT PNDIS_HANDLE    LogHandle);

VOID
EXPIMP
NdisMDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE MiniportHandle);

VOID
EXPIMP
NdisMDeregisterInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt);

VOID
EXPIMP
NdisMDeregisterIoPortRange(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts,
    IN  PVOID       PortOffset);

/*
 * VOID NdisMEthIndicateReceive(
 *     IN  NDIS_HANDLE MiniportAdapterHandle,
 *     IN  NDIS_HANDLE MiniportReceiveContext,
 *     IN  PVOID       HeaderBuffer,
 *     IN  UINT        HeaderBufferSize,
 *     IN  PVOID       LookaheadBuffer,
 *     IN  UINT        LookaheadBufferSize,
 *     IN  UINT        PacketSize);
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
        (((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FilterDbs.u.EthDB), \
		(MiniportReceiveContext), \
		(HeaderBuffer),           \
		(HeaderBuffer),           \
		(HeaderBufferSize),       \
		(LookaheadBuffer),        \
		(LookaheadBufferSize),    \
		(PacketSize));            \
}

/*
 * VOID NdisMEthIndicateReceiveComplete(
 *     IN  NDIS_HANDLE MiniportAdapterHandle);
 */
#define NdisMEthIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                              \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->EthRxCompleteHandler)( \
        ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.u.EthDB);    \
}

/*
 * VOID NdisMFddiIndicateReceive(
 *     IN  NDIS_HANDLE MiniportAdapterHandle,
 *     IN  NDIS_HANDLE MiniportReceiveContext,
 *     IN  PVOID       HeaderBuffer,
 *     IN  UINT        HeaderBufferSize,
 *     IN  PVOID       LookaheadBuffer,
 *     IN  UINT        LookaheadBufferSize,
 *     IN  UINT        PacketSize);
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
 * VOID NdisMFddiIndicateReceiveComplete(
 *     IN  NDIS_HANDLE MiniportAdapterHandle);
 */
#define NdisMFddiIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                               \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FddiRxCompleteHandler)( \
        ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.FddiDB);      \
}

VOID
EXPIMP
NdisMFlushLog(
    IN  NDIS_HANDLE LogHandle);

VOID
EXPIMP
NdisMFreeMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle);

VOID
EXPIMP
NdisMIndicateStatus(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize);

VOID
EXPIMP
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle);

/*
 * VOID NdisMInitializeWrapper(
 *     OUT PNDIS_HANDLE    NdisWrapperHandle,
 *     IN  PVOID           SystemSpecific1,
 *     IN  PVOID           SystemSpecific2,
 *     IN  PVOID           SystemSpecific3);
 */
#define NdisMInitializeWrapper(NdisWrapperHandle, \
                               SystemSpecific1,   \
                               SystemSpecific2,   \
                               SystemSpecific3)   \
    NdisInitializeWrapper((NdisWrapperHandle),    \
                          (SystemSpecific1),      \
                          (SystemSpecific2),      \
                          (SystemSpecific3))

NDIS_STATUS
EXPIMP
NdisMMapIoSpace(
    OUT PVOID *                 VirtualAddress,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length);

VOID
EXPIMP
NdisMQueryInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status);

VOID
EXPIMP
NdisMRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 MiniportHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler);

NDIS_STATUS
EXPIMP
NdisMRegisterInterrupt(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  UINT                        InterruptVector,
    IN  UINT                        InterruptLevel,
    IN  BOOLEAN	                    RequestIsr,
    IN  BOOLEAN                     SharedInterrupt,
    IN  NDIS_INTERRUPT_MODE         InterruptMode);

NDIS_STATUS
EXPIMP
NdisMRegisterIoPortRange(
    OUT PVOID *     PortOffset,
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts);

NDIS_STATUS
EXPIMP
NdisMRegisterMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength);


#ifndef NDIS_WRAPPER

/*
 * VOID NdisMResetComplete(
 *     IN  NDIS_HANDLE MiniportAdapterHandle,
 *     IN  NDIS_STATUS Status,
 *     IN  BOOLEAN     AddressingReset);
 */
#define	NdisMResetComplete(MiniportAdapterHandle, \
                           Status,                \
                           AddressingReset)       \
{                                                 \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->ResetCompleteHandler)( \
        MiniportAdapterHandle, Status, AddressingReset); \
}

/*
 * VOID NdisMSendComplete(
 *     IN  NDIS_HANDLE     MiniportAdapterHandle,
 *     IN  PNDIS_PACKET    Packet,
 *     IN  NDIS_STATUS     Status);
 */
#define	NdisMSendComplete(MiniportAdapterHandle, \
                          Packet,                \
                          Status)                \
{                                                \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->SendCompleteHandler)( \
        MiniportAdapterHandle, Packet, Status);  \
}

/*
 * VOID NdisMSendResourcesAvailable(
 *     IN  NDIS_HANDLE MiniportAdapterHandle);
 */
#define	NdisMSendResourcesAvailable(MiniportAdapterHandle) \
{                                                \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->SendResourcesHandler)( \
        MiniportAdapterHandle); \
}

/*
 * VOID NdisMTransferDataComplete(
 *     IN  NDIS_HANDLE     MiniportAdapterHandle,
 *     IN  PNDIS_PACKET    Packet,
 *     IN  NDIS_STATUS     Status,
 *     IN  UINT            BytesTransferred);
 */
#define	NdisMTransferDataComplete(MiniportAdapterHandle, \
                                  Packet,                \
                                  Status,                \
                                  BytesTransferred)      \
{                                                        \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->TDCompleteHandler)( \
        MiniportAdapterHandle, Packet, Status, BytesTransferred)           \
}

#endif /* NDIS_WRAPPER */


VOID
EXPIMP
NdisMSetAttributes(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  BOOLEAN             BusMaster,
    IN  NDIS_INTERFACE_TYPE AdapterType);

VOID 
EXPIMP
NdisMSetAttributesEx(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  UINT                CheckForHangTimeInSeconds   OPTIONAL,
    IN  ULONG               AttributeFlags,
    IN  NDIS_INTERFACE_TYPE AdapterType); 

VOID
EXPIMP
NdisMSetInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status);

VOID
EXPIMP
NdisMSleep(
    IN  ULONG   MicrosecondsToSleep);

BOOLEAN
EXPIMP
NdisMSynchronizeWithInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  PVOID                       SynchronizeFunction,
    IN  PVOID                       SynchronizeContext);

/*
 * VOID NdisMTrIndicateReceive(
 *     IN  NDIS_HANDLE MiniportAdapterHandle,
 *     IN  NDIS_HANDLE MiniportReceiveContext,
 *     IN  PVOID       HeaderBuffer,
 *     IN  UINT        HeaderBufferSize,
 *     IN  PVOID       LookaheadBuffer,
 *     IN  UINT        LookaheadBufferSize,
 *     IN  UINT        PacketSize);
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
        (((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->FilterDbs.TrDB),   \
		(MiniportReceiveContext), \
		(HeaderBuffer),           \
		(HeaderBuffer),           \
		(HeaderBufferSize),       \
		(LookaheadBuffer),        \
		(LookaheadBufferSize),    \
		(PacketSize));            \
}

/*
 * VOID NdisMTrIndicateReceiveComplete(
 *     IN  NDIS_HANDLE  MiniportAdapterHandle);
 */
#define NdisMTrIndicateReceiveComplete(MiniportAdapterHandle) \
{                                                             \
    (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->TrRxCompleteHandler)( \
        ((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle)->FilterDbs.TrDB);      \
}

NDIS_STATUS
EXPIMP
NdisMWriteLogData(
    IN  NDIS_HANDLE LogHandle,
    IN  PVOID       LogBuffer,
    IN  UINT        LogBufferSize);

VOID
EXPIMP
NdisMQueryAdapterResources(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         WrapperConfigurationContext,
    OUT PNDIS_RESOURCE_LIST ResourceList,
    IN  OUT PUINT           BufferSize);

VOID
EXPIMP
NdisTerminateWrapper(
    IN  NDIS_HANDLE NdisWrapperHandle,
    IN  PVOID       SystemSpecific);

VOID
EXPIMP
NdisMUnmapIoSpace(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PVOID       VirtualAddress,
    IN  UINT        Length);



/* NDIS intermediate miniport structures */

typedef VOID (*W_MINIPORT_CALLBACK)(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  PVOID       CallbackContext);



/* Routines for intermediate miniport drivers */

NDIS_STATUS
EXPIMP
NdisIMDeInitializeDeviceInstance(
    IN  NDIS_HANDLE NdisMiniportHandle);

NDIS_STATUS
EXPIMP
NdisIMInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance);

NDIS_STATUS
EXPIMP
NdisIMQueueMiniportCallback(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  W_MINIPORT_CALLBACK CallbackRoutine,
    IN  PVOID               CallbackContext);

NDIS_STATUS
EXPIMP
NdisIMRegisterLayeredMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength,
    OUT PNDIS_HANDLE                    DriverHandle);

VOID
EXPIMP
NdisIMRevertBack(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE SwitchHandle);

BOOLEAN
EXPIMP
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    OUT PNDIS_HANDLE    SwitchHandle);


/* Functions obsoleted by NDIS 5.0 */

VOID
EXPIMP
NdisFreeDmaChannel(
    IN  PNDIS_HANDLE    NdisDmaHandle);

VOID
EXPIMP
NdisFreeSharedMemory(
    IN NDIS_HANDLE              NdisAdapterHandle,
    IN ULONG                    Length,
    IN BOOLEAN                  Cached,
    IN PVOID                    VirtualAddress,
    IN NDIS_PHYSICAL_ADDRESS    PhysicalAddress); 

NDIS_STATUS
EXPIMP
NdisIMQueueMiniportCallback(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  W_MINIPORT_CALLBACK CallbackRoutine,
    IN  PVOID               CallbackContext);

VOID
EXPIMP
NdisIMRevertBack(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE SwitchHandle);

BOOLEAN
EXPIMP
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    OUT PNDIS_HANDLE    SwitchHandle);

VOID
EXPIMP
NdisSetupDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    NdisDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice);

NTSTATUS
EXPIMP
NdisUpcaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,  
    IN  PUNICODE_STRING SourceString);

VOID
EXPIMP
NdisUpdateSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress);


/* Routines for NDIS protocol drivers */

#if BINARY_COMPATIBLE

VOID
EXPIMP
NdisRequest(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest);

VOID
EXPIMP
NdisReset(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle);

VOID
EXPIMP
NdisSend(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_PACKET    Packet);

VOID
EXPIMP
NdisSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);

VOID
EXPIMP
NdisTransferData(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         NdisBindingHandle,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  UINT                ByteOffset,
    IN  UINT                BytesToTransfer,
    IN  OUT PNDIS_PACKET    Packet,
    OUT PUINT               BytesTransferred);

#else /* BINARY_COMPATIBLE */

#define NdisRequest(Status,            \
                    NdisBindingHandle, \
                    NdisRequest)       \
{                                      \
    *(Status) = (((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->RequestHandler)(         \
        ((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle, (NdisRequest)); \
}

#define NdisReset(Status,            \
                  NdisBindingHandle) \
{                                    \
    *(Status) = (((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->ResetHandler)( \
        ((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle);      \
}

#define NdisSend(Status,            \
                 NdisBindingHandle, \
                 Packet)            \
{                                   \
    *(Status) = (((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->u1.SendHandler)(    \
        ((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle, (Packet)); \
}

#define NdisSendPackets(NdisBindingHandle, \
                        PacketArray,       \
                        NumberOfPackets)   \
{                                          \
    (((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->SendPacketsHandler)( \
        (PNDIS_OPEN_BLOCK)(NdisBindingHandle), (PacketArray), (NumberOfPackets)); \
}

#define NdisTransferData(Status,           \
                        NdisBindingHandle, \
                        MacReceiveContext, \
                        ByteOffset,        \
                        BytesToTransfer,   \
                        Packet,            \
                        BytesTransferred)  \
{                                          \
    *(Status) =	(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->TransferDataHandler)( \
        ((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle,              \
        (MacReceiveContext), \
        (ByteOffset),        \
        (BytesToTransfer),   \
        (Packet),            \
        (BytesTransferred)); \
}

#endif /* BINARY_COMPATIBLE */


VOID
EXPIMP
NdisCloseAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle);

VOID
EXPIMP
NdisCompleteBindAdapter(
    IN  NDIS_HANDLE BindAdapterContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenStatus);

VOID
EXPIMP
NdisCompleteUnbindAdapter(
    IN  NDIS_HANDLE UnbindAdapterContext,
    IN  NDIS_STATUS Status);

VOID
EXPIMP
NdisDeregisterProtocol(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisProtocolHandle);

VOID
EXPIMP
NdisOpenAdapter(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PNDIS_HANDLE    NdisBindingHandle,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     NdisProtocolHandle,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_STRING    AdapterName,
    IN  UINT            OpenOptions,
    IN  PSTRING         AddressingInformation);

VOID
EXPIMP
NdisOpenProtocolConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  PNDIS_STRING    ProtocolSection);

NDIS_STATUS
EXPIMP
NdisQueryReceiveInformation(
    IN  NDIS_HANDLE NdisBindingHandle,
    IN  NDIS_HANDLE MacContext,
    OUT PLONGLONG   TimeSent            OPTIONAL,
    OUT PLONGLONG   TimeReceived        OPTIONAL,
    IN  PUCHAR      Buffer,
    IN  UINT        BufferSize,
    OUT PUINT       SizeNeeded);

VOID
EXPIMP
NdisRegisterProtocol(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    NdisProtocolHandle,
    IN  PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN  UINT                            CharacteristicsLength);

VOID
EXPIMP
NdisReturnPackets(
    IN  PNDIS_PACKET    *PacketsToReturn,
    IN  UINT            NumberOfPackets);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __NDIS_H */

/* EOF */
