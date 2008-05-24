INT
Test_GdiDeleteLocalDC(PTESTINFO pti)
{
    RTEST(GdiDeleteLocalDC((HDC)-1) == TRUE);
    RTEST(GdiDeleteLocalDC((HDC)0) == TRUE);
    RTEST(GdiDeleteLocalDC((HDC)1) == TRUE);
    RTEST(GdiDeleteLocalDC((HDC)2) == TRUE);
    return APISTATUS_NORMAL;
}
