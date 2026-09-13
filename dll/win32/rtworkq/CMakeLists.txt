
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x603)
spec2def(rtworkq.dll rtworkq.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    queue.c
    ${CMAKE_CURRENT_BINARY_DIR}/rtworkq_stubs.c)

add_library(rtworkq MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/rtworkq.def)

set_module_type(rtworkq win32dll)
target_link_libraries(rtworkq wine)
add_importlibs(rtworkq ole32 crypt32 advapi32 msvcrt kernel32 kernel32_vista ntdll)
add_dependencies(rtworkq psdk)
add_cd_file(TARGET rtworkq DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(rtworkq)
