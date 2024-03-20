#include <rtltests.h>
#include <pseh/pseh2.h>
#include <compat_undoc.h>
#include <compatguid_undoc.h>

#define NDEBUG
#include <debug.h>

BOOLEAN LdrpShutdownInProgress;
HANDLE LdrpShutdownThreadId;

VOID
NTAPI
LdrpInitializeProcessCompat(PVOID pProcessActctx, PVOID* pOldShimData)
{
}

