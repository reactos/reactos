/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file shared.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _SHARED_H_
#define _SHARED_H_

#define PC_IMPLEMENTATION 1

//
// Get the NTDDK headers instead of the WDM headers that portcls.h wants to include.
//
#define WIN9X_COMPAT_SPINLOCK

#ifdef UNDER_NT
#ifdef __cplusplus
extern "C" {
#endif
    #include <ntddk.h>
#ifdef __cplusplus
} // extern "C"
#endif
#endif

#include <portcls.h>
#include <stdunk.h>
#include "ichreg.h"
#include "ac97reg.h"
#include "debug.h"


/*****************************************************************************
 * Structures and Typedefs 
 */

const ULONG PoolTag = '79CA';

// This enum defines all the possible pin configurations. It is pretty easy,
// cause a pin can be there or not, depending if the CoDec supports it (like
// Headphone output (PINC_HPOUT_PRESENT)) or if the OEM disabled the feature
// with a private inf file.
// Look at common.h file for the registry string names.
// ATTN: Don't change without changing the static struct in common.cpp too.
enum TopoPinConfig
{
    PINC_PCBEEP_PRESENT = 0,
    PINC_PHONE_PRESENT,
    PINC_MIC2_PRESENT,
    PINC_VIDEO_PRESENT,
    PINC_AUX_PRESENT,
    PINC_HPOUT_PRESENT,
    PINC_MONOOUT_PRESENT,
    PINC_MICIN_PRESENT,
    PINC_MIC_PRESENT,
    PINC_LINEIN_PRESENT,
    PINC_CD_PRESENT,
    PINC_SURROUND_PRESENT,
    PINC_CENTER_LFE_PRESENT,
    PINC_TOP_ELEMENT            // number of PINC's
};

// This enum defines the functional configuration, called nodes. Nodes are
// black boxes that implement a functionality like 3d (NODEC_3D_PRESENT).
// At startup, we probe the Codec for features (like the pins above) and
// initialize an array which holds the configuration.
enum TopoNodeConfig
{
    NODEC_3D_PRESENT = 0,
    NODEC_TONE_PRESENT,
    NODEC_LOUDNESS_PRESENT,
    NODEC_SIMUL_STEREO_PRESENT,
    NODEC_6BIT_MASTER_VOLUME,
    NODEC_6BIT_HPOUT_VOLUME,
    NODEC_6BIT_MONOOUT_VOLUME,
    NODEC_6BIT_SURROUND_VOLUME,
    NODEC_6BIT_CENTER_LFE_VOLUME,
    NODEC_3D_CENTER_ADJUSTABLE,
    NODEC_3D_DEPTH_ADJUSTABLE,
    NODEC_PCM_VARIABLERATE_SUPPORTED,
    NODEC_PCM_VSR_INDEPENDENT_RATES,
    NODEC_PCM_DOUBLERATE_SUPPORTED,
    NODEC_MIC_VARIABLERATE_SUPPORTED,
    NODEC_CENTER_DAC_PRESENT,
    NODEC_SURROUND_DAC_PRESENT,
    NODEC_LFE_DAC_PRESENT,
    NODEC_TOP_ELEMENT           // number of NODES's
};

//
// Pin Defininition goes here
// We define all the possible pins in the AC97 CoDec and some "virtual" pins
// that are used for the topology to connect special functionality like 3D.
//
enum TopoPins
{
    // Source is something that goes into the AC97, dest goes out.
    PIN_WAVEOUT_SOURCE  = 0,
    PIN_PCBEEP_SOURCE,
    PIN_PHONE_SOURCE,
    PIN_MIC_SOURCE,
    PIN_LINEIN_SOURCE,
    PIN_CD_SOURCE,
    PIN_VIDEO_SOURCE,
    PIN_AUX_SOURCE,
    PIN_VIRT_3D_CENTER_SOURCE,
    PIN_VIRT_3D_DEPTH_SOURCE,
    PIN_VIRT_3D_MIX_MONO_SOURCE,
    PIN_VIRT_TONE_MIX_SOURCE,
    PIN_VIRT_TONE_MIX_MONO_SOURCE,
    PIN_VIRT_SURROUND_SOURCE,
    PIN_VIRT_CENTER_SOURCE,
    PIN_VIRT_LFE_SOURCE,
    PIN_VIRT_FRONT_SOURCE,
    PIN_MASTEROUT_DEST,
    PIN_HPOUT_SOURCE,
    PIN_MONOOUT_DEST,
    PIN_WAVEIN_DEST,
    PIN_MICIN_DEST,
    PIN_TOP_ELEMENT,            // number of pins
    PIN_INVALID
};

