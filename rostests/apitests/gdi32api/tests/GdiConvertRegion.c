INT
Test_GdiConvertRegion(PTESTINFO pti)
{
    RTEST(GdiConvertRegion((HRGN)-1) == (HRGN)-1);
    RTEST(GdiConvertRegion((HRGN)0) == (HRGN)0);
    RTEST(GdiConvertRegion((HRGN)1) == (HRGN)1);
    RTEST(GdiConvertRegion((HRGN)2) == (HRGN)2);
    RTEST(GdiConvertRegion((HRGN)3) == (HRGN)3);
    RTEST(GdiConvertRegion((HRGN)4) == (HRGN)4);
    return APISTATUS_NORMAL;
}

