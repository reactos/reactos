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

    ok(FsRtlLookupLargeMcbEntry(&LargeMcb, 513, &Lbn, &SectorCount, &StartingLbn, &CountFromStartingLbn, &Index) == TRUE, "expected TRUE, got FALSE\n");
    ok(Lbn == 513, "Expected Lbn 513, got: %I64d\n", Lbn);
    ok(SectorCount == 512, "Expected SectorCount 512, got: %I64d\n", SectorCount);
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

    FsRtlInitializeLargeMcb(&LargeMcb, PagedPool);
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 0, "Expected 0 runs, got: %lu\n", NbRuns);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 0, 1, 1024) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 1, "Expected 1 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 0, 2, 1024) == FALSE, "expected FALSE, got TRUE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 1, "Expected 1 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 0, 0, 1024) == FALSE, "expected FALSE, got TRUE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 1, "Expected 1 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 1, 2, 1023) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 1, "Expected 1 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1024, "Expected SectorCount 1024, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);

    FsRtlUninitializeLargeMcb(&LargeMcb);

    FsRtlInitializeLargeMcb(&LargeMcb, PagedPool);
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 0, "Expected 0 runs, got: %lu\n", NbRuns);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 0, 1, 1) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 1, "Expected 1 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);


    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 1, 10, 1) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 2, "Expected 2 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 1, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 10, "Expected Lbn 10, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);

    ok(FsRtlAddLargeMcbEntry(&LargeMcb, 2, 20, 1) == TRUE, "expected TRUE, got FALSE\n");
    NbRuns = FsRtlNumberOfRunsInLargeMcb(&LargeMcb);
    ok(NbRuns == 3, "Expected 3 runs, got: %lu\n", NbRuns);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 0, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 0, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == 1, "Expected Lbn 1, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 1, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 1, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 10, "Expected Lbn 10, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);
    ok(FsRtlGetNextLargeMcbEntry(&LargeMcb, 2, &Vbn, &Lbn, &SectorCount) == TRUE, "expected TRUE, got FALSE\n");
    ok(Vbn == 2, "Expected Vbn 2, got: %I64d\n", Vbn);
    ok(Lbn == 20, "Expected Lbn 20, got: %I64d\n", Lbn);
    ok(SectorCount == 1, "Expected SectorCount 1, got: %I64d\n", SectorCount);
    DumpAllRuns(&LargeMcb);

    FsRtlUninitializeLargeMcb(&LargeMcb);
}

