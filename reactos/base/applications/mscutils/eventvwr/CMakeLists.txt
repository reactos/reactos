
file(GLOB eventvwr_rc_deps res/*.*)
add_rc_deps(eventvwr.rc ${eventvwr_rc_deps})
add_executable(eventvwr eventvwr.c eventvwr.rc)
set_module_type(eventvwr win32gui UNICODE)
add_importlibs(eventvwr user32 comctl32 comdlg32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(eventvwr ntdll)
endif()

add_cd_file(TARGET eventvwr DESTINATION reactos/system32 FOR all)
