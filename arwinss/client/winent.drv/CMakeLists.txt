

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/arwinss/include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(winent.drv winent.drv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    bitblt.c
    clipboard.c
    dib.c
    event.c
    font.c
    gdidrv.c
    gdiobj.c
    graphics.c
    main.c
    userdrv.c
    mouse.c
    window.c
    ogldrv.c

    winent.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winent_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winent.def)

add_library(winent SHARED ${SOURCE})
set_module_type(winent win32dll UNICODE)
set_target_properties(winent PROPERTIES SUFFIX ".drv")

target_link_libraries(winent
    win32ksys
    dxguid
    wine
    ${PSEH_LIB})

add_importlibs(winent gdi32 user32 imm32 msvcrt kernel32 ntdll)
#add_pch(winent include/precomp.h SOURCE)
add_cd_file(TARGET winent DESTINATION reactos/system32 FOR all)