static VOID FsRtlLargeMcbTestsExt2()
{
    LARGE_MCB FirstMcb, SecondMcb;
    LONGLONG Lbn, SectorCountFromLbn, StartingLbn, SectorCountFromStartingLbn, Vbn, SectorCount;
    ULONG Index, NbRuns;
    BOOLEAN Result;

    FsRtlInitializeLargeMcb(&FirstMcb, PagedPool);
    FsRtlInitializeLargeMcb(&SecondMcb, PagedPool);

    FsRtlTruncateLargeMcb(&FirstMcb, 0);
    FsRtlTruncateLargeMcb(&SecondMcb, 0);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 1, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&FirstMcb, 1, 198657, 1);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    DumpAllRuns(&FirstMcb);

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&FirstMcb);
    ok(NbRuns == 2, "Expected 2 runs, got: %lu\n", NbRuns);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 1, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 1LL, "Expected SectorCountFromLbn 1, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 198657LL, "Expected StartingLbn 198657, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 1LL, "Expected SectorCountFromStartingLbn 1, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %d\n", Index);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 2, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&FirstMcb, 2, 199169, 11);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    DumpAllRuns(&FirstMcb);

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&FirstMcb);
    ok(NbRuns == 3, "Expected 3 runs, got: %lu\n", NbRuns);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 2, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 199169LL, "Expected Lbn 199169, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 11LL, "Expected SectorCountFromLbn 11, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 199169LL, "Expected StartingLbn 199169, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 11LL, "Expected SectorCountFromStartingLbn 11, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 2, "Expected Index 2, got: %d\n", Index);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 0, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 0LL, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1LL, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 1, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 1LL, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 2, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 2LL, "Expected Vbn 2, got: %I64d\n", Vbn);
    ok(Lbn == 199169LL, "Expected Lbn 199169, got: %I64d\n", Lbn);
    ok(SectorCount == 11LL, "Expected SectorCount 11, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 3, &Vbn, &Lbn, &SectorCount);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&SecondMcb, 197128, 197128, 1);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    Result = FsRtlLookupLargeMcbEntry(&SecondMcb, 197128, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 197128LL, "Expected Lbn 197128, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 1LL, "Expected SectorCountFromLbn 1, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 197128LL, "Expected StartingLbn 197128, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 1LL, "Expected SectorCountFromStartingLbn 1, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %d\n", Index);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 13, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&FirstMcb, 13, 199180, 4);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    DumpAllRuns(&FirstMcb);

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&FirstMcb);
    ok(NbRuns == 3, "Expected 3 runs, got: %lu\n", NbRuns);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 13, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 199180LL, "Expected Lbn 199180, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 4LL, "Expected SectorCountFromLbn 4, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 199169LL, "Expected StartingLbn 199169, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 15LL, "Expected SectorCountFromStartingLbn 15, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 2, "Expected Index 2, got: %d\n", Index);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 0, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 0LL, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1LL, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 1, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 1LL, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 2, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 2LL, "Expected Vbn 2, got: %I64d\n", Vbn);
    ok(Lbn == 199169LL, "Expected Lbn 199169, got: %I64d\n", Lbn);
    ok(SectorCount == 15LL, "Expected SectorCount 15, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 3, &Vbn, &Lbn, &SectorCount);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&SecondMcb, 197128, 197128, 1);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    Result = FsRtlLookupLargeMcbEntry(&SecondMcb, 197128, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 197128LL, "Expected Lbn 197128, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 1LL, "Expected SectorCountFromLbn 1, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 197128LL, "Expected StartingLbn 197128, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 1LL, "Expected SectorCountFromStartingLbn 1, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %d\n", Index);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 17, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&FirstMcb, 17, 1105, 16);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    DumpAllRuns(&FirstMcb);

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&FirstMcb);
    ok(NbRuns == 4, "Expected 4 runs, got: %lu\n", NbRuns);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 17, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 1105LL, "Expected Lbn 1105, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 16LL, "Expected SectorCountFromLbn 16, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 1105LL, "Expected StartingLbn 1105, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 16LL, "Expected SectorCountFromStartingLbn 16, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 3, "Expected Index 3, got: %d\n", Index);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 0, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 0LL, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1LL, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 1, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 1LL, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 2, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 2LL, "Expected Vbn 2, got: %I64d\n", Vbn);
    ok(Lbn == 199169LL, "Expected Lbn 199169, got: %I64d\n", Lbn);
    ok(SectorCount == 15LL, "Expected SectorCount 15, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 3, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 17LL, "Expected Vbn 17, got: %I64d\n", Vbn);
    ok(Lbn == 1105LL, "Expected Lbn 1105, got: %I64d\n", Lbn);
    ok(SectorCount == 16LL, "Expected SectorCount 16, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 4, &Vbn, &Lbn, &SectorCount);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&SecondMcb, 197128, 197128, 1);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    Result = FsRtlLookupLargeMcbEntry(&SecondMcb, 197128, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 197128LL, "Expected Lbn 197128, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 1LL, "Expected SectorCountFromLbn 1, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 197128LL, "Expected StartingLbn 197128, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 1LL, "Expected SectorCountFromStartingLbn 1, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %d\n", Index);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 33, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&FirstMcb, 33, 1185, 32);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    DumpAllRuns(&FirstMcb);

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&FirstMcb);
    ok(NbRuns == 5, "Expected 5 runs, got: %lu\n", NbRuns);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 33, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 1185LL, "Expected Lbn 1185, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 32LL, "Expected SectorCountFromLbn 32, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 1185LL, "Expected StartingLbn 1185, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 32LL, "Expected SectorCountFromStartingLbn 32, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 4, "Expected Index 4, got: %d\n", Index);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 0, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 0LL, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1LL, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 1, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 1LL, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 2, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 2LL, "Expected Vbn 2, got: %I64d\n", Vbn);
    ok(Lbn == 199169LL, "Expected Lbn 199169, got: %I64d\n", Lbn);
    ok(SectorCount == 15LL, "Expected SectorCount 15, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 3, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 17LL, "Expected Vbn 17, got: %I64d\n", Vbn);
    ok(Lbn == 1105LL, "Expected Lbn 1105, got: %I64d\n", Lbn);
    ok(SectorCount == 16LL, "Expected SectorCount 16, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 4, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 33LL, "Expected Vbn 33, got: %I64d\n", Vbn);
    ok(Lbn == 1185LL, "Expected Lbn 1185, got: %I64d\n", Lbn);
    ok(SectorCount == 32LL, "Expected SectorCount 32, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 5, &Vbn, &Lbn, &SectorCount);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&SecondMcb, 197128, 197128, 1);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    Result = FsRtlLookupLargeMcbEntry(&SecondMcb, 197128, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 197128LL, "Expected Lbn 197128, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 1LL, "Expected SectorCountFromLbn 1, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 197128LL, "Expected StartingLbn 197128, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 1LL, "Expected SectorCountFromStartingLbn 1, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 1, "Expected Index 1, got: %d\n", Index);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 65, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlAddLargeMcbEntry(&FirstMcb, 65, 1249, 44);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");

    DumpAllRuns(&FirstMcb);

    NbRuns = FsRtlNumberOfRunsInLargeMcb(&FirstMcb);
    ok(NbRuns == 6, "Expected 6 runs, got: %lu\n", NbRuns);

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 65, &Lbn, &SectorCountFromLbn, &StartingLbn, &SectorCountFromStartingLbn, &Index);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 1249LL, "Expected Lbn 1249, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 44LL, "Expected SectorCountFromLbn 44, got: %I64d\n", SectorCountFromLbn);
    ok(StartingLbn == 1249LL, "Expected StartingLbn 1249, got: %I64d\n", StartingLbn);
    ok(SectorCountFromStartingLbn == 44LL, "Expected SectorCountFromStartingLbn 44, got: %I64d\n", SectorCountFromStartingLbn);
    ok(Index == 5, "Expected Index 1, got: %d\n", Index);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 0, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 0LL, "Expected Vbn 0, got: %I64d\n", Vbn);
    ok(Lbn == -1LL, "Expected Lbn -1, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 1, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 1LL, "Expected Vbn 1, got: %I64d\n", Vbn);
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCount == 1LL, "Expected SectorCount 1, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 2, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 2LL, "Expected Vbn 2, got: %I64d\n", Vbn);
    ok(Lbn == 199169LL, "Expected Lbn 199169, got: %I64d\n", Lbn);
    ok(SectorCount == 15LL, "Expected SectorCount 15, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 3, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 17LL, "Expected Vbn 17, got: %I64d\n", Vbn);
    ok(Lbn == 1105LL, "Expected Lbn 1105, got: %I64d\n", Lbn);
    ok(SectorCount == 16LL, "Expected SectorCount 16, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 4, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 33LL, "Expected Vbn 33, got: %I64d\n", Vbn);
    ok(Lbn == 1185LL, "Expected Lbn 1185, got: %I64d\n", Lbn);
    ok(SectorCount == 32LL, "Expected SectorCount 32, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 5, &Vbn, &Lbn, &SectorCount);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Vbn == 65LL, "Expected Vbn 65, got: %I64d\n", Vbn);
    ok(Lbn == 1249LL, "Expected Lbn 1249, got: %I64d\n", Lbn);
    ok(SectorCount == 44LL, "Expected SectorCount 44, got: %I64d\n", SectorCount);

    Result = FsRtlGetNextLargeMcbEntry(&FirstMcb, 6, &Vbn, &Lbn, &SectorCount);
    ok(Result == FALSE, "Expected FALSE, got TRUE\n");

    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 1, &Lbn, &SectorCountFromLbn, NULL, NULL, NULL);
    ok(Result == TRUE, "Expected TRUE, got FALSE\n");
    ok(Lbn == 198657LL, "Expected Lbn 198657, got: %I64d\n", Lbn);
    ok(SectorCountFromLbn == 1LL, "Expected SectorCountFromLbn 108, got: %I64d\n", SectorCountFromLbn);

    FsRtlUninitializeLargeMcb(&SecondMcb);
    FsRtlUninitializeLargeMcb(&FirstMcb);
}

#define Check_Lookup(_Mcb, _Vbn, _ExpectedRet, _ExpectedLbn, _ExpectedSectorCountFromLbn, _ExpectedStartLbn, _ExpectedSectorCountFromStartLbn, _ExpectedIndex)  \
do {                                                                                                                                                            \
    LONGLONG Lbn = 0xDEADBEEF;                                                                                                                                  \
    LONGLONG SectorCountFromLbn = 0xBAADF00D;                                                                                                                   \
    LONGLONG StartLbn = 0xBEEFDEAD;                                                                                                                             \
    LONGLONG SectorCountFromStartLbn = 0xF00DBAAD;                                                                                                              \
    ULONG Index = 0xDEADBABE;                                                                                                                                   \
    Result = FsRtlLookupLargeMcbEntry(&Mcb, _Vbn, &Lbn, &SectorCountFromLbn, &StartLbn, &SectorCountFromStartLbn, &Index);                                   \
    ok(Result == _ExpectedRet, "Expected FsRtlLookupLargeMcbEntry to %s.\n", _ExpectedRet ? "succeed" : "fail");                                                \
    ok(Lbn == _ExpectedLbn, "Unexpected Lbn: %I64d.\n", Lbn);                                                                                                   \
    ok(SectorCountFromLbn == _ExpectedSectorCountFromLbn, "Unexpected sector count from Lbn: %I64d\n", SectorCountFromLbn);                                              \
    ok(StartLbn == _ExpectedStartLbn, "Unexpected starting Lbn: %I64d.\n", StartLbn);                                                                                                   \
    ok(SectorCountFromStartLbn == _ExpectedSectorCountFromStartLbn, "Unexpected sector count from start Lbn: %I64d\n", SectorCountFromStartLbn);                                              \
    ok(Index == _ExpectedIndex, "Unexpected Index: %I64d.\n", Index);                                                                                                   \
} while(0)

#define ok_lookup_fails(_Mcb, _Vbn) Check_Lookup(_Mcb, _Vbn, FALSE, 0xDEADBEEF, 0xBAADF00D, 0xBEEFDEAD, 0xF00DBAAD, 0xDEADBABE)
#define ok_lookup_succeeds(_Mcb, _Vbn, _ExpectedLbn, _ExpectedSectorCountFromLbn, _ExpectedStartLbn, _ExpectedSectorCountFromStartLbn, _ExpectedIndex) \
    Check_Lookup(_Mcb, _Vbn, TRUE, _ExpectedLbn, _ExpectedSectorCountFromLbn, _ExpectedStartLbn, _ExpectedSectorCountFromStartLbn, _ExpectedIndex)

static VOID FsRtlLargeMcbTestsFastFat()
{
    LARGE_MCB Mcb;
    BOOLEAN Result;
    ULONG RunCount;

    FsRtlInitializeLargeMcb(&Mcb, PagedPool);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 0, "Expected no run, got %lu\n", RunCount);

    ok_lookup_fails(&Mcb, 0);
    ok_lookup_fails(&Mcb, 1);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");
    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_fails(&Mcb, 33);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 1);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");
    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_fails(&Mcb, 33);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 1);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 2);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 2, 32, 2, 1);
    ok_lookup_fails(&Mcb, 34);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 2);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 33, 33, 10);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected one run, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, -1, 1, -1, 33, 0);
    ok_lookup_succeeds(&Mcb, 33, 33, 10, 33, 10, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 33, 7);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 40, 40, 3, 40, 3, 1);

    FsRtlUninitializeLargeMcb(&Mcb);
}

