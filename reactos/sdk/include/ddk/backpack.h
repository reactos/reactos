#ifndef _BACKPACK_
#define _BACKPACK_

typedef struct _THROTTLING_STATE
{
    LARGE_INTEGER NextTime;
    volatile ULONG CurrentIncrement;
    ULONG MaximumDelay;
    LARGE_INTEGER Increment;
    volatile ULONG NumberOfQueries;
} THROTTLING_STATE, *PTHROTTLING_STATE;

#define RxInitializeThrottlingState(BP, Inc, MaxDelay) \
{                                                      \
    if ((Inc) > 0)                                     \
    {                                                  \
        (BP)->Increment.QuadPart = (Inc) * 10000;      \
        (BP)->MaximumDelay = (MaxDelay) / (Inc);       \
        (BP)->CurrentIncrement = 0;                    \
    }                                                  \
}

#endif
