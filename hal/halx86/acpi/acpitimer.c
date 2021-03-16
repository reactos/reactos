/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/acpi/halacpi.c
 * PURPOSE:         HAL ACPI Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#ifdef _M_IX86
PHALP_STALL_EXEC_PROC TimerStallExecProc = HalpPmTimerStallExecProc;
PHALP_CALIBRATE_PERF_COUNT TimerCalibratePerfCount = HalpPmTimerCalibratePerfCount;
PHALP_QUERY_PERF_COUNT TimerQueryPerfCount = HalpPmTimerQueryPerfCount;
PHALP_SET_TIME_INCREMENT TimerSetTimeIncrement = HalpPmTimerSetTimeIncrement;

PHALP_QUERY_TIMER QueryTimer = HalpQueryPerformanceCounter;
ULONG PMTimerFreq = 3579545;

/* Stall execute */
#define HAL_STALL_LOOP  42
ULONG StallLoopValue = HAL_STALL_LOOP;
UCHAR StallExecCounter = 0;

HALP_TIMER_INFO TimerInfo;
BOOLEAN HalpBrokenAcpiTimer = FALSE;
#endif

/* PM TIMER FUNCTIONS *********************************************************/

#ifdef _M_IX86
VOID
NTAPI
HaliPmTimerQueryPerfCount(_Out_ LARGE_INTEGER * OutPerfCount,
                          _Out_ LARGE_INTEGER * OutPerfFrequency)
{
    OutPerfCount->QuadPart = TimerInfo.PerformanceCounter.QuadPart + QueryTimer();

    if (OutPerfFrequency) {
        OutPerfFrequency->QuadPart = PMTimerFreq;
    }
}

ULONGLONG
__cdecl
HalpQueryPerformanceCounter(VOID)
{
    LARGE_INTEGER TimeValue;
    LARGE_INTEGER PerfCounter;
    ULONG CurrentTime;
    ULONG ValueExt;

    do
    {
        YieldProcessor();
    }
    while (TimerInfo.AcpiTimeValue.HighPart != TimerInfo.TimerCarry);

    CurrentTime = READ_PORT_ULONG(TimerInfo.TimerPort);

    TimeValue = TimerInfo.AcpiTimeValue;
    ValueExt = TimerInfo.ValueExt;

    PerfCounter.HighPart = TimeValue.HighPart;
    PerfCounter.LowPart = ((CurrentTime & (~ValueExt)) | (TimeValue.LowPart & (~(ValueExt - 1))));

    PerfCounter.QuadPart += ((CurrentTime ^ TimeValue.LowPart) & ValueExt);

    return PerfCounter.QuadPart;
}
#endif

/* PUBLIC PM TIMER FUNCTIONS **************************************************/

#ifdef _M_IX86
VOID
NTAPI
HalpCalibrateStallExecution(VOID)
{
    ;
}

VOID
NTAPI 
KeStallExecutionProcessor(_In_ ULONG MicroSeconds)
{
    TimerStallExecProc(MicroSeconds);
}
VOID
NTAPI
HalpPmTimerStallExecProc(_In_ ULONG MicroSeconds)
{
    INT CpuInfo[4];
    ULONGLONG Start;
    ULONG StallValue;
    LARGE_INTEGER Current;
    LARGE_INTEGER Next;
    ULONG TotalStall;
    ULONG StallCounter;

    //DPRINT1("HalpPmTimerStallExecProc: MicroSeconds %X\n", MicroSeconds);

    /* Serializing instruction execution */
    __cpuid(CpuInfo, 0);

    if (!MicroSeconds)
    {
        goto Exit;
    }

    //HalpStallExecCounter++; // Statistics

    StallValue = (PMTimerFreq * (ULONGLONG)MicroSeconds) / 1000000 - 1; // ~2,579545 * MicroSeconds
    Start = QueryTimer();

    TotalStall = 0;
    StallCounter = StallLoopValue;

    while (TRUE)
    {
        while (TRUE)
        {
            TotalStall += StallCounter;

            _mm_pause();

            do
            {
                StallCounter--;
            }
            while (StallCounter);

            Current.QuadPart = (QueryTimer() - Start);

            if (Current.HighPart)
            {
                Current.LowPart = 0x7FFFFFFF;
            }

            if (Current.LowPart >= 3)
            {
                break;
            }

            StallLoopValue += HAL_STALL_LOOP;
            StallCounter = StallLoopValue;
        }

        if (!Current.LowPart)
        {
            KeBugCheckEx(0xA5, 0x20000, Current.HighPart, 0, 0);
        }

        if (Current.LowPart >= StallValue)
        {
            break;
        }

        Next.LowPart = (StallValue - Current.LowPart) * TotalStall;
        Next.HighPart = (((ULONGLONG)(StallValue - Current.LowPart) * TotalStall) >> 32);
        Next.HighPart &= 3;

        StallCounter = ((ULONGLONG)Next.QuadPart / Current.LowPart + 1);
    }

Exit:

    /* StallExecCounter 0-255 */
    StallExecCounter++;

    if (!StallExecCounter && (StallLoopValue > HAL_STALL_LOOP))
    {
        StallLoopValue -= HAL_STALL_LOOP;
    }
}

