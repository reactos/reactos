
add_definitions(
    -DWIN32
    -DNDEBUG
    -D_WINDOWS
    -DPNG_BUILD_DLL)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libpng
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib)

add_library(libpng SHARED
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
    pngwutil.c)

set_entrypoint(libpng 0)
target_link_libraries(libpng zlib)
add_importlibs(libpng msvcrt kernel32 ntdll)
add_dependencies(libpng psdk)
add_cd_file(TARGET libpng DESTINATION reactos/system32 FOR all)
