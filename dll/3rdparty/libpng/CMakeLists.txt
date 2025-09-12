
add_definitions(
    -DWIN32
    -DNDEBUG=
    -D_WINDOWS
    -DPNG_BUILD_DLL)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libpng
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

spec2def(libpng.dll libpng.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    png.c
    pngerror.c
    pngget.c
    pngmem.c
    pngpread.c
    pngread.c
    pngrio.c
    pngrtran.c
    pngrutil.c
    pngset.c
    pngtrans.c
    pngwio.c
    pngwrite.c
    pngwtran.c
    pngwutil.c
    pngpriv.h)

add_library(libpng MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/libpng.def)
set_module_type(libpng win32dll)
target_link_libraries(libpng zlib)
add_importlibs(libpng msvcrt kernel32 ntdll)
add_pch(libpng pngpriv.h SOURCE)
add_cd_file(TARGET libpng DESTINATION reactos/system32 FOR all)
