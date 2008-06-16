
INT
Test_EngDeleteSemaphore(PTESTINFO pti)
{

    HSEMAPHORE hsem;
    PRTL_CRITICAL_SECTION lpcrit;

    /* test Create then delete */
    hsem = EngCreateSemaphore();
    ASSERT(hsem != NULL);
    lpcrit = (PRTL_CRITICAL_SECTION) hsem;
    EngDeleteSemaphore(hsem);

//    RTEST (lpcrit->LockCount > 0); doesn't work on XP
    RTEST (lpcrit->RecursionCount == 0);
    RTEST (lpcrit->OwningThread == 0);
    RTEST (lpcrit->LockSemaphore == 0);
    RTEST (lpcrit->SpinCount == 0);

//    ASSERT(lpcrit->DebugInfo != NULL); doesn't work on XP
    RTEST (lpcrit->DebugInfo->Type != 0);
    RTEST (lpcrit->DebugInfo->CreatorBackTraceIndex != 0);
    RTEST (lpcrit->DebugInfo->EntryCount != 0);
    RTEST (lpcrit->DebugInfo->ContentionCount != 0);


    /* test EngAcquireSemaphore and release it, then delete it */
    hsem = EngCreateSemaphore();
    ASSERT(hsem != NULL);
    lpcrit = (PRTL_CRITICAL_SECTION) hsem;

    EngAcquireSemaphore(hsem);
    EngReleaseSemaphore(hsem);
    EngDeleteSemaphore(hsem);

    RTEST (lpcrit->LockCount > 0);
    RTEST (lpcrit->RecursionCount == 0);
    RTEST (lpcrit->OwningThread == 0);
    RTEST (lpcrit->LockSemaphore == 0);
    RTEST (lpcrit->SpinCount == 0);

    ASSERT(lpcrit->DebugInfo != NULL);
    RTEST (lpcrit->DebugInfo->Type != 0);
    RTEST (lpcrit->DebugInfo->CreatorBackTraceIndex != 0);
    RTEST (lpcrit->DebugInfo->EntryCount != 0);
    RTEST (lpcrit->DebugInfo->ContentionCount != 0);

    return APISTATUS_NORMAL;
}