VOID
NTAPI 
HalCalibratePerformanceCounter(_In_ volatile PLONG Count,
                               _In_ ULONGLONG NewCount)
{
    TimerCalibratePerfCount(Count, NewCount);
}
VOID
NTAPI
HalpPmTimerCalibratePerfCount(_In_ volatile PLONG Count,
                              _In_ ULONGLONG NewCount)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

LARGE_INTEGER
NTAPI 
KeQueryPerformanceCounter(_Out_opt_ LARGE_INTEGER * OutPerfFrequency)
{
    return TimerQueryPerfCount(OutPerfFrequency);
}
LARGE_INTEGER
NTAPI
HalpPmTimerQueryPerfCount(_Out_opt_ LARGE_INTEGER * OutPerfFrequency)
{
    LARGE_INTEGER PerfCount;
    HaliPmTimerQueryPerfCount(&PerfCount, OutPerfFrequency);
    return PerfCount;
}

ULONG
NTAPI 
HalSetTimeIncrement(_In_ ULONG Increment)
{
    return TimerSetTimeIncrement(Increment);
}
ULONG
NTAPI
HalpPmTimerSetTimeIncrement(_In_ ULONG Increment)
{
    ULONG Result;
    UNIMPLEMENTED;
    ASSERT(FALSE);Result = 0;
    return Result;
}
#endif

/* PRIVATE FUNCTIONS **********************************************************/

#ifdef _M_AMD64
VOID
NTAPI
HaliAcpiTimerInit(IN ULONG TimerPort,
                  IN ULONG TimerValExt)
{
    PAGED_CODE();

    /* Is this in the init phase? */
    if (!TimerPort)
    {
        /* Get the data from the FADT */
        TimerPort = HalpFixedAcpiDescTable.pm_tmr_blk_io_port;
        TimerValExt = HalpFixedAcpiDescTable.flags & ACPI_TMR_VAL_EXT;
        DPRINT1("ACPI Timer at: %Xh (EXT: %d)\n", TimerPort, TimerValExt);
    }

    /* FIXME: Now proceed to the timer initialization */
    //HalaAcpiTimerInit(TimerPort, TimerValExt);
}
#else
VOID
NTAPI
HalaAcpiTimerInit(_In_ PULONG TimerPort,
                  _In_ BOOLEAN IsTimerValExt32bit)
{
    DPRINT("HalaAcpiTimerInit: Port %p, IsValExt %X\n", TimerPort, IsTimerValExt32bit);

    RtlZeroMemory(&TimerInfo, sizeof(TimerInfo));

    TimerInfo.TimerPort = TimerPort;

    if (IsTimerValExt32bit)
    {
        TimerInfo.ValueExt = FADT_TMR_VAL_EXT_32BIT;
    }
    else
    {
        TimerInfo.ValueExt = FADT_TMR_VAL_EXT_24BIT;
    }

    if (!HalpBrokenAcpiTimer)
    {
        return;
    }

    DPRINT1("HalaAcpiTimerInit: HalpBrokenAcpiTimer. DbgBreakPoint()\n");
    DbgBreakPoint();
}

VOID
NTAPI
HaliAcpiTimerInit(_In_ PULONG TimerPort,
                  _In_ BOOLEAN IsTimerValExt32bit)
{
    PAGED_CODE();
    DPRINT("HaliAcpiTimerInit: Port %p, ValExt %X, flags %X\n", TimerPort, IsTimerValExt32bit, HalpFixedAcpiDescTable.flags);

    /* Is this in the init phase? */
    if (!TimerPort)
    {
        /* Get the data from the FADT */

        /* System port address of the Power Management (PM) Timer Control Register Block */
        TimerPort = (PULONG)HalpFixedAcpiDescTable.pm_tmr_blk_io_port;

        /* A zero flags bit indicates TMR_VAL is implemented as a 24-bit value.
           A one indicates TMR_VAL is implemented as a 32-bit value. 
        */
        IsTimerValExt32bit = ((HalpFixedAcpiDescTable.flags & ACPI_TMR_VAL_EXT) != 0);
        DPRINT("TimerPort %p, IsTimerValExt32bit %X\n", TimerPort, IsTimerValExt32bit);
    }

    HaliAcpiSetUsePmClock();

    /* Now proceed to the timer initialization */
    HalaAcpiTimerInit(TimerPort, IsTimerValExt32bit);
}

VOID
NTAPI
HalAcpiTimerCarry(VOID)
{
    ULONG Time;
    LARGE_INTEGER Value;

    Time = READ_PORT_ULONG(TimerInfo.TimerPort);

    Value.QuadPart = TimerInfo.AcpiTimeValue.QuadPart + TimerInfo.ValueExt;
    Value.QuadPart += ((Value.LowPart ^ Time) & TimerInfo.ValueExt);

    TimerInfo.TimerCarry = Value.HighPart;
    TimerInfo.AcpiTimeValue.QuadPart = Value.QuadPart;
}

VOID
NTAPI
HalAcpiBrokenPiix4TimerCarry(VOID)
{
    DPRINT("HalAcpiBrokenPiix4TimerCarry()\n");
    /* Nothing */
    ;
}
#endif

/* EOF */
