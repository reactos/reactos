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

static VOID FsRtlLargeMcbTestsFastFat()
{
    LARGE_MCB FirstMcb;
    LONGLONG Lbn, Vbn, SectorCount;
    ULONG Index;
    BOOLEAN Result;

    FsRtlInitializeLargeMcb(&FirstMcb, PagedPool);

    Lbn = -1;
    SectorCount = -1;
    Result = FsRtlLookupLargeMcbEntry(&FirstMcb, 8388607LL, &Lbn, &SectorCount, NULL, NULL, NULL);
    ok_bool_false(Result, "FsRtlLookupLargeMcbEntry returned");
    ok_eq_longlong(Lbn, -1);
    ok_eq_longlong(SectorCount, -1);

    Vbn = -1;
    Lbn = -1;
    Index = (ULONG) -1;
    Result = FsRtlLookupLastLargeMcbEntryAndIndex(&FirstMcb, &Vbn, &Lbn, &Index);
    ok_bool_false(Result, "FsRtlLookupLastLargeMcbEntryAndIndex returned");
    ok_eq_longlong(Vbn, -1);
    ok_eq_longlong(Lbn, -1);
    ok_eq_ulong(Index, (ULONG) -1);

    FsRtlUninitializeLargeMcb(&FirstMcb);
}

START_TEST(FsRtlMcb)
{
    FsRtlMcbTest();
    FsRtlLargeMcbTest();
    FsRtlLargeMcbTestsExt2();
    FsRtlLargeMcbTestsFastFat();
}
