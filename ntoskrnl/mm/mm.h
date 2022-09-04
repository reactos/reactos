#pragma once

static inline VOID UpdateTotalCommittedPages(VOID)
{
    /*
     *   Add up all the used "Committed" memory + pagefile.
     *   Not sure this is right. 8^\
     *   MmTotalCommittedPages should be adjusted consistently with
    *    other counters at different places.
     */

    MmTotalCommittedPages = MiMemoryConsumers[MC_SYSTEM].PagesUsed +
                            MiMemoryConsumers[MC_USER].PagesUsed +
                            MiUsedSwapPages;

    if (MmTotalCommittedPages > MmPeakCommitment)
        MmPeakCommitment = MmTotalCommittedPages;
}
