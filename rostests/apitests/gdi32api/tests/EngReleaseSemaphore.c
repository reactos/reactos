/* Simple test of EngAcquireSemaphore only check if we got a lock or not */
INT
Test_EngReleaseSemaphore(PTESTINFO pti)
{

    HSEMAPHORE hsem;
    PRTL_CRITICAL_SECTION lpcrit;

    hsem = EngCreateSemaphore();
    ASSERT(hsem != NULL);

    lpcrit = (PRTL_CRITICAL_SECTION) hsem;

    EngAcquireSemaphore(hsem);
    EngReleaseSemaphore(hsem);

    RTEST (lpcrit->LockCount != 0);
    RTEST (lpcrit->RecursionCount == 0);
    RTEST (lpcrit->OwningThread == 0);
    RTEST (lpcrit->LockSemaphore == 0);
    RTEST (lpcrit->SpinCount == 0);

    ASSERT(lpcrit->DebugInfo != NULL);
    RTEST (lpcrit->DebugInfo->Type == 0);
    RTEST (lpcrit->DebugInfo->CreatorBackTraceIndex == 0);
    RTEST (lpcrit->DebugInfo->EntryCount == 0);
    RTEST (lpcrit->DebugInfo->ContentionCount == 0);

    EngDeleteSemaphore(hsem);

    /* try with deleted Semaphore */
//    EngReleaseSemaphore(hsem);  -> this leads to heap correuption
//    RTEST (lpcrit->LockCount > 0);
//    RTEST (lpcrit->RecursionCount != 0);
//    RTEST (lpcrit->OwningThread == 0);
//    RTEST (lpcrit->LockSemaphore == 0);
//    RTEST (lpcrit->SpinCount == 0);

//    ASSERT(lpcrit->DebugInfo != NULL);
//    RTEST (lpcrit->DebugInfo->Type != 0);
//    RTEST (lpcrit->DebugInfo->CreatorBackTraceIndex != 0);
//    RTEST (lpcrit->DebugInfo->EntryCount != 0);
//    RTEST (lpcrit->DebugInfo->ContentionCount != 0);

    /* NULL pointer test */
    // Note NULL pointer test crash in Vista */
    // EngReleaseSemaphore(NULL);

    /* negtive pointer test */
    // Note negtive pointer test crash in Vista */
    // EngReleaseSemaphore((HSEMAPHORE)-1);


    return APISTATUS_NORMAL;
}

