
INT
Test_GetSystemMetrics(PTESTINFO pti)
{
    INT ret;
    HDC hDC;
    BOOL BoolVal;
    UINT UintVal;
    RECT rect;

    SetLastError(0);
    hDC = GetDC(0);

    ret = GetSystemMetrics(0);
    TEST(ret > 0);

    ret = GetSystemMetrics(64);
    TEST(ret == 0);
    ret = GetSystemMetrics(65);
    TEST(ret == 0);
    ret = GetSystemMetrics(66);
    TEST(ret == 0);


    ret = GetSystemMetrics(SM_CXSCREEN);
    TEST(ret == GetDeviceCaps(hDC, HORZRES));
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYSCREEN);
    TEST(ret == GetDeviceCaps(hDC, VERTRES));
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXVSCROLL);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYHSCROLL);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYCAPTION);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXBORDER);
    SystemParametersInfoW(SPI_GETFOCUSBORDERWIDTH, 0, &UintVal, 0);
    TEST(ret == UintVal);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYBORDER);
    SystemParametersInfoW(SPI_GETFOCUSBORDERHEIGHT, 0, &UintVal, 0);
    TEST(ret == UintVal);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXDLGFRAME);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYDLGFRAME);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYVTHUMB);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXHTHUMB);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXICON);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYICON);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXCURSOR);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYCURSOR);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMENU);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rect, 0);
    ret = GetSystemMetrics(SM_CXFULLSCREEN);
    TEST(ret == rect.right);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYFULLSCREEN);
    TEST(ret == rect.bottom - rect.top - GetSystemMetrics(SM_CYCAPTION));
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYKANJIWINDOW);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_MOUSEPRESENT);
    TEST(ret == 1);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYVSCROLL);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXHSCROLL);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_DEBUG);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_SWAPBUTTON);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_RESERVED1);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_RESERVED2);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_RESERVED3);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_RESERVED4);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMIN);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMIN);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXSIZE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYSIZE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXFRAME);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYFRAME);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMINTRACK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMINTRACK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXDOUBLECLK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYDOUBLECLK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXICONSPACING);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYICONSPACING);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_MENUDROPALIGNMENT);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_PENWINDOWS);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_DBCSENABLED);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CMOUSEBUTTONS);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

#if(WINVER >= 0x0400)
    ret = GetSystemMetrics(SM_SECURE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXEDGE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYEDGE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMINSPACING);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMINSPACING);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXSMICON);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYSMICON);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYSMCAPTION);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXSMSIZE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYSMSIZE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMENUSIZE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMENUSIZE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_ARRANGE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMINIMIZED);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMINIMIZED);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMAXTRACK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMAXTRACK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMAXIMIZED);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMAXIMIZED);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_NETWORK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CLEANBOOT);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXDRAG);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYDRAG);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_SHOWSOUNDS);
    SystemParametersInfoW(SPI_GETSHOWSOUNDS, 0, &BoolVal, 0);
    TEST(ret == BoolVal);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXMENUCHECK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYMENUCHECK);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_SLOWMACHINE);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_MIDEASTENABLED);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);
#endif

#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
    ret = GetSystemMetrics(SM_MOUSEWHEELPRESENT);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);
#endif

#if(WINVER >= 0x0500)
    ret = GetSystemMetrics(SM_XVIRTUALSCREEN);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_YVIRTUALSCREEN);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXVIRTUALSCREEN);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYVIRTUALSCREEN);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CMONITORS);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_SAMEDISPLAYFORMAT);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);
#endif

#if(_WIN32_WINNT >= 0x0500)
    ret = GetSystemMetrics(SM_IMMENABLED);
    TEST(ret == 0);
    TEST(GetLastError() == 0);
#endif

#if(_WIN32_WINNT >= 0x0501)
    ret = GetSystemMetrics(SM_CXFOCUSBORDER);
    SystemParametersInfoW(SPI_GETFOCUSBORDERWIDTH, 0, &UintVal, 0);
    TEST(ret == UintVal);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CYFOCUSBORDER);
    SystemParametersInfoW(SPI_GETFOCUSBORDERHEIGHT, 0, &UintVal, 0);
    TEST(ret == UintVal);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_TABLETPC);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_MEDIACENTER);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_STARTER);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_SERVERR2);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);
#endif

#if(_WIN32_WINNT >= 0x0600)
    ret = GetSystemMetrics(SM_MOUSEHORIZONTALWHEELPRESENT);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);

    ret = GetSystemMetrics(SM_CXPADDEDBORDER);
//    TEST(ret == 0);
    TEST(GetLastError() == 0);
#endif

    return APISTATUS_NORMAL;
}
