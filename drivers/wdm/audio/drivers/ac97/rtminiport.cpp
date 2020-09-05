/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file rtminiport.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
static char STR_MODULENAME[] = "AC97 RT Miniport: ";

#include "rtminiport.h"
#include "rtstream.h"

#if (NTDDI_VERSION >= NTDDI_VISTA)

/*****************************************************************************
 * PinDataRangesPCMStream
 *****************************************************************************
 * The next 3 arrays contain information about the data ranges of the pin for
 * wave capture, wave render and mic capture.
 * These arrays are filled dynamically by BuildDataRangeInformation().
 */

static KSDATARANGE_AUDIO PinDataRangesPCMStreamRender[WAVE_SAMPLERATES_TESTED];
static KSDATARANGE_AUDIO PinDataRangesPCMStreamCapture[WAVE_SAMPLERATES_TESTED];
static KSDATARANGE_AUDIO PinDataRangesMicStream[MIC_SAMPLERATES_TESTED];

static KSDATARANGE PinDataRangesAnalogBridge[] =
{
   {
      sizeof(KSDATARANGE),
      0,
      0,
      0,
      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

/*****************************************************************************
 * PinDataRangesPointersPCMStream
 *****************************************************************************
 * The next 3 arrays contain the pointers to the data range information of
 * the pin for wave capture, wave render and mic capture.
 * These arrays are filled dynamically by BuildDataRangeInformation().
 */
static PKSDATARANGE PinDataRangePointersPCMStreamRender[WAVE_SAMPLERATES_TESTED];
static PKSDATARANGE PinDataRangePointersPCMStreamCapture[WAVE_SAMPLERATES_TESTED];
static PKSDATARANGE PinDataRangePointersMicStream[MIC_SAMPLERATES_TESTED];

/*****************************************************************************
 * PinDataRangePointerAnalogStream
 *****************************************************************************
 * This structure pointers to the data range structures for the wave pins.
 */
static PKSDATARANGE PinDataRangePointersAnalogBridge[] =
{
    (PKSDATARANGE) PinDataRangesAnalogBridge
};


/*****************************************************************************
 * Miniport Topology
 *==================
 *
 *                              +-----------+
 *                              |           |
 *    Capture (PIN_WAVEIN)  <---|2 --ADC-- 3|<=== (PIN_WAVEIN_BRIDGE)
 *                              |           |
 *     Render (PIN_WAVEOUT) --->|0 --DAC-- 1|===> (PIN_WAVEOUT_BRIDGE)
 *                              |           |
 *        Mic (PIN_MICIN)   <---|4 --ADC-- 5|<=== (PIN_MICIN_BRIDGE)
 *                              +-----------+
 *
 * Note that the exposed pins (left side) have to be a multiple of 2
 * since there are some dependencies in the stream object.
 */

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * This structure describes pin (stream) types provided by this miniport.
 * The field that sets the number of data range entries (SIZEOF_ARRAY) is
 * overwritten by BuildDataRangeInformation().
 */
static PCPIN_DESCRIPTOR MiniportPins[] =
{
    // PIN_WAVEOUT
    {
        1,1,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersPCMStreamRender),  // DataRangesCount
            PinDataRangePointersPCMStreamRender,                // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // PIN_WAVEOUT_BRIDGE
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersAnalogBridge),    // DataRangesCount
            PinDataRangePointersAnalogBridge,                  // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // PIN_WAVEIN
    {
        1,1,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersPCMStreamCapture), // DataRangesCount
            PinDataRangePointersPCMStreamCapture,               // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *) &PINNAME_CAPTURE,                  // Category
            &KSAUDFNAME_RECORDING_CONTROL,              // Name
            0                                           // Reserved
        }
    },

    // PIN_WAVEIN_BRIDGE
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersAnalogBridge),    // DataRangesCount
            PinDataRangePointersAnalogBridge,                  // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    //
    // The Microphone pins are not used if PINC_MICIN_PRESENT is not set.
    // To remove them, Init() will reduce the "PinCount" in the
    // MiniportFilterDescriptor.
    //
    // PIN_MICIN
    {
        1,1,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersMicStream),// DataRangesCount
            PinDataRangePointersMicStream,              // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // PIN_MICIN_BRIDGE
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersAnalogBridge),    // DataRangesCount
            PinDataRangePointersAnalogBridge,                  // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

/*****************************************************************************
 * PropertiesDAC
 *****************************************************************************
 * Properties for the DAC node.
 */
static PCPROPERTY_ITEM PropertiesDAC[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CHANNEL_CONFIG,
        KSPROPERTY_TYPE_SET,
        CAC97MiniportWaveRT::PropertyChannelConfig
    }
};

/*****************************************************************************
 * AutomationVolume
 *****************************************************************************
 * Automation table for volume controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP (AutomationDAC, PropertiesDAC);

/*****************************************************************************
 * TopologyNodes
 *****************************************************************************
 * List of nodes.
 */
static PCNODE_DESCRIPTOR MiniportNodes[] =
{
    // NODE_WAVEOUT_DAC
    {
        0,                      // Flags
        &AutomationDAC,         // AutomationTable
        &KSNODETYPE_DAC,        // Type
        NULL                    // Name
    },
    // NODE_WAVEIN_ADC
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_ADC,        // Type
        NULL                    // Name
    },
    //
    // The Microphone node is not used if PINC_MICIN_PRESENT is not set.
    // To remove them, Init() will reduce the "NodeCount" in the
    // MiniportFilterDescriptor.
    //
    //  NODE_MICIN_ADC
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_ADC,        // Type
        NULL                    // Name
    }
};

/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * This structure identifies the connections between filter pins and
 * node pins.
 */
static PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    //from_node             from_pin            to_node             to_pin
    { PCFILTER_NODE,        PIN_WAVEOUT,        NODE_WAVEOUT_DAC,   1},
    { NODE_WAVEOUT_DAC,     0,                  PCFILTER_NODE,      PIN_WAVEOUT_BRIDGE},
    { PCFILTER_NODE,        PIN_WAVEIN_BRIDGE,  NODE_WAVEIN_ADC,    1},
    { NODE_WAVEIN_ADC,      0,                  PCFILTER_NODE,      PIN_WAVEIN},
    //
    // The Microphone connection is not used if PINC_MICIN_PRESENT is not set.
    // To remove them, Init() will reduce the "ConnectionCount" in the
    // MiniportFilterDescriptor.
    //
    { PCFILTER_NODE,        PIN_MICIN_BRIDGE,   NODE_MICIN_ADC,     1},
    { NODE_MICIN_ADC,       0,                  PCFILTER_NODE,      PIN_MICIN}
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport description.
 * Init() modifies the pin count, node count and connection count in absence
 * of the MicIn recording line.
 */
static PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
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
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use defaults (audio, render, capture)
};

#pragma code_seg("PAGE")
/*****************************************************************************
 * CAC97MiniportWaveRT::PropertyChannelConfig
 *****************************************************************************
 * This is the property handler for KSPROPERTY_AUDIO_CHANNEL_CONFIG of the
 * DAC node. It sets the channel configuration (how many channels, how user
 * was setting up the speakers).
 */
NTSTATUS CAC97MiniportWaveRT::PropertyChannelConfig
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::PropertyChannelConfig]"));

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    // The major target is the object pointer to the wave miniport.
    CAC97MiniportWaveRT *that =
        (CAC97MiniportWaveRT *) (PMINIPORTWAVERT)PropertyRequest->MajorTarget;

    ASSERT (that);

    // We only have a set defined.
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        // validate buffer size.
        if (PropertyRequest->ValueSize < sizeof(LONG))
            return ntStatus;

        // The "Value" is the input buffer with the channel config.
        if (PropertyRequest->Value)
        {
            // We can accept different channel configurations, depending
            // on the number of channels we can play.
            if (that->AdapterCommon->GetPinConfig (PINC_SURROUND_PRESENT))
            {
                if (that->AdapterCommon->GetPinConfig (PINC_CENTER_LFE_PRESENT))
                {
                    // we accept 5.1
                    if (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_5POINT1)
                    {
                        that->m_dwChannelMask =  *(PLONG)PropertyRequest->Value;
                        that->m_wChannels = 6;
                        that->AdapterCommon->WriteChannelConfigDefault (that->m_dwChannelMask);
                        ntStatus = STATUS_SUCCESS;
                    }
                }

                // accept also surround or quad.
                if ((*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_QUAD) ||
                    (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_SURROUND))
                {
                    that->m_dwChannelMask =  *(PLONG)PropertyRequest->Value;
                    that->m_wChannels = 4;
                    that->AdapterCommon->WriteChannelConfigDefault (that->m_dwChannelMask);
                    ntStatus = STATUS_SUCCESS;
                }
            }

            // accept also stereo speakers.
            if (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_STEREO)
            {
                that->m_dwChannelMask =  *(PLONG)PropertyRequest->Value;
                that->m_wChannels = 2;
                that->AdapterCommon->WriteChannelConfigDefault (that->m_dwChannelMask);
                ntStatus = STATUS_SUCCESS;
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CreateAC97MiniportWaveRT
 *****************************************************************************
 * Creates a RT miniport object for the AC97 adapter.
 * This uses a macro from STDUNK.H to do all the work.
 */
NTSTATUS CreateAC97MiniportWaveRT
(
    OUT PUNKNOWN   *Unknown,
    IN  REFCLSID,
    IN  PUNKNOWN    UnknownOuter    OPTIONAL,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN  POOL_TYPE   PoolType
)
{
    PAGED_CODE ();

    ASSERT (Unknown);

    DOUT (DBG_PRINT, ("[CreateMiniportWaveRT]"));

    STD_CREATE_BODY_WITH_TAG_(CAC97MiniportWaveRT,Unknown,UnknownOuter,PoolType,
                     PoolTag, PMINIPORTWAVERT);
}


/*****************************************************************************
 * CAC97MiniportWaveRT::NonDelegatingQueryInterface
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::NonDelegatingQueryInterface
(
    _In_         REFIID  Interface,
    _COM_Outptr_ PVOID  *Object
)
{
    PAGED_CODE ();

    ASSERT (Object);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::NonDelegatingQueryInterface]"));

    // Is it IID_IUnknown?
    if (IsEqualGUIDAligned (Interface, IID_IUnknown))
    {
        *Object = (PVOID)(PUNKNOWN)(PMINIPORTWAVERT)this;
    }
    // or IID_IMiniport ...
    else if (IsEqualGUIDAligned (Interface, IID_IMiniport))
    {
        *Object = (PVOID)(PMINIPORT)this;
    }
    // or IID_IMiniportWaveRT ...
    else if (IsEqualGUIDAligned (Interface, IID_IMiniportWaveRT))
    {
        *Object = (PVOID)(PMINIPORTWAVERT)this;
    }
    // or IID_IPowerNotify ...
    else if (IsEqualGUIDAligned (Interface, IID_IPowerNotify))
    {
        *Object = (PVOID)(PPOWERNOTIFY)this;
    }
    else
    {
        // nothing found, must be an unknown interface.
        *Object = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We reference the interface for the caller.
    //
    ((PUNKNOWN)(*Object))->AddRef();
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::~CAC97MiniportWaveRT
 *****************************************************************************
 * Destructor.
 */
CAC97MiniportWaveRT::~CAC97MiniportWaveRT ()
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::~CAC97MiniportWaveRT]"));

    //
    // Release the interrupt sync.
    //
    if (InterruptSync)
    {
        InterruptSync->Release ();
        InterruptSync = NULL;
    }

    //
    // Release adapter common object.
    //
    if (AdapterCommon)
    {
        AdapterCommon->Release ();
        AdapterCommon = NULL;
    }

    //
    // Release the port.
    //
    if (Port)
    {
        Port->Release ();
        Port = NULL;
    }
}


