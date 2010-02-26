/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/rtc.c
 * PURPOSE:         Real Time Clock and Environment Variable Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

#define RTC_DATA   (PVOID)0x101E8000

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalQueryRealTimeClock(IN PTIME_FIELDS Time)
{
    LARGE_INTEGER LargeTime;
    ULONG Seconds;
    
    /* Query the RTC value */
    Seconds = READ_REGISTER_ULONG(RTC_DATA);
    
    /* Convert to time */
    RtlSecondsSince1970ToTime(Seconds, &LargeTime);
    
    /* Convert to time-fields */
    RtlTimeToTimeFields(&LargeTime, Time);
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
HalSetRealTimeClock(IN PTIME_FIELDS Time)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

/*
 * @unimplemented
 */
ARC_STATUS
NTAPI
HalSetEnvironmentVariable(IN PCH Name,
                          IN PCH Value)
{
    UNIMPLEMENTED;
    while (TRUE);
    return ESUCCESS;
}

/*
 * @unimplemented
 */
ARC_STATUS
NTAPI
HalGetEnvironmentVariable(IN PCH Name,
                          IN USHORT ValueLength,
                          IN PCH Value)
{
    UNIMPLEMENTED;
    while (TRUE);
    return ENOENT;
}

/* EOF */
