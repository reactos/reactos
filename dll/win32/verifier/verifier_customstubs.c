/*
 * PROJECT:     Application verifier
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Custom stubs, using only ntdll functions
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

#define EXCEPTION_WINE_STUB       0x80000100

#define __wine_spec_unimplemented_stub(module, function) \
{ \
    EXCEPTION_RECORD ExceptionRecord = {0}; \
    ExceptionRecord.ExceptionRecord = NULL; \
    ExceptionRecord.ExceptionCode = EXCEPTION_WINE_STUB; \
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE; \
    ExceptionRecord.ExceptionInformation[0] = (ULONG_PTR)module; \
    ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR)function; \
    ExceptionRecord.NumberParameters = 2; \
    RtlRaiseException(&ExceptionRecord); \
}

int NTAPI VerifierAddFreeMemoryCallback(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierAddFreeMemoryCallback(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierCreateRpcPageHeap(PVOID arg0, PVOID arg1, PVOID arg2, PVOID arg3, PVOID arg4, PVOID arg5)
{
    DbgPrint("WARNING: calling stub VerifierCreateRpcPageHeap(%p, %p, %p, %p, %p, %p)\n", arg0, arg1, arg2, arg3, arg4, arg5);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierDeleteFreeMemoryCallback(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierDeleteFreeMemoryCallback(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierDestroyRpcPageHeap(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierDestroyRpcPageHeap(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierDisableFaultInjectionExclusionRange(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierDisableFaultInjectionExclusionRange(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierDisableFaultInjectionTargetRange(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierDisableFaultInjectionTargetRange(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierEnableFaultInjectionExclusionRange(PVOID arg0, PVOID arg1)
{
    DbgPrint("WARNING: calling stub VerifierEnableFaultInjectionExclusionRange(%p, %p)\n", arg0, arg1);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierEnableFaultInjectionTargetRange(PVOID arg0, PVOID arg1)
{
    DbgPrint("WARNING: calling stub VerifierEnableFaultInjectionTargetRange(%p, %p)\n", arg0, arg1);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierEnumerateResource(PVOID arg0, PVOID arg1, PVOID arg2, PVOID arg3, PVOID arg4)
{
    DbgPrint("WARNING: calling stub VerifierEnumerateResource(%p, %p, %p, %p, %p)\n", arg0, arg1, arg2, arg3, arg4);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierIsCurrentThreadHoldingLocks()
{
    DbgPrint("WARNING: calling stub VerifierIsCurrentThreadHoldingLocks()\n");
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierIsDllEntryActive(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierIsDllEntryActive(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int __cdecl VerifierLogMessage()
{
    DbgPrint("WARNING: calling stub VerifierLogMessage()\n");
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierQueryRuntimeFlags(PVOID arg0, PVOID arg1)
{
    DbgPrint("WARNING: calling stub VerifierQueryRuntimeFlags(%p, %p)\n", arg0, arg1);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierSetFaultInjectionProbability(PVOID arg0, PVOID arg1)
{
    DbgPrint("WARNING: calling stub VerifierSetFaultInjectionProbability(%p, %p)\n", arg0, arg1);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierSetFlags(PVOID arg0, PVOID arg1, PVOID arg2)
{
    DbgPrint("WARNING: calling stub VerifierSetFlags(%p, %p, %p)\n", arg0, arg1, arg2);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierSetRuntimeFlags(PVOID arg0)
{
    DbgPrint("WARNING: calling stub VerifierSetRuntimeFlags(%p)\n", arg0);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

int NTAPI VerifierStopMessage(PVOID arg0, PVOID arg1, PVOID arg2, PVOID arg3, PVOID arg4, PVOID arg5, PVOID arg6, PVOID arg7, PVOID arg8, PVOID arg9)
{
    DbgPrint("WARNING: calling stub VerifierStopMessage(%p, %p, %p, %p, %p, %p, %p, %p, %p, %p)\n", arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    __wine_spec_unimplemented_stub("verifier.dll", __FUNCTION__);
    return 0;
}

