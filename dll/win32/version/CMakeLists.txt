
add_definitions(-D__WINESRC__)
spec2def(version.dll version.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    version.c
    ${CMAKE_CURRENT_BINARY_DIR}/version.def)

add_library(version MODULE ${SOURCE} version.rc)
set_module_type(version win32dll)
target_link_libraries(version wine oldnames)
add_importlibs(version msvcrt kernel32 ntdll)
add_cd_file(TARGET version DESTINATION reactos/system32 FOR all)

if(MSVC)
    if(ARCH STREQUAL "amd64")
        # Disable warning "conversion from 'size_t' to 'DWORD', possible loss of data" for Wine code
        target_compile_options(version PRIVATE "/wd4267")
    endif()
endif()
