
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

#ifdef _WIN64
    return (TickCount.QuadPart * SharedUserData->TickCountMultiplier) >> 24;
#else
    ULONG TickCountMultiplier = SharedUserData->TickCountMultiplier;
    return (UInt32x32To64(TickCount.LowPart, TickCountMultiplier) >> 24) +
           (UInt32x32To64(TickCount.HighPart, TickCountMultiplier) << 8);
#endif
}
