
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/wine)

list(APPEND SOURCE
    msiexec.c
    service.c
    precomp.h)

add_rc_deps(rsrc.rc ${CMAKE_CURRENT_SOURCE_DIR}/msiexec.ico)
add_executable(msiexec ${SOURCE} rsrc.rc)
set_module_type(msiexec win32gui)
target_link_libraries(msiexec uuid wine)
add_importlibs(msiexec user32 advapi32 ole32 msi msvcrt kernel32 ntdll)
add_pch(msiexec precomp.h SOURCE)
add_cd_file(TARGET msiexec DESTINATION reactos/system32 FOR all)
