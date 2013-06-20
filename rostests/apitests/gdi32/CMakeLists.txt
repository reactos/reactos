
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/win32ss)

list(APPEND SOURCE
    AddFontResource.c
    AddFontResourceEx.c
    BeginPath.c
    CombineRgn.c
    CombineTransform.c
    CreateBitmap.c
    CreateBitmapIndirect.c
    CreateCompatibleDC.c
    CreateDIBitmap.c
    CreateDIBPatternBrush
    CreateFont.c
    CreateFontIndirect.c
    CreateIconIndirect.c
    CreatePen.c
    CreateRectRgn.c
    DPtoLP.c
    EngAcquireSemaphore.c
    EngCreateSemaphore.c
    EngDeleteSemaphore.c
    EngReleaseSemaphore.c
    ExtCreatePen.c
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
    GetClipRgn.c
    GetCurrentObject.c
    GetDIBColorTable.c
    GetDIBits.c
    GetObject.c
    GetRandomRgn.c
    GetPixel.c
    GetStockObject.c
    GetTextExtentExPoint.c
    GetTextFace.c
    MaskBlt.c
    PatBlt.c
    Rectangle.c
    SelectObject.c
    SetBrushOrgEx.c
    SetDCPenColor.c
    SetDIBits.c
    SetDIBitsToDevice.c
    SetMapMode.c
    SetPixel.c
    SetSysColors.c
    SetWindowExtEx.c
    SetWorldTransform.c
    init.c
    testlist.c)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(gdi32_apitest)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

add_executable(gdi32_apitest ${SOURCE})
target_link_libraries(gdi32_apitest wine ${PSEH_LIB})
set_module_type(gdi32_apitest win32cui)
add_importlibs(gdi32_apitest gdi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET gdi32_apitest DESTINATION reactos/bin FOR all)
