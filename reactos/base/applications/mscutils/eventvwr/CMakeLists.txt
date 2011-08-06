
set_unicode()
set_rc_compiler()

add_executable(eventvwr eventvwr.c eventvwr.rc)

set_module_type(eventvwr win32gui)
add_importlibs(eventvwr user32 comctl32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(eventvwr ntdll)
endif()

add_cd_file(TARGET eventvwr DESTINATION reactos/system32 FOR all)
