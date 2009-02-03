/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/reentrancy.c
 *
 * PURPOSE:     Provides entry-point mutex guards.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

HANDLE EntrypointMutexes[SOUND_DEVICE_TYPES];

/*
    Creates a set of mutexes which are used for the purpose of guarding the
    device-type specific module entry-points. If any of these fail creation,
    all of them will be destroyed and the failure reported.
*/
MMRESULT
InitEntrypointMutexes()
{
    UCHAR i;
    MMRESULT Result = MMSYSERR_NOERROR;

    /* Blank all entries ni the table first */
    for ( i = 0; i < SOUND_DEVICE_TYPES; ++ i )
    {
        EntrypointMutexes[i] = NULL;
    }

    /* Now create the mutexes */
    for ( i = 0; i < SOUND_DEVICE_TYPES; ++ i )
    {
        EntrypointMutexes[i] = CreateMutex(NULL, FALSE, NULL);

        if ( ! EntrypointMutexes[i] )
        {
            Result = Win32ErrorToMmResult(GetLastError());

            /* Clean up any mutexes we successfully created */
            CleanupEntrypointMutexes();
            break;
        }
    }

    return Result;
}

/*
    Cleans up any of the entry-point guard mutexes. This will only close the
    handles of mutexes which have been created, making it safe for use as a
    cleanup routine even within the InitEntrypointMutexes routine above.
*/
VOID
CleanupEntrypointMutexes()
{
    UCHAR i;

    /* Only clean up a mutex if it actually exists */
    for ( i = 0; i < SOUND_DEVICE_TYPES; ++ i )
    {
        if ( EntrypointMutexes[i] )
        {
            CloseHandle(EntrypointMutexes[i]);
            EntrypointMutexes[i] = NULL;
        }
    }
}

/*
    Grabs an entry-point mutex.
*/
VOID
AcquireEntrypointMutex(
    IN  MMDEVICE_TYPE DeviceType)
{
    UCHAR i;

    SND_ASSERT( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );
    i = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    SND_ASSERT( EntrypointMutexes[i] );

    WaitForSingleObject(EntrypointMutexes[i], INFINITE);
}

/*
    Releases an entry-point mutex.
*/
VOID
ReleaseEntrypointMutex(
    IN  MMDEVICE_TYPE DeviceType)
{
    UCHAR i;

    SND_ASSERT( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );
    i = SOUND_DEVICE_TYPE_TO_INDEX(DeviceType);

    SND_ASSERT( EntrypointMutexes[i] );

    ReleaseMutex(EntrypointMutexes[i]);
}
