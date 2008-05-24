INT
Test_GdiReleaseLocalDC(PTESTINFO pti)
{
    RTEST(GdiReleaseLocalDC((HDC)-1) == TRUE);
    RTEST(GdiReleaseLocalDC((HDC)0) == TRUE);
    RTEST(GdiReleaseLocalDC((HDC)1) == TRUE);
    RTEST(GdiReleaseLocalDC((HDC)2) == TRUE);
    return APISTATUS_NORMAL;
}
