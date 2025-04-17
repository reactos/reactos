
#include "k32_vista.h"

/*
 * @implemented
 */
ULONGLONG
WINAPI
GetTickCount64(VOID)
{
    LARGE_INTEGER TickCount;

    TickCount = KiReadSystemTime(&SharedUserData->TickCount);

    /* Convert to milliseconds */
    return KiTickCountToMs(TickCount);
}
