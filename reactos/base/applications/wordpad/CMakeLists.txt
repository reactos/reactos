
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    olecallback.c
    print.c
    registry.c
    wordpad.c)

add_executable(wordpad
    ${SOURCE}
    rsrc.rc)

set_module_type(wordpad win32gui)
target_link_libraries(wordpad wine uuid)
add_importlibs(wordpad comdlg32 ole32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wordpad DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(wordpad)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()
