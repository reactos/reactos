/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite FsRtl Test
 * PROGRAMMER:      Pierre Schweitzer <pierre.schweitzer@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static VOID FsRtlMcbTest()
{
}

static VOID DumpAllRuns(PLARGE_MCB Mcb)
{
    ULONG i;
    LONGLONG Vbn, Lbn, Count;

    trace("MCB %p:\n", Mcb);

    for (i = 0; FsRtlGetNextLargeMcbEntry(Mcb, i, &Vbn, &Lbn, &Count); i++)
    {
        // print out vbn, lbn, and count
        trace("\t[%I64d,%I64d,%I64d]\n", Vbn, Lbn, Count);
    }
    trace("\n");
}

static VOID FsRtlLargeMcbTest()
{
    LARGE_MCB LargeMcb;
    ULONG NbRuns, Index;
    LONGLONG Vbn, Lbn, SectorCount, StartingLbn, CountFromStartingLbn;

    FsRtlInitializeLargeMcb(&LargeMcb, PagedPool);

    ok(FsRtlLookupLastLargeMcbEntry(&LargeMcb, &Vbn, &Lbn) == FALSE, "expected FALSE, got TRUE\n");
    ok(FsRtlLookupLastLargeMcbEntryAndIndex(&LargeMcb, &Vbn, &Lbn, &Index) == FALSE, "expected FALSE, got TRUE\n");

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 1, 1, 1024) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 2, "Expected 2 runs, got: %lu\n", NbRuns);
    DumpAllRuns(&LargeMcb); // [0,-1,1][1,1,1024]          [vbn,lbn,sc]
    ok(FsRtlLookupLastLargeMcbEntry(&LargeMcb, &Vbn, &Lbn) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1024, "Expected Vbn 1024, got: %I64d\n", Vbn);
    ok(Lbn == 1024, "Expected Lbn 1024, got: %I64d\n", Lbn);
    ok(FsRtlLookupLastLargeMcbEntryAndIndex(&LargeMcb, &Vbn, &Lbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1024, "Expected Vbn 1024, got: %I64d\n", Vbn);
    ok(Lbn == 1024, "Expected Lbn 1024, got: %I64d\n", Lbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 2048, 2, 1024) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 4, "Expected 4 runs, got: %lu\n", NbRuns);
    DumpAllRuns(&LargeMcb); // [0,-1,1][1,1,1024][1025,-1,1023][2048,2,1024]  ======= [(0,1) hole] [(1,1025)=>(1,1025)] [(1025, 2048) hole] [(2048,3072)=>(2,1026)]
    ok(FsRtlLookupLastLargeMcbEntry(&LargeMcb, &Vbn, &Lbn) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 3071, "Expected Vbn 3071, got: %I64d\n", Vbn);
    ok(Lbn == 1025, "Expected Lbn 1025, got: %I64d\n", Lbn);
    ok(FsRtlLookupLastLargeMcbEntryAndIndex(&LargeMcb, &Vbn, &Lbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 3071, "Expected Vbn 3071, got: %I64d\n", Vbn);
    ok(Lbn == 1025, "Expected Lbn 1025, got: %I64d\n", Lbn);
    ok(Index == 3, "Expected Index 3, got: %lu\n", Index);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 1, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 2, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1025, "Expected Vbn 1025, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1023, "Expected SectorCount 1023, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 3, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 2048, "Expected Vbn 2048, got: %I64d\n", Vbn);
    ok(Lbn == 2, "Expected Lbn 2, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 4, &Vbn, &Lbn, &SectorCount) == FALSE, "expected FALSE, got TRUE\n");

    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 1, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    ok(StartingLbn == 1, "Expected StartingLbn 1, got: %I64d\n", StartingLbn);
    ok(CountFromStartingLbn == 1024, "Expected CountFromStartingLbn 1024, got: %I64d\n", CountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);

    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 2048, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == 2, "Expected Lbn 2, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    ok(StartingLbn == 2, "Expected StartingLbn 2, got: %I64d\n", StartingLbn);
    ok(CountFromStartingLbn == 1024, "Expected CountFromStartingLbn 1024, got: %I64d\n", CountFromStartingLbn);
    ok(Index == 3, "Expected Index 3, got: %lu\n", Index);

    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 3073, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == FALSE, "expected FALSE, got TRUE\n");

    FsRtlRemoveLargeMcbEntry(&LargeMcb, 1, 1024);
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 2, "Expected 2 runs, got: %lu\n", NbRuns);
    DumpAllRuns(&LargeMcb);  // [0,-1,2048][2048,2,1024]
    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 512, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1536, "Expected SectorCount 1536, got: %I64d\n", SectorCount);
    ok(StartingLbn == -1, "Expected StartingLbn -1, got: %I64d\n", StartingLbn);
    ok(CountFromStartingLbn == 2048, "Expected CountFromStartingLbn 2048, got: %I64d\n", CountFromStartingLbn);
    ok(Index == 0, "Expected Index 0, got: %lu\n", Index);
    ok(FsRtlLookupLastLargeMcbEntryAndIndex(&LargeMcb, &Vbn, &Lbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 3071, "Expected Vbn 3071, got: %I64d\n", Vbn);
    ok(Lbn == 1025, "Expected Lbn 1025, got: %I64d\n", Lbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);

    ok(FsRtlSplitLargeMcb(&LargeMcb, 2048, 1024) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 2, "Expected 2 runs, got: %lu\n", NbRuns);
    DumpAllRuns(&LargeMcb);  // [0,-1,3072][3072,2,1024]
    ok(FsRtlLookupLastLargeMcbEntryAndIndex(&LargeMcb, &Vbn, &Lbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 4095, "Expected Vbn 4095, got: %I64d\n", Vbn);
    ok(Lbn == 1025, "Expected Lbn 1025, got: %I64d\n", Lbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);
    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 2048, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    ok(StartingLbn == -1, "Expected StartingLbn -1, got: %I64d\n", StartingLbn);
    ok(CountFromStartingLbn == 3072, "Expected CountFromStartingLbn 3072, got: %I64d\n", CountFromStartingLbn);
    ok(Index == 0, "Expected Index 0, got: %lu\n", Index);
    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 3072, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == 2, "Expected Lbn 2, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    ok(StartingLbn == 2, "Expected StartingLbn 2, got: %I64d\n", StartingLbn);
    ok(CountFromStartingLbn == 1024, "Expected CountFromStartingLbn 1024, got: %I64d\n", CountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 3584, 3, 1024) == FALSE, "expected FALSE, got TRUE\n");

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 4095, 1025, 1024) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 2, "Expected 2 runs, got: %lu\n", NbRuns);
    DumpAllRuns(&LargeMcb); // [0,-1,3072][3072,2,2047]
    ok(FsRtlLookupLastLargeMcbEntry(&LargeMcb, &Vbn, &Lbn) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 5118, "Expected Vbn 5118, got: %I64d\n", Vbn);
    ok(Lbn == 2048, "Expected Lbn 2048, got: %I64d\n", Lbn);
    ok(FsRtlLookupLastLargeMcbEntryAndIndex(&LargeMcb, &Vbn, &Lbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 5118, "Expected Vbn 5118, got: %I64d\n", Vbn);
    ok(Lbn == 2048, "Expected Lbn 2048, got: %I64d\n", Lbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);

    FsRtlTruncateLargeMcb(&LargeMcb, 4607);
    DumpAllRuns(&LargeMcb); // [0,-1,3072][3072,2,1535]
    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 4095, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == 1025, "Expected Lbn 1025, got: %I64d\n", Lbn);
    ok(SectorCount == 512, "Expected SectorCount 512, got: %I64d\n", SectorCount);
    ok(StartingLbn == 2, "Expected StartingLbn 2, got: %I64d\n", StartingLbn);
    ok(CountFromStartingLbn == 1535, "Expected CountFromStartingLbn 1535, got: %I64d\n", CountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %lu\n", Index);

    FsRtlUninitializeLargeMcb(&LargeMcb);

    FsRtlInitializeLargeMcb(&LargeMcb, PagedPool);
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 0, "Expected 0 runs, got: %lu\n", NbRuns);

    /* Create a mapping with three holes between each mapping
     * It looks like that:
     * ----//////-----/////-----///////
     */
    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 1024, 1025, 1024) == TRUE, "expected TRUE, got FALSE\n");
    DumpAllRuns(&LargeMcb); // [0,-1,1024][1024,1024,1024]
    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 3072, 3072, 1024) == TRUE, "expected TRUE, got FALSE\n");
    DumpAllRuns(&LargeMcb); // [0,-1,1024][1024,1024,1024][2048,-1,1024][3072,3072,1024]
    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 5120, 5120, 1024) == TRUE, "expected TRUE, got FALSE\n");
    DumpAllRuns(&LargeMcb); // [0,-1,1024][1024,1024,1024][2048,-1,1024][3072,3072,1024][4096,-1,1024][5120,5120,1024]

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 6, "Expected 6 runs, got: %lu\n", NbRuns);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 1, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1024, "Expected Vbn 1024, got: %I64d\n", Vbn);
    ok(Lbn == 1025, "Expected Lbn 1024, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 2, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 2048, "Expected Vbn 2048, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 3, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 3072, "Expected Vbn 3072, got: %I64d\n", Vbn);
    ok(Lbn == 3072, "Expected Lbn 3072, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 4, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 4096, "Expected Vbn 4096, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 5, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 5120, "Expected Vbn 5120, got: %I64d\n", Vbn);
    ok(Lbn == 5120, "Expected Lbn 5120, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    /* Fill first hole */
    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 0, 1, 1024) == TRUE, "expected TRUE, got FALSE\n");
    DumpAllRuns(&LargeMcb); // [0,1,2048][2048,-1,1024][3072,3072,1024][4096,-1,1024][5120,5120,1024]

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 5, "Expected 5 runs, got: %lu\n", NbRuns);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 2048, "Expected SectorCount 2048, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 1, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 2048, "Expected Vbn 2048, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 2, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 3072, "Expected Vbn 3072, got: %I64d\n", Vbn);
    ok(Lbn == 3072, "Expected Lbn 3072, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 3, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 4096, "Expected Vbn 4096, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 4, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 5120, "Expected Vbn 5120, got: %I64d\n", Vbn);
    ok(Lbn == 5120, "Expected Lbn 5120, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    /* Fill half of the last hole and overlap */
    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 4608, 4608, 1024) == TRUE, "expected TRUE, got FALSE\n");
    DumpAllRuns(&LargeMcb); // [0,1,2048][2048,-1,1024][3072,3072,1024][4096,-1,512][4608,4608,1536]

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 5, "Expected 5 runs, got: %lu\n", NbRuns);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 2048, "Expected SectorCount 2048, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 1, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 2048, "Expected Vbn 2048, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 2, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 3072, "Expected Vbn 3072, got: %I64d\n", Vbn);
    ok(Lbn == 3072, "Expected Lbn 3072, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 3, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 4096, "Expected Vbn 4096, got: %I64d\n", Vbn);
    ok(Lbn == -1, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 512, "Expected SectorCount 512, got: %I64d\n", SectorCount);

    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 4, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 4608, "Expected Vbn 4608, got: %I64d\n", Vbn);
    ok(Lbn == 4608, "Expected Lbn 4608, got: %I64d\n", Lbn);
    ok(SectorCount == 1536, "Expected SectorCount 1536, got: %I64d\n", SectorCount);

    FsRtlUninitializeLargeMcb(&LargeMcb);
}

START_TEST(FsRtlMcb)
{
    FsRtlMcbTest();
    FsRtlLargeMcbTest();
}
