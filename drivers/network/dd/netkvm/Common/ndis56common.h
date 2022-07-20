/*
 * This file contains general definitions for VirtIO network adapter driver,
 * common for both NDIS5 and NDIS6
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef PARANDIS_56_COMMON_H
#define PARANDIS_56_COMMON_H

//#define PARANDIS_TEST_TX_KICK_ALWAYS

#if defined(OFFLOAD_UNIT_TEST)
#include <windows.h>
#include <stdio.h>

#define ETH_LENGTH_OF_ADDRESS       6
#define DoPrint(fmt, ...) printf(fmt##"\n", __VA_ARGS__)
#define DPrintf(a,b) DoPrint b
#define RtlOffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG_PTR)(O))  ))

#include "ethernetutils.h"
#endif //+OFFLOAD_UNIT_TEST

#if !defined(OFFLOAD_UNIT_TEST)

#if !defined(RtlOffsetToPointer)
#define RtlOffsetToPointer(Base,Offset)  ((PCHAR)(((PCHAR)(Base))+((ULONG_PTR)(Offset))))
#endif

#if !defined(RtlPointerToOffset)
#define RtlPointerToOffset(Base,Pointer)  ((ULONG)(((PCHAR)(Pointer))-((PCHAR)(Base))))
#endif


#include <ndis.h>
#include "osdep.h"
#include "kdebugprint.h"
#include "ethernetutils.h"
#include "virtio_pci.h"
#include "VirtIO.h"
#include "virtio_ring.h"
#include "IONetDescriptor.h"
#include "DebugData.h"

// those stuff defined in NDIS
//NDIS_MINIPORT_MAJOR_VERSION
//NDIS_MINIPORT_MINOR_VERSION
// those stuff defined in build environment
// PARANDIS_MAJOR_DRIVER_VERSION
// PARANDIS_MINOR_DRIVER_VERSION

#if !defined(NDIS_MINIPORT_MAJOR_VERSION) || !defined(NDIS_MINIPORT_MINOR_VERSION)
#error "Something is wrong with NDIS environment"
#endif

//define to see when the status register is unreadable(see ParaNdis_ResetVirtIONetDevice)
//#define VIRTIO_RESET_VERIFY

//define to if hardware raise interrupt on error (see ParaNdis_DPCWorkBody)
//#define VIRTIO_SIGNAL_ERROR

// define if qemu supports logging to static IO port for synchronization
// of driver output with qemu printouts; in this case define the port number
// #define VIRTIO_DBG_USE_IOPORT    0x99

// to be set to real limit later
#define MAX_RX_LOOPS    1000

// maximum number of virtio queues used by the driver
#define MAX_NUM_OF_QUEUES 3

/* The feature bitmap for virtio net */
#define VIRTIO_NET_F_CSUM   0   /* Host handles pkts w/ partial csum */
#define VIRTIO_NET_F_GUEST_CSUM 1   /* Guest handles pkts w/ partial csum */
#define VIRTIO_NET_F_MAC    5   /* Host has given MAC address. */
#define VIRTIO_NET_F_GSO    6   /* Host handles pkts w/ any GSO type */
#define VIRTIO_NET_F_GUEST_TSO4 7   /* Guest can handle TSOv4 in. */
#define VIRTIO_NET_F_GUEST_TSO6 8   /* Guest can handle TSOv6 in. */
#define VIRTIO_NET_F_GUEST_ECN  9   /* Guest can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_GUEST_UFO  10  /* Guest can handle UFO in. */
#define VIRTIO_NET_F_HOST_TSO4  11  /* Host can handle TSOv4 in. */
#define VIRTIO_NET_F_HOST_TSO6  12  /* Host can handle TSOv6 in. */
#define VIRTIO_NET_F_HOST_ECN   13  /* Host can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_HOST_UFO   14  /* Host can handle UFO in. */
#define VIRTIO_NET_F_MRG_RXBUF  15  /* Host can handle merged Rx buffers and requires bigger header for that. */
#define VIRTIO_NET_F_STATUS     16
#define VIRTIO_NET_F_CTRL_VQ    17      /* Control channel available */
#define VIRTIO_NET_F_CTRL_RX    18      /* Control channel RX mode support */
#define VIRTIO_NET_F_CTRL_VLAN  19      /* Control channel VLAN filtering */
#define VIRTIO_NET_F_CTRL_RX_EXTRA 20   /* Extra RX mode control support */

