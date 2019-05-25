/*****************************************************************************
 * miniport_dmus.cpp - UART miniport implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *      Feb 98    MartinP   --  based on UART, began deltas for DirectMusic.
 *
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

//  + for absolute / - for relative

#define kOneMillisec (10 * 1000)

//
// MPU401 ports
//
#define MPU401_REG_STATUS   0x01    // Status register
#define MPU401_DRR          0x40    // Output ready (for command or data)
                                    // if this bit is set, the output FIFO is FULL
#define MPU401_DSR          0x80    // Input ready (for data)
                                    // if this bit is set, the input FIFO is empty

#define MPU401_REG_DATA     0x00    // Data in
#define MPU401_REG_COMMAND  0x01    // Commands
#define MPU401_CMD_RESET    0xFF    // Reset command
#define MPU401_CMD_UART     0x3F    // Switch to UART mod


/*****************************************************************************
 * Prototypes
 */

NTSTATUS NTAPI InitMPU(IN PINTERRUPTSYNC InterruptSync,IN PVOID DynamicContext);
NTSTATUS ResetHardware(PUCHAR portBase);
NTSTATUS ValidatePropertyRequest(IN PPCPROPERTY_REQUEST pRequest, IN ULONG ulValueSize, IN BOOLEAN fValueRequired);
NTSTATUS NTAPI PropertyHandler_Synth(IN PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI DMusMPUInterruptServiceRoutine(PINTERRUPTSYNC InterruptSync,PVOID DynamicContext);
VOID NTAPI DMusUARTTimerDPC(PKDPC Dpc,PVOID DeferredContext,PVOID SystemArgument1,PVOID SystemArgument2);
NTSTATUS NTAPI SynchronizedDMusMPUWrite(PINTERRUPTSYNC InterruptSync,PVOID syncWriteContext);
/*****************************************************************************
 * Constants
 */

const BOOLEAN   COMMAND   = TRUE;
const BOOLEAN   DATA      = FALSE;
const ULONG      kMPUInputBufferSize = 128;


/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportDMusUART
 *****************************************************************************
 * MPU-401 miniport.  This object is associated with the device and is 
 * created when the device is started.  The class inherits IMiniportDMus
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportDMusUART
:   public IMiniportDMus,
    public IMusicTechnology,
    public IPowerNotify
{
private:
    LONG            m_Ref;                  // Reference count
    KSSTATE         m_KSStateInput;         // Miniport state (RUN/PAUSE/ACQUIRE/STOP)
    PPORTDMUS       m_pPort;                // Callback interface.
    PUCHAR          m_pPortBase;            // Base port address.
    PINTERRUPTSYNC  m_pInterruptSync;       // Interrupt synchronization object.
    PSERVICEGROUP   m_pServiceGroup;        // Service group for capture.
    PMASTERCLOCK    m_MasterClock;          // for input data
    REFERENCE_TIME  m_InputTimeStamp;       // capture data timestamp
    USHORT          m_NumRenderStreams;     // Num active render streams.
    USHORT          m_NumCaptureStreams;    // Num active capture streams.
    ULONG            m_MPUInputBufferHead;   // Index of the newest byte in the FIFO.
    ULONG            m_MPUInputBufferTail;   // Index of the oldest empty space in the FIFO.
    GUID            m_MusicFormatTechnology;
    POWER_STATE     m_PowerState;           // Saved power state (D0 = full power, D3 = off)
    BOOLEAN         m_fMPUInitialized;      // Is the MPU HW initialized.
    BOOLEAN         m_UseIRQ;               // FALSE if no IRQ is used for MIDI.
    UCHAR           m_MPUInputBuffer[kMPUInputBufferSize];  // Internal SW FIFO.

    /*************************************************************************
     * CMiniportDMusUART methods
     *
     * These are private member functions used internally by the object.
     * See MINIPORT.CPP for specific descriptions.
     */
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );
    NTSTATUS InitializeHardware(PINTERRUPTSYNC interruptSync,PUCHAR portBase);

public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    CMiniportDMusUART(IUnknown * Unknown){}
    virtual ~CMiniportDMusUART();

    /*************************************************************************
     * IMiniport methods
     */
    STDMETHODIMP_(NTSTATUS) 
    GetDescription
    (   OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
    );
    STDMETHODIMP_(NTSTATUS) 
    DataRangeIntersection
    (   IN      ULONG           PinId
    ,   IN      PKSDATARANGE    DataRange
    ,   IN      PKSDATARANGE    MatchingDataRange
    ,   IN      ULONG           OutputBufferLength
    ,   OUT     PVOID           ResultantFormat
    ,   OUT     PULONG          ResultantFormatLength
    )
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /*************************************************************************
     * IMiniportDMus methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTDMUS       Port,
        OUT     PSERVICEGROUP * ServiceGroup
    );
    STDMETHODIMP_(NTSTATUS) NewStream
    (
        OUT     PMXF                  * Stream,
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,
        IN      POOL_TYPE               PoolType,
        IN      ULONG                   PinID,
        IN      DMUS_STREAM_TYPE        StreamType,
        IN      PKSDATAFORMAT           DataFormat,
        OUT     PSERVICEGROUP         * ServiceGroup,
        IN      PAllocatorMXF           AllocatorMXF,
        IN      PMASTERCLOCK            MasterClock,
        OUT     PULONGLONG              SchedulePreFetch
    );
    STDMETHODIMP_(void) Service
    (   void
    );

    /*************************************************************************
     * IMusicTechnology methods
     */
    IMP_IMusicTechnology;

    /*************************************************************************
     * IPowerNotify methods
     */
    IMP_IPowerNotify;

    /*************************************************************************
     * Friends 
     */
    friend class CMiniportDMusUARTStream;
    friend NTSTATUS NTAPI
        DMusMPUInterruptServiceRoutine(PINTERRUPTSYNC InterruptSync,PVOID DynamicContext);
    friend NTSTATUS NTAPI
        SynchronizedDMusMPUWrite(PINTERRUPTSYNC InterruptSync,PVOID syncWriteContext);
    friend VOID NTAPI 
        DMusUARTTimerDPC(PKDPC Dpc,PVOID DeferredContext,PVOID SystemArgument1,PVOID SystemArgument2);
    friend NTSTATUS NTAPI PropertyHandler_Synth(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend STDMETHODIMP_(NTSTATUS) SnapTimeStamp(PINTERRUPTSYNC InterruptSync,PVOID pStream);
};

/*****************************************************************************
 * CMiniportDMusUARTStream
 *****************************************************************************
 * MPU-401 miniport stream.  This object is associated with the pin and is
 * created when the pin is instantiated.  It inherits IMXF
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportDMusUARTStream :   public IMXF
{
private:
    LONG                m_Ref;                  // Reference Count
    CMiniportDMusUART * m_pMiniport;            // Parent.
    REFERENCE_TIME      m_SnapshotTimeStamp;    // Current snapshot of miniport's input timestamp.
    PUCHAR              m_pPortBase;            // Base port address.
    BOOLEAN             m_fCapture;             // Whether this is capture.
    long                m_NumFailedMPUTries;    // Deadman timeout for MPU hardware.
    PAllocatorMXF       m_AllocatorMXF;         // source/sink for DMus structs
    PMXF                m_sinkMXF;              // sink for DMus capture
    PDMUS_KERNEL_EVENT  m_DMKEvtQueue;          // queue of waiting events
    ULONG               m_NumberOfRetries;      // Number of consecutive times the h/w was busy/full
    ULONG               m_DMKEvtOffset;         // offset into the event
    KDPC                m_Dpc;                  // DPC for timer
    KTIMER              m_TimerEvent;           // timer 
    BOOL                m_TimerQueued;          // whether a timer has been set
    KSPIN_LOCK          m_DpcSpinLock;          // protects the ConsumeEvents DPC

    STDMETHODIMP_(NTSTATUS) SourceEvtsToPort();
    STDMETHODIMP_(NTSTATUS) ConsumeEvents();
    STDMETHODIMP_(NTSTATUS) PutMessageLocked(PDMUS_KERNEL_EVENT pDMKEvt);

public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    virtual ~CMiniportDMusUARTStream();

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CMiniportDMusUART * pMiniport,
        IN      PUCHAR              pPortBase,
        IN      BOOLEAN             fCapture,
        IN      PAllocatorMXF       allocatorMXF,
        IN      PMASTERCLOCK        masterClock
    );

    NTSTATUS HandlePortParams
    (
        IN      PPCPROPERTY_REQUEST     Request
    );

    /*************************************************************************
     * IMiniportStreamDMusUART methods
     */
    IMP_IMXF;

    STDMETHODIMP_(NTSTATUS) Write
    (
        IN      PVOID       BufferAddress,
        IN      ULONG       BytesToWrite,
        OUT     PULONG      BytesWritten
    );

    friend VOID NTAPI
    DMusUARTTimerDPC
    (
        IN      PKDPC   Dpc,
        IN      PVOID   DeferredContext,
        IN      PVOID   SystemArgument1,
        IN      PVOID   SystemArgument2
    );
    friend NTSTATUS NTAPI PropertyHandler_Synth(IN PPCPROPERTY_REQUEST);
    friend STDMETHODIMP_(NTSTATUS) SnapTimeStamp(PINTERRUPTSYNC InterruptSync,PVOID pStream);
};



