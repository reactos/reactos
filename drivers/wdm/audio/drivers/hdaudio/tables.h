/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Filter descriptor
 * COPYRIGHT:       Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#ifndef _TABLES_H_
#define _TABLES_H_

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

static PCNODE_DESCRIPTOR TopoInNodes[] = {{0, VolumeAutomationTable, &KSNODETYPE_VOLUME, &KSNODETYPE_MICROPHONE},
	                                      {0, MuteAutomationTable, &KSNODETYPE_MUTE, &KSNODETYPE_MICROPHONE}};

static PCNODE_DESCRIPTOR TopoOutNodes[] = {{0, VolumeAutomationTable, &KSNODETYPE_VOLUME, &KSAUDFNAME_MASTER_VOLUME},
	                                       {0, MuteAutomationTable, &KSNODETYPE_MUTE, &KSAUDFNAME_MASTER_MUTE}};

static PCCONNECTION_DESCRIPTOR DacConnections[] = {{KSFILTER_NODE, 0, 0, 1}, {0, 0, KSFILTER_NODE, 1}};

static PCCONNECTION_DESCRIPTOR AdcConnections[] = {{KSFILTER_NODE, 1, 0, 1}, {0, 0, KSFILTER_NODE, 0}};

static PCCONNECTION_DESCRIPTOR TopoInConnections[] = {{KSFILTER_NODE, 1, 0, 1}, {1, 0, KSFILTER_NODE, 0}, {0, 0, 1, 1}};

static PCCONNECTION_DESCRIPTOR TopoOutConnections[] = {{KSFILTER_NODE, 0, 0, 1}, {1, 0, KSFILTER_NODE, 1}, {0, 0, 1, 1}};

static KSDATARANGE_AUDIO DataRange[1];

static PKSDATARANGE DataRanges[1];

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
    DataRangeBridge
};

static PCPIN_DESCRIPTOR WaveInPins[] =
{
    {   // Pin 0 -- WaveIn
        1,1,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRanges,                                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            &PINNAME_CAPTURE,                           // Category
            &KSAUDFNAME_RECORDING_CONTROL,              // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- WaveIn Bridge
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
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
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRanges,                                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
            &KSAUDFNAME_PC_SPEAKER,                     // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- WaveOut Bridge
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
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
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- Mic Source
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
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
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {   // Pin 1 -- Speakers Dest
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            1,                                          // DataRangesCount
            DataRangesBridge,                           // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_SPEAKER,                        // Category
            &KSAUDFNAME_PC_SPEAKER,                     // Name
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
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use defaults (audio, render, capture)
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
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use defaults (audio, render, capture)
};

static PCFILTER_DESCRIPTOR TopoInFilterDescription[] =
{
    0,                                  // Version
    &TopologyAutomationTable,           // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    2,                                  // PinCount
    TopoInPins,                         // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    2,                                  // NodeCount
    TopoInNodes,                        // Nodes
    3,                                  // ConnectionCount
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
