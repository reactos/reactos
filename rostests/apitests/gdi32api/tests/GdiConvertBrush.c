INT
Test_GdiConvertBrush(PTESTINFO pti)
{
    RTEST(GdiConvertBrush((HBRUSH)-1) == (HBRUSH)-1);
    RTEST(GdiConvertBrush((HBRUSH)0) == (HBRUSH)0);
    RTEST(GdiConvertBrush((HBRUSH)1) == (HBRUSH)1);
    RTEST(GdiConvertBrush((HBRUSH)2) == (HBRUSH)2);
    return APISTATUS_NORMAL;
}
