/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Filter descriptor
 * COPYRIGHT:       Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#ifndef _TABLES_H_
#define _TABLES_H_

// default formats range
#define MAX_CHANNELS 2
#define MIN_BITS_PER_SAMPLE 8
#define MAX_BITS_PER_SAMPLE 32
#define MIN_SAMPLE_FREQUENCY 8000
#define MAX_SAMPLE_FREQUENCY 96000

static GUID KSCATEGORY_RangeAudio = {STATIC_KSCATEGORY_AUDIO};
static GUID KSCATEGORY_Audio = {STATIC_KSCATEGORY_AUDIO};
static GUID KSAUDFNAME_Pc_Speaker = {STATIC_KSAUDFNAME_PC_SPEAKER};
static GUID PINNAME_Capture = {STATIC_PINNAME_CAPTURE};
static GUID KSAUDFNAME_Recording_Control = {STATIC_KSAUDFNAME_RECORDING_CONTROL};

static KSPIN_INTERFACE StandardPinInterface = {{STATIC_KSINTERFACESETID_Standard}, KSINTERFACE_STANDARD_STREAMING, 0};

static KSPIN_MEDIUM StandardPinMedium = {{STATIC_KSMEDIUMSETID_Standard}, KSMEDIUM_TYPE_ANYINSTANCE, 0};

