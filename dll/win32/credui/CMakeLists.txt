
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(credui.dll credui.spec)

list(APPEND SOURCE
    credui_main.c
    credui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/credui_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/credui.def)

add_library(credui SHARED ${SOURCE})

set_module_type(credui win32dll)

target_link_libraries(credui wine)

add_importlibs(credui advapi32 user32 comctl32 msvcrt kernel32 ntdll)
add_importlib_target(credui.spec)
add_cd_file(TARGET credui DESTINATION reactos/system32 FOR all)
