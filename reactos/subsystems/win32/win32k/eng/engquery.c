/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engquery.c
 * PURPOSE:         Miscellaneous Query Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
EngQueryLocalTime(OUT PENG_TIME_FIELDS ptf)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
EngQueryPerformanceCounter(OUT PLONGLONG PerformanceCount)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
EngQueryPerformanceFrequency(OUT PLONGLONG Frequency)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngQuerySystemAttribute(IN ENG_SYSTEM_ATTRIBUTE  CapNum,
                        OUT PDWORD Capability)
{
    UNIMPLEMENTED;
	return FALSE;
}
