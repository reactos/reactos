/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file common.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
static char STR_MODULENAME[] = "AC97 Common: ";

#include "common.h"


/*****************************************************************************
 * Static Members
 *****************************************************************************
 */

//
// This is the register cache including registry names and default values. The
// first WORD contains the register value and the second WORD contains a flag.
// Currently, we only set SHREG_INVALID if we have to read the register at
// startup (that's true when there is no constant default value for the
// register).  Note that we cache the registers only to prevent read access to
// the AC97 CoDec during runtime, because this is slow (40us).
// We only set SHREG_INIT if we want to set the register to default at driver
// startup.  If needed, the third field contains the registry name and the
// forth field contains a default value that is used when there is no registry
// entry.
// The flag SHREG_NOCACHE is used when we don't want to cache the register
// at all.  This is neccessary for status registers and sample rate registers.
//
tAC97Registers CAC97AdapterCommon::m_stAC97Registers[] =
{
{0x0000, SHREG_INVALID, NULL,               0},         // AC97REG_RESET
{0x8000, SHREG_INIT,    L"MasterVolume",    0x0000},    // AC97REG_MASTER_VOLUME
{0x8000, SHREG_INIT,    L"HeadphoneVolume", 0x0000},    // AC97REG_HPHONE_VOLUME
{0x8000, SHREG_INIT,    L"MonooutVolume",   0x0000},    // AC97REG_MMONO_VOLUME
{0x0F0F, SHREG_INIT,    L"ToneControls",    0x0F0F},    // AC97REG_MASTER_TONE
{0x0000, SHREG_INVALID |
         SHREG_INIT,    L"BeepVolume",      0x0000},    // AC97REG_BEEP_VOLUME
{0x8008, SHREG_INIT,    L"PhoneVolume",     0x8008},    // AC97REG_PHONE_VOLUME
{0x8008, SHREG_INIT,    L"MicVolume",       0x8008},    // AC97REG_MIC_VOLUME
{0x8808, SHREG_INIT,    L"LineInVolume",    0x0808},    // AC97REG_LINE_IN_VOLUME
{0x8808, SHREG_INIT,    L"CDVolume",        0x0808},    // AC97REG_CD_VOLUME
{0x8808, SHREG_INIT,    L"VideoVolume",     0x0808},    // AC97REG_VIDEO_VOLUME
{0x8808, SHREG_INIT,    L"AUXVolume",       0x0808},    // AC97REG_AUX_VOLUME
{0x8808, SHREG_INIT,    L"WaveOutVolume",   0x0808},    // AC97REG_PCM_OUT_VOLUME
{0x0000, SHREG_INIT,    L"RecordSelect",    0x0404},    // AC97REG_RECORD_SELECT
{0x8000, SHREG_INIT,    L"RecordGain",      0x0000},    // AC97REG_RECORD_GAIN
{0x8000, SHREG_INIT,    L"RecordGainMic",   0x0000},    // AC97REG_RECORD_GAIN_MIC
{0x0000, SHREG_INIT,    L"GeneralPurpose",  0x0000},    // AC97REG_GENERAL
{0x0000, SHREG_INIT,    L"3DControl",       0x0000},    // AC97REG_3D_CONTROL
{0x0000, SHREG_NOCACHE, NULL,               0},         // AC97REG_RESERVED
{0x0000, SHREG_NOCACHE |
         SHREG_INIT,    L"PowerDown",       0},         // AC97REG_POWERDOWN
// AC97-2.0 registers
{0x0000, SHREG_INVALID, NULL,               0},         // AC97REG_EXT_AUDIO_ID
{0x0000, SHREG_NOCACHE |
         SHREG_INIT,    L"ExtAudioCtrl",    0x4001},    // AC97REG_EXT_AUDIO_CTRL
{0xBB80, SHREG_NOCACHE, NULL,               0},         // AC97REG_FRONT_SAMPLERATE
{0xBB80, SHREG_NOCACHE, NULL,               0},         // AC97REG_SURROUND_SAMPLERATE
{0xBB80, SHREG_NOCACHE, NULL,               0},         // AC97REG_LFE_SAMPLERATE
{0xBB80, SHREG_NOCACHE, NULL,               0},         // AC97REG_RECORD_SAMPLERATE
{0xBB80, SHREG_NOCACHE, NULL,               0},         // AC97REG_MIC_SAMPLERATE
{0x8080, SHREG_INIT,    L"CenterLFEVolume", 0x0000},    // AC97REG_CENTER_LFE_VOLUME
{0x8080, SHREG_INIT,    L"SurroundVolume",  0x0000},    // AC97REG_SURROUND_VOLUME
{0x0000, SHREG_NOCACHE, NULL,               0}          // AC97REG_RESERVED2

// We leave the other values blank.  There would be a huge gap with 31
// elements that are currently unused, and then there would be 2 other
// (used) values, the vendor IDs.  We just force a read from the vendor
// IDs in the end of ProbeHWConfig to fill the cache.
};


//
// This is the hardware configuration information.  The first struct is for
// nodes, which we default to FALSE.  The second struct is for Pins, which
// contains the configuration (FALSE) and the registry string which is the
// reason for making a static struct so we can just fill in the name.
//
tHardwareConfig CAC97AdapterCommon::m_stHardwareConfig =
{
    // Nodes
    {{FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE},
     {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE}, {FALSE},
     {FALSE}},
    // Pins
    {{FALSE, L"DisablePCBeep"},     // PINC_PCBEEP_PRESENT
     {FALSE, L"DisablePhone"},      // PINC_PHONE_PRESENT
     {FALSE, L"DisableMic2"},       // PINC_MIC2_PRESENT
     {FALSE, L"DisableVideo"},      // PINC_VIDEO_PRESENT
     {FALSE, L"DisableAUX"},        // PINC_AUX_PRESENT
     {FALSE, L"DisableHeadphone"},  // PINC_HPOUT_PRESENT
     {FALSE, L"DisableMonoOut"},    // PINC_MONOOUT_PRESENT
     {FALSE, L"DisableMicIn"},      // PINC_MICIN_PRESENT
     {FALSE, L"DisableMic"},        // PINC_MIC_PRESENT
     {FALSE, L"DisableLineIn"},     // PINC_LINEIN_PRESENT
     {FALSE, L"DisableCD"},         // PINC_CD_PRESENT
     {FALSE, L"DisableSurround"},   // PINC_SURROUND_PRESENT
     {FALSE, L"DisableCenterLFE"}}  // PINC_CENTER_LFE_PRESENT
};


#pragma code_seg("PAGE")
/*****************************************************************************
 * NewAdapterCommon
 *****************************************************************************
 * Create a new adapter common object.
 */
NTSTATUS NewAdapterCommon
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

    DOUT (DBG_PRINT, ("[NewAdapterCommon]"));

    STD_CREATE_BODY_WITH_TAG_(CAC97AdapterCommon,Unknown,UnknownOuter,PoolType,
                              PoolTag, PADAPTERCOMMON);
}


/*****************************************************************************
 * CAC97AdapterCommon::Init
 *****************************************************************************
 * Initialize the adapter common object -> initialize and probe HW.
 * Pass only checked resources.
 */
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::Init
(
    IN  PRESOURCELIST   ResourceList,
    IN  PDEVICE_OBJECT  DeviceObject
)
{
    PAGED_CODE ();

    ASSERT (ResourceList);
    ASSERT (DeviceObject);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::Init]"));

    //
    // Set the topology pointer to NULL.
    //
    m_Topology = NULL;

    //
    // Save the device object
    //
    m_pDeviceObject = DeviceObject;

    //
    // Get the base address for the AC97 codec and bus master.
    //
    ASSERT (ResourceList->FindTranslatedPort (0));
    m_pCodecBase = (PUSHORT)ResourceList->FindTranslatedPort (0)->
                           u.Port.Start.QuadPart;

    ASSERT (ResourceList->FindTranslatedPort (1));
    m_pBusMasterBase = (PUCHAR)ResourceList->FindTranslatedPort (1)->
                              u.Port.Start.QuadPart;

    DOUT (DBG_SYSINFO, ("Configuration:\n"
                        "   Bus Master = 0x%p\n"
                        "   Codec      = 0x%p",
                        m_pBusMasterBase, m_pCodecBase));

    //
    // Set m_bDirectRead to TRUE so that all AC97 register read and
    // writes are going directly to the HW
    //
    m_bDirectRead = TRUE;

    //
    // Initialize the hardware.
    //
    ntStatus = InitAC97 ();
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Probe hardware configuration
    //
    ntStatus = ProbeHWConfig ();
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Probing of hardware configuration failed!"));
        return ntStatus;
    }

    //
    // Now, every AC97 read access goes to the cache.
    //
    m_bDirectRead = FALSE;

    //
    // Restore the AC97 registers now.
    //