/*****************************************************************************
 * CAC97MiniportWaveRT::Init
 *****************************************************************************
 * Initializes the miniport.
 * Initializes variables and modifies the wave topology if needed.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::Init
(
    _In_  PUNKNOWN      UnknownAdapter,
    _In_  PRESOURCELIST ResourceList,
    _In_  PPORTWAVERT   Port_
)
{
    PAGED_CODE ();

    ASSERT (UnknownAdapter);
    ASSERT (ResourceList);
    ASSERT (Port_);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::Init]"));

    //
    // AddRef() is required because we are keeping this pointer.
    //
    Port = Port_;
    Port->AddRef ();

    //
    // Set initial device power state
    //
    m_PowerState = PowerDeviceD0;

    NTSTATUS ntStatus = UnknownAdapter->
        QueryInterface (IID_IAC97AdapterCommon, (PVOID *)&AdapterCommon);
    if (NT_SUCCESS (ntStatus))
    {
        //
        // Alter the topology for the wave miniport.
        //
        if (!(AdapterCommon->GetPinConfig (PINC_MICIN_PRESENT) &&
              AdapterCommon->GetPinConfig (PINC_MIC_PRESENT)))
        {
            //
            // Remove the pins, nodes and connections for the MICIN.
            //
            MiniportFilterDescriptor.PinCount = SIZEOF_ARRAY(MiniportPins) - 2;
            MiniportFilterDescriptor.NodeCount = SIZEOF_ARRAY(MiniportNodes) - 1;
            MiniportFilterDescriptor.ConnectionCount = SIZEOF_ARRAY(MiniportConnections) - 2;
        }

        //
        // Process the resources.
        //
        ntStatus = ProcessResources (ResourceList);

        //
        // Get the default channel config
        //
        AdapterCommon->ReadChannelConfigDefault (&m_dwChannelMask, &m_wChannels);

        //
        // If we came till that point, check the CoDec for supported standard
        // sample rates. This function will then fill the data range information
        //
        if (NT_SUCCESS (ntStatus))
            ntStatus = BuildDataRangeInformation ();
    }

    //
    // If we fail we get destroyed anyway (that's where we clean up).
    //
    return ntStatus;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::ProcessResources
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 * Sets up the Interrupt + Service routine and DMA.
 */
