
#include "k32_vista.h"

/*
 * @implemented
 */
ULONGLONG
WINAPI
GetTickCount64(VOID)
{
    ULARGE_INTEGER TickCount;

    while (TRUE)
    {
        TickCount.HighPart = (ULONG)SharedUserData->TickCount.High1Time;
        TickCount.LowPart = SharedUserData->TickCount.LowPart;

        if (TickCount.HighPart == (ULONG)SharedUserData->TickCount.High2Time) break;

        YieldProcessor();
     }

     return (UInt32x32To64(TickCount.LowPart, SharedUserData->TickCountMultiplier) >> 24) +
            (UInt32x32To64(TickCount.HighPart, SharedUserData->TickCountMultiplier) << 8);
}
