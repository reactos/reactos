
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fast486)

spec2def(ntvdm.exe ntvdm.spec)

list(APPEND SOURCE
    bios.c
    bop.c
    cmos.c
    dos.c
    emulator.c
    int32.c
    pic.c
    ps2.c
    registers.c
    speaker.c
    timer.c
    vga.c
    ntvdm.c
    ntvdm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntvdm.def)

add_executable(ntvdm ${SOURCE})
set_module_type(ntvdm win32cui UNICODE)
target_link_libraries(ntvdm fast486)
add_importlibs(ntvdm msvcrt user32 gdi32 kernel32 ntdll)
add_cd_file(TARGET ntvdm DESTINATION reactos/system32 FOR all)