static VOID FsRtlLargeMcbTestsFastFat_2(VOID)
{
    LARGE_MCB Mcb;
    BOOLEAN Result;
    ULONG RunCount;

    FsRtlInitializeLargeMcb(&Mcb, PagedPool);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_fails(&Mcb, 33);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 1);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 11);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 11, 32, 11, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 40, 40, 3, 40, 3, 1);
    ok_lookup_fails(&Mcb, 43);

    FsRtlRemoveLargeMcbEntry(&Mcb, 40, 3);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 42, 42, 32);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 40, -1, 2, -1, 42, 0);
    ok_lookup_succeeds(&Mcb, 42, 42, 32, 42, 32, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 42, 6);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 48, 48, 26, 48, 26, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 48, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 56, 56, 18, 56, 18, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 56, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 64, 64, 10, 64, 10, 1);
    ok_lookup_succeeds(&Mcb, 64, 64, 10, 64, 10, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 64, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 72, 72, 2, 72, 2, 1);
    ok_lookup_fails(&Mcb, 74);

    FsRtlRemoveLargeMcbEntry(&Mcb, 72, 2);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 73, 73, 2);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 74, 74, 33);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 106, 106, 20);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_succeeds(&Mcb, 33, -1, 40, -1, 40, 2);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 1);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 72, -1, 1, -1, 73, 0);
    ok_lookup_succeeds(&Mcb, 73, 73, 53, 73, 53, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 73, 7);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 80, 80, 46, 80, 46, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 80, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 88, 88, 38, 88, 38, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 88, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 96, 96, 30, 96, 30, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 96, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 104, 104, 22, 104, 22, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 104, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 112, 112, 14, 112, 14, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 112, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 120, 120, 6, 120, 6, 1);
    ok_lookup_fails(&Mcb, 126);

    FsRtlRemoveLargeMcbEntry(&Mcb, 120, 6);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 125, 125, 9);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 120, -1, 5, -1, 125, 0);
    ok_lookup_succeeds(&Mcb, 125, 125, 9, 125, 9, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 125, 3);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 128, 128, 6, 128, 6, 1);
    ok_lookup_fails(&Mcb, 134);

    FsRtlRemoveLargeMcbEntry(&Mcb, 128, 6);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 133, 133, 45);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 106, 106, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 177, 177, 17);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 104, -1, 2, -1, 106, 0);
    ok_lookup_succeeds(&Mcb, 106, 106, 1, 106, 1, 1);
    ok_lookup_succeeds(&Mcb, 107, -1, 26, -1, 26, 2);

    FsRtlRemoveLargeMcbEntry(&Mcb, 106, 1);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 128, -1, 5, -1, 133, 0);
    ok_lookup_succeeds(&Mcb, 133, 133, 61, 133, 61, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 133, 3);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 136, 136, 58, 136, 58, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 136, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 144, 144, 50, 144, 50, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 144, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 152, 152, 42, 152, 42, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 152, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 160, 160, 34, 160, 34, 1);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 193, 193, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    FsRtlRemoveLargeMcbEntry(&Mcb, 160, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 168, 168, 26, 168, 26, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 168, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 176, 176, 18, 176, 18, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 176, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 184, 184, 10, 184, 10, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 184, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 192, 192, 2, 192, 2, 1);
    ok_lookup_fails(&Mcb, 194);

    FsRtlRemoveLargeMcbEntry(&Mcb, 192, 2);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 193, 193, 3);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 192, -1, 1, -1, 193, 0);
    ok_lookup_succeeds(&Mcb, 193, 193, 3, 193, 3, 1);
    ok_lookup_fails(&Mcb, 196);

    FsRtlRemoveLargeMcbEntry(&Mcb, 193, 3);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 195, 195, 7);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 192, -1, 3, -1, 195, 0);
    ok_lookup_succeeds(&Mcb, 195, 195, 7, 195, 7, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 195, 5);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 200, 200, 2, 200, 2, 1);
    ok_lookup_fails(&Mcb, 202);

    FsRtlRemoveLargeMcbEntry(&Mcb, 200, 2);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 201, 201, 3);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 204, 204, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 200, -1, 1, -1, 201, 0);
    ok_lookup_succeeds(&Mcb, 201, 201, 4, 201, 4, 1);
    ok_lookup_fails(&Mcb, 205);

    FsRtlRemoveLargeMcbEntry(&Mcb, 201, 4);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 204, 204, 34);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 200, -1, 4, -1, 204, 0);
    ok_lookup_succeeds(&Mcb, 204, 204, 34, 204, 34, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 204, 4);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 208, 208, 30, 208, 30, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 208, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 216, 216, 22, 216, 22, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 216, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 224, 224, 14, 224, 14, 1);
    FsRtlRemoveLargeMcbEntry(&Mcb, 224, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 232, 232, 6, 232, 6, 1);
    ok_lookup_fails(&Mcb, 238);

    FsRtlRemoveLargeMcbEntry(&Mcb, 232, 6);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 237, 237, 11);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 232, -1, 5, -1, 237, 0);
    ok_lookup_succeeds(&Mcb, 237, 237, 11, 237, 11, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 237, 3);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 240, 240, 8, 240, 8, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 240, 8);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 247, 247, 8);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 32, 32, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 254, 254, 5);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    Result = FsRtlAddLargeMcbEntry(&Mcb, 259, 259, 21);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 254, 254, 1);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 279, 279, 13);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 290, 290, 4);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    Result = FsRtlAddLargeMcbEntry(&Mcb, 293, 293, 9);
    ok(Result == TRUE, "Expected FsRtlAddLargeMcbEntry to succeed.\n");

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 32, 32, 1, 32, 1, 1);
    ok_lookup_succeeds(&Mcb, 33, -1, 214, -1, 214, 2);

    FsRtlRemoveLargeMcbEntry(&Mcb, 32, 1);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 240, -1, 7, -1, 247, 0);
    ok_lookup_succeeds(&Mcb, 247, 247, 55, 247, 55, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 247, 1);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 248, 248, 54, 248, 54, 1);
    ok_lookup_succeeds(&Mcb, 255, 255, 47, 248, 54, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 248, 7);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 2, "Expected two runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 256, 256, 46, 255, 47, 1);
    ok_lookup_succeeds(&Mcb, 264, 264, 38, 255, 47, 1);
    ok_lookup_succeeds(&Mcb, 272, 272, 30, 255, 47, 1);
    ok_lookup_succeeds(&Mcb, 279, 279, 23, 255, 47, 1);

    FsRtlRemoveLargeMcbEntry(&Mcb, 279, 1);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 280, 280, 22, 280, 22, 3);

    FsRtlRemoveLargeMcbEntry(&Mcb, 280, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 288, 288, 14, 288, 14, 3);
    ok_lookup_succeeds(&Mcb, 292, 292, 10, 288, 14, 3);
    ok_lookup_succeeds(&Mcb, 293, 293, 9, 288, 14, 3);

    FsRtlRemoveLargeMcbEntry(&Mcb, 288, 8);

    RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
    ok(RunCount == 4, "Expected four runs, got %lu\n", RunCount);

    ok_lookup_succeeds(&Mcb, 296, 296, 6, 296, 6, 3);
    ok_lookup_succeeds(&Mcb, 301, 301, 1, 296, 6, 3);

    FsRtlUninitializeLargeMcb(&Mcb);
}

