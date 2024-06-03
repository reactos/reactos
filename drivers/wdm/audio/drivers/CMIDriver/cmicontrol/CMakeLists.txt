
include_directories(..)

add_executable(cmicontrol main.cpp window.rc)

if(NOT MSVC)
    set_source_files_properties(main.cpp PROPERTIES COMPILE_OPTIONS "-Wno-write-strings")
endif()

add_dependencies(cmicontrol psdk)

add_importlibs(cmicontrol user32 gdi32 ole32 advapi32 comctl32 shell32 dsound setupapi winmm msvcrt kernel32)
set_module_type(cmicontrol win32gui)
add_cd_file(TARGET cmicontrol DESTINATION reactos/system32/drivers FOR all)
