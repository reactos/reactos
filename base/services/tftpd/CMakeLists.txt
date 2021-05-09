
add_executable(tftpd tftpd.cpp)
set_module_type(tftpd win32cui)
add_importlibs(tftpd advapi32 ws2_32 iphlpapi msvcrt kernel32)
add_cd_file(TARGET tftpd DESTINATION reactos/system32 FOR all)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tftpd PRIVATE -Wno-format-overflow)
endif()
