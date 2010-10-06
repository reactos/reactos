
set_unicode()

add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(ntmarta ${CMAKE_CURRENT_SOURCE_DIR}/ntmarta.spec ${CMAKE_CURRENT_BINARY_DIR}/ntmarta.def)

add_library(ntmarta SHARED ntmarta.c ntmarta.rc)

set_module_type(ntmarta win32dll)

target_link_libraries(ntmarta
    ${CMAKE_CURRENT_BINARY_DIR}/ntmarta.def)

add_importlibs(ntmarta
    user32
    advapi32
    kernel32
    ntdll)

add_dependencies(ntmarta ntmarta_def)