#if (DBG)
    DumpConfig ();
#endif
    ntStatus = SetAC97Default ();

    //
    // Initialize the device state.
    //
    m_PowerState = PowerDeviceD0;

    return ntStatus;
}


/*****************************************************************************
 * CAC97AdapterCommon::~CAC97AdapterCommon
 *****************************************************************************
 * Destructor.
 */
CAC97AdapterCommon::~CAC97AdapterCommon ()
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::~CAC97AdapterCommon]"));
}


#if (DBG)
/*****************************************************************************
 * CAC97AdapterCommon::DumpConfig
 *****************************************************************************
 * Dumps the HW configuration for the AC97 codec.
 */
void CAC97AdapterCommon::DumpConfig (void)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::DumpConfig]"));

    //
    // Print debug output for MICIN.
    //
    if (GetPinConfig (PINC_MICIN_PRESENT))
    {
        DOUT (DBG_PROBE, ("MICIN found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No MICIN found"));
    }

    //
    // Print debug output for tone controls.
    //
    if (GetNodeConfig (NODEC_TONE_PRESENT))
    {
        DOUT (DBG_PROBE, ("Tone controls found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No tone controls found"));
    }

    //
    // Print debug output for mono out.
    //
    if (!GetPinConfig (PINC_MONOOUT_PRESENT))
    {
        DOUT (DBG_PROBE, ("No mono out found"));
    }

    //
    // Print debug output for headphones.
    //
    if (!GetPinConfig (PINC_HPOUT_PRESENT))
    {
        DOUT (DBG_PROBE, ("No headphone out found"));
    }

    //
    // Print debug output for loudness.
    //
    if (GetNodeConfig (NODEC_LOUDNESS_PRESENT))
    {
        DOUT (DBG_PROBE, ("Loudness found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No Loudness found"));
    }

    //
    // Print debug output for 3D.
    //
    if (GetNodeConfig (NODEC_3D_PRESENT))
    {
        DOUT (DBG_PROBE, ("3D controls found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No 3D controls found"));
    }

    //
    // Print debug output for pc beep.
    //
    if (GetPinConfig (PINC_PCBEEP_PRESENT))
    {
        DOUT (DBG_PROBE, ("PC beep found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No PC beep found"));
    }

    //
    // Print debug output for phone line (or mono line input).
    //
    if (GetPinConfig (PINC_PHONE_PRESENT))
    {
        DOUT (DBG_PROBE, ("Phone found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No Phone found"));
    }

    //
    // Print debug output for video.
    //
    if (GetPinConfig (PINC_VIDEO_PRESENT))
    {
        DOUT (DBG_PROBE, ("Video in found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No Video in found"));
    }

    //
    // Print debug output for AUX.
    //
    if (GetPinConfig (PINC_AUX_PRESENT))
    {
        DOUT (DBG_PROBE, ("AUX in found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No AUX in found"));
    }

    //
    // Print debug output for second miorophone.
    //
    if (GetPinConfig (PINC_MIC2_PRESENT))
    {
        DOUT (DBG_PROBE, ("MIC2 found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("No MIC2 found"));
    }

    //
    // Print debug output for 3D stuff.
    //
    if (GetNodeConfig (NODEC_3D_PRESENT))
    {
        if (GetNodeConfig (NODEC_3D_CENTER_ADJUSTABLE))
        {
            DOUT (DBG_PROBE, ("Adjustable 3D center control found"));
        }
        if (GetNodeConfig (NODEC_3D_DEPTH_ADJUSTABLE))
        {
            DOUT (DBG_PROBE, ("Nonadjustable 3D depth control found"));
        }
    }

    //
    // Print debug output for quality of master volume.
    //
    if (GetNodeConfig (NODEC_6BIT_MASTER_VOLUME))
    {
        DOUT (DBG_PROBE, ("6bit master out found"));
    }
    else
    {
        DOUT (DBG_PROBE, ("5bit master out found"));
    }

    //
    // Print debug output for quality of headphones volume.
    //
    if (GetPinConfig (PINC_HPOUT_PRESENT))
    {
        if (GetNodeConfig (NODEC_6BIT_HPOUT_VOLUME))
        {
            DOUT (DBG_PROBE, ("6bit headphone out found"));
        }
        else
        {
            DOUT (DBG_PROBE, ("5bit headphone out found"));
        }
    }

    //
    // Print debug output for quality of mono out volume.
    //
    if (GetPinConfig (PINC_MONOOUT_PRESENT))
    {
        if (GetNodeConfig (NODEC_6BIT_MONOOUT_VOLUME))
        {
            DOUT (DBG_PROBE, ("6bit mono out found"));
        }
        else
        {
            DOUT (DBG_PROBE, ("5bit mono out found"));
        }
    }

    //
    // Print sample rate information.
    //
    if (GetNodeConfig (NODEC_PCM_VARIABLERATE_SUPPORTED))
    {
        DOUT (DBG_PROBE, ("PCM variable sample rate supported"));
    }
    else
    {
        DOUT (DBG_PROBE, ("only 48KHz PCM supported"));
    }

    //
    // Print double rate information.
    //
    if (GetNodeConfig (NODEC_PCM_DOUBLERATE_SUPPORTED))
    {
        DOUT (DBG_PROBE, ("PCM double sample rate supported"));
    }

    //
    // Print mic rate information.
    //
    if (GetNodeConfig (NODEC_MIC_VARIABLERATE_SUPPORTED))
    {
        DOUT (DBG_PROBE, ("MIC variable sample rate supported"));
    }
    else
    {
        DOUT (DBG_PROBE, ("only 48KHz MIC supported"));
    }

    // print DAC information
    if (GetNodeConfig (NODEC_CENTER_DAC_PRESENT))
    {
        DOUT (DBG_PROBE, ("center DAC found"));
    }
    if (GetNodeConfig (NODEC_SURROUND_DAC_PRESENT))
    {
        DOUT (DBG_PROBE, ("surround DAC found"));
    }
    if (GetNodeConfig (NODEC_LFE_DAC_PRESENT))
    {
        DOUT (DBG_PROBE, ("LFE DAC found"));
    }
}
#endif

/*****************************************************************************
 * CAC97AdapterCommon::NonDelegatingQueryInterface
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 * We basically just check any GUID we know and return this object in case we
 * know it.
 */
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::NonDelegatingQueryInterface
(
    _In_         REFIID  Interface,
    _COM_Outptr_ PVOID * Object
)
{
    PAGED_CODE ();

    ASSERT (Object);

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::NonDelegatingQueryInterface]"));

    // Is it IID_IUnknown?
    if (IsEqualGUIDAligned (Interface, IID_IUnknown))
    {
        *Object = (PVOID)(PUNKNOWN)(PADAPTERCOMMON)this;
    }
    else
    // or IID_IAC97AdapterCommon ...
    if (IsEqualGUIDAligned (Interface, IID_IAC97AdapterCommon))
    {
        *Object = (PVOID)(PADAPTERCOMMON)this;
    }
    else
    // or IID_IAdapterPowerManagement ...
    if (IsEqualGUIDAligned (Interface, IID_IAdapterPowerManagement))
    {
        *Object = (PVOID)(PADAPTERPOWERMANAGEMENT)this;
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
    ((PUNKNOWN)*Object)->AddRef ();
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAC97AdapterCommon::InitAC97
 *****************************************************************************
 * Initialize the AC97 (without hosing the modem if it got installed first).
 */
NTSTATUS CAC97AdapterCommon::InitAC97 (void)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::InitAC97]"));

    //
    // First check if there is an AC link to the primary CoDec.
    //
    NTSTATUS ntStatus = PrimaryCodecReady ();
    if (NT_SUCCESS (ntStatus))
    {
        //
        // Second, reset this primary CoDec; If this is a AMC97 CoDec, only
        // the audio registers are reset. If this is a MC97 CoDec, the CoDec
        // should ignore the reset (according to the spec).
        //
        WriteCodecRegister (AC97REG_RESET, 0x00, 0xFFFF);

        ntStatus = PowerUpCodec ();
    }
    else
    {
        DOUT (DBG_ERROR, ("Initialization of AC97 CoDec failed."));
    }

    return ntStatus;
}

/*****************************************************************************
 * CAC97AdapterCommon::Check6thBitSupport
 *****************************************************************************
 * Probes for 6th bit volume control support.
 * The passed parameters are the AC97 register that has the volume control and
 * the node config that should be set in this case.
 */
