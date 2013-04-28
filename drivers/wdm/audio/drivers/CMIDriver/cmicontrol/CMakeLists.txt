
include_directories(..)

set_cpp()

add_executable(cmicontrol
    main.cpp
    window.rc
)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(NOT MSVC)
    #add_target_compile_flags(cmicontrol "-fno-exceptions -fno-rtti")
    set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS "-Wno-write-strings")
endif()

add_importlibs(cmicontrol
    user32
    gdi32
    ole32
    advapi32
    comctl32
    shell32
    dsound
    setupapi
    winmm
    msvcrt
    kernel32
)

set_module_type(cmicontrol win32gui)

add_cd_file(TARGET cmicontrol DESTINATION reactos/system32/drivers FOR all)