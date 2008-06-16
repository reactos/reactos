/* Simple test of EngAcquireSemaphore only check if we got a lock or not */
INT
Test_EngAcquireSemaphore(PTESTINFO pti)
{

    HSEMAPHORE hsem;
    PRTL_CRITICAL_SECTION lpcrit;

    hsem = EngCreateSemaphore();
    RTEST ( hsem != NULL );
    ASSERT(hsem != NULL);
    lpcrit = (PRTL_CRITICAL_SECTION) hsem;

    /* real data test */
    EngAcquireSemaphore(hsem);
//    RTEST (lpcrit->LockCount == -2); doesn't work on XP
    RTEST (lpcrit->RecursionCount == 1);
    RTEST (lpcrit->OwningThread != 0);
    RTEST (lpcrit->LockSemaphore == 0);
    RTEST (lpcrit->SpinCount == 0);

    ASSERT(lpcrit->DebugInfo != NULL);
    RTEST (lpcrit->DebugInfo->Type == 0);
    RTEST (lpcrit->DebugInfo->CreatorBackTraceIndex == 0);
    RTEST (lpcrit->DebugInfo->EntryCount == 0);
    RTEST (lpcrit->DebugInfo->ContentionCount == 0);

    EngReleaseSemaphore(hsem);
    EngDeleteSemaphore(hsem);

    /* NULL pointer test */
    // Note NULL pointer test crash in Vista */
    // EngAcquireSemaphore(NULL);

    /* negtive pointer test */
    // Note negtive pointer test crash in Vista */
    // EngAcquireSemaphore((HSEMAPHORE)-1);

    /* try with deleted Semaphore */
    // Note deleted Semaphore pointer test does freze the whole program in Vista */
    // EngAcquireSemaphore(hsem);

    return APISTATUS_NORMAL;
}