enum mcb_test_op
{
    mcb_add,
    mcb_remove,
    mcb_lookup,
    end_test
};
struct mcb_test_entry
{
    enum mcb_test_op test_op;
    LONGLONG Vbn;
    union
    {
        struct
        {
            LONGLONG Lbn;
            LONGLONG SectorCount;
            ULONG RunCount;
            struct
            {
                LONGLONG Vbn;
                LONGLONG Lbn;
                LONGLONG SectorCount;
            } Runs[20];
        } add;
        struct
        {
            LONGLONG SectorCount;
            ULONG RunCount;
            struct
            {
                LONGLONG Vbn;
                LONGLONG Lbn;
                LONGLONG SectorCount;
            } Runs[20];
        } remove;
        struct
        {
            BOOLEAN Result;
            LONGLONG Lbn;
            LONGLONG SectorCount;
            LONGLONG StartLbn;
            LONGLONG SectorCountFromStart;
            ULONG Index;
        } lookup;
    };
};

static VOID FsRtlLargeMcbTestsFastFat_3(VOID)
{
    static const struct mcb_test_entry test_entries[] =
    {
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_remove, 32, .remove={1, 0}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 32, .add={32, 1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_remove, 33, .remove={1, 2, {{0, -1, 32}, {32, 32, 1}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 33, .add={33, 1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_remove, 34, .remove={1, 2, {{0, -1, 32}, {32, 32, 2}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 34, .add={34, 1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_remove, 35, .remove={1, 2, {{0, -1, 32}, {32, 32, 3}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 35, .add={35, 1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 36, .add={36, 1, 2, {{0, -1, 32}, {32, 32, 5}}}},
        {mcb_remove, 36, .remove={1, 2, {{0, -1, 32}, {32, 32, 4}}}},
        {mcb_add, 36, .add={36, 1, 2, {{0, -1, 32}, {32, 32, 5}}}},
        {mcb_add, 37, .add={37, 1, 2, {{0, -1, 32}, {32, 32, 6}}}},
        {mcb_remove, 37, .remove={1, 2, {{0, -1, 32}, {32, 32, 5}}}},
        {mcb_lookup, 32, .lookup={TRUE, 32, 5, 32, 5, 1}},
        {mcb_lookup, 32, .lookup={TRUE, 32, 5, 32, 5, 1}},
        {mcb_lookup, 38, .lookup={FALSE}},
        {mcb_remove, 32, .remove={6, 0}},
        {mcb_add, 37, .add={37, 1, 2, {{0, -1, 37}, {37, 37, 1}}}},
        {mcb_remove, 37, .remove={1, 0}},
        {mcb_add, 37, .add={37, 1, 2, {{0, -1, 37}, {37, 37, 1}}}},
        {mcb_add, 37, .add={37, 1, 2, {{0, -1, 37}, {37, 37, 1}}}},
        {mcb_add, 37, .add={37, 1, 2, {{0, -1, 37}, {37, 37, 1}}}},
        {mcb_add, 38, .add={38, 1, 2, {{0, -1, 37}, {37, 37, 2}}}},
        {mcb_remove, 38, .remove={1, 2, {{0, -1, 37}, {37, 37, 1}}}},
        {mcb_add, 38, .add={38, 1, 2, {{0, -1, 37}, {37, 37, 2}}}},
        {mcb_add, 38, .add={38, 1, 2, {{0, -1, 37}, {37, 37, 2}}}},
        {mcb_add, 38, .add={38, 1, 2, {{0, -1, 37}, {37, 37, 2}}}},
        {mcb_add, 39, .add={39, 1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_remove, 39, .remove={1, 2, {{0, -1, 37}, {37, 37, 2}}}},
        {mcb_add, 39, .add={39, 1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 39, .add={39, 1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 39, .add={39, 1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 39, .add={39, 1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 40, .add={40, 1, 2, {{0, -1, 37}, {37, 37, 4}}}},
        {mcb_remove, 40, .remove={1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 41, .add={41, 1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 1}, {41, 41, 1}}}},
        {mcb_remove, 41, .remove={1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 42, .add={42, 1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 1}}}},
        {mcb_remove, 42, .remove={1, 2, {{0, -1, 37}, {37, 37, 3}}}},
        {mcb_add, 42, .add={42, 1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 1}}}},
        {mcb_add, 43, .add={43, 1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_remove, 43, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 1}}}},
        {mcb_add, 43, .add={43, 1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 44, .add={44, 1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 3}}}},
        {mcb_remove, 44, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 45, .add={45, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 1}, {45, 45, 1}}}},
        {mcb_remove, 45, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 46, .add={46, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 2}, {46, 46, 1}}}},
        {mcb_remove, 46, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 47, .add={47, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 3}, {47, 47, 1}}}},
        {mcb_remove, 47, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 48, .add={48, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 4}, {48, 48, 1}}}},
        {mcb_remove, 48, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 49, .add={49, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 5}, {49, 49, 1}}}},
        {mcb_remove, 49, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 50, .add={50, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 6}, {50, 50, 1}}}},
        {mcb_remove, 50, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 51, .add={51, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 7}, {51, 51, 1}}}},
        {mcb_remove, 51, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 52, .add={52, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 8}, {52, 52, 1}}}},
        {mcb_remove, 52, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 53, .add={53, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 9}, {53, 53, 1}}}},
        {mcb_remove, 53, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 54, .add={54, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 10}, {54, 54, 1}}}},
        {mcb_remove, 54, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 55, .add={55, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 11}, {55, 55, 1}}}},
        {mcb_remove, 55, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 56, .add={56, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 12}, {56, 56, 1}}}},
        {mcb_remove, 56, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 57, .add={57, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 13}, {57, 57, 1}}}},
        {mcb_remove, 57, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 58, .add={58, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 14}, {58, 58, 1}}}},
        {mcb_remove, 58, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 59, .add={59, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 15}, {59, 59, 1}}}},
        {mcb_remove, 59, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 60, .add={60, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 16}, {60, 60, 1}}}},
        {mcb_remove, 60, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 61, .add={61, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 17}, {61, 61, 1}}}},
        {mcb_remove, 61, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 62, .add={62, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 18}, {62, 62, 1}}}},
        {mcb_remove, 62, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 63, .add={63, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 19}, {63, 63, 1}}}},
        {mcb_remove, 63, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 64, .add={64, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 20}, {64, 64, 1}}}},
        {mcb_remove, 64, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 65, .add={65, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 21}, {65, 65, 1}}}},
        {mcb_remove, 65, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 66, .add={66, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 22}, {66, 66, 1}}}},
        {mcb_remove, 66, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 67, .add={67, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 23}, {67, 67, 1}}}},
        {mcb_remove, 67, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 68, .add={68, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 24}, {68, 68, 1}}}},
        {mcb_remove, 68, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 69, .add={69, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_remove, 69, .remove={1, 4, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}}}},
        {mcb_add, 69, .add={69, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_add, 69, .add={69, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_add, 69, .add={69, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_add, 69, .add={69, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_add, 69, .add={69, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_add, 70, .add={70, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 2}}}},
        {mcb_remove, 70, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 1}}}},
        {mcb_add, 70, .add={70, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 2}}}},
        {mcb_add, 70, .add={70, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 2}}}},
        {mcb_add, 70, .add={70, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 2}}}},
        {mcb_add, 70, .add={70, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 2}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_remove, 71, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 2}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 71, .add={71, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 72, .add={72, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 4}}}},
        {mcb_remove, 72, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 3}}}},
        {mcb_add, 72, .add={72, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 4}}}},
        {mcb_add, 72, .add={72, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 4}}}},
        {mcb_add, 72, .add={72, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 4}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_remove, 73, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 4}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 73, .add={73, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_remove, 74, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 5}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 74, .add={74, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_remove, 75, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 6}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 75, .add={75, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_remove, 76, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 7}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 76, .add={76, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 77, .add={77, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 9}}}},
        {mcb_remove, 77, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 8}}}},
        {mcb_add, 77, .add={77, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 9}}}},
        {mcb_add, 77, .add={77, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 9}}}},
        {mcb_add, 77, .add={77, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 9}}}},
        {mcb_add, 77, .add={77, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 9}}}},
        {mcb_add, 78, .add={78, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 10}}}},
        {mcb_remove, 78, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 9}}}},
        {mcb_add, 78, .add={78, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 10}}}},
        {mcb_add, 78, .add={78, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 10}}}},
        {mcb_add, 78, .add={78, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 10}}}},
        {mcb_add, 79, .add={79, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 11}}}},
        {mcb_remove, 79, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 10}}}},
        {mcb_add, 79, .add={79, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 11}}}},
        {mcb_add, 79, .add={79, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 11}}}},
        {mcb_add, 79, .add={79, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 11}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_remove, 80, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 11}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 80, .add={80, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_remove, 81, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 12}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 81, .add={81, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 82, .add={82, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 14}}}},
        {mcb_remove, 82, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 13}}}},
        {mcb_add, 82, .add={82, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 14}}}},
        {mcb_add, 82, .add={82, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 14}}}},
        {mcb_add, 82, .add={82, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 14}}}},
        {mcb_add, 83, .add={83, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 15}}}},
        {mcb_remove, 83, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 14}}}},
        {mcb_add, 83, .add={83, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 15}}}},
        {mcb_add, 83, .add={83, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 15}}}},
        {mcb_add, 83, .add={83, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 15}}}},
        {mcb_add, 84, .add={84, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 16}}}},
        {mcb_remove, 84, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 15}}}},
        {mcb_add, 84, .add={84, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 16}}}},
        {mcb_add, 85, .add={85, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 17}}}},
        {mcb_remove, 85, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 16}}}},
        {mcb_add, 85, .add={85, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 17}}}},
        {mcb_add, 85, .add={85, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 17}}}},
        {mcb_add, 85, .add={85, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 17}}}},
        {mcb_add, 85, .add={85, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 17}}}},
        {mcb_add, 86, .add={86, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 18}}}},
        {mcb_remove, 86, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 17}}}},
        {mcb_add, 86, .add={86, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 18}}}},
        {mcb_add, 86, .add={86, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 18}}}},
        {mcb_add, 86, .add={86, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 18}}}},
        {mcb_add, 86, .add={86, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 18}}}},
        {mcb_add, 87, .add={87, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 19}}}},
        {mcb_remove, 87, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 18}}}},
        {mcb_add, 87, .add={87, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 19}}}},
        {mcb_add, 87, .add={87, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 19}}}},
        {mcb_add, 87, .add={87, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 19}}}},
        {mcb_add, 88, .add={88, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_remove, 88, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 19}}}},
        {mcb_add, 88, .add={88, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_add, 88, .add={88, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_add, 88, .add={88, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_add, 88, .add={88, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_add, 88, .add={88, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_remove, 89, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 20}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 89, .add={89, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_remove, 90, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 21}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 90, .add={90, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_remove, 91, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 22}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 91, .add={91, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 92, .add={92, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}}}},
        {mcb_remove, 92, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 23}}}},
        {mcb_add, 92, .add={92, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}}}},
        {mcb_add, 93, .add={93, 1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 25}}}},
        {mcb_remove, 93, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}}}},
        {mcb_add, 94, .add={94, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_remove, 94, .remove={1, 6, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}}}},
        {mcb_add, 94, .add={94, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_add, 94, .add={94, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_add, 94, .add={94, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_add, 94, .add={94, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_add, 94, .add={94, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_remove, 95, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 1}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 95, .add={95, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 96, .add={96, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_remove, 96, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 2}}}},
        {mcb_add, 96, .add={96, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_add, 96, .add={96, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_add, 96, .add={96, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_add, 96, .add={96, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_add, 96, .add={96, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_add, 97, .add={97, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 4}}}},
        {mcb_remove, 97, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 3}}}},
        {mcb_add, 97, .add={97, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 4}}}},
        {mcb_add, 97, .add={97, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 4}}}},
        {mcb_add, 97, .add={97, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 4}}}},
        {mcb_add, 97, .add={97, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 4}}}},
        {mcb_add, 98, .add={98, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 5}}}},
        {mcb_remove, 98, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 4}}}},
        {mcb_add, 98, .add={98, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 5}}}},
        {mcb_add, 98, .add={98, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 5}}}},
        {mcb_add, 99, .add={99, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 6}}}},
        {mcb_remove, 99, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 5}}}},
        {mcb_add, 99, .add={99, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 6}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_remove, 100, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 6}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 32, .add={32, 1, 10, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_remove, 32, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_remove, 101, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 7}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 100, .add={100, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_remove, 102, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 8}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 101, .add={101, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 102, .add={102, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 103, .add={103, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 10}}}},
        {mcb_remove, 103, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 9}}}},
        {mcb_add, 103, .add={103, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 10}}}},
        {mcb_add, 103, .add={103, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 10}}}},
        {mcb_add, 103, .add={103, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 10}}}},
        {mcb_add, 103, .add={103, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 10}}}},
        {mcb_add, 104, .add={104, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 11}}}},
        {mcb_remove, 104, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 10}}}},
        {mcb_add, 104, .add={104, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 11}}}},
        {mcb_add, 104, .add={104, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 11}}}},
        {mcb_add, 104, .add={104, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 11}}}},
        {mcb_add, 105, .add={105, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 12}}}},
        {mcb_remove, 105, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 11}}}},
        {mcb_add, 105, .add={105, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 12}}}},
        {mcb_add, 105, .add={105, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 12}}}},
        {mcb_add, 105, .add={105, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 12}}}},
        {mcb_add, 106, .add={106, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 13}}}},
        {mcb_remove, 106, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 12}}}},
        {mcb_add, 106, .add={106, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 13}}}},
        {mcb_add, 106, .add={106, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 13}}}},
        {mcb_add, 106, .add={106, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 13}}}},
        {mcb_add, 107, .add={107, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 14}}}},
        {mcb_remove, 107, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 13}}}},
        {mcb_add, 107, .add={107, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 14}}}},
        {mcb_add, 107, .add={107, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 14}}}},
        {mcb_add, 107, .add={107, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 14}}}},
        {mcb_add, 108, .add={108, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 15}}}},
        {mcb_remove, 108, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 14}}}},
        {mcb_add, 108, .add={108, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 15}}}},
        {mcb_add, 108, .add={108, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 15}}}},
        {mcb_add, 108, .add={108, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 15}}}},
        {mcb_add, 109, .add={109, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 16}}}},
        {mcb_remove, 109, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 15}}}},
        {mcb_add, 109, .add={109, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 16}}}},
        {mcb_add, 109, .add={109, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 16}}}},
        {mcb_add, 109, .add={109, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 16}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_remove, 110, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 16}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 110, .add={110, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 111, .add={111, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 18}}}},
        {mcb_remove, 111, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 17}}}},
        {mcb_add, 111, .add={111, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 18}}}},
        {mcb_add, 111, .add={111, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 18}}}},
        {mcb_add, 112, .add={112, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 19}}}},
        {mcb_remove, 112, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 18}}}},
        {mcb_add, 112, .add={112, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 19}}}},
        {mcb_add, 112, .add={112, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 19}}}},
        {mcb_add, 113, .add={113, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 20}}}},
        {mcb_remove, 113, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 19}}}},
        {mcb_add, 113, .add={113, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 20}}}},
        {mcb_add, 114, .add={114, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 21}}}},
        {mcb_remove, 114, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 20}}}},
        {mcb_add, 114, .add={114, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 21}}}},
        {mcb_add, 115, .add={115, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 22}}}},
        {mcb_remove, 115, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 21}}}},
        {mcb_add, 115, .add={115, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 22}}}},
        {mcb_add, 115, .add={115, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 22}}}},
        {mcb_add, 116, .add={116, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 23}}}},
        {mcb_remove, 116, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 22}}}},
        {mcb_add, 116, .add={116, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 23}}}},
        {mcb_add, 117, .add={117, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 24}}}},
        {mcb_remove, 117, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 23}}}},
        {mcb_add, 117, .add={117, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 24}}}},
        {mcb_add, 118, .add={118, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 25}}}},
        {mcb_remove, 118, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 24}}}},
        {mcb_add, 118, .add={118, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 25}}}},
        {mcb_add, 118, .add={118, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 25}}}},
        {mcb_add, 119, .add={119, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 26}}}},
        {mcb_remove, 119, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 25}}}},
        {mcb_add, 119, .add={119, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 26}}}},
        {mcb_add, 120, .add={120, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 27}}}},
        {mcb_remove, 120, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 26}}}},
        {mcb_add, 120, .add={120, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 27}}}},
        {mcb_add, 121, .add={121, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 28}}}},
        {mcb_remove, 121, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 27}}}},
        {mcb_add, 121, .add={121, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 28}}}},
        {mcb_add, 121, .add={121, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 28}}}},
        {mcb_add, 122, .add={122, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 29}}}},
        {mcb_remove, 122, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 28}}}},
        {mcb_add, 122, .add={122, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 29}}}},
        {mcb_add, 123, .add={123, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 30}}}},
        {mcb_remove, 123, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 29}}}},
        {mcb_add, 123, .add={123, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 30}}}},
        {mcb_add, 124, .add={124, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 31}}}},
        {mcb_remove, 124, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 30}}}},
        {mcb_add, 124, .add={124, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 31}}}},
        {mcb_add, 124, .add={124, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 31}}}},
        {mcb_add, 125, .add={125, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 32}}}},
        {mcb_remove, 125, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 31}}}},
        {mcb_add, 125, .add={125, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 32}}}},
        {mcb_add, 126, .add={126, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 33}}}},
        {mcb_remove, 126, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 32}}}},
        {mcb_add, 126, .add={126, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 33}}}},
        {mcb_add, 127, .add={127, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 34}}}},
        {mcb_remove, 127, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 33}}}},
        {mcb_add, 127, .add={127, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 34}}}},
        {mcb_add, 127, .add={127, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 34}}}},
        {mcb_add, 127, .add={127, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 34}}}},
        {mcb_add, 128, .add={128, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_remove, 128, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 34}}}},
        {mcb_add, 128, .add={128, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_add, 128, .add={128, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_add, 128, .add={128, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_add, 128, .add={128, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_add, 128, .add={128, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_add, 129, .add={129, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_remove, 129, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 35}}}},
        {mcb_add, 129, .add={129, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_add, 129, .add={129, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_add, 129, .add={129, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_add, 129, .add={129, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_add, 129, .add={129, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_add, 130, .add={130, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 37}}}},
        {mcb_remove, 130, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 36}}}},
        {mcb_add, 130, .add={130, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 37}}}},
        {mcb_add, 130, .add={130, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 37}}}},
        {mcb_add, 131, .add={131, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}}}},
        {mcb_remove, 131, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 37}}}},
        {mcb_add, 131, .add={131, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}}}},
        {mcb_add, 132, .add={132, 1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 39}}}},
        {mcb_remove, 132, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_remove, 133, .remove={1, 8, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 10, {{0, -1, 37}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 32, .add={32, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 133, .add={133, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_remove, 134, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 1}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 134, .add={134, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 135, .add={135, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_remove, 135, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 2}}}},
        {mcb_add, 135, .add={135, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_add, 135, .add={135, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_add, 135, .add={135, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_add, 135, .add={135, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_add, 135, .add={135, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_add, 136, .add={136, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 4}}}},
        {mcb_remove, 136, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 3}}}},
        {mcb_add, 136, .add={136, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 4}}}},
        {mcb_add, 136, .add={136, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 4}}}},
        {mcb_add, 136, .add={136, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 4}}}},
        {mcb_add, 136, .add={136, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 4}}}},
        {mcb_add, 137, .add={137, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_remove, 137, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 4}}}},
        {mcb_add, 137, .add={137, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_add, 137, .add={137, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_add, 137, .add={137, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_add, 138, .add={138, 1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 6}}}},
        {mcb_remove, 138, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_add, 139, .add={139, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 1}, {139, 139, 1}}}},
        {mcb_remove, 139, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_add, 140, .add={140, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 1}}}},
        {mcb_remove, 140, .remove={1, 12, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}}}},
        {mcb_add, 140, .add={140, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 1}}}},
        {mcb_add, 140, .add={140, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 1}}}},
        {mcb_add, 141, .add={141, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_remove, 141, .remove={1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 1}}}},
        {mcb_add, 141, .add={141, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_add, 141, .add={141, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_add, 141, .add={141, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_add, 141, .add={141, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_add, 142, .add={142, 1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 3}}}},
        {mcb_remove, 142, .remove={1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_add, 143, .add={143, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 1}}}},
        {mcb_remove, 143, .remove={1, 14, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}}}},
        {mcb_add, 143, .add={143, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 1}}}},
        {mcb_add, 143, .add={143, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 1}}}},
        {mcb_add, 143, .add={143, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 1}}}},
        {mcb_add, 143, .add={143, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 1}}}},
        {mcb_add, 144, .add={144, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_remove, 144, .remove={1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 1}}}},
        {mcb_add, 144, .add={144, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_add, 144, .add={144, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_add, 144, .add={144, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_add, 144, .add={144, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_add, 144, .add={144, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_add, 145, .add={145, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 3}}}},
        {mcb_remove, 145, .remove={1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 2}}}},
        {mcb_add, 145, .add={145, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 3}}}},
        {mcb_add, 145, .add={145, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 3}}}},
        {mcb_add, 145, .add={145, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 3}}}},
        {mcb_add, 145, .add={145, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 3}}}},
        {mcb_add, 146, .add={146, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 4}}}},
        {mcb_remove, 146, .remove={1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 3}}}},
        {mcb_add, 146, .add={146, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 4}}}},
        {mcb_add, 146, .add={146, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 4}}}},
        {mcb_add, 147, .add={147, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_remove, 147, .remove={1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 4}}}},
        {mcb_add, 147, .add={147, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 147, .add={147, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 147, .add={147, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 147, .add={147, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 147, .add={147, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 148, .add={148, 1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 6}}}},
        {mcb_remove, 148, .remove={1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 149, .add={149, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 1}}}},
        {mcb_remove, 149, .remove={1, 16, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}}}},
        {mcb_add, 149, .add={149, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 1}}}},
        {mcb_add, 150, .add={150, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 2}}}},
        {mcb_remove, 150, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 1}}}},
        {mcb_add, 150, .add={150, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 2}}}},
        {mcb_add, 151, .add={151, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 3}}}},
        {mcb_remove, 151, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 2}}}},
        {mcb_add, 151, .add={151, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 3}}}},
        {mcb_add, 151, .add={151, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 3}}}},
        {mcb_add, 151, .add={151, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 3}}}},
        {mcb_add, 152, .add={152, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 4}}}},
        {mcb_remove, 152, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 3}}}},
        {mcb_add, 152, .add={152, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 4}}}},
        {mcb_add, 152, .add={152, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 4}}}},
        {mcb_add, 152, .add={152, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 4}}}},
        {mcb_add, 153, .add={153, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 5}}}},
        {mcb_remove, 153, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 4}}}},
        {mcb_add, 153, .add={153, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 5}}}},
        {mcb_add, 154, .add={154, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 6}}}},
        {mcb_remove, 154, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 5}}}},
        {mcb_add, 154, .add={154, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 6}}}},
        {mcb_add, 154, .add={154, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 6}}}},
        {mcb_add, 154, .add={154, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 6}}}},
        {mcb_add, 154, .add={154, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 6}}}},
        {mcb_add, 155, .add={155, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 7}}}},
        {mcb_remove, 155, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 6}}}},
        {mcb_add, 155, .add={155, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 7}}}},
        {mcb_add, 155, .add={155, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 7}}}},
        {mcb_add, 155, .add={155, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 7}}}},
        {mcb_add, 155, .add={155, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 7}}}},
        {mcb_add, 156, .add={156, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 8}}}},
        {mcb_remove, 156, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 7}}}},
        {mcb_add, 156, .add={156, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 8}}}},
        {mcb_add, 156, .add={156, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 8}}}},
        {mcb_add, 156, .add={156, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 8}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_remove, 157, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 8}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 157, .add={157, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 158, .add={158, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 10}}}},
        {mcb_remove, 158, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 9}}}},
        {mcb_add, 158, .add={158, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 10}}}},
        {mcb_add, 159, .add={159, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_remove, 159, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 10}}}},
        {mcb_add, 159, .add={159, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 159, .add={159, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 159, .add={159, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 159, .add={159, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 160, .add={160, 1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 12}}}},
        {mcb_remove, 160, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 161, .add={161, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 1}, {161, 161, 1}}}},
        {mcb_remove, 161, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 162, .add={162, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 2}, {162, 162, 1}}}},
        {mcb_remove, 162, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 163, .add={163, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 3}, {163, 163, 1}}}},
        {mcb_remove, 163, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 164, .add={164, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 4}, {164, 164, 1}}}},
        {mcb_remove, 164, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 165, .add={165, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 5}, {165, 165, 1}}}},
        {mcb_remove, 165, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 166, .add={166, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 6}, {166, 166, 1}}}},
        {mcb_remove, 166, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 167, .add={167, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 7}, {167, 167, 1}}}},
        {mcb_remove, 167, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 168, .add={168, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 8}, {168, 168, 1}}}},
        {mcb_remove, 168, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 169, .add={169, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 9}, {169, 169, 1}}}},
        {mcb_remove, 169, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 170, .add={170, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 10}, {170, 170, 1}}}},
        {mcb_remove, 170, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_add, 171, .add={171, 1, 20, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}, {160, -1, 11}, {171, 171, 1}}}},
        {mcb_remove, 171, .remove={1, 18, {{0, -1, 32}, {32, 32, 1}, {33, -1, 4}, {37, 37, 3}, {40, -1, 2}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_lookup, 32, .lookup={TRUE, 32, 1, 32, 1, 1}},
        {mcb_lookup, 32, .lookup={TRUE, 32, 1, 32, 1, 1}},
        {mcb_lookup, 33, .lookup={TRUE, -1, 4, -1, 4, 2}},
        {mcb_lookup, 37, .lookup={TRUE, 37, 3, 37, 3, 3}},
        {mcb_remove, 32, .remove={8, 14, {{0, -1, 42}, {42, 42, 2}, {44, -1, 25}, {69, 69, 24}, {93, -1, 1}, {94, 94, 38}, {132, -1, 1}, {133, 133, 5}, {138, -1, 2}, {140, 140, 2}, {142, -1, 1}, {143, 143, 5}, {148, -1, 1}, {149, 149, 11}}}},
        {mcb_lookup, 40, .lookup={TRUE, -1, 2, -1, 42, 0}},
        {mcb_lookup, 45, .lookup={TRUE, -1, 24, -1, 25, 2}},
        {end_test},
    };
    const struct mcb_test_entry* test_entry = &test_entries[0];
    LARGE_MCB Mcb;

    FsRtlInitializeLargeMcb(&Mcb, PagedPool);

    while(TRUE)
    {
        if (test_entry->test_op == end_test)
            goto end_test;

#if 0 /* Enable if you want to dump the expected results */
        if (test_entry->test_op == mcb_add)
        {
            ULONG RunCount;
            ULONG i;
            char trace_buffer[1024];
            char* cur_buffer = trace_buffer;

            ok(FsRtlAddLargeMcbEntry(&Mcb, test_entry->Vbn, test_entry->add.Lbn, test_entry->add.SectorCount), "Expected FsRtlAddLargeMcbEntry to succeed for test %d\n", test_entry - test_entries);

            RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);

            cur_buffer += sprintf(cur_buffer, "{mcb_add, %I64d, .add={%I64d, %I64d, %lu, {", test_entry->Vbn, test_entry->add.Lbn, test_entry->add.SectorCount, RunCount);

            for (i = 0; i < RunCount; i++)
            {
                LONGLONG Vbn, Lbn, SectorCount;
                ok(FsRtlGetNextLargeMcbEntry(&Mcb, i, &Vbn, &Lbn, &SectorCount), "Expected FsRtlGetNextLargeMcbEntry to succeed.\n");
                cur_buffer += sprintf(cur_buffer, "{%I64d, %I64d, %I64d}, ", Vbn, Lbn, SectorCount);
            }
            if (RunCount)
                cur_buffer -= 2;
            else
                cur_buffer -= 3;
            cur_buffer += sprintf(cur_buffer, "%s}},", RunCount ? "}" : "");

            ok(0, "%s\n", trace_buffer);
        }
        else if (test_entry->test_op == mcb_remove)
        {
            ULONG RunCount;
            ULONG i;
            char trace_buffer[1024];
            char* cur_buffer = trace_buffer;

            FsRtlRemoveLargeMcbEntry(&Mcb, test_entry->Vbn, test_entry->remove.SectorCount);

            RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);

            cur_buffer += sprintf(cur_buffer, "{mcb_remove, %I64d, .remove={%I64d, %lu, {", test_entry->Vbn, test_entry->remove.SectorCount, RunCount);

            for (i = 0; i < RunCount; i++)
            {
                LONGLONG Vbn, Lbn, SectorCount;
                ok(FsRtlGetNextLargeMcbEntry(&Mcb, i, &Vbn, &Lbn, &SectorCount), "Expected FsRtlGetNextLargeMcbEntry to succeed.\n");
                cur_buffer += sprintf(cur_buffer, "{%I64d, %I64d, %I64d}, ", Vbn, Lbn, SectorCount);
            }
            if (RunCount)
                cur_buffer -= 2;
            else
                cur_buffer -= 3;
            cur_buffer += sprintf(cur_buffer, "%s}},", RunCount ? "}" : "");

            ok(0, "%s\n", trace_buffer);
        }
        else if (test_entry->test_op == mcb_lookup)
        {
            LONGLONG Lbn, SectorCount, StartLbn, SectorCountFromStart;
            ULONG Index;
            BOOLEAN Result;

            Result = FsRtlLookupLargeMcbEntry(&Mcb, test_entry->Vbn, &Lbn, &SectorCount, &StartLbn, &SectorCountFromStart, &Index);
            if (Result)
                ok(0, "{mcb_lookup, %I64d, .lookup={TRUE, %I64d, %I64d, %I64d, %I64d, %lu}}\n", test_entry->Vbn, Lbn, SectorCount, StartLbn, SectorCountFromStart, Index);
            else
                ok(0, "{mcb_lookup, %I64d, .lookup={FALSE}}\n", test_entry->Vbn, Lbn, SectorCount, StartLbn, SectorCountFromStart, Index);
        }
