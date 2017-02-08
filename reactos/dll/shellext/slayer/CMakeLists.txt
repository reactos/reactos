
spec2def(slayer.dll slayer.spec)

list(APPEND SOURCE
    slayer.c
    slayer.rc
    ${CMAKE_CURRENT_BINARY_DIR}/slayer.def)

add_library(slayer SHARED ${SOURCE})
set_module_type(slayer win32dll UNICODE)
target_link_libraries(slayer uuid)
add_importlibs(slayer user32 comctl32 advapi32 ole32 shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET slayer DESTINATION reactos/system32 FOR all)
