INT
Test_GdiConvertPalette(PTESTINFO pti)
{
    RTEST(GdiConvertPalette((HPALETTE)-1) == (HPALETTE)-1);
    RTEST(GdiConvertPalette((HPALETTE)0) == (HPALETTE)0);
    RTEST(GdiConvertPalette((HPALETTE)1) == (HPALETTE)1);
    RTEST(GdiConvertPalette((HPALETTE)2) == (HPALETTE)2);
    return APISTATUS_NORMAL;
}