#else
        if (test_entry->test_op == mcb_add)
        {
            ULONG RunCount;

            ok(FsRtlAddLargeMcbEntry(&Mcb, test_entry->Vbn, test_entry->add.Lbn, test_entry->add.SectorCount), "Expected FsRtlAddLargeMcbEntry to succeed for test %d\n", test_entry - test_entries);

            RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
            ok(RunCount == test_entry->add.RunCount, "Test %d: Expected Runcount %lu, got %lu.\n", test_entry - test_entries, test_entry->add.RunCount, RunCount);

            if (RunCount != test_entry->add.RunCount)
            {
                DumpAllRuns(&Mcb);
            }
            else
            {
                ULONG i;
                for (i = 0; i < RunCount; i++)
                {
                    LONGLONG Vbn, Lbn, SectorCount;
                    ok(FsRtlGetNextLargeMcbEntry(&Mcb, i, &Vbn, &Lbn, &SectorCount), "Expected FsRtlGetNextLargeMcbEntry to succeed.\n");
                    ok (Vbn == test_entry->add.Runs[i].Vbn, "Test %d, Run %lu: Expected Vbn %I64d, got %I64d.\n", test_entry - test_entries, i, test_entry->add.Runs[i].Vbn, Vbn);
                    ok (Lbn == test_entry->add.Runs[i].Lbn, "Test %d, Run %lu: Expected Lbn %I64d, got %I64d.\n", test_entry - test_entries, i, test_entry->add.Runs[i].Lbn, Lbn);
                    ok (SectorCount == test_entry->add.Runs[i].SectorCount, "Test %d, Run %lu: Expected SectorCount %I64d, got %I64d.\n", test_entry - test_entries, i, test_entry->add.Runs[i].SectorCount, SectorCount);
                }
            }
        }
        else if (test_entry->test_op == mcb_remove)
        {
            ULONG RunCount;

            FsRtlRemoveLargeMcbEntry(&Mcb, test_entry->Vbn, test_entry->remove.SectorCount);

            RunCount = FsRtlNumberOfRunsInLargeMcb(&Mcb);
            ok(RunCount == test_entry->remove.RunCount, "Test %d: Expected Runcount %lu, got %lu.\n", test_entry - test_entries, test_entry->remove.RunCount, RunCount);

            if (RunCount != test_entry->remove.RunCount)
            {
                DumpAllRuns(&Mcb);
            }
            else
            {
                ULONG i;
                for (i = 0; i < RunCount; i++)
                {
                    LONGLONG Vbn, Lbn, SectorCount;
                    ok(FsRtlGetNextLargeMcbEntry(&Mcb, i, &Vbn, &Lbn, &SectorCount), "Test %d, Run %lu: Expected FsRtlGetNextLargeMcbEntry to succeed.\n", test_entry - test_entries, i);
                    ok (Vbn == test_entry->remove.Runs[i].Vbn, "Test %d, Run %lu: Expected Vbn %I64d, got %I64d.\n", test_entry - test_entries, i, test_entry->remove.Runs[i].Vbn, Vbn);
                    ok (Lbn == test_entry->remove.Runs[i].Lbn, "Test %d, Run %lu: Expected Lbn %I64d, got %I64d.\n", test_entry - test_entries, i, test_entry->remove.Runs[i].Lbn, Lbn);
                    ok (SectorCount == test_entry->remove.Runs[i].SectorCount, "Test %d, Run %lu: Expected SectorCount %I64d, got %I64d.\n", test_entry - test_entries, i, test_entry->remove.Runs[i].SectorCount, SectorCount);
                }
            }
        }
        else if (test_entry->test_op == mcb_lookup)
        {
            LONGLONG Lbn, SectorCount, StartLbn, SectorCountFromStart;
            ULONG Index;
            BOOLEAN Result;

            Result = FsRtlLookupLargeMcbEntry(&Mcb, test_entry->Vbn, &Lbn, &SectorCount, &StartLbn, &SectorCountFromStart, &Index);
            ok (Result == test_entry->lookup.Result, "Test %d: Expected FsRtlLookupLargeMcbEntry to %s.\n", test_entry - test_entries, test_entry->lookup.Result ? "succeed" : "fail");
            if (Result)
            {
                ok (Lbn == test_entry->lookup.Lbn, "Test %d: Expected Lbn %I64d, got %I64d.\n", test_entry - test_entries, test_entry->lookup.Lbn, Lbn);
                ok (SectorCount == test_entry->lookup.SectorCount, "Test %d: Expected SectorCount %I64d, got %I64d.\n", test_entry - test_entries, test_entry->lookup.SectorCount, SectorCount);
                ok (StartLbn == test_entry->lookup.StartLbn, "Test %d: Expected StartLbn %I64d, got %I64d.\n", test_entry - test_entries, test_entry->lookup.StartLbn, StartLbn);
                ok (SectorCountFromStart == test_entry->lookup.SectorCountFromStart, "Test %d: Expected SectorCountFromStart %I64d, got %I64d.\n", test_entry - test_entries, test_entry->lookup.SectorCountFromStart, SectorCountFromStart);
            }
        }
#endif

        test_entry++;
    }
end_test:
    FsRtlUninitializeLargeMcb(&Mcb);
}

START_TEST(FsRtlMcb)
{
    FsRtlMcbTest();
    FsRtlLargeMcbTest();
    FsRtlLargeMcbTestsExt2();
    FsRtlLargeMcbTestsFastFat();
    FsRtlLargeMcbTestsFastFat_2();
    FsRtlLargeMcbTestsFastFat_3();
}