#define STR_MODULENAME "DMusUART:Miniport: "

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

#define UartFifoOkForWrite(status)  ((status & MPU401_DRR) == 0)
#define UartFifoOkForRead(status)   ((status & MPU401_DSR) == 0)

typedef struct
{
    CMiniportDMusUART  *Miniport;
    PUCHAR              PortBase;
    PVOID               BufferAddress;
    ULONG               Length;
    PULONG              BytesRead;
}
SYNCWRITECONTEXT, *PSYNCWRITECONTEXT;

/*****************************************************************************
 * PinDataRangesStreamLegacy
 * PinDataRangesStreamDMusic
 *****************************************************************************
 * Structures indicating range of valid format values for live pins.
 */
static
KSDATARANGE_MUSIC PinDataRangesStreamLegacy =
{
    {
        {
            sizeof(KSDATARANGE_MUSIC),
            0,
            0,
            0,
            {STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC)},
            {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI)},
            {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)}
        }
    },
    {STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT)},
    0,
    0,
    0xFFFF
};
static
KSDATARANGE_MUSIC PinDataRangesStreamDMusic =
{
    {
        {
            sizeof(KSDATARANGE_MUSIC),
            0,
            0,
            0,
            {STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC)},
            {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_DIRECTMUSIC)},
            {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)}
        }
    },
    {STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT)},
    0,
    0,
    0xFFFF
};

/*****************************************************************************
 * PinDataRangePointersStreamLegacy
 * PinDataRangePointersStreamDMusic
 * PinDataRangePointersStreamCombined
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for live pins.
 */
static
PKSDATARANGE PinDataRangePointersStreamLegacy[] =
{
    PKSDATARANGE(&PinDataRangesStreamLegacy)
};
static
PKSDATARANGE PinDataRangePointersStreamDMusic[] =
{
    PKSDATARANGE(&PinDataRangesStreamDMusic)
};
static
PKSDATARANGE PinDataRangePointersStreamCombined[] =
{
    PKSDATARANGE(&PinDataRangesStreamLegacy)
   ,PKSDATARANGE(&PinDataRangesStreamDMusic)
};

/*****************************************************************************
 * PinDataRangesBridge
 *****************************************************************************
 * Structures indicating range of valid format values for bridge pins.
 */
static
KSDATARANGE PinDataRangesBridge[] =
{
    {
        {
            sizeof(KSDATARANGE),
            0,
            0,
            0,
            {STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC)},
            {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI_BUS)},
            {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)}
        }
   }
};

/*****************************************************************************
 * PinDataRangePointersBridge
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for bridge pins.
 */
static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

/*****************************************************************************
 * SynthProperties
 *****************************************************************************
 * List of properties in the Synth set.
 */
static
PCPROPERTY_ITEM
SynthProperties[] =
{
    // Global: S/Get synthesizer caps
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CAPS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Global: S/Get port parameters
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_PORTPARAMETERS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per stream: S/Get channel groups
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CHANNELGROUPS,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per stream: Get current latency time
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_LATENCYCLOCK,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    }
};
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSynth,  SynthProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSynth2, SynthProperties);

#define kMaxNumCaptureStreams       1
#define kMaxNumLegacyRenderStreams  1
#define kMaxNumDMusicRenderStreams  1

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR MiniportPins[] =
{
    {
        kMaxNumLegacyRenderStreams,kMaxNumLegacyRenderStreams,0,    // InstanceCount
        NULL,                                                       // AutomationTable
        {                                                           // KsPinDescriptor
            0,                                              // InterfacesCount
            NULL,                                           // Interfaces
            0,                                              // MediumsCount
            NULL,                                           // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStreamLegacy), // DataRangesCount
            PinDataRangePointersStreamLegacy,               // DataRanges
            KSPIN_DATAFLOW_IN,                              // DataFlow
            KSPIN_COMMUNICATION_SINK,                       // Communication
            (GUID *) &KSCATEGORY_AUDIO,                     // Category
            &KSAUDFNAME_MIDI,                               // Name
            {0}                                               // Reserved
        }
    },
    {
        kMaxNumDMusicRenderStreams,kMaxNumDMusicRenderStreams,0,    // InstanceCount
        NULL,                                                       // AutomationTable
        {                                                           // KsPinDescriptor
            0,                                              // InterfacesCount
            NULL,                                           // Interfaces
            0,                                              // MediumsCount
            NULL,                                           // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStreamDMusic), // DataRangesCount
            PinDataRangePointersStreamDMusic,               // DataRanges
            KSPIN_DATAFLOW_IN,                              // DataFlow
            KSPIN_COMMUNICATION_SINK,                       // Communication
            (GUID *) &KSCATEGORY_AUDIO,                     // Category
            &KSAUDFNAME_DMUSIC_MPU_OUT,                     // Name
            {0}                                               // Reserved
        }
    },
    {
        0,0,0,                                      // InstanceCount
        NULL,                                       // AutomationTable
        {                                           // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            {0}                                           // Reserved
        }
    },
    {
        0,0,0,                                      // InstanceCount
        NULL,                                       // AutomationTable
        {                                           // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            {0}                                           // Reserved
        }
    },
    {
        kMaxNumCaptureStreams,kMaxNumCaptureStreams,0,      // InstanceCount
        NULL,                                               // AutomationTable
        {                                                   // KsPinDescriptor
            0,                                                // InterfacesCount
            NULL,                                             // Interfaces
            0,                                                // MediumsCount
            NULL,                                             // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStreamCombined), // DataRangesCount
            PinDataRangePointersStreamCombined,               // DataRanges
            KSPIN_DATAFLOW_OUT,                               // DataFlow
            KSPIN_COMMUNICATION_SINK,                         // Communication
            (GUID *) &KSCATEGORY_AUDIO,                       // Category
            &KSAUDFNAME_DMUSIC_MPU_IN,                        // Name
            {0}                                                 // Reserved
        }
    }
};

/*****************************************************************************
 * MiniportNodes
 *****************************************************************************
 * List of nodes.
 */
#define CONST_PCNODE_DESCRIPTOR(n)          { 0, NULL, &n, NULL }
#define CONST_PCNODE_DESCRIPTOR_AUTO(n,a)   { 0, &a, &n, NULL }
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
      CONST_PCNODE_DESCRIPTOR_AUTO(KSNODETYPE_SYNTHESIZER, AutomationSynth)
    , CONST_PCNODE_DESCRIPTOR_AUTO(KSNODETYPE_SYNTHESIZER, AutomationSynth2)
};

/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * List of connections.
 */
enum {
      eSynthNode  = 0
    , eInputNode
};

enum {
      eFilterInputPinLeg = 0,
      eFilterInputPinDM,
      eBridgeOutputPin,
      eBridgeInputPin,
      eFilterOutputPin
};

static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{  // From                                   To
   // Node           pin                     Node           pin
    { PCFILTER_NODE, eFilterInputPinLeg,     PCFILTER_NODE, eBridgeOutputPin }      // Legacy Stream in to synth.
  , { PCFILTER_NODE, eFilterInputPinDM,      eSynthNode,    KSNODEPIN_STANDARD_IN } // DM Stream in to synth.
  , { eSynthNode,    KSNODEPIN_STANDARD_OUT, PCFILTER_NODE, eBridgeOutputPin }      // Synth to bridge out.
  , { PCFILTER_NODE, eBridgeInputPin,        eInputNode,    KSNODEPIN_STANDARD_IN } // Bridge in to input.
  , { eInputNode,    KSNODEPIN_STANDARD_OUT, PCFILTER_NODE, eFilterOutputPin }      // Input to DM/Legacy Stream out.
};

