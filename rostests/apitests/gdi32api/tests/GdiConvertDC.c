INT
Test_GdiConvertDC(PTESTINFO pti)
{
    RTEST(GdiConvertDC((HDC)-1) == (HDC)-1);
    RTEST(GdiConvertDC((HDC)0) == (HDC)0);
    RTEST(GdiConvertDC((HDC)1) == (HDC)1);
    RTEST(GdiConvertDC((HDC)2) == (HDC)2);
    return APISTATUS_NORMAL;
}