NTSTATUS CAC97MiniportWaveRT::ProcessResources
(
    IN  PRESOURCELIST ResourceList
)
{
    PAGED_CODE ();

    ASSERT (ResourceList);


    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::ProcessResources]"));


    ULONG countIRQ = ResourceList->NumberOfInterrupts ();
    if (countIRQ < 1)
    {
        DOUT (DBG_ERROR, ("Unknown configuration for wave miniport!"));
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // Create an interrupt sync object
    //
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ntStatus = PcNewInterruptSync (&InterruptSync,
                                   NULL,
                                   ResourceList,
                                   0,
                                   InterruptSyncModeNormal);

    if (!NT_SUCCESS (ntStatus) || !InterruptSync)
    {
        DOUT (DBG_ERROR, ("Failed to create an interrupt sync!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Register our ISR.
    //
    ntStatus = InterruptSync->RegisterServiceRoutine (InterruptServiceRoutine,
                                                      (PVOID)this, FALSE);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Failed to register ISR!"));
        return ntStatus;
    }

    //
    // Connect the interrupt.
    //
    ntStatus = InterruptSync->Connect ();
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Failed to connect the ISR with InterruptSync!"));
        return ntStatus;
    }

    //
    // On failure object is destroyed which cleans up.
    //
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::BuildDataRangeInformation
 *****************************************************************************
 * This function dynamically build the data range information for the pins.
 * It also connects the static arrays with the data range information
 * structure.
 * If this function returns with an error the miniport should be destroyed.
 *
 * To build the data range information, we test the most popular sample rates,
 * the functions calls ProgramSampleRate in AdapterCommon object to actually
 * program the sample rate. After probing that way for multiple sample rates,
 * the original value, which is 48KHz, gets restored.
 * We have to test the sample rates for playback, capture and microphone
 * separately. Every time we succeed, we update the data range information and
 * the pointers that point to it.
 */
NTSTATUS CAC97MiniportWaveRT::BuildDataRangeInformation (void)
{
    PAGED_CODE ();

    NTSTATUS    ntStatus;
    int nWavePlaybackEntries = 0;
    int nWaveRecordingEntries = 0;
    int nMicEntries = 0;
    int nChannels;
    int nLoop;

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::BuildDataRangeInformation]"));

    //
    // Calculate the number of max. channels available in the codec.
    //
    if (AdapterCommon->GetPinConfig (PINC_SURROUND_PRESENT))
    {
        if (AdapterCommon->GetPinConfig (PINC_CENTER_LFE_PRESENT))
        {
            nChannels = 6;
        }
        else
        {
            nChannels = 4;
        }
    }
    else
    {
        nChannels = 2;
    }

    // Check for the render sample rates.
    for (nLoop = 0; nLoop < WAVE_SAMPLERATES_TESTED; nLoop++)
    {
        ntStatus = AdapterCommon->ProgramSampleRate (AC97REG_FRONT_SAMPLERATE,
                                                     dwWaveSampleRates[nLoop]);

        // We support the sample rate?
        if (NT_SUCCESS (ntStatus))
        {
            // Add it to the PinDataRange
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.Flags      = 0;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.SampleSize = nChannels * 2;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.Reserved   = 0;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].DataRange.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].MaximumChannels = nChannels;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].MinimumBitsPerSample = 16;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].MaximumBitsPerSample = 16;
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].MinimumSampleFrequency = dwWaveSampleRates[nLoop];
            PinDataRangesPCMStreamRender[nWavePlaybackEntries].MaximumSampleFrequency = dwWaveSampleRates[nLoop];

            // Add it to the PinDataRangePointer
            PinDataRangePointersPCMStreamRender[nWavePlaybackEntries] = (PKSDATARANGE)&PinDataRangesPCMStreamRender[nWavePlaybackEntries];

            // Increase count
            nWavePlaybackEntries++;
        }
    }

    // Check for the capture sample rates.
    for (nLoop = 0; nLoop < WAVE_SAMPLERATES_TESTED; nLoop++)
    {
        ntStatus = AdapterCommon->ProgramSampleRate (AC97REG_RECORD_SAMPLERATE, dwWaveSampleRates[nLoop]);

        // We support the sample rate?
        if (NT_SUCCESS (ntStatus))
        {
            // Add it to the PinDataRange
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.Flags      = 0;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.SampleSize = 4;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.Reserved   = 0;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].DataRange.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].MaximumChannels = 2;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].MinimumBitsPerSample = 16;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].MaximumBitsPerSample = 16;
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].MinimumSampleFrequency = dwWaveSampleRates[nLoop];
            PinDataRangesPCMStreamCapture[nWaveRecordingEntries].MaximumSampleFrequency = dwWaveSampleRates[nLoop];

            // Add it to the PinDataRangePointer
            PinDataRangePointersPCMStreamCapture[nWaveRecordingEntries] = (PKSDATARANGE)&PinDataRangesPCMStreamCapture[nWaveRecordingEntries];

            // Increase count
            nWaveRecordingEntries++;
        }
    }

    // Check for the MIC sample rates.
    for (nLoop = 0; nLoop < MIC_SAMPLERATES_TESTED; nLoop++)
    {
        ntStatus = AdapterCommon->ProgramSampleRate (AC97REG_MIC_SAMPLERATE, dwMicSampleRates[nLoop]);

        // We support the sample rate?
        if (NT_SUCCESS (ntStatus))
        {
            // Add it to the PinDataRange
            PinDataRangesMicStream[nMicEntries].DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
            PinDataRangesMicStream[nMicEntries].DataRange.Flags      = 0;
            PinDataRangesMicStream[nMicEntries].DataRange.SampleSize = 2;
            PinDataRangesMicStream[nMicEntries].DataRange.Reserved   = 0;
            PinDataRangesMicStream[nMicEntries].DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            PinDataRangesMicStream[nMicEntries].DataRange.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
            PinDataRangesMicStream[nMicEntries].DataRange.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            PinDataRangesMicStream[nMicEntries].MaximumChannels = 1;
            PinDataRangesMicStream[nMicEntries].MinimumBitsPerSample = 16;
            PinDataRangesMicStream[nMicEntries].MaximumBitsPerSample = 16;
            PinDataRangesMicStream[nMicEntries].MinimumSampleFrequency = dwMicSampleRates[nLoop];
            PinDataRangesMicStream[nMicEntries].MaximumSampleFrequency = dwMicSampleRates[nLoop];

            // Add it to the PinDataRangePointer
            PinDataRangePointersMicStream[nMicEntries] = (PKSDATARANGE)&PinDataRangesMicStream[nMicEntries];

            // Increase count
            nMicEntries++;
        }
    }

    // Now go through the pin descriptor list and change the data range entries to the actual number.
    for (nLoop = 0; nLoop < SIZEOF_ARRAY(MiniportPins); nLoop++)
    {
        if (MiniportPins[nLoop].KsPinDescriptor.DataRanges == PinDataRangePointersPCMStreamRender)
            MiniportPins[nLoop].KsPinDescriptor.DataRangesCount = nWavePlaybackEntries;
        if (MiniportPins[nLoop].KsPinDescriptor.DataRanges == PinDataRangePointersPCMStreamCapture)
            MiniportPins[nLoop].KsPinDescriptor.DataRangesCount = nWaveRecordingEntries;
        if (MiniportPins[nLoop].KsPinDescriptor.DataRanges == PinDataRangePointersMicStream)
            MiniportPins[nLoop].KsPinDescriptor.DataRangesCount = nMicEntries;
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::NewStream
 *****************************************************************************
 * Creates a new stream.
 * This function is called when a streaming pin is created.
 * It checks if the channel is already in use, tests the data format, creates
 * and initializes the stream object.
 */
STDMETHODIMP CAC97MiniportWaveRT::NewStream
(
    _Out_ PMINIPORTWAVERTSTREAM       *Stream,
    _In_  PPORTWAVERTSTREAM           PortStream,
    _In_  ULONG                   Channel_,
    _In_  BOOLEAN                 Capture,
    _In_  PKSDATAFORMAT           DataFormat
)
{
    PAGED_CODE ();

    ASSERT (Stream);
    ASSERT (PortStream);
    ASSERT (DataFormat);

    CAC97MiniportWaveRTStream *pStream = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::NewStream]"));

    //
    // Validate the channel (pin id).
    //
    if ((Channel_ != PIN_WAVEOUT) && (Channel_ != PIN_WAVEIN) &&
       (Channel_ != PIN_MICIN))
    {
        DOUT (DBG_ERROR, ("[NewStream] Invalid channel passed!"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if the pin is already in use
    //
    ULONG Channel = Channel_ >> 1;
    if (Streams[Channel])
    {
        DOUT (DBG_ERROR, ("[NewStream] Pin is already in use!"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check parameters.
    //
    ntStatus = TestDataFormat (DataFormat, (WavePins)Channel_);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_VSR, ("[NewStream] TestDataFormat failed!"));
        return ntStatus;
    }

    //
    // Create a new stream.
    //
    ntStatus = CreateAC97MiniportWaveRTStream (&pStream);

    //
    // Return in case of an error.
    //
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("[NewStream] Failed to create stream!"));
        return ntStatus;
    }

    //
    // Initialize the stream.
    //
    ntStatus = pStream->Init (this,
                              PortStream,
                              Channel,
                              Capture,
                              DataFormat);
    if (!NT_SUCCESS (ntStatus))
    {
        //
        // Release the stream and clean up.
        //
        DOUT (DBG_ERROR, ("[NewStream] Failed to init stream!"));
        pStream->Release ();
        *Stream = NULL;
        return ntStatus;
    }

    //
    // Save the pointers.
    //
    *Stream = (PMINIPORTWAVERTSTREAM)pStream;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::GetDescription
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::GetDescription
(
    _Out_ PPCFILTER_DESCRIPTOR *OutFilterDescriptor
)
{
    PAGED_CODE ();

    ASSERT (OutFilterDescriptor);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::GetDescription]"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;


    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::GetDeviceDescription
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::GetDeviceDescription
(
    _Out_ PDEVICE_DESCRIPTION   DmaDeviceDescription
)
{
    PAGED_CODE ();

    ASSERT (DmaDeviceDescription);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::GetDeviceDescription]"));

    RtlZeroMemory (DmaDeviceDescription, sizeof (DEVICE_DESCRIPTION));
    DmaDeviceDescription->Master = TRUE;
    DmaDeviceDescription->ScatterGather = TRUE;
    DmaDeviceDescription->Dma32BitAddresses = TRUE;
    DmaDeviceDescription->InterfaceType = PCIBus;
    DmaDeviceDescription->MaximumLength = 0x1FFFE;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::DataRangeIntersection
 *****************************************************************************
 * Tests a data range intersection.
 * Cause the AC97 controller does not support mono render or capture, we have
 * to check the max. channel field (unfortunately, there is no MinimumChannel
 * and MaximumChannel field, just a MaximumChannel field).
 * If the MaximumChannel is 2, then we can pass this to the default handler of
 * portcls which always chooses the most (SampleFrequency, Channel, Bits etc.)
 *
 * This DataRangeIntersection function is strictly only for the exposed formats
 * in this sample driver. If you intend to add other formats like AC3 then
 * you have to be make sure that you check the GUIDs and the data range, since
 * portcls only checks the data range for waveformatex.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::DataRangeIntersection
(
    _In_      ULONG           PinId,
    _In_      PKSDATARANGE    ClientsDataRange,
    _In_      PKSDATARANGE    MyDataRange,
    _In_      ULONG           OutputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultantFormatLength)
              PVOID           ResultantFormat,
    _Out_     PULONG          ResultantFormatLength
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::DataRangeIntersection]"));

    //
    // This function gets only called if the GUIDS in the KSDATARANGE_AUDIO
    // structure that we attached to the pin are equal with the requested
    // format (see "BuildDataRangeInformation).
    // Additionally, for waveformatex portcls checks that the requested sample
    // frequency range fits into our exposed sample frequency range. Since we
    // only have discrete sample frequencies in the pin's data range, we don't
    // have to check that either.
    // There is one exception to this rule: portcls clones all WAVEFORMATEX
    // data ranges to DSOUND dataranges, so we might get a data range
    // intersection that has a DSOUND specifier. We don't support that
    // since this is only used for HW acceleration
    //
    if (IsEqualGUIDAligned (ClientsDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND))
    {
        DOUT (DBG_PRINT, ("[DataRangeIntersection] We don't support DSOUND specifier"));
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Start with checking the size of the output buffer and validating that it is not NULL.
    //
    if (!OutputBufferLength || NULL == ResultantFormat)
    {
        *ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (OutputBufferLength < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX)))
    {
        DOUT (DBG_WARNING, ("[DataRangeIntersection] Buffer too small"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // We can only play or record multichannel (>=2 channels) except for the MIC
    // recording channel where we can only record mono. Portcls checked the channels
    // already, however, since we have no minimum channels field, the KSDATARANGE_AUDIO
    // could have MaximumChannels = 1.
    //
    if (PinId != PIN_MICIN)
    {
        // reject mono format for normal wave playback or capture.
        if (((PKSDATARANGE_AUDIO)ClientsDataRange)->MaximumChannels < 2)
        {
            DOUT (DBG_WARNING, ("[DataRangeIntersection] Mono requested for WaveIn or WaveOut"));
            return STATUS_NO_MATCH;
        }
    }

    //
    // Fill in the structure the datarange structure.
    // KSDATARANGE and KSDATAFORMAT are the same.
    //
    *(PKSDATAFORMAT)ResultantFormat = *MyDataRange;

    //
    // Modify the size of the data format structure to fit the WAVEFORMATPCMEX
    // structure.
    //
    ((PKSDATAFORMAT)ResultantFormat)->FormatSize =
        sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);

    //
    // Append the WAVEFORMATPCMEX structur.
    //
    PWAVEFORMATPCMEX WaveFormat = (PWAVEFORMATPCMEX)((PKSDATAFORMAT)ResultantFormat + 1);

    // We want a WAFEFORMATEXTENSIBLE which is equal to WAVEFORMATPCMEX.
    WaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    // Set the number of channels
    if (PinId == PIN_WAVEOUT)
    {
        // Get the max. possible channels for playback.
        ULONG nMaxChannels = min (((PKSDATARANGE_AUDIO)ClientsDataRange)->MaximumChannels, m_wChannels);

        // We cannot play uneven number of channels
        if (nMaxChannels & 0x01)
            nMaxChannels--;
        // ... and also 0 channels wouldn't be a good request.
        if (!nMaxChannels)
            return STATUS_NO_MATCH;

        WaveFormat->Format.nChannels = (WORD)nMaxChannels;
    }
    else
        // This will be 2 for normal record and 1 for MIC record.
        WaveFormat->Format.nChannels = (WORD)((PKSDATARANGE_AUDIO)MyDataRange)->MaximumChannels;

    //
    // Hack for codecs that have only one sample rate converter that has both
    // playback and recording data.
    //
    if ((Streams[PIN_WAVEIN_OFFSET] || Streams[PIN_WAVEOUT_OFFSET]) &&
        !AdapterCommon->GetNodeConfig (NODEC_PCM_VSR_INDEPENDENT_RATES))
    {
        //
        // We have to return this sample rate that is used in the open stream.
        //
        ULONG   ulFrequency;

        if (Streams[PIN_WAVEIN_OFFSET])
            ulFrequency = Streams[PIN_WAVEIN_OFFSET]->GetCurrentSampleRate();
        else
            ulFrequency = Streams[PIN_WAVEOUT_OFFSET]->GetCurrentSampleRate();

        //
        // Check if this sample rate is in the requested data range of the client.
        //
        if ((((PKSDATARANGE_AUDIO)ClientsDataRange)->MaximumSampleFrequency < ulFrequency) ||
            (((PKSDATARANGE_AUDIO)ClientsDataRange)->MinimumSampleFrequency > ulFrequency))
        {
            return STATUS_NO_MATCH;
        }

        WaveFormat->Format.nSamplesPerSec = ulFrequency;
    }
    else
    {
        // Since we have discrete frequencies in the data range, min = max.
        WaveFormat->Format.nSamplesPerSec = ((PKSDATARANGE_AUDIO)MyDataRange)->MaximumSampleFrequency;
    }

    // Will be 16.
    WaveFormat->Format.wBitsPerSample = (WORD)((PKSDATARANGE_AUDIO)MyDataRange)->MaximumBitsPerSample;
    // Will be 2 * channels.
    WaveFormat->Format.nBlockAlign = (WaveFormat->Format.wBitsPerSample * WaveFormat->Format.nChannels) / 8;
    // That is played in a sec.
    WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
    // WAVEFORMATPCMEX
    WaveFormat->Format.cbSize = 22;
    // We have as many valid bits as the bit depth is (16).
    WaveFormat->Samples.wValidBitsPerSample = WaveFormat->Format.wBitsPerSample;
    // Set the channel mask
    if (PinId == PIN_WAVEOUT)
    {
        // If we can play in our configuration, then set the channel mask
        if (WaveFormat->Format.nChannels == m_wChannels)
            // Set the playback channel mask to the current speaker config.
            WaveFormat->dwChannelMask = m_dwChannelMask;
        else
        {
            //
            // We have to set a channel mask.
            // nChannles can only be 4 if we are in 6 channel mode. In that
            // case it must be a QUAD configurations. The only other value
            // allowed is 2 channels, which defaults to stereo.
            //
            if (WaveFormat->Format.nChannels == 4)
                WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_QUAD;
            else
                WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        }
    }
    else
    {
        // This will be KSAUDIO_SPEAKER_STEREO for normal record and KSAUDIO_SPEAKER_MONO
        // for MIC record.
        if (PinId == PIN_MICIN)
            // MicIn -> 1 channel
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_MONO;
        else
            // normal record -> 2 channels
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    }
    // Here we specify the subtype of the WAVEFORMATEXTENSIBLE.
    WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    // Now overwrite also the sample size in the ksdataformat structure.
    ((PKSDATAFORMAT)ResultantFormat)->SampleSize = WaveFormat->Format.nBlockAlign;

    //
    // That we will return.
    //
    *ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);

    DOUT (DBG_STREAM, ("[DataRangeIntersection] Frequency: %d, Channels: %d, bps: %d, ChannelMask: %X",
          WaveFormat->Format.nSamplesPerSec, WaveFormat->Format.nChannels,
          WaveFormat->Format.wBitsPerSample, WaveFormat->dwChannelMask));

    // Let portcls do some work ...
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::TestDataFormat
 *****************************************************************************
 * Checks if the passed data format is known to the driver and verifies that
 * the number of channels, the width of one sample match to the AC97
 * specification.
 */
NTSTATUS CAC97MiniportWaveRT::TestDataFormat
(
    IN  PKSDATAFORMAT Format,
    IN  WavePins      Pin
)
{
    PAGED_CODE ();

    ASSERT (Format);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::TestDataFormat]"));

    //
    // KSDATAFORMAT contains three GUIDs to support extensible format.  The
    // first two GUIDs identify the type of data.  The third indicates the
    // type of specifier used to indicate format specifics.  We are only
    // supporting PCM audio formats that use WAVEFORMATEX.
    //
    if (!IsEqualGUIDAligned (Format->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) ||
        !IsEqualGUIDAligned (Format->SubFormat, KSDATAFORMAT_SUBTYPE_PCM)  ||
        !IsEqualGUIDAligned (Format->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        DOUT (DBG_ERROR, ("[TestDataFormat] Invalid format type!"));
        return STATUS_INVALID_PARAMETER;
    }

    PWAVEFORMATPCMEX waveFormat = (PWAVEFORMATPCMEX)(Format + 1);

    //
    // If the size doesn't match, then something is messed up.
    //
    if (Format->FormatSize < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX)))
    {
        DOUT (DBG_WARNING, ("[TestDataFormat] Invalid FormatSize!"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We only support PCM, 16-bit.
    //
    if (waveFormat->Format.wBitsPerSample != 16)
    {
        DOUT (DBG_WARNING, ("[TestDataFormat] Bits Per Sample must be 16!"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We support WaveFormatPCMEX (=WAVEFORMATEXTENSIBLE) or WaveFormatPCM.
    //
    if ((waveFormat->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) &&
        (waveFormat->Format.wFormatTag != WAVE_FORMAT_PCM))
    {
        DOUT (DBG_WARNING, ("[TestDataFormat] Invalid Format Tag!"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make additional checks for the WAVEFORMATEXTENSIBLE
    //
    if (waveFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        //
        // If the size doesn't match, then something is messed up.
        //
        if (Format->FormatSize < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX)))
        {
            DOUT (DBG_WARNING, ("[TestDataFormat] Invalid FormatSize!"));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Check also the subtype (PCM) and the size of the extended data.
        //
        if (!IsEqualGUIDAligned (waveFormat->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
            (waveFormat->Format.cbSize < 22))
        {
            DOUT (DBG_WARNING, ("[TestDataFormat] Unsupported WAVEFORMATEXTENSIBLE!"));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Check the channel mask. We support 1, 2 channels or whatever was set
        // with the Speaker config dialog.
        //
        if (((waveFormat->Format.nChannels == 1) &&
             (waveFormat->dwChannelMask != KSAUDIO_SPEAKER_MONO)) ||
            ((waveFormat->Format.nChannels == 2) &&
             (waveFormat->dwChannelMask != KSAUDIO_SPEAKER_STEREO)) ||
            ((waveFormat->Format.nChannels == m_wChannels) &&
             (waveFormat->dwChannelMask != m_dwChannelMask)))
        {
            DOUT (DBG_WARNING, ("[TestDataFormat] Channel Mask!"));
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Check the number of channels.
    //
    switch (Pin)
    {
        case PIN_MICIN:     // 1 channel
            if (waveFormat->Format.nChannels != 1)
            {
                DOUT (DBG_WARNING, ("[TestDataFormat] Invalid Number of Channels for PIN_MICIN!"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        case PIN_WAVEIN:    // 2 channels
            if (waveFormat->Format.nChannels != 2)
            {
                DOUT (DBG_WARNING, ("[TestDataFormat] Invalid Number of Channels for PIN_WAVEIN!"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        case PIN_WAVEOUT:   // channel and mask from PropertyChannelConfig or standard.
            if (waveFormat->Format.nChannels != m_wChannels)
            {
                DOUT (DBG_WARNING, ("[TestDataFormat] Invalid Number of Channels for PIN_WAVEOUT!"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
    }

    //
    // Print the information.
    //
    if (waveFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        DOUT (DBG_STREAM, ("[TestDataFormat] PCMEX - Frequency: %d, Channels: %d, bps: %d, ChannelMask: %X",
              waveFormat->Format.nSamplesPerSec, waveFormat->Format.nChannels,
              waveFormat->Format.wBitsPerSample, waveFormat->dwChannelMask));
    }
    else
    {
        DOUT (DBG_STREAM, ("[TestDataFormat] PCM - Frequency: %d, Channels: %d, bps: %d",
              waveFormat->Format.nSamplesPerSec, waveFormat->Format.nChannels,
              waveFormat->Format.wBitsPerSample));
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::PowerChangeNotify
 *****************************************************************************
 * This routine gets called as a result of hooking up the IPowerNotify
 * interface. This interface indicates the driver's desire to receive explicit
 * notification of power state changes. The interface provides a single method
 * (or callback) that is called by the miniport's corresponding port driver in
 * response to a power state change. Using wave audio as an example, when the
 * device is requested to go to a sleep state the port driver pauses any
 * active streams and then calls the power notify callback to inform the
 * miniport of the impending power down. The miniport then has an opportunity
 * to save any necessary context before the adapter's PowerChangeState method
 * is called. The process is reversed when the device is powering up. PortCls
 * first calls the adapter's PowerChangeState method to power up the adapter.
 * The port driver then calls the miniport's callback to allow the miniport to
 * restore its context. Finally, the port driver unpauses any previously paused
 * active audio streams.
 */
STDMETHODIMP_(void) CAC97MiniportWaveRT::PowerChangeNotify
(
    _In_  POWER_STATE NewState
)
{
    PAGED_CODE ();
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::PowerChangeNotify]"));

    //
    // Check to see if this is the current power state.
    //
    if (NewState.DeviceState == m_PowerState)
    {
        DOUT (DBG_POWER, ("New device state equals old state."));
        return;
    }

    //
    // Check the new device state.
    //
    if ((NewState.DeviceState < PowerDeviceD0) ||
        (NewState.DeviceState > PowerDeviceD3))
    {
        DOUT (DBG_ERROR, ("Unknown device state: D%d.",
             (ULONG)NewState.DeviceState - (ULONG)PowerDeviceD0));
        return;
    }

    DOUT (DBG_POWER, ("Changing state to D%d.", (ULONG)NewState.DeviceState -
                    (ULONG)PowerDeviceD0));

    //
    // In case we return to D0 power state from a D3 state, restore the
    // interrupt connection.
    //
    if (NewState.DeviceState == PowerDeviceD0)
    {
        ntStatus = InterruptSync->Connect ();
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("Failed to connect the ISR with InterruptSync!"));
            // We can do nothing else than just continue ...
        }
    }

    //
    // Call the stream routine which takes care of the DMA engine.
    // That's all we have to do.
    //
    for (int loop = PIN_WAVEOUT_OFFSET; loop < PIN_MICIN_OFFSET; loop++)
    {
        if (Streams[loop])
        {
            ntStatus = Streams[loop]->PowerChangeNotify (NewState);
            if (!NT_SUCCESS (ntStatus))
            {
                DOUT (DBG_ERROR, ("PowerChangeNotify D%d for the stream failed",
                              (ULONG)NewState.DeviceState - (ULONG)PowerDeviceD0));
            }
        }
    }

    //
    // In case we go to any sleep state we disconnect the interrupt service
    // reoutine from the interrupt.
    // Normally this is not required to do, but for some reason this fixes
    // a problem where we won't have any interrupts on specific motherboards
    // after resume.
    //
    if (NewState.DeviceState != PowerDeviceD0)
    {
        InterruptSync->Disconnect ();
    }

    //
    // Save the new state.  This local value is used to determine when to
    // cache property accesses and when to permit the driver from accessing
    // the hardware.
    //
    m_PowerState = NewState.DeviceState;
    DOUT (DBG_POWER, ("Entering D%d",
            (ULONG)m_PowerState - (ULONG)PowerDeviceD0));
}

/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#pragma code_seg()
/*****************************************************************************
 * InterruptServiceRoutine
 *****************************************************************************
 * The task of the ISR is to clear an interrupt from this device so we don't
 * get an interrupt storm and schedule a DPC which actually does the
 * real work.
 */
NTSTATUS CAC97MiniportWaveRT::InterruptServiceRoutine
(
    IN  PINTERRUPTSYNC  InterruptSync,
    IN  PVOID           DynamicContext
)
{
    UNREFERENCED_PARAMETER(InterruptSync);

    ASSERT (InterruptSync);
    ASSERT (DynamicContext);

    ULONG   GlobalStatus;
    USHORT  DMAStatusRegister;

    //
    // Get our context which is a pointer to class CAC97MiniportWaveRT.
    //
    CAC97MiniportWaveRT *that = (CAC97MiniportWaveRT *)DynamicContext;

    //
    // Check for a valid AdapterCommon pointer.
    //
    if (!that->AdapterCommon)
    {
        //
        // In case we didn't handle the interrupt, unsuccessful tells the system
        // to call the next interrupt handler in the chain.
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // From this point down, basically in the complete ISR, we cannot use
    // relative addresses (stream class base address + X_CR for example)
    // cause we might get called when the stream class is destroyed or
    // not existent. This doesn't make too much sense (that there is an
    // interrupt for a non-existing stream) but could happen and we have
    // to deal with the interrupt.
    //

    //
    // Read the global register to check the interrupt bits
    //
    GlobalStatus = that->AdapterCommon->ReadBMControlRegister32 (GLOB_STA);

    //
    // Check for weird return values. Could happen if the PCI device is already
    // disabled and another device that shares this interrupt generated an
    // interrupt.
    // The register should never have all bits cleared or set.
    //
    if (!GlobalStatus || (GlobalStatus == 0xFFFFFFFF))
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check for PCM out interrupt.
    //
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    if (GlobalStatus & GLOB_STA_POINT)
    {
        //
        // Read PCM out DMA status registers.
        //
        DMAStatusRegister = (USHORT)that->AdapterCommon->
            ReadBMControlRegister16 (PO_SR);


        //
        // We could now check for every possible error condition
        // (like FIFO error) and monitor the different errors, but currently
        // we have the same action for every INT and therefore we simplify
        // this routine enormous with just clearing the bits.
        //
        if (that->Streams[PIN_WAVEOUT_OFFSET])
        {
            //
            // ACK the interrupt.
            //
            that->AdapterCommon->WriteBMControlRegister (PO_SR, DMAStatusRegister);
            ntStatus = STATUS_SUCCESS;


            //
            // Update the LVI so that we cycle around in the scatter gather list.
            //
            UCHAR CIV = that->AdapterCommon->ReadBMControlRegister8 (PO_CIV);
            that->AdapterCommon->WriteBMControlRegister (PO_LVI, (UCHAR)((CIV-1) & BDL_MASK));
        }
    }

    //
    // Check for PCM in interrupt.
    //
    if (GlobalStatus & GLOB_STA_PIINT)
    {
        //
        // Read PCM in DMA status registers.
        //
        DMAStatusRegister = (USHORT)that->AdapterCommon->
            ReadBMControlRegister16 (PI_SR);

        //
        // We could now check for every possible error condition
        // (like FIFO error) and monitor the different errors, but currently
        // we have the same action for every INT and therefore we simplify
        // this routine enormous with just clearing the bits.
        //
        if (that->Streams[PIN_WAVEIN_OFFSET])
        {
            //
            // ACK the interrupt.
            //
            that->AdapterCommon->WriteBMControlRegister (PI_SR, DMAStatusRegister);
            ntStatus = STATUS_SUCCESS;


            //
            // Update the LVI so that we cycle around in the scatter gather list.
            //
            UCHAR CIV = that->AdapterCommon->ReadBMControlRegister8 (PI_CIV);
            that->AdapterCommon->WriteBMControlRegister (PI_LVI, (UCHAR)((CIV-1) & BDL_MASK));
        }
    }

    //
    // Check for MIC in interrupt.
    //
    if (GlobalStatus & GLOB_STA_MINT)
    {
        //
        // Read MIC in DMA status registers.
        //
        DMAStatusRegister = (USHORT)that->AdapterCommon->
            ReadBMControlRegister16 (MC_SR);

        //
        // We could now check for every possible error condition
        // (like FIFO error) and monitor the different errors, but currently
        // we have the same action for every INT and therefore we simplify
        // this routine enormous with just clearing the bits.
        //
        if (that->Streams[PIN_MICIN_OFFSET])
        {
            //
            // ACK the interrupt.
            //
            that->AdapterCommon->WriteBMControlRegister (MC_SR, DMAStatusRegister);
            ntStatus = STATUS_SUCCESS;


            //
            // Update the LVI so that we cycle around in the scatter gather list.
            //
            UCHAR CIV = that->AdapterCommon->ReadBMControlRegister8 (MC_CIV);
            that->AdapterCommon->WriteBMControlRegister (MC_LVI, (UCHAR)((CIV-1) & BDL_MASK));
        }
    }

    return ntStatus;
}

#endif

