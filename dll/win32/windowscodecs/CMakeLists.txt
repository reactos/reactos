
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DENTRY_PREFIX=WIC_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libjpeg
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libpng
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libtiff)

spec2def(windowscodecs.dll windowscodecs.spec ADD_IMPORTLIB)
add_rpcproxy_files(windowscodecs_wincodec.idl)

list(APPEND SOURCE
    bmpdecode.c
    bmpencode.c
    clipper.c
    clsfactory.c
    colorcontext.c
    colortransform.c
    converter.c
    fliprotate.c
    gifformat.c
    icnsformat.c
    icoformat.c
    imgfactory.c
    info.c
    jpegformat.c
    main.c
    metadatahandler.c
    metadataquery.c
    palette.c
    pngformat.c
    propertybag.c
    proxy.c
    regsvr.c
    scaler.c
    stream.c
    tgaformat.c
    tiffformat.c
    ungif.c)

if(MSVC)
    if(ARCH STREQUAL "i386")
        list(APPEND SOURCE msvc-thiscall.c)
    endif()
    set_source_files_properties(bitmap.c PROPERTIES COMPILE_OPTIONS "/FImsvc.h")
    list(APPEND PCH_SKIP_SOURCE bitmap.c)
else()
    list(APPEND SOURCE bitmap.c)
endif()

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/windowscodecs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/windowscodecs_wincodec_p.c)

add_library(windowscodecs MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/windowscodecs.def)

if(MSVC)
    # Disable warning C4133: 'function': incompatible types - from 'WICPixelFormatNumericRepresentation *' to 'DWORD *'
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    target_compile_options(windowscodecs PRIVATE /wd4133 /wd4146)

    target_compile_options(windowscodecs PRIVATE /FItypeof.h)
endif()

set_module_type(windowscodecs win32dll)
target_link_libraries(windowscodecs wine uuid ${PSEH_LIB})
add_importlibs(windowscodecs ole32 oleaut32 rpcrt4 shlwapi user32 gdi32 advapi32 advapi32_vista propsys msvcrt kernel32 ntdll)
add_pch(windowscodecs precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET windowscodecs DESTINATION reactos/system32 FOR all)
