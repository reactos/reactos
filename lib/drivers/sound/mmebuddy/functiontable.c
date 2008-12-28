/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/functiontable.c
 *
 * PURPOSE:     Routes function calls through a function table, calling
 *              implementation-defined routines or a default function, depending
 *              on configuration.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmebuddy.h>

/*
    Attaches a function table to a sound device. Any NULL entries in this
    table are automatically set to point to a default routine to handle
    the appropriate function. If NULL is passed as the function table itself,
    the entire function table will use only the default routines.
*/
MMRESULT
SetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PMMFUNCTION_TABLE FunctionTable OPTIONAL)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    /*VALIDATE_MMSYS_PARAMETER( FunctionTable );*/

    /* Zero out the existing function table (if present) */
    ZeroMemory(&SoundDevice->FunctionTable, sizeof(MMFUNCTION_TABLE));

    /* Fill in the client-supplied functions, if provided */
    if ( FunctionTable )
    {
        CopyMemory(&SoundDevice->FunctionTable,
                   FunctionTable,
                   sizeof(MMFUNCTION_TABLE));
    }

    /* Plug any gaps in the function table */
    if ( ! SoundDevice->FunctionTable.GetCapabilities )
    {
        SoundDevice->FunctionTable.GetCapabilities =
            DefaultGetSoundDeviceCapabilities;
    }

    if ( ! SoundDevice->FunctionTable.QueryWaveFormatSupport )
    {
        SoundDevice->FunctionTable.QueryWaveFormatSupport =
            DefaultQueryWaveDeviceFormatSupport;
    }

    if ( ! SoundDevice->FunctionTable.SetWaveFormat )
    {
        SoundDevice->FunctionTable.SetWaveFormat =
            DefaultSetWaveDeviceFormat;
    }

    return MMSYSERR_NOERROR;
}

/*
    Retrieves the function table for a sound device, as previously set using
    SetSoundDeviceFunctionTable.
*/
MMRESULT
GetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PMMFUNCTION_TABLE* FunctionTable)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( FunctionTable );

    *FunctionTable = &SoundDevice->FunctionTable;

    return MMSYSERR_NOERROR;
}
