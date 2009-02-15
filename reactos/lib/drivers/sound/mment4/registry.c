/*
 * PROJECT:     ReactOS Sound System "MME Buddy" NT4 Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mment4/registry.c
 *
 * PURPOSE:     Registry operation helper for audio device drivers.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>

#include <reactos/sndnames.h>
#include <reactos/sndtypes.h>

#include <mmebuddy.h>
#include <mment4.h>

/*
    Open the parameters key of a sound driver.
    NT4 only.
*/
MMRESULT
OpenSoundDriverParametersRegKey(
    IN  LPWSTR ServiceName,
    OUT PHKEY KeyHandle)
{
    ULONG KeyLength;
    PWCHAR ParametersKeyName;

    VALIDATE_MMSYS_PARAMETER( ServiceName );
    VALIDATE_MMSYS_PARAMETER( KeyHandle );

    /* Work out how long the string will be */
    KeyLength = wcslen(REG_SERVICES_KEY_NAME_U) + 1
              + wcslen(ServiceName) + 1
              + wcslen(REG_PARAMETERS_KEY_NAME_U);

    /* Allocate memory for the string */
    ParametersKeyName = AllocateWideString(KeyLength);

    if ( ! ParametersKeyName )
        return MMSYSERR_NOMEM;

    /* Construct the registry path */
    wsprintf(ParametersKeyName,
             L"%s\\%s\\%s",
             REG_SERVICES_KEY_NAME_U,
             ServiceName,
             REG_PARAMETERS_KEY_NAME_U);

    SND_TRACE(L"Opening reg key: %wS\n", ParametersKeyName);

    /* Perform the open */
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      ParametersKeyName,
                      0,
                      KEY_READ,
                      KeyHandle) != ERROR_SUCCESS )
    {
        /* Couldn't open the key */
        SND_ERR(L"Failed to open reg key: %wS\n", ParametersKeyName);
        FreeMemory(ParametersKeyName);
        return MMSYSERR_ERROR;
    }

    FreeMemory(ParametersKeyName);

    return MMSYSERR_NOERROR;
}

/*
    Open one of the Device sub-keys belonging to the sound driver.
    NT4 only.
*/
MMRESULT
OpenSoundDeviceRegKey(
    IN  LPWSTR ServiceName,
    IN  DWORD DeviceIndex,
    OUT PHKEY KeyHandle)
{
    DWORD PathLength;
    PWCHAR RegPath;

    VALIDATE_MMSYS_PARAMETER( ServiceName );
    VALIDATE_MMSYS_PARAMETER( KeyHandle );

    /*
        Work out the space required to hold the path:

        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\
            sndblst\
                Parameters\
                    Device123\
    */
    PathLength = wcslen(REG_SERVICES_KEY_NAME_U) + 1
               + wcslen(ServiceName) + 1
               + wcslen(REG_PARAMETERS_KEY_NAME_U) + 1
               + wcslen(REG_DEVICE_KEY_NAME_U)
               + GetDigitCount(DeviceIndex);

    /* Allocate storage for the string */
    RegPath = AllocateWideString(PathLength);

    if ( ! RegPath )
    {
        return MMSYSERR_NOMEM;
    }

    /* Write the path */
    wsprintf(RegPath,
             L"%ls\\%ls\\%ls\\%ls%d",
             REG_SERVICES_KEY_NAME_U,
             ServiceName,
             REG_PARAMETERS_KEY_NAME_U,
             REG_DEVICE_KEY_NAME_U,
             DeviceIndex);

    SND_TRACE(L"Opening reg key: %wS\n", RegPath);

    /* Perform the open */
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      RegPath,
                      0,
                      KEY_READ,
                      KeyHandle) != ERROR_SUCCESS )
    {
        /* Couldn't open the key */
        SND_ERR(L"Failed to open reg key: %wS\n", RegPath);
        FreeMemory(RegPath);
        return MMSYSERR_ERROR;
    }

    FreeMemory(RegPath);

    return MMSYSERR_NOERROR;
}
