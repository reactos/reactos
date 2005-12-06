/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/wavehdr.c
 * PURPOSE:             WDM Audio Support - Device Control (Play/Stop etc.)
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 23, 2005: Created
 */

#include <windows.h>
#include "wdmaud.h"

/*
    TODO:
    Make these work for the other device types!
*/

MMRESULT StartDevice(PWDMAUD_DEVICE_INFO device)
{
    MMRESULT result;
    DWORD ioctl_code;

    result = ValidateDeviceInfoAndState(device);

    if ( result != MMSYSERR_NOERROR )
        return result;

    ioctl_code = device == WDMAUD_WAVE_IN ? IOCTL_WDMAUD_WAVE_IN_START :
                 device == WDMAUD_WAVE_OUT ? IOCTL_WDMAUD_WAVE_OUT_START :
                 0x0000;

    ASSERT( ioctl_code );
}

MMRESULT StopDevice(PWDMAUD_DEVICE_INFO device)
{
}

MMRESULT PauseDevice(PWDMAUD_DEVICE_INFO device)
{
}

MMRESULT StopDeviceLooping(PWDMAUD_DEVICE_INFO device)
{
}
