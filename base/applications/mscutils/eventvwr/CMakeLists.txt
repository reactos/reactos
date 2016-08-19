
list(APPEND SOURCE
    eventvwr.c
    evtdetctl.c
    eventvwr.h)

file(GLOB eventvwr_rc_deps res/*.*)
add_rc_deps(eventvwr.rc ${eventvwr_rc_deps})
add_executable(eventvwr ${SOURCE} eventvwr.rc)
add_pch(eventvwr eventvwr.h SOURCE)
set_module_type(eventvwr win32gui UNICODE)
target_link_libraries(eventvwr ${PSEH_LIB})
add_importlibs(eventvwr user32 gdi32 comctl32 comdlg32 advapi32 shell32 shlwapi msvcrt kernel32 ntdll)

add_cd_file(TARGET eventvwr DESTINATION reactos/system32 FOR all)
