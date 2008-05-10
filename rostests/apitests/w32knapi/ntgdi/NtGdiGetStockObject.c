
INT
Test_NtGdiGetStockObject(PTESTINFO pti)
{
    HANDLE handle = NULL;

    /* BRUSH testing */
    handle = (HANDLE) NtGdiGetStockObject(WHITE_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(LTGRAY_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(GRAY_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DKGRAY_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(BLACK_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(NULL_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* PEN testing */
    handle = (HANDLE) NtGdiGetStockObject(WHITE_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) != 0);

    handle = (HANDLE) NtGdiGetStockObject(BLACK_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) != 0);

    handle = (HANDLE) NtGdiGetStockObject(NULL_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* Not inuse ? */
    RTEST(NtGdiGetStockObject(9) == 0);

    /* FONT testing */
    handle = (HANDLE) NtGdiGetStockObject(OEM_FIXED_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) != 0);

    handle = (HANDLE) NtGdiGetStockObject(ANSI_FIXED_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(ANSI_VAR_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(SYSTEM_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DEVICE_DEFAULT_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(SYSTEM_FIXED_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DEFAULT_GUI_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* PALETTE testing */
    handle = (HANDLE) NtGdiGetStockObject(DEFAULT_PALETTE);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PALETTE);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* DC testing */
    handle = (HANDLE) NtGdiGetStockObject(DC_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DC_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);


    /*  ? testing */
    handle = (HANDLE) NtGdiGetStockObject(20);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_COLORSPACE);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(21);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BITMAP);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    RTEST(NtGdiGetStockObject(22) == 0);
    RTEST(NtGdiGetStockObject(23) == 0);
    return APISTATUS_NORMAL;
}
