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

#ifndef _TABLES_HPP_
#define _TABLES_HPP_

#include "mintopo.hpp"

#ifndef STATIC_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF
#define STATIC_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF\
    DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DOLBY_AC3_SPDIF)
DEFINE_GUIDSTRUCT("00000092-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF);
#define KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF)
#endif

NTSTATUS NTAPI PropertyHandler_Level(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_CpuResources(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_OnOff(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_ComponentId(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS NTAPI PropertyHandler_Private(PPCPROPERTY_REQUEST PropertyRequest);

static KSDATARANGE PinDataRangesBridge[] =
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

static PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
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

static PKSDATARANGE WavePinDataRangePointersAC3Bridge[] =
{
    &WavePinDataRangesAC3Bridge[0]
};

static PCPIN_DESCRIPTOR MiniportPins[] =
{
    // WAVEOUT_SOURCE - 0
    {
        0,0,0,                                          // InstanceCount
        NULL,                                           // AutomationTable
        {                                               // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // SPDIF_IN_SOURCE - 1
    {
        0,0,0,                                          // InstanceCount
        NULL,                                           // AutomationTable
        {                                               // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_SPDIF_INTERFACE,                // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // MIC_SOURCE - 2
	{
		0,0,0,											// InstanceCount
		NULL,											// AutomationTable
		{												// KsPinDescriptor
			0,											// InterfacesCount
			NULL,										// Interfaces
			0,											// MediumsCount
			NULL,										// Mediums
			SIZEOF_ARRAY(PinDataRangePointersBridge),	// DataRangesCount
			PinDataRangePointersBridge,					// DataRanges
			KSPIN_DATAFLOW_IN,							// DataFlow
			KSPIN_COMMUNICATION_NONE,					// Communication
			&KSNODETYPE_MICROPHONE,						// Category
			NULL,										// Name
			0											// Reserved
		}
	},

    // CD_SOURCE - 3
	{
		0,0,0,											// InstanceCount
		NULL,											// AutomationTable
		{												// KsPinDescriptor
			0,											// InterfacesCount
			NULL,										// Interfaces
			0,											// MediumsCount
			NULL,										// Mediums
			SIZEOF_ARRAY(PinDataRangePointersBridge),	// DataRangesCount
			PinDataRangePointersBridge,					// DataRanges
			KSPIN_DATAFLOW_IN,							// DataFlow
			KSPIN_COMMUNICATION_NONE,					// Communication
			&KSNODETYPE_CD_PLAYER,						// Category
			NULL,										// Name
			0											// Reserved
		}
	},

    // LINEIN_SOURCE - 4
	{
		0,0,0,											// InstanceCount
		NULL,											// AutomationTable
		{												// KsPinDescriptor
			0,											// InterfacesCount
			NULL,										// Interfaces
			0,											// MediumsCount
			NULL,										// Mediums
			SIZEOF_ARRAY(PinDataRangePointersBridge),	// DataRangesCount
			PinDataRangePointersBridge,					// DataRanges
			KSPIN_DATAFLOW_IN,							// DataFlow
			KSPIN_COMMUNICATION_NONE,					// Communication
			&KSNODETYPE_LINE_CONNECTOR,					// Category
			NULL,										// Name
			0											// Reserved
		}
	},

    // AUX_SOURCE - 5
	{
		0,0,0,											// InstanceCount
		NULL,											// AutomationTable
		{												// KsPinDescriptor
			0,											// InterfacesCount
			NULL,										// Interfaces
			0,											// MediumsCount
			NULL,										// Mediums
			SIZEOF_ARRAY(PinDataRangePointersBridge),	// DataRangesCount
			PinDataRangePointersBridge,					// DataRanges
			KSPIN_DATAFLOW_IN,							// DataFlow
			KSPIN_COMMUNICATION_NONE,					// Communication
			&KSNODETYPE_ANALOG_CONNECTOR,				// Category
			NULL,										// Name
			0											// Reserved
		}
	},

    // DAC_SOURCE - 6
	{
		0,0,0,											// InstanceCount
		NULL,											// AutomationTable
		{												// KsPinDescriptor
			0,											// InterfacesCount
			NULL,										// Interfaces
			0,											// MediumsCount
			NULL,										// Mediums
			SIZEOF_ARRAY(PinDataRangePointersBridge),	// DataRangesCount
			PinDataRangePointersBridge,					// DataRanges
			KSPIN_DATAFLOW_IN,							// DataFlow
			KSPIN_COMMUNICATION_NONE,					// Communication
			&KSNODETYPE_ANALOG_CONNECTOR,				// Category
			&CMINAME_DAC,								// Name
			0											// Reserved
		}
	},

    // LINEOUT_DEST - 7
    {
        0,0,0,                                          // InstanceCount
        NULL,                                           // AutomationTable
        {                                               // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_SPEAKER,                        // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // WAVEIN_DEST - 8
    {
        0,0,0,                                          // InstanceCount
        NULL,                                           // AutomationTable
        {                                               // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // SPDIF_AC3_SOURCE - 9
    {
        0,0,0,                                          // InstanceCount
        NULL,                                           // AutomationTable
        {                                               // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(WavePinDataRangePointersAC3Bridge),   // DataRangesCount
            WavePinDataRangePointersAC3Bridge,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // SPDIF_AC3_DEST - 10
    {
        0,0,0,                                          // InstanceCount
        NULL,                                           // AutomationTable
        {                                               // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(WavePinDataRangePointersAC3Bridge),   // DataRangesCount
            WavePinDataRangePointersAC3Bridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            NULL,                                       // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

static PCPROPERTY_ITEM PropertiesVolume[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_VOLUMELEVEL,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_Level
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationVolume,PropertiesVolume);

static PCPROPERTY_ITEM PropertiesLoudness[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_LOUDNESS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_CpuResources
    }
};
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationLoudness,PropertiesLoudness);

static PCPROPERTY_ITEM PropertiesMute[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MUTE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        (PCPFNPROPERTY_HANDLER)PropertyHandler_CpuResources
    }
};
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMute,PropertiesMute);

static PCPROPERTY_ITEM PropertiesFilter[] =
{
  {
    &KSPROPSETID_General,
    KSPROPERTY_GENERAL_COMPONENTID,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    (PCPFNPROPERTY_HANDLER)PropertyHandler_ComponentId
  },
  {
    &KSPROPSETID_CMI,
    KSPROPERTY_CMI_GET,
    KSPROPERTY_TYPE_GET,
    (PCPFNPROPERTY_HANDLER)PropertyHandler_Private
  },
  {
    &KSPROPSETID_CMI,
    KSPROPERTY_CMI_SET,
    KSPROPERTY_TYPE_SET,
    (PCPFNPROPERTY_HANDLER)PropertyHandler_Private
  }
};
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationFilter,PropertiesFilter);


const VolumeTable VolTable[] =
{//	  Node                          Reg   Mask  Shift   max  min  step dbshift
	{ KSNODE_TOPO_LINEOUT_VOLUME,   0x30, 0x1F, 3,      0,   -62, 2,   1 },
	{ KSNODE_TOPO_WAVEOUT_VOLUME,   0x32, 0x1F, 3,      0,   -62, 2,   1 },
	{ KSNODE_TOPO_CD_VOLUME,        0x36, 0x1F, 3,      0,   -62, 2,   1 },
	{ KSNODE_TOPO_LINEIN_VOLUME,    0x38, 0x1F, 3,      0,   -62, 2,   1 },
	{ KSNODE_TOPO_MICOUT_VOLUME,    0x3a, 0x1F, 3,      0,   -62, 2,   1 }
};

static PCNODE_DESCRIPTOR TopologyNodes[] =
{//	  Flags  AutomationTable      Type                  Name
// 0  - WAVEOUT_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_WAVE_VOLUME      },
// 1  - WAVEOUT_MUTE
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_WAVE_MUTE        },
// 2  - MICOUT_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_MIC_VOLUME       },
// 3  - LINEOUT_MIX
	{ 0,    NULL,                 &KSNODETYPE_SUM,      NULL                         },
// 4  - LINEOUT_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_MASTER_VOLUME    },
// 5  - WAVEIN_SUM
	{ 0,     &AutomationMute,     &KSNODETYPE_SUM,      &KSAUDFNAME_RECORDING_SOURCE },
// 6  - CD_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_CD_VOLUME        },
// 7  - LINEIN_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_LINE_IN_VOLUME   },
// 8  - AUX_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_AUX_VOLUME       },
// 9  - MICIN_VOLUME
	{ 0,     &AutomationVolume,   &KSNODETYPE_VOLUME,   &KSAUDFNAME_MIC_IN_VOLUME    },
// 10 - MICIN_LOUDNESS
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &KSAUDFNAME_MICROPHONE_BOOST },
// 11 - MICOUT_LOUDNESS
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &KSAUDFNAME_MICROPHONE_BOOST },
// 12 - CD_MUTE
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_CD_MUTE          },
// 13 - LINEIN_MUTE
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_LINE_MUTE        },
// 14 - MICOUT_MUTE
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_MIC_MUTE         },
// 15 - AUX_MUTE
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_AUX_MUTE         },
// 16 - LINEIN_MUTE_IN
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_LINE_MUTE        },
// 17 - MIC_MUTE_IN
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_MIC_MUTE         },
// 18 - AUX_MUTE_IN
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_CD_MUTE          },
// 19 - CD_MUTE_IN
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_AUX_MUTE         },
// 20 - WAVEOUT_MUTE_IN
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_WAVE_MUTE        },
// 21 - IEC_5V
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_5V              },
// 22 - IEC_OUT
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_OUT             },
// 23 - IEC_INVERSE
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_INVERSE         },
// 24 - IEC_MONITOR
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_MONITOR         },
// 25 - IEC_SELECT
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_SELECT          },
// 26 - SPDIF_AC3_MUTE - the WDM is truly braindamaged
	{ 0,     NULL,                &KSNODETYPE_MUTE,     NULL                         },
