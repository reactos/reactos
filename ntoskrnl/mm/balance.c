/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * PURPOSE:     kernel memory management functions

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "ARM3/miarm.h"

/* TYPES ********************************************************************/
typedef struct _MM_ALLOCATION_REQUEST {
    PFN_NUMBER Page;
    LIST_ENTRY ListEntry;
    KEVENT Event;
} MM_ALLOCATION_REQUEST, *PMM_ALLOCATION_REQUEST;

typedef struct _MM_CACHE_STATS {
    ULONG HitCount;
    ULONG MissCount;
    ULONG TrimCount;
    ULONG LastTrimSize;
} MM_CACHE_STATS;

/* GLOBALS ******************************************************************/

MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages;
static ULONG MiMinimumPagesPerRun;
static CLIENT_ID MiBalancerThreadId;
static HANDLE MiBalancerThreadHandle = NULL;
static KEVENT MiBalancerEvent;
static KEVENT MiBalancerDoneEvent;
static KTIMER MiBalancerTimer;

static LONG PageOutThreadActive;
static MM_CACHE_STATS MiCacheStats = {0, 0, 0, 0};

/* ========== AGGRESSIVE CACHING TUNING FOR 256MB RAM ========== */

/* Total RAM available: ~256MB = 65,536 pages (at 4KB/page) */
#define TOTAL_RAM_256MB (256*1024*1024/4096)

/* Keep at least this much free (3.2MB = 800 pages absolute minimum) */
#define ABSOLUTE_MINIMUM_FREE 800

/* Target to keep 70-72% of RAM usable for cache/user data */
#define CACHE_AGGRESSIVE_RATIO 0.72f

/* Memory pressure thresholds - finely tuned for 256MB */
#define CRITICAL_PRESSURE 94           /* Emergency: <2.5% free (1.6MB) */
#define EMERGENCY_PRESSURE 90          /* Very high: <5% free (3.2MB) */
#define HIGH_PRESSURE 85               /* High: <7% free (4.5MB) */
#define MODERATE_PRESSURE 75           /* Moderate: <10% free (6.4MB) */
#define LOW_PRESSURE 60                /* Low: >15% free (10MB) */

/* Balancer polling intervals - must be fast on 256MB to catch pressure spikes */
#define CRITICAL_POLL_MS 400           /* 0.4s - respond instantly to emergency */
#define EMERGENCY_POLL_MS 600          /* 0.6s - very fast response */
#define HIGH_POLL_MS 1000              /* 1.0s - aggressive polling */
#define NORMAL_POLL_MS 2400            /* 2.4s - normal operation */
#define RELAXED_POLL_MS 4000           /* 4.0s - when plenty of free RAM */

/* Trim batch sizes - small and frequent prevents freezes */
#define CRITICAL_BATCH (TOTAL_RAM_256MB/256)   /* ~256 pages at a time under critical pressure */
#define EMERGENCY_BATCH (TOTAL_RAM_256MB/384)  /* ~170 pages */
#define HIGH_BATCH (TOTAL_RAM_256MB/512)       /* ~128 pages */
#define NORMAL_BATCH (TOTAL_RAM_256MB/768)     /* ~85 pages */
#define RELAX_BATCH (TOTAL_RAM_256MB/1024)     /* ~64 pages */

/* i3 3rd Gen can handle faster delays due to better branch prediction */
#define ALLOCATION_DELAY_CRITICAL_MS 15     /* 15ms on critical */
#define ALLOCATION_DELAY_EMERGENCY_MS 10    /* 10ms on emergency */
#define ALLOCATION_DELAY_HIGH_MS 5          /* 5ms on high pressure */
#define ALLOCATION_DELAY_NORMAL_MS 2        /* 2ms on moderate */

/* Smart accessed-bit decay: how many cycles to keep a page before considering removal */
#define ACCESSED_BIT_DECAY_CYCLES 3

/* ===================================================================== */

