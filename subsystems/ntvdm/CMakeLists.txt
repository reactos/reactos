
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/fast486
    ntvdm)

spec2def(ntvdm.exe ntvdm.spec)

list(APPEND SOURCE
    bios/bios.c
    hardware/cmos.c
    hardware/pic.c
    hardware/ps2.c
    hardware/speaker.c
    hardware/timer.c
    hardware/vga.c
    dos/dos.c
    bop.c
    emulator.c
    int32.c
    io.c
    registers.c
    vddsup.c
    ntvdm.c
    ntvdm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntvdm.def)

add_executable(ntvdm ${SOURCE})
set_module_type(ntvdm win32cui UNICODE)
target_link_libraries(ntvdm fast486)
add_importlibs(ntvdm user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntvdm DESTINATION reactos/system32 FOR all)