NTSTATUS CAC97AdapterCommon::Check6thBitSupport
(
    IN AC97Register AC97Reg,
    IN TopoNodeConfig Config
)
{
    PAGED_CODE();

    NTSTATUS    ntStatus;
    WORD        wCodecReg;
    WORD        wOriginal;

    // Read the current value.
    ntStatus = ReadCodecRegister (AC97Reg, &wOriginal);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Write the 6th bit; for mono controls we write 0x20, for stereo
    // controls 0x2020.
    ntStatus = WriteCodecRegister (AC97Reg,
                    (AC97Reg == AC97REG_MMONO_VOLUME) ? 0x0020 : 0x2020, 0xFFFF);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // And read back.
    ntStatus = ReadCodecRegister (AC97Reg, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Check return. For mono 0x20 and for stereo 0x2020.
    if (((wCodecReg & 0x0020) && (AC97Reg == AC97REG_MMONO_VOLUME)) ||
        (wCodecReg & 0x2020))
    {
        SetNodeConfig (Config, TRUE);
    }
    else
    {
        SetNodeConfig (Config, FALSE);
    }

    // Restore original value.
    WriteCodecRegister (AC97Reg, wOriginal, 0xFFFF);

    return ntStatus;
}

/*****************************************************************************
 * CAC97AdapterCommon::ProbeHWConfig
 *****************************************************************************
 * Probes the hardware configuration.
 * If this function returns with an error, then the configuration is not
 * complete! Probing the registers is done by reading them (and comparing with
 * the HW default value) or when the default is unknown, writing to them and
 * reading back + restoring.
 * Additionally, we read the registry so that a HW vendor can overwrite (means
 * disable) found registers in case the adapter (e.g. video) is not visible to
 * the user (he can't plug in a video audio there).
 *
 * This is a very long function with all of the error checking!
 */
NTSTATUS CAC97AdapterCommon::ProbeHWConfig (void)
{
    PAGED_CODE ();

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD    dwGlobalStatus;
    WORD     wCodecID;
    WORD     wCodecReg;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ProbeHWConfig]"));

    //
    // Wait for the whatever 97 to complete reset and establish a link.
    //
    ntStatus = PrimaryCodecReady ();
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Master volume is one of the supported registers on an AC97
    //
    ntStatus = ReadCodecRegister (AC97REG_MASTER_VOLUME, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Default is x8000.
    if (wCodecReg != 0x8000)
        return STATUS_NO_SUCH_DEVICE;

    //
    // This gives us information about the AC97 CoDec
    //
    ntStatus = ReadCodecRegister (AC97REG_RESET, &wCodecID);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Fill out the configuration stuff.
    //

    SetPinConfig (PINC_MICIN_PRESENT, wCodecID & 0x0001);

    // Check if OEM wants to disable MIC record line.
    if (DisableAC97Pin (PINC_MICIN_PRESENT))
        SetPinConfig (PINC_MICIN_PRESENT, FALSE);

    // If we still have MIC record line, enable the DAC in ext. audio register.
    if (GetPinConfig (PINC_MICIN_PRESENT))
        // Enable ADC MIC.
        WriteCodecRegister (AC97REG_EXT_AUDIO_CTRL, 0, 0x4000);
    else
        // Disable ADC MIC.
        WriteCodecRegister (AC97REG_EXT_AUDIO_CTRL, 0x4000, 0x4000);

    //
    // Continue setting configuration information.
    //

    SetNodeConfig (NODEC_TONE_PRESENT, wCodecID & 0x0004);
    SetNodeConfig (NODEC_SIMUL_STEREO_PRESENT, wCodecID & 0x0008);
    SetPinConfig (PINC_HPOUT_PRESENT, wCodecID & 0x0010);

    // Check if OEM wants to disable headphone output.
    if (DisableAC97Pin (PINC_HPOUT_PRESENT))
        SetPinConfig (PINC_HPOUT_PRESENT, FALSE);

    SetNodeConfig (NODEC_LOUDNESS_PRESENT, wCodecID & 0x0020);
    SetNodeConfig (NODEC_3D_PRESENT, wCodecID & 0x7C00);

    //
    // Test for the input pins that are always there but could be disabled
    // by the HW vender
    //

    // Check if OEM wants to disable mic input.
    SetPinConfig (PINC_MIC_PRESENT, !DisableAC97Pin (PINC_MIC_PRESENT));

    // Check if OEM wants to disable line input.
    SetPinConfig (PINC_LINEIN_PRESENT, !DisableAC97Pin (PINC_LINEIN_PRESENT));

    // Check if OEM wants to disable CD input.
    SetPinConfig (PINC_CD_PRESENT, !DisableAC97Pin (PINC_CD_PRESENT));


    //
    // For the rest, we have to probe the registers.
    //

    //
    // Test for Mono out.
    //
    ntStatus = ReadCodecRegister (AC97REG_MMONO_VOLUME, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Default is x8000.
    SetPinConfig (PINC_MONOOUT_PRESENT, (wCodecReg == 0x8000));

    // Check if OEM wants to disable mono output.
    if (DisableAC97Pin (PINC_MONOOUT_PRESENT))
        SetPinConfig (PINC_MONOOUT_PRESENT, FALSE);

    //
    // Test for PC beeper support.
    //
    ntStatus = ReadCodecRegister (AC97REG_BEEP_VOLUME, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // default is x0 or x8000. If it's 0x8000 then we know for sure that the
    // CoDec has a PcBeep, otherwise we have to check the register
    if (wCodecReg == 0x8000)
        SetPinConfig (PINC_PCBEEP_PRESENT, TRUE);
    else if (!wCodecReg)
    {
        // mute the pc beeper.
        ntStatus = WriteCodecRegister (AC97REG_BEEP_VOLUME, 0x8000, 0xFFFF);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        // read back
        ntStatus = ReadCodecRegister (AC97REG_BEEP_VOLUME, &wCodecReg);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        if (wCodecReg == 0x8000)
        {
            // yep, we have support.
            SetPinConfig (PINC_PCBEEP_PRESENT, TRUE);
            // reset to default value.
            WriteCodecRegister (AC97REG_BEEP_VOLUME, 0x0, 0xFFFF);
        }
        else
            // nope, not present
            SetPinConfig (PINC_PCBEEP_PRESENT, FALSE);
    }
    else
        // any other value then 0x0 and 0x8000.
        SetPinConfig (PINC_PCBEEP_PRESENT, FALSE);

    // Check if OEM wants to disable beeper support.
    if (DisableAC97Pin (PINC_PCBEEP_PRESENT))
        SetPinConfig (PINC_PCBEEP_PRESENT, FALSE);

    //
    // Test for phone support.
    //
    ntStatus = ReadCodecRegister (AC97REG_PHONE_VOLUME, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Default is x8008.
    SetPinConfig (PINC_PHONE_PRESENT, (wCodecReg == 0x8008));

    // Check if OEM wants to disable phone input.
    if (DisableAC97Pin (PINC_PHONE_PRESENT))
        SetPinConfig (PINC_PHONE_PRESENT, FALSE);

    //
    // Test for video support.
    //
    ntStatus = ReadCodecRegister (AC97REG_VIDEO_VOLUME, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Default is x8808.
    SetPinConfig (PINC_VIDEO_PRESENT, (wCodecReg == 0x8808));

    // Check if OEM wants to disable video input.
    if (DisableAC97Pin (PINC_VIDEO_PRESENT))
        SetPinConfig (PINC_VIDEO_PRESENT, FALSE);

    //
    // Test for Aux support.
    //
    ntStatus = ReadCodecRegister (AC97REG_AUX_VOLUME, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Default is 0x8808.
    SetPinConfig (PINC_AUX_PRESENT, (wCodecReg == 0x8808));

    // Check if OEM wants to disable aux input.
    if (DisableAC97Pin (PINC_AUX_PRESENT))
        SetPinConfig (PINC_AUX_PRESENT, FALSE);

    //
    // Test for Mic2 source.
    //
    ntStatus = ReadCodecRegister (AC97REG_GENERAL, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    // Test for Mic2 select bit.
    if (wCodecReg & 0x0100)
        SetPinConfig (PINC_MIC2_PRESENT, TRUE);
    else
    {
        // Select Mic2 as source.
        ntStatus = WriteCodecRegister (AC97REG_GENERAL, 0x0100, 0x0100);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        // Read back.
        ntStatus = ReadCodecRegister (AC97REG_GENERAL, &wCodecReg);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        if (wCodecReg & 0x0100)
        {
            // Yep, we have support so set it to the default value.
            SetPinConfig (PINC_MIC2_PRESENT, TRUE);
            // reset to default value.
            WriteCodecRegister (AC97REG_GENERAL, 0, 0x0100);
        }
        else
            SetPinConfig (PINC_MIC2_PRESENT, FALSE);
    }

    // Check if OEM wants to disable mic2 input.
    if (DisableAC97Pin (PINC_MIC2_PRESENT))
        SetPinConfig (PINC_MIC2_PRESENT, FALSE);

    //
    // Test the 3D controls.
    //
    if (GetNodeConfig (NODEC_3D_PRESENT))
    {
        //
        // First test for fixed 3D controls. Write default value ...
        //
        ntStatus = WriteCodecRegister (AC97REG_3D_CONTROL, 0, 0xFFFF);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        // Read 3D register. Default is 0 when adjustable, otherwise it is
        // a fixed value.
        ntStatus = ReadCodecRegister (AC97REG_3D_CONTROL, &wCodecReg);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        //
        // Check center and depth separately.
        //

        // For center
        SetNodeConfig (NODEC_3D_CENTER_ADJUSTABLE, !(wCodecReg & 0x0F00));

        // For depth
        SetNodeConfig (NODEC_3D_DEPTH_ADJUSTABLE, !(wCodecReg & 0x000F));

        //
        // Test for adjustable controls.
        //
        WriteCodecRegister (AC97REG_3D_CONTROL, 0x0A0A, 0xFFFF);

        // Read 3D register. Now it should be 0x0A0A for adjustable controls,
        // otherwise it is a fixed control or simply not there.
        ReadCodecRegister (AC97REG_3D_CONTROL, &wCodecReg);

        // Restore the default value
        WriteCodecRegister (AC97REG_3D_CONTROL, 0, 0xFFFF);

        // Check the center control for beeing adjustable
        if (GetNodeConfig (NODEC_3D_CENTER_ADJUSTABLE) &&
            (wCodecReg & 0x0F00) != 0x0A00)
        {
            SetNodeConfig (NODEC_3D_CENTER_ADJUSTABLE, FALSE);
        }

        // Check the depth control for beeing adjustable
        if (GetNodeConfig (NODEC_3D_DEPTH_ADJUSTABLE) &&
            (wCodecReg & 0x000F) != 0x000A)
        {
            SetNodeConfig (NODEC_3D_DEPTH_ADJUSTABLE, FALSE);
        }
    }

    //
    // Check for 6th bit support in volume controls.  To check the 6th bit,
    // we first have to write a value (with 6th bit set) and then read it
    // back.  After that, we should restore the register to its default value.
    //

    //
    // Start with the master volume.
    //
    Check6thBitSupport (AC97REG_MASTER_VOLUME, NODEC_6BIT_MASTER_VOLUME);

    //
    // Check for a headphone volume control.
    //
    if (GetPinConfig (PINC_HPOUT_PRESENT))
    {
        Check6thBitSupport (AC97REG_HPHONE_VOLUME, NODEC_6BIT_HPOUT_VOLUME);
    }

    //
    // Mono out there?
    //
    if (GetPinConfig (PINC_MONOOUT_PRESENT))
    {
        Check6thBitSupport (AC97REG_MMONO_VOLUME, NODEC_6BIT_MONOOUT_VOLUME);
    }

    //
    // Get extended AC97 V2.0 information
    //
    ntStatus = ReadCodecRegister (AC97REG_EXT_AUDIO_ID, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Store the information
    //
    SetNodeConfig (NODEC_PCM_VARIABLERATE_SUPPORTED, wCodecReg & 0x0001);
    SetNodeConfig (NODEC_PCM_DOUBLERATE_SUPPORTED, wCodecReg & 0x0002);
    SetNodeConfig (NODEC_MIC_VARIABLERATE_SUPPORTED, wCodecReg & 0x0008);
    SetNodeConfig (NODEC_CENTER_DAC_PRESENT, wCodecReg & 0x0040);
    SetNodeConfig (NODEC_SURROUND_DAC_PRESENT, wCodecReg & 0x0080);
    SetNodeConfig (NODEC_LFE_DAC_PRESENT, wCodecReg & 0x0100);

    //
    // In case we have some features get some more information and program
    // the codec.
    //
    if (wCodecReg)
    {
        //
        // Enable variable sample rate in the control register and disable
        // double rate. Also enable all DACs.
        //
        WriteCodecRegister (AC97REG_EXT_AUDIO_CTRL, wCodecReg & 0x0009, 0x380B);

        //
        // Check for codecs that have only one sample rate converter. These
        // codecs will stick registers AC97REG_FRONT_SAMPLERATE and
        // AC97REG_RECORD_SAMPLERATE together.
        //
        if (GetNodeConfig (NODEC_PCM_VARIABLERATE_SUPPORTED))
        {
            // The default of the sample rate registers should be 0xBB80.
            WriteCodecRegister (AC97REG_FRONT_SAMPLERATE, 0xBB80, 0xFFFF);

            // Write 44.1KHz into record VSR, then check playback again.
            WriteCodecRegister (AC97REG_RECORD_SAMPLERATE, 0xAC44, 0xFFFF);
            ntStatus = ReadCodecRegister (AC97REG_FRONT_SAMPLERATE, &wCodecReg);
            WriteCodecRegister (AC97REG_RECORD_SAMPLERATE, 0xBB80, 0xFFFF);
            if (!NT_SUCCESS (ntStatus))
                return ntStatus;

            //
            // Set the flag accordingly
            //
            SetNodeConfig (NODEC_PCM_VSR_INDEPENDENT_RATES, (wCodecReg == 0xBB80));
        }

        //
        // Check multichanel support on the AC97.
        //
        if (GetNodeConfig (NODEC_SURROUND_DAC_PRESENT))
        {
            dwGlobalStatus = ReadBMControlRegister32 (GLOB_STA);

            //
            // Codec supports >2 chanel, does AC97 too?
            //
            if ((GetNodeConfig (NODEC_CENTER_DAC_PRESENT) ||
                GetNodeConfig (NODEC_LFE_DAC_PRESENT)) &&
                (dwGlobalStatus & GLOB_STA_MC6))
            {
                SetPinConfig (PINC_CENTER_LFE_PRESENT, TRUE);
            }
            else
            {
                SetPinConfig (PINC_CENTER_LFE_PRESENT, FALSE);
            }

            //
            // Do we support at least 4 channels?
            //
            SetPinConfig (PINC_SURROUND_PRESENT, (dwGlobalStatus & GLOB_STA_MC4));
        }
        else
        {
            //
            // Only 2 channel (stereo) support.
            //
            SetPinConfig (PINC_CENTER_LFE_PRESENT, FALSE);
            SetPinConfig (PINC_SURROUND_PRESENT, FALSE);
        }
    }

    // Check if OEM wants to disable surround output.
    if (DisableAC97Pin (PINC_SURROUND_PRESENT))
        SetPinConfig (PINC_SURROUND_PRESENT, FALSE);

    // Check if OEM wants to disable center and LFE output.
    if (DisableAC97Pin (PINC_CENTER_LFE_PRESENT))
        SetPinConfig (PINC_CENTER_LFE_PRESENT, FALSE);

    //
    // Check the 6th bit support for the additional channels.
    //
    if (GetPinConfig (PINC_SURROUND_PRESENT))
        Check6thBitSupport (AC97REG_SURROUND_VOLUME, NODEC_6BIT_SURROUND_VOLUME);

    if (GetPinConfig (PINC_CENTER_LFE_PRESENT))
        Check6thBitSupport (AC97REG_CENTER_LFE_VOLUME, NODEC_6BIT_CENTER_LFE_VOLUME);

    //
    // We read these registers because they are dependent on the codec.
    //
    ReadCodecRegister (AC97REG_VENDOR_ID1, &wCodecReg);
    ReadCodecRegister (AC97REG_VENDOR_ID2, &wCodecReg);

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97AdapterCommon::AcquireCodecSemiphore
 *****************************************************************************
 * Acquires the AC97 semiphore.  This can not be called at dispatch level
 * because it can timeout if a lower IRQL thread has the semaphore.
 */
NTSTATUS CAC97AdapterCommon::AcquireCodecSemiphore ()
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::AcquireCodecSemiphore]"));

    ULONG ulCount = 0;
    while (READ_PORT_UCHAR (m_pBusMasterBase + CAS) & CAS_CAS)
    {
        //
        // Do we want to give up??
        //
        if (ulCount++ > 100)
        {
            DOUT (DBG_ERROR, ("Cannot acquire semaphore."));
            return STATUS_IO_TIMEOUT;
        }

        //
        // Let's wait a little, 40us and then try again.
        //
        KeStallExecutionProcessor (40L);
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97AdapterCommon::ReadCodecRegister
 *****************************************************************************
 * Reads a AC97 register. Don't call at PASSIVE_LEVEL.
 */
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::ReadCodecRegister
(
    _In_range_(0, AC97REG_INVALID)  AC97Register reg,
    _Out_ PWORD wData
)
{
    PAGED_CODE ();

    ASSERT (wData);
    ASSERT (reg < AC97REG_INVALID);    // audio can only be in the primary codec
    _Analysis_assume_(reg < AC97REG_INVALID);

    NTSTATUS ntStatus;
    ULONG    Status;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ReadCodecRegister]"));

    //
    // Check if we have to access the HW directly.
    //
    if (m_bDirectRead || (m_stAC97Registers[reg].wFlags & SHREG_INVALID) ||
        (m_stAC97Registers[reg].wFlags & SHREG_NOCACHE))
    {
        //
        // Grab the codec access semiphore.
        //
        ntStatus = AcquireCodecSemiphore ();
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("ReadCodecRegister couldn't acquire the semiphore"
                " for reg. %s", reg <= AC97REG_RESERVED2 ? RegStrings[reg] :
                                reg == AC97REG_VENDOR_ID1 ? "REG_VENDOR_ID1" :
                                reg == AC97REG_VENDOR_ID2 ? "REG_VENDOR_ID2" :
                                "REG_INVALID"));
            return ntStatus;
        }

        //
        // Read the data.
        //
        *wData = READ_PORT_USHORT (m_pCodecBase + reg);

        //
        // Check to see if the read was successful.
        //
        Status = READ_PORT_ULONG ((PULONG)(m_pBusMasterBase + GLOB_STA));
        if (Status & GLOB_STA_RCS)
        {
            //
            // clear the timeout bit
            //
            WRITE_PORT_ULONG ((PULONG)(m_pBusMasterBase + GLOB_STA), Status);
            *wData = 0;
            DOUT (DBG_ERROR, ("ReadCodecRegister timed out for register %s",
                    reg <= AC97REG_RESERVED2 ? RegStrings[reg] :
                    reg == AC97REG_VENDOR_ID1 ? "REG_VENDOR_ID1" :
                    reg == AC97REG_VENDOR_ID2 ? "REG_VENDOR_ID2" :
                    "REG_INVALID"));
            return STATUS_IO_TIMEOUT;
        }

        //
        // Clear invalid flag
        //
        m_stAC97Registers[reg].wCache = *wData;
        m_stAC97Registers[reg].wFlags &= ~SHREG_INVALID;

        DOUT (DBG_REGS, ("AC97READ: %s = 0x%04x (HW)",
                reg <= AC97REG_RESERVED2 ? RegStrings[reg] :
                reg == AC97REG_VENDOR_ID1 ? "REG_VENDOR_ID1" :
                reg == AC97REG_VENDOR_ID2 ? "REG_VENDOR_ID2" :
                "REG_INVALID", *wData));
    }
    else
    {
        //
        // Otherwise, use the value in the cache.
        //
        *wData = m_stAC97Registers[reg].wCache;
        DOUT (DBG_REGS, ("AC97READ: %s = 0x%04x (C)",
                reg <= AC97REG_RESERVED2 ? RegStrings[reg] :
                reg == AC97REG_VENDOR_ID1 ? "REG_VENDOR_ID1" :
                reg == AC97REG_VENDOR_ID2 ? "REG_VENDOR_ID2" :
                "REG_INVALID", *wData));
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97AdapterCommon::WriteCodecRegister
 *****************************************************************************
 * Writes to a AC97 register.  This can only be done at passive level because
 * the AcquireCodecSemiphore call could fail!
 */
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::WriteCodecRegister
(
    _In_range_(0, AC97REG_INVALID)  AC97Register reg,
    _In_  WORD wData,
    _In_  WORD wMask
)
{
    PAGED_CODE ();

    ASSERT (reg < AC97REG_INVALID);    // audio can only be in the primary codec

    WORD TempData = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::WriteCodecRegister]"));

    //
    // No mask?  Could happen when you try to prg. left channel of a
    // mono volume.
    //
    if (!wMask)
        return STATUS_SUCCESS;

    //
    // Check to see if we are only writing specific bits.  If so, we want
    // to leave some bits in the register alone.
    //
    if (wMask != 0xffff)
    {
        //
        // Read the current register contents.
        //
        ntStatus = ReadCodecRegister (reg, &TempData);
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("WriteCodecRegiser read for mask failed"));
            return ntStatus;
        }

        //
        // Do the masking.
        //
        TempData &= ~wMask;
        TempData |= (wMask & wData);
    }
    else
    {
        TempData = wData;
    }


    //
    // Grab the codec access semiphore.
    //
    ntStatus = AcquireCodecSemiphore ();
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("WriteCodecRegister failed for register %s",
            reg <= AC97REG_RESERVED2 ? RegStrings[reg] :
            reg == AC97REG_VENDOR_ID1 ? "REG_VENDOR_ID1" :
            reg == AC97REG_VENDOR_ID2 ? "REG_VENDOR_ID2" : "REG_INVALID"));
        return ntStatus;
    }

    //
    // Write the data.
    //
    WRITE_PORT_USHORT (m_pCodecBase + reg, TempData);

    //
    // Update cache.
    //
    _Analysis_assume_(reg < AC97REG_INVALID);
    m_stAC97Registers[reg].wCache = TempData;

    DOUT (DBG_REGS, ("AC97WRITE: %s -> 0x%04x",
                   reg <= AC97REG_RESERVED2 ? RegStrings[reg] :
                   reg == AC97REG_VENDOR_ID1 ? "REG_VENDOR_ID1" :
                   reg == AC97REG_VENDOR_ID2 ? "REG_VENDOR_ID2" :
                   "REG_INVALID", TempData));


    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97AdapterCommon::PrimaryCodecReady
 *****************************************************************************
 * Checks whether the primary codec is present and ready.  This may take
 * awhile if we are bringing it up from a cold reset so give it a second
 * before giving up.
 */
NTSTATUS CAC97AdapterCommon::PrimaryCodecReady (void)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::PrimaryCodecReady]"));


    //
    // Enable the AC link and raise the reset line.
    //
    DWORD dwRegValue = ReadBMControlRegister32 (GLOB_CNT);

    // If someone enabled GPI Interrupt Enable, then he hopefully handles that
    // too.
    dwRegValue = (dwRegValue | GLOB_CNT_COLD) & ~(GLOB_CNT_ACLOFF | GLOB_CNT_PRIE);
    WriteBMControlRegister (GLOB_CNT, dwRegValue);

    //
    // Wait for the Codec to be ready.
    //
    ULONG           WaitCycles = 200;
    LARGE_INTEGER   WaitTime;

    WaitTime.QuadPart = (-50000);   // wait 5000us (5ms) relative

    do
    {
        if (READ_PORT_ULONG ((PULONG)(m_pBusMasterBase + GLOB_STA)) &
            GLOB_STA_PCR)
        {
            return STATUS_SUCCESS;
        }

        KeDelayExecutionThread (KernelMode, FALSE, &WaitTime);
    } while (WaitCycles--);

    DOUT (DBG_ERROR, ("PrimaryCodecReady timed out!"));
    return STATUS_IO_TIMEOUT;
}


/*****************************************************************************
 * CAC97AdapterCommon::PowerUpCodec
 *****************************************************************************
 * Sets the Codec to the highest power state and waits until the Codec reports
 * that the power state is reached.
 */
NTSTATUS CAC97AdapterCommon::PowerUpCodec (void)
{
    PAGED_CODE ();

    WORD        wCodecReg;
    NTSTATUS    ntStatus;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::PowerUpCodec]"));

    //
    // Power up the Codec.
    //
    WriteCodecRegister (AC97REG_POWERDOWN, 0x00, 0xFFFF);

    //
    // Wait for the Codec to be powered up.
    //
    ULONG           WaitCycles = 200;
    LARGE_INTEGER   WaitTime;

    WaitTime.QuadPart = (-50000);   // wait 5000us (5ms) relative

    do
    {
        //
        // Read the power management register.
        //
        ntStatus = ReadCodecRegister (AC97REG_POWERDOWN, &wCodecReg);
        if (!NT_SUCCESS (ntStatus))
        {
            wCodecReg = 0;      // Will cause an error.
            break;
        }

        //
        // Check the power state. Should be ready.
        //
        if ((wCodecReg & 0x0f) == 0x0f)
            break;

        //
        // Let's wait a little, 5ms and then try again.
        //
        KeDelayExecutionThread (KernelMode, FALSE, &WaitTime);
    } while (WaitCycles--);

    // Check if we timed out.
    if ((wCodecReg & 0x0f) != 0x0f)
    {
        DOUT (DBG_ERROR, ("PowerUpCodec timed out. CoDec not powered up."));
        ntStatus = STATUS_DEVICE_NOT_READY;
    }

    return ntStatus;
}


/*****************************************************************************
 * CAC97AdapterCommon::ProgramSampleRate
 *****************************************************************************
 * Programs the sample rate. If the rate cannot be programmed, the routine
 * restores the register and returns STATUS_UNSUCCESSFUL.
 * We don't handle double rate sample rates here, because the Intel AC97 con-
 * troller cannot serve CoDecs with double rate or surround sound. If you want
 * to modify this driver for another AC97 controller, then you might want to
 * change this function too.
 */
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::ProgramSampleRate
(
    IN  AC97Register    Register,
    IN  DWORD           dwSampleRate
)
{
    PAGED_CODE ();

    WORD     wOldRateReg, wCodecReg;
    NTSTATUS ntStatus;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ProgramSampleRate]"));

    //
    // Check if we support variable sample rate.
    //
    switch(Register)
    {
        case AC97REG_MIC_SAMPLERATE:
            //
            // Variable sample rate supported?
            //
            if (GetNodeConfig (NODEC_MIC_VARIABLERATE_SUPPORTED))
            {
                // Range supported?
                if (dwSampleRate > 48000ul)
                {
                    // Not possible.
                    DOUT (DBG_VSR, ("Samplerate %d not supported", dwSampleRate));
                    return STATUS_NOT_SUPPORTED;
                }
            }
            else
            {
                // Only 48000KHz possible.
                if (dwSampleRate != 48000ul)
                {
                    DOUT (DBG_VSR, ("Samplerate %d not supported", dwSampleRate));
                    return STATUS_NOT_SUPPORTED;
                }

                return STATUS_SUCCESS;
            }
            break;

        case AC97REG_FRONT_SAMPLERATE:
        case AC97REG_SURROUND_SAMPLERATE:
        case AC97REG_LFE_SAMPLERATE:
        case AC97REG_RECORD_SAMPLERATE:
            //
            // Variable sample rate supported?
            //
            if (GetNodeConfig (NODEC_PCM_VARIABLERATE_SUPPORTED))
            {
                //
                // Check range supported
                //
                if (dwSampleRate > 48000ul)
                {
                    DOUT (DBG_VSR, ("Samplerate %d not supported", dwSampleRate));
                    return STATUS_NOT_SUPPORTED;
                }
            }
            else
            {
                // Only 48KHz possible.
                if (dwSampleRate != 48000ul)
                {
                    DOUT (DBG_VSR, ("Samplerate %d not supported", dwSampleRate));
                    return STATUS_NOT_SUPPORTED;
                }

                return STATUS_SUCCESS;
            }
            break;

        default:
            DOUT (DBG_ERROR, ("Invalid sample rate register!"));
            return STATUS_UNSUCCESSFUL;
    }


    //
    // Save the old sample rate register.
    //
    ntStatus = ReadCodecRegister (Register, &wOldRateReg);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // program the rate.
    //
    ntStatus = WriteCodecRegister (Register, (WORD)dwSampleRate, 0xFFFF);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Cannot program sample rate."));
        return ntStatus;
    }

    //
    // Read it back.
    //
    ntStatus = ReadCodecRegister (Register, &wCodecReg);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Cannot read sample rate."));
        return ntStatus;
    }

    //
    // Validate.
    //
    if (wCodecReg != dwSampleRate)
    {
        //
        // restore sample rate and ctrl register.
        //
        WriteCodecRegister (Register, wOldRateReg, 0xFFFF);

        DOUT (DBG_VSR, ("Samplerate %d not supported", dwSampleRate));
        return STATUS_NOT_SUPPORTED;
    }

    DOUT (DBG_VSR, ("Samplerate changed to %d.", dwSampleRate));
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97AdapterCommon::PowerChangeState
 *****************************************************************************
 * Change power state for the device.  We handle the codec, PowerChangeNotify
 * in the wave miniport handles the DMA registers.
 */
