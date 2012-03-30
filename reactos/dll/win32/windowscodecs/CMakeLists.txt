
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libjpeg
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libpng
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libtiff)
set_rc_compiler()

if(MSVC)
    add_compile_flags("/FItypeof.h")
endif()

spec2def(windowscodecs.dll windowscodecs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    bmpdecode.c
    bmpencode.c
    clsfactory.c
    converter.c
    fliprotate.c
    gifformat.c
    icoformat.c
    imgfactory.c
    info.c
    jpegformat.c
    main.c
    palette.c
    pngformat.c
    propertybag.c
    regsvr.c
    stream.c
    tiffformat.c
    ungif.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/windowscodecs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/windowscodecs.def)

add_library(windowscodecs SHARED ${SOURCE})

set_module_type(windowscodecs win32dll)

target_link_libraries(windowscodecs wine uuid)

add_importlibs(windowscodecs ole32 shlwapi advapi32 msvcrt kernel32 ntdll)


add_cd_file(TARGET windowscodecs DESTINATION reactos/system32 FOR all)
