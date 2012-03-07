
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/wine)

set_rc_compiler()

add_executable(msiexec
    msiexec.c
    service.c
    rsrc.rc)

set_module_type(msiexec win32gui)
target_link_libraries(msiexec uuid wine)
add_importlibs(msiexec user32 advapi32 ole32 msi msvcrt kernel32 ntdll)
add_cd_file(TARGET msiexec DESTINATION reactos/system32 FOR all)
