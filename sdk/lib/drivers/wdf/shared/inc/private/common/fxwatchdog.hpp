/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxWatchDog.hpp

Abstract:

    This module implements the watchdog utility class for the power and power
    policy state machines

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef __FX_WATCHDOG__
#define __FX_WATCHDOG__

struct FxWatchdog {
    FxWatchdog(
        __in FxPkgPnp* PkgPnp
        )
    {
        m_PkgPnp = PkgPnp;
    }

    VOID
    StartTimer(
        __in ULONG State
        )
    {
        LARGE_INTEGER time;
        NTSTATUS status;

        if (State & WdfDevStateNP) {
            //
            // This is a non-pageable state.  So we set the timer watchdog.
            // If it fires, it means that the driver stalled, probably due
            // to a page fault, which means that the whole machine is wedged.
            // We will then attempt to put the failure in the trace log
            // and bring the machine down, hopefully getting the reason we
            // stopped responding into the crashdump.  (This will probably only work
            // if the machine is hibernating.)
            //
            // If the state function returns, then we'll cancel the timer
            // and no love will be lost.
            //
            status = m_Timer.Initialize(this, _WatchdogDpc, 0);

            //
            // This code is not used in UM. Hence we can assert and assume that
            // timer start will always be successful.
            //
            WDF_VERIFY_KM_ONLY_CODE();

            FX_ASSERT_AND_ASSUME_FOR_PREFAST(NT_SUCCESS(status));

            m_CallingThread = Mx::MxGetCurrentThread();

            if (m_PkgPnp->m_SharedPower.m_ExtendWatchDogTimer) {
                //
                // 24 hours:
                //  60 = to minutes
                //  60 = to hours
                //  24 = number of hours
                //
                time.QuadPart = WDF_REL_TIMEOUT_IN_SEC(60 * 60 * 24);
            }
            else {
                //
                // 10 minutes from now (same as the Vista timeout)
                //
                time.QuadPart = WDF_REL_TIMEOUT_IN_SEC(60 * 10);
            }

            m_Timer.Start(time);
        }
    }

    VOID
    CancelTimer(
        __in ULONG State
        )
    {
        if (State & WdfDevStateNP) {
            //
            // The state was successfully handled without the timer expiring.
            // We don't care about the return code in this case.
            //
            (void) m_Timer.Stop();
        }
    }

    static
    MdDeferredRoutineType
    _WatchdogDpc;

    MxTimer m_Timer;

    FxPkgPnp* m_PkgPnp;

    MxThread m_CallingThread;
};

#endif // __FX_WATCHDOG__

