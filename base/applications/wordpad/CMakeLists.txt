
remove_definitions(-D_WIN32_IE=0x600)

add_definitions(-D__ROS_LONG64__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

if(MSVC)
    # Disable warning C4477 (printf format warnings)
    add_compile_flags("/wd4477")
endif()

list(APPEND SOURCE
    olecallback.c
    print.c
    registry.c
    wordpad.c)

file(GLOB wordpad_rc_deps res/*.*)
add_rc_deps(rsrc.rc ${wordpad_rc_deps})
add_executable(wordpad ${SOURCE} rsrc.rc)
set_module_type(wordpad win32gui)
target_link_libraries(wordpad wine uuid)
add_importlibs(wordpad comdlg32 ole32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wordpad DESTINATION reactos/system32 FOR all)
