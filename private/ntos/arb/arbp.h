#ifndef _ARBP_
#define _ARBP_

#ifndef FAR
#define FAR
#endif

#define ARB_DBG 1 // DBG

#if NTOS_KERNEL

//
// If we are in the kernel use the in-kernel headers so we get the efficient
// definitions of things
//

#include "ntos.h"
#include "zwapi.h"

#else

//
// If we are building the library for bus drivers to use make sure we use the 
// same definitions of things as them
//

#include "ntddk.h"

#endif

#include "arbiter.h"
#include <stdlib.h>     // for __min and __max


#if ARB_DBG

extern PCHAR ArbpActionStrings[];
extern ULONG ArbStopOnError;
extern ULONG ArbReplayOnError;

VOID
ArbpDumpArbiterInstance(
    LONG Level,
    PARBITER_INSTANCE Arbiter
    );

VOID
ArbpDumpArbiterRange(
    LONG Level,
    PRTL_RANGE_LIST List,
    PUCHAR RangeText
    );

VOID
ArbpDumpArbitrationList(
    LONG Level,
    PLIST_ENTRY ArbitrationList
    );

#endif // ARB_DBG

#endif _ARBP_