#define VIRTIO_NET_S_LINK_UP    1       /* Link is up */

#define VIRTIO_NET_INVALID_INTERRUPT_STATUS     0xFF

#define PARANDIS_MULTICAST_LIST_SIZE        32
#define PARANDIS_MEMORY_TAG                 '5muQ'
#define PARANDIS_FORMAL_LINK_SPEED          (pContext->ulFormalLinkSpeed)
#define PARANDIS_MAXIMUM_TRANSMIT_SPEED     PARANDIS_FORMAL_LINK_SPEED
#define PARANDIS_MAXIMUM_RECEIVE_SPEED      PARANDIS_FORMAL_LINK_SPEED
#define PARANDIS_MIN_LSO_SEGMENTS           2
// reported
#define PARANDIS_MAX_LSO_SIZE               0xF800

#define PARANDIS_UNLIMITED_PACKETS_TO_INDICATE  (~0ul)

extern VirtIOSystemOps ParaNdisSystemOps;

typedef enum _tagInterruptSource
{
    isControl  = VIRTIO_PCI_ISR_CONFIG,
    isReceive  = 0x10,
    isTransmit = 0x20,
    isUnknown  = 0x40,
    isBothTransmitReceive = isReceive | isTransmit,
    isAny      = isReceive | isTransmit | isControl | isUnknown,
    isDisable  = 0x80
}tInterruptSource;

static const ULONG PARANDIS_PACKET_FILTERS =
    NDIS_PACKET_TYPE_DIRECTED |
    NDIS_PACKET_TYPE_MULTICAST |
    NDIS_PACKET_TYPE_BROADCAST |
    NDIS_PACKET_TYPE_PROMISCUOUS |
    NDIS_PACKET_TYPE_ALL_MULTICAST;

typedef VOID (*ONPAUSECOMPLETEPROC)(VOID *);


typedef enum _tagSendReceiveState
{
    srsDisabled = 0,        // initial state
    srsPausing,
    srsEnabled
} tSendReceiveState;

typedef struct _tagBusResource {
    NDIS_PHYSICAL_ADDRESS BasePA;
    ULONG                 uLength;
    PVOID                 pBase;
    BOOLEAN               bPortSpace;
    BOOLEAN               bUsed;
} tBusResource;

typedef struct _tagAdapterResources
{
    tBusResource PciBars[PCI_TYPE0_ADDRESSES];
    ULONG Vector;
    ULONG Level;
    KAFFINITY Affinity;
    ULONG InterruptFlags;
} tAdapterResources;

typedef enum _tagOffloadSettingsBit
{
    osbT4IpChecksum = (1 << 0),
    osbT4TcpChecksum = (1 << 1),
    osbT4UdpChecksum = (1 << 2),
    osbT4TcpOptionsChecksum = (1 << 3),
    osbT4IpOptionsChecksum = (1 << 4),
    osbT4Lso = (1 << 5),
    osbT4LsoIp = (1 << 6),
    osbT4LsoTcp = (1 << 7),
    osbT4RxTCPChecksum = (1 << 8),
    osbT4RxTCPOptionsChecksum = (1 << 9),
    osbT4RxIPChecksum = (1 << 10),
    osbT4RxIPOptionsChecksum = (1 << 11),
    osbT4RxUDPChecksum = (1 << 12),
    osbT6TcpChecksum = (1 << 13),
    osbT6UdpChecksum = (1 << 14),
    osbT6TcpOptionsChecksum = (1 << 15),
    osbT6IpExtChecksum = (1 << 16),
    osbT6Lso = (1 << 17),
    osbT6LsoIpExt = (1 << 18),
    osbT6LsoTcpOptions = (1 << 19),
    osbT6RxTCPChecksum = (1 << 20),
    osbT6RxTCPOptionsChecksum = (1 << 21),
    osbT6RxUDPChecksum = (1 << 22),
    osbT6RxIpExtChecksum = (1 << 23),
}tOffloadSettingsBit;

