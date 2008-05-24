INT
Test_GdiGetLocalBrush(PTESTINFO pti)
{
    RTEST(GdiGetLocalBrush((HBRUSH)-1) == (HBRUSH)-1);
    RTEST(GdiGetLocalBrush((HBRUSH)0) == (HBRUSH)0);
    RTEST(GdiGetLocalBrush((HBRUSH)1) == (HBRUSH)1);
    RTEST(GdiGetLocalBrush((HBRUSH)2) == (HBRUSH)2);
    RTEST(GdiGetLocalBrush((HBRUSH)3) == (HBRUSH)3);
    RTEST(GdiGetLocalBrush((HBRUSH)4) == (HBRUSH)4);
    return APISTATUS_NORMAL;
}
