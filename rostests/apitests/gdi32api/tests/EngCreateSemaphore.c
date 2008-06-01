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

    EngDeleteSemaphore(hsem);

    return APISTATUS_NORMAL;
}