STDMETHODIMP_(void) CAC97AdapterCommon::PowerChangeState
(
    _In_  POWER_STATE NewState
)
{
    PAGED_CODE ();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::PowerChangeNotify]"));

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
    // Switch on new state.
    //
    switch (NewState.DeviceState)
    {
        case PowerDeviceD0:
            //
            // If we are coming from D2 or D3 we have to restore the registers cause
            // there might have been a power loss.
            //
            if ((m_PowerState == PowerDeviceD3) || (m_PowerState == PowerDeviceD2))
            {
                //
                // Reset AD3 to indicate that we are now awake.
                // Because the system has only one power irp at a time, we are sure
                // that the modem driver doesn't get called while we are restoring
                // power.
                //
                WriteBMControlRegister (GLOB_STA,
                    ReadBMControlRegister32 (GLOB_STA) & ~GLOB_STA_AD3);

                //
                // Restore codec registers.
                //
                ntStatus = RestoreCodecRegisters ();
            }
            else        // We are coming from power state D1
            {
                ntStatus = PowerUpCodec ();
            }

            // Print error code.
            if (!NT_SUCCESS (ntStatus))
            {
                DOUT (DBG_ERROR, ("PowerChangeState failed to restore the codec."));
            }
            break;

        case PowerDeviceD1:
            //
            // This sleep state is the lowest latency sleep state with respect
            // to the latency time required to return to D0. If the
            // driver is not being used an inactivity timer in portcls will
            // place the driver in this state after a timeout period
            // controllable via the registry.
            //

            // Let's power down the DAC/ADC's and analog mixer.
            WriteCodecRegister (AC97REG_POWERDOWN, 0x0700, 0xFFFF);
            break;

        case PowerDeviceD2:
        case PowerDeviceD3:
            //
            // This is a full hibernation state and is the longest latency sleep
            // state. In this modes the power could be removed or reduced that
            // much that the AC97 controller looses information, so we save
            // whatever we have to save.
            //

            //
            // Powerdown ADC, DAC, Mixer, Vref, HP amp, and Exernal Amp but not
            // AC-link and Clk
            //
            WriteCodecRegister (AC97REG_POWERDOWN, 0xCF00, 0xFFFF);

            //
            // Only in D3 mode we set the AD3 bit and evtl. shut off the AC link.
            //
            if (NewState.DeviceState == PowerDeviceD3)
            {
                //
                // Set the AD3 bit.
                //
                ULONG ulReg = ReadBMControlRegister32 (GLOB_STA);
                WriteBMControlRegister (GLOB_STA, ulReg | GLOB_STA_AD3);

                //
                // We check if the modem is sleeping. If it is, we can shut off the
                // AC link also. We shut off the AC link also if the modem is not
                // there.
                //
                if ((ulReg & GLOB_STA_MD3) || !(ulReg & GLOB_STA_SCR))
                {
                    // Set Codec to super sleep
                    WriteCodecRegister (AC97REG_POWERDOWN, 0xFF00, 0xFFFF);

                    // Disable the AC-link signals
                    ulReg = ReadBMControlRegister32 (GLOB_CNT);
                    WriteBMControlRegister (GLOB_CNT, (ulReg | GLOB_CNT_ACLOFF) & ~GLOB_CNT_COLD);
                }
            }
            break;
    }

    //
    // Save the new state.  This local value is used to determine when to
    // cache property accesses and when to permit the driver from accessing
    // the hardware.
    //
    m_PowerState = NewState.DeviceState;
    DOUT (DBG_POWER, ("Entering D%d", (ULONG)m_PowerState -
                      (ULONG)PowerDeviceD0));
}


