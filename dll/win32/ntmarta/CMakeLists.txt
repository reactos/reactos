
set_unicode()

add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_library(ntmarta SHARED ntmarta.c ntmarta.rc ntmarta.def)

if(NOT MSVC)
    set_source_files_properties(ntmarta.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(ntmarta win32dll)

add_importlibs(ntmarta
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cab_target(ntmarta 1)
