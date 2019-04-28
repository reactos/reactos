
if(NOT MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
    add_compile_flags("-Wno-format-overflow")
endif()

add_executable(tftpd tftpd.cpp)
set_module_type(tftpd win32cui)
add_importlibs(tftpd advapi32 ws2_32 iphlpapi msvcrt kernel32)
add_cd_file(TARGET tftpd DESTINATION reactos/system32 FOR all)
