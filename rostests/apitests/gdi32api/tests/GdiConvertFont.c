INT
Test_GdiConvertFont(PTESTINFO pti)
{
    RTEST(GdiConvertFont((HFONT)-1) == (HFONT)-1);
    RTEST(GdiConvertFont((HFONT)0) == (HFONT)0);
    RTEST(GdiConvertFont((HFONT)1) == (HFONT)1);
    RTEST(GdiConvertFont((HFONT)2) == (HFONT)2);
    return APISTATUS_NORMAL;
}
