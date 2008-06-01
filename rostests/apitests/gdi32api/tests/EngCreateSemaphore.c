/* Simple test of EngAcquireSemaphore only check if we got a lock or not */
INT
Test_EngCreateSemaphore(PTESTINFO pti)
{

    HSEMAPHORE hsem;
    PRTL_CRITICAL_SECTION lpcrit;

    hsem = EngCreateSemaphore();
    RTEST ( hsem != NULL );
    ASSERT(hsem != NULL);

    lpcrit = (PRTL_CRITICAL_SECTION) hsem;
    RTEST ( lpcrit->DebugInfo != NULL);
    RTEST (lpcrit->LockCount == -1);
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

    RTEST (lpcrit->DebugInfo != NULL);
    RTEST (lpcrit->LockCount > 0);
    RTEST (lpcrit->RecursionCount == 0);
    RTEST (lpcrit->OwningThread == 0);
    RTEST (lpcrit->LockSemaphore == 0);
    RTEST (lpcrit->SpinCount == 0);

    ASSERT(lpcrit->DebugInfo != NULL);
    // my (magnus olsen) value I getting back in vista RTEST (lpcrit->DebugInfo->Type == 0xA478);
    RTEST (lpcrit->DebugInfo->Type != 0);
    RTEST (lpcrit->DebugInfo->CreatorBackTraceIndex != 0);
    RTEST (lpcrit->DebugInfo->EntryCount != 0);
    // my (magnus olsen) value I getting back RTEST in vista (lpcrit->DebugInfo->ContentionCount == 0x20000);
    RTEST (lpcrit->DebugInfo->ContentionCount != 0);

    return APISTATUS_NORMAL;
}

