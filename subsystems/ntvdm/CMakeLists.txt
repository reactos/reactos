
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/softx86)

spec2def(ntvdm.exe ntvdm.spec)

list(APPEND SOURCE
    bios.c
    dos.c
    emulator.c
    pic.c
    registers.c
    timer.c
    ps2.c
    vga.c
    ntvdm.c
    ntvdm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntvdm.def)

add_executable(ntvdm ${SOURCE})
set_module_type(ntvdm win32cui UNICODE)
target_link_libraries(ntvdm softx86 softx87)
add_importlibs(ntvdm msvcrt user32 gdi32 kernel32 ntdll)
add_dependencies(ntvdm softx86 softx87)
add_cd_file(TARGET ntvdm DESTINATION reactos/system32 FOR all)
