/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _MINWAVETABLES_HPP_
#define _MINWAVETABLES_HPP_

#define STATIC_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF\
    DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DOLBY_AC3_SPDIF)
DEFINE_GUIDSTRUCT("00000092-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF);
#define KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF)

NTSTATUS NTAPI PropertyHandler_ChannelConfig(PPCPROPERTY_REQUEST PropertyRequest);


static KSDATARANGE_AUDIO WavePinDataRangesPCMStream[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        MAX_CHANNELS_PCM,
        MIN_BITS_PER_SAMPLE_PCM,
        MAX_BITS_PER_SAMPLE_PCM,
        MIN_SAMPLE_RATE,
        MAX_SAMPLE_RATE
    }
};

static KSDATARANGE_AUDIO WavePinDataRangesAC3Stream[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        MAX_CHANNELS_AC3,
        MIN_BITS_PER_SAMPLE_AC3,
        MAX_BITS_PER_SAMPLE_AC3,
        MIN_SAMPLE_RATE_AC3,
        MAX_SAMPLE_RATE_AC3
    },
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_DSOUND)
        },
        MAX_CHANNELS_AC3,
        MIN_BITS_PER_SAMPLE_AC3,
        MAX_BITS_PER_SAMPLE_AC3,
        MIN_SAMPLE_RATE_AC3,
        MAX_SAMPLE_RATE_AC3
    }
};

static PKSDATARANGE WavePinDataRangePointersPCMStream[] =
{
    PKSDATARANGE(&WavePinDataRangesPCMStream[0])
};

static PKSDATARANGE WavePinDataRangePointersAC3Stream[] =
{
    PKSDATARANGE(&WavePinDataRangesAC3Stream[0]),
    PKSDATARANGE(&WavePinDataRangesAC3Stream[1]),
};



static KSDATARANGE WavePinDataRangesPCMBridge[] =
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

static KSDATARANGE WavePinDataRangesAC3Bridge[] =
{
	{
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_AC3_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static PKSDATARANGE WavePinDataRangePointersPCMBridge[] =
{
    &WavePinDataRangesPCMBridge[0]
};

static PKSDATARANGE WavePinDataRangePointersAC3Bridge[] =
{
    &WavePinDataRangesAC3Bridge[0]
};

static PCPIN_DESCRIPTOR WaveMiniportPins[] =
{
    // PIN_WAVE_CAPTURE_SINK - 0
    {
        MAX_OUTPUT_STREAMS,
        MAX_OUTPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(WavePinDataRangePointersPCMStream),
            WavePinDataRangePointersPCMStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            &KSAUDFNAME_RECORDING_CONTROL,
            0
        }
    },

    // PIN_WAVE_CAPTURE_SOURCE - 1
    {
        0,
        0,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(WavePinDataRangePointersPCMBridge),
            WavePinDataRangePointersPCMBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },

    // PIN_WAVE_RENDER_SINK - 2
    {
        MAX_INPUT_STREAMS,
        MAX_INPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(WavePinDataRangePointersPCMStream),
            WavePinDataRangePointersPCMStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            &KSAUDFNAME_VOLUME_CONTROL,
            0
        }
    },

    // PIN_WAVE_RENDER_SOURCE - 3
    {
        0,
        0,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(WavePinDataRangePointersPCMBridge),
            WavePinDataRangePointersPCMBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_SPEAKER,
            NULL,
            0
        }
    },

    // PIN_WAVE_AC3_RENDER_SINK - 4
    {
        MAX_AC3_INPUT_STREAMS,
        MAX_AC3_INPUT_STREAMS,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(WavePinDataRangePointersAC3Stream),
            WavePinDataRangePointersAC3Stream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },


    // PIN_WAVE_AC3_RENDER_SOURCE - 5
    {
        0,
        0,
        0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(WavePinDataRangePointersAC3Bridge),
            WavePinDataRangePointersAC3Bridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_SPDIF_INTERFACE,
            NULL,
            0
        }
    }
};