/*****************************************************************************
 * CAC97AdapterCommon::QueryPowerChangeState
 *****************************************************************************
 * Query to see if the device can change to this power state
 */
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::QueryPowerChangeState
(
    _In_  POWER_STATE NewState
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::QueryPowerChangeState]"));

    // Check here to see of a legitimate state is being requested
    // based on the device state and fail the call if the device/driver
    // cannot support the change requested.  Otherwise, return STATUS_SUCCESS.
    // Note: A QueryPowerChangeState() call is not guaranteed to always preceed
    // a PowerChangeState() call.

    // check the new state being requested
    switch (NewState.DeviceState)
    {
        case PowerDeviceD0:
        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            return STATUS_SUCCESS;

        default:
            DOUT (DBG_ERROR, ("Unknown device state: D%d.",
                 (ULONG)NewState.DeviceState - (ULONG)PowerDeviceD0));
            return STATUS_NOT_IMPLEMENTED;
    }
}


/*****************************************************************************
 * CAC97AdapterCommon::QueryDeviceCapabilities
 *****************************************************************************
 * Called at startup to get the caps for the device.  This structure provides
 * the system with the mappings between system power state and device power
 * state.  This typically will not need modification by the driver.
 * If the driver modifies these mappings then the driver is not allowed to
 * change the mapping to a weaker power state (e.g. from S1->D3 to S1->D1).
 *
 */
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS) CAC97AdapterCommon::QueryDeviceCapabilities
(
    PDEVICE_CAPABILITIES PowerDeviceCaps
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::QueryDeviceCapabilities]"));

    UNREFERENCED_PARAMETER(PowerDeviceCaps);

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97AdapterCommon::RestoreAC97Registers
 *****************************************************************************
 * Preset the AC97 registers with default values. The routine first checks if
 * There are registry entries for the default values. If not, we have hard
 * coded values too ;)
 */