typedef struct _tagOffloadSettingsFlags
{
    ULONG fTxIPChecksum     : 1;
    ULONG fTxTCPChecksum    : 1;
    ULONG fTxUDPChecksum    : 1;
    ULONG fTxTCPOptions     : 1;
    ULONG fTxIPOptions      : 1;
    ULONG fTxLso            : 1;
    ULONG fTxLsoIP          : 1;
    ULONG fTxLsoTCP         : 1;
    ULONG fRxIPChecksum     : 1;
    ULONG fRxTCPChecksum    : 1;
    ULONG fRxUDPChecksum    : 1;
    ULONG fRxTCPOptions     : 1;
    ULONG fRxIPOptions      : 1;
    ULONG fTxTCPv6Checksum  : 1;
    ULONG fTxUDPv6Checksum  : 1;
    ULONG fTxTCPv6Options   : 1;
    ULONG fTxIPv6Ext        : 1;
    ULONG fTxLsov6          : 1;
    ULONG fTxLsov6IP        : 1;
    ULONG fTxLsov6TCP       : 1;
    ULONG fRxTCPv6Checksum  : 1;
    ULONG fRxUDPv6Checksum  : 1;
    ULONG fRxTCPv6Options   : 1;
    ULONG fRxIPv6Ext        : 1;
}tOffloadSettingsFlags;


typedef struct _tagOffloadSettings
{
    /* current value of enabled offload features */
    tOffloadSettingsFlags flags;
    /* load once, do not modify - bitmask of offload features, enabled in configuration */
    ULONG flagsValue;
    ULONG ipHeaderOffset;
    ULONG maxPacketSize;
}tOffloadSettings;

typedef struct _tagChecksumCheckResult
{
    union
    {
        struct
        {
            ULONG   TcpFailed       :1;
            ULONG   UdpFailed       :1;
            ULONG   IpFailed        :1;
            ULONG   TcpOK           :1;
            ULONG   UdpOK           :1;
            ULONG   IpOK            :1;
        } flags;
        ULONG value;
    };
}tChecksumCheckResult;

/*
for simplicity, we use for NDIS5 the same statistics as native NDIS6 uses
*/
typedef struct _tagNdisStatistics
{
    ULONG64                     ifHCInOctets;
    ULONG64                     ifHCInUcastPkts;
    ULONG64                     ifHCInUcastOctets;
    ULONG64                     ifHCInMulticastPkts;
    ULONG64                     ifHCInMulticastOctets;
    ULONG64                     ifHCInBroadcastPkts;
    ULONG64                     ifHCInBroadcastOctets;
    ULONG64                     ifInDiscards;
    ULONG64                     ifInErrors;
    ULONG64                     ifHCOutOctets;
    ULONG64                     ifHCOutUcastPkts;
    ULONG64                     ifHCOutUcastOctets;
    ULONG64                     ifHCOutMulticastPkts;
    ULONG64                     ifHCOutMulticastOctets;
    ULONG64                     ifHCOutBroadcastPkts;
    ULONG64                     ifHCOutBroadcastOctets;
    ULONG64                     ifOutDiscards;
    ULONG64                     ifOutErrors;
}NDIS_STATISTICS_INFO;

typedef PNDIS_PACKET tPacketType;
typedef PNDIS_PACKET tPacketHolderType;
typedef PNDIS_PACKET tPacketIndicationType;

typedef struct _tagNdisOffloadParams
{
    UCHAR   IPv4Checksum;
    UCHAR   TCPIPv4Checksum;
    UCHAR   UDPIPv4Checksum;
    UCHAR   LsoV1;
    UCHAR   LsoV2IPv4;
    UCHAR   TCPIPv6Checksum;
    UCHAR   UDPIPv6Checksum;
    UCHAR   LsoV2IPv6;
}NDIS_OFFLOAD_PARAMETERS;

//#define UNIFY_LOCKS

typedef struct _tagOurCounters
{
    UINT nReusedRxBuffers;
    UINT nPrintDiagnostic;
    ULONG64 prevIn;
    UINT nRxInactivity;
}tOurCounters;

typedef struct _tagMaxPacketSize
{
    UINT nMaxDataSize;
    UINT nMaxFullSizeOS;
    UINT nMaxFullSizeHwTx;
    UINT nMaxFullSizeHwRx;
}tMaxPacketSize;

typedef struct _tagCompletePhysicalAddress
{
    PHYSICAL_ADDRESS    Physical;
    PVOID               Virtual;
    ULONG               size;
    ULONG               IsCached        : 1;
    ULONG               IsTX            : 1;
} tCompletePhysicalAddress;

typedef struct _tagMulticastData
{
    ULONG                   nofMulticastEntries;
    UCHAR                   MulticastList[ETH_LENGTH_OF_ADDRESS * PARANDIS_MULTICAST_LIST_SIZE];
}tMulticastData;