// 27 - SPDIF_AC3_MUX
	{ 0,     NULL,                &KSNODETYPE_MUX,      NULL                         },
// 28 - XCHG_FB
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_XCHG_FB             },
// 29 - BASS2LINE
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_BASS2LINE           },
// 30 - CENTER2LINE
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_CENTER2LINE         },
// 31 - IEC_COPYRIGHT
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_COPYRIGHT       },
// 32 - IEC_POLVALID
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_POLVALID        },
// 33 - IEC_LOOP
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_IEC_LOOP            },
// 34 - REAR2LINE
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_REAR2LINE           },
// 35 - CENTER2MIC
	{ 0,     &AutomationLoudness, &KSNODETYPE_LOUDNESS, &CMINAME_CENTER2MIC          },
// 36 - MASTER_MUTE_DUMMY
	{ 0,     &AutomationMute,     &KSNODETYPE_MUTE,     &KSAUDFNAME_MASTER_MUTE      }
};

static PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{// FromNode,                      FromPin,               ToNode,                        ToPin
  { PCFILTER_NODE,                 PIN_WAVEOUT_SOURCE,    KSNODE_TOPO_WAVEOUT_VOLUME,    1                  },
  { KSNODE_TOPO_WAVEOUT_VOLUME,    0,                     KSNODE_TOPO_WAVEOUT_MUTE,      1                  },
  { KSNODE_TOPO_WAVEOUT_MUTE,      0,                     KSNODE_TOPO_LINEOUT_MIX,       1                  },

  { PCFILTER_NODE,                 PIN_CD_SOURCE,         KSNODE_TOPO_CD_VOLUME,         1                  },
  { KSNODE_TOPO_CD_VOLUME,         0,                     KSNODE_TOPO_CD_MUTE,           1                  },
  { KSNODE_TOPO_CD_MUTE,           0,                     KSNODE_TOPO_LINEOUT_MIX,       2                  },

  { PCFILTER_NODE,                 PIN_LINEIN_SOURCE,     KSNODE_TOPO_LINEIN_VOLUME,     1                  },
  { KSNODE_TOPO_LINEIN_VOLUME,     0,                     KSNODE_TOPO_LINEIN_MUTE,       1                  },
  { KSNODE_TOPO_LINEIN_MUTE,       0,                     KSNODE_TOPO_LINEOUT_MIX,       3                  },

  { PCFILTER_NODE,                 PIN_AUX_SOURCE,        KSNODE_TOPO_AUX_VOLUME,        1                  },
  { KSNODE_TOPO_AUX_VOLUME,        0,                     KSNODE_TOPO_AUX_MUTE,          1                  },
  { KSNODE_TOPO_AUX_MUTE,          0,                     KSNODE_TOPO_LINEOUT_MIX,       3                  },

  { PCFILTER_NODE,                 PIN_MIC_SOURCE,        KSNODE_TOPO_MICOUT_LOUDNESS,   1                  },
  { KSNODE_TOPO_MICOUT_LOUDNESS,   0,                     KSNODE_TOPO_MICOUT_VOLUME,     1                  },
  { KSNODE_TOPO_MICOUT_VOLUME,     0,                     KSNODE_TOPO_MICOUT_MUTE,       1                  },
  { KSNODE_TOPO_MICOUT_MUTE,       0,                     KSNODE_TOPO_LINEOUT_MIX,       4                  },

  { PCFILTER_NODE,                 PIN_DAC_SOURCE,        KSNODE_TOPO_IEC_MONITOR,       1                  },
  { KSNODE_TOPO_IEC_MONITOR,       0,                     KSNODE_TOPO_BASS2LINE,         1                  },
  { KSNODE_TOPO_BASS2LINE,         0,                     KSNODE_TOPO_CENTER2LINE,       1                  },
  { KSNODE_TOPO_CENTER2LINE,       0,                     KSNODE_TOPO_REAR2LINE,         1                  },
  { KSNODE_TOPO_REAR2LINE,         0,                     KSNODE_TOPO_CENTER2MIC,        1                  },
  { KSNODE_TOPO_CENTER2MIC,        0,                     KSNODE_TOPO_XCHG_FB,           1                  },
  { KSNODE_TOPO_XCHG_FB,           0,                     KSNODE_TOPO_WAVEOUT_VOLUME,    1                  },
  { KSNODE_TOPO_WAVEOUT_VOLUME,    0,                     KSNODE_TOPO_WAVEOUT_MUTE,      1                  },
  { KSNODE_TOPO_WAVEOUT_MUTE,      0,                     KSNODE_TOPO_LINEOUT_MIX,       5                  },

  { KSNODE_TOPO_LINEOUT_MIX,       0,                     KSNODE_TOPO_IEC_OUT,           1                  },
  { KSNODE_TOPO_IEC_OUT,           0,                     KSNODE_TOPO_IEC_5V,            1                  },
  { KSNODE_TOPO_IEC_5V,            0,                     KSNODE_TOPO_MASTER_MUTE_DUMMY, 1                  },
  { KSNODE_TOPO_MASTER_MUTE_DUMMY, 0,                     KSNODE_TOPO_LINEOUT_VOLUME,    1                  },
  { KSNODE_TOPO_LINEOUT_VOLUME,    0,                     PCFILTER_NODE,                 PIN_LINEOUT_DEST   },

  { PCFILTER_NODE,                 PIN_MIC_SOURCE,        KSNODE_TOPO_MICIN_LOUDNESS,    1                  },
  { KSNODE_TOPO_MICIN_LOUDNESS,    0,                     KSNODE_TOPO_MICIN_VOLUME,      1                  },
  { KSNODE_TOPO_MICIN_VOLUME,      0,                     KSNODE_TOPO_MIC_MUTE_IN,       1                  },
  { KSNODE_TOPO_MIC_MUTE_IN,       0,                     KSNODE_TOPO_WAVEIN_SUM,        1                  },

  { KSNODE_TOPO_LINEIN_VOLUME,     0,                     KSNODE_TOPO_LINEIN_MUTE_IN,    1                  },
  { KSNODE_TOPO_LINEIN_MUTE_IN,    0,                     KSNODE_TOPO_WAVEIN_SUM,        2                  },

  { KSNODE_TOPO_CD_VOLUME,         0,                     KSNODE_TOPO_CD_MUTE_IN,        1                  },
  { KSNODE_TOPO_CD_MUTE_IN,        0,                     KSNODE_TOPO_WAVEIN_SUM,        3                  },

  { KSNODE_TOPO_AUX_VOLUME,        0,                     KSNODE_TOPO_AUX_MUTE_IN,       1                  },
  { KSNODE_TOPO_AUX_MUTE_IN,       0,                     KSNODE_TOPO_WAVEIN_SUM,        4                  },

  { PCFILTER_NODE,                 PIN_SPDIFIN_SOURCE,    KSNODE_TOPO_WAVEOUT_MUTE_IN,   1                  },
  { KSNODE_TOPO_WAVEOUT_MUTE_IN,   0,                     KSNODE_TOPO_IEC_INVERSE,       1                  },
  { KSNODE_TOPO_IEC_INVERSE,       0,                     KSNODE_TOPO_IEC_SELECT,        1                  },
  { KSNODE_TOPO_IEC_SELECT,        0,                     KSNODE_TOPO_IEC_COPYRIGHT,     1                  },
  { KSNODE_TOPO_IEC_COPYRIGHT,     0,                     KSNODE_TOPO_IEC_POLVALID,      1                  },
  { KSNODE_TOPO_IEC_POLVALID,      0,                     KSNODE_TOPO_IEC_LOOP,          1                  },
  { KSNODE_TOPO_IEC_LOOP,          0,                     KSNODE_TOPO_WAVEIN_SUM,        5                  },

  { KSNODE_TOPO_WAVEIN_SUM,        0,                     PCFILTER_NODE,                 PIN_WAVEIN_DEST    },

  { PCFILTER_NODE,                 PIN_SPDIF_AC3_SOURCE,  KSNODE_TOPO_SPDIF_AC3_MUTE,    1                  },
  { KSNODE_TOPO_SPDIF_AC3_MUTE,    0,                     KSNODE_TOPO_SPDIF_AC3_MUX,     1                  },
  { KSNODE_TOPO_SPDIF_AC3_MUX,     0,                     PCFILTER_NODE,                 PIN_SPDIF_AC3_DEST },
};


static PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                  // Version
    &AutomationFilter,                  // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    SIZEOF_ARRAY(TopologyNodes),        // NodeCount
    TopologyNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use default (audio, render, capture)
};


#endif
