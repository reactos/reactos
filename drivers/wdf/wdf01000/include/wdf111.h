#ifndef _WDF111_H_
#define _WDF111_H_

#include <ntddk.h>


typedef struct _WDF_TIMER_CONFIG_V1_11 {
    ULONG Size;

    PFN_WDF_TIMER EvtTimerFunc;

    ULONG Period;

    // 
    // If this is TRUE, the Timer will automatically serialize
    // with the event callback handlers of its Parent Object.
    // 
    // Parent Object's callback constraints should be compatible
    // with the Timer DPC (DISPATCH_LEVEL), or the request will fail.
    // 
    BOOLEAN AutomaticSerialization;

    // 
    // Optional tolerance for the timer in milliseconds.
    // 
    ULONG TolerableDelay;

} WDF_TIMER_CONFIG_V1_11, *PWDF_TIMER_CONFIG_V1_11;

#endif //_WDF111_H_