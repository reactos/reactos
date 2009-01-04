
INT
Test_CreateFontIndirectA(PTESTINFO pti)
{
    LOGFONTA logfont;
    HFONT hFont;
    ULONG ret;
    ENUMLOGFONTEXDVW elfedv2;

    logfont.lfHeight = 12;
    logfont.lfWidth = 0;
    logfont.lfEscapement = 0;
    logfont.lfOrientation = 0;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 0;
    logfont.lfUnderline = 0;
    logfont.lfStrikeOut = 0;
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = PROOF_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    memset(logfont.lfFaceName, 'A', LF_FACESIZE);
    hFont = CreateFontIndirectA(&logfont);
    TEST(hFont != 0);

    memset(&elfedv2, 0, sizeof(elfedv2));
    ret = GetObjectW(hFont, sizeof(elfedv2), &elfedv2);
    TEST(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD));
    TEST(elfedv2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == 0);
    TEST(elfedv2.elfEnumLogfontEx.elfFullName[0] == 0);

    return APISTATUS_NORMAL;
}

INT
Test_CreateFontIndirectW(PTESTINFO pti)
{
    LOGFONTW logfont;
    HFONT hFont;
    ULONG ret;
    ENUMLOGFONTEXDVW elfedv2;

    logfont.lfHeight = 12;
    logfont.lfWidth = 0;
    logfont.lfEscapement = 0;
    logfont.lfOrientation = 0;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 0;
    logfont.lfUnderline = 0;
    logfont.lfStrikeOut = 0;
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = PROOF_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    memset(logfont.lfFaceName, 'A', LF_FACESIZE * 2);
    hFont = CreateFontIndirectW(&logfont);
    TEST(hFont != 0);

    memset(&elfedv2, 0, sizeof(elfedv2));
    ret = GetObjectW(hFont, sizeof(elfedv2), &elfedv2);
    TEST(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD));
    TEST(elfedv2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == ((WCHAR)'A' << 8) + 'A');
    TEST(elfedv2.elfEnumLogfontEx.elfFullName[0] == 0);
    /* Theres a bunch of data in elfFullName ... */

    return APISTATUS_NORMAL;
}

INT
Test_CreateFontIndirectExA(PTESTINFO pti)
{
    ENUMLOGFONTEXDVA elfedva, elfedva2;
    ENUMLOGFONTEXDVW elfedvw;
    ENUMLOGFONTEXA *penumlfa;
    LOGFONTA *plogfonta;
    HFONT hFont;
    ULONG ret;
 
    memset(&elfedva, 0, sizeof(elfedva));
    penumlfa = &elfedva.elfEnumLogfontEx;
    plogfonta = &elfedva.elfEnumLogfontEx.elfLogFont;

    plogfonta->lfHeight = 12;
    plogfonta->lfWidth = 0;
    plogfonta->lfEscapement = 0;
    plogfonta->lfOrientation = 0;
    plogfonta->lfWeight = FW_NORMAL;
    plogfonta->lfItalic = 0;
    plogfonta->lfUnderline = 0;
    plogfonta->lfStrikeOut = 0;
    plogfonta->lfCharSet = DEFAULT_CHARSET;
    plogfonta->lfOutPrecision = OUT_DEFAULT_PRECIS;
    plogfonta->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    plogfonta->lfQuality = PROOF_QUALITY;
    plogfonta->lfPitchAndFamily = DEFAULT_PITCH;

    memset(plogfonta->lfFaceName, 'A', LF_FACESIZE * sizeof(WCHAR));
    memset(penumlfa->elfFullName, 'B', LF_FULLFACESIZE * sizeof(WCHAR));

    hFont = CreateFontIndirectExA(&elfedva);
    TEST(hFont != 0);

    ret = GetObjectW(hFont, sizeof(elfedvw), &elfedvw);
    TEST(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD));
    TEST(elfedvw.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == 0);
    TEST(elfedvw.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == 0);

    memset(&elfedva2, 0, sizeof(elfedva2));
    ret = GetObjectA(hFont, sizeof(elfedva2), &elfedva2);
    TEST(ret == sizeof(ENUMLOGFONTEXDVA));
    TEST(elfedva2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == 0);
    TEST(elfedva2.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == 0);

    return APISTATUS_NORMAL;
}

INT
Test_CreateFontIndirectExW(PTESTINFO pti)
{
    ENUMLOGFONTEXDVW elfedv, elfedv2;
    ENUMLOGFONTEXDVA elfedva;
    ENUMLOGFONTEXW *penumlfw;
    LOGFONTW *plogfontw;
    HFONT hFont;
    ULONG ret;
 
    memset(&elfedv, 0, sizeof(elfedv));
    penumlfw = &elfedv.elfEnumLogfontEx;
    plogfontw = &elfedv.elfEnumLogfontEx.elfLogFont;

    plogfontw->lfHeight = 12;
    plogfontw->lfWidth = 0;
    plogfontw->lfEscapement = 0;
    plogfontw->lfOrientation = 0;
    plogfontw->lfWeight = FW_NORMAL;
    plogfontw->lfItalic = 0;
    plogfontw->lfUnderline = 0;
    plogfontw->lfStrikeOut = 0;
    plogfontw->lfCharSet = DEFAULT_CHARSET;
    plogfontw->lfOutPrecision = OUT_DEFAULT_PRECIS;
    plogfontw->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    plogfontw->lfQuality = PROOF_QUALITY;
    plogfontw->lfPitchAndFamily = DEFAULT_PITCH;

    memset(plogfontw->lfFaceName, 'A', LF_FACESIZE * sizeof(WCHAR));
    memset(penumlfw->elfFullName, 'B', LF_FULLFACESIZE * sizeof(WCHAR));

    hFont = CreateFontIndirectExW(&elfedv);
    TEST(hFont != 0);

    memset(&elfedv2, 0, sizeof(elfedv2));
    ret = GetObjectW(hFont, sizeof(elfedv2), &elfedv2);
    TEST(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD));
    TEST(elfedv2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == ((WCHAR)'A' << 8) + 'A');
    TEST(elfedv2.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == ((WCHAR)'B' << 8) + 'B');

    memset(&elfedva, 0, sizeof(elfedva));
    ret = GetObjectA(hFont, sizeof(elfedva), &elfedva);
    TEST(ret == sizeof(ENUMLOGFONTEXDVA));
    TEST(elfedva.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == '?');
    TEST(elfedva.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == 0);

    return APISTATUS_NORMAL;
}


INT
Test_CreateFontIndirect(PTESTINFO pti)
{

    Test_CreateFontIndirectA(pti);
    Test_CreateFontIndirectW(pti);
    Test_CreateFontIndirectExA(pti);
    Test_CreateFontIndirectExW(pti);


    return APISTATUS_NORMAL;
}