NTSTATUS CAC97AdapterCommon::SetAC97Default (void)
{
    PAGED_CODE ();

    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;
    UNICODE_STRING  sKeyName;
    ULONG           ulDisposition;
    ULONG           ulResultLength;
    PVOID           KeyInfo = NULL;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::SetAC97Default]"));

    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey (&DriverKey,        // IRegistryKey
                                          NULL,              // OuterUnknown
                                          DriverRegistryKey, // Registry key type
                                          KEY_READ,          // Access flags
                                          m_pDeviceObject,   // Device object
                                          NULL,              // Subdevice
                                          NULL,              // ObjectAttributes
                                          0,                 // Create options
                                          NULL);             // Disposition
    if (NT_SUCCESS (ntStatus))
    {
        // make a unicode string for the subkey name
        RtlInitUnicodeString (&sKeyName, L"Settings");

        // open the settings subkey
        ntStatus = DriverKey->NewSubKey (&SettingsKey,            // Subkey
                                         NULL,                    // OuterUnknown
                                         KEY_READ,                // Access flags
                                         &sKeyName,               // Subkey name
                                         REG_OPTION_NON_VOLATILE, // Create options
                                         &ulDisposition);

        if (NT_SUCCESS (ntStatus))
        {
            // allocate data to hold key info
            KeyInfo = ExAllocatePoolWithTag (PagedPool,
                                      sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                      sizeof(WORD), PoolTag);
            if (NULL != KeyInfo)
            {
                // loop through all mixer settings
                for (AC97Register i = AC97REG_RESET; i <= AC97REG_RESERVED2;
                    i = (AC97Register)(i + 1))
                {
                    if (m_stAC97Registers[i].wFlags & SHREG_INIT)
                    {
                        // init key name
                        RtlInitUnicodeString (&sKeyName,
                                              m_stAC97Registers[i].sRegistryName);

                        // query the value key
                        ntStatus = SettingsKey->QueryValueKey (&sKeyName,
                                        KeyValuePartialInformation,
                                        KeyInfo,
                                        sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                            sizeof(WORD),
                                        &ulResultLength);
                        if (NT_SUCCESS (ntStatus))
                        {
                            PKEY_VALUE_PARTIAL_INFORMATION PartialInfo =
                                        (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;

                            if (PartialInfo->DataLength == sizeof(WORD))
                            {
                                // set mixer register to registry value
                                WriteCodecRegister
                                    (i, *(PWORD)PartialInfo->Data, 0xFFFF);
                            }
                            else    // write the hard coded default
                            {
                                // if key access failed, set to default
                                WriteCodecRegister
                                    (i, m_stAC97Registers[i].wWantedDefault, 0xFFFF);
                            }
                        }
                        else  // write the hard coded default
                        {
                            // if key access failed, set to default
                            WriteCodecRegister
                                (i, m_stAC97Registers[i].wWantedDefault, 0xFFFF);
                        }
                    }
                }

                // we want to return status success even if the last QueryValueKey
                // failed.
                ntStatus = STATUS_SUCCESS;

                // free the key info
                ExFreePoolWithTag (KeyInfo,PoolTag);
            }
            else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            // release the settings key
            SettingsKey->Release ();
        }

        // release the driver key
        DriverKey->Release ();
    }


    // in case we did not query the registry (cause of lack of resources)
    // restore default values and return insufficient resources.
    if (!NT_SUCCESS (ntStatus))
    {
        // copy hard coded default settings
        for (AC97Register i = AC97REG_RESET; i < AC97REG_RESERVED2;
             i = (AC97Register)(i + 1))
        {
            if (m_stAC97Registers[i].wFlags & SHREG_INIT)
            {
                WriteCodecRegister (i, m_stAC97Registers[i].wWantedDefault, 0xFFFF);
            }
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * CAC97AdapterCommon::DisableAC97Pin
 *****************************************************************************
 * Returns TRUE when the HW vendor wants to disable the pin. A disabled pin is
 * not shown to the user (means it is not included in the topology). The
 * reason for doing this could be that some of the input lines like Aux or
 * Video are not available to the user (to plug in something) but the codec
 * can handle those lines.
 */
BOOL CAC97AdapterCommon::DisableAC97Pin
(
    IN  TopoPinConfig pin
)
{
    PAGED_CODE ();

    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;
    UNICODE_STRING  sKeyName;
    ULONG           ulDisposition;
    ULONG           ulResultLength;
    PVOID           KeyInfo = NULL;
    BOOL            bDisable = FALSE;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::DisableAC97Pin]"));

    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey (&DriverKey,        // IRegistryKey
                                          NULL,              // OuterUnknown
                                          DriverRegistryKey, // Registry key type
                                          KEY_READ,          // Access flags
                                          m_pDeviceObject,   // Device object
                                          NULL,              // Subdevice
                                          NULL,              // ObjectAttributes
                                          0,                 // Create options
                                          NULL);             // Disposition
    if (NT_SUCCESS (ntStatus))
    {
        // make a unicode string for the subkey name
        RtlInitUnicodeString (&sKeyName, L"Settings");

        // open the settings subkey
        ntStatus = DriverKey->NewSubKey (&SettingsKey,            // Subkey
                                         NULL,                    // OuterUnknown
                                         KEY_READ,                // Access flags
                                         &sKeyName,               // Subkey name
                                         REG_OPTION_NON_VOLATILE, // Create options
                                         &ulDisposition);

        if (NT_SUCCESS (ntStatus))
        {
            // allocate data to hold key info
            KeyInfo = ExAllocatePoolWithTag (PagedPool,
                                      sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                      sizeof(BYTE), PoolTag);
            if (NULL != KeyInfo)
            {
                // init key name
                RtlInitUnicodeString (&sKeyName, m_stHardwareConfig.
                                            Pins[pin].sRegistryName);

                // query the value key
                ntStatus = SettingsKey->QueryValueKey (&sKeyName,
                                   KeyValuePartialInformation,
                                   KeyInfo,
                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                        sizeof(BYTE),
                                   &ulResultLength );
                if (NT_SUCCESS (ntStatus))
                {
                    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo =
                                (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;

                    if (PartialInfo->DataLength == sizeof(BYTE))
                    {
                        // store the value
                        if (*(PBYTE)PartialInfo->Data)
                            bDisable = TRUE;
                        else
                            bDisable = FALSE;
                    }
                }

                // free the key info
                ExFreePoolWithTag (KeyInfo,PoolTag);
            }

            // release the settings key
            SettingsKey->Release ();
        }

        // release the driver key
        DriverKey->Release ();
    }

    // if one of the stuff above fails we return the default, which is FALSE.
    return bDisable;
}


/*****************************************************************************
 * CAC97AdapterCommon::RestoreCodecRegisters
 *****************************************************************************
 * write back cached mixer values to codec registers
 */
NTSTATUS CAC97AdapterCommon::RestoreCodecRegisters (void)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::RestoreCodecRegisters]"));

    //
    // Initialize the AC97 codec.
    //
    NTSTATUS ntStatus = InitAC97 ();
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Restore all codec registers.  Failure is not critical.
    //
    for (AC97Register i = AC97REG_MASTER_VOLUME; i < AC97REG_RESERVED2;
        i = (AC97Register)(i + 1))
    {
        WriteCodecRegister (i, m_stAC97Registers[i].wCache, 0xFFFF);
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAC97AdapterCommon::ReadChannelConfigDefault
 *****************************************************************************
 * This function reads the default channel config from the registry. The
 * registry entry "ChannelConfig" is set every every time we get a
 * KSPROPERTY_AUDIO_CHANNEL_CONFIG for the DAC node.
 * In case the key doesn't exist we assume a channel config of stereo speakers,
 * cause that is the default of DSOUND.
 */
STDMETHODIMP_(void) CAC97AdapterCommon::ReadChannelConfigDefault
(
    PDWORD  pdwChannelConfig,
    PWORD   pwChannels
)
{
    PAGED_CODE ();

    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;
    UNICODE_STRING  sKeyName;
    ULONG           ulDisposition;
    ULONG           ulResultLength;
    PVOID           KeyInfo = NULL;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ReadChannelConfigDefault]"));

    // This is the default: 2 speakers, stereo.
    *pdwChannelConfig = KSAUDIO_SPEAKER_STEREO;
    *pwChannels = 2;

    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey (&DriverKey,        // IRegistryKey
                                          NULL,              // OuterUnknown
                                          DriverRegistryKey, // Registry key type
                                          KEY_READ,          // Access flags
                                          m_pDeviceObject,   // Device object
                                          NULL,              // Subdevice
                                          NULL,              // ObjectAttributes
                                          0,                 // Create options
                                          NULL);             // Disposition
    if (NT_SUCCESS (ntStatus))
    {
        // make a unicode string for the subkey name
        RtlInitUnicodeString (&sKeyName, L"Settings");

        // open the settings subkey
        ntStatus = DriverKey->NewSubKey (&SettingsKey,            // Subkey
                                         NULL,                    // OuterUnknown
                                         KEY_READ,                // Access flags
                                         &sKeyName,               // Subkey name
                                         REG_OPTION_NON_VOLATILE, // Create options
                                         &ulDisposition);

        if (NT_SUCCESS (ntStatus))
        {
            // allocate data to hold key info
            KeyInfo = ExAllocatePoolWithTag (PagedPool,
                                      sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                      sizeof(DWORD), PoolTag);
            if (NULL != KeyInfo)
            {
                // init key name
                RtlInitUnicodeString (&sKeyName, L"ChannelConfig");

                // query the value key
                ntStatus = SettingsKey->QueryValueKey (&sKeyName,
                                   KeyValuePartialInformation,
                                   KeyInfo,
                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                        sizeof(DWORD),
                                   &ulResultLength );
                if (NT_SUCCESS (ntStatus))
                {
                    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo =
                                (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;

                    if (PartialInfo->DataLength == sizeof(DWORD))
                    {
                        switch (*(PLONG)PartialInfo->Data)
                        {
                        case KSAUDIO_SPEAKER_QUAD:
                        case KSAUDIO_SPEAKER_SURROUND:
                            if (GetPinConfig (PINC_SURROUND_PRESENT))
                            {
                                *pdwChannelConfig = *(PDWORD)PartialInfo->Data;
                                *pwChannels = 4;
                            }
                            break;

                        case KSAUDIO_SPEAKER_5POINT1:
                            if (GetPinConfig (PINC_SURROUND_PRESENT) &&
                                GetPinConfig (PINC_CENTER_LFE_PRESENT))
                            {
                                *pdwChannelConfig = *(PDWORD)PartialInfo->Data;
                                *pwChannels = 6;
                            }
                            break;
                        }
                    }
                }

                // free the key info
                ExFreePoolWithTag (KeyInfo,PoolTag);
            }

            // release the settings key
            SettingsKey->Release ();
        }

        // release the driver key
        DriverKey->Release ();
    }
}

/*****************************************************************************
 * CAC97AdapterCommon::WriteChannelConfigDefault
 *****************************************************************************
 * This function writes the default channel config to the registry. The
 * registry entry "ChannelConfig" is set every every time we get a
 * KSPROPERTY_AUDIO_CHANNEL_CONFIG for the DAC node.
 */
STDMETHODIMP_(void) CAC97AdapterCommon::WriteChannelConfigDefault (DWORD dwChannelConfig)
{
    PAGED_CODE ();

    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;
    UNICODE_STRING  sKeyName;
    ULONG           ulDisposition;

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::WriteChannelConfigDefault]"));

    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey (&DriverKey,        // IRegistryKey
                                          NULL,              // OuterUnknown
                                          DriverRegistryKey, // Registry key type
                                          KEY_WRITE,         // Access flags
                                          m_pDeviceObject,   // Device object
                                          NULL,              // Subdevice
                                          NULL,              // ObjectAttributes
                                          0,                 // Create options
                                          NULL);             // Disposition
    if (NT_SUCCESS (ntStatus))
    {
        // make a unicode string for the subkey name
        RtlInitUnicodeString (&sKeyName, L"Settings");

        // open the settings subkey
        ntStatus = DriverKey->NewSubKey (&SettingsKey,            // Subkey
                                         NULL,                    // OuterUnknown
                                         KEY_WRITE,               // Access flags
                                         &sKeyName,               // Subkey name
                                         REG_OPTION_NON_VOLATILE, // Create options
                                         &ulDisposition);

        if (NT_SUCCESS (ntStatus))
        {
            // init key name
            RtlInitUnicodeString (&sKeyName, L"ChannelConfig");

            // query the value key
            ntStatus = SettingsKey->SetValueKey (&sKeyName,
                                                 REG_DWORD,
                                                 &dwChannelConfig,
                                                 sizeof (DWORD));
            if (!NT_SUCCESS (ntStatus))
            {
                DOUT (DBG_ERROR, ("Could not write the ChannelConfig to registry."));
            }

            // release the settings key
            SettingsKey->Release ();
        }

        // release the driver key
        DriverKey->Release ();
    }
}

/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#pragma code_seg()
/*****************************************************************************
 * CAC97AdapterCommon::WriteBMControlRegister
 *****************************************************************************
 * Writes a byte (UCHAR) to BusMaster Control register.
 */
STDMETHODIMP_(void) CAC97AdapterCommon::WriteBMControlRegister
(
    IN  ULONG ulOffset,
    IN  UCHAR ucValue
)
{
    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::WriteBMControlRegister] (UCHAR)"));

    WRITE_PORT_UCHAR ((PUCHAR)(m_pBusMasterBase + ulOffset), ucValue);

    DOUT (DBG_REGS, ("WriteBMControlRegister wrote 0x%2x to 0x%4p.",
                   ucValue, m_pBusMasterBase + ulOffset));
}

/*****************************************************************************
 * CAC97AdapterCommon::WriteBMControlRegister
 *****************************************************************************
 * Writes a word (USHORT) to BusMaster Control register.
 */
STDMETHODIMP_(void) CAC97AdapterCommon::WriteBMControlRegister
(
    IN  ULONG ulOffset,
    IN  USHORT usValue
)
{
    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::WriteBMControlRegister (USHORT)]"));

    WRITE_PORT_USHORT ((PUSHORT)(m_pBusMasterBase + ulOffset), usValue);

    DOUT (DBG_REGS, ("WriteBMControlRegister wrote 0x%4x to 0x%4p",
                   usValue, m_pBusMasterBase + ulOffset));
}

