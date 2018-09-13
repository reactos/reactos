#include "proj.h"
#include "dbutil.h"

///////
// Critical section helper stuff
//
#ifdef DEBUG
UINT g_CriticalSectionCount = 0;
DWORD g_CriticalSectionOwner = 0;
#ifdef STACKBACKTRACE
DBstkback g_CriticalSectionLastCall[4] = { 0 };
#endif


void Dll_EnterCriticalSection(CRITICAL_SECTION * pcsDll)
{
#ifdef STACKBACKTRACE
    int var0;       // *must* be 1st on frame
#endif

    EnterCriticalSection(pcsDll);
    if (g_CriticalSectionCount++ == 0)
    {
        g_CriticalSectionOwner = GetCurrentThreadId();
#ifdef STACKBACKTRACE
        int fp = (int) (1 + (int *)&var0);
        DBGetStackBack(&fp, g_CriticalSectionLastCall, ARRAYSIZE(g_CriticalSectionLastCall));
#endif
    }
}

void Dll_LeaveCriticalSection(CRITICAL_SECTION * pcsDll)
{
    if (--g_CriticalSectionCount == 0)
        g_CriticalSectionOwner = 0;
    LeaveCriticalSection(pcsDll);
}
#endif
