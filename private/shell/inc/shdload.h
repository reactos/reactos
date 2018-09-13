// Override the linkers default behaviour to DELAYLOAD failures.  Instead of throwing an exception
// try and return a function that simulates a failure in that API, therefore allowing the caller
// to correctly handle it.
//
// To use this functionality exactly one source must include this with COMPILE_DELAYLOAD_STUBS defined,
// and link to shlwapi.

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif // __cplusplus

#ifdef COMPILE_DELAYLOAD_STUBS

#include "delayimp.h"

// NOTE: The names __pfnDliNotifyHook / __pfnDliFailureHook must not be changed,
// NOTE: as they are referenced by the linker's DELAYLOAD handler so we can hook
// NOTE: and process failures during symbol import.  

FARPROC WINAPI ShellDelayLoadHelper(UINT unReason, PDelayLoadInfo pInfo);

PfnDliHook  __pfnDliNotifyHook = ShellDelayLoadHelper;
PfnDliHook  __pfnDliFailureHook = ShellDelayLoadHelper;

#endif

#ifdef __cplusplus
}
#endif // __cplusplus