NTSTATUS NTAPI PropertyHandler_JackDescription(IN PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_ChannelConfig(IN PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_SpeakerGeometry(IN PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_Volume(IN PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_Mute(IN PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI EventHandler_Volume(IN PPCEVENT_REQUEST EventRequest);

static PCPROPERTY_ITEM WaveProperty[] = 
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CHANNEL_CONFIG,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        PropertyHandler_ChannelConfig
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_STEREO_SPEAKER_GEOMETRY,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        PropertyHandler_SpeakerGeometry
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(WaveAutomationTable, WaveProperty);

static PCPROPERTY_ITEM VolumeProperty[] = 
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_VOLUMELEVEL,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Volume
    }
};

static PCPROPERTY_ITEM MuteProperty[] = 
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MUTE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Mute
    }
};

static PCEVENT_ITEM VolumeEvent[] = 
{
    {
        &KSEVENTSETID_AudioControlChange,
        KSEVENT_CONTROL_CHANGE,
        KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
        EventHandler_Volume
    }
};

static PCAUTOMATION_TABLE VolumeAutomationTable[] =
{
    {
        sizeof(PCPROPERTY_ITEM),
        1,
        VolumeProperty,
        sizeof(PCMETHOD_ITEM),
        0,
        NULL,
        sizeof(PCEVENT_ITEM),
        1,
        VolumeEvent,
        0
    }
};

static PCAUTOMATION_TABLE MuteAutomationTable[] =
{
    {
        sizeof(PCPROPERTY_ITEM),
        1,
        MuteProperty,
        sizeof(PCMETHOD_ITEM),
        0,
        NULL,
        sizeof(PCEVENT_ITEM),
        1,
        VolumeEvent,
        0
    }
};

static PCPROPERTY_ITEM TopologyProperty[] = 
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_JackDescription
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(TopologyAutomationTable, TopologyProperty);

static PCNODE_DESCRIPTOR DacNode[] = {{0, &WaveAutomationTable, &KSNODETYPE_DAC, NULL}};

static PCNODE_DESCRIPTOR AdcNode[] = {{0, NULL, &KSNODETYPE_ADC, NULL}};

static PCNODE_DESCRIPTOR TopoInNodes[] = {{0, NULL, &KSNODETYPE_SUM, NULL},
	                                      {0, VolumeAutomationTable, &KSNODETYPE_VOLUME, &KSNODETYPE_MICROPHONE},
	                                      {0, MuteAutomationTable, &KSNODETYPE_MUTE, &KSNODETYPE_MICROPHONE},
	                                      {0, VolumeAutomationTable, &KSNODETYPE_VOLUME, &KSNODETYPE_MICROPHONE},
	                                      {0, MuteAutomationTable, &KSNODETYPE_MUTE, &KSNODETYPE_MICROPHONE}};

static PCNODE_DESCRIPTOR TopoOutNodes[] = {{0, VolumeAutomationTable, &KSNODETYPE_VOLUME, &KSAUDFNAME_MASTER_VOLUME},
	                                       {0, MuteAutomationTable, &KSNODETYPE_MUTE, &KSAUDFNAME_MASTER_MUTE}};

static PCCONNECTION_DESCRIPTOR DacConnections[] = {{KSFILTER_NODE, 0, 0, 1}, {0, 0, KSFILTER_NODE, 1}};

static PCCONNECTION_DESCRIPTOR AdcConnections[] = {{KSFILTER_NODE, 1, 0, 1}, {0, 0, KSFILTER_NODE, 0}};

static PCCONNECTION_DESCRIPTOR TopoInConnections[] = {{0, 0, KSFILTER_NODE, 0}, {KSFILTER_NODE, 1, 1, 1},
	                                                  {1, 0, 2, 1}, {2, 0, 0, 1},
	                                                  {KSFILTER_NODE, 2, 3, 1}, {3, 0, 4, 1}, {4, 0, 0, 2}};

static PCCONNECTION_DESCRIPTOR TopoOutConnections[] = {{KSFILTER_NODE, 0, 0, 1}, {0, 0, 1, 1}, {1, 0, KSFILTER_NODE, 1}};

static GUID AdcCategories[] = {KSCATEGORY_AUDIO, KSCATEGORY_CAPTURE};

static GUID DacCategories[] = {KSCATEGORY_AUDIO, KSCATEGORY_RENDER};

static KSDATARANGE_AUDIO DataRange[] =
{
    {
       sizeof(KSDATARANGE_AUDIO),
       0, 0, 0,
       STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
       STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM), // PCM is default
       STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    MAX_CHANNELS,
    MIN_BITS_PER_SAMPLE,
    MAX_BITS_PER_SAMPLE,
    MIN_SAMPLE_FREQUENCY,
    MAX_SAMPLE_FREQUENCY
};

static PKSDATARANGE_AUDIO DataRanges[] =
{
    &DataRange[0]
};

static KSDATARANGE DataRangeBridge[] =
{
   {
      sizeof(KSDATARANGE),
      0, 0, 0,
      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

static PKSDATARANGE DataRangesBridge[] =
{
    &DataRangeBridge[0]
};

static PCPIN_DESCRIPTOR WaveInPins[] =
{
    {   // Pin 0 -- WaveIn
        1,1,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            (PKSDATARANGE *)DataRanges,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            &KSCATEGORY_Audio,                          // Category
            &KSAUDFNAME_Recording_Control,              // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- WaveIn Bridge
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_Audio,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

static PCPIN_DESCRIPTOR WaveOutPins[] =
{
    {   // Pin 0 -- WaveOut
        1,1,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            (PKSDATARANGE *)DataRanges,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            &KSCATEGORY_Audio,                          // Category
            &KSAUDFNAME_VOLUME_CONTROL,                 // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- WaveOut Bridge
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_SPEAKER,                        // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

static PCPIN_DESCRIPTOR TopoInPins[] =
{
    {   // Pin 0 -- WaveIn Dest
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_Audio,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- Mic Source
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_MICROPHONE,                     // Category
            &KSNODETYPE_MICROPHONE,                     // Name
            0                                           // Reserved
        }
    },
    {   // Pin 2 -- Mic 2 Source
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_MICROPHONE,                     // Category
            &KSNODETYPE_MICROPHONE,                     // Name
            0                                           // Reserved
        }
    }
};

static PCPIN_DESCRIPTOR TopoOutPins[] =
{
    {   // Pin 0 -- WaveOut Source
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_Audio,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- Speakers Dest
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            1,                                          // InterfacesCount
            &StandardPinInterface,                      // Interfaces
            1,                                          // MediumsCount
            &StandardPinMedium,                         // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_SPEAKER,                        // Category
            &KSNODETYPE_SPEAKER,                        // Name
            0                                           // Reserved
        }
    }
};

static PCFILTER_DESCRIPTOR WaveInFilterDescription[] =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    2,                                  // PinCount
    WaveInPins,                         // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    1,                                  // NodeCount
    AdcNode,                            // Nodes
    2,                                  // ConnectionCount
    AdcConnections,                     // Connections
    2,                                  // CategoryCount
    AdcCategories                       // Categories: NULL->use defaults (audio, render, capture)
};

static PCFILTER_DESCRIPTOR WaveOutFilterDescription[] =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    2,                                  // PinCount
    WaveOutPins,                        // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    1,                                  // NodeCount
    DacNode,                            // Nodes
    2,                                  // ConnectionCount
    DacConnections,                     // Connections
    2,                                  // CategoryCount
    DacCategories                       // Categories: NULL->use defaults (audio, render, capture)
};

static PCFILTER_DESCRIPTOR TopoInFilterDescription[] =
{
    0,                                  // Version
    &TopologyAutomationTable,           // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    3,                                  // PinCount
    TopoInPins,                         // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    5,                                  // NodeCount
    TopoInNodes,                        // Nodes
    7,                                  // ConnectionCount
    TopoInConnections,                  // Connections
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use defaults (audio, render, capture)
};

static PCFILTER_DESCRIPTOR TopoOutFilterDescription[] =
{
    0,                                  // Version
    &TopologyAutomationTable,           // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    2,                                  // PinCount
    TopoOutPins,                        // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    2,                                  // NodeCount
    TopoOutNodes,                       // Nodes
    3,                                  // ConnectionCount
    TopoOutConnections,                 // Connections
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use defaults (audio, render, capture)
};

#endif // _TABLES_H_
