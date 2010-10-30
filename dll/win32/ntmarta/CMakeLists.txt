
set_unicode()

add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(ntmarta.dll ntmarta.spec)

add_library(ntmarta SHARED ntmarta.c ntmarta.rc ${CMAKE_CURRENT_BINARY_DIR}/ntmarta.def)

set_module_type(ntmarta win32dll)

add_importlibs(ntmarta
    user32
    advapi32
    kernel32
    ntdll)


add_cab_target(ntmarta 1)
