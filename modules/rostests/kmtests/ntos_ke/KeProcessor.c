/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Executive Regressions KM-Test
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

/* TODO: this test doesn't process any test results; it also takes very long */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static VOID KeStallExecutionProcessorTest(VOID)
{
    ULONG i;
    LARGE_INTEGER TimeStart, TimeFinish;

    DPRINT1("Waiting for 30 secs with 50us stalls...\n");
    KeQuerySystemTime(&TimeStart);
    for (i = 0; i < (30*1000*20); i++)
    {
        KeStallExecutionProcessor(50);
    }
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 30

    DPRINT1("Waiting for 30 secs with 1000us stalls...\n");
    KeQuerySystemTime(&TimeStart);
    for (i = 0; i < (30*1000); i++)
    {
        KeStallExecutionProcessor(1000);
    }
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 30

    DPRINT1("Waiting for 30 secs with 1us stalls...\n");
    KeQuerySystemTime(&TimeStart);
    for (i = 0; i < (30*1000*1000); i++)
    {
        KeStallExecutionProcessor(1);
    }
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 43

    DPRINT1("Waiting for 30 secs with one huge stall...\n");
    KeQuerySystemTime(&TimeStart);
    KeStallExecutionProcessor(30*1000000);
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 30
}

START_TEST(KeProcessor)
{
    KeStallExecutionProcessorTest();
}