typedef struct _tagIONetDescriptor {
    LIST_ENTRY listEntry;
    tCompletePhysicalAddress HeaderInfo;
    tCompletePhysicalAddress DataInfo;
    tPacketHolderType pHolder;
    PVOID ReferenceValue;
    UINT  nofUsedBuffers;
} IONetDescriptor, * pIONetDescriptor;

typedef void (*tReuseReceiveBufferProc)(void *pContext, pIONetDescriptor pDescriptor);

typedef struct _tagPARANDIS_ADAPTER
{
    NDIS_HANDLE             DriverHandle;
    NDIS_HANDLE             MiniportHandle;
    NDIS_EVENT              ResetEvent;
    tAdapterResources       AdapterResources;
    tBusResource            SharedMemoryRanges[MAX_NUM_OF_QUEUES];

    VirtIODevice            IODevice;
    BOOLEAN                 bIODeviceInitialized;
    ULONGLONG               ullHostFeatures;
    ULONGLONG               ullGuestFeatures;

    LARGE_INTEGER           LastTxCompletionTimeStamp;
#ifdef PARANDIS_DEBUG_INTERRUPTS
    LARGE_INTEGER           LastInterruptTimeStamp;
#endif
    BOOLEAN                 bConnected;
    BOOLEAN                 bEnableInterruptHandlingDPC;
    BOOLEAN                 bEnableInterruptChecking;
    BOOLEAN                 bDoInterruptRecovery;
    BOOLEAN                 bDoSupportPriority;
    BOOLEAN                 bDoHwPacketFiltering;
    BOOLEAN                 bUseScatterGather;
    BOOLEAN                 bBatchReceive;
    BOOLEAN                 bLinkDetectSupported;
    BOOLEAN                 bDoHardwareChecksum;
    BOOLEAN                 bDoGuestChecksumOnReceive;
    BOOLEAN                 bDoIPCheckTx;
    BOOLEAN                 bDoIPCheckRx;
    BOOLEAN                 bUseMergedBuffers;
    BOOLEAN                 bDoKickOnNoBuffer;
    BOOLEAN                 bSurprizeRemoved;
    BOOLEAN                 bUsingMSIX;
    BOOLEAN                 bUseIndirect;
    BOOLEAN                 bHasHardwareFilters;
    BOOLEAN                 bHasControlQueue;
    BOOLEAN                 bNoPauseOnSuspend;
    BOOLEAN                 bFastSuspendInProcess;
    BOOLEAN                 bResetInProgress;
    ULONG                   ulCurrentVlansFilterSet;
    tMulticastData          MulticastData;
    UINT                    uNumberOfHandledRXPacketsInDPC;
    NDIS_DEVICE_POWER_STATE powerState;
    LONG                    dpcReceiveActive;
    LONG                    counterDPCInside;
    LONG                    bDPCInactive;
    LONG                    InterruptStatus;
    ULONG                   ulPriorityVlanSetting;
    ULONG                   VlanId;
    ULONGLONG               ulFormalLinkSpeed;
    ULONG                   ulEnableWakeup;
    tMaxPacketSize          MaxPacketSize;
    ULONG                   nEnableDPCChecker;
    ULONG                   ulUniqueID;
    UCHAR                   PermanentMacAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                   CurrentMacAddress[ETH_LENGTH_OF_ADDRESS];
    ULONG                   PacketFilter;
    ULONG                   DummyLookAhead;
    ULONG                   ulMilliesToConnect;
    ULONG                   nDetectedStoppedTx;
    ULONG                   nDetectedInactivity;
    ULONG                   nVirtioHeaderSize;
    /* send part */
#if !defined(UNIFY_LOCKS)
    NDIS_SPIN_LOCK          SendLock;
    NDIS_SPIN_LOCK          ReceiveLock;
#else
    union
    {
    NDIS_SPIN_LOCK          SendLock;
    NDIS_SPIN_LOCK          ReceiveLock;
    };
#endif
    NDIS_STATISTICS_INFO    Statistics;
    struct
    {
        ULONG framesCSOffload;
        ULONG framesLSO;
        ULONG framesIndirect;
        ULONG framesRxPriority;
        ULONG framesRxCSHwOK;
        ULONG framesRxCSHwMissedBad;
        ULONG framesRxCSHwMissedGood;
        ULONG framesFilteredOut;
    } extraStatistics;
    tOurCounters            Counters;
    tOurCounters            Limits;
    tSendReceiveState       SendState;
    tSendReceiveState       ReceiveState;
    ONPAUSECOMPLETEPROC     SendPauseCompletionProc;
    ONPAUSECOMPLETEPROC     ReceivePauseCompletionProc;
    tReuseReceiveBufferProc ReuseBufferProc;
    /* Net part - management of buffers and queues of QEMU */
    struct virtqueue *      NetControlQueue;
    tCompletePhysicalAddress ControlData;
    struct virtqueue *      NetReceiveQueue;
    struct virtqueue *      NetSendQueue;
    /* list of Rx buffers available for data (under VIRTIO management) */
    LIST_ENTRY              NetReceiveBuffers;
    UINT                    NetNofReceiveBuffers;
    /* list of Rx buffers waiting for return (under NDIS management) */
    LIST_ENTRY              NetReceiveBuffersWaiting;
    /* list of Tx buffers in process (under VIRTIO management) */
    LIST_ENTRY              NetSendBuffersInUse;
    /* list of Tx buffers ready for data (under MINIPORT management) */
    LIST_ENTRY              NetFreeSendBuffers;
    /* current number of free Tx descriptors */
    UINT                    nofFreeTxDescriptors;
    /* initial number of free Tx descriptor(from cfg) - max number of available Tx descriptors */
    UINT                    maxFreeTxDescriptors;
    /* current number of free Tx buffers, which can be submitted */
    UINT                    nofFreeHardwareBuffers;
    /* maximal number of free Tx buffers, which can be used by SG */
    UINT                    maxFreeHardwareBuffers;
    /* minimal number of free Tx buffers */
    UINT                    minFreeHardwareBuffers;
    /* current number of Tx packets (or lists) to return */
    LONG                    NetTxPacketsToReturn;
    /* total of Rx buffer in turnaround */
    UINT                    NetMaxReceiveBuffers;
    struct VirtIOBufferDescriptor *sgTxGatherTable;
    UINT                    nPnpEventIndex;
    NDIS_DEVICE_PNP_EVENT   PnpEvents[16];
    tOffloadSettings        Offload;
    NDIS_OFFLOAD_PARAMETERS InitialOffloadParameters;
    // we keep these members common for XP and Vista
    // for XP and non-MSI case of Vista they are set to zero
    ULONG                       ulRxMessage;
    ULONG                       ulTxMessage;
    ULONG                       ulControlMessage;

    NDIS_MINIPORT_INTERRUPT     Interrupt;
    NDIS_HANDLE                 PacketPool;
    NDIS_HANDLE                 BuffersPool;
    NDIS_HANDLE                 WrapperConfigurationHandle;
    LIST_ENTRY                  SendQueue;
    LIST_ENTRY                  TxWaitingList;
    NDIS_EVENT                  HaltEvent;
    NDIS_TIMER                  ConnectTimer;
    NDIS_TIMER                  DPCPostProcessTimer;
    BOOLEAN                     bDmaInitialized;
}PARANDIS_ADAPTER, *PPARANDIS_ADAPTER;

