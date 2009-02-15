/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/capabilities.c
 *
 * PURPOSE:     Queries sound devices for their capabilities.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <sndtypes.h>
#include <mmebuddy.h>

/*
    Obtains the capabilities of a sound device. This routine ensures that the
    supplied CapabilitiesSize parameter at least meets the minimum size of the
    relevant capabilities structure.

    Ultimately, it will call the GetCapabilities function specified in the
    sound device's function table. Note that there are several of these, in a
    union. This is simply to avoid manually typecasting when implementing the
    functions.
*/
MMRESULT
GetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMDEVICE_TYPE DeviceType;
    PMMFUNCTION_TABLE FunctionTable;
    BOOLEAN GoodSize = FALSE;
    MMRESULT Result;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Capabilities );
    VALIDATE_MMSYS_PARAMETER( CapabilitiesSize > 0 );

    /* Obtain the device type */
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Obtain the function table */
    Result = GetSoundDeviceFunctionTable(SoundDevice, &FunctionTable);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( ! MMSUCCESS(Result) )
        return TranslateInternalMmResult(Result);

    /* Check that the capabilities structure is of a valid size */
    switch ( DeviceType )
    {
        case WAVE_OUT_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(WAVEOUTCAPS);
            break;
        }
        case WAVE_IN_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(WAVEINCAPS);
            break;
        }
        case MIDI_OUT_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(MIDIOUTCAPS);
            break;
        }
        case MIDI_IN_DEVICE_TYPE :
        {
            GoodSize = CapabilitiesSize >= sizeof(MIDIINCAPS);
            break;
        }
        /* TODO: Others... */
        default :
        {
            SND_ASSERT(FALSE);
        }
    };

    if ( ! GoodSize )
    {
        SND_ERR(L"Device capabilities structure too small\n");
        return MMSYSERR_INVALPARAM;
    }

    /* Call the "get capabilities" function within the function table */
    SND_ASSERT( FunctionTable->GetCapabilities );

    if ( ! FunctionTable->GetCapabilities )
        return MMSYSERR_NOTSUPPORTED;

    return FunctionTable->GetCapabilities(SoundDevice,
                                          Capabilities,
                                          CapabilitiesSize);
}
