INT
Test_GdiSetAttrs(PTESTINFO pti)
{
    RTEST(GdiSetAttrs((HDC)-1) == TRUE);
    RTEST(GdiSetAttrs((HDC)0) == TRUE);
    RTEST(GdiSetAttrs((HDC)1) == TRUE);
    RTEST(GdiSetAttrs((HDC)2) == TRUE);
    return APISTATUS_NORMAL;
}
