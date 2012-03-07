
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(localui.dll localui.spec)

list(APPEND SOURCE
    localui.c
    localui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/localui.def)

add_library(localui SHARED ${SOURCE})

set_module_type(localui win32dll)
target_link_libraries(localui uuid wine)


add_importlibs(localui winspool user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET localui DESTINATION reactos/system32 FOR all)