/*****************************************************************************
 * MiniportCategories
 *****************************************************************************
 * List of categories.
 */
static
GUID MiniportCategories[] =
{
    {STATICGUIDOF(KSCATEGORY_AUDIO)},
    {STATICGUIDOF(KSCATEGORY_RENDER)},
    {STATICGUIDOF(KSCATEGORY_CAPTURE)}
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport filter description.
 */
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    SIZEOF_ARRAY(MiniportNodes),        // NodeCount
    MiniportNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    SIZEOF_ARRAY(MiniportCategories),   // CategoryCount
    MiniportCategories                  // Categories
};

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

BOOLEAN  TryMPU(IN PUCHAR PortBase);
NTSTATUS WriteMPU(IN PUCHAR PortBase,IN BOOLEAN IsCommand,IN UCHAR Value);

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

//  make sure we're in UART mode
NTSTATUS ResetHardware(PUCHAR portBase)
{
    PAGED_CODE();

    return WriteMPU(portBase,COMMAND,MPU401_CMD_UART);
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

//
// We initialize the UART with interrupts suppressed so we don't
// try to service the chip prematurely.
//
NTSTATUS CMiniportDMusUART::InitializeHardware(PINTERRUPTSYNC interruptSync,PUCHAR portBase)
{
    PAGED_CODE();

    NTSTATUS    ntStatus;
    if (m_UseIRQ)
    {
        ntStatus = interruptSync->CallSynchronizedRoutine(InitMPU,PVOID(portBase));
    }
    else
    {
        ntStatus = InitMPU(NULL,PVOID(portBase));
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Start the UART (this should trigger an interrupt).
        //
        ntStatus = ResetHardware(portBase);
    }
    else
    {
        DPRINT("*** InitMPU returned with ntStatus 0x%08x ***", ntStatus);
    }

    m_fMPUInitialized = NT_SUCCESS(ntStatus);

    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * InitMPU()
 *****************************************************************************
 * Synchronized routine to initialize the MPU401.
 */
NTSTATUS
NTAPI
InitMPU
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    DPRINT("InitMPU");
    if (!DynamicContext)
    {
        return STATUS_INVALID_PARAMETER_2;
    }
        
    PUCHAR      portBase = PUCHAR(DynamicContext);
    UCHAR       status;
    ULONGLONG   startTime;
    BOOLEAN     success;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    
    //
    // Reset the card (puts it into "smart mode")
    //
    ntStatus = WriteMPU(portBase,COMMAND,MPU401_CMD_RESET);

    // wait for the acknowledgement
    // NOTE: When the Ack arrives, it will trigger an interrupt.  
    //       Normally the DPC routine would read in the ack byte and we
    //       would never see it, however since we have the hardware locked (HwEnter),
    //       we can read the port before the DPC can and thus we receive the Ack.
    startTime = PcGetTimeInterval(0);
    success = FALSE;
    while(PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        
        if (UartFifoOkForRead(status))                      // Is data waiting?
        {
            READ_PORT_UCHAR(portBase + MPU401_REG_DATA);    // yep.. read ACK 
            success = TRUE;                                 // don't need to do more 
            break;
        }
        KeStallExecutionProcessor(25);  //  microseconds
    }
#if (DBG)
    if (!success)
    {
        DPRINT("First attempt to reset the MPU didn't get ACKed.\n");
    }
#endif  //  (DBG)

    // NOTE: We cannot check the ACK byte because if the card was already in
    // UART mode it will not send an ACK but it will reset.

    // reset the card again
    (void) WriteMPU(portBase,COMMAND,MPU401_CMD_RESET);

                                    // wait for ack (again)
    startTime = PcGetTimeInterval(0); // This might take a while
    BYTE dataByte = 0;
    success = FALSE;
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        if (UartFifoOkForRead(status))                              // Is data waiting?
        {
            dataByte = READ_PORT_UCHAR(portBase + MPU401_REG_DATA); // yep.. read ACK
            success = TRUE;                                         // don't need to do more
            break;
        }
        KeStallExecutionProcessor(25);
    }

    if ((0xFE != dataByte) || !success)   // Did we succeed? If no second ACK, something is hosed  
    {                       
        DPRINT("Second attempt to reset the MPU didn't get ACKed.\n");
        DPRINT("Init Reset failure error. Ack = %X", ULONG(dataByte));

        ntStatus = STATUS_IO_DEVICE_ERROR;
    }
    
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * CMiniportDMusUARTStream::Write()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
Write
(
    IN      PVOID       BufferAddress,
    IN      ULONG       Length,
    OUT     PULONG      BytesWritten
)
{
    DPRINT("Write\n");
    ASSERT(BytesWritten);
    if (!BufferAddress)
    {
        Length = 0;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!m_fCapture)
    {
        PUCHAR  pMidiData;
        ULONG   count;

        count = 0;
        pMidiData = PUCHAR(BufferAddress);

        if (Length)
        {
            SYNCWRITECONTEXT context;
            context.Miniport        = (m_pMiniport);
            context.PortBase        = m_pPortBase;
            context.BufferAddress   = pMidiData;
            context.Length          = Length;
            context.BytesRead       = &count;

            if (m_pMiniport->m_UseIRQ)
            {
                ntStatus = m_pMiniport->m_pInterruptSync->
                                CallSynchronizedRoutine(SynchronizedDMusMPUWrite,PVOID(&context));
            }
            else    //  !m_UseIRQ
            {
                ntStatus = SynchronizedDMusMPUWrite(NULL,PVOID(&context));
            }       //  !m_UseIRQ

            if (count == 0)
            {
                m_NumFailedMPUTries++;
                if (m_NumFailedMPUTries >= 100)
                {
                    ntStatus = STATUS_IO_DEVICE_ERROR;
                    m_NumFailedMPUTries = 0;
                }
            }
            else
            {
                m_NumFailedMPUTries = 0;
            }
        }           //  if we have data at all
        *BytesWritten = count;
    }
    else    //  called write on the read stream
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * SynchronizedDMusMPUWrite()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
NTAPI
SynchronizedDMusMPUWrite
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           syncWriteContext
)
{
    PSYNCWRITECONTEXT context;
    context = (PSYNCWRITECONTEXT)syncWriteContext;
    ASSERT(context->Miniport);
    ASSERT(context->PortBase);
    ASSERT(context->BufferAddress);
    ASSERT(context->Length);
    ASSERT(context->BytesRead);

    PUCHAR  pChar = PUCHAR(context->BufferAddress);
    NTSTATUS ntStatus; // , readStatus
    ntStatus = STATUS_SUCCESS;
    //
    // while we're not there yet, and
    // while we don't have to wait on an aligned byte (including 0)
    // (we never wait on a byte.  Better to come back later)
    /*readStatus = */ DMusMPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    while (  (*(context->BytesRead) < context->Length)
          && (  TryMPU(context->PortBase) 
             || (*(context->BytesRead)%3)
          )  )
    {
        ntStatus = WriteMPU(context->PortBase,DATA,*pChar);
        if (NT_SUCCESS(ntStatus))
        {
            pChar++;
            *(context->BytesRead) = *(context->BytesRead) + 1;
//            readStatus = DMusMPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
        }
        else
        {
            DPRINT("SynchronizedDMusMPUWrite failed (0x%08x)",ntStatus);
            break;
        }
    }
    /*readStatus = */ DMusMPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    return ntStatus;
}

#define kMPUPollTimeout 2

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * TryMPU()
 *****************************************************************************
 * See if the MPU401 is free.
 */
BOOLEAN
TryMPU
(
    IN      PUCHAR      PortBase
)
{
    BOOLEAN success;
    USHORT  numPolls;
    UCHAR   status;

    DPRINT("TryMPU");
    numPolls = 0;

    while (numPolls < kMPUPollTimeout)
    {
        status = READ_PORT_UCHAR(PortBase + MPU401_REG_STATUS);
                                       
        if (UartFifoOkForWrite(status)) // Is this a good time to write data?
        {
            break;
        }
        numPolls++;
    }
    if (numPolls >= kMPUPollTimeout)
    {
        success = FALSE;
        DPRINT("TryMPU failed");
    }
    else
    {
        success = TRUE;
    }

    return success;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * WriteMPU()
 *****************************************************************************
 * Write a byte out to the MPU401.
 */
NTSTATUS
WriteMPU
(
    IN      PUCHAR      PortBase,
    IN      BOOLEAN     IsCommand,
    IN      UCHAR       Value
)
{
    DPRINT("WriteMPU");
    NTSTATUS ntStatus = STATUS_IO_DEVICE_ERROR;

    if (!PortBase)
    {
        DPRINT("O: PortBase is zero\n");
        return ntStatus;
    }
    PUCHAR deviceAddr = PortBase + MPU401_REG_DATA;

    if (IsCommand)
    {
        deviceAddr = PortBase + MPU401_REG_COMMAND;
    }

    ULONGLONG startTime = PcGetTimeInterval(0);
    
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        UCHAR status
        = READ_PORT_UCHAR(PortBase + MPU401_REG_STATUS);

        if (UartFifoOkForWrite(status)) // Is this a good time to write data?
        {                               // yep (Jon comment)
            WRITE_PORT_UCHAR(deviceAddr,Value);
            DPRINT("WriteMPU emitted 0x%02x",Value);
            ntStatus = STATUS_SUCCESS;
            break;
        }
    }
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * SnapTimeStamp()
 *****************************************************************************
 *
 * At synchronized execution to ISR, copy miniport's volatile m_InputTimeStamp 
 * to stream's m_SnapshotTimeStamp and zero m_InputTimeStamp.
 *
 */
STDMETHODIMP_(NTSTATUS) 
SnapTimeStamp(PINTERRUPTSYNC InterruptSync,PVOID pStream)
{
    CMiniportDMusUARTStream *pMPStream = (CMiniportDMusUARTStream *)pStream;

    //  cache the timestamp
    pMPStream->m_SnapshotTimeStamp = pMPStream->m_pMiniport->m_InputTimeStamp;

    //  if the window is closed, zero the timestamp
    if (pMPStream->m_pMiniport->m_MPUInputBufferHead == 
        pMPStream->m_pMiniport->m_MPUInputBufferTail)
    {
        pMPStream->m_pMiniport->m_InputTimeStamp = 0;
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportDMusUARTStream::SourceEvtsToPort()
 *****************************************************************************
 *
 * Reads incoming MIDI data, feeds into DMus events.
 * No need to touch the hardware, just read from our SW FIFO.
 *
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::SourceEvtsToPort()
{
    NTSTATUS    ntStatus;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    DPRINT("SourceEvtsToPort");

    if (m_fCapture)
    {
        ntStatus = STATUS_SUCCESS;
        if (m_pMiniport->m_MPUInputBufferHead != m_pMiniport->m_MPUInputBufferTail)
        {
            PDMUS_KERNEL_EVENT  aDMKEvt,eventTail,eventHead = NULL;

            while (m_pMiniport->m_MPUInputBufferHead != m_pMiniport->m_MPUInputBufferTail)
            {
                (void) m_AllocatorMXF->GetMessage(&aDMKEvt);
                if (!aDMKEvt)
                {
                    DPRINT("SourceEvtsToPort can't allocate DMKEvt");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //  put this event at the end of the list
                if (!eventHead)
                {
                    eventHead = aDMKEvt;
                }
                else
                {
                    eventTail = eventHead;
                    while (eventTail->pNextEvt)
                    {
                        eventTail = eventTail->pNextEvt;
                    }
                    eventTail->pNextEvt = aDMKEvt;
                }
                //  read all the bytes out of the buffer, into event(s)
                for (aDMKEvt->cbEvent = 0; aDMKEvt->cbEvent < sizeof(PBYTE); aDMKEvt->cbEvent++)
                {
                    if (m_pMiniport->m_MPUInputBufferHead == m_pMiniport->m_MPUInputBufferTail)
                    {
//                        _DbgPrintF(DEBUGLVL_TERSE, ("SourceEvtsToPort m_MPUInputBufferHead met m_MPUInputBufferTail, overrun"));
                        break;
                    }
                    aDMKEvt->uData.abData[aDMKEvt->cbEvent] = m_pMiniport->m_MPUInputBuffer[m_pMiniport->m_MPUInputBufferHead];
                    m_pMiniport->m_MPUInputBufferHead++;
                    if (m_pMiniport->m_MPUInputBufferHead >= kMPUInputBufferSize)
                    {
                        m_pMiniport->m_MPUInputBufferHead = 0;
                    }
                }
            }

            if (m_pMiniport->m_UseIRQ)
            {
                ntStatus = m_pMiniport->m_pInterruptSync->CallSynchronizedRoutine(SnapTimeStamp,PVOID(this));
            }
            else    //  !m_UseIRQ
            {
                ntStatus = SnapTimeStamp(NULL,PVOID(this));
            }       //  !m_UseIRQ
            aDMKEvt = eventHead;
            while (aDMKEvt)
            {
                aDMKEvt->ullPresTime100ns = m_SnapshotTimeStamp;
                aDMKEvt->usChannelGroup = 1;
                aDMKEvt->usFlags = DMUS_KEF_EVENT_INCOMPLETE;
                aDMKEvt = aDMKEvt->pNextEvt;
            }
            (void)m_sinkMXF->PutMessage(eventHead);
        }
    }
    else    //  render stream
    {
        DPRINT("SourceEvtsToPort called on render stream");
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * DMusMPUInterruptServiceRoutine()
 *****************************************************************************
 * ISR.
 */
NTSTATUS
NTAPI
DMusMPUInterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    DPRINT("DMusMPUInterruptServiceRoutine");
    ULONGLONG   startTime;

    ASSERT(DynamicContext);

    NTSTATUS            ntStatus;
    BOOL                newBytesAvailable;
    CMiniportDMusUART   *that;
    NTSTATUS            clockStatus;

    that = (CMiniportDMusUART *) DynamicContext;
    newBytesAvailable = FALSE;
    ntStatus = STATUS_UNSUCCESSFUL;

    UCHAR portStatus = 0xff;

    //
    // Read the MPU status byte.
    //
    if (that->m_pPortBase)
    {
        portStatus =
            READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_STATUS);

        //
        // If there is outstanding work to do and there is a port-driver for
        // the MPU miniport...
        //
        if (UartFifoOkForRead(portStatus) && that->m_pPort)
        {
            startTime = PcGetTimeInterval(0);
            while ( (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50)) 
                &&  (UartFifoOkForRead(portStatus)) )
            {
                UCHAR uDest = READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_DATA);
                if (    (that->m_KSStateInput == KSSTATE_RUN) 
                    &&  (that->m_NumCaptureStreams)
                   )
                {
                    ULONG    buffHead = that->m_MPUInputBufferHead;
                    if (   (that->m_MPUInputBufferTail + 1 == buffHead)
                        || (that->m_MPUInputBufferTail + 1 - kMPUInputBufferSize == buffHead))
                    {
                        DPRINT("*****MPU Input Buffer Overflow*****");
                    }
                    else
                    {
                        if (!that->m_InputTimeStamp)
                        {
                            clockStatus = that->m_MasterClock->GetTime(&that->m_InputTimeStamp);
                            if (STATUS_SUCCESS != clockStatus)
                            {
                                DPRINT("GetTime failed for clock 0x%08x",that->m_MasterClock);
                            }
                        }
                        newBytesAvailable = TRUE;
                        //  ...place the data in our FIFO...
                        that->m_MPUInputBuffer[that->m_MPUInputBufferTail] = uDest;
                        ASSERT(that->m_MPUInputBufferTail < kMPUInputBufferSize);
                        
                        that->m_MPUInputBufferTail++;
                        if (that->m_MPUInputBufferTail >= kMPUInputBufferSize)
                        {
                            that->m_MPUInputBufferTail = 0;
                        }
                    } 
                }
                //
                // Look for more MIDI data.
                //
                portStatus =
                    READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_STATUS);
            }   //  either there's no data or we ran too long
            if (newBytesAvailable)
            {
                //
                // ...notify the MPU port driver that we have bytes.
                //
                that->m_pPort->Notify(that->m_pServiceGroup);
            }
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportDMusUART::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    DPRINT("GetDescription");

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

NTSTATUS
NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId)
{
    CMiniportDMusUART * This;
    NTSTATUS Status;

    This= new(NonPagedPool, TAG_PORTCLASS) CMiniportDMusUART(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = This->QueryInterface(IID_IMiniport, (PVOID*)OutMiniport);

    if (!NT_SUCCESS(Status))
    {
        delete This;
    }

    DPRINT("NewMiniportDMusUART %p Status %x\n", *OutMiniport, Status);
    return Status;
}


#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

/*****************************************************************************
 * CMiniportDMusUART::ProcessResources()
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 */
NTSTATUS
CMiniportDMusUART::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    DPRINT("ProcessResources");

    ASSERT(ResourceList);
    if (!ResourceList)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    //
    // Get counts for the types of resources.
    //
    ULONG   countIO     = ResourceList->NumberOfPorts();
    ULONG   countIRQ    = ResourceList->NumberOfInterrupts();
    ULONG   countDMA    = ResourceList->NumberOfDmas();
    ULONG   lengthIO    = ResourceList->FindTranslatedPort(0)->u.Port.Length;

#if DBG
    DPRINT("Starting MPU401 Port 0x%lx", ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart);
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure we have the expected number of resources.
    //
    if  (   (countIO != 1)
        ||  (countIRQ  > 1)
        ||  (countDMA != 0)
        ||  (lengthIO == 0)
        )
    {
        DPRINT("Unknown ResourceList configuration");
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Get the port address.
        //
        m_pPortBase =
            PUCHAR(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);

        ntStatus = InitializeHardware(m_pInterruptSync,m_pPortBase);
    }

    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUART::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::QueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    DPRINT("Miniport::NonDelegatingQueryInterface");
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTDMUS(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportDMus))
    {
        *Object = PVOID(PMINIPORTDMUS(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMusicTechnology))
    {
        *Object = PVOID(PMUSICTECHNOLOGY(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IPowerNotify))
    {
        *Object = PVOID(PPOWERNOTIFY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUART::~CMiniportDMusUART()
 *****************************************************************************
 * Destructor.
 */
CMiniportDMusUART::~CMiniportDMusUART(void)
{
    PAGED_CODE();

    DPRINT("~CMiniportDMusUART");

    ASSERT(0 == m_NumCaptureStreams);
    ASSERT(0 == m_NumRenderStreams);

    //  reset the HW so we don't get anymore interrupts
    if (m_UseIRQ && m_pInterruptSync)
    {
        (void) m_pInterruptSync->CallSynchronizedRoutine((PINTERRUPTSYNCROUTINE)InitMPU,PVOID(m_pPortBase));
    }
    else
    {
        (void) InitMPU(NULL,PVOID(m_pPortBase));
    }

    if (m_pInterruptSync)
    {
        m_pInterruptSync->Release();
        m_pInterruptSync = NULL;
    }
    if (m_pServiceGroup)
    {
        m_pServiceGroup->Release();
        m_pServiceGroup = NULL;
    }
    if (m_pPort)
    {
        m_pPort->Release();
        m_pPort = NULL;
    }
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUART::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
Init
(
    IN      PUNKNOWN        UnknownInterruptSync    OPTIONAL,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTDMUS       Port_,
    OUT     PSERVICEGROUP * ServiceGroup
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    if (!ResourceList)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    ASSERT(Port_);
    ASSERT(ServiceGroup);

    DPRINT("Init");

    *ServiceGroup = NULL;
    m_pPortBase = 0;
    m_fMPUInitialized = FALSE;

    // This will remain unspecified if the miniport does not get any power
    // messages.
    //
    m_PowerState.DeviceState = PowerDeviceUnspecified;

    //
    // AddRef() is required because we are keeping this pointer.
    //
    m_pPort = Port_;
    m_pPort->AddRef();

    // Set dataformat.
    //
    if (IsEqualGUIDAligned(m_MusicFormatTechnology, GUID_NULL))
    {
        RtlCopyMemory(  &m_MusicFormatTechnology,
                        &KSMUSIC_TECHNOLOGY_PORT,
                        sizeof(GUID));
    }
    RtlCopyMemory(  &PinDataRangesStreamLegacy.Technology,
                    &m_MusicFormatTechnology,
                    sizeof(GUID));
    RtlCopyMemory(  &PinDataRangesStreamDMusic.Technology,
                    &m_MusicFormatTechnology,
                    sizeof(GUID));

    for (ULONG bufferCount = 0;bufferCount < kMPUInputBufferSize;bufferCount++)
    {
        m_MPUInputBuffer[bufferCount] = 0;
    }
    m_MPUInputBufferHead = 0;
    m_MPUInputBufferTail = 0;
    m_InputTimeStamp = 0;
    m_KSStateInput = KSSTATE_STOP;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    m_NumRenderStreams = 0;
    m_NumCaptureStreams = 0;

    m_UseIRQ = TRUE;
    if (ResourceList->NumberOfInterrupts() == 0)
    {
        m_UseIRQ = FALSE;
    }

    ntStatus = PcNewServiceGroup(&m_pServiceGroup,NULL);
    if (NT_SUCCESS(ntStatus) && !m_pServiceGroup)   //  keep any error
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        *ServiceGroup = m_pServiceGroup;
        m_pServiceGroup->AddRef();

        //
        // Register the service group with the port early so the port is
        // prepared to handle interrupts.
        //
        m_pPort->RegisterServiceGroup(m_pServiceGroup);
    }

    if (NT_SUCCESS(ntStatus) && m_UseIRQ)
    {
        //
        //  Due to a bug in the InterruptSync design, we shouldn't share
        //  the interrupt sync object.  Whoever goes away first
        //  will disconnect it, and the other points off into nowhere.
        //
        //  Instead we generate our own interrupt sync object.
        //
        UnknownInterruptSync = NULL;

        if (UnknownInterruptSync)
        {
            ntStatus =
                UnknownInterruptSync->QueryInterface
                (
                    IID_IInterruptSync,
                    (PVOID *) &m_pInterruptSync
                );

            if (!m_pInterruptSync && NT_SUCCESS(ntStatus))  //  keep any error
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            if (NT_SUCCESS(ntStatus))
            {                                                                           //  run this ISR first
                ntStatus = m_pInterruptSync->
                    RegisterServiceRoutine(DMusMPUInterruptServiceRoutine,PVOID(this),TRUE);
            }

        }
        else
        {   // create our own interruptsync mechanism.
            ntStatus =
                PcNewInterruptSync
                (
                    &m_pInterruptSync,
                    NULL,
                    ResourceList,
                    0,                          // Resource Index
                    InterruptSyncModeNormal     // Run ISRs once until we get SUCCESS
                );

            if (!m_pInterruptSync && NT_SUCCESS(ntStatus))    //  keep any error
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->RegisterServiceRoutine(
                    DMusMPUInterruptServiceRoutine,
                    PVOID(this),
                    TRUE);          //  run this ISR first
            }
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->Connect();
            }
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ProcessResources(ResourceList);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        //
        // clean up our mess
        //

        // clean up the interrupt sync
        if( m_pInterruptSync )
        {
            m_pInterruptSync->Release();
            m_pInterruptSync = NULL;
        }

        // clean up the service group
        if( m_pServiceGroup )
        {
            m_pServiceGroup->Release();
            m_pServiceGroup = NULL;
        }

        // clean up the out param service group.
        if (*ServiceGroup)
        {
            (*ServiceGroup)->Release();
            (*ServiceGroup) = NULL;
        }

        // release the port
        m_pPort->Release();
        m_pPort = NULL;
    }

    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUART::NewStream()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
NewStream
(
    OUT     PMXF                  * MXF,
    IN      PUNKNOWN                OuterUnknown    OPTIONAL,
    IN      POOL_TYPE               PoolType,
    IN      ULONG                   PinID,
    IN      DMUS_STREAM_TYPE        StreamType,
    IN      PKSDATAFORMAT           DataFormat,
    OUT     PSERVICEGROUP         * ServiceGroup,
    IN      PAllocatorMXF           AllocatorMXF,
    IN      PMASTERCLOCK            MasterClock,
    OUT     PULONGLONG              SchedulePreFetch
)
{
    PAGED_CODE();

    DPRINT("NewStream");
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // In 100 ns, we want stuff as soon as it comes in
    //
    *SchedulePreFetch = 0;

    // if we don't have any streams already open, get the hardware ready.
    if ((!m_NumCaptureStreams) && (!m_NumRenderStreams))
    {
        ntStatus = ResetHardware(m_pPortBase);
        if (!NT_SUCCESS(ntStatus))
        {
            DPRINT("CMiniportDMusUART::NewStream ResetHardware failed");
            return ntStatus;
        }
    }

    if  (   ((m_NumCaptureStreams < kMaxNumCaptureStreams)
            && (StreamType == DMUS_STREAM_MIDI_CAPTURE))
        ||  ((m_NumRenderStreams < kMaxNumLegacyRenderStreams + kMaxNumDMusicRenderStreams)
            && (StreamType == DMUS_STREAM_MIDI_RENDER))
        )
    {
        CMiniportDMusUARTStream *pStream =
            new(PoolType, 'wNcP') CMiniportDMusUARTStream();

        if (pStream)
        {
            pStream->AddRef();

            ntStatus =
                pStream->Init(this,m_pPortBase,(StreamType == DMUS_STREAM_MIDI_CAPTURE),AllocatorMXF,MasterClock);

            if (NT_SUCCESS(ntStatus))
            {
                *MXF = PMXF(pStream);
                (*MXF)->AddRef();

                if (StreamType == DMUS_STREAM_MIDI_CAPTURE)
                {
                    m_NumCaptureStreams++;
                    *ServiceGroup = m_pServiceGroup;
                    (*ServiceGroup)->AddRef();
                }
                else
                {
                    m_NumRenderStreams++;
                    *ServiceGroup = NULL;
                }
            }

            pStream->Release();
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        if (StreamType == DMUS_STREAM_MIDI_CAPTURE)
        {
            DPRINT("NewStream failed, too many capture streams");
        }
        else if (StreamType == DMUS_STREAM_MIDI_RENDER)
        {
            DPRINT("NewStream failed, too many render streams");
        }
        else
        {
            DPRINT("NewStream invalid stream type");
        }
    }

    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUART::SetTechnology()
 *****************************************************************************
 * Sets pindatarange technology.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
SetTechnology
(
    IN      const GUID *            Technology
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // Fail if miniport has already been initialized.
    //
    if (NULL == m_pPort)
    {
        RtlCopyMemory(&m_MusicFormatTechnology, Technology, sizeof(GUID));
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
} // SetTechnology

/*****************************************************************************
 * CMiniportDMusUART::PowerChangeNotify()
 *****************************************************************************
 * Handle power state change for the miniport.
 */
#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

STDMETHODIMP_(void)
CMiniportDMusUART::
PowerChangeNotify
(
    IN      POWER_STATE             PowerState
)
{
    PAGED_CODE();

    DPRINT("CMiniportDMusUART::PoweChangeNotify D%d", PowerState.DeviceState);

    switch (PowerState.DeviceState)
    {
        case PowerDeviceD0:
            if (m_PowerState.DeviceState != PowerDeviceD0)
            {
                if (!NT_SUCCESS(InitializeHardware(m_pInterruptSync,m_pPortBase)))
                {
                    DPRINT("InitializeHardware failed when resuming");
                }
            }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
        default:
            break;
    }
    m_PowerState.DeviceState = PowerState.DeviceState;
} // PowerChangeNotify

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUARTStream::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::QueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    DPRINT("Stream::NonDelegatingQueryInterface");
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMXF))
    {
        *Object = PVOID(PMXF(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUARTStream::~CMiniportDMusUARTStream()
 *****************************************************************************
 * Destructs a stream.
 */
CMiniportDMusUARTStream::~CMiniportDMusUARTStream(void)
{
    PAGED_CODE();

    DPRINT("~CMiniportDMusUARTStream");

    KeCancelTimer(&m_TimerEvent);

    if (m_DMKEvtQueue)
    {
        if (m_AllocatorMXF)
        {
            m_AllocatorMXF->PutMessage(m_DMKEvtQueue);
        }
        else
        {
            DPRINT("~CMiniportDMusUARTStream, no allocator, can't flush DMKEvts");
        }
        m_DMKEvtQueue = NULL;
    }
    if (m_AllocatorMXF)
    {
        m_AllocatorMXF->Release();
        m_AllocatorMXF = NULL;
    }

    if (m_pMiniport)
    {
        if (m_fCapture)
        {
            m_pMiniport->m_NumCaptureStreams--;
        }
        else
        {
            m_pMiniport->m_NumRenderStreams--;
        }

        m_pMiniport->Release();
    }
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUARTStream::Init()
 *****************************************************************************
 * Initializes a stream.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
Init
(
    IN      CMiniportDMusUART * pMiniport,
    IN      PUCHAR              pPortBase,
    IN      BOOLEAN             fCapture,
    IN      PAllocatorMXF       allocatorMXF,
    IN      PMASTERCLOCK        masterClock
)
{
    PAGED_CODE();

    ASSERT(pMiniport);
    ASSERT(pPortBase);

    DPRINT("Init");

    m_NumFailedMPUTries = 0;
    m_TimerQueued = FALSE;
    KeInitializeSpinLock(&m_DpcSpinLock);
    m_pMiniport = pMiniport;
    m_pMiniport->AddRef();

    pMiniport->m_MasterClock = masterClock;

    m_pPortBase = pPortBase;
    m_fCapture = fCapture;

    m_SnapshotTimeStamp = 0;
    m_DMKEvtQueue = NULL;
    m_DMKEvtOffset = 0;

    m_NumberOfRetries = 0;

    if (allocatorMXF)
    {
        allocatorMXF->AddRef();
        m_AllocatorMXF = allocatorMXF;
        m_sinkMXF = m_AllocatorMXF;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeDpc
    (
        &m_Dpc,
        &::DMusUARTTimerDPC,
        PVOID(this)
    );
    KeInitializeTimer(&m_TimerEvent);

    return STATUS_SUCCESS;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CMiniportDMusUARTStream::SetState()
 *****************************************************************************
 * Sets the state of the channel.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    DPRINT("SetState %d",NewState);

    if (NewState == KSSTATE_RUN)
    {
        if (m_pMiniport->m_fMPUInitialized)
        {
            LARGE_INTEGER timeDue100ns;
            timeDue100ns.QuadPart = 0;
            KeSetTimer(&m_TimerEvent,timeDue100ns,&m_Dpc);
        }
        else
        {
            DPRINT("CMiniportDMusUARTStream::SetState KSSTATE_RUN failed due to uninitialized MPU");
            return STATUS_INVALID_DEVICE_STATE;
        }
    }

    if (m_fCapture)
    {
        m_pMiniport->m_KSStateInput = NewState;
        if (NewState == KSSTATE_STOP)   //  STOPping
        {
            m_pMiniport->m_MPUInputBufferHead = 0;   // Previously read bytes are discarded.
            m_pMiniport->m_MPUInputBufferTail = 0;   // The entire FIFO is available.
        }
    }
    return STATUS_SUCCESS;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif


/*****************************************************************************
 * CMiniportDMusUART::Service()
 *****************************************************************************
 * DPC-mode service call from the port.
 */
STDMETHODIMP_(void)
CMiniportDMusUART::
Service
(   void
)
{
    DPRINT("Service");
    if (!m_NumCaptureStreams)
    {
        //  we should never get here....
        //  if we do, we must have read some trash,
        //  so just reset the input FIFO
        m_MPUInputBufferTail = m_MPUInputBufferHead = 0;
    }
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

/*****************************************************************************
 * CMiniportDMusUARTStream::ConnectOutput()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
CMiniportDMusUARTStream::
ConnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    if (m_fCapture)
    {
        if ((sinkMXF) && (m_sinkMXF == m_AllocatorMXF))
        {
            DPRINT("ConnectOutput");
            m_sinkMXF = sinkMXF;
            return STATUS_SUCCESS;
        }
        else
        {
            DPRINT("ConnectOutput failed");
        }
    }
    else
    {
        DPRINT("ConnectOutput called on renderer; failed");
    }
    return STATUS_UNSUCCESSFUL;
}

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

/*****************************************************************************
 * CMiniportDMusUARTStream::DisconnectOutput()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
CMiniportDMusUARTStream::
DisconnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    if (m_fCapture)
    {
        if ((m_sinkMXF == sinkMXF) || (!sinkMXF))
        {
            DPRINT("DisconnectOutput");
            m_sinkMXF = m_AllocatorMXF;
            return STATUS_SUCCESS;
        }
        else
        {
            DPRINT("DisconnectOutput failed");
        }
    }
    else
    {
        DPRINT("DisconnectOutput called on renderer; failed");
    }
    return STATUS_UNSUCCESSFUL;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif


/*****************************************************************************
 * CMiniportDMusUARTStream::PutMessageLocked()
 *****************************************************************************
 * Now that the spinlock is held, add this message to the queue.
 *
 * Writes an outgoing MIDI message.
 * We don't sort a new message into the queue -- we append it.
 * This is fine, since the sequencer feeds us sequenced data.
 * Timestamps will ascend by design.
 */
NTSTATUS CMiniportDMusUARTStream::PutMessageLocked(PDMUS_KERNEL_EVENT pDMKEvt)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDMUS_KERNEL_EVENT  aDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_fCapture)
    {
        DPRINT("PutMessage to render stream");
        if (pDMKEvt)
        {
            // m_DpcSpinLock already held

            if (m_DMKEvtQueue)
            {
                aDMKEvt = m_DMKEvtQueue;            //  put pDMKEvt in event queue

                while (aDMKEvt->pNextEvt)
                {
                    aDMKEvt = aDMKEvt->pNextEvt;
                }
                aDMKEvt->pNextEvt = pDMKEvt;        //  here is end of queue
            }
            else                                    //  currently nothing in queue
            {
                m_DMKEvtQueue = pDMKEvt;
                if (m_DMKEvtOffset)
                {
                    DPRINT("PutMessage  Nothing in the queue, but m_DMKEvtOffset == %d",m_DMKEvtOffset);
                    m_DMKEvtOffset = 0;
                }
            }

            // m_DpcSpinLock already held
        }
        if (!m_TimerQueued)
        {
            (void) ConsumeEvents();
        }
    }
    else    //  capture
    {
        DPRINT("PutMessage to capture stream");
        ASSERT(NULL == pDMKEvt);

        SourceEvtsToPort();
    }
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * CMiniportDMusUARTStream::PutMessage()
 *****************************************************************************
 * Writes an outgoing MIDI message.
 * We don't sort a new message into the queue -- we append it.
 * This is fine, since the sequencer feeds us sequenced data.
 * Timestamps will ascend by design.
 */
NTSTATUS CMiniportDMusUARTStream::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDMUS_KERNEL_EVENT  aDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_fCapture)
    {
        DPRINT("PutMessage to render stream");
        if (pDMKEvt)
        {
            KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);

            if (m_DMKEvtQueue)
            {
                aDMKEvt = m_DMKEvtQueue;            //  put pDMKEvt in event queue

                while (aDMKEvt->pNextEvt)
                {
                    aDMKEvt = aDMKEvt->pNextEvt;
                }
                aDMKEvt->pNextEvt = pDMKEvt;        //  here is end of queue
            }
            else                                    //  currently nothing in queue
            {
                m_DMKEvtQueue = pDMKEvt;
                if (m_DMKEvtOffset)
                {
                    DPRINT("PutMessage  Nothing in the queue, but m_DMKEvtOffset == %d", m_DMKEvtOffset);
                    m_DMKEvtOffset = 0;
                }
            }

            KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
        }
        if (!m_TimerQueued)
        {
            (void) ConsumeEvents();
        }
    }
    else    //  capture
    {
        DPRINT("PutMessage to capture stream");
        ASSERT(NULL == pDMKEvt);

        SourceEvtsToPort();
    }
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * CMiniportDMusUARTStream::ConsumeEvents()
 *****************************************************************************
 * Attempts to empty the render message queue.
 * Called either from DPC timer or upon IRP submittal.
//  TODO: support packages right
//  process the package (actually, should do this above.
//  treat the package as a list fragment that shouldn't be sorted.
//  better yet, go through each event in the package, and when
//  an event is exhausted, delete it and decrement m_offset.
 */
NTSTATUS CMiniportDMusUARTStream::ConsumeEvents(void)
{
    PDMUS_KERNEL_EVENT aDMKEvt;

    NTSTATUS        ntStatus = STATUS_SUCCESS;
    ULONG           bytesRemaining = 0,bytesWritten = 0;
    LARGE_INTEGER   aMillisecIn100ns;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);

    m_TimerQueued = FALSE;
    while (m_DMKEvtQueue)                   //  do we have anything to play at all?
    {
        aDMKEvt = m_DMKEvtQueue;                            //  event we try to play
        if (aDMKEvt->cbEvent)
        {
            bytesRemaining = aDMKEvt->cbEvent - m_DMKEvtOffset; //  number of bytes left in this evt

            ASSERT(bytesRemaining > 0);
            if (bytesRemaining <= 0)
            {
                bytesRemaining = aDMKEvt->cbEvent;
            }

            if (aDMKEvt->cbEvent <= sizeof(PBYTE))                //  short message
            {
                DPRINT("ConsumeEvents(%02x%02x%02x%02x)", aDMKEvt->uData.abData[0], aDMKEvt->uData.abData[1], aDMKEvt->uData.abData[2], aDMKEvt->uData.abData[3]);
                ntStatus = Write(aDMKEvt->uData.abData + m_DMKEvtOffset,bytesRemaining,&bytesWritten);
            }
            else if (PACKAGE_EVT(aDMKEvt))
            {
                ASSERT(m_DMKEvtOffset == 0);
                m_DMKEvtOffset = 0;
                DPRINT("ConsumeEvents(Package)");

                ntStatus = PutMessageLocked(aDMKEvt->uData.pPackageEvt);  // we already own the spinlock

                // null this because we are about to throw it in the allocator
                aDMKEvt->uData.pPackageEvt = NULL;
                aDMKEvt->cbEvent = 0;
                bytesWritten = bytesRemaining;
            }
            else                //  SysEx message
            {
                DPRINT("ConsumeEvents(%02x%02x%02x%02x)", aDMKEvt->uData.pbData[0], aDMKEvt->uData.pbData[1], aDMKEvt->uData.pbData[2], aDMKEvt->uData.pbData[3]);

                ntStatus = Write(aDMKEvt->uData.pbData + m_DMKEvtOffset,bytesRemaining,&bytesWritten);
            }
        }   //  if (aDMKEvt->cbEvent)
        if (STATUS_SUCCESS != ntStatus)
        {
            DPRINT("ConsumeEvents: Write returned 0x%08x", ntStatus);
            bytesWritten = bytesRemaining;  //  just bail on this event and try next time
        }

        ASSERT(bytesWritten <= bytesRemaining);
        if (bytesWritten == bytesRemaining)
        {
            m_DMKEvtQueue = m_DMKEvtQueue->pNextEvt;
            aDMKEvt->pNextEvt = NULL;

            m_AllocatorMXF->PutMessage(aDMKEvt);    //  throw back in free pool
            m_DMKEvtOffset = 0;                     //  start fresh on next evt
            m_NumberOfRetries = 0;
        }           //  but wait ... there's more!
        else        //  our FIFO is full for now.
        {
            //  update our offset by that amount we did write
            m_DMKEvtOffset += bytesWritten;
            ASSERT(m_DMKEvtOffset < aDMKEvt->cbEvent);

            DPRINT("ConsumeEvents tried %d, wrote %d, at offset %d", bytesRemaining,bytesWritten,m_DMKEvtOffset);
            aMillisecIn100ns.QuadPart = -(kOneMillisec);    //  set timer, come back later
            m_TimerQueued = TRUE;
            m_NumberOfRetries++;
            ntStatus = KeSetTimer( &m_TimerEvent, aMillisecIn100ns, &m_Dpc );
            break;
        }   //  we didn't write it all
    }       //  go back, Jack, do it again (while m_DMKEvtQueue)
    KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * CMiniportDMusUARTStream::HandlePortParams()
 *****************************************************************************
 * Writes an outgoing MIDI message.
 */
NTSTATUS
CMiniportDMusUARTStream::
HandlePortParams
(
    IN      PPCPROPERTY_REQUEST     pRequest
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if (pRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ntStatus = ValidatePropertyRequest(pRequest, sizeof(SYNTH_PORTPARAMS), TRUE);
    if (NT_SUCCESS(ntStatus))
    {
        RtlCopyMemory(pRequest->Value, pRequest->Instance, sizeof(SYNTH_PORTPARAMS));

        PSYNTH_PORTPARAMS Params = (PSYNTH_PORTPARAMS)pRequest->Value;

        if (Params->ValidParams & ~SYNTH_PORTPARAMS_CHANNELGROUPS)
        {
            Params->ValidParams &= SYNTH_PORTPARAMS_CHANNELGROUPS;
        }

        if (!(Params->ValidParams & SYNTH_PORTPARAMS_CHANNELGROUPS))
        {
            Params->ChannelGroups = 1;
        }
        else if (Params->ChannelGroups != 1)
        {
            Params->ChannelGroups = 1;
        }

        pRequest->ValueSize = sizeof(SYNTH_PORTPARAMS);
    }

    return ntStatus;
}

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*****************************************************************************
 * DMusTimerDPC()
 *****************************************************************************
 * The timer DPC callback. Thunks to a C++ member function.
 * This is called by the OS in response to the DirectMusic pin
 * wanting to wakeup later to process more DirectMusic stuff.
 */
VOID
NTAPI
DMusUARTTimerDPC
(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
)
{
    ASSERT(DeferredContext);

    CMiniportDMusUARTStream *aStream;
    aStream = (CMiniportDMusUARTStream *) DeferredContext;
    if (aStream)
    {
        DPRINT("DMusUARTTimerDPC");
        if (false == aStream->m_fCapture)
        {
            (void) aStream->ConsumeEvents();
        }
        //  ignores return value!
    }
}

/*****************************************************************************
 * DirectMusic properties
 ****************************************************************************/

#ifdef _MSC_VER
#pragma code_seg()
#endif

/*
 *  Properties concerning synthesizer functions.
 */
const WCHAR wszDescOut[] = L"DMusic MPU-401 Out ";
const WCHAR wszDescIn[] = L"DMusic MPU-401 In ";

NTSTATUS
NTAPI
PropertyHandler_Synth
(
    IN      PPCPROPERTY_REQUEST     pRequest
)
{
    NTSTATUS    ntStatus;

    PAGED_CODE();

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = ValidatePropertyRequest(pRequest, sizeof(ULONG), TRUE);
        if (NT_SUCCESS(ntStatus))
        {
            // if return buffer can hold a ULONG, return the access flags
            PULONG AccessFlags = PULONG(pRequest->Value);

            *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT;
            switch (pRequest->PropertyItem->Id)
            {
                case KSPROPERTY_SYNTH_CAPS:
                case KSPROPERTY_SYNTH_CHANNELGROUPS:
                    *AccessFlags |= KSPROPERTY_TYPE_GET;
            }
            switch (pRequest->PropertyItem->Id)
            {
                case KSPROPERTY_SYNTH_CHANNELGROUPS:
                    *AccessFlags |= KSPROPERTY_TYPE_SET;
            }
            ntStatus = STATUS_SUCCESS;
            pRequest->ValueSize = sizeof(ULONG);

            switch (pRequest->PropertyItem->Id)
            {
                case KSPROPERTY_SYNTH_PORTPARAMETERS:
                    if (pRequest->MinorTarget)
                    {
                        *AccessFlags |= KSPROPERTY_TYPE_GET;
                    }
                    else
                    {
                        pRequest->ValueSize = 0;
                        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                    }
            }
        }
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
        switch(pRequest->PropertyItem->Id)
        {
            case KSPROPERTY_SYNTH_CAPS:
                DPRINT("PropertyHandler_Synth:KSPROPERTY_SYNTH_CAPS");

                if (pRequest->Verb & KSPROPERTY_TYPE_SET)
                {
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                }

                if (NT_SUCCESS(ntStatus))
                {
                    ntStatus = ValidatePropertyRequest(pRequest, sizeof(SYNTHCAPS), TRUE);

                    if (NT_SUCCESS(ntStatus))
                    {
                        SYNTHCAPS *caps = (SYNTHCAPS*)pRequest->Value;
                        int increment;
                        RtlZeroMemory(caps, sizeof(SYNTHCAPS));
                        // XXX Different guids for different instances!
                        //
                        if (pRequest->Node == eSynthNode)
                        {
                            increment = sizeof(wszDescOut) - 2;
                            RtlCopyMemory( caps->Description,wszDescOut,increment);
                            caps->Guid           = CLSID_MiniportDriverDMusUART;
                        }
                        else
                        {
                            increment = sizeof(wszDescIn) - 2;
                            RtlCopyMemory( caps->Description,wszDescIn,increment);
                            caps->Guid           = CLSID_MiniportDriverDMusUARTCapture;
                        }

                        caps->Flags              = SYNTH_PC_EXTERNAL;
                        caps->MemorySize         = 0;
                        caps->MaxChannelGroups   = 1;
                        caps->MaxVoices          = 0xFFFFFFFF;
                        caps->MaxAudioChannels   = 0xFFFFFFFF;

                        caps->EffectFlags        = 0;

                        CMiniportDMusUART *aMiniport;
                        ASSERT(pRequest->MajorTarget);
                        aMiniport = (CMiniportDMusUART *)(PMINIPORTDMUS)(pRequest->MajorTarget);
                        WCHAR wszDesc2[16];
                        int cLen;
                        cLen = swprintf(wszDesc2,L"[%03x]\0",PtrToUlong(aMiniport->m_pPortBase));

                        cLen *= sizeof(WCHAR);
                        RtlCopyMemory((WCHAR *)((DWORD_PTR)(caps->Description) + increment),
                                       wszDesc2,
                                       cLen);


                        pRequest->ValueSize = sizeof(SYNTHCAPS);
                    }
                }

                break;

             case KSPROPERTY_SYNTH_PORTPARAMETERS:
                DPRINT("PropertyHandler_Synth:KSPROPERTY_SYNTH_PORTPARAMETERS");
    {
                CMiniportDMusUARTStream *aStream;

                aStream = (CMiniportDMusUARTStream*)(pRequest->MinorTarget);
                if (aStream)
                {
                    ntStatus = aStream->HandlePortParams(pRequest);
                }
                else
                {
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
               }
               break;

            case KSPROPERTY_SYNTH_CHANNELGROUPS:
                DPRINT("PropertyHandler_Synth:KSPROPERTY_SYNTH_CHANNELGROUPS");

                ntStatus = ValidatePropertyRequest(pRequest, sizeof(ULONG), TRUE);
                if (NT_SUCCESS(ntStatus))
                {
                    *(PULONG)(pRequest->Value) = 1;
                    pRequest->ValueSize = sizeof(ULONG);
                }
                break;

            case KSPROPERTY_SYNTH_LATENCYCLOCK:
                DPRINT("PropertyHandler_Synth:KSPROPERTY_SYNTH_LATENCYCLOCK");

                if(pRequest->Verb & KSPROPERTY_TYPE_SET)
                {
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
                else
                {
                    ntStatus = ValidatePropertyRequest(pRequest, sizeof(ULONGLONG), TRUE);
                    if(NT_SUCCESS(ntStatus))
                    {
                        REFERENCE_TIME rtLatency;
                        CMiniportDMusUARTStream *aStream;

                        aStream = (CMiniportDMusUARTStream*)(pRequest->MinorTarget);
                        if(aStream == NULL)
                        {
                            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                        }
                        else
                        {
                            aStream->m_pMiniport->m_MasterClock->GetTime(&rtLatency);
                            *((PULONGLONG)pRequest->Value) = rtLatency;
                            pRequest->ValueSize = sizeof(ULONGLONG);
                        }
                    }
                }
                break;

            default:
                DPRINT("Unhandled property in PropertyHandler_Synth");
                break;
        }
    }
    return ntStatus;
}

/*****************************************************************************
 * ValidatePropertyRequest()
 *****************************************************************************
 * Validates pRequest.
 *  Checks if the ValueSize is valid
 *  Checks if the Value is valid
 *
 *  This does not update pRequest->ValueSize if it returns NT_SUCCESS.
 *  Caller must set pRequest->ValueSize in case of NT_SUCCESS.
 */
NTSTATUS ValidatePropertyRequest
(
    IN      PPCPROPERTY_REQUEST     pRequest,
    IN      ULONG                   ulValueSize,
    IN      BOOLEAN                 fValueRequired
)
{
    NTSTATUS    ntStatus;

    if (pRequest->ValueSize >= ulValueSize)
    {
        if (fValueRequired && NULL == pRequest->Value)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        else
        {
            ntStatus = STATUS_SUCCESS;
        }
    }
    else  if (0 == pRequest->ValueSize)
    {
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    if (STATUS_BUFFER_OVERFLOW == ntStatus)
    {
        pRequest->ValueSize = ulValueSize;
    }
    else
    {
        pRequest->ValueSize = 0;
    }

    return ntStatus;
} // ValidatePropertyRequest

#ifdef _MSC_VER
#pragma code_seg()
#endif


