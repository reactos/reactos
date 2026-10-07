/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test that KeQueryPerformanceCounter does not go backwards across processors
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define ITERATIONS 200

START_TEST(KeQpc)
{
    LONG Count = KeNumberProcessors;
    LONG Cpu, Iteration, Misplaced = 0;
    LONGLONG Previous, Current;
    LONGLONG Backwards = 0, WorstDelta = 0;
    LONGLONG First[MAXIMUM_PROCESSORS] = { 0 };

    if (Count < 2)
    {
        trace("Only %ld processor(s); the cross-processor check needs at least two\n", Count);
        return;
    }
    if (Count > MAXIMUM_PROCESSORS)
        Count = MAXIMUM_PROCESSORS;

    Previous = KeQueryPerformanceCounter(NULL).QuadPart;

    for (Iteration = 0; Iteration < ITERATIONS; Iteration++)
    {
        for (Cpu = 0; Cpu < Count; Cpu++)
        {
            KeSetSystemAffinityThread((KAFFINITY)1 << Cpu);
            if (KeGetCurrentProcessorNumber() != (ULONG)Cpu)
                Misplaced++;

            Current = KeQueryPerformanceCounter(NULL).QuadPart;

            if (Iteration == 0)
                First[Cpu] = Current;

            if (Current < Previous)
            {
                Backwards++;
                if (Previous - Current > WorstDelta)
                    WorstDelta = Previous - Current;
            }
            Previous = Current;
        }
    }

    KeRevertToUserAffinityThread();

    for (Cpu = 0; Cpu < Count; Cpu++)
        trace("CPU %ld first QPC sample: %I64d\n", Cpu, First[Cpu]);

    ok(Misplaced == 0, "%ld sample(s) were taken on the wrong processor\n", Misplaced);
    ok(Backwards == 0,
       "KeQueryPerformanceCounter went backwards %I64d time(s) across processors, worst delta %I64d\n",
       Backwards, WorstDelta);
}
