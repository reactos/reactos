INT
Test_GdiConvertBitmap(PTESTINFO pti)
{
    RTEST(GdiConvertBitmap((HDC)-1) == (HDC)-1);
    RTEST(GdiConvertBitmap((HDC)0) == (HDC)0);
    RTEST(GdiConvertBitmap((HDC)1) == (HDC)1);
    RTEST(GdiConvertBitmap((HDC)2) == (HDC)2);
    return APISTATUS_NORMAL;
}

