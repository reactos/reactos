/***
*slbeep.c - Sleep and beep
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _sleep() and _beep()
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <stdlib.h>

/***
*void _sleep(duration) - Length of sleep
*
*Purpose:
*
*Entry:
*       unsigned long duration - length of sleep in milliseconds or
*       one of the following special values:
*
*           _SLEEP_MINIMUM - Sends a yield message without any delay
*           _SLEEP_FOREVER - Never return
*
*Exit:
*       None
*
*Exceptions:
*
*******************************************************************************/

extern "C" void __cdecl _sleep(unsigned long duration)
{
    if (duration == 0)
        ++duration;
    
    Sleep(duration);
}

/***
*void _beep(frequency, duration) - Length of sleep
*
*Purpose:
*
*Entry:
*       unsigned frequency - frequency in hertz
*       unsigned duration - length of beep in milliseconds
*
*Exit:
*       None
*
*Exceptions:
*
*******************************************************************************/

extern "C" void __cdecl _beep(unsigned const frequency, unsigned const duration)
{
    Beep(frequency, duration);
}