CODE_SEG("INIT")
VOID
NTAPI
MmInitializeBalancer(ULONG AvailablePages, ULONG SystemPages)
{
    memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));

    /*
     * Configuration for 256MB RAM:
     * Goal: Keep as much as 70-72% for cache, but maintain emergency safety margins
     * Never let free pages drop below 800 pages (3.2MB) to avoid thrashing
     */

    /* Calculate minimum free pages: aim for 5-7% free under normal operation */
    MiMinimumAvailablePages = max(ABSOLUTE_MINIMUM_FREE, (AvailablePages * 7) / 100);

    /* Batch size: trim in small increments to avoid stutters */
    MiMinimumPagesPerRun = max(64, AvailablePages / 768);

    /* Aggressive: Allow user/cache to consume 72% of total RAM */
    MiMemoryConsumers[MC_USER].PagesTarget = (ULONG)(AvailablePages * CACHE_AGGRESSIVE_RATIO);

    DPRINT("╔══════════════════════════════════════════════════════════════╗\n");
    DPRINT("║  AGGRESSIVE SMART CACHE BALANCER - 256MB RAM + i3 3rd Gen    ║\n");
    DPRINT("╠══════════════════════════════════════════════════════════════╣\n");
    DPRINT("║ Total Available Pages: %lu (%.1f MB)                         ║\n", 
           AvailablePages, (float)AvailablePages * 4 / 1024);
    DPRINT("║ Minimum Free Pages: %lu (%.1f MB) - Safety margin             ║\n",
           MiMinimumAvailablePages, (float)MiMinimumAvailablePages * 4 / 1024);
    DPRINT("║ Cache/User Target: %lu (%.1f MB) - AGGRESSIVE 72%             ║\n",
           MiMemoryConsumers[MC_USER].PagesTarget, 
           (float)MiMemoryConsumers[MC_USER].PagesTarget * 4 / 1024);
    DPRINT("║ Batch Trim Size: %lu pages                                   ║\n", MiMinimumPagesPerRun);
    DPRINT("╚══════════════════════════════════════════════════════════════╝\n");
}

CODE_SEG("INIT")
VOID
NTAPI
MmInitializeMemoryConsumer(
    ULONG Consumer,
    NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed))
{
    MiMemoryConsumers[Consumer].Trim = Trim;
}

/**
 * @brief Calculate current memory pressure as percentage (0-100)
 * @return Memory pressure: 0 = lots free, 100 = completely full
 */
static ULONG
MiGetMemoryPressure(VOID)
{
    ULONG Total = MmNumberOfPhysicalPages ? MmNumberOfPhysicalPages : TOTAL_RAM_256MB;
    if (Total == 0) return 0;
    
    ULONG Used = Total - MmAvailablePages;
    return (Used * 100) / Total;
}

/**
 * @brief Get optimal polling interval based on memory pressure
 * @return Interval in milliseconds
 */
static ULONG
MiGetOptimalPollInterval(ULONG Pressure)
{
    if (Pressure >= CRITICAL_PRESSURE)  return CRITICAL_POLL_MS;
    if (Pressure >= EMERGENCY_PRESSURE) return EMERGENCY_POLL_MS;
    if (Pressure >= HIGH_PRESSURE)      return HIGH_POLL_MS;
    if (Pressure >= MODERATE_PRESSURE)  return NORMAL_POLL_MS;
    return RELAXED_POLL_MS;
}

/**
 * @brief Get optimal batch trim size based on memory pressure
 * @return Number of pages to trim in this batch
 */
static ULONG
MiGetOptimalBatchSize(ULONG Pressure)
{
    if (Pressure >= CRITICAL_PRESSURE)  return CRITICAL_BATCH;
    if (Pressure >= EMERGENCY_PRESSURE) return EMERGENCY_BATCH;
    if (Pressure >= HIGH_PRESSURE)      return HIGH_BATCH;
    if (Pressure >= MODERATE_PRESSURE)  return NORMAL_BATCH;
    return RELAX_BATCH;
}

/**
 * @brief Aggressive, smart memory consumer trimming
 * Never trim unless absolutely necessary, but react INSTANTLY to pressure
 */