#if (DBG)
// In case we print some debug information about the pins, we use the names
// defined here.
const PCHAR TopoPinStrings[] =
{
    "PIN_WAVEOUT_SOURCE",
    "PIN_PCBEEP_SOURCE",
    "PIN_PHONE_SOURCE",
    "PIN_MIC_SOURCE",
    "PIN_LINEIN_SOURCE",
    "PIN_CD_SOURCE",
    "PIN_VIDEO_SOURCE",
    "PIN_AUX_SOURCE",
    "PIN_VIRT_3D_CENTER_SOURCE",
    "PIN_VIRT_3D_DEPTH_SOURCE",
    "PIN_VIRT_3D_MIX_MONO_SOURCE",
    "PIN_VIRT_TONE_MIX_SOURCE",
    "PIN_VIRT_TONE_MIX_MONO_SOURCE",
    "PIN_VIRT_SURROUND_SOURCE",
    "PIN_VIRT_CENTER_SOURCE",
    "PIN_VIRT_LFE_SOURCE",
    "PIN_VIRT_FRONT_SOURCE",
    "PIN_MASTEROUT_DEST",
    "PIN_HPOUT_SOURCE",
    "PIN_MONOOUT_DEST",
    "PIN_WAVEIN_DEST",
    "PIN_MICIN_DEST",
    "TOP_ELEMENT",              // should never dump this
    "INVALID"                   // or this either
};
#endif


//
// Node Definition goes here.
// We define all the possible nodes here (nodes are black boxes that represent
// a functional block like bass volume) and some virtual nodes, mainly volume
// controls, that are used to represent special functionality in the topology
// like 3D controls (exposed as volumes) or to give the user volume controls
// for each possible record line. In that case, the volume is placed in front
// of the record selector (mux). The topology is not parsed correctly if there
// are no volume controls between the pins and a muxer. Also, these virtual
// controls only represent volumes and no mutes, cause mutes wouldn't be dis-
// played by sndvol32.
// ATTN: DON'T  change without first looking at the table in ac97reg.h!!!
enum TopoNodes
{
    NODE_WAVEOUT_VOLUME = 0,
    NODE_WAVEOUT_MUTE,
    NODE_VIRT_WAVEOUT_3D_BYPASS,        // exposed as AGC control
    NODE_PCBEEP_VOLUME,
    NODE_PCBEEP_MUTE,
    NODE_PHONE_VOLUME,
    NODE_PHONE_MUTE,
    NODE_MIC_SELECT,
    NODE_MIC_BOOST,
    NODE_MIC_VOLUME,
    NODE_MIC_MUTE,
    NODE_LINEIN_VOLUME,
    NODE_LINEIN_MUTE,
    NODE_CD_VOLUME,
    NODE_CD_MUTE,
    NODE_VIDEO_VOLUME,
    NODE_VIDEO_MUTE,
    NODE_AUX_VOLUME,
    NODE_AUX_MUTE,
    NODE_MAIN_MIX,
    NODE_VIRT_3D_CENTER,                // we have no 3D control type, so we
    NODE_VIRT_3D_DEPTH,                 // expose 2 volume controls and 2 mute
    NODE_VIRT_3D_ENABLE,                // checkboxs (the other is bypass).
    NODE_BEEP_MIX,
    NODE_BASS,
    NODE_TREBLE,
    NODE_LOUDNESS,
    NODE_SIMUL_STEREO,
    NODE_MASTEROUT_VOLUME,
    NODE_MASTEROUT_MUTE,
    NODE_HPOUT_VOLUME,
    NODE_HPOUT_MUTE,
    NODE_MONOOUT_SELECT,
    NODE_VIRT_MONOOUT_VOLUME1,          // each mono out must have volume
    NODE_VIRT_MONOOUT_VOLUME2,
    NODE_WAVEIN_SELECT,
    NODE_VIRT_MASTER_INPUT_VOLUME1,     // boy, each master input must have a
    NODE_VIRT_MASTER_INPUT_VOLUME2,     // volume
    NODE_VIRT_MASTER_INPUT_VOLUME3,
    NODE_VIRT_MASTER_INPUT_VOLUME4,
    NODE_VIRT_MASTER_INPUT_VOLUME5,
    NODE_VIRT_MASTER_INPUT_VOLUME6,
    NODE_VIRT_MASTER_INPUT_VOLUME7,
    NODE_VIRT_MASTER_INPUT_VOLUME8,
    NODE_MICIN_VOLUME,
    NODE_MICIN_MUTE,
    NODE_SURROUND_VOLUME,
    NODE_SURROUND_MUTE,
    NODE_CENTER_VOLUME,
    NODE_CENTER_MUTE,
    NODE_LFE_VOLUME,
    NODE_LFE_MUTE,
    NODE_FRONT_VOLUME,
    NODE_FRONT_MUTE,
    NODE_VIRT_MASTERMONO_VOLUME,        // used for multichannel or headphone
    NODE_VIRT_MASTERMONO_MUTE,
    NODE_TOP_ELEMENT,                   // number of nodes
    NODE_INVALID
};

