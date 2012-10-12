
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers/ndisuio)
add_executable(wlanconf wlanconf.c wlanconf.rc)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(NOT MSVC)
    #add_target_compile_flags(wlanconf "-Wno-format")
    set_source_files_properties(wlanconf.c PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()

set_module_type(wlanconf win32cui)
add_importlibs(wlanconf msvcrt user32 iphlpapi kernel32)
add_cd_file(TARGET wlanconf DESTINATION reactos/system32 FOR all)