/*****************************************************************************
 * CAC97AdapterCommon::WriteBMControlRegister
 *****************************************************************************
 * Writes a DWORD (ULONG) to BusMaster Control register.
 */
STDMETHODIMP_(void) CAC97AdapterCommon::WriteBMControlRegister
(
    IN  ULONG ulOffset,
    IN  ULONG ulValue
)
{
    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::WriteBMControlRegister (ULONG)]"));

    WRITE_PORT_ULONG ((PULONG)(m_pBusMasterBase + ulOffset), ulValue);

    DOUT (DBG_REGS, ("WriteBMControlRegister wrote 0x%8x to 0x%4p.",
                   ulValue, m_pBusMasterBase + ulOffset));
}

/*****************************************************************************
 * CAC97AdapterCommon::ReadBMControlRegister8
 *****************************************************************************
 * Read a byte (UCHAR) from BusMaster Control register.
 */
STDMETHODIMP_(UCHAR) CAC97AdapterCommon::ReadBMControlRegister8
(
    IN  ULONG ulOffset
)
{
    UCHAR ucValue = UCHAR(-1);

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ReadBMControlRegister8]"));

    ucValue = READ_PORT_UCHAR ((PUCHAR)(m_pBusMasterBase + ulOffset));

    DOUT (DBG_REGS, ("ReadBMControlRegister read 0x%2x from 0x%4p.", ucValue,
                   m_pBusMasterBase + ulOffset));

    return ucValue;
}