#if (DBG)
// In case we print some debug information about the nodes, we use names
// defined here.
const PCHAR NodeStrings[] =
{
    "WAVEOUT_VOLUME",
    "WAVEOUT_MUTE",
    "WAVEOUT_3D_BYPASS",
    "PCBEEP_VOLUME",
    "PCBEEP_MUTE",
    "PHONE_VOLUME",
    "PHONE_MUTE",
    "MIC_SELECT",
    "MIC_BOOST",
    "MIC_VOLUME",
    "MIC_MUTE",
    "LINEIN_VOLUME",
    "LINEIN_MUTE",
    "CD_VOLUME",
    "CD_MUTE",
    "VIDEO_VOLUME",
    "VIDEO_MUTE",
    "AUX_VOLUME",
    "AUX_MUTE",
    "MAIN_MIX",
    "3D_CENTER",
    "3D_DEPTH",
    "3D_ENABLE",
    "BEEP_MIX",
    "BASS",
    "TREBLE",
    "LOUDNESS",
    "SIMUL_STEREO",
    "MASTER_VOLUME",
    "MASTER_MUTE",
    "HPOUT_VOLUME",
    "HPOUT_MUTE",
    "MONOOUT_SELECT",
    "MONOOUT_VOLUME_3D_MIX",
    "MONOOUT_VOLUME_MIC",
    "WAVEIN_SELECT",
    "MASTER_INPUT_VOLUME_MIC",
    "MASTER_INPUT_VOLUME_CD",
    "MASTER_INPUT_VOLUME_VIDEO",
    "MASTER_INPUT_VOLUME_AUX",
    "MASTER_INPUT_VOLUME_LINEIN",
    "MASTER_INPUT_VOLUME_TONE_MIX",
    "MASTER_INPUT_VOLUME_TONE_MIX_MONO",
    "MASTER_INPUT_VOLUME_PHONE",
    "MICIN_VOLUME",
    "MICIN_MUTE",
    "SURROUND_VOLUME",
    "SURROUND_MUTE",
    "CENTER_VOLUME",
    "CENTER_MUTE",
    "LFE_VOLUME",
    "LFE_MUTE",
    "FRONT_VOLUME",
    "FRONT_MUTE",
    "VIRT_MASTERMONO_VOLUME",
    "VIRT_MASTERMONO_MUTE",
    "TOP_ELEMENT",      // should never dump this
    "INVALID"           // or this
};
#endif

//
// The pins used for the wave miniport connection.
//
enum WavePins
{
    PIN_WAVEOUT = 0,
    PIN_WAVEOUT_BRIDGE,
    PIN_WAVEIN,
    PIN_WAVEIN_BRIDGE,
    PIN_MICIN,
    PIN_MICIN_BRIDGE
};

//
// The nodes used for the wave miniport connection.
//
enum WaveNodes
{
    NODE_WAVEOUT_DAC,
    NODE_WAVEIN_ADC,
    NODE_MICIN_ADC
};

/*****************************************************************************
 * Function prototypes
 */

/*****************************************************************************
 * NewAdapterCommon()
 *****************************************************************************
 * Create a new adapter common object.
 */
NTSTATUS NewAdapterCommon
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN      POOL_TYPE   PoolType
);

/*****************************************************************************
 * Class definitions
 */

/*****************************************************************************
 * IAC97MiniportTopology
 *****************************************************************************
 * Interface for topology miniport.
 */
DECLARE_INTERFACE_(IAC97MiniportTopology,IMiniportTopology)
{
    STDMETHOD_(NTSTATUS,GetPhysicalConnectionPins)
    (   THIS_
        OUT     PULONG  WaveOutSource,
        OUT     PULONG  WaveInDest,
        OUT     PULONG  MicInDest
    )   PURE;
    // Used for DRM:
    STDMETHOD_(void, SetCopyProtectFlag)
    (   THIS_
        IN      BOOL
    )   PURE;
};