typedef enum { cpeOK, cpeNoBuffer, cpeInternalError, cpeTooLarge, cpeNoIndirect } tCopyPacketError;
typedef struct _tagCopyPacketResult
{
    ULONG       size;
    tCopyPacketError error;
}tCopyPacketResult;

typedef struct _tagSynchronizedContext
{
    PARANDIS_ADAPTER    *pContext;
    PVOID               Parameter;
}tSynchronizedContext;

typedef BOOLEAN (NTAPI *tSynchronizedProcedure)(tSynchronizedContext *context);

/**********************************************************
LAZY release procedure returns buffers to VirtIO
only where there are no free buffers available

NON-LAZY release releases transmit buffers from VirtIO
library every time there is something to release
***********************************************************/
//#define LAZY_TX_RELEASE

static inline bool VirtIODeviceGetHostFeature(PARANDIS_ADAPTER *pContext, unsigned uFeature)
{
   DPrintf(4, ("%s\n", __FUNCTION__));

   return virtio_is_feature_enabled(pContext->ullHostFeatures, uFeature);
}

static inline void VirtIODeviceEnableGuestFeature(PARANDIS_ADAPTER *pContext, unsigned uFeature)
{
   DPrintf(4, ("%s\n", __FUNCTION__));

   virtio_feature_enable(pContext->ullGuestFeatures, uFeature);
}

