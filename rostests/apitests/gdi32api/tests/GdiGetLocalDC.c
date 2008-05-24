INT
Test_GdiGetLocalDC(PTESTINFO pti)
{
    RTEST(GdiGetLocalDC((HDC)-1) == (HDC)-1);
    RTEST(GdiGetLocalDC((HDC)0) == (HDC)0);
    RTEST(GdiGetLocalDC((HDC)1) == (HDC)1);
    RTEST(GdiGetLocalDC((HDC)2) == (HDC)2);
    RTEST(GdiGetLocalDC((HDC)3) == (HDC)3);
    RTEST(GdiGetLocalDC((HDC)4) == (HDC)4);
    return APISTATUS_NORMAL;
}
