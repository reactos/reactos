/* Simple test of EngAcquireSemaphore only check if we got a lock or not */
INT
Test_EngCreateSemaphore(PTESTINFO pti)
{

    HSEMAPHORE hsem;
    hsem = EngCreateSemaphore();

    RTEST ( hsem != NULL );

    EngDeleteSemaphore(hsem);

    return APISTATUS_NORMAL;
}

