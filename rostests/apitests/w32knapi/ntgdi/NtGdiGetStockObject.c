
INT
Test_NtGdiGetStockObject(PTESTINFO pti)
{
    RTEST(NtGdiGetStockObject(WHITE_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(LTGRAY_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(GRAY_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(DKGRAY_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(BLACK_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(NULL_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(WHITE_PEN) != 0);
    RTEST(NtGdiGetStockObject(BLACK_PEN) != 0);
    RTEST(NtGdiGetStockObject(NULL_PEN) != 0);
    RTEST(NtGdiGetStockObject(9) == 0);
    RTEST(NtGdiGetStockObject(OEM_FIXED_FONT) != 0);
    RTEST(NtGdiGetStockObject(ANSI_FIXED_FONT) != 0);
    RTEST(NtGdiGetStockObject(ANSI_VAR_FONT) != 0);
    RTEST(NtGdiGetStockObject(SYSTEM_FONT) != 0);
    RTEST(NtGdiGetStockObject(DEVICE_DEFAULT_FONT) != 0);
    RTEST(NtGdiGetStockObject(DEFAULT_PALETTE) != 0);
    RTEST(NtGdiGetStockObject(SYSTEM_FIXED_FONT) != 0);
    RTEST(NtGdiGetStockObject(DEFAULT_GUI_FONT) != 0);
    RTEST(NtGdiGetStockObject(DC_BRUSH) != 0);
    RTEST(NtGdiGetStockObject(DC_PEN) != 0);
    RTEST(NtGdiGetStockObject(20) != 0);
    RTEST(NtGdiGetStockObject(21) != 0);
    RTEST(NtGdiGetStockObject(22) == 0);
    RTEST(NtGdiGetStockObject(23) == 0);
    return APISTATUS_NORMAL;
}
