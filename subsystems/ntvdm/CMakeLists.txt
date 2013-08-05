
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/softx86)

list(APPEND SOURCE
    bios.c
    dos.c
    emulator.c
    pic.c
    timer.c
    ps2.c
    vga.c
    ntvdm.c
    ntvdm.rc)

add_executable(ntvdm ${SOURCE})
set_module_type(ntvdm win32cui UNICODE)
target_link_libraries(ntvdm softx86 softx87)
add_importlibs(ntvdm msvcrt user32 kernel32 ntdll)
add_dependencies(ntvdm softx86 softx87)
add_cd_file(TARGET ntvdm DESTINATION reactos/system32 FOR all)
