
add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(mlang.dll mlang.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    mlang.c
    mlang.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mlang_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mlang.def)

add_library(mlang SHARED ${SOURCE})
set_module_type(mlang win32dll ENTRYPOINT 0)
target_link_libraries(mlang uuid wine)
add_delay_importlibs(mlang oleaut32)

add_importlibs(mlang
    ole32
    user32
    gdi32
    advapi32
    kernel32
    ntdll)


add_dependencies(mlang psdk)
add_cd_file(TARGET mlang DESTINATION reactos/system32 FOR all)
