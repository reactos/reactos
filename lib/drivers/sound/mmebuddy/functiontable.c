/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/functiontable.c
 *
 * PURPOSE:     Routes function calls through a function table, calling
 *              implementation-defined routines or a default function, depending
 *              on configuration.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmebuddy.h>

/*
    Attaches a function table to a sound device. Any NULL entries in this
    table are automatically set to point to a default routine to handle
    the appropriate function.
*/
MMRESULT
SetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PMMFUNCTION_TABLE FunctionTable)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( FunctionTable );

    /* Zero out the existing function table (if present) */
    ZeroMemory(&SoundDevice->FunctionTable, sizeof(MMFUNCTION_TABLE));

    if ( ! FunctionTable )
        return MMSYSERR_INVALPARAM;

    /* Fill in the client-supplied functions */
    CopyMemory(&SoundDevice->FunctionTable,
                FunctionTable,
                sizeof(MMFUNCTION_TABLE));

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
