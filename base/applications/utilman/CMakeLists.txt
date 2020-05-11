
list(APPEND SOURCE
    utilman.c
    precomp.h)

add_rc_deps(utilman.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/utilman.ico)
add_executable(utilman ${SOURCE} utilman.rc)
set_module_type(utilman win32gui UNICODE)
add_importlibs(utilman user32 msvcrt kernel32 ntdll)
add_pch(utilman precomp.h SOURCE)
add_cd_file(TARGET utilman DESTINATION reactos/system32 FOR all)
add_subdirectory(umandlg)