/*****************************************************************************
 * CAC97AdapterCommon::ReadBMControlRegister16
 *****************************************************************************
 * Read a word (USHORT) from BusMaster Control register.
 */
STDMETHODIMP_(USHORT) CAC97AdapterCommon::ReadBMControlRegister16
(
    IN  ULONG ulOffset
)
{
    USHORT usValue = USHORT(-1);

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ReadBMControlRegister16]"));

    usValue = READ_PORT_USHORT ((PUSHORT)(m_pBusMasterBase + ulOffset));

    DOUT (DBG_REGS, ("ReadBMControlRegister read 0x%4x = 0x%4p", usValue,
                   m_pBusMasterBase + ulOffset));

    return usValue;
}

/*****************************************************************************
 * CAC97AdapterCommon::ReadBMControlRegister32
 *****************************************************************************
 * Read a dword (ULONG) from BusMaster Control register.
 */
STDMETHODIMP_(ULONG) CAC97AdapterCommon::ReadBMControlRegister32
(
    IN  ULONG ulOffset
)
{
    ULONG ulValue = ULONG(-1);

    DOUT (DBG_PRINT, ("[CAC97AdapterCommon::ReadBMControlRegister32]"));

    ulValue = READ_PORT_ULONG ((PULONG)(m_pBusMasterBase + ulOffset));

    DOUT (DBG_REGS, ("ReadBMControlRegister read 0x%8x = 0x%4p", ulValue,
                   m_pBusMasterBase + ulOffset));

    return ulValue;
}

