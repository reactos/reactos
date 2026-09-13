
add_definitions(
    -DHAVE_ZLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

spec2def(cabinet.dll cabinet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cabinet_main.c
    fci.c
    fdi.c)

list(APPEND PCH_SKIP_SOURCE
    stubs.c)

add_library(cabinet MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    cabinet.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def)

set_module_type(cabinet win32dll)
if(MSVC)
    # Disable warning C4005: 'CHAR_BIT': macro redefinition at sdk\include\vcruntime\limits.h(22)
    target_compile_options(cabinet PRIVATE /wd4005)
endif()
target_link_libraries(cabinet wine zlib)
add_importlibs(cabinet msvcrt kernel32 ntdll)
add_pch(cabinet precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET cabinet DESTINATION reactos/system32 FOR all)
set_wine_module(cabinet)