static PCPROPERTY_ITEM PropertiesChannels[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CHANNEL_CONFIG,
        KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_ChannelConfig
    }
};
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationChans,PropertiesChannels);

static PCNODE_DESCRIPTOR WaveMiniportNodes[] =
{//   Flags  AutomationTable   Type                         Name
// 0 - KSNODE_WAVE_ADC
    { 0,     NULL,             &KSNODETYPE_ADC,             NULL },
// 1 - KSNODE_WAVE_VOLUME1
    { 0,     NULL,             &KSNODETYPE_VOLUME,          NULL },
// 2 - KSNODE_WAVE_3D_EFFECTS
    { 0,     NULL,             &KSNODETYPE_3D_EFFECTS,      NULL },
// 3 - KSNODE_WAVE_SUPERMIX
    { 0,     NULL,             &KSNODETYPE_SUPERMIX,        NULL },
// 4 - KSNODE_WAVE_VOLUME2
    { 0,     NULL,             &KSNODETYPE_VOLUME,          NULL },
// 5 - KSNODE_WAVE_SRC
    { 0,     NULL,             &KSNODETYPE_SRC,             NULL },
// 6 - KSNODE_WAVE_SUM
    { 0,     NULL,             &KSNODETYPE_SUM,             NULL },
// 7 - KSNODE_WAVE_DAC
    { 0,     &AutomationChans, &KSNODETYPE_DAC,             NULL },
// 8 - KSNODE_WAVE_SPDIF (XP crashes if the pins are directly connected)
    { 0,     NULL,             &KSNODETYPE_SPDIF_INTERFACE, NULL },
};

static PCCONNECTION_DESCRIPTOR WaveMiniportConnections[] =
{// FromNode,               FromPin,                 ToNode,                        ToPin
  { PCFILTER_NODE,          PIN_WAVE_CAPTURE_SOURCE, KSNODE_WAVE_ADC,               1                          },
  { KSNODE_WAVE_ADC,        0,                       PCFILTER_NODE,                 PIN_WAVE_CAPTURE_SINK      },

  { PCFILTER_NODE,          PIN_WAVE_RENDER_SINK,    KSNODE_WAVE_VOLUME1,           1                          },
  { KSNODE_WAVE_VOLUME1,    0,                       KSNODE_WAVE_3D_EFFECTS,        1                          },
  { KSNODE_WAVE_3D_EFFECTS, 0,                       KSNODE_WAVE_SUPERMIX,          1                          },
  { KSNODE_WAVE_SUPERMIX,   0,                       KSNODE_WAVE_VOLUME2,           1                          },
  { KSNODE_WAVE_VOLUME2,    0,                       KSNODE_WAVE_SRC,               1                          },
  { KSNODE_WAVE_SRC,        0,                       KSNODE_WAVE_SUM,               1                          },
  { KSNODE_WAVE_SUM,        0,                       KSNODE_WAVE_DAC,               1                          },
  { KSNODE_WAVE_DAC,        0,                       PCFILTER_NODE,                 PIN_WAVE_RENDER_SOURCE     },

  { PCFILTER_NODE,          PIN_WAVE_AC3_RENDER_SINK,KSNODE_WAVE_SPDIF,             1                          },
  { KSNODE_WAVE_SPDIF,      0,                       PCFILTER_NODE,                 PIN_WAVE_AC3_RENDER_SOURCE },
};

static PCFILTER_DESCRIPTOR WaveMiniportFilterDescriptor =
{
    0,                                      // Version
    NULL,                                   // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),               // PinSize
    SIZEOF_ARRAY(WaveMiniportPins),         // PinCount
    WaveMiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),              // NodeSize
    SIZEOF_ARRAY(WaveMiniportNodes),        // NodeCount
    WaveMiniportNodes,                      // Nodes
    SIZEOF_ARRAY(WaveMiniportConnections),  // ConnectionCount
    WaveMiniportConnections,                // Connections
    0,                                      // CategoryCount
    NULL                                    // Categories  - use the default categories (audio, render, capture)
};

#endif //_MINWAVETABLES_HPP_