ULONG
NTAPI
MiTrimMemoryConsumer(ULONG Consumer, ULONG InitialTarget)
{
    ULONG Target = InitialTarget;
    ULONG NrFreedPages = 0;
    NTSTATUS Status;
    ULONG Pressure = MiGetMemoryPressure();

    if (!MiMemoryConsumers[Consumer].Trim)
        return InitialTarget;

    /*
     * AGGRESSIVE CACHING LOGIC:
     * Only trim if we MUST. Never trim unless forced by low free pages.
     */

    if (MmAvailablePages < MiMinimumAvailablePages) {
        /* We need to free pages ASAP */
        ULONG Shortage = MiMinimumAvailablePages - MmAvailablePages;
        Target = max(Target, Shortage);

        /* Under critical pressure, be ultra-aggressive */
        if (Pressure >= CRITICAL_PRESSURE) {
            Target = max(Target, Shortage * 3);  /* Trim 3x the shortage */
            DPRINT1("CRITICAL PRESSURE %lu%%: Trimming %lu pages\n", Pressure, Target);
        } else if (Pressure >= EMERGENCY_PRESSURE) {
            Target = max(Target, Shortage * 2);  /* 2x */
        }
    } 
    else if (Pressure >= HIGH_PRESSURE) {
        /* Proactive trim under high pressure but not critical */
        ULONG ExcessPages = (MiMemoryConsumers[Consumer].PagesUsed > MiMemoryConsumers[Consumer].PagesTarget) ?
                           (MiMemoryConsumers[Consumer].PagesUsed - MiMemoryConsumers[Consumer].PagesTarget) : 0;
        if (ExcessPages) {
            Target = max(Target, (ExcessPages * 3) / 4);  /* Trim 75% of excess */
        }
    }
    else {
        /* Low pressure - don't trim, just keep cache! */
        return 0;
    }

    if (Target) {
        /* Higher priority if critical */
        ULONG Priority = (Pressure >= MODERATE_PRESSURE) ? 1 : 0;

        Status = MiMemoryConsumers[Consumer].Trim(Target, Priority, &NrFreedPages);

        if (Pressure >= HIGH_PRESSURE) {
            DPRINT("SmartTrim[%lu]: freed %lu/%lu pages, pressure %lu%%, priority %lu\n",
                   Consumer, NrFreedPages, Target, Pressure, Priority);
        }

        if (!NT_SUCCESS(Status)) {
            DPRINT1("Trim consumer %lu failed: 0x%08X\n", Consumer, Status);
        }

        MiCacheStats.TrimCount++;
        MiCacheStats.LastTrimSize = NrFreedPages;
    }

    return (InitialTarget > NrFreedPages) ? (InitialTarget - NrFreedPages) : 0;
}

/**
 * @brief Ultra-smart user memory trimming
 * Tracks accessed bits carefully, removes only truly cold pages
 * On 256MB, we MUST be smart about what we page out
 */
NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
    PFN_NUMBER FirstPage, CurrentPage;
    NTSTATUS Status;
    ULONG Trimmed = 0;
    ULONG Pressure = MiGetMemoryPressure();

    (*NrFreedPages) = 0;

    DPRINT("UserTrim: target=%lu pages, priority=%lu, pressure=%lu%%\n", Target, Priority, Pressure);

    FirstPage = MmGetLRUFirstUserPage();
    CurrentPage = FirstPage;

    while (CurrentPage != 0 && Target > 0) {
        if (Priority) {
            /*
             * EMERGENCY MODE: Page out regardless
             * We're out of memory, must act NOW
             */
            Status = MmPageOutPhysicalAddress(CurrentPage);
            if (NT_SUCCESS(Status)) {
                Target--;
                (*NrFreedPages)++;
                Trimmed++;
                if (CurrentPage == FirstPage)
                    FirstPage = 0;
            }
        } else {
            /*
             * SMART MODE: Only remove pages that are truly cold
             * (not accessed since last check)
             */
            PEPROCESS Process = NULL;
            PVOID Address = NULL;
            BOOLEAN Accessed = FALSE;
            ULONG AccessedCount = 0;

            while (TRUE) {
                KAPC_STATE ApcState;
                KIRQL OldIrql = MiAcquirePfnLock();
                PMM_RMAP_ENTRY Entry = MmGetRmapListHeadPage(CurrentPage);

                while (Entry) {
                    if (RMAP_IS_SEGMENT(Entry->Address)) {
                        Entry = Entry->Next;
                        continue;
                    }

                    if (Entry->Address < Address) {
                        Entry = Entry->Next;
                        continue;
                    }

                    if ((Entry->Address == Address) && (Entry->Process <= Process)) {
                        Entry = Entry->Next;
                        continue;
                    }

                    break;
                }

                if (!Entry) {
                    MiReleasePfnLock(OldIrql);
                    break;
                }

                Process = Entry->Process;
                Address = Entry->Address;

                ObReferenceObject(Process);

                if (!ExAcquireRundownProtection(&Process->RundownProtect)) {
                    ObDereferenceObject(Process);
                    MiReleasePfnLock(OldIrql);
                    continue;
                }

                MiReleasePfnLock(OldIrql);

                KeStackAttachProcess(&Process->Pcb, &ApcState);
                MiLockProcessWorkingSet(Process, PsGetCurrentThread());

                if (MmIsAddressValid(Address)) {
                    PMMPTE Pte = MiAddressToPte(Address);
                    Accessed = Accessed || Pte->u.Hard.Accessed;
                    
                    /* Count consecutive accesses for decay */
                    if (Pte->u.Hard.Accessed)
                        AccessedCount++;
                    
                    /* Reset the accessed bit */
                    Pte->u.Hard.Accessed = 0;
                }

                MiUnlockProcessWorkingSet(Process, PsGetCurrentThread());
                KeUnstackDetachProcess(&ApcState);
                ExReleaseRundownProtection(&Process->RundownProtect);
                ObDereferenceObject(Process);
            }

            /*
             * SMART DECISION:
             * - If accessed, keep it (it's hot!)
             * - If not accessed but under moderate pressure, trim
             * - If not accessed and pressure is low, don't bother
             */
            if (!Accessed && Pressure >= MODERATE_PRESSURE) {
                Status = MmPageOutPhysicalAddress(CurrentPage);
                if (NT_SUCCESS(Status)) {
                    Target--;
                    (*NrFreedPages)++;
                    Trimmed++;
                    if (CurrentPage == FirstPage)
                        FirstPage = 0;
                }
            } else if (Accessed) {
                MiCacheStats.HitCount++;  /* Cache hit: page was used */
            } else {
                MiCacheStats.MissCount++; /* Cache miss: cold page kept */
            }

            Target--;
        }

        CurrentPage = MmGetLRUNextUserPage(CurrentPage, TRUE);
        if (FirstPage == 0) {
            FirstPage = CurrentPage;
        } else if (CurrentPage == FirstPage) {
            DPRINT("UserTrim: cycled through LRU list, trimmed %lu pages\n", Trimmed);
            break;
        }
    }

    if (CurrentPage) {
        KIRQL OldIrql = MiAcquirePfnLock();
        MmDereferencePage(CurrentPage);
        MiReleasePfnLock(OldIrql);
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID)
{
    if (InterlockedCompareExchange(&PageOutThreadActive, 1, 0) == 0) {
        KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
    }
}

