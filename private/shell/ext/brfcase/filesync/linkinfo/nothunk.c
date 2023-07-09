#include "project.h"

// BUGBUG - BobDay - This function needs to be added to KERNEL32. NOPE,
// according to markl we only need this because the critical section
// was located in shared memory.  Possible solution here might be to create
// a named event or mutex and synchronize via it. Another possible solution
// might be to move each of the objects for which there is a critical section
// out of the shared memory segment and maintain a per-process data structure.
VOID WINAPI NoThunkReinitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
) {
    InitializeCriticalSection( lpCriticalSection );
}
