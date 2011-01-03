
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    AddFontResource.c
    AddFontResourceEx.c
    BeginPath.c
    CreateBitmapIndirect.c
    CreateCompatibleDC.c
    CreateFont.c
    CreateFontIndirect.c
    CreatePen.c
    CreateRectRgn.c
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
    GetDIBits.c
    GetObject.c
    GetStockObject.c
    GetTextExtentExPoint.c
    GetTextFace.c
    SelectObject.c
    SetDCPenColor.c
    SetMapMode.c
    SetSysColors.c
    SetWindowExtEx.c
    SetWorldTransform.c
    testlist.c)

add_executable(gdi32_apitest ${SOURCE})
target_link_libraries(gdi32_apitest wine ${PSEH_LIB})
set_module_type(gdi32_apitest win32cui)
add_importlibs(gdi32_apitest gdi32 user32 msvcrt kernel32 ntdll)
add_cab_target(gdi32_apitest 7)
