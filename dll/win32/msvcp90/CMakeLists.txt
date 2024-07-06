
spec2def(msvcp90.dll msvcp90.spec ADD_IMPORTLIB)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MT
    -DCRTDLL
    -D__MINGW_IMPORT="")

list(APPEND SOURCE
    details.c
    exception.c
    ios.c
    locale.c
    math.c
    memory.c
    misc.c
    msvcp_main.c
    string.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcp90.def)

add_library(msvcp90 MODULE ${SOURCE})
set_module_type(msvcp90 win32dll)
add_dependencies(msvcp90 psdk)
target_compile_definitions(msvcp90 PRIVATE -D_MSVCP_VER=60)
target_include_directories(msvcp90 PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/wine)
target_link_libraries(msvcp90 wine ${PSEH_LIB})

add_importlibs(msvcp90 msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcp90 DESTINATION reactos/system32 FOR all)
