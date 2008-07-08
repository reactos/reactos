#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

CCHAR KeNumberProcessors;
ULONG KeDcacheFlushCount;
ULONG KeActiveProcessors;
ULONG KeProcessorArchitecture;
ULONG KeProcessorLevel;
ULONG KeProcessorRevision;
ULONG KeFeatureBits;