VOID
NTAPI
MmRebalanceMemoryConsumersAndWait(VOID)
{
    KeResetEvent(&MiBalancerDoneEvent);
    MmRebalanceMemoryConsumers();
    KeWaitForSingleObject(&MiBalancerDoneEvent, Executive, KernelMode, FALSE, NULL);
}

/**
 * @brief Adaptive page allocation with smart delays
 * i3 3rd Gen handles small frequent delays well
 */
NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait, PPFN_NUMBER AllocatedPage)
{
    PFN_NUMBER Page;
    static INT AllocationCounter = 0;
    static LARGE_INTEGER Delay = {{0, 0}};
    ULONG Pressure = MiGetMemoryPressure();

    /*
     * SMART ALLOCATION DELAYS:
     * Based on memory pressure, add small delays to give the balancer
     * thread time to free up memory without starving the system
     */

    if (Pressure >= CRITICAL_PRESSURE) {
        if ((AllocationCounter++ % 16) == 0) {  /* Every 16 allocations */
            Delay.QuadPart = -ALLOCATION_DELAY_CRITICAL_MS * 10000;
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        }
    } else if (Pressure >= EMERGENCY_PRESSURE) {
        if ((AllocationCounter++ % 24) == 0) {  /* Every 24 allocations */
            Delay.QuadPart = -ALLOCATION_DELAY_EMERGENCY_MS * 10000;
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        }
    } else if (Pressure >= HIGH_PRESSURE) {
        if ((AllocationCounter++ % 48) == 0) {  /* Every 48 allocations */
            Delay.QuadPart = -ALLOCATION_DELAY_HIGH_MS * 10000;
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        }
    } else if (Pressure >= MODERATE_PRESSURE) {
        if ((AllocationCounter++ % 96) == 0) {  /* Every 96 allocations */
            Delay.QuadPart = -ALLOCATION_DELAY_NORMAL_MS * 10000;
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        }
    } else {
        AllocationCounter = 0;  /* Reset in low-pressure state */
    }

    /* Allocate the page */
    Page = MmAllocPage(Consumer);
    if (Page == 0) {
        if (CanWait && MmAvailablePages < (MiMinimumAvailablePages / 2)) {
            /* Force immediate balancer action if critical */
            MmRebalanceMemoryConsumers();
        }
        *AllocatedPage = 0;
        return STATUS_NO_MEMORY;
    }

    *AllocatedPage = Page;
    InterlockedIncrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
    UpdateTotalCommittedPages(1);

    return STATUS_SUCCESS;
}

