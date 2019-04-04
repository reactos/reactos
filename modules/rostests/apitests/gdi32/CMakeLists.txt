
include_directories(${REACTOS_SOURCE_DIR}/win32ss)

list(APPEND SOURCE
    AddFontMemResourceEx.c
    AddFontResource.c
    AddFontResourceEx.c
    BeginPath.c
    CombineRgn.c
    CombineTransform.c
    CreateBitmap.c
    CreateBitmapIndirect.c
    CreateCompatibleDC.c
    CreateDIBitmap.c
    CreateDIBPatternBrush.c
    CreateFont.c
    CreateFontIndirect.c
    CreateIconIndirect.c
    CreatePen.c
    CreateRectRgn.c
    DPtoLP.c
    EngAcquireSemaphore.c
    EngCreateSemaphore.c
    EngReleaseSemaphore.c
    EnumFontFamilies.c
    ExcludeClipRect.c
    ExtCreatePen.c
    ExtCreateRegion.c
    FrameRgn.c
    GdiConvertBitmap.c
    GdiConvertBrush.c
    GdiConvertDC.c
    GdiConvertFont.c
    GdiConvertPalette.c
    GdiConvertRegion.c
    GdiDeleteLocalDC.c
    GdiGetCharDimensions.c
    GdiGetLocalBrush.c
    GdiGetLocalDC.c
    GdiReleaseLocalDC.c
    GdiSetAttrs.c
    GetClipBox.c
    GetClipRgn.c
    GetCurrentObject.c
    GetDIBColorTable.c
    GetDIBits.c
    GetFontResourceInfoW.c
    GetGlyphIndices.c
    GetGlyphOutline.c
    GetObject.c
    GetRandomRgn.c
    GetPixel.c
    GetStockObject.c
    GetTextExtentExPoint.c
    GetTextFace.c
    GetTextMetrics.c
    LPtoDP.c
    MaskBlt.c
    NtGdiAddFontResource.c
    OffsetClipRgn.c
    OffsetRgn.c
    PaintRgn.c
    PatBlt.c
    Rectangle.c
    RealizePalette.c
    SelectObject.c
    SetBoundsRect.c
    SetBrushOrgEx.c
    SetDCPenColor.c
    SetDIBits.c
    SetDIBitsToDevice.c
    SetMapMode.c
    SetPixel.c
    SetSysColors.c
    SetWindowExtEx.c
    SetWorldTransform.c
    TextTransform.c
    init.c
    precomp.h)

add_executable(gdi32_apitest ${SOURCE} testlist.c resource.rc)
target_link_libraries(gdi32_apitest ${PSEH_LIB} win32ksys)
set_module_type(gdi32_apitest win32cui)
add_importlibs(gdi32_apitest gdi32 user32 msvcrt kernel32 ntdll)
add_pch(gdi32_apitest precomp.h SOURCE)
add_rostests_file(TARGET gdi32_apitest)
