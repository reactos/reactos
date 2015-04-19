
add_executable(winefile winefile.c rsrc.rc)

set_module_type(winefile win32gui UNICODE)
add_importlibs(winefile advapi32 gdi32 user32 ws2_32 msimg32 version mpr comctl32 comdlg32 ole32 oleaut32 shell32 shlwapi msvcrt kernel32 ntdll)
target_link_libraries(winefile uuid)

add_cd_file(TARGET winefile DESTINATION reactos/system32 FOR all)

#if(NOT MSVC)
#    FIXME: http://www.cmake.org/Bug/view.php?id=12998
#    add_target_compile_flags(winefile "-Wno-format")
#endif()