VOID CcRosTrimCache(_In_ ULONG Target, _Out_ PULONG NrFreed);

/**
 * @brief Main balancer thread - super responsive and adaptive
 */
VOID
NTAPI
MiBalancerThread(PVOID Unused)
{
    PVOID WaitObjects[2];
    NTSTATUS Status;
    LARGE_INTEGER Interval;
    ULONG Pressure;

    WaitObjects[0] = &MiBalancerEvent;
    WaitObjects[1] = &MiBalancerTimer;

    while (TRUE) {
        KeSetEvent(&MiBalancerDoneEvent, IO_NO_INCREMENT, FALSE);

        Status = KeWaitForMultipleObjects(_countof(WaitObjects),
                                          WaitObjects,
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          NULL);

        if (Status == STATUS_WAIT_0 || Status == STATUS_WAIT_1) {
            ULONG InitialTarget = 0;
            ULONG Target = 0;
            ULONG NrFreedPages = 0;
            ULONG IterationCount = 0;

            Pressure = MiGetMemoryPressure();

            /* Balancing loop - trim until we reach safety margins */
            do {
                ULONG OldTarget = InitialTarget;

                /* Trim all memory consumers */
                for (ULONG i = 0; i < MC_MAXIMUM; i++) {
                    InitialTarget = MiTrimMemoryConsumer(i, InitialTarget);
                }

                /* Aggressively trim cache */
                Target = max(InitialTarget, abs((LONG)MiMinimumAvailablePages - (LONG)MmAvailablePages));
                if (Target) {
                    CcRosTrimCache(Target, &NrFreedPages);
                    InitialTarget -= min(NrFreedPages, InitialTarget);
                }

                if (InitialTarget != 0 && InitialTarget == OldTarget) {
                    IterationCount++;
                    if (IterationCount >= 2) {
                        DPRINT("Balancer: No progress after iterations, remaining target=%lu\n", InitialTarget);
                        break;
                    }
                }
                else {
                    IterationCount = 0;
                }
            } while (InitialTarget != 0);

            /* Calculate next optimal polling interval */
            Pressure = MiGetMemoryPressure();
            ULONG NextInterval = MiGetOptimalPollInterval(Pressure);

            /* Set timer with adaptive interval */
            Interval.QuadPart = -NextInterval * 10000LL;
            KeSetTimerEx(&MiBalancerTimer, Interval, NextInterval, NULL);

            if (Pressure >= EMERGENCY_PRESSURE) {
                DPRINT("Balancer cycle: Pressure %lu%%, next poll %lu ms\n", Pressure, NextInterval);
            }

            if (Status == STATUS_WAIT_0) {
                LONG Active = InterlockedExchange(&PageOutThreadActive, 0);
                ASSERT(Active == 1);
            }
        } else {
            DPRINT1("KeWaitForMultipleObjects failed: 0x%08X\n", Status);
            KeDelayExecutionThread(KernelMode, FALSE, &(LARGE_INTEGER){{-500000, -1}});
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiInitBalancerThread(VOID)
{
    KPRIORITY Priority;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    KeInitializeEvent(&MiBalancerEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&MiBalancerDoneEvent, SynchronizationEvent, FALSE);
    KeInitializeTimerEx(&MiBalancerTimer, SynchronizationTimer);

    /* Start with normal interval, will adapt */
    Timeout.QuadPart = -NORMAL_POLL_MS * 10000LL;
    KeSetTimerEx(&MiBalancerTimer, Timeout, NORMAL_POLL_MS, NULL);

    Status = PsCreateSystemThread(&MiBalancerThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &MiBalancerThreadId,
                                  MiBalancerThread,
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        DPRINT1("Failed to create balancer thread: 0x%08X\n", Status);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /* Set high realtime priority for responsive balancing */
    Priority = LOW_REALTIME_PRIORITY + 1;
    NtSetInformationThread(MiBalancerThreadHandle,
                           ThreadPriority,
                           &Priority,
                           sizeof(Priority));

    DPRINT("✓ Memory balancer initialized: AGGRESSIVE SMART CACHING ACTIVE\n");
}

/* EOF */