static BOOLEAN FORCEINLINE IsTimeToReleaseTx(PARANDIS_ADAPTER *pContext)
{
#ifndef LAZY_TX_RELEASE
    return pContext->nofFreeTxDescriptors < pContext->maxFreeTxDescriptors;
#else
    return pContext->nofFreeTxDescriptors == 0;
#endif
}

static BOOLEAN FORCEINLINE IsValidVlanId(PARANDIS_ADAPTER *pContext, ULONG VlanID)
{
    return pContext->VlanId == 0 || pContext->VlanId == VlanID;
}

static BOOLEAN FORCEINLINE IsVlanSupported(PARANDIS_ADAPTER *pContext)
{
    return pContext->ulPriorityVlanSetting & 2;
}

static BOOLEAN FORCEINLINE IsPrioritySupported(PARANDIS_ADAPTER *pContext)
{
    return pContext->ulPriorityVlanSetting & 1;
}

BOOLEAN ParaNdis_ValidateMacAddress(
    PUCHAR pcMacAddress,
    BOOLEAN bLocal);

NDIS_STATUS ParaNdis_InitializeContext(
    PARANDIS_ADAPTER *pContext,
    PNDIS_RESOURCE_LIST ResourceList);

NDIS_STATUS ParaNdis_FinishInitialization(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_CleanupContext(
    PARANDIS_ADAPTER *pContext);


UINT ParaNdis_VirtIONetReleaseTransmitBuffers(
    PARANDIS_ADAPTER *pContext);

ULONG ParaNdis_DPCWorkBody(
  PARANDIS_ADAPTER *pContext,
  ULONG ulMaxPacketsToIndicate);

NDIS_STATUS ParaNdis_SetMulticastList(
    PARANDIS_ADAPTER *pContext,
    PVOID Buffer,
    ULONG BufferSize,
    PUINT pBytesRead,
    PUINT pBytesNeeded);

VOID ParaNdis_VirtIOEnableIrqSynchronized(
    PARANDIS_ADAPTER *pContext,
    ULONG interruptSource);

VOID ParaNdis_VirtIODisableIrqSynchronized(
    PARANDIS_ADAPTER *pContext,
    ULONG interruptSource);

static __inline struct virtqueue *
ParaNdis_GetQueueForInterrupt(PARANDIS_ADAPTER *pContext, ULONG interruptSource)
{
    if (interruptSource & isTransmit)
        return pContext->NetSendQueue;
    if (interruptSource & isReceive)
        return pContext->NetReceiveQueue;

    return NULL;
}

static __inline BOOLEAN
ParaNDIS_IsQueueInterruptEnabled(struct virtqueue * _vq)
{
    return virtqueue_is_interrupt_enabled(_vq);
}

VOID ParaNdis_OnPnPEvent(
    PARANDIS_ADAPTER *pContext,
    NDIS_DEVICE_PNP_EVENT pEvent,
    PVOID   pInfo,
    ULONG   ulSize);

BOOLEAN ParaNdis_OnLegacyInterrupt(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN *pRunDpc);

BOOLEAN ParaNdis_OnQueuedInterrupt(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN *pRunDpc,
    ULONG knownInterruptSources);

VOID ParaNdis_OnShutdown(
    PARANDIS_ADAPTER *pContext);

BOOLEAN ParaNdis_CheckForHang(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_ReportLinkStatus(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN bForce);

NDIS_STATUS ParaNdis_PowerOn(
    PARANDIS_ADAPTER *pContext
);

VOID ParaNdis_PowerOff(
    PARANDIS_ADAPTER *pContext
);

void ParaNdis_DebugInitialize(PVOID DriverObject,PVOID RegistryPath);
void ParaNdis_DebugCleanup(PDRIVER_OBJECT  pDriverObject);
void ParaNdis_DebugRegisterMiniport(PARANDIS_ADAPTER *pContext, BOOLEAN bRegister);


//#define ENABLE_HISTORY_LOG
#if !defined(ENABLE_HISTORY_LOG)

static void FORCEINLINE ParaNdis_DebugHistory(
    PARANDIS_ADAPTER *pContext,
    eHistoryLogOperation op,
    PVOID pParam1,
    ULONG lParam2,
    ULONG lParam3,
    ULONG lParam4)
{

}

#else

void ParaNdis_DebugHistory(
    PARANDIS_ADAPTER *pContext,
    eHistoryLogOperation op,
    PVOID pParam1,
    ULONG lParam2,
    ULONG lParam3,
    ULONG lParam4);

#endif

typedef struct _tagTxOperationParameters
{
    tPacketType     packet;
    PVOID           ReferenceValue;
    UINT            nofSGFragments;
    ULONG           ulDataSize;
    ULONG           offloadMss;
    ULONG           tcpHeaderOffset;
    ULONG           flags;      //see tPacketOffloadRequest
}tTxOperationParameters;

tCopyPacketResult ParaNdis_DoCopyPacketData(
    PARANDIS_ADAPTER *pContext,
    tTxOperationParameters *pParams);

typedef struct _tagMapperResult
{
    USHORT  usBuffersMapped;
    USHORT  usBufferSpaceUsed;
    ULONG   ulDataSize;
}tMapperResult;


tCopyPacketResult ParaNdis_DoSubmitPacket(PARANDIS_ADAPTER *pContext, tTxOperationParameters *Params);

void ParaNdis_ResetOffloadSettings(PARANDIS_ADAPTER *pContext, tOffloadSettingsFlags *pDest, PULONG from);

tChecksumCheckResult ParaNdis_CheckRxChecksum(PARANDIS_ADAPTER *pContext, ULONG virtioFlags, PVOID pRxPacket, ULONG len);

void ParaNdis_CallOnBugCheck(PARANDIS_ADAPTER *pContext);

/*****************************************************
Procedures to implement for NDIS specific implementation
******************************************************/

PVOID ParaNdis_AllocateMemory(
    PARANDIS_ADAPTER *pContext,
    ULONG ulRequiredSize);

NDIS_STATUS NTAPI ParaNdis_FinishSpecificInitialization(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_FinalizeCleanup(
    PARANDIS_ADAPTER *pContext);

NDIS_HANDLE ParaNdis_OpenNICConfiguration(
    PARANDIS_ADAPTER *pContext);

tPacketIndicationType ParaNdis_IndicateReceivedPacket(
    PARANDIS_ADAPTER *pContext,
    PVOID dataBuffer,
    PULONG pLength,
    BOOLEAN bPrepareOnly,
    pIONetDescriptor pBufferDesc);

VOID ParaNdis_IndicateReceivedBatch(
    PARANDIS_ADAPTER *pContext,
    tPacketIndicationType *pBatch,
    ULONG nofPackets);

VOID ParaNdis_PacketMapper(
    PARANDIS_ADAPTER *pContext,
    tPacketType packet,
    PVOID Reference,
    struct VirtIOBufferDescriptor *buffers,
    pIONetDescriptor pDesc,
    tMapperResult *pMapperResult
    );

tCopyPacketResult ParaNdis_PacketCopier(
    tPacketType packet,
    PVOID dest,
    ULONG maxSize,
    PVOID refValue,
    BOOLEAN bPreview);

BOOLEAN ParaNdis_ProcessTx(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN IsDpc,
    BOOLEAN IsInterrupt);

BOOLEAN ParaNdis_SetTimer(
    NDIS_HANDLE timer,
    LONG millies);

BOOLEAN ParaNdis_SynchronizeWithInterrupt(
    PARANDIS_ADAPTER *pContext,
    ULONG messageId,
    tSynchronizedProcedure procedure,
    PVOID parameter);

VOID ParaNdis_Suspend(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_Resume(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_OnTransmitBufferReleased(
    PARANDIS_ADAPTER *pContext,
    IONetDescriptor *pDesc);


typedef VOID (*tOnAdditionalPhysicalMemoryAllocated)(
    PARANDIS_ADAPTER *pContext,
    tCompletePhysicalAddress *pAddresses);


typedef struct _tagPhysicalAddressAllocationContext
{
    tCompletePhysicalAddress address;
    PARANDIS_ADAPTER *pContext;
    tOnAdditionalPhysicalMemoryAllocated Callback;
} tPhysicalAddressAllocationContext;


BOOLEAN ParaNdis_InitialAllocatePhysicalMemory(
    PARANDIS_ADAPTER *pContext,
    tCompletePhysicalAddress *pAddresses);

VOID ParaNdis_FreePhysicalMemory(
    PARANDIS_ADAPTER *pContext,
    tCompletePhysicalAddress *pAddresses);

BOOLEAN ParaNdis_BindBufferToPacket(
    PARANDIS_ADAPTER *pContext,
    pIONetDescriptor pBufferDesc);

void ParaNdis_UnbindBufferFromPacket(
    PARANDIS_ADAPTER *pContext,
    pIONetDescriptor pBufferDesc);

void ParaNdis_IndicateConnect(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN bConnected,
    BOOLEAN bForce);

void ParaNdis_RestoreDeviceConfigurationAfterReset(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_UpdateDeviceFilters(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_DeviceFiltersUpdateVlanId(
    PARANDIS_ADAPTER *pContext);

VOID ParaNdis_SetPowerState(
    PARANDIS_ADAPTER *pContext,
    NDIS_DEVICE_POWER_STATE newState);


#endif //-OFFLOAD_UNIT_TEST

typedef enum _tagppResult
{
    ppresNotTested = 0,
    ppresNotIP     = 1,
    ppresIPV4      = 2,
    ppresIPV6      = 3,
    ppresIPTooShort  = 1,
    ppresPCSOK       = 1,
    ppresCSOK        = 2,
    ppresCSBad       = 3,
    ppresXxpOther    = 1,
    ppresXxpKnown    = 2,
    ppresXxpIncomplete = 3,
    ppresIsTCP         = 0,
    ppresIsUDP         = 1,
}ppResult;

typedef union _tagTcpIpPacketParsingResult
{
    struct {
        /* 0 - not tested, 1 - not IP, 2 - IPV4, 3 - IPV6 */
        ULONG ipStatus          : 2;
        /* 0 - not tested, 1 - n/a, 2 - CS, 3 - bad */
        ULONG ipCheckSum        : 2;
        /* 0 - not tested, 1 - PCS, 2 - CS, 3 - bad */
        ULONG xxpCheckSum       : 2;
        /* 0 - not tested, 1 - other, 2 - known(contains basic TCP or UDP header), 3 - known incomplete */
        ULONG xxpStatus         : 2;
        /* 1 - contains complete payload */
        ULONG xxpFull           : 1;
        ULONG TcpUdp            : 1;
        ULONG fixedIpCS         : 1;
        ULONG fixedXxpCS        : 1;
        ULONG IsFragment        : 1;
        ULONG reserved          : 3;
        ULONG ipHeaderSize      : 8;
        ULONG XxpIpHeaderSize   : 8;
    };
    ULONG value;
}tTcpIpPacketParsingResult;

typedef enum _tagPacketOffloadRequest
{
    pcrIpChecksum  = (1 << 0),
    pcrTcpV4Checksum = (1 << 1),
    pcrUdpV4Checksum = (1 << 2),
    pcrTcpV6Checksum = (1 << 3),
    pcrUdpV6Checksum = (1 << 4),
    pcrTcpChecksum = (pcrTcpV4Checksum | pcrTcpV6Checksum),
    pcrUdpChecksum = (pcrUdpV4Checksum | pcrUdpV6Checksum),
    pcrAnyChecksum = (pcrIpChecksum | pcrTcpV4Checksum | pcrUdpV4Checksum | pcrTcpV6Checksum | pcrUdpV6Checksum),
    pcrLSO   = (1 << 5),
    pcrIsIP  = (1 << 6),
    pcrFixIPChecksum = (1 << 7),
    pcrFixPHChecksum = (1 << 8),
    pcrFixTcpV4Checksum = (1 << 9),
    pcrFixUdpV4Checksum = (1 << 10),
    pcrFixTcpV6Checksum = (1 << 11),
    pcrFixUdpV6Checksum = (1 << 12),
    pcrFixXxpChecksum = (pcrFixTcpV4Checksum | pcrFixUdpV4Checksum | pcrFixTcpV6Checksum | pcrFixUdpV6Checksum),
    pcrPriorityTag = (1 << 13),
    pcrNoIndirect  = (1 << 14)
}tPacketOffloadRequest;

// sw offload
tTcpIpPacketParsingResult ParaNdis_CheckSumVerify(PVOID buffer, ULONG size, ULONG flags, LPCSTR caller);
tTcpIpPacketParsingResult ParaNdis_ReviewIPPacket(PVOID buffer, ULONG size, LPCSTR caller);

void ParaNdis_PadPacketReceived(PVOID pDataBuffer, PULONG pLength);

#endif