typedef IAC97MiniportTopology *PAC97MINIPORTTOPOLOGY;

/*****************************************************************************
 * IAC97AdapterCommon
 *****************************************************************************
 * Interface for adapter common object.
 */
DECLARE_INTERFACE_(IAC97AdapterCommon,IUnknown)
{
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PRESOURCELIST   ResourceList,
        IN      PDEVICE_OBJECT  DeviceObject
    )   PURE;
    STDMETHOD_(BOOL,GetPinConfig)
    (   THIS_
        IN      TopoPinConfig
    )   PURE;
    STDMETHOD_(void,SetPinConfig)
    (   THIS_
        IN      TopoPinConfig,
        IN      BOOL
    )   PURE;
    STDMETHOD_(BOOL,GetNodeConfig)
    (   THIS_
        IN      TopoNodeConfig
    )   PURE;
    STDMETHOD_(void,SetNodeConfig)
    (   THIS_
        IN      TopoNodeConfig,
        IN      BOOL
    )   PURE;
    STDMETHOD_(AC97Register,GetNodeReg)
    (   THIS_
        IN      TopoNodes
    )   PURE;
    STDMETHOD_(WORD,GetNodeMask)
    (   THIS_
        IN      TopoNodes
    )   PURE;
    STDMETHOD_(NTSTATUS,ReadCodecRegister)
    (   THIS_
        _In_range_(0, AC97REG_INVALID) IN      AC97Register    Register,
        _Out_ OUT     PWORD           wData
    )   PURE;
    STDMETHOD_(NTSTATUS,WriteCodecRegister)
    (   THIS_
        _In_range_(0, AC97REG_INVALID) IN      AC97Register    Register,
        _In_ IN      WORD            wData,
        _In_ IN      WORD            wMask
    )   PURE;
    STDMETHOD_(UCHAR,ReadBMControlRegister8)
    (   THIS_
        IN      ULONG   Offset
    )   PURE;
    STDMETHOD_(USHORT,ReadBMControlRegister16)
    (   THIS_
        IN      ULONG   Offset
    )   PURE;
    STDMETHOD_(ULONG,ReadBMControlRegister32)
    (   THIS_
        IN      ULONG   Offset
    )   PURE;    
    STDMETHOD_(void,WriteBMControlRegister)
    (   THIS_
        IN      ULONG   Offset,
        IN      UCHAR   Value
    )   PURE;
    STDMETHOD_(void,WriteBMControlRegister)
    (   THIS_
        IN      ULONG   Offset,
        IN      USHORT  Value
    )   PURE;
    STDMETHOD_(void,WriteBMControlRegister)
    (   THIS_
        IN      ULONG   Offset,
        IN      ULONG   Value
    )   PURE;
    STDMETHOD_(NTSTATUS, RestoreCodecRegisters)
    (   THIS_
        void
    )   PURE;
    STDMETHOD_(NTSTATUS, ProgramSampleRate)
    (   THIS_
        IN      AC97Register  Register,
        IN      DWORD         dwSampleRate
    )   PURE;
    // Used for DRM:
    STDMETHOD_(void, SetMiniportTopology)
    (   THIS_
        IN      PAC97MINIPORTTOPOLOGY
    )   PURE;
    STDMETHOD_(PAC97MINIPORTTOPOLOGY, GetMiniportTopology)
    (   THIS_
        void
    )   PURE;
    // These are used by the wave miniport.
    STDMETHOD_(void, ReadChannelConfigDefault)
    (   THIS_
        PDWORD  pwChannelConfig,
        PWORD   pwChannels
    )   PURE;
    STDMETHOD_(void, WriteChannelConfigDefault)
    (   THIS_
        DWORD   dwChannelConfig
    )   PURE;
};

typedef IAC97AdapterCommon *PADAPTERCOMMON;

/*****************************************************************************
 * Guids for the Interfaces
 *****************************************************************************
 */

// {77481FA0-1EF2-11d2-883A-0080C765647D}
DEFINE_GUID(IID_IAC97AdapterCommon, 
0x77481fa0, 0x1ef2, 0x11d2, 0x88, 0x3a, 0x0, 0x80, 0xc7, 0x65, 0x64, 0x7d);

// {245AE964-49C8-11d2-95D7-00C04FB925D3}
DEFINE_GUID(IID_IAC97MiniportTopology, 
0x245ae964, 0x49c8, 0x11d2, 0x95, 0xd7, 0x0, 0xc0, 0x4f, 0xb9, 0x25, 0xd3);

#endif  //_SHARED_H_

